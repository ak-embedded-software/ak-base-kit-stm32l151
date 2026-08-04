/**
 ******************************************************************************
 * @brief:  Cầu nối giữa firmware thật và bản giả lập.
 *
 *  Chỗ này làm ba việc:
 *    1. Dựng kernel AK y như main_app() làm trên bo.
 *    2. Định tuyến nút bấm giống hệt app_bsp.cpp.
 *    3. Đưa framebuffer với dòng trạng thái ra cho phần vẽ.
 ******************************************************************************
**/
#include <stdio.h>
#include <string.h>

#include "sim.h"

extern "C" {
#include "ak.h"
#include "task.h"
#include "message.h"
#include "timer.h"
}
#include "app.h"
#include "task_list.h"
#include "screen_manager.h"
#include "screens.h"
#include "task_display.h"
#include "view_render.h"
#include "app_eeprom.h"
#include "dungeon_runtime.h"

scr_mng_t scr_mng_app;
void task_display(ak_msg_t* msg) { scr_mng_dispatch(msg); }

/*****************************************************************************/
/*  Khởi động
 */
/*****************************************************************************/
void sim_boot() {
	sim_heap_init();

	/* Đúng thứ tự của main_app(): kernel trước, mọi thứ khác sau. */
	task_init();
	task_create((task_t*)app_task_table);
	task_polling_create((task_polling_t*)app_task_polling_table);

	/* app_task_init() */
	SCREEN_CTOR(&scr_mng_app, scr_startup_handle, &scr_startup);

	/* app_start_timer(): màn hình khởi động sau 100 ms */
	timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_INITIAL, AC_DISPLAY_INITIAL_INTERVAL, TIMER_ONE_SHOT);
}

/*****************************************************************************/
/*  Một nhịp 1 ms, đúng như ngắt SysTick trên bo
 *
 *  Trên bo, SysTick chỉ tăng biến mili giây rồi gọi timer_tick(1); phần điều
 *  phối message là do vòng lặp của task_run() lo. Ở đây giữ nguyên vai trò đó:
 *  hàm này chỉ nhích thời gian, không tự gọi scheduler.
 */
/*****************************************************************************/
void sim_tick_1ms() {
	sim_ms++;
	timer_tick(1);
}

/*****************************************************************************/
/*  Nút bấm
 *
 *  Chép nguyên nhánh rẽ của app_bsp.cpp: lúc đang chơi thì nút bay thẳng sang
 *  task game, còn lại mới đi qua task display. Sai chỗ này là bản giả lập
 *  chạy khác bo thật ngay.
 */
/*****************************************************************************/
void sim_press(int button) {
	switch (button) {
	case SIM_BTN_MODE:
		if (dungeon_game_state == GAME_PLAY) {
			task_post_pure_msg(DUNGEON_ACTION_ID, DUNGEON_ACTION_SHOOT);
		}
		else {
			task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_MODE_RELEASED);
		}
		break;

	case SIM_BTN_UP:
		if (dungeon_game_state == GAME_PLAY) {
			task_post_pure_msg(DUNGEON_CONTROL_ID, DUNGEON_CONTROL_UP);
		}
		else {
			task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_UP_RELEASED);
		}
		break;

	case SIM_BTN_DOWN:
		if (dungeon_game_state == GAME_PLAY) {
			task_post_pure_msg(DUNGEON_CONTROL_ID, DUNGEON_CONTROL_DOWN);
		}
		else {
			task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_DOWN_RELEASED);
		}
		break;

	/* Bấm giữ luôn đi qua task display, kể cả khi đang chơi. */
	case SIM_BTN_MODE_LONG:
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_MODE_LONG_PRESSED);
		break;
	case SIM_BTN_UP_LONG:
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_UP_LONG_PRESSED);
		break;
	case SIM_BTN_DOWN_LONG:
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTTON_DOWN_LONG_PRESSED);
		break;

	/* Nút Reset trên bo cắt nguồn MCU. Ở đây dựng lại kernel từ đầu, EEPROM
	 * giữ nguyên - đúng bằng việc rút điện rồi cắm lại. */
	case SIM_BTN_RESET:
		printf("[sim] RESET - dung lai kernel, EEPROM giu nguyen\n");
		sim_boot();
		break;

	default:
		break;
	}
}

/*****************************************************************************/
/*  Đưa ra cho phần vẽ
 */
/*****************************************************************************/
const uint8_t* sim_framebuffer() {
	return view_render.fb;
}

const char* sim_status_line() {
	static char buf[160];
	static const char* view_name[] = { "TRAVEL", "CHEST", "MSG", "BATTLE" };
	static const char* state_name[] = { "OFF", "PLAY", "OVER" };

	snprintf(buf, sizeof(buf),
	         "%us  state=%s  view=%s  L%u S%u/%u  hp=%d/%d  mob=%d  score=%lu  ee=%ld  buzz=%ld  fatal=%ld",
	         (unsigned)(sim_ms / 1000),
	         state_name[(dungeon_game_state <= 2) ? dungeon_game_state : 0],
	         view_name[dungeon_runtime.current_view & 3],
	         (unsigned)dungeon_runtime.level,
	         (unsigned)dungeon_runtime.stage,
	         (unsigned)dungeon_runtime.total_stages,
	         (int)dungeon_runtime.player_hp, (int)dungeon_runtime.player_max_hp,
	         (int)dungeon_runtime.monster_hp,
	         (unsigned long)dungeon_get_score_value(),
	         sim_ee_writes, sim_buzzer_count, sim_fatal_count);
	return buf;
}
