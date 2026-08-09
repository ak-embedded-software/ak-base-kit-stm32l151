#ifndef __SYS_BOOT_H__
#define __SYS_BOOT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "port.h"

#define SYS_BOOT_OK							0x00
#define SYS_BOOT_NG							0x01

#define FIRMWARE_PSK						0x1A2B3C4D /* magic number */

#define SYS_BOOT_CMD_NONE					0x01
#define SYS_BOOT_CMD_UPDATE_REQ				0x02
#define SYS_BOOT_CMD_UPDATE_RES				0x03

#define SYS_BOOT_CONTAINER_DIRECTLY			0x01
#define SYS_BOOT_CONTAINER_EXTERNAL_FLASH	0x02
#define SYS_BOOT_CONTAINER_INTERNAL_FLASH	0x03
#define SYS_BOOT_CONTAINER_EXTERNAL_EPPROM	0x04
#define SYS_BOOT_CONTAINER_INTERNAL_EPPROM	0x05
#define SYS_BOOT_CONTAINER_SDCARD			0x06

#define SYS_BOOT_IO_DRIVER_NONE				0x01
#define SYS_BOOT_IO_DRIVER_UART				0x02
#define SYS_BOOT_IO_DRIVER_SPI				0x03

typedef struct {
	uint32_t psk;
	uint32_t bin_len;
	uint16_t checksum;
} firmware_header_t;

typedef struct {
	uint8_t type;
	uint8_t src_task_id;
	uint8_t des_task_id;
	uint8_t sig;
	uint8_t if_src_type;
	uint8_t if_des_type;
} __AK_PACKETED ak_msg_host_res_t;

typedef struct {
	uint8_t cmd; /* none, update request, verify request ... */
	uint8_t container; /* external FLASH, EPPROM or directly via io driver... */
	uint8_t io_driver; /* SPI, UART, ... */
	uint32_t des_addr; /* start destination address */
	uint32_t src_addr; /* start source address */
	ak_msg_host_res_t ak_msg_res; /* host message response when update completed */
} firmware_boot_cmd_t;

typedef struct {
	/* current firmwre header, that field will be update when system start */
	firmware_header_t current_fw_boot_header;
	firmware_header_t current_fw_app_header;

	/* new firmwre header, that field contain new firmware header */
	firmware_header_t update_fw_boot_header;
	firmware_header_t update_fw_app_header;

	/* firmware's boot command */
	firmware_boot_cmd_t fw_boot_cmd;

	/* firmware's app command */
	firmware_boot_cmd_t fw_app_cmd;
} sys_boot_t;

extern void sys_boot_init();
extern void sys_boot_get(sys_boot_t*);
extern uint8_t sys_boot_set(sys_boot_t*);

#ifdef __cplusplus
}
#endif

#endif //__SYS_BOOT_H__
