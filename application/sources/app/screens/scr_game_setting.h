/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 30/04/2026
 *          End:   01/05/2026
 ******************************************************************************
**/
#ifndef __SCR_GAME_SETTING_H__
#define __SCR_GAME_SETTING_H__

#include "fsm.h"
#include "port.h"
#include "message.h"
#include "timer.h"

#include "sys_ctrl.h"
#include "sys_dbg.h"

#include "app.h"
#include "app_dbg.h"
#include "task_list.h"
#include "task_display.h"
#include "view_render.h"

#include "buzzer.h"

#include "eeprom.h"
#include "app_eeprom.h"

#include "screens.h"
#include "screens_bitmap.h"

// chosse
#define STEP_SETTING_CHOSSE 					(15)
// items
#define SETTING_ITEM_ARRDESS_0					(0)
#define SETTING_ITEM_ARRDESS_1					(STEP_SETTING_CHOSSE)
#define SETTING_ITEM_ARRDESS_2					(STEP_SETTING_CHOSSE*2)
#define SETTING_ITEM_ARRDESS_3					(STEP_SETTING_CHOSSE*3)
#define SETTING_ITEM_ARRDESS_4					(STEP_SETTING_CHOSSE*4)
/* Bố cục màn Setting bám theo lề chung 3 px (xem screens_layout.h).
 * Vùng vẽ là dòng 3..60, đúng 58 dòng. Xếp 4 hàng:
 *   4 hàng cao 13 + 3 khe 2 px = 13*4 + 2*3 = 58. Vừa khít.
 * Hàng nằm ở 3..15, 18..30, 33..45, 48..60. Bước nhảy 15. */
#define DUNGEON_SETTING_ROW_COUNT				(4)
#define DUNGEON_SETTING_FRAMES_AXIS_Y_1			SCR_PAD_T		/* 3 */
#define DUNGEON_SETTING_FRAMES_STEP 			(15)
#define DUNGEON_SETTING_FRAMES_SIZE_H			(13)
#define DUNGEON_SETTING_FRAMES_SIZE_R			(3)

/* Con trỏ chọn: icon chosse_icon cũ là 20x20, cao hơn cả một hàng (13 px)
 * nên nó luôn đè sang hàng bên cạnh, nhìn không biết đang chọn hàng nào.
 * Thay bằng một tam giác nhỏ 6x7 vẽ bằng fillTriangle, canh giữa hàng.
 *   con trỏ 3..8, khung 13..124 (đúng SCR_PAD_R). */
#define DUNGEON_SETTING_CURSOR_AXIS_X			SCR_PAD_L		/* 3 */
#define DUNGEON_SETTING_CURSOR_W				(6)
#define DUNGEON_SETTING_CURSOR_H				(7)

#define DUNGEON_SETTING_FRAMES_AXIS_X			(13)
#define DUNGEON_SETTING_FRAMES_SIZE_W			(SCR_PAD_R - DUNGEON_SETTING_FRAMES_AXIS_X + 1)	/* 112 */

/* Chữ cao 8, hàng cao 13 -> lùi xuống 3 để canh giữa (3..10 trong 0..12). */
#define DUNGEON_SETTING_TEXT_AXIS_X 			(17)
#define DUNGEON_SETTING_TEXT_OFFSET_Y			(3)
/* "(x)" là 3 ký tự = 18 px, đặt ở 106 thì kết ở 123, vẫn trong khung. */
#define DUNGEON_SETTING_NUMBER_AXIS_X			(106)
#define DUNGEON_SETTING_SPEAKER_AXIS_X			(112)
#define DUNGEON_SETTING_SPEAKER_SIZE			(7)


extern view_dynamic_t dyn_view_item_game_setting;
extern view_screen_t scr_game_setting;
extern void scr_game_setting_handle(ak_msg_t* msg);

#endif //__SCR_GAME_SETTING_H__
