/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   08/10/2017
 ******************************************************************************
**/
#ifndef __LINK_H__
#define __LINK_H__

#include <stdint.h>
#include "ak.h"
#include "fsm.h"

#include "link_data.h"

#define LINK_DATA_BUF_SIZE	(LINK_PDU_BUF_SIZE - sizeof(link_frame_header_t))

typedef enum {
	/* private */
	LINK_FRAME_TYPE_NONE,

	/* pulic */
	LINK_FRAME_TYPE_PURE_MSG,
	LINK_FRAME_TYPE_COMMON_MSG,
	LINK_FRAME_TYPE_DYNAMIC_MSG,
	LINK_FRAME_TYPE_DATA,
} link_frame_type_e;

typedef enum {
	/* private */
	LINK_FRAME_SUB_TYPE_NONE,
	/* pulic */
} link_frame_sub_type_e;

typedef struct {
	uint32_t src_addr;
	uint32_t des_addr;
	uint8_t type;
	uint8_t sub_type;
	uint16_t len;
} __AK_PACKETED link_frame_header_t;

typedef struct {
	link_frame_header_t header;
	uint8_t data[LINK_DATA_BUF_SIZE];
} __AK_PACKETED link_frame_t;

extern void link_init_state_machine();

#endif //__LINK_H__

