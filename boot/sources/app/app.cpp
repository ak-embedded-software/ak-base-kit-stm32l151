/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   05/02/2017
 ******************************************************************************
**/
#include <string.h>

/* driver include */
#include "led.h"
#include "flash.h"

/* app include */
#include "app.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_flash.h"

/* sys include */
#include "sys_irq.h"
#include "sys_io.h"
#include "sys_ctrl.h"
#include "sys_dbg.h"
#include "sys_boot.h"
#include "sys_arduino.h"

#include "uart_boot.h"

led_t led_life;

const char* boot_version = BOOT_VER;

boot_app_share_data_t boot_app_share_data;
sys_boot_t app_sys_boot;

static void update_boot_fw_info_to_share_boot();
static void jump_to_application_before_reset_peripheral();

/**************************************************************************
* uart boot handler function declare
***************************************************************************/
void uart_boot_cmd_handshake_res(void*);
void uart_boot_cmd_update_res(void*);
void uart_boot_cmd_transfer_fw_res(void*);
void uart_boot_cmd_checksum_fw_res(void*);

void sys_irq_shell() {
	uint8_t rx_c = sys_ctrl_shell_get_char();
	uart_boot_rx_frame_parser(rx_c);
}

int boot_main() {
	uint32_t external_fw_index;
	uint32_t flash_status;

	APP_PRINT("[BOOT] version: %s\n", boot_version);

	/**************************************************************************
	* hardware configure
	***************************************************************************/
	SPI.begin();
	flash_io_ctrl_init();
	sys_ctrl_independent_watchdog_init();	/* 32s */

	/**************************************************************************
	* software instansce configure
	***************************************************************************/
	led_init(&led_life, led_life_init, led_life_on, led_life_off);

	sys_boot_init();
	sys_boot_get(&app_sys_boot);

#if 0
	APP_PRINT("\n[b_fwc] psk: 0x%08X checksum: 0x%08X bin_len: %d\n", \
					app_sys_boot.current_fw_boot_header.psk, \
					app_sys_boot.current_fw_boot_header.checksum, \
					app_sys_boot.current_fw_boot_header.bin_len);
	APP_PRINT("[b_fwu] psk: 0x%08X checksum: 0x%08X bin_len: %d\n", \
				app_sys_boot.update_fw_boot_header.psk, \
				app_sys_boot.update_fw_boot_header.checksum, \
				app_sys_boot.update_fw_boot_header.bin_len);
	APP_PRINT("[b_cmd] cmd: %d container:%d io_driver: %d des_addr: 0x%08X src_addr: 0x%08X\n", \
				app_sys_boot.fw_boot_cmd.cmd, \
				app_sys_boot.fw_boot_cmd.container, \
				app_sys_boot.fw_boot_cmd.io_driver, \
				app_sys_boot.fw_boot_cmd.des_addr, \
				app_sys_boot.fw_boot_cmd.src_addr);

	APP_PRINT("\n[a_fwc] psk: 0x%08X checksum: 0x%08X bin_len: %d\n", \
				app_sys_boot.current_fw_app_header.psk, \
				app_sys_boot.current_fw_app_header.checksum, \
				app_sys_boot.current_fw_app_header.bin_len);
	APP_PRINT("[a_fwu] psk: 0x%08X checksum: 0x%08X bin_len: %d\n", \
				app_sys_boot.update_fw_app_header.psk, \
				app_sys_boot.update_fw_app_header.checksum, \
				app_sys_boot.update_fw_app_header.bin_len);
	APP_PRINT("[a_cmd] cmd: %d container:%d io_driver: %d des_addr: 0x%08X src_addr: 0x%08X\n", \
				app_sys_boot.fw_app_cmd.cmd, \
				app_sys_boot.fw_app_cmd.container, \
				app_sys_boot.fw_app_cmd.io_driver, \
				app_sys_boot.fw_app_cmd.des_addr, \
				app_sys_boot.fw_app_cmd.src_addr);
#endif

	EXIT_CRITICAL();

	/**************************************************************************
	* uart boot
	***************************************************************************/
	io_button_mode_init();

	uart_boot_init(uart_boot_cmd_handshake_res);

	if (uart_boot_is_required() || \
			(app_sys_boot.fw_app_cmd.cmd		== SYS_BOOT_CMD_UPDATE_REQ	&& \
			 app_sys_boot.fw_app_cmd.container	== SYS_BOOT_CONTAINER_DIRECTLY	&& \
			 app_sys_boot.fw_app_cmd.io_driver	== SYS_BOOT_IO_DRIVER_UART)) {

		APP_PRINT("[BOOT] uart boot started\n");

		led_blink_set(&led_life, 250, 50);

		while (1) {
		}
	}

	/**************************************************************************
	* boot
	***************************************************************************/
	/**
	 * if it have not request update application and application is ready (app_sys_boot.fw_app_cmd.cmd == SYS_BOOT_CMD_NONE)
	 */
	if (app_sys_boot.fw_app_cmd.cmd == SYS_BOOT_CMD_NONE &&
			app_sys_boot.current_fw_app_header.psk == FIRMWARE_PSK) {
		APP_PRINT("[BOOT] start application\n");
		jump_to_application_before_reset_peripheral();
	}

	/* update firmware */
	else if (app_sys_boot.update_fw_app_header.checksum != 0 && \
			 app_sys_boot.update_fw_app_header.bin_len != 0 && \
			 app_sys_boot.fw_app_cmd.cmd == SYS_BOOT_CMD_UPDATE_REQ && \
			 app_sys_boot.fw_app_cmd.container == SYS_BOOT_CONTAINER_EXTERNAL_FLASH){

		/**
		 * unlock flash and clear all pendings flash's status
		 */
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
						FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_OPTVERRUSR);

		/**
		 * erase application internal flash, prepare for new firmware
		 */
		APP_PRINT("[BOOT] erase internal flash\n");
		internal_flash_erase_pages_cal(app_sys_boot.fw_app_cmd.des_addr, app_sys_boot.update_fw_app_header.bin_len);

		/**
		 * copy firmware data from external flash to internal flash
		 */
		APP_PRINT("[BOOT] copy firmware from external flash\n");

		/* update led status */
		led_blink_set(&led_life, 1000, 50);		/* led flash with duty 1s*/

		external_fw_index = 0;
#if 0
		uint32_t temp;
		while (external_fw_index <  app_sys_boot.update_fw_app_header.bin_len) {
			temp = 0;
			sys_ctrl_independent_watchdog_reset();

			flash_read(app_sys_boot.fw_app_cmd.src_addr + external_fw_index, (uint8_t*)&temp, sizeof(uint32_t));

			flash_status = FLASH_FastProgramWord(app_sys_boot.fw_app_cmd.des_addr + external_fw_index, temp);

			if(flash_status == FLASH_COMPLETE) {
				external_fw_index += sizeof(uint32_t);
			}
			else {
				FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
								FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_OPTVERRUSR);
			}
		}
