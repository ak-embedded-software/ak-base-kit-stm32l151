/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   12/09/2016
 ******************************************************************************
**/
#ifndef __SYS_CTRL_H__
#define __SYS_CTRL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "app_data.h"
#include "system.h"

/*****************************************************************************
 * system utilities function.
 *****************************************************************************/
#define SYS_POWER_ON_RESET			0x00
#define SYS_NON_POWER_ON_RESET		0x01

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

/* get current 1ns system timer variable */
extern uint32_t sys_ctrl_micros();

/* get current 1ms system timer variable */
extern uint32_t sys_ctrl_millis();

/* get character of system console */
extern uint8_t sys_ctrl_shell_get_char();

/* put character of system console (NON-BLOCKING using interrupt) */
extern void sys_ctrl_shell_put_char(uint8_t);

/* put character of system console (BLOCKING)*/
extern void sys_ctrl_shell_put_char_block(uint8_t);

/* switch shell put char with blocking */
extern void sys_ctrl_shell_sw_to_block();

/* switch shell put char with non-blocking */
extern void sys_ctrl_shell_sw_to_nonblock();

/* get firmware info */
extern void sys_ctrl_get_firmware_info(firmware_header_t*);

/* goto sleep mode and wait for interrupt */
extern void sys_ctr_sleep_wait_for_irq();

/* get exception number */
extern uint32_t sys_ctr_get_exception_number();

/* system restart application */
extern void sys_ctr_restart_app();

/* get vbat voltage */
extern uint32_t sys_ctr_get_vbat_voltage();

/* get MCU temperature */
extern uint32_t sys_ctr_get_mcu_temperature();

extern void sys_ctr_stop_mcu();

extern int asm_test_add(int, int);

/*****************************************************************************
 * system memory function.
 *****************************************************************************/
extern uint32_t sys_stack_count_init(); //clean stack, return stack size
extern uint32_t sys_stack_usage();
extern uint32_t sys_stack_get_size();

#ifdef __cplusplus
}
#endif

#endif // __SYS_CTRL_H__
