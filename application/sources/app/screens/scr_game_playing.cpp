#include "screens.h"
#include "scr_game_playing.h"
#include "scr_game_over.h"
#include "scr_game_win.h"
#include "game_loneblade.h"
#include "game_player.h"

extern void game_update_high_score();
extern uint32_t game_get_high_score();


static void view_scr_game_playing();

view_dynamic_t dyn_view_game_playing = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_game_playing
};

view_screen_t scr_game_playing = {
	&dyn_view_game_playing,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

static bool game_over_pending = false;
static uint32_t game_over_timer_ms = 0;
static bool game_win_pending = false;
static uint32_t game_win_timer_ms = 0;

static void view_scr_game_playing() {
	view_render.clear();
	game_loneblade_draw();
}

void scr_game_playing_handle(ak_msg_t *msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY scr_game_playing\n");
		game_loneblade_init();
		game_over_pending = false;
		game_over_timer_ms = 0;
		game_win_pending = false;
		game_win_timer_ms = 0;

		timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK, 33, TIMER_PERIODIC);
	} break;

	case SCREEN_EXIT: {
		APP_DBG_SIG("SCREEN_EXIT scr_game_playing\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK);
		game_over_pending = false;
		game_win_pending = false;
	} break;

	case AC_DISPLAY_GAME_TICK: {
		game_loneblade_update(33);

		if (game_over_pending) {
			game_over_timer_ms += 33;
			if (game_over_timer_ms >= 700) {
				SCREEN_TRAN(scr_game_over_handle, &scr_game_over);
			}
		}

		if (game_win_pending) {
			game_win_timer_ms += 33;
			if (game_win_timer_ms >= 1200) {
				SCREEN_TRAN(scr_game_win_handle, &scr_game_win);
			}
		}
	} break;

	case AC_DISPLAY_GAME_OVER: {
		if (!game_over_pending && !game_win_pending) {
			APP_DBG_SIG("AC_DISPLAY_GAME_OVER received\n");
			if (game_get_score() >= game_get_high_score() && game_get_score() > 0) {
				BUZZER_PlaySound(BUZZER_SOUND_HIGHSCORE);
			} else {
				BUZZER_PlaySound(BUZZER_SOUND_LOWSCORE);
			}
			game_update_high_score();
			game_over_pending  = true;
			game_over_timer_ms = 0;
		}
	} break;

	case AC_DISPLAY_GAME_WIN: {
		if (!game_win_pending && !game_over_pending) {
			APP_DBG_SIG("AC_DISPLAY_GAME_WIN received\n");
			game_update_high_score();
			game_win_pending  = true;
			game_win_timer_ms = 0;
		}
	} break;

	case AC_DISPLAY_BUTON_DOWN_PRESSED: {
		if (!game_over_pending && !game_win_pending) {
			APP_DBG_SIG("AC_DISPLAY_BUTON_DOWN_PRESSED: Attack Left\n");
			player_attack(DIR_LEFT);
		}
	} break;

	case AC_DISPLAY_BUTON_UP_PRESSED: {
		if (!game_over_pending && !game_win_pending) {
			APP_DBG_SIG("AC_DISPLAY_BUTON_UP_PRESSED: Attack Right\n");
			player_attack(DIR_RIGHT);
		}
	} break;

	case AC_DISPLAY_BUTON_MODE_PRESSED: {
		if (!game_over_pending && !game_win_pending) {
			APP_DBG_SIG("AC_DISPLAY_BUTON_MODE_PRESSED scr_game_playing -> Trigger Shield\n");
			BUZZER_PlaySound(BUZZER_SOUND_TONE_3);
			player_shield(true);
		}
	} break;

	default:
		break;
	}
}
