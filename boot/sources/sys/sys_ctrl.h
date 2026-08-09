/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   12/09/2016
 ******************************************************************************
**/
#ifndef __SYS_CTRL_H__
#define __SYS_CTRL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "sys_boot.h"

#define SYS_POWER_ON_RESET			0x00
#define SYS_NON_POWER_ON_RESET		0x01
#define SYS_CTRL_JUMP_TO_APP_REQ	((uint32_t)0xEFEFEFEF)

extern uint32_t sys_ctrl_jump_to_app_req;

/* reset system (soft reset) */
extern void sys_ctrl_reset();

/* hardware watchdog interface */
extern void sys_ctrl_independent_watchdog_init();
extern void sys_ctrl_independent_watchdog_reset();

/* software watchdog interface */
extern void sys_ctrl_soft_watchdog_init(uint32_t);
extern void sys_ctrl_soft_watchdog_reset();
extern void sys_ctrl_soft_watchdog_enable();
extern void sys_ctrl_soft_watchdog_disable();
extern void sys_ctrl_soft_watchdog_increase_counter();

/* delay 3 cycles clock of system */
extern void sys_ctrl_delay(volatile uint32_t count);

/* system delay ms unit, this function using timer delay */
extern void sys_ctrl_delay_ms(volatile uint32_t count);

/* system delay us, uint this function using CPU delay*/
extern void sys_ctrl_delay_us(volatile uint32_t count);

/* get current system timer variable */
extern uint32_t sys_ctrl_millis();

/* get character of system console */
extern uint8_t sys_ctrl_shell_get_char();

/* console put charactor */
extern void sys_ctrl_shell_put_char(uint8_t);

/* get firmware info */
extern void sys_ctrl_get_firmware_info(firmware_header_t*);

extern uint8_t sys_is_power_on_reset();

extern void sys_ctrl_jump_to_app();

#ifdef __cplusplus
}
#endif

#endif // __SYS_CTRL_H__
