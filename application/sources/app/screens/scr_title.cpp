/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 29/07/2026
 *          End:   29/07/2026
 ******************************************************************************
**/
#include "scr_title.h"

/*****************************************************************************/
/* Bố cục - title
 *
 *  Vùng vẽ theo lề chung là dòng 3..60, đúng 58 dòng. Chia như sau:
 *
 *      3..18   tên game, chữ cỡ 2 (cao 16)
 *      20..49  dải sprite: hero bên trái, quái bên phải
 *      53..60  dòng "PRESS MODE", nhấp nháy
 *
 *  Cộng lại 16 + 1 + 30 + 3 + 8 = 58. Vừa khít, không dư dòng nào.
 *
 *  Khe hở dồn xuống dưới chứ không dồn lên trên. Đỉnh quái slime là cái râu
 *  mảnh nằm lệch về bên phải, sát chữ 1 px cũng không thấy chật. Còn chân
 *  hero mà sát chữ "PRESS MODE" thì nhìn ra ngay là bị dính.
 *
 *  Hai sprite được canh đáy theo nhau chứ không canh đỉnh. Hero cao 17 mà
 *  quái cao 30, canh đỉnh thì hero trông như đang bay lơ lửng.
 */
/*****************************************************************************/
#define TITLE_TEXT                  "DUNGEON"
#define TITLE_TEXT_LEN              (7)
#define TITLE_ROW                   SCR_PAD_T           /* 3, chiếm 3..18 */

#define TITLE_HINT_TEXT             "PRESS MODE"
#define TITLE_HINT_LEN              (10)

/* Quái slime: bitmap 31x30. Dòng ink cuối nằm ở hàng 28 của bitmap. */
#define TITLE_MONSTER_W             (31)
#define TITLE_MONSTER_H             (30)
#define TITLE_MONSTER_X             (71)                /* 71..101 */
#define TITLE_MONSTER_Y             (20)                /* 20..49  */
#define TITLE_MONSTER_INK_BOTTOM    (TITLE_MONSTER_Y + 28)

/* Hero: bitmap 24x17. Dòng ink cuối nằm ở hàng 15 của bitmap. */
#define TITLE_HERO_W                (24)
#define TITLE_HERO_H                (17)
#define TITLE_HERO_X                (24)                /* 24..47 */
#define TITLE_HERO_Y                (TITLE_MONSTER_INK_BOTTOM - 15)

/* Nhịp nhấp nháy. Nửa chu kỳ, tức sáng 500 ms rồi tắt 500 ms. */
#define TITLE_BLINK_INTERVAL        AC_DISPLAY_TITLE_BLINK_INTERVAL

/*****************************************************************************/
/* Variable Declaration - title */
/*****************************************************************************/
/* true = đang hiện dòng gợi ý. Đảo qua đảo lại mỗi lần timer nổ. */
static bool title_hint_visible = true;

/*****************************************************************************/
/* View - title */
/*****************************************************************************/
static void view_scr_title();

view_dynamic_t dyn_view_item_title = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_title
};

view_screen_t scr_title = {
	&dyn_view_item_title,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

static void view_scr_title() {
	view_render.setTextColor(WHITE);

	/* Tên game, chữ cỡ 2 canh giữa. */
	view_render.setTextSize(2);
	view_render.setCursor(SCR_CENTER_X_BIG(TITLE_TEXT_LEN), TITLE_ROW);
	view_render.print(TITLE_TEXT);

	/* Hero nhìn sang phải, quái đứng chờ bên kia. Đúng thế trận của màn
	 * TRAVEL trong game, nhìn cái là biết sắp phải đánh nhau với ai. */
	view_render.drawBitmap(TITLE_HERO_X, TITLE_HERO_Y,
						   hero_icon, TITLE_HERO_W, TITLE_HERO_H, WHITE);
	view_render.drawBitmap(TITLE_MONSTER_X, TITLE_MONSTER_Y,
						   monster_slime, TITLE_MONSTER_W, TITLE_MONSTER_H, WHITE);

	/* Dòng gợi ý nhấp nháy. Lúc tắt thì không vẽ gì cả, phần nền đã được
	 * view_render_screen() xoá sạch từ đầu mỗi khung hình rồi. */
	if (title_hint_visible) {
		view_render.setTextSize(1);
		view_render.setCursor(SCR_CENTER_X(TITLE_HINT_LEN), SCR_ROW_HINT);
		view_render.print(TITLE_HINT_TEXT);
	}
}

/* Cửa sổ nhỏ cho harness trên máy tính soi trạng thái nhấp nháy.
 * Không dùng trong firmware, nhưng cũng chả tốn gì: một hàm trả về bool. */
int title_hint_probe() {
	return title_hint_visible ? 1 : 0;
}

/*****************************************************************************/
/* Handle - title */
/*****************************************************************************/
/* Dọn timer nhấp nháy trước khi rời màn.
 *
 * Screen manager của AK không phát SCREEN_EXIT, nên không có chỗ nào tự dọn
 * hộ. Timer kiểu PERIODIC mà bỏ quên thì nó cứ 500 ms lại bắn một message
 * AC_DISPLAY_TITLE_BLINK vào task display suốt đời máy chạy. Task display
 * sẽ rơi vào nhánh default: nên không sai chức năng, nhưng mỗi lần như vậy
 * lại kéo theo một lần render lại toàn khung hình 1024 byte qua I2C. */
static void title_stop_blink() {
	timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_TITLE_BLINK);
}

void scr_title_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		view_render.initialize();
		view_render_display_on();

		/* Vào màn là hiện chữ ngay, đừng để người chơi phải chờ nửa giây
		 * mới thấy dòng gợi ý xuất hiện. */
		title_hint_visible = true;
		timer_set(	AC_TASK_DISPLAY_ID, \
					AC_DISPLAY_TITLE_BLINK, \
					TITLE_BLINK_INTERVAL, \
					TIMER_PERIODIC);
	}
		break;

	case AC_DISPLAY_TITLE_BLINK: {
		title_hint_visible = !title_hint_visible;
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_MODE_RELEASED\n");
		title_stop_blink();
		SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
	}
		BUZZER_PlayTones(tones_cc);
		break;

	default:
		break;
	}
}
