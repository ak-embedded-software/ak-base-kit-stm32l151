/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   02/11/2017
 ******************************************************************************
**/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "app_if.h"
#include "app_data.h"
#include "app_dbg.h"

#include "ak.h"
#include "port.h"
#include "message.h"
#include "timer.h"

#include "fifo.h"

#include "sys_ctrl.h"
#include "sys_dbg.h"

#include "task_list.h"
#include "task_list_if.h"

#include "nrf_config.h"
#include "nrf_data.h"
#include "nrf_mac.h"
#include "nrf_phy.h"

#include "hal_nrf.h"

#define MAC_HDR_LEN				(sizeof(nrf_mac_hdr_t))
#define MAX_MAC_PAYLOAD_LEN		(MAX_PHY_PAYLOAD_LEN - MAC_HDR_LEN - sizeof(uint8_t))

#define NWK_PDU_ID_FIFO_SIZE	(NRF_NWK_MSG_POOL_SIZE * 3)

typedef struct {
	uint16_t src_mac_addr;
	uint8_t mac_seq;
	uint8_t frame_num;
	uint8_t frame_seq;
} __AK_PACKETED nrf_mac_hdr_t;

typedef struct {
	nrf_mac_hdr_t hdr;
	uint8_t payload[MAX_MAC_PAYLOAD_LEN];
	uint8_t frame_cs;
} __AK_PACKETED nrf_mac_msg_t;

static nrf_mac_msg_t sending_nrf_mac_frame;

#define SENDING_MAC_FRAME_RETRY_COUNTER_MAX		2
#define SENDING_MAC_FRAME_RETRY_INTERVAL		10	/* send mac_frame retry interval (15 ms) MUST BE > Auto Retransmit Delay (4.4 ms)*/

static uint8_t sending_mac_frame_retry_counter = 0;
static uint8_t sending_mac_sequence = 0;

#define REV_MAC_FRAME_TO_INTERVAL				40 /* (40 ms) receiving mac_frame interval (MUST_BE > SENDING_MAC_FRAME_RETRY_INTERVAL * SENDING_MAC_FRAME_RETRY_COUNTER_MAX + Auto Retransmit Delay) */
static uint8_t receiving_mac_sequence = 0;
static nrf_nwk_pdu_t* receiving_nrf_nwk_frame;

#define SENDING_NRF_NWK_FRAME_RETRY_COUNTER_MAX		2
#define SENDING_NRF_NWK_FRAME_RETRY_INTERVAL		34 /* (34 ms) */
static uint8_t sending_nrf_nwk_frame_retry_counter = 0;
static nrf_nwk_pdu_t* sending_nrf_nwk_frame;

enum mac_send_state_e {
	MAC_SEND_STATE_IDLE,
	MAC_SEND_STATE_SENDING,
};

static mac_send_state_e mac_send_state;
static mac_send_state_e get_mac_send_state();
static void set_mac_send_state(mac_send_state_e);

enum mac_rev_state_e {
	MAC_REV_STATE_IDLE,
	MAC_REV_STATE_RECEIVING,
};

static mac_rev_state_e mac_rev_state;
static mac_rev_state_e get_mac_rev_state();
static void set_mac_rev_state(mac_rev_state_e);

static uint32_t nwk_pdu_id_buf[NWK_PDU_ID_FIFO_SIZE];
static fifo_t nwk_pdu_id_fifo;

static uint8_t calc_frame_cs(uint8_t*, uint16_t);

