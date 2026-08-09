/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   08/10/2017
 ******************************************************************************
**/
#include <stdint.h>
#include "ak.h"
#include "port.h"
#include "sys_dbg.h"

#include "link_config.h"
#include "link_data.h"

static uint32_t mac_src_add = 0xFFFFFFFF;
static uint32_t mac_des_add = 0xFFFFFFFF;

static link_pdu_t* free_link_pdu_pool;
static link_pdu_t link_pdu_pool[LINK_PDU_POOL_SIZE];

static void link_pdu_fatal(const char* s, uint8_t c);

/* link pdu function */
void link_pdu_init() {
	LINK_DBG_DATA("[LINK_DATA] link_pdu_init()\n");
	ENTRY_CRITICAL();
	free_link_pdu_pool = (link_pdu_t*)link_pdu_pool;
	for (uint32_t i = 0; i < LINK_PDU_POOL_SIZE; i++) {
		link_pdu_pool[i].id = i;
		link_pdu_pool[i].is_used = 0;

		if (i == (LINK_PDU_POOL_SIZE - 1)) {
			link_pdu_pool[i].next = LINK_PDU_NULL;
		}
		else {
			link_pdu_pool[i].next = (link_pdu_t*)&link_pdu_pool[i + 1];
		}
	}
	EXIT_CRITICAL();
}

link_pdu_t* link_pdu_malloc() {
	ENTRY_CRITICAL();
	link_pdu_t* allocate_msg = free_link_pdu_pool;
	if (allocate_msg == LINK_PDU_NULL) {
		EXIT_CRITICAL();
		LINK_DBG_DATA("[LINK_DATA] LINK_PDU_NULL == link_pdu_malloc()\n");
		return allocate_msg;
	}
	else {
		allocate_msg->is_used = 1;
		free_link_pdu_pool = free_link_pdu_pool->next;
	}
	EXIT_CRITICAL();
	LINK_DBG_DATA("[LINK_DATA] link_pdu_malloc(%d)\n", allocate_msg->id);
	return allocate_msg;
}

void link_pdu_free(link_pdu_t* link_pdu) {
	LINK_DBG_DATA("[LINK_DATA] link_pdu_free(%d)\n", link_pdu->id);
	ENTRY_CRITICAL();
	if ((link_pdu != LINK_PDU_NULL) && \
			(link_pdu->id < LINK_PDU_POOL_SIZE) && \
			link_pdu->is_used) {
		link_pdu->is_used = 0;
		link_pdu->next = free_link_pdu_pool;
		free_link_pdu_pool = link_pdu;
	}
	else {
		link_pdu_fatal("LINK_PDU", 0x04);
	}
	EXIT_CRITICAL();
}

link_pdu_t* link_pdu_get(uint32_t pdu_id) {
	LINK_DBG_DATA("[LINK_DATA] link_pdu_get(%d)\n", pdu_id);
	link_pdu_t* link_pdu = LINK_PDU_NULL;
	ENTRY_CRITICAL();
	if ((pdu_id < LINK_PDU_POOL_SIZE) && \
			link_pdu_pool[pdu_id].is_used) {
		link_pdu = (link_pdu_t*)&link_pdu_pool[pdu_id];
	}
	else {
		link_pdu_fatal("LINK_PDU", 0x03);
	}
	EXIT_CRITICAL();
	return link_pdu;
}

void link_pdu_free(uint32_t pdu_id) {
	LINK_DBG_DATA("[LINK_DATA] link_pdu_free(%d)\n", pdu_id);
	ENTRY_CRITICAL();
	if (pdu_id < LINK_PDU_POOL_SIZE && link_pdu_pool[pdu_id].is_used) {
		link_pdu_pool[pdu_id].is_used = 0;
		link_pdu_pool[pdu_id].next = free_link_pdu_pool;
		free_link_pdu_pool = &link_pdu_pool[pdu_id];
	}
	else {
		link_pdu_fatal("LINK_PDU", 0x02);
	}
	EXIT_CRITICAL();
}

/* link address utilities */
void link_set_src_addr(uint32_t addr) {
	LINK_DBG_DATA("[LINK_DATA] link_set_src_addr(%d)\n", addr);
	ENTRY_CRITICAL();
	mac_src_add = addr;
	EXIT_CRITICAL();
}

uint32_t link_get_src_addr() {
	uint32_t ret_addr;
	ENTRY_CRITICAL();
	ret_addr = mac_src_add;
	EXIT_CRITICAL();
	return ret_addr;
}

void link_set_des_addr(uint32_t addr) {
	LINK_DBG_DATA("[LINK_DATA] link_set_des_addr(%d)\n", addr);
	ENTRY_CRITICAL();
	mac_des_add = addr;
	EXIT_CRITICAL();
}

uint32_t link_get_des_addr() {
	uint32_t ret_addr;
	ENTRY_CRITICAL();
	ret_addr = mac_des_add;
	EXIT_CRITICAL();
	return ret_addr;
}

void link_pdu_fatal(const char* s, uint8_t c) {
	for (uint32_t i = 0; i < LINK_PDU_POOL_SIZE; i++) {
		LINK_DBG_DATA("id: %d, is_used: %d, len: %d, data:", link_pdu_pool[i].id, link_pdu_pool[i].is_used, link_pdu_pool[i].len);
		for (uint32_t i = 0; i < link_pdu_pool[i].len; i++) {
			LINK_DBG_DATA("%d ", link_pdu_pool[i].payload[i]);
		}
		LINK_DBG_DATA("\n");
	}
	FATAL((const int8_t*)s, c);
}
