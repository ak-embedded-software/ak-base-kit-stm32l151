/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 30/04/2026
 *          End:   01/05/2026
 ******************************************************************************
**/
#include "scr_game_setting.h"

/*****************************************************************************/
/* Variable Declaration - Setting game */
/*****************************************************************************/
/* settingdata now lives in app_eeprom.cpp next to its read/write helpers. */
static uint8_t setting_location_chosse;


/*****************************************************************************/
/* View - Setting game */
/*****************************************************************************/
static void view_scr_game_setting();

view_dynamic_t dyn_view_item_game_setting = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_game_setting
};

view_screen_t scr_game_setting = {
	&dyn_view_item_game_setting,
	ITEM_NULL,
	ITEM_NULL,

	.focus_item = 0,
};

/* Dòng đầu tiên của hàng thứ row (0..3). */
static int16_t setting_row_y(uint8_t row) {
	return (int16_t)(DUNGEON_SETTING_FRAMES_AXIS_Y_1 + DUNGEON_SETTING_FRAMES_STEP * row);
}

/* setting_location_chosse vẫn giữ kiểu mã hoá cũ 15/30/45/60 để phần
 * xử lý nút không phải sửa. Ở đây chỉ đổi nó về chỉ số hàng 0..3. */
static uint8_t setting_row_index() {
	uint8_t row = (uint8_t)((setting_location_chosse / STEP_SETTING_CHOSSE) - 1);
	if (row >= DUNGEON_SETTING_ROW_COUNT) {
		row = 0;
	}
	return row;
}

void view_scr_game_setting() {
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);

	/* Con trỏ là tam giác nhỏ chỉ sang phải, canh giữa hàng đang chọn.
	 * Cao 7 nằm gọn trong hàng cao 13 nên không bao giờ đè sang hàng khác. */
	int16_t cursor_y = setting_row_y(setting_row_index())
			+ ((DUNGEON_SETTING_FRAMES_SIZE_H - DUNGEON_SETTING_CURSOR_H) / 2);
	view_render.fillTriangle(
			DUNGEON_SETTING_CURSOR_AXIS_X, cursor_y,
			DUNGEON_SETTING_CURSOR_AXIS_X, cursor_y + DUNGEON_SETTING_CURSOR_H - 1,
			DUNGEON_SETTING_CURSOR_AXIS_X + DUNGEON_SETTING_CURSOR_W - 1,
			cursor_y + (DUNGEON_SETTING_CURSOR_H / 2),
			WHITE);

	for (uint8_t row = 0; row < DUNGEON_SETTING_ROW_COUNT; row++) {
		view_render.drawRoundRect(DUNGEON_SETTING_FRAMES_AXIS_X, setting_row_y(row),
				DUNGEON_SETTING_FRAMES_SIZE_W, DUNGEON_SETTING_FRAMES_SIZE_H,
				DUNGEON_SETTING_FRAMES_SIZE_R, WHITE);
	}

	/* Hàng 1 - Frames */
	view_render.setCursor(DUNGEON_SETTING_TEXT_AXIS_X, setting_row_y(0) + DUNGEON_SETTING_TEXT_OFFSET_Y);
	view_render.print("Frames");
	view_render.setCursor(DUNGEON_SETTING_NUMBER_AXIS_X, setting_row_y(0) + DUNGEON_SETTING_TEXT_OFFSET_Y);
	view_render.print("(");
	view_render.print(settingdata.party_size);
	view_render.print(")");

	/* Hàng 2 - Lane speed */
	view_render.setCursor(DUNGEON_SETTING_TEXT_AXIS_X, setting_row_y(1) + DUNGEON_SETTING_TEXT_OFFSET_Y);
	view_render.print("Lane speed");
	view_render.setCursor(DUNGEON_SETTING_NUMBER_AXIS_X, setting_row_y(1) + DUNGEON_SETTING_TEXT_OFFSET_Y);
	view_render.print("(");
	view_render.print(settingdata.monster_speed);
	view_render.print(")");

	/* Hàng 3 - Silent, kèm icon loa cao 7 canh giữa hàng cao 13 */
	view_render.setCursor(DUNGEON_SETTING_TEXT_AXIS_X, setting_row_y(2) + DUNGEON_SETTING_TEXT_OFFSET_Y);
	view_render.print("Silent");
	view_render.drawBitmap(DUNGEON_SETTING_SPEAKER_AXIS_X,
			setting_row_y(2) + ((DUNGEON_SETTING_FRAMES_SIZE_H - DUNGEON_SETTING_SPEAKER_SIZE) / 2),
			(settingdata.silent == 0) ? speaker_1 : speaker_2,
			DUNGEON_SETTING_SPEAKER_SIZE, DUNGEON_SETTING_SPEAKER_SIZE, WHITE);

	/* Hàng 4 - EXIT, canh giữa theo bề ngang của khung */
	view_render.setCursor(DUNGEON_SETTING_FRAMES_AXIS_X
			+ ((DUNGEON_SETTING_FRAMES_SIZE_W - (4 * SCR_CHAR_W)) / 2),
			setting_row_y(3) + DUNGEON_SETTING_TEXT_OFFSET_Y);
	view_render.print("EXIT");
	view_render.update();
}

