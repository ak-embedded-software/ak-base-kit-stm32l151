/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   08/10/2017
 ******************************************************************************
**/
#ifndef __LINK_MAC_H__
#define __LINK_MAC_H__

#include "fsm.h"
#include "message.h"

#include "link_phy.h"
#include "link_config.h"

#define LINK_MAC_FRAME_HEADER_SIZE	(sizeof(link_mac_frame_header_t))
#define LINK_MAC_FRAME_DATA_SIZE	(LINK_PHY_FRAME_DATA_SIZE - LINK_MAC_FRAME_HEADER_SIZE)

typedef struct {
	uint32_t des_addr; /* destination address */
	uint32_t src_addr; /* sources address */
	uint8_t type; /* type of frame */
	uint8_t sub_type; /* sub-type of frame */
	uint8_t seq_num; /* frame sequence number */
	uint8_t fnum; /* frame number */
	uint8_t fidx; /* frame index */
	uint16_t len; /* len of data */
	uint8_t fcs; /* frame checksum NOTE: this field must be tail */
} __AK_PACKETED link_mac_frame_header_t;

typedef struct {
	link_mac_frame_header_t header;
	uint8_t data[LINK_MAC_FRAME_DATA_SIZE];
} __AK_PACKETED link_mac_frame_t;

extern fsm_t fsm_link_mac;
extern void fsm_link_mac_state_init(ak_msg_t*);
extern void fsm_link_mac_state_handle(ak_msg_t*);

#endif //__LINK_MAC_H__
