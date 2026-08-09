/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   13/08/2016
 ******************************************************************************
**/

#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "ak.h"
#include "task.h"
#include "timer.h"
#include "message.h"

#include "cmd_line.h"
#include "xprintf.h"
#include "view_render.h"

#include "sys_ctrl.h"
#include "sys_io.h"
#include "sys_dbg.h"
#include "sys_irq.h"

#include "app.h"
#include "app_if.h"
#include "app_dbg.h"
#include "app_data.h"
#include "app_flash.h"
#include "app_eeprom.h"
#include "app_non_clear_ram.h"
#include "app_modbus_pull.h"

#include "task_shell.h"
#include "task_list.h"
#include "task_list_if.h"
#include "task_if.h"
#include "task_life.h"

#include "led.h"
#include "eeprom.h"
#include "Adafruit_oled_drv.h"
#include "flash.h"
#include "buzzer.h"

#include "qrcode.h"

/*****************************************************************************/
/*  local declare
 */
/*****************************************************************************/
#define STR_LIST_MAX_SIZE		8
#define STR_BUFFER_SIZE			128

static char cmd_buffer[STR_BUFFER_SIZE];
static char* str_list[STR_LIST_MAX_SIZE];
static uint8_t str_list_len;

static uint8_t str_parser(char* str);
static char* str_parser_get_attr(uint8_t);
static void shell_lcd_dump_framebuffer();

/*****************************************************************************/
/*  command function declare
 */
/*****************************************************************************/
int32_t shell_reset(uint8_t* argv);
int32_t shell_ver(uint8_t* argv);
int32_t shell_help(uint8_t* argv);
int32_t shell_reboot(uint8_t* argv);
int32_t shell_ram(uint8_t* argv);
int32_t shell_fatal(uint8_t* argv);
int32_t shell_stt(uint8_t* argv);
int32_t shell_epi(uint8_t* argv);
int32_t shell_eps(uint8_t* argv);
int32_t shell_flash(uint8_t* argv);
int32_t shell_lcd(uint8_t* argv);
int32_t shell_dbg(uint8_t* argv);
int32_t shell_boot(uint8_t* argv);
int32_t shell_fwu(uint8_t* argv);
int32_t shell_psv(uint8_t* argv);
int32_t shell_buzzer(uint8_t* argv);
int32_t shell_modbus(uint8_t* argv);

/*****************************************************************************/
/*  command table
 */
/*****************************************************************************/
const cmd_line_t lgn_cmd_table[] = {

	/*************************************************************************/
	/* system command */
	/*************************************************************************/
	{(const int8_t*)"reset",	shell_reset,		(const int8_t*)"reset terminal"},
	{(const int8_t*)"ver",		shell_ver,			(const int8_t*)"version info"},
	{(const int8_t*)"help",		shell_help,			(const int8_t*)"help info"},
	{(const int8_t*)"reboot",	shell_reboot,		(const int8_t*)"reboot"},
	{(const int8_t*)"ram",		shell_ram,			(const int8_t*)"ram"},
	{(const int8_t*)"epi",		shell_epi,			(const int8_t*)"epprom init"},
	{(const int8_t*)"fatal",	shell_fatal,		(const int8_t*)"fatal info"},
	{(const int8_t*)"stt",		shell_stt,			(const int8_t*)"app status"},
	{(const int8_t*)"eps",		shell_eps,			(const int8_t*)"epprom"},
	{(const int8_t*)"flash",	shell_flash,		(const int8_t*)"flash"},
	{(const int8_t*)"lcd",		shell_lcd,			(const int8_t*)"lcd"},
	{(const int8_t*)"boot",		shell_boot,			(const int8_t*)"boot share"},
	{(const int8_t*)"fwu",		shell_fwu,			(const int8_t*)"app burn firmware"},
	{(const int8_t*)"psv",		shell_psv,			(const int8_t*)"psv"},
	{(const int8_t*)"beep",		shell_buzzer,		(const int8_t*)"buzzer play tones"},
	{(const int8_t*)"modbus",	shell_modbus,		(const int8_t*)"modbus master"},

	/*************************************************************************/
	/* debug command */
	/*************************************************************************/
	{(const int8_t*)"dbg",		shell_dbg,			(const int8_t*)"dbg"},

	/* End Of Table */
	{(const int8_t*)0,(pf_cmd_func)0,(const int8_t*)0}
};

