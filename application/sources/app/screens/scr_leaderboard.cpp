/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 13/05/2026
 *          End:   13/05/2026
 ******************************************************************************
**/
#include "scr_leaderboard.h"

static dungeon_game_score_t dungeon_score_board;

static void view_scr_leaderboard();

view_dynamic_t dyn_view_item_leaderboard = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_leaderboard
};

view_screen_t scr_leaderboard = {
	&dyn_view_item_leaderboard,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

static void view_scr_leaderboard() {
	view_render.fillScreen(BLACK);
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(SCR_CENTER_X(11), SCR_ROW_TITLE);
	view_render.print("LEADERBOARD");
	view_render.drawLine(SCR_PAD_L, SCR_ROW_RULE, SCR_PAD_R, SCR_ROW_RULE, WHITE);

	view_render.setCursor(SCR_PAD_L, SCR_ROW_BODY);
	view_render.print("Best progress");

	/* Cỡ chữ 2 cao 16 dòng: đặt ở 25 thì chiếm 25..40. */
	view_render.setTextSize(2);
	view_render.setCursor(SCR_CENTER_X_BIG(5), 25);
	view_render.print("L");
	view_render.print(dungeon_score_board.best_level);
	view_render.print(" S");
	view_render.print(dungeon_score_board.best_stage);

	view_render.setTextSize(1);
	view_render.setCursor(SCR_PAD_L, 43);
	view_render.print("Best score: ");
	view_render.print(dungeon_score_board.best_score);
	view_render.setCursor(SCR_CENTER_X(11), SCR_ROW_HINT);
	view_render.print("MODE = Back");
}

void scr_leaderboard_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		view_render.initialize();
		view_render_display_on();
		dungeon_score_read(&dungeon_score_board);
		if (dungeon_score_board.best_level > 5) {
			dungeon_score_board.best_level = 0;
			dungeon_score_board.best_stage = 0;
		}
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED:
	case AC_DISPLAY_BUTTON_UP_RELEASED:
	case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
		SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
		BUZZER_PlayTones(tones_cc);
	}
		break;

	default:
		break;
	}
}
