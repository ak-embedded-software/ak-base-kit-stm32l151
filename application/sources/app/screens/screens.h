/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 28/04/2026
 *          End:   28/04/2026
 ******************************************************************************
**/
#ifndef __SCREENS_H__
#define __SCREENS_H__

#include "screens_layout.h"
#include "scr_dungeon_game.h"
#include "scr_charts_game.h"
#include "scr_game_over.h"
#include "scr_game_setting.h"
#include "scr_leaderboard.h"
#include "scr_idle.h"
#include "scr_dungeon_how_to_play.h"
#include "scr_menu_game.h"
#include "scr_startup.h"
#include "scr_title.h"

// scr_dungeon_game
extern view_dynamic_t dyn_view_item_dungeon_game;
extern view_screen_t scr_dungeon_game;
extern void scr_dungeon_game_handle(ak_msg_t* msg);

// scr_charts_game
extern view_dynamic_t dyn_view_item_charts_game;
extern view_screen_t scr_charts_game;
extern void scr_charts_game_handle(ak_msg_t* msg);

// scr_game_over
extern view_dynamic_t dyn_view_item_game_over;
extern view_screen_t scr_game_over;
extern void scr_game_over_handle(ak_msg_t* msg);

// scr_game_setting
extern view_dynamic_t dyn_view_item_game_setting;
extern view_screen_t scr_game_setting;
extern void scr_game_setting_handle(ak_msg_t* msg);

// scr_leaderboard
extern view_dynamic_t dyn_view_item_leaderboard;
extern view_screen_t scr_leaderboard;
extern void scr_leaderboard_handle(ak_msg_t* msg);

// scr_dungeon_how_to_play
extern view_dynamic_t dyn_view_item_dungeon_how_to_play;
extern view_screen_t scr_dungeon_how_to_play;
extern void scr_dungeon_how_to_play_handle(ak_msg_t* msg);

// scr_idle
extern view_dynamic_t dyn_view_idle;
extern view_screen_t scr_idle;
extern void scr_idle_handle(ak_msg_t* msg);

// scr_menu_game
extern view_dynamic_t dyn_view_item_menu_game;
extern view_screen_t scr_menu_game;
extern void scr_menu_game_handle(ak_msg_t* msg);

// scr_startup
extern view_dynamic_t dyn_view_startup;
extern view_screen_t scr_startup;
extern void scr_startup_handle(ak_msg_t* msg);

#endif //__SCREENS_H__