#include <stdbool.h>
#include <stdint.h>

#include "app.h"
#include "app_dbg.h"
#include "app_data.h"

#include "task_rf24_if.h"
#include "task_if.h"
#include "task_list.h"

#include "fsm.h"
#include "port.h"
#include "message.h"
#include "timer.h"

#include "fifo.h"

#include "eeprom.h"

#if defined(IF_NETWORK_NRF24_EN)
#include "hal_nrf.h"
#include "nrf_data.h"
#endif

void task_rf24_if(ak_msg_t* msg) {
	switch (msg->sig) {
#if defined(IF_NETWORK_NRF24_EN)
	case AC_RF24_IF_INIT_NETWORK: {
		APP_DBG_SIG("AC_RF24_IF_INIT_NETWORK\n");

		/* init network address */
		nrf_set_static_nwk_addr(IF_TYPE_RF24_ME);

		/* init network state machine */
		FSM(&nrf_nwk_fsm, nrf_nwk_fsm_init);

		/* post message init network request */
		task_post_pure_msg(AC_RF24_NWK_ID, AC_RF24_NWK_INIT);
	}
		break;

	case AC_RF24_IF_PURE_MSG_OUT: {
		APP_DBG_SIG("AC_RF24_IF_PURE_MSG_OUT\n");
		nrf_set_des_nwk_addr(msg->if_des_type);

		/* forward message to nwk layer */
		msg_inc_ref_count(msg);
		set_msg_sig(msg, AC_RF24_NWK_PURE_MSG_OUT);
		task_post(AC_RF24_NWK_ID, msg);
	}
		break;

	case AC_RF24_IF_COMMON_MSG_OUT: {
		APP_DBG_SIG("AC_RF24_IF_COMMON_MSG_OUT\n");
		nrf_set_des_nwk_addr(msg->if_des_type);

		/* forward message to nwk layer */
		msg_inc_ref_count(msg);
		set_msg_sig(msg, AC_RF24_NWK_COMMON_MSG_OUT);
		task_post(AC_RF24_NWK_ID, msg);
	}
		break;

	case AC_RF24_IF_PURE_MSG_IN: {
		APP_DBG_SIG("AC_RF24_IF_PURE_MSG_IN\n");
		msg_inc_ref_count(msg);
		set_msg_sig(msg, AC_IF_PURE_MSG_IN);
		task_post(AC_TASK_IF_ID, msg);
	}
		break;

	case AC_RF24_IF_COMMON_MSG_IN: {
		APP_DBG_SIG("AC_RF24_IF_COMMON_MSG_IN\n");
		msg_inc_ref_count(msg);
		set_msg_sig(msg, AC_IF_COMMON_MSG_IN);
		task_post(AC_TASK_IF_ID, msg);
	}
		break;
#endif
	default:
		break;
	}
}
