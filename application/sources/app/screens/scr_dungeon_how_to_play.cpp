/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 02/05/2026
 *          End:   02/05/2026
 ******************************************************************************
**/
#include "scr_dungeon_how_to_play.h"

static void view_scr_dungeon_how_to_play();

view_dynamic_t dyn_view_item_dungeon_how_to_play = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_dungeon_how_to_play
};

view_screen_t scr_dungeon_how_to_play = {
	&dyn_view_item_dungeon_how_to_play,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

void view_scr_dungeon_how_to_play() {
	view_render.setTextColor(WHITE);
	view_render.setTextSize(1);

	view_render.setCursor(SCR_CENTER_X(7), SCR_ROW_TITLE);
	view_render.print("DUNGEON");

	view_render.drawLine(SCR_PAD_L, SCR_ROW_RULE, SCR_PAD_R, SCR_ROW_RULE, WHITE);

	/* Bốn dòng nội dung cách nhau 9 px (chữ cao 8, chừa 1 px cho dễ đọc).
	 * Dòng cuối ở 42, kết ở 49, còn dư 3 px trước dòng gợi ý ở 53. */
	view_render.setCursor(SCR_PAD_L, SCR_ROW_BODY);
	view_render.print("UP   : MOVE SELECT");
	view_render.setCursor(SCR_PAD_L, SCR_ROW_BODY + 9);
	view_render.print("DOWN : MOVE SELECT");
	view_render.setCursor(SCR_PAD_L, SCR_ROW_BODY + 18);
	view_render.print("MODE : CONFIRM");

	view_render.setCursor(SCR_PAD_L, SCR_ROW_BODY + 27);
	if (dungeon_is_creator_mode()) {
		/* Chuỗi cũ "CREATOR TEST (NO SAVE)" dài 22 ký tự = 132 px, tràn khỏi
		 * màn 128 px. Tối đa là 20 ký tự khi bắt đầu ở SCR_PAD_L. */
		view_render.print("CREATOR TEST NO SAVE");
	}
	else {
		view_render.print("TRAVEL, BATTLE, LOOT");
	}
	view_render.setCursor(SCR_CENTER_X(13), SCR_ROW_HINT);
	view_render.print("MODE TO BEGIN");
}

void scr_dungeon_how_to_play_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		dungeon_game_state = GAME_OFF;
		view_render.initialize();
		view_render_display_on();
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_MODE_RELEASED\n");
		if ((dungeon_start_mode == DUNGEON_START_CONTINUE) && (dungeon_has_save_data() == 0)) {
			dungeon_prepare_new_game();
		}
		SCREEN_TRAN(scr_dungeon_game_handle, &scr_dungeon_game);
	}
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_UP_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_UP_RELEASED\n");
		SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
	}
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_DOWN_RELEASED\n");
		SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
	}
		BUZZER_PlayTones(tones_cc);
		break;

	default:
		break;
	}
}
