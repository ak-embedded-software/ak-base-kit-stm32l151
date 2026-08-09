/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   02/11/2017
 ******************************************************************************
**/
#include <stdbool.h>
#include <stdint.h>

#include "app.h"
#include "app_dbg.h"
#include "app_data.h"

#include "task_list.h"

#include "fsm.h"
#include "port.h"
#include "message.h"
#include "timer.h"

#include "sys_dbg.h"
#include "sys_irq.h"
#include "sys_io.h"

#include "fifo.h"

#include "nrf_config.h"
#include "nrf_nwk_sig.h"
#include "nrf_phy.h"
#include "nrf_data.h"

#include "hal_nrf.h"

static uint8_t pload_frame_buffer[MAX_PHY_PAYLOAD_LEN];

#define PHY_STATE_HARDWARE_NONE		0
#define PHY_STATE_HARDWARE_STARTED	1

static uint8_t phy_state = PHY_STATE_HARDWARE_NONE;

void sys_irq_nrf24l01() {
	if (phy_state == PHY_STATE_HARDWARE_NONE) {
		return;
	}

	uint8_t nrf24_irq_mask = hal_nrf_get_clear_irq_flags();

	switch (nrf24_irq_mask) {
	case (1 << HAL_NRF_MAX_RT): { /* Max retries reached */
		hal_nrf_flush_tx(); /* flush tx fifo, avoid fifo jam */
		task_post_pure_msg(AC_RF24_PHY_ID, AC_RF24_PHY_IRQ_TX_MAX_RT);
	}
		break;

	case (1 << HAL_NRF_TX_DS): { /* Packet sent */
		task_post_pure_msg(AC_RF24_PHY_ID, AC_RF24_PHY_IRQ_TX_DS);
	}
		break;

	case (1 << HAL_NRF_RX_DR): /* Packet received */
		if (!hal_nrf_rx_fifo_empty()) {
			uint8_t pl_len;
			pl_len = hal_nrf_read_rx_pload(pload_frame_buffer);
			if (pl_len == MAX_PHY_PAYLOAD_LEN) {
				task_post_common_msg(AC_RF24_PHY_ID, AC_RF24_PHY_IRQ_RX_DR, pload_frame_buffer, MAX_PHY_PAYLOAD_LEN);
			}
			else {
				FATAL("PHY", 0x01);
			}
		}
		break;

	case ((1 << HAL_NRF_RX_DR) | ( 1 << HAL_NRF_TX_DS)): { /* Ack payload recieved */
		if (!hal_nrf_rx_fifo_empty()) {
			hal_nrf_read_rx_pload(pload_frame_buffer);
			task_post_common_msg(AC_RF24_PHY_ID, AC_RF24_PHY_IRQ_ACK_PR, pload_frame_buffer, MAX_PHY_PAYLOAD_LEN);
		}
	}
		break;

	default: {
		task_post_pure_msg(AC_RF24_PHY_ID, AC_RF24_PHY_IRQ_CLEAR_REQ);
	}
		break;
	}
}

