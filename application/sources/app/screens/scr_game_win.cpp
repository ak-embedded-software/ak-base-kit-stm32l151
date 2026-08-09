#include "scr_game_win.h"
#include "screens.h"
#include "scr_game_playing.h"
#include "scr_game_menu.h"
#include "game_loneblade.h"

extern uint32_t game_get_high_score();


static uint8_t win_select = 0;
static bool win_blink = true;

static void view_scr_game_win();

view_dynamic_t dyn_view_game_win = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_game_win
};

view_screen_t scr_game_win = {
	&dyn_view_game_win,
	ITEM_NULL,
	ITEM_NULL,
	.focus_item = 0,
};

static void draw_sword(int y) {
	view_render.drawLine(10, y, 13, y, WHITE);
	view_render.drawLine(14, y - 2, 14, y + 2, WHITE);
	view_render.drawLine(15, y, 22, y, WHITE);
	view_render.drawPixel(23, y, WHITE);
}

static void view_scr_game_win() {
	view_render.clear();

	view_render.drawRect(0, 0, 128, 64, WHITE);
	view_render.drawRect(2, 2, 124, 60, WHITE);

	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);

	// Title
	view_render.setCursor(22, 5);
	view_render.print("*** YOU WIN! ***");

	view_render.drawLine(4, 15, 123, 15, WHITE);
	view_render.drawLine(4, 16, 123, 16, WHITE);

	view_render.setCursor(8, 20);
	view_render.print("SCORE: ");
	view_render.print(game_get_score());

	view_render.setCursor(8, 29);
	if (game_get_score() >= game_get_high_score()) {
		view_render.print("** NEW RECORD **");
	} else {
		view_render.print("BEST : ");
		view_render.print(game_get_high_score());
	}

	view_render.drawLine(4, 39, 123, 39, WHITE);

	// Option 1
	const char* opt1 = (g_difficulty == 2) ? "PLAY AGAIN" : "PLAY HARD";
	if (win_select == 0 && win_blink) {
		draw_sword(44);
	}
	view_render.setCursor(28, 41);
	view_render.print(opt1);

	// Option 2
	if (win_select == 1 && win_blink) {
		draw_sword(53);
	}
	view_render.setCursor(28, 50);
	view_render.print("BACK TO MENU");
}

void scr_game_win_handle(ak_msg_t *msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		win_select = 0;
		win_blink = true;
		timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_WIN_BLINK, 500, TIMER_PERIODIC);
		BUZZER_PlaySound(BUZZER_SOUND_SUPER_MARIO);
	} break;

	case SCREEN_EXIT: {
		timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_WIN_BLINK);
	} break;

	case AC_DISPLAY_GAME_WIN_BLINK: {
		win_blink = !win_blink;
	} break;

	case AC_DISPLAY_BUTON_UP_PRESSED:
	case AC_DISPLAY_BUTON_DOWN_PRESSED: {
		win_select = (win_select == 0) ? 1 : 0;
		BUZZER_PlaySound(BUZZER_SOUND_CLICK);
	} break;

	case AC_DISPLAY_BUTON_MODE_PRESSED: {
		if (win_select == 0) {
			if (g_difficulty != 2) {
				g_difficulty = 2;
			}
			extern void game_set_keep_score_flag(bool);
			game_set_keep_score_flag(true);
			BUZZER_PlaySound(BUZZER_SOUND_LETS_GO);
			SCREEN_TRAN(scr_game_playing_handle, &scr_game_playing);
		} else {
			BUZZER_PlaySound(BUZZER_SOUND_CLICK);
			SCREEN_TRAN(scr_game_menu_handle, &scr_game_menu);
		}
	} break;

	default:
		break;
	}
}
