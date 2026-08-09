/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   05/09/2016
 ******************************************************************************
**/
#include "sys_dbg.h"
#include "sys_ctrl.h"
#include "sys_irq.h"
#include "sys_cfg.h"
#include "io_cfg.h"

#include "ak.h"

#include "flash.h"

#include "app_eeprom.h"
#include "app_flash.h"
#include "xprintf.h"

#define DUMP_RAM_UNIT_SIZE			256

static fatal_log_t fatal_log;

void sys_dbg_fatal(const int8_t* s, uint8_t c) {
	extern uint32_t _start_ram;
	extern uint32_t _estack;

	uint32_t len_of_ram = (uint32_t)&_estack - (uint32_t)&_start_ram;
	uint32_t ram_dump_num_64k_needed = (len_of_ram / FLASH_BLOCK_64K_SIZE) + 1;
	uint32_t index;

	unsigned char rev_c = 0;
	task_t*		ptemp_current_task;
	ak_msg_t*	ptemp_current_active_object;
	ak_msg_t	t_msg;
	exception_info_t t_exception_info;

#if defined(TIVA_PLATFORM)
	UARTprintf("%s\t%x\n", s, c);
#endif
#if defined(STM32L_PLATFORM) || defined(STM32F10X_PLATFORM)
	xprintf("%s\t%x\n", s, c);
#endif

	/* read fatal data from epprom */
	flash_read(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR, (uint8_t*)&fatal_log, sizeof(fatal_log_t));

	/* increase fatal time */
	fatal_log.fatal_times ++;

	/* set fatal string */
	memset(fatal_log.string, 0, 10);
	strcpy((char*)fatal_log.string, (const char*)s);

	/* set fatal code */
	fatal_log.code = c;

	/* get task fatal */
	ptemp_current_task = get_current_task_info();
	ptemp_current_task->id = get_current_task_id();

	/* get active object fatal */
	ptemp_current_active_object = get_current_active_object();

	/* get core register */
	fatal_log.m3_core_reg.ipsr		= __get_IPSR();
	fatal_log.m3_core_reg.primask	= __get_PRIMASK();
	fatal_log.m3_core_reg.faultmask	= __get_FAULTMASK();
	fatal_log.m3_core_reg.basepri	= __get_BASEPRI();
	fatal_log.m3_core_reg.control	= __get_CONTROL();

	memcpy(&fatal_log.current_task, ptemp_current_task, sizeof(task_t));
	memcpy(&fatal_log.current_active_object, ptemp_current_active_object, sizeof(ak_msg_t));

	/************************
	 *  trace irq info    *
	 ************************/
#if defined(AK_IRQ_OBJ_LOG_ENABLE)
	uint32_t flash_irq_log_address = APP_FLASH_AK_DBG_IRQ_LOG_SECTOR;
	SYS_PRINT("start write irq info\n");
	flash_erase_sector(flash_irq_log_address);
	while(log_queue_len(&log_irq_queue)) {
		log_queue_get(&log_irq_queue, &t_exception_info);
		flash_write(flash_irq_log_address, (uint8_t*)&t_exception_info, sizeof(exception_info_t));
		flash_irq_log_address += sizeof(exception_info_t);
	}
#endif

	/************************
	 *  trace fatal info    *
	 ************************/
	SYS_PRINT("start write fatal info\n");
	flash_erase_sector(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR);
	flash_write(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR, (uint8_t*)&fatal_log, sizeof(fatal_log_t));

	/************************
	 *  trace fatal message *
	 ************************/
#if defined(AK_TASK_OBJ_LOG_ENABLE)
	uint32_t flash_sys_log_address = APP_FLASH_AK_DBG_MSG_SECTOR_1;
	SYS_PRINT("start write message log to flash\n");
	flash_erase_sector(flash_sys_log_address);
	while(log_queue_len(&log_task_dbg_object_queue)) {
		log_queue_get(&log_task_dbg_object_queue, &t_msg);
		flash_write(flash_sys_log_address, (uint8_t*)&t_msg, sizeof(ak_msg_t));
		flash_sys_log_address += sizeof(ak_msg_t);
	}
#endif

	/************************
	 *  dump RAM to flash   *
	 ************************/
	SYS_PRINT("start dump RAM to FLASH\n");
	for (index = 0; index < ram_dump_num_64k_needed; index++) {
		flash_erase_block_64k(APP_FLASH_DUMP_RAM_START_ADDR + (FLASH_BLOCK_64K_SIZE * index));
		sys_ctrl_delay_us(100);
	}

	index = 0;
	while (index < len_of_ram) {
		flash_write(APP_FLASH_DUMP_RAM_START_ADDR + index, (uint8_t*)((uint32_t)&_start_ram + index), DUMP_RAM_UNIT_SIZE);
		index += DUMP_RAM_UNIT_SIZE;
	}

	sys_ctrl_delay_us(1000);

#if defined(RELEASE)
	sys_ctrl_reset();
#endif

	while(1) {
		/* reset watchdog */
		sys_ctrl_independent_watchdog_reset();
		sys_ctrl_soft_watchdog_reset();

		/* FATAL debug option */
		rev_c = sys_ctrl_shell_get_char();
		if (rev_c) {
			switch (rev_c) {
			/* system reset */
			case 'r':
				sys_ctrl_reset();
				break;

				/* dump RAM */
			case 'R': {
				index = 0;
				SYS_PRINT("\n[dump RAM]\n");
				for (uint32_t i = 0; i < len_of_ram; i++) {
					if (!(i % 8)) {
						/* reset watchdog */
						sys_ctrl_independent_watchdog_reset();
						sys_ctrl_soft_watchdog_reset();

						SYS_PRINT("\n0x%x\t" , (uint32_t)&_start_ram + i);
					}

					SYS_PRINT("%d\t", *((uint8_t*)((uint32_t)&_start_ram + i)));
				}
				SYS_PRINT("\n");
			}
				break;

				/* exception info */
			case 'e': {
				SYS_PRINT("\n[exception log]\n");
				uint32_t	flash_irq_log_address = APP_FLASH_AK_DBG_IRQ_LOG_SECTOR;
				for (uint32_t index = 0; index < (LOG_QUEUE_IRQ_SIZE / sizeof(exception_info_t)); index++) {
					/* reset watchdog */
					sys_ctrl_independent_watchdog_reset();
					sys_ctrl_soft_watchdog_reset();

					flash_read(flash_irq_log_address, (uint8_t*)&t_exception_info, sizeof(exception_info_t));
					flash_irq_log_address += sizeof(exception_info_t);

					SYS_PRINT("index: %d\texcept_number: %d\tirq_number: %d\ttimestamp: %d\n"\
							  , index										\
							  , t_exception_info.except_number																				\
							  , (int32_t)((int32_t)t_exception_info.except_number - (int32_t)SYS_IRQ_EXCEPTION_NUMBER_IRQ0_NUMBER_RESPECTIVE)	\
							  , t_exception_info.timestamp);
				}
			}
				break;

			case 'm': {
				SYS_PRINT("\n[active obj log]\n");
				uint32_t	flash_sys_log_address = APP_FLASH_AK_DBG_MSG_SECTOR_1;
				for (uint32_t index = 0; index < (LOG_QUEUE_OBJECT_SIZE / sizeof(ak_msg_t)); index++) {
					/* reset watchdog */
					sys_ctrl_independent_watchdog_reset();
					sys_ctrl_soft_watchdog_reset();

					flash_read(flash_sys_log_address, (uint8_t*)&t_msg, sizeof(ak_msg_t));
					flash_sys_log_address += sizeof(ak_msg_t);

					uint32_t wait_time;
					(void)wait_time;
					if (t_msg.dbg_handler.start_exe >= t_msg.dbg_handler.start_post) {
						wait_time = t_msg.dbg_handler.start_exe - t_msg.dbg_handler.start_post;
					}
					else {
						wait_time = t_msg.dbg_handler.start_exe + (0xFFFFFFFF - t_msg.dbg_handler.start_post);
					}

					uint32_t exe_time;
					(void)exe_time;
					if (t_msg.dbg_handler.stop_exe >= t_msg.dbg_handler.start_exe) {
						exe_time = t_msg.dbg_handler.stop_exe - t_msg.dbg_handler.start_exe;
					}
					else {
						exe_time = t_msg.dbg_handler.stop_exe + (0xFFFFFFFF - t_msg.dbg_handler.start_exe);
					}

					SYS_PRINT("index: %d\ttask_id: %d\tmsg_type:0x%x\tref_count:%d\tsig: %d\t\twait_time: %d\texe_time: %d\n"\
							  , index										\
							  , t_msg.des_task_id								\
							  , (t_msg.ref_count & AK_MSG_TYPE_MASK)		\
							  , (t_msg.ref_count & AK_MSG_REF_COUNT_MASK)	\
							  , t_msg.sig									\
							  , (wait_time)								\
							  , (exe_time));
				}
			}
				break;

			case 'f': {
				SYS_PRINT("\n[fatal info]\n");
				SYS_PRINT("\n");
				SYS_PRINT("[fatal] type: %s\n",		fatal_log.string);
				SYS_PRINT("[fatal] code: 0x%02X\n",	fatal_log.code);

				SYS_PRINT("\n");
				SYS_PRINT("[task] id: %d\n",		fatal_log.current_task.id);
				SYS_PRINT("[task] pri: %d\n",		fatal_log.current_task.pri);
				SYS_PRINT("[task] entry: 0x%x\n",	fatal_log.current_task.task);

				SYS_PRINT("\n");
				SYS_PRINT("[obj] task: %d\n",		fatal_log.current_active_object.des_task_id);
				SYS_PRINT("[obj] sig: %d\n",		fatal_log.current_active_object.sig);
				SYS_PRINT("[obj] type: 0x%x\n",		get_msg_type(&fatal_log.current_active_object));
				SYS_PRINT("[obj] ref count: %d\n",	get_msg_ref_count(&fatal_log.current_active_object));
				SYS_PRINT("[obj] wait time: %d\n",	fatal_log.current_active_object.dbg_handler.start_exe - fatal_log.current_active_object.dbg_handler.start_post);

				SYS_PRINT("\n");
				SYS_PRINT("[core] ipsr: %d\n",			fatal_log.m3_core_reg.ipsr);
				SYS_PRINT("[core] primask: 0x%08X\n",	fatal_log.m3_core_reg.primask);
				SYS_PRINT("[core] faultmask: 0x%08X\n",	fatal_log.m3_core_reg.faultmask);
				SYS_PRINT("[core] basepri: 0x%08X\n",	fatal_log.m3_core_reg.basepri);
				SYS_PRINT("[core] control: 0x%08X\n",	fatal_log.m3_core_reg.control);
			}
				break;

			case 'c': {
				sys_dbg_cpu_dump();
			}
				break;

			case 's': {
				sys_dbg_stack_space_dump();
			}
				break;

			default:
				break;
			}
		}

		/* led notify FATAL */
		led_life_on();
		sys_ctrl_delay_us(200000);
		led_life_off();
		sys_ctrl_delay_us(200000);
	}
}

