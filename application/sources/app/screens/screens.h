#ifndef __SCREENS_H__
#define __SCREENS_H__

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

#include <math.h>
#include <vector>

#include "qrcode.h"
#include "screens_bitmap.h"
#include "scr_game_playing.h"
// scr_idle
extern view_dynamic_t dyn_view_idle;
extern view_screen_t scr_idle;
extern void scr_idle_handle(ak_msg_t* msg);

// scr_qrcode
extern view_dynamic_t dyn_view_qrcode;
extern view_screen_t scr_qrcode;
extern void scr_qrcode_handle(ak_msg_t* msg);

// scr_startup
extern view_dynamic_t dyn_view_startup;
extern view_screen_t scr_startup;
extern void scr_startup_handle(ak_msg_t* msg);

// scr_welcome
extern view_dynamic_t dyn_view_welcome;
extern view_screen_t scr_welcome;
extern void scr_welcome_handle(ak_msg_t* msg);

// scr_game_menu
extern view_dynamic_t dyn_view_game_menu;
extern view_screen_t scr_game_menu;
extern void scr_game_menu_handle(ak_msg_t* msg);

// scr_game_playing
extern view_dynamic_t dyn_view_game_playing;
extern view_screen_t scr_game_playing;
extern void scr_game_playing_handle(ak_msg_t* msg);

// scr_game_over
extern view_dynamic_t dyn_view_game_over;
extern view_screen_t scr_game_over;
extern void scr_game_over_handle(ak_msg_t* msg);

// scr_game_settings
extern view_dynamic_t dyn_view_game_settings;
extern view_screen_t scr_game_settings;
extern void scr_game_settings_handle(ak_msg_t* msg);

// scr_game_win
extern view_dynamic_t dyn_view_game_win;
extern view_screen_t scr_game_win;
extern void scr_game_win_handle(ak_msg_t* msg);

// global settings variables
extern uint8_t g_sound_enabled;
extern uint8_t g_difficulty;

#endif //__SCREENS_H__
