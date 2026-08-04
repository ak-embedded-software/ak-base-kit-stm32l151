/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 01/05/2026
 *          End:   01/05/2026
 ******************************************************************************
**/
#include "scr_game_over.h"

/*****************************************************************************/
/* Variable Declaration - game over */
/*****************************************************************************/
static dungeon_game_score_t dungeon_score_board;
static uint32_t dungeon_score_now;

/*****************************************************************************/
/* View - game over */
/*****************************************************************************/
static void view_scr_game_over();

view_dynamic_t dyn_view_item_game_over = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_game_over
};

view_screen_t scr_game_over = {
	&dyn_view_item_game_over,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

void view_scr_game_over() {
	/* Hai dòng chữ cỡ 2 cao 16 px mỗi dòng: 5..20 và 23..38.
	 * Rồi hai dòng cỡ 1 ở 43 và 53, dòng cuối kết ở 60. */
	view_render.setTextSize(2);
	view_render.setTextColor(WHITE);
	if (dungeon_last_outcome == DUNGEON_OUTCOME_WIN) {
		view_render.setCursor(SCR_CENTER_X_BIG(5), 5);
		view_render.print("LEVEL");
		view_render.setCursor(SCR_CENTER_X_BIG(5), 23);
		view_render.print("CLEAR");
	}
	else {
		view_render.setCursor(SCR_CENTER_X_BIG(4), 5);
		view_render.print("GAME");
		view_render.setCursor(SCR_CENTER_X_BIG(4), 23);
		view_render.print("OVER");
	}

	view_render.setTextSize(1);
	view_render.drawLine(SCR_PAD_L, 41, SCR_PAD_R, 41, WHITE);
	view_render.setCursor(SCR_PAD_L, 44);
	view_render.print("Score:");
	view_render.print(dungeon_score_now);
	view_render.setCursor(SCR_PAD_L, SCR_ROW_HINT);
	view_render.print("Best :");
	view_render.print(dungeon_score_board.best_score);
}

/*****************************************************************************/
/* Handle - game over */
/*****************************************************************************/
void rank_ranking() {
	if (dungeon_is_creator_mode()) {
		return;
	}

	if (dungeon_score_now > dungeon_score_board.best_score) {
		dungeon_score_board.best_score = dungeon_score_now;
		dungeon_score_board.best_level = dungeon_selected_level;
		dungeon_score_board.best_stage = dungeon_get_total_stages();
	}
}

void scr_game_over_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		view_render.initialize();
		view_render_display_on();
		dungeon_score_read(&dungeon_score_board);
		dungeon_last_score_read(&dungeon_score_now);
		if (dungeon_is_creator_mode()) {
			dungeon_score_now = 0;
		}
		rank_ranking();
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_MODE_RELEASED\n");
		dungeon_score_write(&dungeon_score_board);
		SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
	}
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_UP_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_UP_RELEASED\n");
		dungeon_score_write(&dungeon_score_board);
		SCREEN_TRAN(scr_charts_game_handle, &scr_charts_game);
	}
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_DOWN_RELEASED\n");
		dungeon_score_write(&dungeon_score_board);
		dungeon_prepare_level(dungeon_selected_level);
		SCREEN_TRAN(scr_dungeon_how_to_play_handle, &scr_dungeon_how_to_play );
	}	
		BUZZER_PlayTones(tones_cc);
		break;

	default:
		break;
	}
}