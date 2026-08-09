#include "scr_welcome.h"

#define WELCOME_TEXT_LINE_1_LEN		(8)
#define WELCOME_TEXT_LINE_2_LEN		(5)
#define WELCOME_TEXT_TOTAL_LEN		(WELCOME_TEXT_LINE_1_LEN + WELCOME_TEXT_LINE_2_LEN)

static uint8_t welcome_text_index = 0;
static const char* welcome_text_line_1 = "Welcome";
static const char* welcome_text_line_2 = "to AK";

static void view_scr_welcome();

static void welcome_print_text_partial(const char* text, uint8_t max_chars) {
	if (text == (const char*)0) return;

	for (uint8_t i = 0; i < max_chars && text[i] != '\0'; i++) {
		view_render.print(text[i]);
	}
}

view_dynamic_t dyn_view_welcome = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_welcome
};

view_screen_t scr_welcome = {
	&dyn_view_welcome,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

void view_scr_welcome() {
	view_render.clear();
	view_render.drawBitmap(	0, \
		0, \
		bitmap_dolphin, \
		119, \
		62, \
		WHITE);

	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(76, 12);
	welcome_print_text_partial(welcome_text_line_1, welcome_text_index);
	view_render.setCursor(84, 25);
	if (welcome_text_index > WELCOME_TEXT_LINE_1_LEN) {
		welcome_print_text_partial(welcome_text_line_2, welcome_text_index - WELCOME_TEXT_LINE_1_LEN);
	}
}

void scr_welcome_handle(ak_msg_t *msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		welcome_text_index = 0;
		BUZZER_PlaySound(BUZZER_SOUND_WELCOME);
		timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_WELCOME_TEXT_ANIM_TICK, AC_DISPLAY_WELCOME_TEXT_ANIM_TICK_INTERVAL, TIMER_PERIODIC);
		timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_IDLE, AC_DISPLAY_IDLE_INTERVAL, TIMER_ONE_SHOT);
	} break;

	case AC_DISPLAY_WELCOME_TEXT_ANIM_TICK: {
		APP_DBG_SIG("AC_DISPLAY_WELCOME_TEXT_ANIM_TICK\n");
		if (welcome_text_index < WELCOME_TEXT_TOTAL_LEN) {
			welcome_text_index++;
		}
		else {
			timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_WELCOME_TEXT_ANIM_TICK);
		}
	} break;

	case AC_DISPLAY_BUTON_MODE_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_MODE_PRESSED\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_WELCOME_TEXT_ANIM_TICK);
		SCREEN_TRAN(scr_idle_handle, &scr_idle);
	} break;

	case AC_DISPLAY_SHOW_IDLE: {
		APP_DBG_SIG("AC_DISPLAY_SHOW_IDLE\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_WELCOME_TEXT_ANIM_TICK);
		SCREEN_TRAN(scr_idle_handle, &scr_idle);
	} break;

	case AC_DISPLAY_BUTON_UP_PRESSED:
	case AC_DISPLAY_BUTON_DOWN_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_%s_PRESSED\n", msg->sig == AC_DISPLAY_BUTON_UP_PRESSED ? "UP" : "DOWN");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_WELCOME_TEXT_ANIM_TICK);
		SCREEN_TRAN(scr_game_menu_handle, &scr_game_menu);
	} break;

	default:
		break;
	}
}
