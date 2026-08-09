#include <string.h>

#include "ak.h"
#include "port.h"
#include "message.h"
#include "timer.h"

#include "sys_dbg.h"
#include "sys_ctrl.h"
#include "sys_boot.h"
#include "sys_io.h"

#include "flash.h"
#include "led.h"

#include "app.h"
#include "app_if.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_flash.h"

#include "task_if.h"
#include "task_list.h"
#include "task_list_if.h"
#include "task_fw.h"
#include "task_life.h"

/* transfer firmware info */
static firmware_header_t firmware_header_file;

/* position of transfer firmware info */
static uint32_t bin_file_cursor;

static void fw_update_app_req_c_external_flash_io_none(firmware_header_t* firmware_info);
static void fw_update_boot_req_c_external_flash_io_none(firmware_header_t* firmware_info);

static uint8_t host_firmware_task_id;
static uint8_t host_firmware_if_type;

static uint8_t flash_read_buffer[FLASH_PAGE_SIZE];

void task_fw(ak_msg_t* msg) {
	switch (msg->sig) {
	case FW_CHECKING_REQ: {
		APP_DBG_SIG("FW_CHECKING_REQ\n");
		firmware_header_t cr_fw_header;

		sys_boot_t sb;
		sys_boot_get(&sb);

		sys_ctrl_get_firmware_info(&cr_fw_header);

		/* check and update current app header field of boot share data */
		if (memcmp(&cr_fw_header, &(sb.current_fw_app_header), sizeof(firmware_header_t)) != 0) {
			memcpy(&(sb.current_fw_app_header), &cr_fw_header, sizeof(firmware_header_t));
		}

		/* notify update app firmware completed */
		if (sb.fw_app_cmd.cmd == SYS_BOOT_CMD_UPDATE_RES) {
			/* TODO: update firmware completed */
			ak_msg_t* s_msg = get_pure_msg();
			set_if_src_task_id(s_msg,	sb.fw_app_cmd.ak_msg_res.src_task_id);
			set_if_des_task_id(s_msg,	sb.fw_app_cmd.ak_msg_res.des_task_id);
			set_if_src_type(s_msg,		sb.fw_app_cmd.ak_msg_res.if_src_type);
			set_if_des_type(s_msg,		sb.fw_app_cmd.ak_msg_res.if_des_type);
			set_if_sig(s_msg,			sb.fw_app_cmd.ak_msg_res.sig);
			set_msg_sig(s_msg, AC_IF_PURE_MSG_OUT);
			task_post(AC_TASK_IF_ID, s_msg);
		}

		/* reset update command */
		sb.fw_app_cmd.cmd = SYS_BOOT_CMD_NONE;

		/* notify update boot firmware completed */
		if (sb.fw_boot_cmd.cmd == SYS_BOOT_CMD_UPDATE_RES) {
			/* TODO: update firmware completed */
			ak_msg_t* s_msg = get_pure_msg();
			set_if_src_task_id(s_msg,	sb.fw_boot_cmd.ak_msg_res.src_task_id);
			set_if_des_task_id(s_msg,	sb.fw_boot_cmd.ak_msg_res.des_task_id);
			set_if_src_type(s_msg,		sb.fw_boot_cmd.ak_msg_res.if_src_type);
			set_if_des_type(s_msg,		sb.fw_boot_cmd.ak_msg_res.if_des_type);
			set_if_sig(s_msg,			sb.fw_boot_cmd.ak_msg_res.sig);
			set_msg_sig(s_msg, AC_IF_PURE_MSG_OUT);
			task_post(AC_TASK_IF_ID, s_msg);
		}

		/* reset update command */
		sb.fw_boot_cmd.cmd = SYS_BOOT_CMD_NONE;

		/* clear update app && boot firmware header */
		memset(&(sb.update_fw_app_header), 0, sizeof(firmware_header_t));
		memset(&(sb.update_fw_boot_header), 0, sizeof(firmware_header_t));

		sys_boot_t cr_sb;
		sys_boot_get(&cr_sb);

		if (memcmp(&cr_sb, &sb, sizeof(sys_boot_t)) != 0) {
			sys_boot_set(&sb);
		}
	}
		break;

	case FW_CRENT_APP_FW_INFO_REQ: {
		APP_DBG_SIG("FW_CRENT_APP_FW_INFO_REQ\n");
		sys_boot_t sb;
		sys_boot_get(&sb);

		/* update respondse message */
		sb.fw_app_cmd.ak_msg_res.des_task_id = msg->if_src_task_id;
		sb.fw_app_cmd.ak_msg_res.src_task_id = msg->if_des_task_id;
		sb.fw_app_cmd.ak_msg_res.if_des_type = msg->if_src_type;
		sb.fw_app_cmd.ak_msg_res.if_src_type = msg->if_des_type;
		sb.fw_app_cmd.ak_msg_res.sig = GW_FW_UPDATE_COMPLETED;
		sys_boot_set(&sb);

		firmware_header_t firmware_header_req;
		sys_ctrl_get_firmware_info(&firmware_header_req);

		ak_msg_t* s_msg = get_common_msg();

		set_if_src_task_id(s_msg, AC_TASK_FW_ID);
		set_if_des_task_id(s_msg, msg->if_src_task_id);
		set_if_des_type(s_msg, msg->if_src_type);
		set_if_sig(s_msg, GW_FW_CURRENT_INFO_RES);
		set_if_data_common_msg(s_msg, (uint8_t*)&firmware_header_req, sizeof(firmware_header_t));

		set_msg_sig(s_msg, AC_IF_COMMON_MSG_OUT);
		task_post(AC_TASK_IF_ID, s_msg);
	}
		break;

	case FW_CRENT_BOOT_FW_INFO_REQ: {
		APP_DBG_SIG("FW_CRENT_BOOT_FW_INFO_REQ\n");
		sys_boot_t sb;
		sys_boot_get(&sb);

		/* update respondse message */
		sb.fw_boot_cmd.ak_msg_res.des_task_id = msg->if_src_task_id;
		sb.fw_boot_cmd.ak_msg_res.src_task_id = msg->if_des_task_id;
		sb.fw_boot_cmd.ak_msg_res.if_des_type = msg->if_src_type;
		sb.fw_boot_cmd.ak_msg_res.if_src_type = msg->if_des_type;
		sb.fw_boot_cmd.ak_msg_res.sig = GW_FW_UPDATE_COMPLETED;
		sys_boot_set(&sb);

		ak_msg_t* s_msg = get_common_msg();

		set_if_src_task_id(s_msg, AC_TASK_FW_ID);
		set_if_des_task_id(s_msg, msg->if_src_task_id);
		set_if_des_type(s_msg, msg->if_src_type);
		set_if_sig(s_msg, GW_FW_CURRENT_INFO_RES);
		set_if_data_common_msg(s_msg, (uint8_t*)&sb.current_fw_boot_header, sizeof(firmware_header_t));

		set_msg_sig(s_msg, AC_IF_COMMON_MSG_OUT);
		task_post(AC_TASK_IF_ID, s_msg);
	}
		break;

	case FW_UPDATE_REQ: {
		APP_DBG_SIG("FW_UPDATE_REQ\n");

		memcpy(&firmware_header_file, get_data_common_msg(msg), sizeof(firmware_header_t));
		APP_DBG("firmware_header_file.checksum:%04X\n", firmware_header_file.checksum);
		APP_DBG("firmware_header_file.bin_len:%d\n", firmware_header_file.bin_len);

		/* reset bin_file_cursor */
		bin_file_cursor = 0;

		host_firmware_task_id = msg->if_src_task_id;
		host_firmware_if_type = msg->if_src_type;

		/* TODO: when recieve firmware update request.
		 * depend on each system, we need to check system state then decide update or respondse busy signal */
		task_post_pure_msg(AC_TASK_FW_ID, FW_UPDATE_SM_OK);
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_FW_UPDATE);
	}
		break;

	case FW_UPDATE_SM_OK: {
		/* clear flash loader */
		for (int i = 0; i < APP_FLASH_FIRMWARE_BLOCK_64K_SIZE; i++) {
			flash_erase_block_64k(APP_FLASH_FIRMWARE_START_ADDR + (FLASH_BLOCK_64K_SIZE * i));
		}
		APP_DBG("erase temp OK\n");

		ak_msg_t* s_msg = get_pure_msg();

		set_if_src_task_id(s_msg, AC_TASK_FW_ID);
		set_if_des_task_id(s_msg, host_firmware_task_id);
		set_if_des_type(s_msg, host_firmware_if_type);
		set_if_sig(s_msg, GW_FW_UPDATE_RES_OK);

		set_msg_sig(s_msg, AC_IF_PURE_MSG_OUT);
		task_post(AC_TASK_IF_ID, s_msg);

		timer_set(AC_TASK_FW_ID, FW_PACKED_TIMEOUT, FW_PACKED_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
	}
		break;

	case FW_UPDATE_SM_BUSY: {
		APP_DBG_SIG("FW_UPDATE_SM_BUSY\n");
		ak_msg_t* s_msg = get_pure_msg();

		set_if_src_task_id(s_msg, AC_TASK_FW_ID);
		set_if_des_task_id(s_msg, host_firmware_task_id);
		set_if_des_type(s_msg, host_firmware_if_type);
		set_if_sig(s_msg, GW_FW_UPDATE_BUSY);

		set_msg_sig(s_msg, AC_IF_PURE_MSG_OUT);
		task_post(AC_TASK_IF_ID, s_msg);
	}
		break;

	case FW_TRANSFER_REQ: {
		sys_ctrl_independent_watchdog_reset();
		sys_ctrl_soft_watchdog_reset();

		timer_set(AC_TASK_FW_ID, FW_PACKED_TIMEOUT, FW_PACKED_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);

		/* write firmware packet to external flash */
		uint8_t* firmware_packet = get_data_common_msg(msg);
		uint8_t firmware_packet_len = get_data_len_common_msg(msg);
		flash_write(APP_FLASH_FIRMWARE_START_ADDR + bin_file_cursor, firmware_packet, firmware_packet_len);

		/* increase transfer binary file cursor */
		bin_file_cursor += firmware_packet_len;

		/* send response message to getway */
		ak_msg_t* s_msg = get_pure_msg();

		/* transfer completed */
		if (bin_file_cursor >= firmware_header_file.bin_len) {
			timer_remove_attr(AC_TASK_FW_ID, FW_PACKED_TIMEOUT);

			/* start calculate chechsum */
			uint32_t checksum_buffer = 0;
			uint32_t word = 0;

			APP_DBG("start calculate checksum\n");
			for (uint32_t index = 0; index < firmware_header_file.bin_len; index += sizeof(uint32_t)) {
				sys_ctrl_independent_watchdog_reset();
				sys_ctrl_soft_watchdog_reset();

				word = 0;
				flash_read(APP_FLASH_FIRMWARE_START_ADDR + index, (uint8_t*)&word, sizeof(uint32_t));
				checksum_buffer += word;
			}

			uint16_t checksum_calculated = (uint16_t)(checksum_buffer & 0xFFFF);
			APP_DBG("checksum_calculated:%04X\n", checksum_calculated);
			APP_DBG("checksum_transfer:%04X\n", firmware_header_file.checksum);

			/* checksum correctly */
			if (checksum_calculated == firmware_header_file.checksum) {
				APP_DBG("checksum correctly\n");
				set_if_sig(s_msg, GW_FW_INTERNAL_UPDATE_REQ);
			}

			/* checksum incorrect*/
			else {
				APP_DBG("checksum incorrect\n");
				set_if_sig(s_msg, GW_FW_TRANSFER_CHECKSUM_ERR);
			}
		}

		/* transferring countimue */
		else {
			set_if_sig(s_msg, GW_FW_TRANSFER_RES_OK);
		}

		set_if_src_task_id(s_msg, AC_TASK_FW_ID);
		set_if_des_task_id(s_msg, host_firmware_task_id);
		set_if_des_type(s_msg, host_firmware_if_type);

		set_msg_sig(s_msg, AC_IF_PURE_MSG_OUT);
		task_post(AC_TASK_IF_ID, s_msg);
	}
		break;

	case FW_PACKED_TIMEOUT: {
		APP_DBG_SIG("FW_PACKED_TIMEOUT\n");
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_FW_UPDATE_ERR);
	}
		break;

	case FW_INTERNAL_UPDATE_APP_RES_OK: {
		APP_DBG_SIG("FW_INTERNAL_UPDATE_APP_RES_OK\n");
		/* update share flash info */
		fw_update_app_req_c_external_flash_io_none(&firmware_header_file);

		/* system reset */
		sys_ctrl_delay_ms(100);
		sys_ctrl_reset();
	}
		break;

	case FW_INTERNAL_UPDATE_BOOT_RES_OK: {
		APP_DBG_SIG("FW_INTERNAL_UPDATE_BOOT_RES_OK\n");

		fw_update_boot_req_c_external_flash_io_none(&firmware_header_file);

		/* system reset */
		sys_ctrl_delay_ms(100);
		sys_ctrl_reset();
	}
		break;

	case FW_SAFE_MODE_RES_OK: {
		APP_DBG_SIG("FW_SAFE_MODE_RES_OK\n");
	}
		break;

	default:
		break;
	}
}

