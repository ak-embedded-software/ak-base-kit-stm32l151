/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   13/08/2016
 ******************************************************************************
**/

#include <stdbool.h>

#include "fsm.h"
#include "port.h"
#include "message.h"

#include "cmd_line.h"
#include "xprintf.h"

#include "sys_dbg.h"
#include "sys_ctrl.h"
#include "sys_irq.h"
#include "sys_io.h"

#include "app.h"
#include "app_dbg.h"
#include "task_shell.h"
#include "task_uart_if.h"
#include "task_list.h"

//#pragma GCC optimize ("O3")

volatile struct shell_t shell;

uint8_t buffer_console_rev[BUFFER_CONSOLE_REV_SIZE];
ring_buffer_char_t ring_buffer_console_rev;

void sys_irq_shell() {
	volatile uint8_t c = 0;

	c = sys_ctrl_shell_get_char();

	ENTRY_CRITICAL();
	ring_buffer_char_put(&ring_buffer_console_rev, c);
	EXIT_CRITICAL();
}

void task_shell(ak_msg_t* msg) {
	uint8_t fist_char = *(get_data_common_msg(msg));

	switch (msg->sig) {
	case AC_SHELL_LOGIN_CMD:
		break;

	case AC_SHELL_REMOTE_CMD:
		break;

	default:
		FATAL("SHELL", 0x01);
		break;
	}

	switch (cmd_line_parser((cmd_line_t*)lgn_cmd_table, get_data_common_msg(msg))) {
	case CMD_SUCCESS:
		break;

	case CMD_NOT_FOUND:
		if (fist_char != '\r' &&
				fist_char != '\n') {
			LOGIN_PRINT("cmd unknown\n");
		}
		break;

	case CMD_TOO_LONG:
		LOGIN_PRINT("cmd too long\n");
		break;

	case CMD_TBL_NOT_FOUND:
		LOGIN_PRINT("cmd table not found\n");
		break;

	default:
		LOGIN_PRINT("cmd error\n");
		break;
	}

	LOGIN_PRINT("#");
}
