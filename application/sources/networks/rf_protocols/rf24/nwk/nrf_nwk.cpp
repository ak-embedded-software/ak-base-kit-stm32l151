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
#include "task_list.h"

#include "ak.h"
#include "port.h"

#include "sys_dbg.h"

#include "nrf_config.h"
#include "nrf_data.h"
#include "nrf_nwk.h"
#include "nrf_mac.h"

#define NWK_HDR_LEN						(sizeof(nrf_nwk_hdr_t))
#define MAX_NWK_PAYLOAD_LEN				(NRF_NWK_MSG_MAX_LEN - NWK_HDR_LEN)

typedef struct {
	uint16_t src_addr;
	uint16_t des_addr;
	uint8_t type;
	uint16_t payload_len;
} __AK_PACKETED nrf_nwk_hdr_t;

typedef struct {
	nrf_nwk_hdr_t hdr;
	uint8_t payload[MAX_NWK_PAYLOAD_LEN];
} __AK_PACKETED nrf_nwk_msg_t;

fsm_t nrf_nwk_fsm;

void task_rf24_nwk(ak_msg_t* msg) {
	fsm_dispatch(&nrf_nwk_fsm, msg);
}

void nrf_nwk_fsm_init(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_RF24_NWK_INIT: {
		NRF_DBG_SIG("AC_RF24_NWK_INIT\n");

		/* init network message pool */
		nrf_nwk_pdu_init();

		/* init physic layer */
		task_post_pure_msg(AC_RF24_PHY_ID, AC_RF24_PHY_INIT);

		/* init mac layer */
		task_post_pure_msg(AC_RF24_MAC_ID, AC_RF24_MAC_INIT);

		/* switch to idle state */
		FSM_TRAN(&nrf_nwk_fsm, nrf_nwk_fsm_idle);
	}
		break;

	default:
		break;
	}
}

