/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   13/08/2016
 * @brief:  Finite state machine.
 ******************************************************************************
**/

#include "fsm.h"

void fsm_dispatch(fsm_t* me, ak_msg_t* msg) {
	me->state(msg);
}