#else
		uint8_t halfpage_buffer[128]; /* halfpage external flash buffer */
		uint32_t external_fw_reamain; /* remain firmware */
		uint32_t external_fw_read_buf_len; /* actual data is read from external flash */
		while (external_fw_index <  app_sys_boot.update_fw_app_header.bin_len) {
			sys_ctrl_independent_watchdog_reset();

			external_fw_reamain = app_sys_boot.update_fw_app_header.bin_len - external_fw_index;

			if (external_fw_reamain < 128) {
				external_fw_read_buf_len = external_fw_reamain;
			}
			else {
				external_fw_read_buf_len = 128;
			}

			memset(halfpage_buffer, 0 ,128);
			flash_read(app_sys_boot.fw_app_cmd.src_addr + external_fw_index, halfpage_buffer, external_fw_read_buf_len);

			flash_status = FLASH_BUSY;

			ENTRY_CRITICAL();
			flash_status =  FLASH_ProgramHalfPage(app_sys_boot.fw_app_cmd.des_addr + external_fw_index, (uint32_t*)halfpage_buffer);
			EXIT_CRITICAL();

			if (flash_status == FLASH_COMPLETE) {
				external_fw_index += external_fw_read_buf_len;
			}
			else {
				FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
								FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_OPTVERRUSR);
			}
		}
#endif
		/**
		 * calculate checksum, if its incorrectly, restart system and update again
		 */
		uint32_t internal_flash_checksum_cal = 0;
		for (uint32_t index = 0; index < app_sys_boot.update_fw_app_header.bin_len; index += sizeof(uint32_t)) {
			internal_flash_checksum_cal += *((uint32_t*)(app_sys_boot.fw_app_cmd.des_addr + index));
		}

		uint16_t internal_flash_checksum = (uint16_t)(internal_flash_checksum_cal & 0xFFFF);

		if (internal_flash_checksum != app_sys_boot.update_fw_app_header.checksum) {
			APP_PRINT("[BOOT] internal checksum incorrect\n");
			APP_PRINT("[BOOT] system restart\n");
			sys_ctrl_delay_ms(1000);
			sys_ctrl_reset();
		}
		else {
			APP_PRINT("[BOOT] internal checksum correctly\n");

			app_sys_boot.fw_app_cmd.cmd = SYS_BOOT_CMD_UPDATE_RES;
			sys_boot_set(&app_sys_boot);

			led_blink_reset(&led_life);
			led_off(&led_life);

			APP_PRINT("[BOOT] start application\n");
			jump_to_application_before_reset_peripheral();
		}
	}
	else {
		/**
		 * unexpected status
		 * waiting load application
		 */
		APP_PRINT("[BOOT] unexpected status\n");
#if 0
		APP_PRINT("[BOOT] start application\n");
		jump_to_application_before_reset_peripheral();
#else
		APP_PRINT("[BOOT] uart boot started\n");

		led_blink_set(&led_life, 250, 50);

		while (1) {
		}
#endif
	}

	return 0;
}

