/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   08/10/2017
 ******************************************************************************
**/
#ifndef __LINK_PHY_H__
#define __LINK_PHY_H__

#include "fsm.h"
#include "message.h"

#include "link_config.h"

#define LINK_PHY_SOF	0xEF

#define LINK_PHY_FRAME_HEADER_SIZE	(sizeof(link_phy_frame_header_t))
#define LINK_PHY_FRAME_SIZE			(sizeof(link_phy_frame_t))

#define LINK_PHY_FRAME_DATA_SIZE	(AK_COMMON_MSG_DATA_SIZE - LINK_PHY_FRAME_HEADER_SIZE)

typedef struct {
	uint8_t sof; /* start of frame */
	uint32_t des_addr; /* destination address */
	uint32_t src_addr; /* sources address */
	uint8_t type; /* type of frame */
	uint8_t sub_type; /* sub-type of frame */
	uint8_t seq_num; /* frame sequence number */
	uint8_t len; /* len of data */
	uint8_t fcs; /* frame checksum NOTE: this field must be tail */
} __AK_PACKETED link_phy_frame_header_t;

typedef struct {
	link_phy_frame_header_t header;
	uint8_t data[LINK_PHY_FRAME_DATA_SIZE]; /* frame data buffer */
} __AK_PACKETED link_phy_frame_t;

extern fsm_t fsm_link_phy;

extern void fsm_link_phy_state_init(ak_msg_t* msg);
extern void fsm_link_phy_state_handle(ak_msg_t* msg);

extern void link_phy_max_retry_set(uint8_t);
extern uint8_t link_phy_max_retry_get();

extern uint32_t link_phy_get_send_frame_to();

#endif //__LINK_PHY_H__
