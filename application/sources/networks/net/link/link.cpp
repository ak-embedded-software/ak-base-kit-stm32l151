/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   08/10/2017
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
#include "message.h"
#include "fsm.h"
#include "timer.h"

#include "sys_dbg.h"

#include "link.h"
#include "link_sig.h"
#include "link_data.h"
#include "link_mac.h"
#include "link_phy.h"

static fsm_t fsm_link;
static void fsm_link_state_init(ak_msg_t* msg);
static void fsm_link_state_handle(ak_msg_t* msg);

void task_link(ak_msg_t* msg) {
	fsm_dispatch(&fsm_link, msg);
}

void link_init_state_machine() {
	FSM(&fsm_link_phy,	fsm_link_phy_state_init);
	FSM(&fsm_link_mac,	fsm_link_mac_state_init);
	FSM(&fsm_link,		fsm_link_state_init);
}

void fsm_link_state_init(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_LINK_INIT: {
		LINK_DBG_SIG("AC_LINK_INIT\n");

		/* link protocol data unit pool initial */
		link_pdu_init();

		/* request lower layer init */
		task_post_pure_msg(AC_LINK_MAC_ID, AC_LINK_MAC_INIT);
	}
		break;

	case AC_LINK_MAC_LAYER_STARTED: {
		LINK_DBG_SIG("AC_LINK_MAC_LAYER_STARTED\n");
		FSM_TRAN(&fsm_link, fsm_link_state_handle);

		LINK_PRINT("----------\n");
		LINK_PRINT("LINK_PDU pool size: %d (pdu)\n", LINK_PDU_POOL_SIZE);
		LINK_PRINT("LINK_PDU buffer size: %d (bytes)\n", LINK_PDU_BUF_SIZE);
		LINK_PRINT("MAX time link-frame fails: %d (ms)\n", (LINK_MAC_PDU_SENDING_RETRY_COUNTER_MAX + 1) * ((LINK_PHY_MAX_RETRY_SET_DEFAULT + 1)* LINK_PHY_FRAME_SEND_TO_INTERVAL));
		LINK_PRINT("MAX timeout send phy-frame: %d (ms)\n", LINK_PHY_FRAME_SEND_TO_INTERVAL);
		LINK_PRINT("MAX timeout receive NEXT mac-frame: %d (ms)\n", LINK_PHY_FRAME_REV_TO_INTERVAL);
		LINK_PRINT("----------\n");
	}
		break;

	default:
		break;
	}
}

