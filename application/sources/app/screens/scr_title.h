/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 29/07/2026
 *          End:   29/07/2026
 *
 * @brief:  Màn mở màn của game.
 *
 *  Luồng bật máy: scr_startup (logo AK, 2 giây) -> scr_title (đứng chờ)
 *  -> bấm MODE -> scr_menu_game.
 *
 *  Màn AK là của kernel, nó nói "con này chạy AK". Màn này là của game, nó
 *  nói "con này là Dungeon". Tách hai cái ra cho rõ, chứ nhét chung một màn
 *  thì chả biết đang khoe kernel hay khoe game.
 ******************************************************************************
**/
#ifndef __SCR_TITLE_H__
#define __SCR_TITLE_H__

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

#include "screens.h"
#include "screens_bitmap.h"

extern view_dynamic_t dyn_view_item_title;
extern view_screen_t scr_title;
extern void scr_title_handle(ak_msg_t* msg);

#endif //__SCR_TITLE_H__
