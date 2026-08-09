/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   05/09/2016
 ******************************************************************************
**/
#include "sys_dbg.h"
#include "sys_ctrl.h"
#include "sys_cfg.h"

#include "app_eeprom.h"
#include "app_flash.h"

#include "xprintf.h"

void sys_dbg_fatal(const int8_t* s, uint8_t c) {
	xprintf("%s\t%x\n", s, c);
	sys_ctrl_reset();
}
