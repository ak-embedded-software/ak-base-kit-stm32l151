/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   08/10/2017
 ******************************************************************************
**/
#include <stdint.h>
#include <stdlib.h>

#include "app.h"
#include "app_if.h"
#include "app_data.h"
#include "app_dbg.h"
#include "task_list.h"

#include "ak.h"
#include "port.h"
#include "message.h"
#include "fsm.h"
#include "timer.h"

#include "fifo.h"

#include "sys_dbg.h"

#include "link_sig.h"
#include "link_phy.h"
#include "link_mac.h"
#include "link_data.h"
#include "link_config.h"

#define LINK_PDU_ID_BUF_SIZE	LINK_PDU_POOL_SIZE

typedef enum {
	/* private */
	MAC_FRAME_TYPE_NONE,

	/* pulic */
	MAC_FRAME_TYPE_REQ,

} phy_frame_type_e;

typedef enum {
	/* private */
	MAC_FRAME_SUB_TYPE_NONE,

	/* pulic */
} mac_frame_sub_type_e;

static uint8_t link_mac_frame_cals_checksum(link_mac_frame_t* mac_frame);
static uint16_t link_mac_frame_cals_datalen(link_mac_frame_t* mac_frame, uint32_t pdu_data_len);

typedef enum {
	LINK_MAC_SEND_STATE_IDLE,
	LINK_MAC_SEND_STATE_SENDING
} link_mac_send_state_e;

static link_mac_send_state_e link_mac_send_state;
static void link_mac_send_state_set(link_mac_send_state_e);
static link_mac_send_state_e link_mac_send_state_get();

static uint8_t link_mac_pdu_sending_sequence;
static link_pdu_t* link_mac_pdu_sending;
static uint8_t link_mac_pdu_sending_retry_counter;

static link_mac_frame_t link_mac_frame_send;

uint32_t link_mac_frame_send_to_interval;
uint32_t link_mac_frame_rev_to_interval;

static void link_mac_frame_send_end();
static void link_mac_frame_send_req();

/* mac receiving declare */
typedef enum {
	LINK_MAC_REV_STATE_IDLE,
	LINK_MAC_REV_STATE_RECEIVING
} link_mac_rev_state_e;

static link_mac_rev_state_e link_mac_rev_state;
static void link_mac_rev_state_set(link_mac_rev_state_e);
static link_mac_rev_state_e link_mac_rev_state_get();

static uint8_t link_mac_pdu_receiving_sequence;
static link_pdu_t* link_mac_pdu_receiving;

static uint32_t link_pdu_id_buf[LINK_PDU_ID_BUF_SIZE];
static fifo_t link_pdu_id_fifo;

fsm_t fsm_link_mac;

void task_link_mac(ak_msg_t* msg) {
	fsm_dispatch(&fsm_link_mac, msg);
}

void fsm_link_mac_state_init(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_LINK_MAC_INIT: {
		LINK_DBG_SIG("AC_LINK_MAC_INIT\n");
		/* init lower layer */
		task_post_pure_msg(AC_LINK_PHY_ID, AC_LINK_PHY_INIT);
	}
		break;

	case AC_LINK_MAC_PHY_LAYER_STARTED: {
		LINK_DBG_SIG("AC_LINK_MAC_PHY_LAYER_STARTED\n");
		/* init mac layer */
		fifo_init(&link_pdu_id_fifo, link_pdu_id_buf, LINK_PDU_ID_BUF_SIZE, sizeof(uint32_t));

		/* init mac sequence */
		link_mac_pdu_sending_sequence = (uint8_t)rand();
		link_mac_pdu_receiving_sequence = (uint8_t)rand();

		/* init seding/receiving state */
		link_mac_send_state_set(LINK_MAC_SEND_STATE_IDLE);
		link_mac_rev_state_set(LINK_MAC_REV_STATE_IDLE);

		uint32_t link_phy_get_send_frame = link_phy_get_send_frame_to();

		link_mac_frame_send_to_interval = 2 * link_phy_get_send_frame;
		link_mac_frame_rev_to_interval = 2 * link_phy_get_send_frame;

		/* notify complete init to higher layer */
		task_post_pure_msg(AC_LINK_ID, AC_LINK_MAC_LAYER_STARTED);
		FSM_TRAN(&fsm_link_mac, fsm_link_mac_state_handle);
	}
		break;

	default:
		break;
	}
}

