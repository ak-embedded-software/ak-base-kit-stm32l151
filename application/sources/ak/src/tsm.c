/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   12/02/2017
 * @brief:  table state machine.
 ******************************************************************************
**/

#include "tsm.h"
#include "sys_dbg.h"

void tsm_init(tsm_tbl_t* tsm_tbl, tsm_t** tbl, tsm_state_t state) {
	if (tsm_tbl == TSM_NULL_TABLE) {
		FATAL("TSM", 0x01);
	}

	/* assign state table */
	tsm_tbl->table = tbl;

	/* init state */
	tsm_tran(tsm_tbl, state);
}

void tsm_dispatch(tsm_tbl_t* tsm_tbl, ak_msg_t* msg) {
	tsm_t* respective_table = tsm_tbl->table[tsm_tbl->state];

	/* find tsm state respective */
	while (respective_table->sig != msg->sig &&
		   respective_table->sig !=  TSM_NULL_MSG) {
		respective_table++;
	}

	/* checking and updating the next state */
	if (respective_table->next_state != tsm_tbl->state &&
			respective_table->next_state != TSM_NULL_STATE) {
		tsm_tran(tsm_tbl, respective_table->next_state);
	}

	/* handle the message which respective state */
	if (respective_table->tsm_func != TSM_NULL_ROUTINE) {
		respective_table->tsm_func(msg);
	}
}

void tsm_tran(tsm_tbl_t *tsm_tbl, tsm_state_t state) {
	tsm_tbl->state = state;

	/* signal state changed */
	if (tsm_tbl->on_state != TSM_NULL_ON_STATE) {
		tsm_tbl->on_state(tsm_tbl->state);
	}
}
