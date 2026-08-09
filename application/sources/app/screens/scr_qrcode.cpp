#include "scr_qrcode.h"

#define QRCODE_VERSION			(8)
#define QRCODE_AXIS_X			(40)
#define QRCODE_AXIS_Y			(2)
#define QRCODE_MAX_SIZE			(49)
#define QRCODE_BITMAP_STRIDE	((QRCODE_MAX_SIZE + 7) / 8)
#define QRCODE_BITMAP_SIZE		(QRCODE_BITMAP_STRIDE * QRCODE_MAX_SIZE)

#define QRCODE_TEXT				"GitHub URL"
#define QRCODE_TEXT_AXIS_X		(35)
#define QRCODE_TEXT_AXIS_Y		(55)

static uint8_t qrcode_bitmap[QRCODE_BITMAP_SIZE];
static uint8_t qrcode_bitmap_size = 0;

static void view_scr_qrcode();

static void qrcode_bitmap_set_pixel(uint8_t x, uint8_t y) {
	qrcode_bitmap[y * QRCODE_BITMAP_STRIDE + (x / 8)] |= (128 >> (x & 7));
}

static void qrcode_build_bitmap() {
	QRCode qrcode;
	uint8_t qrcodeData[qrcode_getBufferSize(QRCODE_VERSION)];

	memset(qrcode_bitmap, 0, sizeof(qrcode_bitmap));
	qrcode_initText(&qrcode, qrcodeData, QRCODE_VERSION, 0, "https://github.com/the-ak-foundation/ak-base-kit-stm32l151");
	qrcode_bitmap_size = qrcode.size;

	for (uint8_t y = 0; y < qrcode.size; y++) {
		for (uint8_t x = 0; x < qrcode.size; x++) {
			if (qrcode_getModule(&qrcode, x, y)) {
				qrcode_bitmap_set_pixel(x, y);
			}
		}
	}
}

view_dynamic_t dyn_view_qrcode = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_qrcode
};

view_screen_t scr_qrcode = {
	&dyn_view_qrcode,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

class line_segment {
public:
	int x0, x1;
	int distance() {
		return (x1 - x0);
	}
};

void view_scr_qrcode() {
	view_render.drawBitmap(	QRCODE_AXIS_X, \
							QRCODE_AXIS_Y, \
							qrcode_bitmap, \
							qrcode_bitmap_size, \
							qrcode_bitmap_size, \
							WHITE);

	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(QRCODE_TEXT_AXIS_X, QRCODE_TEXT_AXIS_Y);
	view_render.print(QRCODE_TEXT);
}

void scr_qrcode_handle(ak_msg_t *msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		if (qrcode_bitmap_size == 0) {
			qrcode_build_bitmap();
		}
		timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_IDLE, AC_DISPLAY_LOGO_INTERVAL, TIMER_ONE_SHOT);
	} break;

	case AC_DISPLAY_SHOW_IDLE: {
		APP_DBG_SIG("AC_DISPLAY_SHOW_IDLE\n");
		SCREEN_TRAN(scr_welcome_handle, &scr_welcome);
	} break;

	case AC_DISPLAY_BUTON_UP_PRESSED:
	case AC_DISPLAY_BUTON_DOWN_PRESSED:
	case AC_DISPLAY_BUTON_MODE_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTON_%s_PRESSED\n", msg->sig == AC_DISPLAY_BUTON_MODE_PRESSED ? "MODE" :\
													 msg->sig == AC_DISPLAY_BUTON_DOWN_PRESSED ? "DOWN" : "UP");
		timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_SHOW_IDLE);
		SCREEN_TRAN(scr_welcome_handle, &scr_welcome);
	} break;

	default:
		break;
	}
}
