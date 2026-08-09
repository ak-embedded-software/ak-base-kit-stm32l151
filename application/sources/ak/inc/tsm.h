/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   12/02/2017
 * @brief:  table state machine.
 ******************************************************************************
**/

#ifndef __TSM_H__
#define __TSM_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "ak.h"
#include "message.h"

#define TSM_NULL_MSG			((uint8_t)0)
#define TSM_NULL_STATE			((tsm_state_t)0xFF)
#define TSM_NULL_ROUTINE		((tsm_func_f)0)
#define TSM_NULL_ON_STATE		((tsm_on_state_f)0)
#define TSM_NULL_TABLE			((tsm_tbl_t*)0)

typedef uint8_t tsm_state_t;
typedef void (*tsm_func_f)(ak_msg_t*);
typedef void (*tsm_on_state_f)(tsm_state_t);

typedef struct {
	uint8_t sig;				/* signal */
	tsm_state_t next_state;		/* next state */
	tsm_func_f tsm_func;		/* state handler function */
} tsm_t;

typedef struct tsm_tbl_t {
	tsm_state_t state;			/* The index of state table */
	tsm_on_state_f on_state;	/* The callback function will be called when state changes */
	tsm_t** table;
} tsm_tbl_t;

#define TSM(t, tbl, s) tsm_init(t, tbl, s)
#define TSM_TRAN(t, s) tsm_tran(t, s)

void tsm_init(tsm_tbl_t* tsm_tbl, tsm_t** tbl, tsm_state_t state);
void tsm_tran(tsm_tbl_t* tsm_tbl, tsm_state_t state);
void tsm_dispatch(tsm_tbl_t* tsm_tbl, ak_msg_t* msg);

#ifdef __cplusplus
}
#endif

#endif //__TSM_H__