void nrf_nwk_fsm_idle(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_RF24_NWK_PURE_MSG_OUT: {
		NRF_DBG_SIG("AC_RF24_NWK_PURE_MSG_OUT\n");
		ak_msg_pure_if_t if_msg;
		if_msg.header.src_task_id = msg->if_src_task_id;
		if_msg.header.des_task_id = msg->if_des_task_id;
		if_msg.header.type = get_msg_type(msg);
		if_msg.header.if_src_type = msg->if_src_type;
		if_msg.header.if_des_type = msg->if_des_type;
		if_msg.header.sig = msg->if_sig;

		nrf_nwk_pdu_t* st_nrf_nwk_pdu = nrf_nwk_pdu_malloc();

		if (st_nrf_nwk_pdu != NRF_NWK_PDU_NULL) {
			nrf_nwk_msg_t* st_nrf_nwk_msg = (nrf_nwk_msg_t*)(st_nrf_nwk_pdu->payload);
			st_nrf_nwk_msg->hdr.src_addr = nrf_get_static_nwk_addr();
			st_nrf_nwk_msg->hdr.des_addr = nrf_get_des_nwk_addr();
			st_nrf_nwk_msg->hdr.type = RF24_MSG_TYPE_PURE;
			st_nrf_nwk_msg->hdr.payload_len = sizeof(ak_msg_pure_if_t);

			memcpy(st_nrf_nwk_msg->payload, (uint8_t*)&if_msg, sizeof(ak_msg_pure_if_t));
			st_nrf_nwk_pdu->len = sizeof(ak_msg_pure_if_t) + NWK_HDR_LEN;

			uint32_t nwk_pdu_id = st_nrf_nwk_pdu->id;
			task_post_common_msg(AC_RF24_MAC_ID, AC_RF24_MAC_HANDLE_MSG_OUT, (uint8_t*)&nwk_pdu_id, sizeof(uint32_t));
		}
		else {
			FATAL("NWK", 0x01);
		}
	}
		break;

	case AC_RF24_NWK_COMMON_MSG_OUT: {
		NRF_DBG_SIG("AC_RF24_NWK_COMMON_MSG_OUT\n");
		ak_msg_common_if_t if_msg;
		if_msg.header.src_task_id = msg->if_src_task_id;
		if_msg.header.des_task_id = msg->if_des_task_id;
		if_msg.header.type = get_msg_type(msg);
		if_msg.header.if_src_type = msg->if_src_type;
		if_msg.header.if_des_type = msg->if_des_type;
		if_msg.header.sig = msg->if_sig;

		if_msg.len = get_data_len_common_msg(msg);
		memcpy(if_msg.data, get_data_common_msg(msg), if_msg.len);

		nrf_nwk_pdu_t* st_nrf_nwk_pdu = nrf_nwk_pdu_malloc();

		if (st_nrf_nwk_pdu != NRF_NWK_PDU_NULL) {
			nrf_nwk_msg_t* st_nrf_nwk_msg = (nrf_nwk_msg_t*)(st_nrf_nwk_pdu->payload);
			st_nrf_nwk_msg->hdr.src_addr = nrf_get_static_nwk_addr();
			st_nrf_nwk_msg->hdr.des_addr = nrf_get_des_nwk_addr();
			st_nrf_nwk_msg->hdr.type = RF24_MSG_TYPE_COMMON;
			st_nrf_nwk_msg->hdr.payload_len = sizeof(ak_msg_pure_if_t);
			memcpy(st_nrf_nwk_msg->payload, (uint8_t*)&if_msg, sizeof(ak_msg_common_if_t));

			memcpy(st_nrf_nwk_msg->payload, (uint8_t*)&if_msg, sizeof(ak_msg_common_if_t));
			st_nrf_nwk_pdu->len = sizeof(ak_msg_common_if_t) + NWK_HDR_LEN;

			uint32_t nwk_pdu_id = st_nrf_nwk_pdu->id;
			task_post_common_msg(AC_RF24_MAC_ID, AC_RF24_MAC_HANDLE_MSG_OUT, (uint8_t*)&nwk_pdu_id, sizeof(uint32_t));
		}
		else {
			FATAL("NWK", 0x02);
		}
	}
		break;

	case AC_RF24_NWK_DYNAMIC_MSG_OUT: {
		NRF_DBG_SIG("AC_RF24_NWK_DYNAMIC_MSG_OUT\n");
	}
		break;

	case AC_RF24_NWK_DATA_MSG_OUT: {
		NRF_DBG_SIG("AC_RF24_NWK_DATA_MSG_OUT\n");
	}
		break;

	case AC_RF24_NWK_RECV_MSG: {
		NRF_DBG_SIG("AC_RF24_NWK_RECV_MSG\n");

		uint32_t nwk_pdu_id;
		memcpy(&nwk_pdu_id, (uint8_t*)get_data_common_msg(msg), sizeof(uint32_t));

		nrf_nwk_pdu_t* st_nrf_nwk_pdu = nrf_nwk_pdu_get(nwk_pdu_id);
		nrf_nwk_msg_t* st_nrf_nwk_msg = (nrf_nwk_msg_t*)(st_nrf_nwk_pdu->payload);

		NRF_DBG("nrf_nwk_hdr.src_addr: 0x%04x\n", st_nrf_nwk_msg->hdr.src_addr);
		NRF_DBG("nrf_nwk_hdr.des_addr: 0x%04x\n", st_nrf_nwk_msg->hdr.des_addr);
		NRF_DBG("nrf_nwk_hdr.type: %d\n", st_nrf_nwk_msg->hdr.type);
		NRF_DBG("nrf_nwk_hdr.payload_len: %d\n", st_nrf_nwk_msg->hdr.payload_len);

		switch (st_nrf_nwk_msg->hdr.type) {
		case RF24_MSG_TYPE_PURE: {
			ak_msg_pure_if_t* if_msg = (ak_msg_pure_if_t*)st_nrf_nwk_msg->payload;

			ak_msg_t* s_msg = get_pure_msg();
			set_if_src_task_id(s_msg, if_msg->header.src_task_id);
			set_if_des_task_id(s_msg, if_msg->header.des_task_id);
			set_if_src_type(s_msg, if_msg->header.if_src_type);
			set_if_des_type(s_msg, if_msg->header.if_des_type);
			set_if_sig(s_msg, if_msg->header.sig);

			set_msg_sig(s_msg, AC_RF24_IF_PURE_MSG_IN);
			task_post(AC_TASK_RF24_IF_ID, s_msg);
		}
			break;

		case RF24_MSG_TYPE_COMMON: {
			ak_msg_common_if_t* if_msg = (ak_msg_common_if_t*)st_nrf_nwk_msg->payload;

			ak_msg_t* s_msg = get_common_msg();
			set_if_src_task_id(s_msg, if_msg->header.src_task_id);
			set_if_des_task_id(s_msg, if_msg->header.des_task_id);
			set_if_src_type(s_msg, if_msg->header.if_src_type);
			set_if_des_type(s_msg, if_msg->header.if_des_type);
			set_if_sig(s_msg, if_msg->header.sig);
			set_if_data_common_msg(s_msg, if_msg->data, if_msg->len);

			set_msg_sig(s_msg, AC_RF24_IF_COMMON_MSG_IN);
			task_post(AC_TASK_RF24_IF_ID, s_msg);
		}
			break;

		case RF24_MSG_TYPE_DYNAMIC: {
		}
			break;

		case RF24_MSG_TYPE_DATA: {
		}
			break;

		default:
			break;
		}

		/* free receive nwk pdu */
		nrf_nwk_pdu_free(nwk_pdu_id);
	}
		break;

	case AC_RF24_NWK_SEND_MSG_DONE: {
		NRF_DBG_SIG("AC_RF24_NWK_SEND_MSG_DONE\n");

		/* free receive nwk pdu */
		uint32_t nwk_pdu_id;
		memcpy(&nwk_pdu_id, (uint8_t*)get_data_common_msg(msg), sizeof(uint32_t));
		nrf_nwk_pdu_free(nwk_pdu_id);
	}
		break;

	case AC_RF24_NWK_SEND_MSG_ERR_SDF: {
		NRF_DBG_SIG("AC_RF24_NWK_SEND_MSG_ERR_SDF\n");
		/* free error nwk pdu */
		uint32_t nwk_pdu_id;
		memcpy(&nwk_pdu_id, (uint8_t*)get_data_common_msg(msg), sizeof(uint32_t));
		nrf_nwk_pdu_free(nwk_pdu_id);
	}
		break;

	case AC_RF24_NWK_SEND_MSG_ERR_BUSY: {
		NRF_DBG_SIG("AC_RF24_NWK_SEND_MSG_ERR_BUSY\n");

		/* free error nwk pdu */
		uint32_t nwk_pdu_id;
		memcpy(&nwk_pdu_id, (uint8_t*)get_data_common_msg(msg), sizeof(uint32_t));
		nrf_nwk_pdu_free(nwk_pdu_id);
	}
		break;

	default:
		break;
	}
}