void task_rf24_phy(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_RF24_PHY_INIT: {
		NRF_DBG_SIG("AC_RF24_PHY_INIT\n");

		/* init io control of nrf24 (CE, NCS, IRQ) */
		nrf24l01_io_ctrl_init();

		nrf24l01_spi_ctrl_init();

		CE_LOW();
		sys_ctrl_delay_ms(100);

		hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);
		hal_nrf_get_clear_irq_flags();

		hal_nrf_close_pipe(HAL_NRF_ALL); /* First close all radio pipes, Pipe 0 and 1 open by default */
		hal_nrf_open_pipe(HAL_NRF_PIPE0, true); /* Open pipe0, without/autoack (autoack) */

		hal_nrf_set_crc_mode(HAL_NRF_CRC_16BIT); /* Operates in 16bits CRC mode */
		hal_nrf_set_auto_retr(5, 750); /* Enable auto retransmit */

		hal_nrf_set_address_width(HAL_NRF_AW_5BYTES); /* 5 bytes address width */
		hal_nrf_set_address(HAL_NRF_TX, (uint8_t*)nrf_get_src_phy_addr()); /* Set device's addresses */
		hal_nrf_set_address(HAL_NRF_PIPE0, (uint8_t*)nrf_get_src_phy_addr()); /* Sets recieving address on pipe0 */

		hal_nrf_set_operation_mode(HAL_NRF_PRX);
		hal_nrf_set_rx_pload_width((uint8_t)HAL_NRF_PIPE0, 32);

		hal_nrf_set_rf_channel(NRF_PHY_CHANEL_CFG);
		hal_nrf_set_output_power(HAL_NRF_0DBM);
		hal_nrf_set_lna_gain(HAL_NRF_LNA_HCURR);
		hal_nrf_set_datarate(HAL_NRF_2MBPS);

		hal_nrf_set_power_mode(HAL_NRF_PWR_UP); /* Power up device */

		hal_nrf_set_irq_mode(HAL_NRF_MAX_RT, true);
		hal_nrf_set_irq_mode(HAL_NRF_TX_DS, true);
		hal_nrf_set_irq_mode(HAL_NRF_RX_DR, true);

		hal_nrf_flush_rx();
		hal_nrf_flush_tx();

		sys_ctrl_delay_ms(2);
		CE_HIGH();

		ENTRY_CRITICAL();
		phy_state = PHY_STATE_HARDWARE_STARTED;
		EXIT_CRITICAL();
	}
		break;

	case AC_RF24_PHY_IRQ_TX_MAX_RT: {
		NRF_DBG_SIG("AC_RF24_PHY_IRQ_TX_MAX_RT\n");
		task_post_pure_msg(AC_RF24_MAC_ID, AC_RF24_MAC_SEND_FRAME_ERR);
	}
		break;

	case AC_RF24_PHY_IRQ_TX_DS: {
		NRF_DBG_SIG("AC_RF24_PHY_IRQ_TX_DS\n");
		task_post_pure_msg(AC_RF24_MAC_ID, AC_RF24_MAC_SEND_FRAME_DONE);
	}
		break;

	case AC_RF24_PHY_IRQ_RX_DR: {
		NRF_DBG_SIG("AC_RF24_PHY_IRQ_RX_DR\n");
		msg_inc_ref_count(msg);
		set_msg_sig(msg, AC_RF24_MAC_RECV_FRAME);
		task_post(AC_RF24_MAC_ID, msg);
	}
		break;

	case AC_RF24_PHY_IRQ_ACK_PR: {
		NRF_DBG_SIG("AC_RF24_PHY_IRQ_ACK_PR\n");
	}
		break;

	case AC_RF24_PHY_IRQ_CLEAR_REQ: {
		NRF_DBG_SIG("AC_RF24_PHY_IRQ_CLEAR_REQ\n");
		hal_nrf_get_clear_irq_flags();
	}
		break;

	case AC_RF24_PHY_SEND_FRAME_REQ: {
		NRF_DBG_SIG("AC_RF24_PHY_SEND_FRAME_REQ\n");
		uint8_t* payload = get_data_common_msg(msg);
		hal_nrf_set_address(HAL_NRF_TX, (uint8_t*)nrf_get_des_phy_addr()); /* Set device's addresses */
		hal_nrf_set_address(HAL_NRF_PIPE0, (uint8_t*)nrf_get_des_phy_addr()); /* Sets recieving address on pipe0 */
		hal_nrf_write_tx_pload(payload, MAX_PHY_PAYLOAD_LEN);
	}
		break;

	case AC_RF24_PHY_REV_MODE_REQ: {
		NRF_DBG_SIG("AC_RF24_PHY_REV_MODE_REQ\n");
		nrf_phy_switch_prx_mode();
	}
		break;

	case AC_RF24_PHY_SEND_MODE_REQ: {
		NRF_DBG_SIG("AC_RF24_PHY_SEND_MODE_REQ\n");
		nrf_phy_switch_ptx_mode();
	}
		break;

	default:
		break;
	}
}

void nrf_phy_switch_ptx_mode() {
	NRF_DBG("[PHY] nrf_phy_switch_ptx_mode()\n");
	CE_LOW();
	hal_nrf_set_operation_mode(HAL_NRF_PTX);
	sys_ctrl_delay_us(150);
	CE_HIGH();
}

void nrf_phy_switch_prx_mode() {
	NRF_DBG("[PHY] nrf_phy_switch_prx_mode()\n");
	CE_LOW();
	hal_nrf_set_operation_mode(HAL_NRF_PRX);
	hal_nrf_set_address(HAL_NRF_PIPE0, (uint8_t*)nrf_get_src_phy_addr()); /* Sets recieving address on pipe0 */
	sys_ctrl_delay_us(150);
	CE_HIGH();
}

