/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   08/10/2017
 ******************************************************************************
**/
#ifndef __LINK_SIG_H__
#define __LINK_SIG_H__

#include "ak.h"

/*****************************************************************************/
/*  LINK_PHY task define
 */
/*****************************************************************************/
/* private define */
/* define timer */
/* define signal */
enum {
	/* public */
	AC_LINK_PHY_INIT = AK_USER_DEFINE_SIG,
	AC_LINK_PHY_FRAME_SEND_REQ,

	/* private */
	AC_LINK_PHY_FRAME_SEND_TO,
	AC_LINK_PHY_FRAME_REV,
	AC_LINK_PHY_FRAME_REV_TO,
	AC_LINK_PHY_FRAME_REV_CS_ERR,
};

/*****************************************************************************/
/*  LINK_MAC task define
 */
/*****************************************************************************/
/* private define */
/* define timer */
/* define signal */
enum {
	/* public */
	AC_LINK_MAC_INIT = AK_USER_DEFINE_SIG,
	AC_LINK_MAC_PHY_LAYER_STARTED,
	AC_LINK_MAC_FRAME_SEND_REQ,

	/* private */
	AC_LINK_MAC_FRAME_SEND_START,
	AC_LINK_MAC_FRAME_SEND_DONE,
	AC_LINK_MAC_FRAME_SEND_ERR,
	AC_LINK_MAC_FRAME_REV,
	AC_LINK_MAC_FRAME_REV_TO,
};

/*****************************************************************************/
/*  LINK task define
 */
/*****************************************************************************/
/* private define */
/* define timer */
/* define signal */
enum {
	/* public */
	AC_LINK_INIT = AK_USER_DEFINE_SIG,
	AC_LINK_MAC_LAYER_STARTED,
	AC_LINK_SEND_PURE_MSG,
	AC_LINK_SEND_COMMON_MSG,
	AC_LINK_SEND_DYNAMIC_MSG,
	AC_LINK_SEND_DATA,
	AC_LINK_SEND_HANDLE_PDU_FULL,

	/* private */
	AC_LINK_SEND_DONE,
	AC_LINK_SEND_ERR,

	AC_LINK_REV_MSG,
};

#endif //__LINK_SIG_H__