void sys_dbg_func_stack_dump(uint32_t* args) {
	/**
	Stack frame contains:
	r0, r1, r2, r3, r12, r14, the return address and xPSR
	- Stacked R0	<-> args[0]
	- Stacked R1	<-> args[1]
	- Stacked R2	<-> args[2]
	- Stacked R3	<-> args[3]
	- Stacked R12	<-> args[4]
	- Stacked LR	<-> args[5]
	- Stacked PC	<-> args[6]
	- Stacked xPSR	<-> args[7]
	*/
	(void)args;
	SYS_PRINT("[st]R0\t0x%08X\n",		args[0]);
	SYS_PRINT("[st]R1\t0x%08X\n",		args[1]);
	SYS_PRINT("[st]R2\t0x%08X\n",		args[2]);
	SYS_PRINT("[st]R3\t0x%08X\n",		args[3]);
	SYS_PRINT("[st]R12\t0x%08X\n",	args[4]);
	SYS_PRINT("[st]LR\t0x%08X\n",		args[5]);
	SYS_PRINT("[st]PC\t0x%08X\n",		args[6]);
	SYS_PRINT("[st]PSR\t0x%08X\n",	args[7]);
}

void sys_dbg_cpu_dump() {
	SYS_PRINT("[cr]IPSR\t%d\n", __get_IPSR());
	SYS_PRINT("[cr]PRIMASK\t0x%08X\n", __get_PRIMASK());
	SYS_PRINT("[cr]FAULTMASK\t0x%08X\n", __get_FAULTMASK());
	SYS_PRINT("[cr]BASEPRI\t0x%08X\n", __get_BASEPRI());
	SYS_PRINT("[cr]CONTROL\t0x%08X\n", __get_CONTROL());
}

void sys_dbg_stack_space_dump() {
	extern uint32_t _estack;
	uint32_t* start_addr = (uint32_t*)((uint32_t)&_estack) - sizeof(uint32_t);
	uint32_t* end_addr = (uint32_t*)((uint32_t)__get_MSP());
	SYS_PRINT("--- sys_dbg_stack_space_dump ---\n");
	for (uint32_t* i = start_addr; i > end_addr ; i--) {
		SYS_PRINT("[%08X] %08X\n", i, *i);
	}
}