void fsm_link_state_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_LINK_SEND_PURE_MSG: {
		LINK_DBG_SIG("AC_LINK_SEND_PURE_MSG\n");
		link_pdu_t* link_pdu = link_pdu_malloc();

		if (link_pdu == LINK_PDU_NULL) {
			task_post_pure_msg(AC_LINK_ID, AC_LINK_SEND_HANDLE_PDU_FULL);
		}
		else {
			ak_msg_pure_if_t if_msg;
			if_msg.header.src_task_id = msg->if_src_task_id;
			if_msg.header.des_task_id = msg->if_des_task_id;
			if_msg.header.type = get_msg_type(msg);
			if_msg.header.if_src_type = msg->if_src_type;
			if_msg.header.if_des_type = msg->if_des_type;
			if_msg.header.sig = msg->if_sig;

			link_frame_t link_frame;
			link_frame.header.src_addr = if_msg.header.src_task_id;
			link_frame.header.des_addr = if_msg.header.des_task_id;
			link_frame.header.type = LINK_FRAME_TYPE_PURE_MSG;
			link_frame.header.sub_type = LINK_FRAME_SUB_TYPE_NONE;
			link_frame.header.len = sizeof(ak_msg_pure_if_t);
			memcpy(link_frame.data, (uint8_t*)&if_msg, link_frame.header.len);

			link_pdu->len = sizeof(link_frame_header_t) + link_frame.header.len;
			memcpy(link_pdu->payload, (uint8_t*)&link_frame, link_pdu->len);

			uint32_t link_pdu_id = link_pdu->id;
			task_post_common_msg(AC_LINK_MAC_ID, AC_LINK_MAC_FRAME_SEND_REQ, (uint8_t*)&link_pdu_id, sizeof(uint32_t));
		}
	}
		break;

	case AC_LINK_SEND_COMMON_MSG: {
		LINK_DBG_SIG("AC_LINK_SEND_COMMON_MSG\n");
		link_pdu_t* link_pdu = link_pdu_malloc();

		if (link_pdu == LINK_PDU_NULL) {
			task_post_pure_msg(AC_LINK_ID, AC_LINK_SEND_HANDLE_PDU_FULL);
		}
		else {
			ak_msg_common_if_t if_msg;
			if_msg.header.src_task_id = msg->if_src_task_id;
			if_msg.header.des_task_id = msg->if_des_task_id;
			if_msg.header.type = get_msg_type(msg);
			if_msg.header.if_src_type = msg->if_src_type;
			if_msg.header.if_des_type = msg->if_des_type;
			if_msg.header.sig = msg->if_sig;
			if_msg.len = get_data_len_common_msg(msg);
			memcpy(if_msg.data, get_data_common_msg(msg), if_msg.len);

			link_frame_t link_frame;
			link_frame.header.src_addr = if_msg.header.src_task_id;
			link_frame.header.des_addr = if_msg.header.des_task_id;
			link_frame.header.type = LINK_FRAME_TYPE_COMMON_MSG;
			link_frame.header.sub_type = LINK_FRAME_SUB_TYPE_NONE;
			link_frame.header.len = sizeof(ak_msg_common_if_t);
			memcpy(link_frame.data, (uint8_t*)&if_msg, link_frame.header.len);

			link_pdu->len = sizeof(link_frame_header_t) + link_frame.header.len;
			memcpy(link_pdu->payload, (uint8_t*)&link_frame, link_pdu->len);

			uint32_t link_pdu_id = link_pdu->id;
			task_post_common_msg(AC_LINK_MAC_ID, AC_LINK_MAC_FRAME_SEND_REQ, (uint8_t*)&link_pdu_id, sizeof(uint32_t));
		}
	}
		break;

	case AC_LINK_SEND_DYNAMIC_MSG: {
		LINK_DBG_SIG("AC_LINK_SEND_DYNAMIC_MSG\n");
		link_pdu_t* link_pdu = link_pdu_malloc();

		if (link_pdu == LINK_PDU_NULL) {
			task_post_pure_msg(AC_LINK_ID, AC_LINK_SEND_HANDLE_PDU_FULL);
		}
		else {
			ak_msg_dynamic_if_t if_msg;
			if_msg.header.src_task_id = msg->if_src_task_id;
			if_msg.header.des_task_id = msg->if_des_task_id;
			if_msg.header.type = get_msg_type(msg);
			if_msg.header.if_src_type = msg->if_src_type;
			if_msg.header.if_des_type = msg->if_des_type;
			if_msg.header.sig = msg->if_sig;
			if_msg.len = get_data_len_dynamic_msg(msg);

			link_frame_t link_frame;
			link_frame.header.src_addr = if_msg.header.src_task_id;
			link_frame.header.des_addr = if_msg.header.des_task_id;
			link_frame.header.type = LINK_FRAME_TYPE_DYNAMIC_MSG;
			link_frame.header.sub_type = LINK_FRAME_SUB_TYPE_NONE;
			link_frame.header.len = sizeof(ak_msg_if_header_t) + sizeof(uint32_t) + if_msg.len;

			memcpy(&link_frame.data[0], \
					(uint8_t*)&if_msg, \
					sizeof(ak_msg_if_header_t) + sizeof(uint32_t));

			memcpy((uint8_t*)&link_frame.data[sizeof(ak_msg_if_header_t) + sizeof(uint32_t)], \
					get_data_dynamic_msg(msg), \
					if_msg.len);

			link_pdu->len = sizeof(link_frame_header_t) + link_frame.header.len;
			memcpy(link_pdu->payload, (uint8_t*)&link_frame, link_pdu->len);

			uint32_t link_pdu_id = link_pdu->id;
			task_post_common_msg(AC_LINK_MAC_ID, AC_LINK_MAC_FRAME_SEND_REQ, (uint8_t*)&link_pdu_id, sizeof(uint32_t));
		}
	}
		break;

	case AC_LINK_SEND_DATA: {
		LINK_DBG_SIG("AC_LINK_SEND_DATA\n");
		link_pdu_t* link_pdu = link_pdu_malloc();

		if (link_pdu == LINK_PDU_NULL) {
			task_post_pure_msg(AC_LINK_ID, AC_LINK_SEND_HANDLE_PDU_FULL);
		}
		else {
			link_frame_t link_frame;
			link_frame.header.src_addr = link_get_src_addr();
			link_frame.header.des_addr = link_get_des_addr();
			link_frame.header.type = LINK_FRAME_TYPE_DATA;
			link_frame.header.sub_type = LINK_FRAME_SUB_TYPE_NONE;
			link_frame.header.len = get_data_len_common_msg(msg);
			memcpy(link_frame.data, get_data_common_msg(msg), link_frame.header.len);

			link_pdu->len = sizeof(link_frame_header_t) + link_frame.header.len;

			uint32_t link_pdu_id = link_pdu->id;
			task_post_common_msg(AC_LINK_MAC_ID, AC_LINK_MAC_FRAME_SEND_REQ, (uint8_t*)&link_pdu_id, sizeof(uint32_t));
		}
	}
		break;

	case AC_LINK_SEND_HANDLE_PDU_FULL: {
		LINK_DBG_SIG("AC_LINK_SEND_HANDLE_PDU_FULL\n");
	}
		break;

	case AC_LINK_SEND_DONE: {
		LINK_DBG_SIG("AC_LINK_SEND_DONE\n");
		uint32_t* link_pdu_send_done = (uint32_t*)get_data_common_msg(msg);
		link_pdu_free(*link_pdu_send_done);
	}
		break;

	case AC_LINK_SEND_ERR: {
		LINK_DBG_SIG("AC_LINK_SEND_ERR\n");
		uint32_t* link_pdu_send_err = (uint32_t*)get_data_common_msg(msg);
		link_pdu_free(*link_pdu_send_err);

		/* TODO: handle ERROR */
	}
		break;

	case AC_LINK_REV_MSG: {
		LINK_DBG_SIG("AC_LINK_REV_MSG\n");
		uint32_t* rev_pdu_id = (uint32_t*)get_data_common_msg(msg);
		link_pdu_t* link_pdu = link_pdu_get(*rev_pdu_id);
		link_frame_t* link_frame = (link_frame_t*)link_pdu->payload;

		switch (link_frame->header.type) {
		case LINK_FRAME_TYPE_PURE_MSG: {
			ak_msg_pure_if_t* if_msg = (ak_msg_pure_if_t*)link_frame->data;

			ak_msg_t* s_msg = get_pure_msg();
			set_if_src_task_id(s_msg, if_msg->header.src_task_id);
			set_if_des_task_id(s_msg, if_msg->header.des_task_id);
			set_if_src_type(s_msg, if_msg->header.if_src_type);
			set_if_des_type(s_msg, if_msg->header.if_des_type);
			set_if_sig(s_msg, if_msg->header.sig);

			set_msg_sig(s_msg, AC_UART_IF_PURE_MSG_IN);
			task_post(AC_TASK_UART_IF_ID, s_msg);
		}
			break;

		case LINK_FRAME_TYPE_COMMON_MSG: {
			ak_msg_common_if_t* if_msg = (ak_msg_common_if_t*)link_frame->data;

			ak_msg_t* s_msg = get_common_msg();
			set_if_src_task_id(s_msg, if_msg->header.src_task_id);
			set_if_des_task_id(s_msg, if_msg->header.des_task_id);
			set_if_src_type(s_msg, if_msg->header.if_src_type);
			set_if_des_type(s_msg, if_msg->header.if_des_type);
			set_if_sig(s_msg, if_msg->header.sig);
			set_if_data_common_msg(s_msg, if_msg->data, if_msg->len);

			set_msg_sig(s_msg, AC_UART_IF_COMMON_MSG_IN);
			task_post(AC_TASK_UART_IF_ID, s_msg);
		}
			break;

		case LINK_FRAME_TYPE_DYNAMIC_MSG: {
			ak_msg_dynamic_if_t* if_msg = (ak_msg_dynamic_if_t*)link_frame->data;

			ak_msg_t* s_msg = get_dynamic_msg();
			set_if_src_task_id(s_msg, if_msg->header.src_task_id);
			set_if_des_task_id(s_msg, if_msg->header.des_task_id);
			set_if_src_type(s_msg, if_msg->header.if_src_type);
			set_if_des_type(s_msg, if_msg->header.if_des_type);
			set_if_sig(s_msg, if_msg->header.sig);
			set_if_data_dynamic_msg(s_msg, (uint8_t*)&link_frame->data[sizeof(ak_msg_if_header_t) + sizeof(uint32_t)], if_msg->len);

			set_msg_sig(s_msg, AC_UART_IF_DYNAMIC_MSG_IN);
			task_post(AC_TASK_UART_IF_ID, s_msg);
		}
			break;

		case LINK_FRAME_TYPE_DATA: {

		}
			break;

		default:
			break;
		}

		link_pdu_free(*rev_pdu_id);
	}
		break;

	default:
		break;
	}
}
