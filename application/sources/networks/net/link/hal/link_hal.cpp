#include "link_hal.h"
#include "sys_dbg.h"
#include "sys_ctrl.h"
#include "xprintf.h"

/* default base method */
static uint8_t link_hal_write_byte(uint8_t);
static uint8_t link_hal_rev_byte(uint8_t);

/* initial link hal method */
pf_link_hal_write_byte plink_hal_write_byte = link_hal_write_byte;
pf_link_hal_rev_byte plink_hal_rev_byte = link_hal_rev_byte;

uint8_t link_hal_write_byte(uint8_t byte) {
	(void)byte;
	return byte;
}

uint8_t link_hal_rev_byte(uint8_t byte) {
	(void)byte;
	return LINK_HAL_IGNORED;
}

void link_hal_reg_write_byte(pf_link_hal_write_byte f_write_byte) {
	if (f_write_byte != ((pf_link_hal_write_byte)0)) {
		plink_hal_write_byte = f_write_byte;
	}
	else {
		FATAL("link_hal", 0x01);
	}
}

void link_hal_reg_rev_byte(pf_link_hal_rev_byte f_rev_byte) {
	if (f_rev_byte != ((pf_link_hal_rev_byte)0)) {
		plink_hal_rev_byte = f_rev_byte;
	}
	else {
		FATAL("link_hal", 0x02);
	}
}
