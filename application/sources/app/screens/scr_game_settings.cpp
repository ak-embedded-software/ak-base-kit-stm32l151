#include "scr_game_settings.h"
#include "screens_bitmap.h"

static uint8_t settings_select = 0; // 0: SOUND, 1: DIFFICULTY, 2: BACK

static void view_scr_game_settings();
static void draw_settings_selector_sword(int y);

view_dynamic_t dyn_view_game_settings = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_game_settings
};

view_screen_t scr_game_settings = {
	&dyn_view_game_settings,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

static void draw_settings_selector_sword(int y) {
	view_render.drawLine(8, y, 11, y, WHITE);   // Handle
	view_render.drawLine(12, y - 2, 12, y + 2, WHITE); // Crossguard
	view_render.drawLine(13, y, 20, y, WHITE);   // Blade
	view_render.drawPixel(21, y, WHITE);         // Tip
}

static void view_scr_game_settings() {
	view_render.clear();

	view_render.drawRect(0, 0, 124, 60, WHITE);
	view_render.drawRect(2, 2, 120, 56, WHITE);

	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(40, 6);
	view_render.print("SETTINGS");

	view_render.drawLine(20, 15, 104, 15, WHITE);

	view_render.setCursor(26, 21);
	view_render.print("SOUND: ");
	if (g_sound_enabled) {
		view_render.print("< ON >");
	} else {
		view_render.print("< OFF >");
	}

	view_render.setCursor(26, 33);
	view_render.print("DIFF: ");
	if (g_difficulty == 0) {
		view_render.print("< EASY >");
	} else if (g_difficulty == 1) {
		view_render.print("< NORMAL >");
	} else {
		view_render.print("< HARD >");
	}

	view_render.setCursor(26, 45);
	view_render.print("BACK");

	if (settings_select == 0) {
		draw_settings_selector_sword(24);
	} else if (settings_select == 1) {
		draw_settings_selector_sword(36);
	} else {
		draw_settings_selector_sword(48);
	}
}

void scr_game_settings_handle(ak_msg_t *msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY scr_game_settings\n");
		settings_select = 0;
	} break;

	case AC_DISPLAY_BUTON_UP_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_UP_PRESSED in scr_game_settings\n");
		if (settings_select > 0) {
			settings_select--;
		} else {
			settings_select = 2;
		}
		BUZZER_PlaySound(BUZZER_SOUND_CLICK);
	} break;

	case AC_DISPLAY_BUTON_DOWN_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_DOWN_PRESSED in scr_game_settings\n");
		if (settings_select < 2) {
			settings_select++;
		} else {
			settings_select = 0;
		}
		BUZZER_PlaySound(BUZZER_SOUND_CLICK);
	} break;

	case AC_DISPLAY_BUTON_MODE_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_MODE_PRESSED in scr_game_settings\n");
		if (settings_select == 0) {
			g_sound_enabled = !g_sound_enabled;
			BUZZER_Silent(g_sound_enabled);
			BUZZER_PlaySound(BUZZER_SOUND_CLICK);
		} else if (settings_select == 1) {
			g_difficulty = (g_difficulty + 1) % 3;
			BUZZER_PlaySound(BUZZER_SOUND_CLICK);
		} else {
			BUZZER_PlaySound(BUZZER_SOUND_CLICK);
			SCREEN_TRAN(scr_game_menu_handle, &scr_game_menu);
		}
	} break;

	default:
		break;
	}
}