/**
 * @brief update_boot_fw_info_to_share_boot
 */
void update_boot_fw_info_to_share_boot() {
	firmware_header_t crent_boot_fw_header;
	sys_boot_get(&app_sys_boot);
	sys_ctrl_get_firmware_info(&crent_boot_fw_header);
	if (crent_boot_fw_header.checksum != app_sys_boot.current_fw_boot_header.checksum) {
		crent_boot_fw_header.psk = FIRMWARE_PSK;
		memcpy(&app_sys_boot.current_fw_boot_header, &crent_boot_fw_header, sizeof(firmware_header_t));
		sys_boot_set(&app_sys_boot);
	}
}

/**
 * @brief jump_to_application_before_reset_peripheral
 */
void jump_to_application_before_reset_peripheral() {
	update_boot_fw_info_to_share_boot();

	sys_ctrl_jump_to_app_req = SYS_CTRL_JUMP_TO_APP_REQ;
	sys_ctrl_reset();

	while(1) {
		led_life_on();
		sys_ctrl_delay_ms(200);
		led_life_off();
		sys_ctrl_delay_ms(200);
	}
}

/**
 * @brief sys_irq_timer_10ms
 */
void sys_irq_timer_10ms() {
	led_blink_polling(&led_life);
}

/**************************************************************************
* uart boot handler function define
***************************************************************************/
/**
 * @brief uart_boot_cmd_handshake_res
 * @param boot_obj
 */
void uart_boot_cmd_handshake_res(void* boot_obj) {
	uart_boot_data_cmd_t* uart_boot_object = (uart_boot_data_cmd_t*)(((uart_boot_frame_t*)boot_obj)->data);

	if (uart_boot_object->boot_cmd.cmd == UART_BOOT_CMD_HANDSHAKE_REQ) {
		/* update boot info */
		memcpy(&app_sys_boot.fw_app_cmd, (uart_boot_object->data), sizeof(firmware_boot_cmd_t));

		/* respondse command */
		uart_boot_cmd_t uart_boot_cmd_res;
		uart_boot_cmd_res.cmd		= UART_BOOT_CMD_HANDSHAKE_RES;
		uart_boot_cmd_res.subcmd	= 0;
		uart_boot_write((uint8_t*)&uart_boot_cmd_res, sizeof(uart_boot_cmd_t));

		/* next sequence */
		set_uart_boot_cmd_handler(uart_boot_cmd_update_res);
	}
}

/**
 * @brief uart_boot_cmd_update_res
 * @param boot_obj
 */
void uart_boot_cmd_update_res(void* boot_obj) {
	uart_boot_data_cmd_t* uart_boot_object = (uart_boot_data_cmd_t*)(((uart_boot_frame_t*)boot_obj)->data);

	if (uart_boot_object->boot_cmd.cmd == UART_BOOT_CMD_UPDATE_REQ) {
		/* update host firmware header */
		memcpy(&app_sys_boot.update_fw_app_header, uart_boot_object->data, sizeof(firmware_header_t));

		/* prepare repondse command */
		uart_boot_data_cmd_t uart_boot_data_cmd_res;
		uart_boot_data_cmd_res.boot_cmd.cmd		= UART_BOOT_CMD_UPDATE_RES;
		uart_boot_data_cmd_res.boot_cmd.subcmd	= UART_BOOT_SUB_CMD_1;
		uart_boot_data_cmd_res.len				= sizeof(uint32_t);

		uint32_t flash_erase_addr;
		uint32_t page_number = app_sys_boot.update_fw_app_header.bin_len / 256;

		if ((page_number * 256) < app_sys_boot.update_fw_app_header.bin_len) {
			page_number++;
		}

		/**
		 * unlock flash and clear all pendings flash's status
		 */
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
						FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_OPTVERRUSR);

		/**
		 * erase application internal flash, prepare for new firmware
		 */
		for (uint32_t index = 0; index < page_number; index++) {
			flash_erase_addr = app_sys_boot.fw_app_cmd.des_addr + (index * 256);
			FLASH_ErasePage(flash_erase_addr);
			memcpy(uart_boot_data_cmd_res.data, &flash_erase_addr, sizeof(uint32_t));
			uart_boot_write((uint8_t*)&uart_boot_data_cmd_res, uart_boot_data_cmd_get_len_used(&uart_boot_data_cmd_res));
		}

		uart_boot_data_cmd_res.boot_cmd.subcmd	= UART_BOOT_SUB_CMD_2;
		uart_boot_write((uint8_t*)&uart_boot_data_cmd_res, sizeof(uart_boot_cmd_t));

		/* switch handler next state */
		set_uart_boot_cmd_handler(uart_boot_cmd_transfer_fw_res);
	}
}

