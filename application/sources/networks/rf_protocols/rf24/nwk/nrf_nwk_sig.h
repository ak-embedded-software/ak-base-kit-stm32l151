/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   02/11/2017
 ******************************************************************************
**/
#ifndef __NRF_NWK_SIG_H__
#define __NRF_NWK_SIG_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "ak.h"

/*****************************************************************************/
/*  RF24_PHY task define
 */
/*****************************************************************************/
/* private define */
/* define timer */
/* define signal */
enum {
	AC_RF24_PHY_INIT = AK_USER_DEFINE_SIG,
	AC_RF24_PHY_IRQ_TX_MAX_RT,
	AC_RF24_PHY_IRQ_TX_DS,
	AC_RF24_PHY_IRQ_RX_DR,
	AC_RF24_PHY_IRQ_ACK_PR,
	AC_RF24_PHY_SEND_FRAME_REQ,
	AC_RF24_PHY_IRQ_CLEAR_REQ,
	AC_RF24_PHY_REV_MODE_REQ,
	AC_RF24_PHY_SEND_MODE_REQ,
};

/*****************************************************************************/
/*  RF24_MAC task define
 */
/*****************************************************************************/
/* private define */
/* define timer */
/* define signal */
enum {
	AC_RF24_MAC_INIT = AK_USER_DEFINE_SIG,
	AC_RF24_MAC_HANDLE_MSG_OUT,
	AC_RF24_MAC_SEND_FRAME,
	AC_RF24_MAC_SEND_FRAME_DONE,
	AC_RF24_MAC_SEND_FRAME_ERR,
	AC_RF24_MAC_RECV_FRAME,
	AC_RF24_MAC_RECV_FRAME_TO,
	AC_RF24_MAC_HANDLE_MSG_IN,
};

/*****************************************************************************/
/*  RF24_NWK task define
 */
/*****************************************************************************/
/* private define */
/* define timer */
/* define signal */
enum {
	AC_RF24_NWK_INIT = AK_USER_DEFINE_SIG,
	AC_RF24_NWK_PDU_FULL,
	AC_RF24_NWK_PURE_MSG_OUT,
	AC_RF24_NWK_COMMON_MSG_OUT,
	AC_RF24_NWK_DYNAMIC_MSG_OUT,
	AC_RF24_NWK_DATA_MSG_OUT,
	AC_RF24_NWK_SEND_MSG_DONE,
	AC_RF24_NWK_RECV_MSG,
	AC_RF24_NWK_SEND_MSG_ERR_SDF,
	AC_RF24_NWK_SEND_MSG_ERR_BUSY,
};

#ifdef __cplusplus
}
#endif

#endif //__NRF_NWK_SIG_H__