uint8_t str_parser(char* str) {
	strcpy(cmd_buffer, str);
	str_list_len = 0;

	uint8_t i = 0;
	uint8_t str_list_index = 0;
	uint8_t flag_insert_str = 1;

	while (cmd_buffer[i] != 0 && cmd_buffer[i] != '\n' && cmd_buffer[i] != '\r') {
		if (cmd_buffer[i] == ' ') {
			cmd_buffer[i] = 0;
			flag_insert_str = 1;
		}
		else if (flag_insert_str) {
			str_list[str_list_index++] = &cmd_buffer[i];
			flag_insert_str = 0;
		}
		i++;
	}

	cmd_buffer[i] = 0;

	str_list_len = str_list_index;
	return str_list_len;
}

char* str_parser_get_attr(uint8_t index) {
	if (index < str_list_len) {
		return str_list[index];
	}
	return NULL;
}

/*****************************************************************************/
/*  command function definaion
 */
/*****************************************************************************/
int32_t shell_reset(uint8_t* argv) {
	(void)argv;
	xprintf("\033[2J\r");
	return 0;
}

int32_t shell_ver(uint8_t* argv) {
	(void)argv;

	firmware_header_t firmware_header;
	sys_ctrl_get_firmware_info(&firmware_header);

	LOGIN_PRINT("Kernel version: %s\n", AK_VERSION);
	LOGIN_PRINT("App version: %d.%d.%d.%d\n", app_info.version[0] \
			, app_info.version[1] \
			, app_info.version[2] \
			, app_info.version[3]);
	LOGIN_PRINT("Firmware checksum: %04x\n", firmware_header.checksum);
	LOGIN_PRINT("Firmware length: %d\n", firmware_header.bin_len);

	LOGIN_PRINT("\nSystem information:\n");
	LOGIN_PRINT("\tFLASH used:\t%d bytes\n", system_info.flash_used);
	LOGIN_PRINT("\tSRAM used:\t%d bytes\n", system_info.ram_used);
	LOGIN_PRINT("\t\tdata init size:\t\t%d bytes\n", system_info.data_init_size);
	LOGIN_PRINT("\t\tdata non_init size:\t%d bytes\n", system_info.data_non_init_size);
	LOGIN_PRINT("\t\tstack avail:\t\t%d bytes\n", system_info.stack_avail);
	LOGIN_PRINT("\t\theap avail:\t\t%d bytes\n", system_info.heap_avail);
	LOGIN_PRINT("\t\tother:\t\t\t%d bytes\n", system_info.ram_other);
	LOGIN_PRINT("\n");
	LOGIN_PRINT("\tcpu clock:\t%d Hz\n", system_info.cpu_clock);
	LOGIN_PRINT("\ttime tick:\t%d ms\n", system_info.tick);
	LOGIN_PRINT("\tconsole:\t%d bps\n", system_info.console_baudrate);
	LOGIN_PRINT("\n");
	LOGIN_PRINT("\tVCC:\t%d mV\n", sys_ctr_get_vbat_voltage());
	LOGIN_PRINT("\tTEMP:\t%d *C\n", sys_ctr_get_mcu_temperature());
	LOGIN_PRINT("\n\n");
	return 0;
}

int32_t shell_help(uint8_t* argv) {
	uint32_t idx = 0;
	switch (*(argv + 4)) {
	default:
		LOGIN_PRINT("\nCOMMANDS INFORMATION:\n\n");
		while(lgn_cmd_table[idx].cmd != (const int8_t*)0) {
			LOGIN_PRINT("%s\t-> %s\n\n", lgn_cmd_table[idx].cmd, lgn_cmd_table[idx].info);
			idx++;
		}
		break;
	}
	return 0;
}

int32_t shell_reboot(uint8_t* argv) {
	(void)argv;
	sys_ctrl_delay_ms(10);
	sys_ctrl_reset();
	return 0;
}

