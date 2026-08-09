/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   02/11/2017
 ******************************************************************************
**/
#ifndef __TASK_NRF_NWK_H__
#define __TASK_NRF_NWK_H__

#include <stdint.h>

#include "ak.h"
#include "port.h"
#include "message.h"
#include "fsm.h"

extern fsm_t nrf_nwk_fsm;
extern void nrf_nwk_fsm_init(ak_msg_t*);
extern void nrf_nwk_fsm_idle(ak_msg_t*);

#endif // __TASK_NRF_NWK_H__
