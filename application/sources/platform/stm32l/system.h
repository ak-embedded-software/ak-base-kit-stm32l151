/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   05/09/2016
 ******************************************************************************
**/
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef enum {
	IRQ_PRIO_TIMER7_SOFT_WATCHDOG	= 0,
	IRQ_PRIO_SYS_SYSTEMSTICK		= 1,
	IRQ_PRIO_SYS_TIMER_1US			= 1,
	IRQ_PRIO_UART0_CONSOLE			= 2,
	IRQ_PRIO_UART2_IO				= 3,
	IRQ_PRIO_SYS_SVC				= 14,
	IRQ_PRIO_SYS_PENDSV				= 15,
	IRQ_PRIO_TIMER3_MODBUS			= 2,
} system_irq_prio_e;

typedef struct {
	uint32_t cpu_clock;
	uint32_t tick;
	uint32_t console_baudrate;
	uint32_t flash_used;
	uint32_t ram_used;
	uint32_t data_init_size;
	uint32_t data_non_init_size;
	uint32_t stack_avail;
	uint32_t heap_avail;
	uint32_t ram_other;
} system_info_t;

extern system_info_t system_info;

typedef void (*p_jump_func)(void);

#ifdef __cplusplus
}
#endif

#endif //__SYSTEM_H__