/**
 * @brief uart_boot_cmd_transfer_fw_res
 * @param boot_obj
 */
static uint32_t transfer_fw_index = 0;
void uart_boot_cmd_transfer_fw_res(void* boot_obj) {
	uart_boot_data_cmd_t* uart_boot_object = (uart_boot_data_cmd_t*)(((uart_boot_frame_t*)boot_obj)->data);

	if (uart_boot_object->boot_cmd.cmd == UART_BOOT_CMD_TRANSFER_FW_REQ) {
		/* write firmware to internal flash */

		uint32_t flash_status;
#if 0
		uint32_t w_index = 0;

		while(w_index < uart_boot_object->len) {

			flash_status = FLASH_FastProgramWord(NORMAL_START_ADDRESS + transfer_fw_index, *((uint32_t*)(uart_boot_object->data + w_index)));

			if(flash_status == FLASH_COMPLETE) {
				sys_ctrl_independent_watchdog_reset();

				w_index				+= sizeof(uint32_t);
				transfer_fw_index	+= sizeof(uint32_t);
			}
			else {
				FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
								FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_OPTVERRUSR);
			}
		}
#else
		flash_status = FLASH_BUSY;

		while (flash_status != FLASH_COMPLETE) {
			ENTRY_CRITICAL();
			flash_status =  FLASH_ProgramHalfPage(app_sys_boot.fw_app_cmd.des_addr + transfer_fw_index, (uint32_t*)(uart_boot_object->data));
			EXIT_CRITICAL();

			if (memcmp((uint8_t*)(app_sys_boot.fw_app_cmd.des_addr + transfer_fw_index), (uint8_t*)(uart_boot_object->data), uart_boot_object->len) != 0) {
				flash_status = FLASH_BUSY;
			}

			if(flash_status == FLASH_COMPLETE) {
				sys_ctrl_independent_watchdog_reset();
				transfer_fw_index += uart_boot_object->len;
			}
			else {
				FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
								FLASH_FLAG_SIZERR | FLASH_FLAG_OPTVERR | FLASH_FLAG_OPTVERRUSR);
			}
		}
#endif
		if (transfer_fw_index >= app_sys_boot.update_fw_app_header.bin_len) {
			set_uart_boot_cmd_handler(uart_boot_cmd_checksum_fw_res);
		}

		/* response to host */
		uart_boot_cmd_t uart_boot_cmd_res;
		uart_boot_cmd_res.cmd		= UART_BOOT_CMD_TRANSFER_FW_RES;
		uart_boot_cmd_res.subcmd	= 0;
		uart_boot_write((uint8_t*)&uart_boot_cmd_res, sizeof(uart_boot_cmd_t));
	}
}

/**
 * @brief uart_boot_cmd_checksum_fw_res
 */
void uart_boot_cmd_checksum_fw_res(void*) {
	uint32_t check_sum_cal = 0;
	uint32_t end_of_flash =  app_sys_boot.fw_app_cmd.des_addr + app_sys_boot.update_fw_app_header.bin_len;

	for (uint32_t index = app_sys_boot.fw_app_cmd.des_addr; index <= end_of_flash; index += sizeof(uint32_t)) {
		check_sum_cal += *((uint32_t*)index);
	}

	/* response to host */
	uart_boot_cmd_t uart_boot_cmd_res;
	uart_boot_cmd_res.cmd		= UART_BOOT_CMD_CHECKSUM_FW_RES;

	if ((uint16_t)(check_sum_cal & 0xFFFF) == (uint16_t)app_sys_boot.update_fw_app_header.checksum) {
		uart_boot_cmd_res.subcmd	= UART_BOOT_SUB_CMD_1;

		/* Setting boot jump to app */
		app_sys_boot.fw_app_cmd.cmd = SYS_BOOT_CMD_NONE;
		app_sys_boot.current_fw_app_header.psk = FIRMWARE_PSK;
	}
	else {
		uart_boot_cmd_res.subcmd	= UART_BOOT_SUB_CMD_2;
	}

	uart_boot_write((uint8_t*)&uart_boot_cmd_res, sizeof(uart_boot_cmd_t));

	sys_boot_set(&app_sys_boot);

	sys_ctrl_reset();
}
