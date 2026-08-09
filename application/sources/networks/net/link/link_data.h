/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   08/10/2017
 ******************************************************************************
**/
#ifndef __LINK_DATA_H__
#define __LINK_DATA_H__

#include <stdint.h>
#include "link_config.h"

/* define type of link messages */
#define LINK_MSG_TYPE_PURE			(1)
#define LINK_MSG_TYPE_COMMON		(2)
#define LINK_MSG_TYPE_DYNAMIC		(3)
#define LINK_MSG_TYPE_DATA			(4)

/* define static link pdu */
#define LINK_PDU_NULL				((link_pdu_t*)0)

typedef struct link_pdu_t {
	struct link_pdu_t* next;
	uint32_t id;
	uint32_t is_used;
	uint32_t len;
	uint8_t payload[LINK_PDU_BUF_SIZE];
} link_pdu_t;

/* link pdu function */
extern void link_pdu_init();
extern link_pdu_t* link_pdu_malloc();
extern void link_pdu_free(link_pdu_t*);
extern link_pdu_t* link_pdu_get(uint32_t);
extern void link_pdu_free(uint32_t);

/* link address utilities */
extern void link_set_src_addr(uint32_t);
extern uint32_t link_get_src_addr();
extern void link_set_des_addr(uint32_t);
extern uint32_t link_get_des_addr();

#endif //__LINK_DATA_H__
