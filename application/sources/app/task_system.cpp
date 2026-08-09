#include "fsm.h"
#include "port.h"
#include "message.h"

#include "sys_ctrl.h"
#include "sys_dbg.h"

#include "app.h"
#include "app_dbg.h"

#include "task_list.h"
#include "task_system.h"

void task_system(ak_msg_t* msg) {
	switch (msg->sig) {
	case SYSTEM_AK_FLASH_UPDATE_REQ: {
		APP_DBG_SIG("SYSTEM_AK_FLASH_UPDATE_REQ\n");

		sys_boot_t sb;
		sys_boot_get(&sb);

		/* cmd update request */
		sb.fw_app_cmd.cmd			= SYS_BOOT_CMD_UPDATE_REQ;
		sb.fw_app_cmd.container		= SYS_BOOT_CONTAINER_DIRECTLY;
		sb.fw_app_cmd.io_driver		= SYS_BOOT_IO_DRIVER_UART;
		sb.fw_app_cmd.des_addr		= APP_START_ADDR;
		sb.fw_app_cmd.src_addr		= 0;
		sys_boot_set(&sb);

		sys_ctrl_reset();
	}
		break;

	default:
		break;
	}
}