void fsm_link_mac_state_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_LINK_MAC_FRAME_SEND_REQ: {
		LINK_DBG_SIG("AC_LINK_MAC_FRAME_SEND_REQ\n");
		uint32_t pdu_id;
		memcpy(&pdu_id, get_data_common_msg(msg), sizeof(uint32_t));

		if (link_mac_send_state_get() == LINK_MAC_SEND_STATE_IDLE) {
			link_mac_send_state_set(LINK_MAC_SEND_STATE_SENDING);
			task_post_common_msg(AC_LINK_MAC_ID, AC_LINK_MAC_FRAME_SEND_START, (uint8_t*)&pdu_id, sizeof(uint32_t));
		}
		else {
			if (fifo_is_full(&link_pdu_id_fifo)) {
				FATAL("link", 0x03);
			}
			else {
				fifo_put(&link_pdu_id_fifo, (uint8_t*)&pdu_id);
			}
		}
	}
		break;

	case AC_LINK_MAC_FRAME_SEND_START: {
		LINK_DBG_SIG("AC_LINK_MAC_FRAME_SEND_START\n");
		uint32_t pdu_id;
		memcpy(&pdu_id, get_data_common_msg(msg), sizeof(uint32_t));

		if (link_mac_send_state_get() == LINK_MAC_SEND_STATE_SENDING) {
			link_mac_pdu_sending_retry_counter = 0;

			link_mac_pdu_sending = link_pdu_get(pdu_id);

			link_mac_frame_send.header.des_addr = link_get_des_addr();
			link_mac_frame_send.header.src_addr = link_get_src_addr();
			link_mac_frame_send.header.type = MAC_FRAME_TYPE_REQ;
			link_mac_frame_send.header.sub_type = MAC_FRAME_SUB_TYPE_NONE;
			link_mac_frame_send.header.seq_num = link_mac_pdu_sending_sequence++;
			link_mac_frame_send.header.fnum = (link_mac_pdu_sending->len / LINK_MAC_FRAME_DATA_SIZE) + \
					((link_mac_pdu_sending->len % LINK_MAC_FRAME_DATA_SIZE) > 0);

			link_mac_frame_send.header.fidx = 0;
			link_mac_frame_send.header.len = link_mac_frame_cals_datalen(&link_mac_frame_send, link_mac_pdu_sending->len);
			memcpy(link_mac_frame_send.data, (uint8_t*)&link_mac_pdu_sending->payload[link_mac_frame_send.header.fidx * LINK_MAC_FRAME_DATA_SIZE], link_mac_frame_send.header.len);
			link_mac_frame_send.header.fcs = link_mac_frame_cals_checksum(&link_mac_frame_send);

			link_mac_frame_send_req();
		}
		else {
			FATAL("link_mac", 0x04);
		}
	}
		break;

	case AC_LINK_MAC_FRAME_SEND_DONE: {
		LINK_DBG_SIG("AC_LINK_MAC_FRAME_SEND_DONE\n");
		if (link_mac_send_state_get() == LINK_MAC_SEND_STATE_SENDING) {
			if (++link_mac_frame_send.header.fidx < link_mac_frame_send.header.fnum) {
				link_mac_frame_send.header.len = link_mac_frame_cals_datalen(&link_mac_frame_send, link_mac_pdu_sending->len);
				memcpy(link_mac_frame_send.data, (uint8_t*)&link_mac_pdu_sending->payload[link_mac_frame_send.header.fidx * LINK_MAC_FRAME_DATA_SIZE], link_mac_frame_send.header.len);
				link_mac_frame_send.header.fcs = link_mac_frame_cals_checksum(&link_mac_frame_send);

				link_mac_frame_send_req();
			}
			else {
				uint32_t link_pdu_send_done = link_mac_pdu_sending->id;
				task_post_common_msg(AC_LINK_ID, AC_LINK_SEND_DONE, (uint8_t*)&link_pdu_send_done, sizeof(uint32_t));

				/* send link pdu completed */
				link_mac_frame_send_end();
			}
		}
	}
		break;

	case AC_LINK_MAC_FRAME_SEND_ERR: {
		LINK_DBG_SIG("AC_LINK_MAC_FRAME_SEND_ERR\n");
		if (link_mac_pdu_sending_retry_counter >= LINK_MAC_PDU_SENDING_RETRY_COUNTER_MAX) {
			uint32_t send_pdu_err = link_mac_pdu_sending->id;
			task_post_common_msg(AC_LINK_ID, AC_LINK_SEND_ERR, (uint8_t*)&send_pdu_err, sizeof(uint32_t));

			/* mac frame send false */
			link_mac_frame_send_end();
		}
		else {
			/* retry sending PDU */
			link_mac_frame_send.header.fidx = 0;
			link_mac_frame_send.header.len = link_mac_frame_cals_datalen(&link_mac_frame_send, link_mac_pdu_sending->len);
			memcpy(link_mac_frame_send.data, (uint8_t*)&link_mac_pdu_sending->payload[link_mac_frame_send.header.fidx * LINK_MAC_FRAME_DATA_SIZE], link_mac_frame_send.header.len);
			link_mac_frame_send.header.fcs = link_mac_frame_cals_checksum(&link_mac_frame_send);

			link_mac_frame_send_req();
		}
		link_mac_pdu_sending_retry_counter++;
	}
		break;

	case AC_LINK_MAC_FRAME_REV: {
		LINK_DBG_SIG("AC_LINK_MAC_FRAME_REV\n");
		link_mac_frame_t link_mac_frame_rev;
		memcpy(&link_mac_frame_rev, get_data_common_msg(msg), sizeof(link_mac_frame_t));

		if (link_mac_rev_state_get() == LINK_MAC_REV_STATE_IDLE) {
			if (link_mac_pdu_receiving_sequence != link_mac_frame_rev.header.seq_num) {
				link_mac_rev_state_set(LINK_MAC_REV_STATE_RECEIVING);

				/* update receive pdu sequence number */
				link_mac_pdu_receiving_sequence = link_mac_frame_rev.header.seq_num;

				link_mac_pdu_receiving = link_pdu_malloc();

				link_mac_pdu_receiving->len = link_mac_frame_rev.header.len;
				memcpy(&link_mac_pdu_receiving->payload[link_mac_frame_rev.header.fidx * LINK_MAC_FRAME_DATA_SIZE], link_mac_frame_rev.data, link_mac_frame_rev.header.len);
			}
		}
		else if (link_mac_pdu_receiving_sequence == link_mac_frame_rev.header.seq_num &&
				 link_mac_rev_state_get() == LINK_MAC_REV_STATE_RECEIVING) {
			link_mac_pdu_receiving->len += link_mac_frame_rev.header.len;
			memcpy(&link_mac_pdu_receiving->payload[link_mac_frame_rev.header.fidx * LINK_MAC_FRAME_DATA_SIZE], link_mac_frame_rev.data, link_mac_frame_rev.header.len);
		}

		if (link_mac_pdu_receiving_sequence == link_mac_frame_rev.header.seq_num &&
						 link_mac_rev_state_get() == LINK_MAC_REV_STATE_RECEIVING) {

			if (link_mac_frame_rev.header.fidx == (link_mac_frame_rev.header.fnum - 1)) { /* the last frame */
				link_mac_rev_state_set(LINK_MAC_REV_STATE_IDLE);
				timer_remove_attr(AC_LINK_MAC_ID, AC_LINK_MAC_FRAME_REV_TO);

				uint32_t rev_pdu = link_mac_pdu_receiving->id;
				task_post_common_msg(AC_LINK_ID, AC_LINK_REV_MSG, (uint8_t*)&rev_pdu, sizeof(uint32_t));
			}
			else {
				timer_set(AC_LINK_MAC_ID, AC_LINK_MAC_FRAME_REV_TO, link_mac_frame_rev_to_interval, TIMER_ONE_SHOT);
			}
		}
	}
		break;

	case AC_LINK_MAC_FRAME_REV_TO: {
		LINK_DBG_SIG("AC_LINK_MAC_FRAME_REV_TO\n");
		if (link_mac_rev_state_get() == LINK_MAC_REV_STATE_RECEIVING) {
			link_mac_rev_state_set(LINK_MAC_REV_STATE_IDLE);
			link_pdu_free(link_mac_pdu_receiving->id);
		}
	}
		break;

	default:
		break;
	}
}

