#include <string.h>

#include "sys_boot.h"
#include "sys_io.h"
#include "sys_dbg.h"

static sys_boot_t sys_boot_obj;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow="
void sys_boot_init() {
	extern uint32_t _start_boot_share_data_flash;
	memcpy((uint8_t*)&sys_boot_obj, (uint8_t*)((uint32_t)&_start_boot_share_data_flash), sizeof(sys_boot_t));
}

void sys_boot_get(sys_boot_t* obj) {
	memcpy((uint8_t*)obj, (uint8_t*)&sys_boot_obj, sizeof(sys_boot_t));
}

uint8_t sys_boot_set(sys_boot_t* sys_boot) {
	extern uint32_t _start_boot_share_data_flash;

	/* update RAM object */
	memcpy((uint8_t*)&sys_boot_obj, (uint8_t*)sys_boot, sizeof(sys_boot_t));

	/* update internal flash */
	internal_flash_unlock();
	internal_flash_erase_pages_cal((uint32_t)&_start_boot_share_data_flash, sizeof(sys_boot_t));
	internal_flash_write_cal((uint32_t)&_start_boot_share_data_flash, (uint8_t*)sys_boot, sizeof(sys_boot_t));

	return SYS_BOOT_OK;
}
