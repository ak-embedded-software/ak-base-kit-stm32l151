#include "scr_game_menu.h"
#include "screens_bitmap.h"





extern view_screen_t scr_game_settings;
extern void scr_game_settings_handle(ak_msg_t* msg);

extern view_screen_t scr_game_leaderboard;
extern void scr_game_leaderboard_handle(ak_msg_t* msg);

static uint8_t menu_select = 0; // 0: PLAY GAME, 1: LEADERBOARD, 2: SETTINGS, 3: EXIT

static void view_scr_game_menu();
static void draw_selector_sword(int y);

view_dynamic_t dyn_view_game_menu = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_game_menu
};

view_screen_t scr_game_menu = {
	&dyn_view_game_menu,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

static void draw_selector_sword(int y) {
	view_render.drawLine(10, y, 13, y, WHITE);   // Handle 
	view_render.drawLine(14, y - 2, 14, y + 2, WHITE); // Crossguard
	view_render.drawLine(15, y, 22, y, WHITE);   // Blade
	view_render.drawPixel(23, y, WHITE);         // Tip
}

static void view_scr_game_menu() {
	view_render.clear();
	

	view_render.drawRect(0, 0, 124, 60, WHITE);
	view_render.drawRect(2, 2, 120, 56, WHITE);
		
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(32, 7);
	view_render.print("LONE-BLADE");
		
	view_render.drawLine(20, 17, 104, 17, WHITE);
		
	view_render.drawLine(24, 12, 28, 8, WHITE);
	view_render.drawPixel(23, 13, WHITE);
	view_render.drawPixel(22, 14, WHITE);
	
	view_render.drawLine(99, 12, 95, 8, WHITE);
	view_render.drawPixel(100, 13, WHITE);
	view_render.drawPixel(101, 14, WHITE);
		
	// Choice 1: PLAY GAME
	view_render.setCursor(30, 22);
	view_render.print("PLAY GAME");

	// Choice 2: LEADERBOARD
	view_render.setCursor(30, 31);
	view_render.print("LEADERBOARD");

	// Choice 3: SETTINGS
	view_render.setCursor(30, 40);
	view_render.print("SETTINGS");

	// Choice 4: EXIT
	view_render.setCursor(30, 49);
	view_render.print("EXIT");

	if (menu_select == 0) {
		draw_selector_sword(25);
	} else if (menu_select == 1) {
		draw_selector_sword(34);
	} else if (menu_select == 2) {
		draw_selector_sword(43);
	} else {
		draw_selector_sword(52);
	}

}

// Handle game menu interactions

void scr_game_menu_handle(ak_msg_t *msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY scr_game_menu\n");
		menu_select = 0;
		BUZZER_PlaySound(BUZZER_SOUND_WELCOME);
	} break;

	case AC_DISPLAY_BUTON_UP_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_UP_PRESSED in Lone-Blade Menu\n");
			if (menu_select > 0) {
				menu_select--;
			} else {
				menu_select = 3; 
			}
			BUZZER_PlaySound(BUZZER_SOUND_CLICK);
	} break;

	case AC_DISPLAY_BUTON_DOWN_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_DOWN_PRESSED in Lone-Blade Menu\n");
			if (menu_select < 3) {
				menu_select++;
			} else {
				menu_select = 0;
			}
			BUZZER_PlaySound(BUZZER_SOUND_CLICK);
	} break;

	case AC_DISPLAY_BUTON_MODE_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_MODE_PRESSED in Lone-Blade Menu\n");
		if (menu_select == 0) {
			BUZZER_PlaySound(BUZZER_SOUND_LETS_GO);
			SCREEN_TRAN(scr_game_playing_handle, &scr_game_playing);
		} else if (menu_select == 1) {
			BUZZER_PlaySound(BUZZER_SOUND_CLICK);
			SCREEN_TRAN(scr_game_leaderboard_handle, &scr_game_leaderboard);
		} else if (menu_select == 2) {
			BUZZER_PlaySound(BUZZER_SOUND_CLICK);
			SCREEN_TRAN(scr_game_settings_handle, &scr_game_settings);
		} else {
			BUZZER_PlaySound(BUZZER_SOUND_GOODBYE);
			SCREEN_BACK();
		}
	} break;


	default:
		break;
	}
}
