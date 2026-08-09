#ifndef __UART_BOOT_H__
#define __UART_BOOT_H__

#include <stdint.h>

/* uart_boot_frame_t */
#define UART_BOOT_FRAME_DATA_SIZE			(254)
#define UART_BOOT_CMD_DATA_SIZE				(128)

/* uart_boot_command_t */
#define UART_BOOT_CMD_HANDSHAKE_REQ			(0x01)
#define UART_BOOT_CMD_HANDSHAKE_RES			(0x02)
#define UART_BOOT_CMD_UPDATE_REQ			(0x03)
#define UART_BOOT_CMD_UPDATE_RES			(0x04)
#define UART_BOOT_CMD_TRANSFER_FW_REQ		(0x05)
#define UART_BOOT_CMD_TRANSFER_FW_RES		(0x06)
#define UART_BOOT_CMD_CHECKSUM_FW_REQ		(0x07)
#define UART_BOOT_CMD_CHECKSUM_FW_RES		(0x08)

/* sub command */
#define UART_BOOT_SUB_CMD_1					(0x01)
#define UART_BOOT_SUB_CMD_2					(0x02)
#define UART_BOOT_SUB_CMD_3					(0x03)

/* boot frame parser */
#define SOP_STATE		0x00
#define LEN_STATE		0x01
#define DATA_STATE		0x02
#define FCS_STATE		0x03

#define UART_BOOT_SOP_CHAR		0xEF

typedef void (*pf_uart_boot_cmd_handler)(void*);

typedef struct {
	uint8_t state;
	uint8_t sop;
	uint8_t len;
	uint8_t index;
	uint8_t data[UART_BOOT_FRAME_DATA_SIZE];
	uint8_t fcs;
	pf_uart_boot_cmd_handler uart_boot_cmd_handler;
} uart_boot_frame_t;

typedef struct {
	uint8_t cmd;
	uint8_t subcmd;
} uart_boot_cmd_t;

typedef struct {
	uart_boot_cmd_t boot_cmd;
	uint8_t len;
	uint8_t data[UART_BOOT_CMD_DATA_SIZE];
} uart_boot_data_cmd_t;

extern void uart_boot_init(pf_uart_boot_cmd_handler);
extern void set_uart_boot_cmd_handler(pf_uart_boot_cmd_handler);
extern uint32_t uart_boot_is_required();
extern void uart_boot_write(uint8_t*, uint8_t);
extern void uart_boot_rx_frame_parser(uint8_t);
extern uint8_t uart_boot_data_cmd_get_len_used(uart_boot_data_cmd_t* cmd);

#endif //__UART_BOOT_H__
