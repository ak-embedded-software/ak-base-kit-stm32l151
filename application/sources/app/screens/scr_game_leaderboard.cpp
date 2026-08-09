#include "scr_game_leaderboard.h"
#include "screens.h"
#include "scr_game_menu.h"
#include "game_loneblade.h"

extern uint32_t game_get_leaderboard_score(uint8_t index);

static void view_scr_game_leaderboard();

view_dynamic_t dyn_view_game_leaderboard = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_game_leaderboard
};

view_screen_t scr_game_leaderboard = {
	&dyn_view_game_leaderboard,
	ITEM_NULL,
	ITEM_NULL,
	.focus_item = 0,
};

static void view_scr_game_leaderboard() {
	view_render.clear();

	// Draw outer & inner border
	view_render.drawRect(0, 0, 128, 64, WHITE);
	view_render.drawRect(2, 2, 124, 60, WHITE);

	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);

	// Title
	view_render.setCursor(28, 5);
	view_render.print("LEADERBOARD");
	view_render.drawLine(4, 14, 123, 14, WHITE);

	// Draw Top 5 Leaderboard slots
	for (int i = 0; i < 5; i++) {
		int y_pos = 17 + i * 8;
		view_render.setCursor(8, y_pos);
		view_render.print("#");
		view_render.print(i + 1);
		view_render.print(" : ");
		view_render.print(game_get_leaderboard_score(i));
	}

	view_render.drawLine(4, 57, 123, 57, WHITE);
}

void scr_game_leaderboard_handle(ak_msg_t *msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY scr_game_leaderboard\n");
	} break;

	case AC_DISPLAY_BUTON_MODE_PRESSED:
	case AC_DISPLAY_BUTON_UP_PRESSED:
	case AC_DISPLAY_BUTON_DOWN_PRESSED: {
		BUZZER_PlaySound(BUZZER_SOUND_CLICK);
		SCREEN_TRAN(scr_game_menu_handle, &scr_game_menu);
	} break;

	default:
		break;
	}
}