int32_t shell_fatal(uint8_t* argv) {
	fatal_log_t login_fatal_log;
	ak_msg_t t_msg;
	exception_info_t t_exception_info;

	switch (*(argv + 6)) {
	case 't':
		FATAL("TEST", 0x02);
		break;

	case '!':
		while(1);
		break;

	case '@':
		DISABLE_INTERRUPTS();
		while(1);
		break;

	case 'r':
		memset((uint8_t*)&login_fatal_log, 0, sizeof(fatal_log_t));
		flash_erase_sector(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR);
		flash_write(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR, (uint8_t*)&login_fatal_log, sizeof(fatal_log_t));
		LOGIN_PRINT("reset fatal log OK\n");
		break;

	case 'l': {
		flash_read(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR, (uint8_t*)&login_fatal_log, sizeof(fatal_log_t));

		LOGIN_PRINT("[times] fatal: %d\n",		login_fatal_log.fatal_times);
		LOGIN_PRINT("[times] restart: %d\n",	login_fatal_log.restart_times);

		LOGIN_PRINT("\n");
		LOGIN_PRINT("[fatal] type: %s\n",		login_fatal_log.string);
		LOGIN_PRINT("[fatal] code: 0x%02X\n",	login_fatal_log.code);

		LOGIN_PRINT("\n");
		LOGIN_PRINT("[task] id: %d\n",			login_fatal_log.current_task.id);
		LOGIN_PRINT("[task] pri: %d\n",			login_fatal_log.current_task.pri);
		LOGIN_PRINT("[task] entry: 0x%x\n",		login_fatal_log.current_task.task);

		LOGIN_PRINT("\n");
		LOGIN_PRINT("[obj] task: %d\n",			login_fatal_log.current_active_object.des_task_id);
		LOGIN_PRINT("[obj] sig: %d\n",			login_fatal_log.current_active_object.sig);
		LOGIN_PRINT("[obj] type: 0x%x\n",		get_msg_type(&login_fatal_log.current_active_object));
		LOGIN_PRINT("[obj] ref count: %d\n",	get_msg_ref_count(&login_fatal_log.current_active_object));
		LOGIN_PRINT("[obj] wait time: %d\n",	login_fatal_log.current_active_object.dbg_handler.start_exe - login_fatal_log.current_active_object.dbg_handler.start_post);

		LOGIN_PRINT("\n");
		LOGIN_PRINT("[core] ipsr: %d\n",			login_fatal_log.m3_core_reg.ipsr);
		LOGIN_PRINT("[core] primask: 0x%08X\n",		login_fatal_log.m3_core_reg.primask);
		LOGIN_PRINT("[core] faultmask: 0x%08X\n",	login_fatal_log.m3_core_reg.faultmask);
		LOGIN_PRINT("[core] basepri: 0x%08X\n",		login_fatal_log.m3_core_reg.basepri);
		LOGIN_PRINT("[core] control: 0x%08X\n",		login_fatal_log.m3_core_reg.control);

		LOGIN_PRINT("\n");
		LOGIN_PRINT("[irq] IRQ number: %d\n",			(int32_t)((int32_t)login_fatal_log.m3_core_reg.ipsr - (int32_t)SYS_IRQ_EXCEPTION_NUMBER_IRQ0_NUMBER_RESPECTIVE));
	}
		break;

	case 'm': {
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

			sys_ctrl_delay_ms(5);

			LOGIN_PRINT("index: %d\ttask_id: %d\tmsg_type:0x%x\tref_count:%d\tsig: %d\t\twait_time: %d\texe_time: %d\n"\
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

	case 'e': {
		uint32_t	flash_irq_log_address = APP_FLASH_AK_DBG_IRQ_LOG_SECTOR;
		for (uint32_t index = 0; index < (LOG_QUEUE_IRQ_SIZE / sizeof(exception_info_t)); index++) {
			/* reset watchdog */
			sys_ctrl_independent_watchdog_reset();
			sys_ctrl_soft_watchdog_reset();

			flash_read(flash_irq_log_address, (uint8_t*)&t_exception_info, sizeof(exception_info_t));
			flash_irq_log_address += sizeof(exception_info_t);

			LOGIN_PRINT("index: %d\texcept_number: %d\tirq_number: %d\ttimestamp: %d\n"\
						, index										\
						, t_exception_info.except_number																				\
						, (int32_t)((int32_t)t_exception_info.except_number - (int32_t)SYS_IRQ_EXCEPTION_NUMBER_IRQ0_NUMBER_RESPECTIVE)	\
						, t_exception_info.timestamp);
		}
	}
		break;

	case 'R': {
		uint8_t len = str_parser((char*)argv);
		if (len == 3) {
			extern uint32_t _start_ram;
			extern uint32_t _estack;

			uint8_t val;
			uint32_t len_of_ram = (uint32_t)&_estack - (uint32_t)&_start_ram;

			uint32_t colum = (uint32_t)strtol(str_parser_get_attr(2), NULL, 0);

			LOGIN_PRINT("\n");
			for (uint32_t i = 0; i < len_of_ram; i++) {
				if (!(i % colum)) {
					/* reset watchdog */
					sys_ctrl_independent_watchdog_reset();
					sys_ctrl_soft_watchdog_reset();

					LOGIN_PRINT("\n0x%x\t" , (uint32_t)&_start_ram + i);
				}

				flash_read(APP_FLASH_DUMP_RAM_START_ADDR + i, &val, sizeof(uint8_t));
				LOGIN_PRINT("%d\t", val);
			}
			LOGIN_PRINT("\n");
		}
		else {
			LOGIN_PRINT("syntax error\n");
		}
	}
		break;

	default:
		break;
	}

	return 0;
}

int32_t shell_stt(uint8_t* argv) {
	(void)argv;
	return 0;
}

int32_t shell_epi(uint8_t* argv) {
	(void)argv;
	return 0;
}

int32_t shell_eps(uint8_t* argv) {
	uint8_t val = 0;

	switch (*(argv + 4)) {
	case 'd': {					/* data DEC format */
		LOGIN_PRINT("\n");
		for (uint32_t i = 0; i < EEPROM_END_ADDR; i++) {
			if (!(i%16)) {
				/* reset watchdog */
				sys_ctrl_independent_watchdog_reset();
				sys_ctrl_soft_watchdog_reset();

				LOGIN_PRINT("\n0x%x\t" ,i);
			}
			eeprom_read(i, &val, sizeof(uint8_t));
			LOGIN_PRINT("%d\t", val);
		}
		LOGIN_PRINT("\n");
	}
		break;

	case 'h': {					/* data HEX format */
		LOGIN_PRINT("\n");
		for (uint32_t i = 0; i < EEPROM_END_ADDR; i++) {
			if (!(i%16)) {
				/* reset watchdog */
				sys_ctrl_independent_watchdog_reset();
				sys_ctrl_soft_watchdog_reset();

				LOGIN_PRINT("\n0x%x\t" ,i);
			}
			eeprom_read(i, &val, sizeof(uint8_t));
			LOGIN_PRINT("0x%x\t", val);
		}
		LOGIN_PRINT("\n");
	}
		break;

	case 'r': {
		LOGIN_PRINT("erasing...\n");
		eeprom_erase(EEPROM_START_ADDR, EEPROM_END_ADDR - EEPROM_START_ADDR);
		LOGIN_PRINT("completed\n");
	}
		break;

	default:
		LOGIN_PRINT("unkown option !\n");
		break;
	}

	return 0;
}

int32_t shell_flash(uint8_t* argv) {
	/* "flash d 0x1000 0xA000" */
	switch (*(argv + 6)) {
	case 'i':
		LOGIN_PRINT("flash writing.. ");
		flash_erase_sector(APP_FLASH_AK_DBG_FATAL_LOG_SECTOR);
		LOGIN_PRINT("completed!\n");
		break;

	case 'd': {
		uint8_t val;
		uint8_t len = str_parser((char*)argv);
		if (len == 4) {
			char* str_start_addr = str_parser_get_attr(2);
			char* str_stop_addr = str_parser_get_attr(3);
			uint32_t start_addr = strtol(str_start_addr, NULL, 0);
			uint32_t stop_addr = strtol(str_stop_addr, NULL, 0);
			LOGIN_PRINT("start_addr: 0x%x\n", start_addr);
			LOGIN_PRINT("stop_addr: 0x%x\n", stop_addr);

			LOGIN_PRINT("\n");
			for (uint32_t i = start_addr; i < stop_addr; i++) {
				if (!(i%16)) {
					/* reset watchdog */
					sys_ctrl_independent_watchdog_reset();
					sys_ctrl_soft_watchdog_reset();

					LOGIN_PRINT("\n0x%x\t" ,i);
				}
				flash_read(i, &val, sizeof(uint8_t));
				LOGIN_PRINT("%d\t", val);
			}
			LOGIN_PRINT("\n");
		}
	}
		break;

	case 'h': {
		uint8_t val;
		uint8_t len = str_parser((char*)argv);
		if (len == 4) {
			char* str_start_addr = str_parser_get_attr(2);
			char* str_stop_addr = str_parser_get_attr(3);
			uint32_t start_addr = strtol(str_start_addr, NULL, 0);
			uint32_t stop_addr = strtol(str_stop_addr, NULL, 0);
			LOGIN_PRINT("start_addr: 0x%x\n", start_addr);
			LOGIN_PRINT("stop_addr: 0x%x\n", stop_addr);

			LOGIN_PRINT("\n");
			for (uint32_t i = start_addr; i < stop_addr; i++) {
				if (!(i%16)) {
					/* reset watchdog */
					sys_ctrl_independent_watchdog_reset();
					sys_ctrl_soft_watchdog_reset();

					LOGIN_PRINT("\n0x%x\t" ,i);
				}
				flash_read(i, &val, sizeof(uint8_t));
				LOGIN_PRINT("0x%x\t", val);
			}
			LOGIN_PRINT("\n");
		}
	}
		break;

	default:
		LOGIN_PRINT("unknow option\n");
		break;
	}

	return 0;
}

int32_t shell_lcd(uint8_t* argv) {
	switch (*(argv + 4)) {
	case 'i':
		view_render.initialize();
		break;

	case 'o':
		view_render.display_on();
		break;

	case 'f':
		view_render.display_off();
		break;

	case 'b':
		view_render.fillScreen(BLACK);
		view_render.update();
		break;

	case 'w':
		view_render.fillScreen(WHITE);
		view_render.update();
		break;

	case 't':
		/* ak logo */
#define AK_LOGO_AXIS_X	23
#define AK_LOGO_TEXT	(AK_LOGO_AXIS_X + 4)

		view_render.setTextSize(1);
		view_render.setTextColor(WHITE);
		view_render.setCursor(AK_LOGO_AXIS_X, 3);
		view_render.print("   __    _  _ ");
		view_render.setCursor(AK_LOGO_AXIS_X, 10);
		view_render.print("  /__\\  ( )/ )");
		view_render.setCursor(AK_LOGO_AXIS_X, 20);
		view_render.print(" /(__)\\ (   (");
		view_render.setCursor(AK_LOGO_AXIS_X, 30);
		view_render.print("(__)(__)(_)\\_)");
		view_render.setCursor(AK_LOGO_TEXT, 42);
		view_render.print("Active Kernel");
		view_render.update ();
		break;

	case 'r':
		view_render.clear ();
		break;

	case 'a':
		view_render.setTextSize (2);
		view_render.setTextColor (WHITE);
		view_render.setCursor (10, 10);
		view_render.print ("12345");
		view_render.update ();
		break;

	case 'c':
		view_render.setTextSize (2);
		view_render.setTextColor (BLACK);
		view_render.setCursor (10, 40);
		view_render.print ("abcd");
		view_render.update ();
		break;

	case 'p':
		// draw a single pixel
		view_render.drawPixel(9, 1, WHITE);
		view_render.drawPixel(13, 4, WHITE);
		view_render.drawPixel(16, 7, WHITE);
		view_render.update ();
		break;

	case 'd':
		shell_lcd_dump_framebuffer();
		break;

	default:
		LOGIN_PRINT("unknow option\n");
		break;
	}

	return 0;
}

void shell_lcd_dump_framebuffer() {
	const unsigned char* frame_buffer = view_render.getFrameBuffer();
	unsigned int frame_buffer_size = view_render.getFrameBufferSize();

	if (frame_buffer == NULL) {
		LOGIN_PRINT("lcd framebuffer is not initialized\n");
		return;
	}

	LOGIN_PRINT("[DUMP] frame buffer lcd => start\n");
	LOGIN_PRINT("width=128 height=64 bytes=%d\n", frame_buffer_size);

	for (unsigned int i = 0; i < frame_buffer_size; i++) {
		if ((i % 16) == 0) {
			sys_ctrl_independent_watchdog_reset();
			sys_ctrl_soft_watchdog_reset();
			LOGIN_PRINT("\n");
		}
		LOGIN_PRINT("0x%02X", frame_buffer[i]);
		if ((i + 1) < frame_buffer_size) {
			LOGIN_PRINT(",");
		}
	}

	LOGIN_PRINT("\n[DUMP] frame buffer lcd => end\n");
}

/* https://www.charbase.com */
int32_t shell_dbg(uint8_t* argv) {
	(void)(argv);
	switch (*(argv + 4)) {
	case '0': {
#if defined(TASK_ZIGBEE_EN)
		task_post_pure_msg(AC_TASK_ZIGBEE_ID, AC_ZIGBEE_PERMIT_JOINING_REQ);
#endif
	}
		break;

	case 'v': {
		uint32_t vbat;
		(void)vbat;
		vbat = sys_ctr_get_vbat_voltage();
		LOGIN_PRINT("vbat: %d\n", vbat);
	}
		break;

	case 't': {
		uint32_t temperature;
		(void)temperature;
		temperature = sys_ctr_get_mcu_temperature();
		LOGIN_PRINT("temperature: %d\n", temperature);
	}
		break;

	case 's': {
		sys_ctr_stop_mcu();
	}
		break;

	default: {
	}
		break;
	}

	return 0;
}

int32_t shell_ram(uint8_t* argv) {
	extern uint32_t _start_ram;
	extern uint32_t _estack;

	char* str_start_addr = NULL;
	char* str_stop_addr = NULL;
	uint8_t* start_addr = NULL;
	uint8_t* stop_addr = NULL;

	uint8_t len = str_parser((char*)argv);

	LOGIN_PRINT("RAM start: 0x%x\n", ((uint32_t)&_start_ram));
	LOGIN_PRINT("RAM   end: 0x%x\n", ((uint32_t)&_estack));

	/* "ram x 0x1000 0xA000" */
	switch (*(argv + 4)) {
	case 'r': {
		if (len == 4) {
			str_start_addr	= str_parser_get_attr(2);
			str_stop_addr	= str_parser_get_attr(3);
			start_addr	= (uint8_t*)((uint32_t)strtol(str_start_addr, NULL, 0));
			stop_addr 	= (uint8_t*)((uint32_t)strtol(str_stop_addr, NULL, 0));

			LOGIN_PRINT("start_addr: 0x%x\n", start_addr);
			LOGIN_PRINT("stop_addr: 0x%x\n", stop_addr);

			if ((uint32_t)start_addr >= ((uint32_t)&_start_ram) && (uint32_t)stop_addr <= ((uint32_t)&_estack)) {

				/* start dump ram */
				LOGIN_PRINT("\n");

				for (uint8_t* addr_index = start_addr; addr_index < stop_addr; addr_index++) {
					if (!((uint32_t)addr_index % 4)) {
						/* reset watchdog */
						sys_ctrl_independent_watchdog_reset();
						sys_ctrl_soft_watchdog_reset();

						LOGIN_PRINT("\n0x%x\t", addr_index);
					}

					*addr_index = 0;
					LOGIN_PRINT("%d\t", *addr_index);
				}

				LOGIN_PRINT("\n");
				/* end dump ram */
			}
		}
		else {
			LOGIN_PRINT("syntax error\n");
		}
	}
		break;

	case 'd': { /* ram d 0x20000000 0x20004000 */
		if (len == 4) {
			str_start_addr	= str_parser_get_attr(2);
			str_stop_addr	= str_parser_get_attr(3);
			start_addr	= (uint8_t*)((uint32_t)strtol(str_start_addr, NULL, 0));
			stop_addr 	= (uint8_t*)((uint32_t)strtol(str_stop_addr, NULL, 0));

			LOGIN_PRINT("start_addr: 0x%x\n", start_addr);
			LOGIN_PRINT("stop_addr: 0x%x\n", stop_addr);

			if ((uint32_t)start_addr >= ((uint32_t)&_start_ram) && (uint32_t)stop_addr <= ((uint32_t)&_estack)) {

				/* start dump ram */
				LOGIN_PRINT("\n");

				for (uint8_t* addr_index = start_addr; addr_index < stop_addr; addr_index++) {
					if (!((uint32_t)addr_index % 4)) {
						/* reset watchdog */
						sys_ctrl_independent_watchdog_reset();
						sys_ctrl_soft_watchdog_reset();

						LOGIN_PRINT("\n0x%x\t", addr_index);
					}

					LOGIN_PRINT("%d\t", *addr_index);
				}

				LOGIN_PRINT("\n");
				/* end dump ram */
			}
		}
		else {
			LOGIN_PRINT("syntax error\n");
		}
	}
		break;

	case 'h': {
		if (len == 4) {
			str_start_addr	= str_parser_get_attr(2);
			str_stop_addr	= str_parser_get_attr(3);
			start_addr	= (uint8_t*)((uint32_t)strtol(str_start_addr, NULL, 0));
			stop_addr 	= (uint8_t*)((uint32_t)strtol(str_stop_addr, NULL, 0));

			LOGIN_PRINT("start_addr: 0x%x\n", start_addr);
			LOGIN_PRINT("stop_addr: 0x%x\n", stop_addr);

			if ((uint32_t)start_addr >= ((uint32_t)&_start_ram) && (uint32_t)stop_addr <= ((uint32_t)&_estack)) {

				/* start dump ram */
				LOGIN_PRINT("\n");

				for (uint8_t* addr_index = start_addr; addr_index < stop_addr; addr_index++) {
					if (!((uint32_t)addr_index % 4)) {
						/* reset watchdog */
						sys_ctrl_independent_watchdog_reset();
						sys_ctrl_soft_watchdog_reset();

						LOGIN_PRINT("\n0x%x\t", addr_index);
					}

					LOGIN_PRINT("0x%x\t", *addr_index);
				}

				LOGIN_PRINT("\n");
				/* end dump ram */
			}
		}
		else {
			LOGIN_PRINT("syntax error\n");
		}
	}
		break;

	case 's': {
		LOGIN_PRINT("Stack Size: %d\n", sys_stack_get_size());
		LOGIN_PRINT("Stack Usage: %d\n", sys_stack_usage());
		sys_dbg_stack_space_dump();
	}
		break;

	case 'c': {
		sys_dbg_cpu_dump();
	}
		break;

	default: {
		LOGIN_PRINT("unknown option\n");
	}
		break;
	}

	return 0;
}

int32_t shell_boot(uint8_t* argv) {
	switch (*(argv + 5)) {
	case 'i': {
		sys_boot_t sys_boot;
		sys_boot_get(&sys_boot);
		LOGIN_PRINT("\n[b_fwc] psk: 0x%08X checksum: 0x%08X bin_len: %d\n", \
					sys_boot.current_fw_boot_header.psk, \
					sys_boot.current_fw_boot_header.checksum, \
					sys_boot.current_fw_boot_header.bin_len);
		LOGIN_PRINT("[b_fwu] psk: 0x%08X checksum: 0x%08X bin_len: %d\n", \
					sys_boot.update_fw_boot_header.psk, \
					sys_boot.update_fw_boot_header.checksum, \
					sys_boot.update_fw_boot_header.bin_len);
		LOGIN_PRINT("[b_cmd] cmd: %d container:%d io_driver: %d des_addr: 0x%08X src_addr: 0x%08X\n", \
					sys_boot.fw_boot_cmd.cmd, \
					sys_boot.fw_boot_cmd.container, \
					sys_boot.fw_boot_cmd.io_driver, \
					sys_boot.fw_boot_cmd.des_addr, \
					sys_boot.fw_boot_cmd.src_addr);

		LOGIN_PRINT("\n[a_fwc] psk: 0x%08X checksum: 0x%08X bin_len: %d\n", \
					sys_boot.current_fw_app_header.psk, \
					sys_boot.current_fw_app_header.checksum, \
					sys_boot.current_fw_app_header.bin_len);
		LOGIN_PRINT("[a_fwu] psk: 0x%08X checksum: 0x%08X bin_len: %d\n", \
					sys_boot.update_fw_app_header.psk, \
					sys_boot.update_fw_app_header.checksum, \
					sys_boot.update_fw_app_header.bin_len);
		LOGIN_PRINT("[a_cmd] cmd: %d container:%d io_driver: %d des_addr: 0x%08X src_addr: 0x%08X\n", \
					sys_boot.fw_app_cmd.cmd, \
					sys_boot.fw_app_cmd.container, \
					sys_boot.fw_app_cmd.io_driver, \
					sys_boot.fw_app_cmd.des_addr, \
					sys_boot.fw_app_cmd.src_addr);
	}
		break;

	case 'r': {
		sys_boot_t sys_boot;
		memset(&sys_boot, 0, sizeof(sys_boot_t));
		sys_boot_set(&sys_boot);
	}
		break;

	case 't': {
		sys_boot_t sb;
		sys_boot_get(&sb);

		/* cmd update request */
		sb.fw_app_cmd.cmd			= SYS_BOOT_CMD_UPDATE_REQ;
		sb.fw_app_cmd.container		= SYS_BOOT_CONTAINER_EXTERNAL_FLASH;
		sb.fw_app_cmd.io_driver		= SYS_BOOT_IO_DRIVER_NONE;
		sb.fw_app_cmd.des_addr		= APP_START_ADDR;
		sb.fw_app_cmd.src_addr		= APP_FLASH_FIRMWARE_START_ADDR;

		sys_boot_set(&sb);
	}
		break;

	default:
		break;
	}

	return 0;
}

int32_t shell_fwu(uint8_t* argv) {
	(void)argv;
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
	return 0;
}

int32_t shell_psv(uint8_t* argv) {
	(void)argv;
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	return 0;
}

int32_t shell_buzzer(uint8_t* argv) {
	switch (*(argv + 5)) {
	case 'i': {
		BUZZER_Init();
	}
		break;

	case '1': {
		BUZZER_PlaySound(BUZZER_SOUND_STARTUP);
	}
		break;

	case '2': {
		BUZZER_PlaySound(BUZZER_SOUND_3BEEP);
	}
		break;

	case '3': {
		BUZZER_PlaySound(BUZZER_SOUND_SUPER_MARIO);
	}
		break;

	case '4': {
		BUZZER_PlaySound(BUZZER_SOUND_MERRY_CHRISTMAS);
	}
		break;

	default:
		LOGIN_PRINT("\n[HELP]\n");
		LOGIN_PRINT("1. \"beep i\"                           : init buzzer play tones \n");
		LOGIN_PRINT("2. \"beep 1\"                           : buzzer play tones startup \n");
		LOGIN_PRINT("3. \"beep 2\"                           : buzzer play tones three beeps \n");
		LOGIN_PRINT("4. \"beep 3\"                           : buzzer play tones super mario bros \n");
		LOGIN_PRINT("4. \"beep 4\"                           : buzzer play tones merry chrismast \n");
		break;
	}

	return 0;
}

int32_t shell_modbus(uint8_t* argv) {
	switch (*(argv + 7)) {
	case 'r':
		LOGIN_PRINT("Modbus polling all register: \n");
		updateDataModbusDevice(&MB_ES35SW_TH_Sensor);
		LOGIN_PRINT("--ES35-SW--\n");
		for (uint16_t i = 0; i < MB_ES35SW_TH_Sensor.listRegAmount; i++) {
			LOGIN_PRINT("regAddr[%d]: %d\t", i+1, MB_ES35SW_TH_Sensor.listRegDevice[i].regAddress);
			LOGIN_PRINT("\trawValue: %d \n", MB_ES35SW_TH_Sensor.listRegDevice[i].regValue);
		}

		LOGIN_PRINT("\n--LHIO404--\n");
		updateDataModbusDevice(&MB_LHIO404_IO_Device);
		for (uint16_t i = 0; i < MB_LHIO404_IO_Device.listRegAmount; i++) {
			LOGIN_PRINT("regAddr[%d]: %d\t", i+1, MB_LHIO404_IO_Device.listRegDevice[i].regAddress);
			LOGIN_PRINT("\trawValue: %d \n", MB_LHIO404_IO_Device.listRegDevice[i].regValue);
		}
		LOGIN_PRINT("\nDone !\n");
		break;

	default:
		LOGIN_PRINT("\n[HELP]\n");
		LOGIN_PRINT("\nmodbus r\"            : read all register\n");
		break;
	}

	return 0;
}
