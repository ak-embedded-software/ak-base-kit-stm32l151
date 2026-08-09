/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   02/11/2017
 ******************************************************************************
**/
#include <stdint.h>
#include <string.h>

#include "ak.h"
#include "port.h"
#include "sys_dbg.h"

#include "nrf_data.h"

static uint8_t mac_src_addr[5] = {0x3c, 0x3c, 0x3c, 0, 0};
static uint8_t mac_des_addr[5] = {0x3c, 0x3c, 0x3c, 0, 0};

static nrf_nwk_pdu_t* free_nrf_nwk_pdu_list;
static nrf_nwk_pdu_t nrf_nwk_msg_pool[NRF_NWK_MSG_POOL_SIZE];

void nrf_nwk_pdu_init() {
	free_nrf_nwk_pdu_list = (nrf_nwk_pdu_t*)nrf_nwk_msg_pool;

	for (uint32_t i = 0; i < NRF_NWK_MSG_POOL_SIZE; i++) {
		nrf_nwk_msg_pool[i].id = i;
		nrf_nwk_msg_pool[i].is_used = 0;

		if (i == (NRF_NWK_MSG_POOL_SIZE - 1)) {
			nrf_nwk_msg_pool[i].next = NRF_NWK_PDU_NULL;
		}
		else {
			nrf_nwk_msg_pool[i].next = (nrf_nwk_pdu_t*)&nrf_nwk_msg_pool[i + 1];
		}
	}
}

nrf_nwk_pdu_t* nrf_nwk_pdu_malloc() {
	nrf_nwk_pdu_t* allocate_msg = 	free_nrf_nwk_pdu_list;
	ENTRY_CRITICAL();
	if (allocate_msg == NRF_NWK_PDU_NULL) {
		FATAL("NWK_PDU", 0x01);
	}
	else {
		allocate_msg->is_used = 1;
		free_nrf_nwk_pdu_list = free_nrf_nwk_pdu_list->next;
	}
	EXIT_CRITICAL();
	NRF_DBG_DATA("nrf_nwk_pdu_malloc(%d)\n", allocate_msg->id);
	return allocate_msg;
}

void nrf_nwk_pdu_free(nrf_nwk_pdu_t* nwk_msg) {
	NRF_DBG_DATA("nrf_nwk_pdu_free(%d)\n", nwk_msg->id);
	ENTRY_CRITICAL();
	nwk_msg->is_used = 0;
	nwk_msg->next = free_nrf_nwk_pdu_list;
	free_nrf_nwk_pdu_list = nwk_msg;
	EXIT_CRITICAL();
}

nrf_nwk_pdu_t* nrf_nwk_pdu_get(uint32_t id) {
	NRF_DBG_DATA("nrf_nwk_pdu_get(%d)\n", id);
	return (nrf_nwk_pdu_t*)&nrf_nwk_msg_pool[id];
}

void nrf_nwk_pdu_free(uint32_t id) {
	NRF_DBG_DATA("nrf_nwk_pdu_free(%d)\n", id);
	ENTRY_CRITICAL();
	if (id < NRF_NWK_MSG_POOL_SIZE) {
		nrf_nwk_msg_pool[id].is_used = 0;
		nrf_nwk_msg_pool[id].next = free_nrf_nwk_pdu_list;
		free_nrf_nwk_pdu_list = &nrf_nwk_msg_pool[id];
	}
	else {
		FATAL("NWK_PDU", 0x02);
	}
	EXIT_CRITICAL();
}

void nrf_set_static_nwk_addr(uint16_t addr) {
	mac_src_addr[3] = ((addr << 8) & 0xFF);
	mac_src_addr[4] = (addr& 0xFF);
	NRF_DBG_DATA("[NRF_DATA] nrf_set_static_nwk_addr(%02X %02X %02X %02X %02X)\n", \
			mac_src_addr[0],
			mac_src_addr[1],
			mac_src_addr[2],
			mac_src_addr[3],
			mac_src_addr[4]);
}

uint16_t nrf_get_static_nwk_addr() {
	uint16_t ret;
	*((uint8_t*)&ret) = mac_src_addr[3];
	*((uint8_t*)&ret + 1) = mac_src_addr[4];
	NRF_DBG_DATA("[NRF_DATA] nrf_get_static_nwk_addr(%02X %02X %02X %02X %02X)\n", \
			mac_src_addr[0],
			mac_src_addr[1],
			mac_src_addr[2],
			mac_src_addr[3],
			mac_src_addr[4]);
	return ret;
}

void nrf_set_des_nwk_addr(uint16_t addr) {
	mac_des_addr[3] = ((addr << 8) & 0xFF);
	mac_des_addr[4] = (addr& 0xFF);
	NRF_DBG_DATA("[NRF_DATA] nrf_set_des_nwk_addr(%02X %02X %02X %02X %02X)\n", \
			mac_des_addr[0],
			mac_des_addr[1],
			mac_des_addr[2],
			mac_des_addr[3],
			mac_des_addr[4]);
}

uint16_t nrf_get_des_nwk_addr() {
	uint16_t ret;
	*((uint8_t*)&ret) = mac_des_addr[3];
	*((uint8_t*)&ret + 1) = mac_des_addr[4];
	NRF_DBG_DATA("[NRF_DATA] nrf_get_des_nwk_addr(%02X %02X %02X %02X %02X)\n", \
			mac_des_addr[0],
			mac_des_addr[1],
			mac_des_addr[2],
			mac_des_addr[3],
			mac_des_addr[4]);
	return ret;
}

uint8_t* nrf_get_des_phy_addr() {
	NRF_DBG_DATA("[NRF_DATA] nrf_get_des_phy_addr(%02X %02X %02X %02X %02X)\n", \
			mac_des_addr[0],
			mac_des_addr[1],
			mac_des_addr[2],
			mac_des_addr[3],
			mac_des_addr[4]);
	return mac_des_addr;
}

uint8_t* nrf_get_src_phy_addr() {
	NRF_DBG_DATA("[NRF_DATA] nrf_get_src_phy_addr(%02X %02X %02X %02X %02X)\n", \
			mac_src_addr[0],
			mac_src_addr[1],
			mac_src_addr[2],
			mac_src_addr[3],
			mac_src_addr[4]);
	return mac_src_addr;
}