void link_mac_send_state_set(link_mac_send_state_e state) {
	link_mac_send_state = state;
	LINK_DBG("[MAC] link_mac_send_state_set -> %d\n", state);
}

link_mac_send_state_e link_mac_send_state_get() {
	return link_mac_send_state;
}

void link_mac_rev_state_set(link_mac_rev_state_e state) {
	link_mac_rev_state = state;
	LINK_DBG("[MAC] link_mac_rev_state_set -> %d\n", state);
}

link_mac_rev_state_e link_mac_rev_state_get() {
	return link_mac_rev_state;
}

uint8_t link_mac_frame_cals_checksum(link_mac_frame_t* mac_frame) {
	uint8_t* frame_header = (uint8_t*)mac_frame;
	uint8_t ret_check_sum = 0;

	/* cals checksum of header frame */
	for (uint32_t i = 0; i < (sizeof(link_mac_frame_t) - sizeof(uint8_t)); i++) {
		ret_check_sum ^= *(frame_header + i);
	}

	/* cals checksum of data frame */
	for (uint32_t i = 0; i < mac_frame->header.len; i++) {
		ret_check_sum ^= mac_frame->data[i];
	}

	return (uint8_t)ret_check_sum;
}

uint16_t link_mac_frame_cals_datalen(link_mac_frame_t* mac_frame, uint32_t pdu_data_len) {
	uint16_t ret_len = pdu_data_len - (mac_frame->header.fidx * LINK_MAC_FRAME_DATA_SIZE);
	if (ret_len <= LINK_MAC_FRAME_DATA_SIZE) {
		return ret_len;
	}
	return ((uint16_t)LINK_MAC_FRAME_DATA_SIZE);
}

void link_mac_frame_send_req() {
	if (link_mac_send_state_get() == LINK_MAC_SEND_STATE_SENDING) {
		task_post_common_msg(AC_LINK_PHY_ID, AC_LINK_PHY_FRAME_SEND_REQ, (uint8_t*)&link_mac_frame_send, sizeof(link_mac_frame_t));
	}
	else {
		FATAL("link_mac", 0x01);
	}
}

void link_mac_frame_send_end() {
	if (fifo_availble(&link_pdu_id_fifo)) {
		uint32_t link_pdu_send;
		fifo_get(&link_pdu_id_fifo, (uint8_t*)&link_pdu_send);
		task_post_common_msg(AC_LINK_MAC_ID, AC_LINK_MAC_FRAME_SEND_START, (uint8_t*)&link_pdu_send, sizeof(uint32_t));
	}
	else {
		link_mac_send_state_set(LINK_MAC_SEND_STATE_IDLE);
	}
}
