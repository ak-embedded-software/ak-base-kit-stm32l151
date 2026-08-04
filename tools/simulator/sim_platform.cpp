/**
 ******************************************************************************
 * @brief:  Lớp phần cứng giả cho bản chạy trên máy tính.
 *
 *  Tất cả những gì bo mạch cung cấp mà cái laptop không có thì nằm hết ở đây:
 *  critical section, đồng hồ hệ thống, EEPROM, còi, watchdog. Kernel AK và
 *  toàn bộ logic game phía trên KHÔNG biết mình đang chạy giả lập.
 *
 *  Khác biệt duy nhất đáng kể so với bo thật:
 *    - EEPROM lưu ra file thay vì ô nhớ trong chip, để tắt chương trình rồi
 *      mở lại vẫn bấm Continue được (đúng như rút điện bo rồi cắm lại).
 *    - Còi chỉ đếm số lần kêu, không phát ra tiếng.
 ******************************************************************************
**/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "sim.h"

extern "C" {
#include "ak.h"
#include "task.h"
#include "message.h"
#include "timer.h"
}
#include "buzzer.h"

/*****************************************************************************/
/*  Critical section
 *
 *  Trên bo là bật/tắt ngắt. Ở đây chỉ có một luồng nên chỉ cần đếm độ sâu
 *  lồng nhau, đủ để kernel kiểm tra cân bằng entry/exit.
 */
/*****************************************************************************/
static int nest = 0;
extern "C" void enable_interrupts() {}
extern "C" void disable_interrupts() {}
extern "C" void entry_critical() { nest++; }
extern "C" void exit_critical() { nest--; }
extern "C" int  get_nest_entry_critical_counter() { return nest; }

/*****************************************************************************/
/*  Đồng hồ hệ thống
 *
 *  sim_ms do vòng lặp chính tăng, mỗi lần tăng 1 tương ứng 1 ms trên bo.
 */
/*****************************************************************************/
uint32_t sim_ms = 0;

extern "C" void sys_ctrl_reset() { printf("[sim] sys_ctrl_reset() - firmware yeu cau reset\n"); }
extern "C" void sys_ctrl_independent_watchdog_init() {}
extern "C" void sys_ctrl_independent_watchdog_reset() {}
extern "C" void sys_ctrl_soft_watchdog_init(uint32_t) {}
extern "C" void sys_ctrl_soft_watchdog_reset() {}
extern "C" void sys_ctrl_soft_watchdog_enable() {}
extern "C" void sys_ctrl_soft_watchdog_disable() {}
extern "C" void sys_ctrl_soft_watchdog_increase_counter() {}
extern "C" void sys_ctrl_delay(volatile uint32_t) {}
extern "C" void sys_ctrl_delay_ms(volatile uint32_t) {}
extern "C" void sys_ctrl_delay_us(volatile uint32_t) {}
extern "C" uint32_t sys_ctrl_micros() { return sim_ms * 1000; }
extern "C" uint32_t sys_ctrl_millis() { return sim_ms; }
extern "C" void sys_ctrl_shell_sw_to_block() {}
extern "C" void sys_ctrl_shell_sw_to_nonblock() {}

/*****************************************************************************/
/*  EEPROM
 *
 *  4 KB, giống hệt EEPROM trong STM32L151. Nạp từ file lúc khởi động, ghi lại
 *  file sau mỗi lần eeprom_write để tắt ngang chương trình cũng không mất.
 */
/*****************************************************************************/
static uint8_t ee[4096];
static const char* ee_path = 0;
long sim_ee_writes = 0;

void sim_eeprom_open(const char* path) {
	ee_path = path;
	memset(ee, 0, sizeof(ee));
	FILE* f = fopen(path, "rb");
	if (f) {
		size_t n = fread(ee, 1, sizeof(ee), f);
		fclose(f);
		printf("[sim] doc EEPROM tu %s (%zu byte)\n", path, n);
	}
	else {
		printf("[sim] chua co %s, EEPROM bat dau trang\n", path);
	}
}

void sim_eeprom_wipe() {
	memset(ee, 0, sizeof(ee));
	if (ee_path) { remove(ee_path); }
	printf("[sim] da xoa sach EEPROM\n");
}

static void ee_flush() {
	if (!ee_path) { return; }
	FILE* f = fopen(ee_path, "wb");
	if (f) { fwrite(ee, 1, sizeof(ee), f); fclose(f); }
}

extern "C" uint8_t eeprom_read(uint32_t a, uint8_t* p, uint32_t n) {
	if (a + n > sizeof(ee)) { printf("[sim] !! eeprom_read ngoai vung a=%u n=%u\n", a, n); return 1; }
	memcpy(p, ee + a, n);
	return 0;
}

extern "C" uint8_t eeprom_write(uint32_t a, uint8_t* p, uint32_t n) {
	if (a + n > sizeof(ee)) { printf("[sim] !! eeprom_write ngoai vung a=%u n=%u\n", a, n); return 1; }
	memcpy(ee + a, p, n);
	sim_ee_writes++;
	ee_flush();
	return 0;
}

extern "C" uint8_t eeprom_erase(uint32_t a, uint32_t n) {
	if (a + n > sizeof(ee)) { return 1; }
	memset(ee + a, 0xFF, n);
	ee_flush();
	return 0;
}

/*****************************************************************************/
/*  Còi
 */
/*****************************************************************************/
long sim_buzzer_count = 0;
extern "C" void BUZZER_Init(void) {}
extern "C" void BUZZER_Disable(void) {}
extern "C" void BUZZER_PlayTones(const Tone_TypeDef*) { sim_buzzer_count++; }
extern "C" void BUZZER_Sleep(bool) {}

/*****************************************************************************/
/*  Bẫy lỗi nghiêm trọng
 */
/*****************************************************************************/
long sim_fatal_count = 0;
extern "C" void sys_dbg_fatal(const int8_t* s, uint8_t c) {
	sim_fatal_count++;
	printf("[sim] !! FATAL(%s, 0x%02X)\n", (const char*)s, c);
}

/* AK cấp phát message động bằng cách so con trỏ với __heap_end__ */
extern "C" { uint8_t ak_host_heap[8192]; uint32_t __heap_end__; }

void sim_heap_init() {
	__heap_end__ = (uint32_t)(uintptr_t)(ak_host_heap + sizeof(ak_host_heap));
}

/*****************************************************************************/
/*  Mấy task không thuộc phần game, để trống cho bảng task link được
 */
/*****************************************************************************/
void task_fw(ak_msg_t*) {}
void task_shell(ak_msg_t*) {}
void task_life(ak_msg_t*) {}
void task_if(ak_msg_t*) {}
void task_uart_if(ak_msg_t*) {}
void task_link_phy(ak_msg_t*) {}
void task_link_mac(ak_msg_t*) {}
void task_link(ak_msg_t*) {}

/* Kernel gọi hàm này mỗi vòng của task_run(), sau khi hàng đợi message đã
 * được rút cạn. Bản giả lập bám vào đây làm nhịp: nhích đồng hồ, đọc bàn
 * phím, vẽ màn hình. Nhờ vậy không phải sửa một dòng nào trong ak/. */
void task_polling_console() { sim_poll(); }
