#include "uart_boot.h"
#include "sys_ctrl.h"
#include "sys_io.h"

static uart_boot_frame_t uart_boot_frame;

static uint8_t uart_boot_calcfcs(uint8_t len, uint8_t *data_ptr);

uint8_t uart_boot_calcfcs(uint8_t len, uint8_t *data_ptr) {
	uint8_t xor_result;
	xor_result = len;
	for (int i = 0; i < len; i++, data_ptr++) {
		xor_result = xor_result ^ *data_ptr;
	}
	return xor_result;
}

void uart_boot_init(pf_uart_boot_cmd_handler boot_entry_handler) {
	uart_boot_frame.index	= 0;
	uart_boot_frame.len		= 0;
	uart_boot_frame.state	= SOP_STATE;
	uart_boot_frame.uart_boot_cmd_handler = boot_entry_handler;
}

void set_uart_boot_cmd_handler(pf_uart_boot_cmd_handler handler) {
	if (handler) {
		uart_boot_frame.uart_boot_cmd_handler = handler;
	}
}

uint32_t uart_boot_is_required() {
	for (int i = 0; i < 300; i++) {
		if (io_button_mode_read()) {
			return 0;
		}
	}
	return 1;
}

void uart_boot_write(uint8_t* data, uint8_t len) {
	sys_ctrl_shell_put_char(UART_BOOT_SOP_CHAR);
	sys_ctrl_shell_put_char(len);
	for (uint32_t i = 0; i < len; i++) {
		sys_ctrl_shell_put_char((uint8_t)data[i]);
	}
	sys_ctrl_shell_put_char(uart_boot_calcfcs(len, data));
}

void uart_boot_rx_frame_parser(uint8_t ch) {
	switch (uart_boot_frame.state) {
	case SOP_STATE: {
		if (UART_BOOT_SOP_CHAR == ch) {
			uart_boot_frame.state = LEN_STATE;
		}
	}
		break;

	case LEN_STATE: {
		if (ch > UART_BOOT_FRAME_DATA_SIZE) {
			uart_boot_frame.state = SOP_STATE;
			return;
		}
		else {
			uart_boot_frame.len = ch;
			uart_boot_frame.index = 0;
			uart_boot_frame.state = DATA_STATE;
		}
	}
		break;

	case DATA_STATE: {
		uart_boot_frame.data[uart_boot_frame.index++] = ch;

		if (uart_boot_frame.index == uart_boot_frame.len) {
			uart_boot_frame.state = FCS_STATE;
		}
	}
		break;

	case FCS_STATE: {
		uart_boot_frame.state = SOP_STATE;

		uart_boot_frame.fcs = ch;

		if (uart_boot_frame.fcs \
				== uart_boot_calcfcs(uart_boot_frame.len, uart_boot_frame.data)) {
			if (uart_boot_frame.uart_boot_cmd_handler) {
				uart_boot_frame.uart_boot_cmd_handler((uart_boot_frame_t*)&uart_boot_frame);
			}
		}
		else {
			/* TODO: handle checksum incorrect */
		}
	}
		break;

	default:
		break;
	}
}

uint8_t uart_boot_data_cmd_get_len_used(uart_boot_data_cmd_t* cmd) {
	uint8_t len = sizeof(uart_boot_cmd_t) + 1 + cmd->len;
	return len;
}
