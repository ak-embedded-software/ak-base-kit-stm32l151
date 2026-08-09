/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   13/08/2016
 * Mechanism of task scheduler is referenced by doc/Samek0607.pdf
 ******************************************************************************
**/

#ifndef __TASK_H__
#define __TASK_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

#include "ak.h"
#include "port.h"
#include "message.h"

#include "log_queue.h"

#define LOG_QUEUE_OBJECT_SIZE			(512)
#define LOG_QUEUE_IRQ_SIZE				(128)

typedef uint8_t	task_pri_t;
typedef uint8_t	task_id_t;
typedef void (*pf_task)(ak_msg_t*);
typedef void (*pf_task_polling)();

typedef struct {
	task_id_t id;
	task_pri_t pri;
	pf_task task;
} task_t;

typedef struct {
	task_id_t id;
	uint8_t ability;
	pf_task_polling task_polling;
} task_polling_t;

typedef struct {
	uint32_t except_number;
	uint32_t timestamp;
} exception_info_t;

/* active object queue */
#if defined(AK_TASK_OBJ_LOG_ENABLE)
extern log_queue_t log_task_dbg_object_queue;
#endif

/* exception log queue */
#if defined(AK_IRQ_OBJ_LOG_ENABLE)
extern log_queue_t log_irq_queue;
#endif

extern void task_create(task_t* task_tbl);
extern void task_post(task_id_t des_task_id, ak_msg_t* msg);
extern void task_post_pure_msg(task_id_t des_task_id, uint8_t sig);
extern void task_post_common_msg(task_id_t des_task_id, uint8_t sig, uint8_t* data, uint8_t len);
extern void task_post_dynamic_msg(task_id_t des_task_id, uint8_t sig, uint8_t* data, uint32_t len);
extern uint8_t task_remove_msg(task_id_t task_id, uint8_t sig); /* return numbers message's removed */
extern int task_init();
extern int task_run();

extern void task_polling_create(task_polling_t* task_polling_tbl);
extern void task_polling_set_ability(task_id_t task_polling_id, uint8_t ability);
extern void task_polling_run();

extern void task_entry_interrupt(); /* this function MUST-BE called when entry interrupt handler */
extern void task_exit_interrupt(); /* this function MUST-BE called when exit interrupt handler */

extern task_id_t task_self(); /* get current task id, except interrupt task */
extern task_id_t get_current_task_id(); /* get current task id include interrupt task */
extern task_t* get_current_task_info(); /* get current task info */
extern ak_msg_t* get_current_active_object(); /* get current active object info (active message) */

extern void task_irq_io_entry_trigger(); /* this function MUST-BE redefine */
extern void task_irq_io_exit_trigger() ; /* this function MUST-BE redefine */

extern void task_pri_queue_dump(); /* use only for debug */

#ifdef __cplusplus
}
#endif

#endif //__TASK_H__
