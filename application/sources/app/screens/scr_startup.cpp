/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 29/04/2026
 *          End:   30/04/2026
 ******************************************************************************
**/
#include "scr_startup.h"

/*****************************************************************************/
/* View - startup */
/*****************************************************************************/
static void view_scr_startup();

view_dynamic_t dyn_view_startup = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_startup
};

view_screen_t scr_startup = {
	&dyn_view_startup,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

/* Logo AK là chữ ASCII 4 dòng, mỗi dòng 14 ký tự.
 * Chữ cao 8 px nên 4 dòng phải cách nhau đúng 8 thì nét mới liền nhau.
 * Khối logo chiếm 10..41, dòng tên chiếm 45..52. Cả khối nằm giữa 3..60. */
#define AK_LOGO_AXIS_X		SCR_CENTER_X(14)
#define AK_LOGO_AXIS_Y		(10)
#define AK_LOGO_LINE_H		(SCR_CHAR_H)
#define AK_LOGO_TEXT_X		SCR_CENTER_X(13)
#define AK_LOGO_TEXT_Y		(45)

void view_scr_startup() {
	/* ak logo */
	BUZZER_PlayTones(tones_startup);
	view_render.clear();
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(AK_LOGO_AXIS_X, AK_LOGO_AXIS_Y);
	view_render.print("   __    _  _ ");
	view_render.setCursor(AK_LOGO_AXIS_X, AK_LOGO_AXIS_Y + AK_LOGO_LINE_H);
	view_render.print("  /__\\  ( )/ )");
	view_render.setCursor(AK_LOGO_AXIS_X, AK_LOGO_AXIS_Y + AK_LOGO_LINE_H * 2);
	view_render.print(" /(__)\\ (   (");
	view_render.setCursor(AK_LOGO_AXIS_X, AK_LOGO_AXIS_Y + AK_LOGO_LINE_H * 3);
	view_render.print("(__)(__)(_)\\_)");
	view_render.setCursor(AK_LOGO_TEXT_X, AK_LOGO_TEXT_Y);
	view_render.print("Active Kernel");
	view_render.update();
}

/*****************************************************************************/
/* Handle - startup */
/*****************************************************************************/
void scr_startup_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_DISPLAY_INITIAL: {
		APP_DBG_SIG("AC_DISPLAY_INITIAL\n");
		view_render.initialize();
		view_render_display_on();
		timer_set(	AC_TASK_DISPLAY_ID, \
					AC_DISPLAY_SHOW_LOGO, \
					AC_DISPLAY_STARTUP_INTERVAL, \
					TIMER_ONE_SHOT);
		// Read setting
		dungeon_setting_read(&settingdata);
		BUZZER_Sleep(settingdata.silent);
	}
		break;

	/* Bấm MODE là bỏ qua 2 giây chờ, nhảy thẳng sang màn mở màn của game.
	 * Nhớ huỷ timer AC_DISPLAY_SHOW_LOGO, không thì 2 giây sau nó vẫn nổ và
	 * lại đá màn hình về scr_title một lần nữa. */
	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_MODE_RELEASED\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_LOGO);
		SCREEN_TRAN(scr_title_handle, &scr_title);
	}
		break;

	case AC_DISPLAY_SHOW_LOGO: {
		APP_DBG_SIG("AC_DISPLAY_SHOW_LOGO\n");
		SCREEN_TRAN(scr_title_handle, &scr_title);
	}
		break;

	default:
		break;
	}
}
