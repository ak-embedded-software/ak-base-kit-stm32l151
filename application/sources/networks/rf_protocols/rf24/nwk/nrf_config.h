/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   02/11/2017
 ******************************************************************************
**/
#ifndef __NRF_CONFIG_H__
#define __NRF_CONFIG_H__

#define NRF_NWK_MSG_POOL_SIZE			3
#define NRF_NWK_MSG_MAX_LEN				256

#define NRF_PHY_CHANEL_CFG				120

#define MAX_PHY_PAYLOAD_LEN				32

/* DEBUG */
#define NRF_DBG_SIG_EN		0
#define NRF_DBG_DATA_EN		0
#define NRF_DBG_EN			0

#if (NRF_DBG_SIG_EN == 1)
#define NRF_DBG_SIG(fmt, ...)       xprintf("-LSIG-> " fmt, ##__VA_ARGS__)
#else
#define NRF_DBG_SIG(fmt, ...)
#endif

#if (NRF_DBG_DATA_EN == 1)
#define NRF_DBG_DATA(fmt, ...)       xprintf(fmt, ##__VA_ARGS__)
#else
#define NRF_DBG_DATA(fmt, ...)
#endif

#if (NRF_DBG_EN == 1)
#define NRF_DBG(fmt, ...)       xprintf(fmt, ##__VA_ARGS__)
#else
#define NRF_DBG(fmt, ...)
#endif

#endif // __NRF_CONFIG_H__
