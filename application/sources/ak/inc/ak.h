/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   13/08/2016
 * @brief:  Main defination of active kernel
 ******************************************************************************
**/

#ifndef __AK_H__
#define __AK_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

#include "fsm.h"
#include "task.h"
#include "port.h"

/*****************************************************************************
 * DEFINITION: active kernel
 *
 *****************************************************************************/
#define AK_VERSION						"1.3"
#define AK_ENABLE						(0x01)
#define AK_DISABLE						(0x00)

#define AK_FLAG_ON						(0x01)
#define AK_FLAG_OFF						(0x00)

#define AK_RET_OK						(0x01)
#define AK_RET_NG						(0x00)

/*****************************************************************************
 * DEFINITION: signals
 *
 *****************************************************************************/
#define AK_SYS_DEFINE_SIG				(0)
#define AK_USER_DEFINE_SIG				(10)

/*****************************************************************************
 * DEFINITION: tasking
 *
 *****************************************************************************/
#define TASK_PRI_MAX_SIZE				(8)

#define TASK_PRI_LEVEL_0				(0)
#define TASK_PRI_LEVEL_1				(1)
#define TASK_PRI_LEVEL_2				(2)
#define TASK_PRI_LEVEL_3				(3)
#define TASK_PRI_LEVEL_4				(4)
#define TASK_PRI_LEVEL_5				(5)
#define TASK_PRI_LEVEL_6				(6)
#define TASK_PRI_LEVEL_7				(7)

#define AK_TASK_INTERRUPT_ID			(0xEE)
#define AK_TASK_IDLE_ID					(0xEF)

#ifdef __cplusplus
}
#endif

#endif // __AK_H__
