/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   02/11/2017
 ******************************************************************************
**/
#ifndef __NRF_DATA_H__
#define __NRF_DATA_H__

#include <stdint.h>
#include "nrf_config.h"
#include "nrf_nwk.h"
#include "nrf_mac.h"

#define RF24_MSG_TYPE_PURE				(1)
#define RF24_MSG_TYPE_COMMON			(2)
#define RF24_MSG_TYPE_DYNAMIC			(3)
#define RF24_MSG_TYPE_DATA				(4)

#define NRF_NWK_PDU_NULL				((nrf_nwk_pdu_t*)0)

typedef struct nrf_nwk_pdu_t {
	struct nrf_nwk_pdu_t* next;
	uint32_t id;
	uint32_t len;
	uint32_t is_used;
	uint8_t payload[NRF_NWK_MSG_MAX_LEN];
} nrf_nwk_pdu_t;

extern void nrf_nwk_pdu_init();
extern nrf_nwk_pdu_t* nrf_nwk_pdu_malloc();
extern void nrf_nwk_pdu_free(nrf_nwk_pdu_t*);
extern nrf_nwk_pdu_t* nrf_nwk_pdu_get(uint32_t);
extern void nrf_nwk_pdu_free(uint32_t);

extern void nrf_set_static_nwk_addr(uint16_t);
extern uint16_t nrf_get_static_nwk_addr();
extern void nrf_set_des_nwk_addr(uint16_t);
extern uint16_t nrf_get_des_nwk_addr();
extern uint8_t* nrf_get_des_phy_addr();
extern uint8_t* nrf_get_src_phy_addr();

#endif //__NRF_DATA_H__