void task_rf24_mac(ak_msg_t* msg) {
	switch (msg->sig) {

	case AC_RF24_MAC_INIT: {
		NRF_DBG_SIG("AC_RF24_MAC_INIT\n");

		/* init sending nwk_frame FIFO */
		fifo_init(&nwk_pdu_id_fifo, nwk_pdu_id_buf, NWK_PDU_ID_FIFO_SIZE, sizeof(uint32_t));

		/* init MAC sequence */
		sending_mac_sequence = (uint8_t)rand();
		receiving_mac_sequence = (uint8_t)rand();

		/* init seding/receiving state */
		set_mac_send_state(MAC_SEND_STATE_IDLE);
		set_mac_rev_state(MAC_REV_STATE_IDLE);
	}
		break;

	case AC_RF24_MAC_HANDLE_MSG_OUT: {
		NRF_DBG_SIG("AC_RF24_MAC_HANDLE_MSG_OUT\n");
		uint32_t st_nwk_pdu_id;
		memcpy(&st_nwk_pdu_id, get_data_common_msg(msg), sizeof(uint32_t));

		if (get_mac_send_state() == MAC_SEND_STATE_SENDING ||
				get_mac_rev_state() == MAC_REV_STATE_RECEIVING) {
			if (fifo_is_full(&nwk_pdu_id_fifo)) {
				task_post_common_msg(AC_RF24_NWK_ID, AC_RF24_NWK_SEND_MSG_ERR_BUSY, (uint8_t*)&st_nwk_pdu_id, sizeof(uint32_t));
			}
			else {
				fifo_put(&nwk_pdu_id_fifo, (uint8_t*)&st_nwk_pdu_id);
			}
		}
		else {
			/* increase sending_mac_sequence */
			sending_mac_sequence ++;

			/* get nwk_frame from nwk_frame pool */
			sending_nrf_nwk_frame = nrf_nwk_pdu_get(st_nwk_pdu_id);
			sending_nrf_mac_frame.hdr.src_mac_addr = nrf_get_static_nwk_addr();
			sending_nrf_mac_frame.hdr.mac_seq = sending_mac_sequence;
			sending_nrf_mac_frame.hdr.frame_num = (sending_nrf_nwk_frame->len / MAX_MAC_PAYLOAD_LEN) + ((sending_nrf_nwk_frame->len % MAX_MAC_PAYLOAD_LEN) > 0);
			sending_nrf_mac_frame.hdr.frame_seq = 0;

			sending_nrf_nwk_frame_retry_counter = 0;
			sending_mac_frame_retry_counter = 0;
			set_mac_send_state(MAC_SEND_STATE_SENDING);
			task_post_pure_msg(AC_RF24_MAC_ID, AC_RF24_MAC_SEND_FRAME);
		}
	}
		break;

	case AC_RF24_MAC_SEND_FRAME: {
		NRF_DBG_SIG("AC_RF24_MAC_SEND_FRAME\n");
		if (get_mac_send_state() == MAC_SEND_STATE_SENDING) {
			NRF_DBG("[MAC] [SF] sma:0x%X ms:%d fn:%d fs:%d\n", sending_nrf_mac_frame.hdr.src_mac_addr, \
					sending_nrf_mac_frame.hdr.mac_seq, \
					sending_nrf_mac_frame.hdr.frame_num, \
					sending_nrf_mac_frame.hdr.frame_seq);

			/* div nwk_frame and post mac frame to physic layer */
			memcpy(sending_nrf_mac_frame.payload, (sending_nrf_nwk_frame->payload + (sending_nrf_mac_frame.hdr.frame_seq * MAX_MAC_PAYLOAD_LEN)), MAX_MAC_PAYLOAD_LEN);
			sending_nrf_mac_frame.frame_cs = calc_frame_cs(sending_nrf_mac_frame.payload, MAX_MAC_PAYLOAD_LEN);
			task_post_common_msg(AC_RF24_PHY_ID, AC_RF24_PHY_SEND_FRAME_REQ, (uint8_t*)&sending_nrf_mac_frame, sizeof(nrf_mac_msg_t));
		}
	}
		break;

	case AC_RF24_MAC_SEND_FRAME_DONE: {
		NRF_DBG_SIG("AC_RF24_MAC_SEND_FRAME_DONE\n");
		if (get_mac_send_state() == MAC_SEND_STATE_SENDING) {
			/* reset mac_frame retry counter */
			sending_mac_frame_retry_counter = 0;

			if (sending_nrf_mac_frame.hdr.frame_seq == (sending_nrf_mac_frame.hdr.frame_num - 1)) { /* send completed */
				uint32_t nwk_pdu_id = sending_nrf_nwk_frame->id;
				task_post_common_msg(AC_RF24_NWK_ID, AC_RF24_NWK_SEND_MSG_DONE, (uint8_t*)&nwk_pdu_id, sizeof(uint32_t));

				set_mac_send_state(MAC_SEND_STATE_IDLE);

				if (fifo_availble(&nwk_pdu_id_fifo)) {
					uint32_t get_pdu_id;
					fifo_get(&nwk_pdu_id_fifo, &get_pdu_id);
					task_post_common_msg(AC_RF24_MAC_ID, AC_RF24_MAC_HANDLE_MSG_OUT, (uint8_t*)&get_pdu_id, sizeof(uint32_t));
				}
			}
			else { /* send next frame */
				sending_nrf_mac_frame.hdr.frame_seq++;
				task_post_pure_msg(AC_RF24_MAC_ID, AC_RF24_MAC_SEND_FRAME);
			}
		}
	}
		break;

	case AC_RF24_MAC_SEND_FRAME_ERR: {
		NRF_DBG_SIG("AC_RF24_MAC_SEND_FRAME_ERR\n");
		if (get_mac_send_state() == MAC_SEND_STATE_SENDING) {
			if (sending_mac_frame_retry_counter++ < SENDING_MAC_FRAME_RETRY_COUNTER_MAX) {
				/* mac frame retry */
				timer_set(AC_RF24_MAC_ID, AC_RF24_MAC_SEND_FRAME, SENDING_MAC_FRAME_RETRY_INTERVAL, TIMER_ONE_SHOT);
			}
			else {
				sending_mac_frame_retry_counter = 0;

				if (sending_nrf_nwk_frame_retry_counter++ < SENDING_NRF_NWK_FRAME_RETRY_COUNTER_MAX) {
					/* nwk_frame retry */
					sending_mac_sequence++;
					sending_nrf_mac_frame.hdr.frame_seq = 0;
					sending_nrf_mac_frame.hdr.mac_seq = sending_mac_sequence;
					timer_set(AC_RF24_MAC_ID, AC_RF24_MAC_SEND_FRAME, SENDING_NRF_NWK_FRAME_RETRY_INTERVAL, TIMER_ONE_SHOT);
				}
				else {
					sending_nrf_nwk_frame_retry_counter = 0;
					set_mac_send_state(MAC_SEND_STATE_IDLE);

					uint32_t nwk_pdu_id = sending_nrf_nwk_frame->id;
					task_post_common_msg(AC_RF24_NWK_ID, AC_RF24_NWK_SEND_MSG_ERR_SDF, (uint8_t*)&nwk_pdu_id, sizeof(uint32_t));

					if (fifo_availble(&nwk_pdu_id_fifo)) {
						uint32_t get_pdu_id;
						fifo_get(&nwk_pdu_id_fifo, &get_pdu_id);
						task_post_common_msg(AC_RF24_MAC_ID, AC_RF24_MAC_HANDLE_MSG_OUT, (uint8_t*)&get_pdu_id, sizeof(uint32_t));
					}
				}
			}
		}
	}
		break;

	case AC_RF24_MAC_RECV_FRAME: {
		NRF_DBG_SIG("AC_RF24_MAC_RECV_FRAME\n");
		nrf_mac_msg_t* st_nrf_mac_msg = (nrf_mac_msg_t*)get_data_common_msg(msg);
		NRF_DBG("[MAC] [RF] sma:0x%X ms:%d fn:%d fs:%d\trms:%d\n", st_nrf_mac_msg->hdr.src_mac_addr, \
				st_nrf_mac_msg->hdr.mac_seq, \
				st_nrf_mac_msg->hdr.frame_num, \
				st_nrf_mac_msg->hdr.frame_seq, \
				receiving_mac_sequence);

		uint8_t calc_fcs = calc_frame_cs(st_nrf_mac_msg->payload, MAX_MAC_PAYLOAD_LEN);

		if (calc_fcs == st_nrf_mac_msg->frame_cs) {
			if (st_nrf_mac_msg->hdr.mac_seq != receiving_mac_sequence) { /* receive first mac_frame */
				if (get_mac_rev_state() == MAC_REV_STATE_IDLE) {
					if(st_nrf_mac_msg->hdr.frame_seq == 0) {
						receiving_nrf_nwk_frame = nrf_nwk_pdu_malloc();

						if (receiving_nrf_nwk_frame != NRF_NWK_PDU_NULL) {
							receiving_mac_sequence = st_nrf_mac_msg->hdr.mac_seq;
							set_mac_rev_state(MAC_REV_STATE_RECEIVING);
						}
					}
				}
			}

			if (get_mac_rev_state() == MAC_REV_STATE_RECEIVING) {
				if (st_nrf_mac_msg->hdr.frame_seq < st_nrf_mac_msg->hdr.frame_num) {
					if (st_nrf_mac_msg->hdr.frame_seq == (st_nrf_mac_msg->hdr.frame_num - 1)) { /* receive last mac_frame */
						/* clear receiving mac_frame TO and switch rev_state to idle in order to allready receiving new mac_frame */
						timer_remove_attr(AC_RF24_MAC_ID, AC_RF24_MAC_RECV_FRAME_TO);
						set_mac_rev_state(MAC_REV_STATE_IDLE);
					}
					else {
						timer_set(AC_RF24_MAC_ID, AC_RF24_MAC_RECV_FRAME_TO, REV_MAC_FRAME_TO_INTERVAL, TIMER_ONE_SHOT);
					}

					msg_inc_ref_count(msg);
					set_msg_sig(msg, AC_RF24_MAC_HANDLE_MSG_IN);
					task_post(AC_RF24_MAC_ID, msg);
				}
			}
		}
	}
		break;

	case AC_RF24_MAC_RECV_FRAME_TO: {
		NRF_DBG_SIG("AC_RF24_MAC_RECV_FRAME_TO\n");
		if (get_mac_rev_state() == MAC_REV_STATE_RECEIVING) {
			nrf_nwk_pdu_free(receiving_nrf_nwk_frame);
			set_mac_rev_state(MAC_REV_STATE_IDLE);
		}
	}
		break;

	case AC_RF24_MAC_HANDLE_MSG_IN: {
		NRF_DBG_SIG("AC_RF24_MAC_HANDLE_MSG_IN\n");
		nrf_mac_msg_t* receiving_nrf_mac_frame = (nrf_mac_msg_t*)get_data_common_msg(msg);
		memcpy((receiving_nrf_nwk_frame->payload + (receiving_nrf_mac_frame->hdr.frame_seq * MAX_MAC_PAYLOAD_LEN)), receiving_nrf_mac_frame->payload, MAX_MAC_PAYLOAD_LEN);

		if (receiving_nrf_mac_frame->hdr.frame_seq == (receiving_nrf_mac_frame->hdr.frame_num - 1)) {
			uint32_t nwk_pdu_id = receiving_nrf_nwk_frame->id;
			task_post_common_msg(AC_RF24_NWK_ID, AC_RF24_NWK_RECV_MSG, (uint8_t*)&nwk_pdu_id, sizeof(uint32_t));
		}
	}
		break;

	default:
		break;
	}
}

mac_send_state_e get_mac_send_state() {
	return mac_send_state;
}

void set_mac_send_state(mac_send_state_e state) {
	mac_send_state = state;
	NRF_DBG("[MAC] set_mac_send_state -> %d\n", mac_send_state);
	if (mac_send_state != MAC_SEND_STATE_SENDING) {
		nrf_phy_switch_prx_mode();
	}
	else {
		nrf_phy_switch_ptx_mode();
	}
}

mac_rev_state_e get_mac_rev_state() {
	return mac_rev_state;
}

void set_mac_rev_state(mac_rev_state_e state) {
	mac_rev_state = state;
	NRF_DBG("[MAC] set_mac_rev_state -> %d\n", mac_rev_state);
}

uint8_t calc_frame_cs(uint8_t* data, uint16_t len) {
	uint8_t ret = 0;
	for (uint16_t i = 0; i < len; i++) {
		ret ^= *(data + i);
	}
	return ret;
}
