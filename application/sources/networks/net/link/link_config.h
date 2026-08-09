/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   08/10/2017
 ******************************************************************************
**/
#ifndef __LINK_CONFIG_H__
#define __LINK_CONFIG_H__

#include "ak.h"

#define LINK_PDU_BUF_SIZE			384
#define LINK_PDU_POOL_SIZE			4

#define LINK_PHY_FRAME_SEND_TO_INTERVAL		250 /* ms */
#define LINK_PHY_FRAME_REV_TO_INTERVAL		250 /* ms */

#define LINK_PHY_MAX_RETRY_SET_DEFAULT			1
#define LINK_MAC_PDU_SENDING_RETRY_COUNTER_MAX	1

/* DEBUG */
#define LINK_PRINT_EN		1
#define LINK_DBG_SIG_EN		0
#define LINK_DBG_DATA_EN	0
#define LINK_DBG_EN			0

#if (LINK_PRINT_EN == 1)
#define LINK_PRINT(fmt, ...)       xprintf("-LINK_PRINT- " fmt, ##__VA_ARGS__)
#else
#define LINK_PRINT(fmt, ...)
#endif

#if (LINK_DBG_SIG_EN == 1)
#define LINK_DBG_SIG(fmt, ...)       xprintf("-LSIG-> " fmt, ##__VA_ARGS__)
#else
#define LINK_DBG_SIG(fmt, ...)
#endif

#if (LINK_DBG_DATA_EN == 1)
#define LINK_DBG_DATA(fmt, ...)       xprintf(fmt, ##__VA_ARGS__)
#else
#define LINK_DBG_DATA(fmt, ...)
#endif

#if (LINK_DBG_EN == 1)
#define LINK_DBG(fmt, ...)       xprintf(fmt, ##__VA_ARGS__)
#else
#define LINK_DBG(fmt, ...)
#endif

#endif //__LINK_CONFIG_H__