/*****************************************************************************/
/* Handle - Setting game */
/*****************************************************************************/
void scr_game_setting_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		view_render.clear();
		setting_location_chosse = SETTING_ITEM_ARRDESS_1;
		dungeon_setting_read(&settingdata);
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_MODE_RELEASED\n");
		switch (setting_location_chosse) {
		case SETTING_ITEM_ARRDESS_1: {
			settingdata.party_size++;
			if (settingdata.party_size > 5) {
				settingdata.party_size = 1;
			}
		}
			break;

		case SETTING_ITEM_ARRDESS_2: {
			settingdata.monster_speed++;
			if (settingdata.monster_speed > 5) {
				settingdata.monster_speed = 1;
			}
		}
			break;

		case SETTING_ITEM_ARRDESS_3: {
			settingdata.silent = !settingdata.silent;
			BUZZER_Sleep(settingdata.silent);
		}
			break;

		case SETTING_ITEM_ARRDESS_4: {
			settingdata.anim_speed = 4;
			dungeon_setting_write(&settingdata);
			SCREEN_TRAN(scr_menu_game_handle, &scr_menu_game);
			BUZZER_PlayTones(tones_startup);
		}
			break;

		default:
			break;
		}
	}
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_UP_LONG_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_UP_LONG_PRESSED\n");
		settingdata.party_size = 5;
		settingdata.monster_speed = 5;
		settingdata.silent = 0;
	}
		BUZZER_Sleep(settingdata.silent);
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_UP_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_UP_RELEASED\n");
		setting_location_chosse -= STEP_SETTING_CHOSSE;
		if (setting_location_chosse == SETTING_ITEM_ARRDESS_0) {
			setting_location_chosse = SETTING_ITEM_ARRDESS_4;
		}
	}
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_DOWN_LONG_PRESSED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_DOWN_LONG_PRESSED\n");
		settingdata.party_size = 1;
		settingdata.monster_speed = 1;
		settingdata.silent = 1;
	}
		BUZZER_Sleep(settingdata.silent);
		BUZZER_PlayTones(tones_cc);
		break;

	case AC_DISPLAY_BUTTON_DOWN_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_DOWN_RELEASED\n");
		setting_location_chosse += STEP_SETTING_CHOSSE;
		if (setting_location_chosse > SETTING_ITEM_ARRDESS_4) {
			setting_location_chosse = SETTING_ITEM_ARRDESS_1;
		}
	}
		BUZZER_PlayTones(tones_cc);
		break;

	default:
		break;
	}
}
