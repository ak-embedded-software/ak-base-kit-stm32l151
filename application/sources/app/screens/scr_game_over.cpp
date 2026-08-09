#include "screens.h"
#include "scr_game_over.h"
#include "scr_game_playing.h"
#include "game_loneblade.h"

extern uint32_t game_get_high_score();


static bool blink_show = true;

static void view_scr_game_over();

view_dynamic_t dyn_view_game_over = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_game_over
};

view_screen_t scr_game_over = {
	&dyn_view_game_over,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};


static void view_scr_game_over() {
	view_render.clear();

	view_render.drawRect(0, 0, 128, 64, WHITE);
	view_render.drawRect(2, 2, 124, 60, WHITE);

	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);

	view_render.drawLine(4, 14, 123, 14, WHITE);
	view_render.drawLine(4, 15, 123, 15, WHITE);

	view_render.setCursor(25, 5);
	view_render.print("*** GAME OVER ***");

	view_render.setCursor(12, 19);
	view_render.print("WAVE  : ");
	view_render.print(game_get_wave());

	view_render.setCursor(12, 28);
	view_render.print("SCORE : ");
	view_render.print(game_get_score());

	view_render.setCursor(12, 37);
	view_render.print("BEST  : ");
	view_render.print(game_get_high_score());

	view_render.drawLine(4, 47, 123, 47, WHITE);

	if (blink_show) {
		view_render.setCursor(10, 51);
		view_render.print("[MODE] Back to Menu");
	}
}

void scr_game_over_handle(ak_msg_t *msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY scr_game_over\n");
		blink_show = true;
		timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_OVER_BLINK, 500, TIMER_PERIODIC);
	} break;

	case SCREEN_EXIT: {
		APP_DBG_SIG("SCREEN_EXIT scr_game_over\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_OVER_BLINK);
	} break;

	case AC_DISPLAY_GAME_OVER_BLINK: {
		blink_show = !blink_show;
	} break;

	case AC_DISPLAY_BUTON_MODE_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_MODE_PRESSED scr_game_over -> Back to Menu\n");
		BUZZER_PlaySound(BUZZER_SOUND_CLICK);
		SCREEN_TRAN(scr_game_menu_handle, &scr_game_menu);
	} break;

	case AC_DISPLAY_BUTON_UP_PRESSED:
	case AC_DISPLAY_BUTON_DOWN_PRESSED: {
		APP_DBG_SIG("RETRY pressed in scr_game_over\n");
		BUZZER_PlaySound(BUZZER_SOUND_LETS_GO);
		SCREEN_TRAN(scr_game_playing_handle, &scr_game_playing);
	} break;

	default:
		break;
	}
}
