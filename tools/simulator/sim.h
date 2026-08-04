/**
 ******************************************************************************
 * @brief:  Khai báo dùng chung giữa mấy file của bản giả lập.
 ******************************************************************************
**/
#ifndef __SIM_H__
#define __SIM_H__

#include <stdint.h>

/* đồng hồ giả, vòng lặp chính tăng mỗi 1 ms */
extern uint32_t sim_ms;

/* đếm để in ra thanh trạng thái */
extern long sim_ee_writes;
extern long sim_buzzer_count;
extern long sim_fatal_count;

void sim_eeprom_open(const char* path);
void sim_eeprom_wipe();
void sim_heap_init();

/* ba nút của bo, thêm reset mềm */
enum {
	SIM_BTN_UP = 0,
	SIM_BTN_DOWN,
	SIM_BTN_MODE,
	SIM_BTN_UP_LONG,
	SIM_BTN_DOWN_LONG,
	SIM_BTN_MODE_LONG,
	SIM_BTN_RESET,
};

/* Đẩy một lần bấm vào firmware. Định tuyến y hệt app_bsp.cpp trên bo thật. */
void sim_press(int button);

/* con trỏ tới framebuffer 128x64, mỗi byte là 1 pixel (0 hoặc 1) */
const uint8_t* sim_framebuffer();

/* dòng mô tả trạng thái game để in ra thanh dưới */
const char* sim_status_line();

/* Được gọi mỗi vòng lặp của task_run(), qua polling task của kernel.
 * Đây là chỗ bản giả lập nhích đồng hồ, đọc bàn phím và vẽ màn hình. */
void sim_poll();

#endif /* __SIM_H__ */