void fw_update_app_req_c_external_flash_io_none(firmware_header_t* firmware_info) {
	firmware_info->psk = FIRMWARE_PSK;
	sys_boot_t sb;
	sys_boot_get(&sb);

	/* update new app header */
	memcpy(&(sb.update_fw_app_header), firmware_info, sizeof(firmware_header_t));

	/* cmd update request */
	sb.fw_app_cmd.cmd			= SYS_BOOT_CMD_UPDATE_REQ;
	sb.fw_app_cmd.container		= SYS_BOOT_CONTAINER_EXTERNAL_FLASH;
	sb.fw_app_cmd.io_driver		= SYS_BOOT_IO_DRIVER_NONE;
	sb.fw_app_cmd.des_addr		= APP_START_ADDR;
	sb.fw_app_cmd.src_addr		= APP_FLASH_FIRMWARE_START_ADDR;

	sys_boot_set(&sb);
}

void fw_update_boot_req_c_external_flash_io_none(firmware_header_t* firmware_info) {
	firmware_info->psk = FIRMWARE_PSK;
	sys_boot_t sb;
	sys_boot_get(&sb);

	/* update new app header */
	memcpy(&(sb.update_fw_boot_header), firmware_info, sizeof(firmware_header_t));

	/* cmd update request */
	sb.fw_boot_cmd.cmd			= SYS_BOOT_CMD_UPDATE_RES; /* using notify internal boot firmware is update success */
	sb.fw_boot_cmd.container	= SYS_BOOT_CONTAINER_EXTERNAL_FLASH;
	sb.fw_boot_cmd.io_driver	= SYS_BOOT_IO_DRIVER_NONE;
	sb.fw_boot_cmd.des_addr		= 0x08000000;
	sb.fw_boot_cmd.src_addr		= APP_FLASH_FIRMWARE_START_ADDR;

	sys_boot_set(&sb);

	internal_flash_unlock();
	internal_flash_erase_pages_cal(sb.fw_boot_cmd.des_addr, sb.update_fw_boot_header.bin_len);

	uint32_t external_fw_index = 0;
	uint32_t fw_packet_write_len = 0;
	uint32_t remain_fw_len = sb.update_fw_boot_header.bin_len;

	while (external_fw_index < sb.update_fw_boot_header.bin_len) {
		sys_ctrl_independent_watchdog_reset();
		sys_ctrl_soft_watchdog_reset();

		remain_fw_len = sb.update_fw_boot_header.bin_len - external_fw_index;
		if (remain_fw_len < FLASH_PAGE_SIZE) {
			fw_packet_write_len = remain_fw_len;
		}
		else {
			fw_packet_write_len = FLASH_PAGE_SIZE;
		}

		flash_read(sb.fw_boot_cmd.src_addr + external_fw_index, (uint8_t*)&flash_read_buffer, fw_packet_write_len);
		internal_flash_write_cal(sb.fw_boot_cmd.des_addr + external_fw_index, flash_read_buffer, fw_packet_write_len);

		external_fw_index += fw_packet_write_len;
	}

	internal_flash_lock();
}
