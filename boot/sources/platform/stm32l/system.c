/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   05/09/2016
 ******************************************************************************
**/
#include <stdint.h>
#include <stdbool.h>

#include "sys_cfg.h"
#include "system.h"
#include "stm32l.h"

#include "system_stm32l1xx.h"
#include "stm32l1xx.h"
#include "stm32l1xx_conf.h"
#include "core_cm3.h"

#include "sys_dbg.h"
#include "sys_ctrl.h"
#include "sys_irq.h"
#include "sys_boot.h"

#include "app.h"

/*****************************************************************************/
/* linker variable                                                           */
/*****************************************************************************/
extern uint32_t _ldata;
extern uint32_t _data;
extern uint32_t _edata;
extern uint32_t _bss;
extern uint32_t _ebss;
extern uint32_t _estack;

/*****************************************************************************/
/* static function prototype                                                 */
/*****************************************************************************/
/*****************************/
/* system interrupt function */
/*****************************/
void default_handler();
void reset_handler();

/* cortex-M processor fault exceptions */
void nmi_handler()          __attribute__ ((weak));
void hard_fault_handler()   __attribute__ ((weak));
void mem_manage_handler()   __attribute__ ((weak));
void bus_fault_handler()    __attribute__ ((weak));
void usage_fault_handler()  __attribute__ ((weak));

/* cortex-M processor non-fault exceptions */
void svc_handler()          __attribute__ ((weak, alias("default_handler")));
void dg_monitor_handler()   __attribute__ ((weak, alias("default_handler")));
void pendsv_handler()       __attribute__ ((weak, alias("default_handler")));
void systick_handler();

/* external interrupts */
static void shell_handler();

/*****************************************************************************/
/* interrupt vector table                                                    */
/*****************************************************************************/
__attribute__((section(".isr_vector")))
void (* const isr_vector[])() = {
		((void (*)())(uint32_t)&_estack),		//	The initial stack pointer
		reset_handler,							//	The reset handler
		nmi_handler,							//	The NMI handler
		hard_fault_handler,						//	The hard fault handler
		mem_manage_handler,						//	The MPU fault handler
		bus_fault_handler,						//	The bus fault handler
		usage_fault_handler,					//	The usage fault handler
		0,										//	Reserved
		0,										//	Reserved
		0,										//	Reserved
		0,										//	Reserved
		svc_handler,							//	SVCall handler
		dg_monitor_handler,						//	Debug monitor handler
		0,										//	Reserved
		pendsv_handler,							//	The PendSV handler
		systick_handler,						//	The SysTick handler

		//External Interrupts
		default_handler,						//	Window Watchdog
		default_handler,						//	PVD through EXTI Line detect
		default_handler,						//	Tamper and Time Stamp
		default_handler,						//	RTC Wakeup
		default_handler,						//	FLASH
		default_handler,						//	RCC
		default_handler,						//	EXTI Line 0
		default_handler,						//	EXTI Line 1
		default_handler,						//	EXTI Line 2
		default_handler,						//	EXTI Line 3
		default_handler,						//	EXTI Line 4
		default_handler,						//	DMA1 Channel 1
		default_handler,						//	DMA1 Channel 2
		default_handler,						//	DMA1 Channel 3
		default_handler,						//	DMA1 Channel 4
		default_handler,						//	DMA1 Channel 5
		default_handler,						//	DMA1 Channel 6
		default_handler,						//	DMA1 Channel 7
		default_handler,						//	ADC1
		default_handler,						//	USB High Priority
		default_handler,						//	USB Low  Priority
		default_handler,						//	DAC
		default_handler,						//	COMP through EXTI Line
		default_handler,						//	EXTI Line 9..5
		default_handler,						//	LCD
		default_handler,						//	TIM9
		default_handler,						//	TIM10
		default_handler,						//	TIM11
		default_handler,						//	TIM2
		default_handler,						//	TIM3
		default_handler,						//	TIM4
		default_handler,						//	I2C1 Event
		default_handler,						//	I2C1 Error
		default_handler,						//	I2C2 Event
		default_handler,						//	I2C2 Error
		default_handler,						//	SPI1
		default_handler,						//	SPI2
		shell_handler,							//	USART1
		default_handler,						//	USART2
		default_handler,						//	USART3
		default_handler,						//	EXTI Line 15..10
		default_handler,						//	RTC Alarm through EXTI Line
		default_handler,						//	USB FS Wakeup from suspend
		default_handler,						//	TIM6
		default_handler,						//	TIM7
		};

void sys_delay_us(volatile uint32_t count) {
	volatile uint32_t delay_value = 0;
	delay_value = count * 4;
	while(delay_value--);
}

void sys_delay_ms(volatile uint32_t count) {
	volatile uint32_t delay = 1000 * count;
	sys_delay_us(delay);
}

/*****************************************************************************/
/* static function defination                                                */
/*****************************************************************************/
void default_handler() {
}

uint8_t power_on_reset __attribute__ ((section ("non_clear_ram")));

uint8_t sys_is_power_on_reset() {
	return (uint8_t)power_on_reset;
}

void reset_handler() {
	power_on_reset = SYS_NON_POWER_ON_RESET;
	uint32_t *pSrc	= &_ldata;
	uint32_t *pDest	= &_data;

	if  (RCC_GetFlagStatus(RCC_FLAG_PORRST)){
		power_on_reset = SYS_POWER_ON_RESET;
	}
	RCC_ClearFlag();

	if (sys_ctrl_jump_to_app_req == SYS_CTRL_JUMP_TO_APP_REQ) {
		sys_ctrl_jump_to_app();
	}

	/* init system */
	SystemInit();

	/* copy init data from FLASH to SRAM */
	while(pDest < &_edata) {
		*pDest++ = *pSrc++;
	}

	/* zero bss */
	for (pDest = &_bss; pDest < &_ebss; pDest++) {
		*pDest = 0UL;
	}

	ENTRY_CRITICAL();

	sys_cfg_tick();
	sys_cfg_console();

	/* entry app function */
	boot_main();
}

/***************************************/
/* cortex-M processor fault exceptions */
/***************************************/
void nmi_handler() {
}

void hard_fault_handler() {
}

void mem_manage_handler() {
}

void bus_fault_handler() {
}

void usage_fault_handler() {
}

/*******************************************/
/* cortex-M processor non-fault exceptions */
/*******************************************/
void systick_handler() {
	sys_irq_timer_10ms();
}

/************************/
/* external interrupts  */
/************************/
void shell_handler() {
	if (USART_GetITStatus(USARTx, USART_IT_RXNE) == SET) {
		sys_irq_shell();
	}
}
