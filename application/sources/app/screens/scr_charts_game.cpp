/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 01/05/2026
 *          End:   02/05/2026
 ******************************************************************************
**/
#include "scr_charts_game.h"

static const char* dungeon_level_mode_name[5] = {
	"Easy",
	"Easy",
	"Medium",
	"Medium",
	"Hard",
};

static const uint8_t dungeon_level_stage_count[5] = {4, 5, 6, 7, 8};
static uint8_t dungeon_level_view_mode = DUNGEON_LEVEL_VIEW_INFO;

void dungeon_set_level_view_mode(uint8_t mode) {
	if (mode == DUNGEON_LEVEL_VIEW_CREATOR) {
		dungeon_level_view_mode = DUNGEON_LEVEL_VIEW_CREATOR;
	}
	else {
		dungeon_level_view_mode = DUNGEON_LEVEL_VIEW_INFO;
	}
}

/*****************************************************************************/
/* View - Charts game */
/*****************************************************************************/
static void view_scr_charts_game();

view_dynamic_t dyn_view_item_charts_game = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_charts_game
};

view_screen_t scr_charts_game = {
	&dyn_view_item_charts_game,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

void view_scr_charts_game() {
	view_render.fillScreen(BLACK);
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	if (dungeon_level_view_mode == DUNGEON_LEVEL_VIEW_CREATOR) {
		view_render.setCursor(SCR_CENTER_X(7), SCR_ROW_TITLE);
		view_render.print("CREATOR");
	}
	else {
		view_render.setCursor(SCR_CENTER_X(10), SCR_ROW_TITLE);
		view_render.print("LEVEL INFO");
	}
	view_render.drawLine(SCR_PAD_L, SCR_ROW_RULE, SCR_PAD_R, SCR_ROW_RULE, WHITE);

	/* Cỡ chữ 2, chuỗi 2 ký tự "L3" -> 24 px, canh giữa. Chiếm dòng 16..31. */
	view_render.setTextSize(2);
	view_render.setCursor(SCR_CENTER_X_BIG(2), 16);
	view_render.print("L");
	view_render.print(dungeon_selected_level);

	/* Hai dòng mô tả canh trái giống màn LeaderBoard cho đồng bộ. */
	view_render.setTextSize(1);
	view_render.setCursor(SCR_PAD_L, 35);
	view_render.print("Mode  : ");
	view_render.print(dungeon_level_mode_name[dungeon_selected_level - 1]);
	view_render.setCursor(SCR_PAD_L, 44);
	view_render.print("Stages: ");
	view_render.print(dungeon_level_stage_count[dungeon_selected_level - 1]);

	if (dungeon_level_view_mode == DUNGEON_LEVEL_VIEW_CREATOR) {
		view_render.setCursor(SCR_CENTER_X(16), SCR_ROW_HINT);
		view_render.print("MODE: Start test");
	}
	else {
		view_render.setCursor(SCR_CENTER_X(10), SCR_ROW_HINT);
		view_render.print("MODE: Back");
	}
}

/*****************************************************************************/
/* Handle - Charts game */
/*****************************************************************************/
void scr_charts_game_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		view_render.initialize();
		view_render_display_on();
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_MODE_RELEASED\n");
		if (dungeon_level_view_mode == DUNGEON_LEVEL_VIEW_CREATOR) {
			dungeon_prepare_creator_mode(dungeon_selected_level);
			SCREEN_TRAN(scr_dungeon_how_to_play_handle, &scr_dungeon_how_to_play);
		}
		else {
			SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
		}
	}
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_UP_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_UP_RELEASED\n");
		if (dungeon_selected_level > 1) {
			dungeon_selected_level--;
		}
	}
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_DOWN_RELEASED\n");
		if (dungeon_selected_level < 5) {
			dungeon_selected_level++;
		}
	}	
		BUZZER_PlayTones(tones_cc);
		break;

	default:
		break;
	}
}