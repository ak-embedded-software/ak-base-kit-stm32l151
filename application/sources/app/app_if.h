/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   30/1/2017
 * @brief:  app external signal define
 ******************************************************************************
**/
#ifndef __APP_IF_H__
#define __APP_IF_H__
#include <stdint.h>

/*****************************************************************************/
/*  task GW_RF24 define.
 */
/*****************************************************************************/
/* define timer */
#define GW_RF24_IF_TIMER_PACKET_DELAY_INTERVAL		(100)

/* define signal */
enum {
	GW_RF24_IF_PURE_MSG_OUT = AK_USER_DEFINE_SIG,
	GW_RF24_IF_COMMON_MSG_OUT,
	GW_RF24_IF_TIMER_PACKET_DELAY,
};

/*****************************************************************************/
/* task GW_IF define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	GW_CPU_SERIAL_IF_PURE_MSG_IN = AK_USER_DEFINE_SIG,
	GW_CPU_SERIAL_IF_PURE_MSG_OUT,
	GW_CPU_SERIAL_IF_COMMON_MSG_IN,
	GW_CPU_SERIAL_IF_COMMON_MSG_OUT,
	GW_CPU_SERIAL_IF_DYNAMIC_MSG_IN,
	GW_CPU_SERIAL_IF_DYNAMIC_MSG_OUT,
};

/*****************************************************************************/
/*  task GW_CONSOLE define
 */
/*****************************************************************************/
/* define timer */

/* define signal */
enum {
	GW_CONSOLE_INTERNAL_LOGIN_CMD = AK_USER_DEFINE_SIG,
};

/*****************************************************************************/
/* task GW_IF define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	GW_IF_PURE_MSG_IN = AK_USER_DEFINE_SIG,
	GW_IF_PURE_MSG_OUT,
	GW_IF_COMMON_MSG_IN,
	GW_IF_COMMON_MSG_OUT,
	GW_IF_DYNAMIC_MSG_IN,
	GW_IF_DYNAMIC_MSG_OUT,
};

/*****************************************************************************/
/* task GW_FW define
 */
/*****************************************************************************/
/* define timer */
#define GW_TIMER_FIRMWARE_PACKED_TIMEOUT_INTERVAL			(10000)		/* 10s */
#define GW_TIMER_FIRMWARE_GET_FIRMWARE_INFO_TIMEOUT			(10000)		/* 10s */
#define GW_TIMER_FIRMWARE_DEV_INTERNAL_UPDATE_TIMEOUT		(180000)	/* 3' */

/* define signal */
enum {
	GW_FW_OTA_REQ = AK_USER_DEFINE_SIG,
	GW_FW_CURRENT_INFO_RES,
	GW_FW_UPDATE_RES_OK,
	GW_FW_TRANFER_REQ,
	GW_FW_TRANSFER_RES_OK,
	GW_FW_TRANSFER_CHECKSUM_ERR,
	GW_FW_INTERNAL_UPDATE_REQ,
	GW_FW_UPDATE_BUSY,
	GW_FW_PACKED_TIMEOUT,
	GW_FW_UPDATE_COMPLETED,
	GW_FW_SM_UPDATE_RES_OK,
	GW_FW_SM_RELEASE_RES_OK,
	GW_FW_GET_FIRMWARE_INFO_TIMEOUT,
	GW_FW_DEV_INTERNAL_UPDATE_TIMEOUT,
};

/*****************************************************************************/
/*  task GW_TASK_DEBUG_MSG define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	GW_DEBUG_1 = AK_USER_DEFINE_SIG,
};


#endif //__APP_IF_H__
