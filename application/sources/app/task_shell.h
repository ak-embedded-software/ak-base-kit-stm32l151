/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   13/08/2016
 ******************************************************************************
**/

#ifndef __TASK_SHELL_H__
#define __TASK_SHELL_H__

#include "cmd_line.h"
#include "ring_buffer.h"

#if defined (IF_LINK_UART_EN)
#include "link_phy.h"
#include "link_hal.h"
#endif

#define SHELL_BUFFER_LENGHT				(32)
#define BUFFER_CONSOLE_REV_SIZE			(256)

struct shell_t {
	uint8_t index;
	uint8_t data[SHELL_BUFFER_LENGHT];
};

extern volatile struct shell_t shell;

extern const cmd_line_t lgn_cmd_table[];

extern uint8_t buffer_console_rev[];
extern ring_buffer_char_t ring_buffer_console_rev;

#endif //__TASK_SHELL_H__
