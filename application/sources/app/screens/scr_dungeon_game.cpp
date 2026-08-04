/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 05/05/2026
 *          End:   13/05/2026
 ******************************************************************************
**/
#include "scr_dungeon_game.h"

#include <stdio.h>
#include <string.h>

#include "screens.h"
#include "dungeon_runtime.h"

/* Global state shared with other screens/modules. */
uint8_t dungeon_game_state;                       /* GAME_OFF / GAME_PLAY / GAME_OVER */
dungeon_game_setting_t settingsetup;             /* User gameplay settings from EEPROM */
uint8_t dungeon_start_mode = DUNGEON_START_NEW_GAME; /* Start policy (new/continue/level/creator) */
uint8_t dungeon_selected_level = 1;              /* Current selected level from menu */
uint8_t dungeon_last_outcome = DUNGEON_OUTCOME_NONE; /* Win/Lose result passed to game-over screen */
uint8_t dungeon_persist_enabled = 1;

/*****************************************************************************/
/*  Hằng số bố cục riêng của màn dungeon.
 *  Lề, hàng chữ và các macro canh giữa nằm ở screens_layout.h dùng chung.
 */
/*****************************************************************************/
#define DUNGEON_ROW_STATS           SCR_PAD_T   /* HP / D / S / E          */
#define DUNGEON_ROW_RULE            SCR_ROW_RULE
#define DUNGEON_ROW_PROGRESS        (13)        /* L1 S:1/4 ... SC:0       */
#define DUNGEON_ROW_ARENA_TOP       (21)        /* mép trên vùng sân       */
#define DUNGEON_ROW_ARENA_BOTTOM    (51)        /* mép dưới vùng sân       */
#define DUNGEON_ROW_CAPTION         SCR_ROW_HINT
#define DUNGEON_ROW_BUTTON          (53)        /* hàng nút hành động      */

/* Sân đánh chia đôi: hero bên trái, quái cùng thông tin của nó bên phải. */
#define DUNGEON_BATTLE_SPLIT_X      (47)        /* vạch dọc chia hai nửa   */
#define DUNGEON_BATTLE_INFO_X       (50)        /* cột thông tin quái      */
#define DUNGEON_MONSTER_X           (94)        /* mép trái sprite quái    */

/* Nút hành động: 5 nút rộng 22 px, bước 25 px -> 3..124, cân lề hai bên. */
#define DUNGEON_BUTTON_W            (22)
#define DUNGEON_BUTTON_H            (8)
#define DUNGEON_BUTTON_PITCH        (25)

/* Ô vật phẩm màn CHEST: 3 ô rộng 38 px, khe 4 px -> 3..124. */
#define DUNGEON_CHEST_BOX_W         (38)
#define DUNGEON_CHEST_BOX_H         (33)
#define DUNGEON_CHEST_BOX_PITCH     (42)
#define DUNGEON_CHEST_BOX_TOP       (18)

/* Quái chỉ xuất hiện khi hero đã đi tới ngưỡng này, và xuất hiện nguyên con
 * chứ không hé dần. Trước ngưỡng thì không vẽ gì cả. */
#define DUNGEON_MONSTER_REVEAL_AT   (70)

static void view_scr_dungeon_game();

/* Game rules now live in the five dungeon tasks (see dungeon_runtime.h).
 * What is left in this file is the view: bitmap lookup, the draw_* routines
 * and the screen message handler. */

view_dynamic_t dyn_view_item_dungeon_game = {
	{
		.item_type = ITEM_TYPE_DYNAMIC,
	},
	view_scr_dungeon_game
};

view_screen_t scr_dungeon_game = {
	&dyn_view_item_dungeon_game,
	ITEM_NULL,
	ITEM_NULL,
	.focus_item = 0,
};

void dungeon_load_setting() {
	/* dungeon_setting_read() validates magic + checksum, falls back to
	 * defaults, and clamps out-of-range fields. */
	dungeon_setting_read(&settingsetup);
}

static void dungeon_start_tick_timer() {
	timer_set(AC_TASK_DISPLAY_ID, DUNGEON_TIME_TICK, DUNGEON_TIME_TICK_INTERVAL, TIMER_PERIODIC);
}

static dungeon_bitmap_t dungeon_monster_bitmap(uint8_t monster) {
	switch (monster) {
	case DUNGEON_MONSTER_SLIME:
		return {monster_slime, 31, 30};
	case DUNGEON_MONSTER_GOBLIN:
		return {monster_goblin, 31, 30};
	case DUNGEON_MONSTER_WOLF:
		return {monster_wolf, 30, 30};
	case DUNGEON_MONSTER_GORILLA:
		return {monster_gorilla, 30, 30};
	case DUNGEON_MONSTER_DRAGON:
		return {monster_dragon, 31, 30};
	default:
		return {monster_eye, 30, 30};
	}
}

static dungeon_bitmap_t dungeon_item_bitmap(uint8_t item) {
	switch (item) {
	case DUNGEON_ITEM_SWORD:
		return {item_sword, 20, 20};
	case DUNGEON_ITEM_SHIELD:
		return {item_shield, 20, 20};
	case DUNGEON_ITEM_HEAL:
		return {item_heal, 20, 20};
	case DUNGEON_ITEM_BOMB:
		/* Bản 20x20, không phải item_bomb 20x25. Bản cao tràn khỏi ô CHEST. */
		return {item_bomb20, 20, 20};
	case DUNGEON_ITEM_ANTIDOTE:
		return {item_shrine, 20, 20};
	case DUNGEON_ITEM_PURIFY:
		return {item_heal, 20, 20};
	default:
		return {item_trap, 15, 20};
	}
}

/* Thanh chỉ số người chơi. Bỏ khung bao, chỉ còn chữ với một dòng kẻ ngăn.
 * Bốn mốc x chừa chỗ cho HP 3 chữ số. */
static void dungeon_draw_stats_line() {
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(SCR_PAD_L, DUNGEON_ROW_STATS);
	view_render.print("HP:");
	view_render.print(dungeon_runtime.player_hp);
	view_render.setCursor(42, DUNGEON_ROW_STATS);
	view_render.print("D:");
	view_render.print(dungeon_runtime.player_atk);
	view_render.setCursor(70, DUNGEON_ROW_STATS);
	view_render.print("S:");
	view_render.print(dungeon_runtime.player_def);
	view_render.setCursor(98, DUNGEON_ROW_STATS);
	view_render.print("E:");
	view_render.print(dungeon_effect_damage());
	view_render.drawLine(SCR_PAD_L, DUNGEON_ROW_RULE, SCR_PAD_R, DUNGEON_ROW_RULE, WHITE);
}

/* Dòng tiến độ dùng chung cho travel và battle. */
static void dungeon_draw_progress_line(bool with_colon) {
	view_render.setCursor(SCR_PAD_L, DUNGEON_ROW_PROGRESS);
	view_render.print("L");
	view_render.print(dungeon_runtime.level);
	view_render.print(with_colon ? " S:" : " S");
	view_render.print(dungeon_runtime.stage);
	view_render.print("/");
	view_render.print(dungeon_runtime.total_stages);
	view_render.setCursor(79, DUNGEON_ROW_PROGRESS);
	view_render.print("SC:");
	view_render.print(dungeon_game_score);
}

static void dungeon_draw_travel() {
	/* Hero dừng trước chỗ quái đứng. Công thức cũ (8 + progress*84/100) cho hero
	 * tới x=92 lúc về đích, đè lên sprite quái ở x=94. */
	int16_t hero_x = SCR_PAD_L + ((int16_t)dungeon_runtime.travel_progress * 63) / 100;
	int16_t hero_y = 0;
	uint8_t phase = (uint8_t)((dungeon_control.action_image * 2) + (dungeon_runtime.travel_progress / 6));
	dungeon_bitmap_t monster = dungeon_monster_bitmap(dungeon_runtime.current_monster);

	view_render.fillScreen(BLACK);

	/* Dải hang CỐ Ý tràn hết chiều ngang màn hình, từ cột 0 tới 127, không theo
	 * lề 3 px. Nó là đường di chuyển nên phải chạy suốt hai rìa mới ra cảm giác
	 * hang dài vô tận. Chỉ phần chữ (chỉ số, tiến độ, chú thích) mới giữ lề. */
	for (int16_t x = 0; x < LCD_WIDTH; x++) {
		int16_t wave_a = (x + phase) % 52;
		int16_t wave_b = (x + (phase / 2) + 11) % 74;
		if (wave_a > 26) {
			wave_a = 52 - wave_a;
		}
		if (wave_b > 37) {
			wave_b = 74 - wave_b;
		}

		int16_t top = 22 + (wave_a / 5) - (wave_b / 10);
		int16_t bottom = 47 + (wave_b / 9) - (wave_a / 12);

		if (top < DUNGEON_ROW_ARENA_TOP) {
			top = DUNGEON_ROW_ARENA_TOP;
		}
		if (bottom > DUNGEON_ROW_ARENA_BOTTOM) {
			bottom = DUNGEON_ROW_ARENA_BOTTOM;
		}
		if ((bottom - top) < 18) {
			bottom = top + 18;
		}
		if (bottom > DUNGEON_ROW_ARENA_BOTTOM) {
			bottom = DUNGEON_ROW_ARENA_BOTTOM;
		}

		if ((x >= hero_x) && (x <= (hero_x + 23))) {
			hero_y = top + ((bottom - top) / 2) - 8;
		}

		view_render.drawFastVLine(x, top, bottom - top + 1, WHITE);
	}

	dungeon_draw_stats_line();
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	dungeon_draw_progress_line(true);

	/* Hero là bóng ĐEN khoét trên dải hang trắng. */
	view_render.drawBitmap(hero_x, hero_y, hero_icon, 24, 17, BLACK);

	/* Quái: ẩn hoàn toàn cho tới khi hero đi đủ gần, rồi hiện nguyên con.
	 * Không hé dần từng cột. */
	bool monster_shown = false;
	if ((dungeon_runtime.support_event == 0) &&
		(dungeon_runtime.travel_progress >= DUNGEON_MONSTER_REVEAL_AT)) {
		view_render.drawBitmap(DUNGEON_MONSTER_X, DUNGEON_ROW_ARENA_TOP,
							   monster.data, monster.width, monster.height, BLACK);
		monster_shown = true;
	}

	if (dungeon_runtime.support_event > 0) {
		view_render.drawRect(104, 26, 14, 14, BLACK);
		view_render.setCursor(108, 29);
		view_render.setTextColor(BLACK);
		view_render.print("?");
		view_render.setTextColor(WHITE);
	}

	/* Chú thích nằm DƯỚI dải hang. Trước đây vẽ ở row 44, tức là chữ trắng
	 * trên dải trắng nên vô hình. */
	if (dungeon_runtime.support_event > 0) {
		view_render.setCursor(25, DUNGEON_ROW_CAPTION);
		view_render.print("Move to chest");
	}
	else if (monster_shown) {
		if (dungeon_runtime.stage >= dungeon_runtime.total_stages) {
			view_render.setCursor(37, DUNGEON_ROW_CAPTION);
			view_render.print("Boss ahead");
		}
		else {
			view_render.setCursor(31, DUNGEON_ROW_CAPTION);
			view_render.print("Enemy ahead");
		}
	}
}

static void dungeon_draw_message() {
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(SCR_PAD_L, 8);
	view_render.print(dungeon_runtime.line_1);
	view_render.setCursor(SCR_PAD_L, 22);
	view_render.print(dungeon_runtime.line_2);
	view_render.setCursor(SCR_PAD_L, 36);
	view_render.print(dungeon_runtime.line_3);
	view_render.drawLine(SCR_PAD_L, 48, SCR_PAD_R, 48, WHITE);
	view_render.setCursor(28, DUNGEON_ROW_CAPTION);
	view_render.print("MODE TO NEXT");
}

/* Nhãn ngắn của từng món trong rương. Dài nhất là POISON, 6 ký tự = 36 px,
 * vẫn vừa ô rộng 38 px. Tách ra hàm riêng để chỗ vẽ đo được độ dài chuỗi
 * mà canh giữa ô, thay vì dán sát mép trái. */
static const char* dungeon_chest_item_label(uint8_t item) {
	switch (item) {
	case DUNGEON_ITEM_SWORD:    return "SWORD";
	case DUNGEON_ITEM_SHIELD:   return "SHIELD";
	case DUNGEON_ITEM_HEAL:     return "HEART";
	case DUNGEON_ITEM_BOMB:     return "BOMB";
	case DUNGEON_ITEM_ANTIDOTE: return "ANTI";
	case DUNGEON_ITEM_PURIFY:   return "PURE";
	default:                    return "POISON";
	}
}

static void dungeon_draw_chest() {
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	view_render.setCursor(SCR_CENTER_X(13), SCR_PAD_T);
	view_render.print("MYSTERY CHEST");

	/* Con trỏ tam giác nằm phía trên ô đang chọn, chừa 1 dòng với mép ô. */
	int16_t tri_x = SCR_PAD_L + (DUNGEON_CHEST_BOX_W / 2) +
					(dungeon_runtime.selected_support_item * DUNGEON_CHEST_BOX_PITCH);
	view_render.fillTriangle(tri_x, DUNGEON_CHEST_BOX_TOP - 2,
							 tri_x - 4, DUNGEON_CHEST_BOX_TOP - 6,
							 tri_x + 4, DUNGEON_CHEST_BOX_TOP - 6, WHITE);

	for (uint8_t index = 0; index < 3; index++) {
		dungeon_bitmap_t bitmap = dungeon_item_bitmap(dungeon_runtime.chest_options[index]);
		int16_t axis_x = SCR_PAD_L + (index * DUNGEON_CHEST_BOX_PITCH);
		int16_t axis_y = DUNGEON_CHEST_BOX_TOP;
		view_render.drawRect(axis_x, axis_y, DUNGEON_CHEST_BOX_W, DUNGEON_CHEST_BOX_H, WHITE);

		/* Nhãn canh giữa ô: đo độ dài chuỗi rồi chia đôi phần dư.
		 * Ví dụ "BOMB" 4 ký tự = 24 px, ô 38 px -> lùi vào (38-24)/2 = 7 px. */
		const char* label = dungeon_chest_item_label(dungeon_runtime.chest_options[index]);
		int16_t label_w = (int16_t)strlen(label) * SCR_CHAR_W;
		view_render.setCursor(axis_x + ((DUNGEON_CHEST_BOX_W - label_w) / 2), axis_y + 1);
		view_render.print(label);

		/* Sprite 20x20 canh giữa ô 38 px, chừa lề dưới trong ô. */
		view_render.drawBitmap(axis_x + ((DUNGEON_CHEST_BOX_W - 20) / 2), axis_y + 10,
							   bitmap.data, bitmap.width, bitmap.height, WHITE);
	}
	view_render.setCursor(SCR_CENTER_X(14), DUNGEON_ROW_CAPTION);
	view_render.print("UP/DOWN + MODE");
}

static void dungeon_draw_battle() {
	dungeon_bitmap_t monster = dungeon_monster_bitmap(dungeon_runtime.current_monster);
	int8_t hero_dx = 0;
	int8_t hero_dy = 0;
	int8_t monster_dx = 0;
	int8_t monster_dy = 0;
	uint8_t anim_progress = 0;
	int8_t lunge = 0;
	uint8_t monster_hit_visible = 0;
	uint8_t hero_hit_visible = 0;

	if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_HERO_ATK_LUNGE) {
		if (dungeon_runtime.battle_wait_ticks < DUNGEON_BATTLE_STEP_TICKS) {
			anim_progress = (uint8_t)(DUNGEON_BATTLE_STEP_TICKS - dungeon_runtime.battle_wait_ticks);
		}
		if (anim_progress < 5) {
			lunge = (int8_t)(anim_progress + 1);
		}
		hero_dx = lunge;
		hero_dy = (lunge > 0) ? -1 : 0;
		monster_dx = -(lunge / 2);
	}
	else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_HERO_ATK_HIT) {
		hero_dx = 3;
		hero_dy = -1;
		monster_hit_visible = (dungeon_runtime.pending_attack_hit != 0);
	}
	else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_LUNGE) {
		if (dungeon_runtime.battle_wait_ticks < DUNGEON_BATTLE_WAIT_TICKS) {
			anim_progress = (uint8_t)(DUNGEON_BATTLE_WAIT_TICKS - dungeon_runtime.battle_wait_ticks);
		}
		if (anim_progress < 5) {
			lunge = (int8_t)(anim_progress + 1);
		}
		else if (anim_progress < 9) {
			lunge = (int8_t)(10 - anim_progress);
		}
		monster_dx = -lunge;
		monster_dy = (lunge > 0) ? -1 : 0;
		hero_dx = lunge / 2;
	}
	else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_HIT) {
		hero_hit_visible = 1;
	}

	if ((dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_HIT) || ((dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_INPUT) && (dungeon_runtime.player_shake_ticks > 0))) {
		hero_dx += ((dungeon_runtime.player_shake_ticks & 0x02) == 0) ? 1 : -1;
		hero_dy += ((dungeon_runtime.player_shake_ticks & 0x04) == 0) ? -1 : 1;
	}
	if ((monster_hit_visible != 0) || ((dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_INPUT) && (dungeon_runtime.monster_shake_ticks > 0))) {
		monster_dx += ((dungeon_runtime.monster_shake_ticks & 0x02) == 0) ? 1 : -1;
		monster_dy += ((dungeon_runtime.monster_shake_ticks & 0x04) == 0) ? -1 : 1;
	}
	dungeon_draw_stats_line();
	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);
	dungeon_draw_progress_line(false);

	/* Vạch dọc chia sân: hero bên trái, quái cùng thông tin của nó bên phải.
	 * Không có vạch này thì cột thông tin trông như trôi giữa màn hình. */
	view_render.drawFastVLine(DUNGEON_BATTLE_SPLIT_X, DUNGEON_ROW_ARENA_TOP,
							  DUNGEON_ROW_ARENA_BOTTOM - DUNGEON_ROW_ARENA_TOP + 1, WHITE);

	view_render.drawBitmap(SCR_PAD_L + hero_dx, 28 + hero_dy, hero_icon, 24, 17, WHITE);
	if (dungeon_runtime.defend_icon_active) {
		int16_t shield_x = 27 + hero_dx;
		int16_t shield_y = 30 + hero_dy;
		view_render.drawPixel(shield_x, shield_y + 0, WHITE);
		view_render.drawPixel(shield_x + 1, shield_y - 1, WHITE);
		view_render.drawPixel(shield_x + 2, shield_y - 2, WHITE);
		view_render.drawPixel(shield_x + 3, shield_y - 3, WHITE);
		view_render.drawPixel(shield_x + 4, shield_y - 3, WHITE);
		view_render.drawPixel(shield_x + 5, shield_y - 2, WHITE);
		view_render.drawPixel(shield_x + 6, shield_y - 1, WHITE);
		view_render.drawPixel(shield_x + 7, shield_y + 0, WHITE);
		view_render.drawPixel(shield_x + 7, shield_y + 1, WHITE);
		view_render.drawPixel(shield_x + 6, shield_y + 2, WHITE);
		view_render.drawPixel(shield_x + 5, shield_y + 3, WHITE);
		view_render.drawPixel(shield_x + 4, shield_y + 4, WHITE);
		view_render.drawPixel(shield_x + 3, shield_y + 4, WHITE);
		view_render.drawPixel(shield_x + 2, shield_y + 3, WHITE);
		view_render.drawPixel(shield_x + 1, shield_y + 2, WHITE);
	}

	view_render.drawBitmap(DUNGEON_MONSTER_X + monster_dx, DUNGEON_ROW_ARENA_TOP + monster_dy,
						   monster.data, monster.width, monster.height, WHITE);

	if (monster_hit_visible) {
		/* Vệt chém hiện trước bước rung để chuỗi đòn đọc được rõ. */
		view_render.drawLine(100 + monster_dx, 24 + monster_dy, 110 + monster_dx, 34 + monster_dy, WHITE);
		view_render.drawLine(98 + monster_dx, 30 + monster_dy, 108 + monster_dx, 40 + monster_dy, WHITE);
		view_render.drawLine(104 + monster_dx, 26 + monster_dy, 114 + monster_dx, 36 + monster_dy, WHITE);
	}
	if (hero_hit_visible) {
		view_render.drawLine(11 + hero_dx, 28 + hero_dy, 21 + hero_dx, 38 + hero_dy, WHITE);
		view_render.drawLine(9 + hero_dx, 34 + hero_dy, 19 + hero_dx, 44 + hero_dy, WHITE);
		view_render.drawLine(15 + hero_dx, 30 + hero_dy, 25 + hero_dx, 40 + hero_dy, WHITE);
	}

	/* Popup số damage. Bay lên trong vùng sân, không đè lên thanh chỉ số. */
	if (dungeon_runtime.player_hp_popup_ticks > 0) {
		uint8_t rise = (uint8_t)(dungeon_runtime.player_hp_popup_ticks / 6);
		view_render.setCursor(SCR_PAD_L, 24 - rise);
		view_render.print("-");
		view_render.print(dungeon_runtime.player_hp_popup_value);
	}
	if (dungeon_runtime.monster_hp_popup_ticks > 0) {
		uint8_t rise = (uint8_t)(dungeon_runtime.monster_hp_popup_ticks / 6);
		view_render.setCursor(98, 24 - rise);
		view_render.print("-");
		view_render.print(dungeon_runtime.monster_hp_popup_value);
	}
	if (dungeon_runtime.monster_armor_popup_ticks > 0) {
		uint8_t rise = (uint8_t)(dungeon_runtime.monster_armor_popup_ticks / 7);
		view_render.setCursor(DUNGEON_BATTLE_INFO_X, 45 - rise);
		view_render.print("D-");
		view_render.print(dungeon_runtime.monster_armor_popup_value);
	}

	/* Thông tin quái nằm bên nửa của quái, dán sát sprite.
	 * Cột này rộng 43 px = 7 ký tự, nên tên phải dùng bản rút gọn và nhãn
	 * phải bỏ dấu hai chấm. */
	view_render.setCursor(DUNGEON_BATTLE_INFO_X, DUNGEON_ROW_ARENA_TOP);
	view_render.print(dungeon_monster_name_short[dungeon_runtime.current_monster]);
	view_render.setCursor(DUNGEON_BATTLE_INFO_X, DUNGEON_ROW_ARENA_TOP + 8);
	view_render.print("HP");
	view_render.print(dungeon_runtime.monster_hp);
	view_render.print("/");
	view_render.print(dungeon_runtime.monster_max_hp);
	view_render.setCursor(DUNGEON_BATTLE_INFO_X, DUNGEON_ROW_ARENA_TOP + 16);
	view_render.print("ATK");
	view_render.print(dungeon_runtime.monster_dmg);
	view_render.setCursor(DUNGEON_BATTLE_INFO_X, DUNGEON_ROW_ARENA_TOP + 24);
	view_render.print("DEF");
	if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_INPUT) {
		view_render.print(dungeon_runtime.monster_armor);
	}
	else {
		view_render.print("..");
	}

	for (uint8_t index = 0; index < DUNGEON_ACTION_COUNT; index++) {
		int16_t axis_x = SCR_PAD_L + (index * DUNGEON_BUTTON_PITCH);
		view_render.drawRoundRect(axis_x, DUNGEON_ROW_BUTTON,
								  DUNGEON_BUTTON_W, DUNGEON_BUTTON_H, 2, WHITE);
		if (index == dungeon_runtime.selected_action) {
			view_render.fillRoundRect(axis_x, DUNGEON_ROW_BUTTON,
									  DUNGEON_BUTTON_W, DUNGEON_BUTTON_H, 2, WHITE);
			view_render.setTextColor(BLACK);
			view_render.setCursor(axis_x + 2, DUNGEON_ROW_BUTTON + 1);
			view_render.print(dungeon_action_name[index]);
			view_render.setTextColor(WHITE);
		}
		else {
			view_render.setCursor(axis_x + 2, DUNGEON_ROW_BUTTON + 1);
			view_render.print(dungeon_action_name[index]);
		}
	}
}

static void view_scr_dungeon_game() {
	view_render.clear();
	if (dungeon_game_state == GAME_PLAY) {
		if (dungeon_runtime.current_view == DUNGEON_VIEW_TRAVEL) {
			dungeon_draw_travel();
		}
		else if (dungeon_runtime.current_view == DUNGEON_VIEW_CHEST) {
			dungeon_draw_chest();
		}
		else if (dungeon_runtime.current_view == DUNGEON_VIEW_BATTLE) {
			dungeon_draw_battle();
		}
		else {
			dungeon_draw_message();
		}
	}
	else if (dungeon_game_state == GAME_OVER) {
		dungeon_draw_stats_line();
		/* Cỡ chữ 2: mỗi ký tự 12 px ngang, 14 px cao. 5 ký tự = 60 px.
		 * Canh giữa trong vùng 3..124 -> x = 3 + (122-60)/2 = 34. */
		view_render.setTextSize(2);
		view_render.setTextColor(WHITE);
		if (dungeon_last_outcome == DUNGEON_OUTCOME_WIN) {
			view_render.setCursor(34, 19);
			view_render.print("LEVEL");
			view_render.setCursor(40, 35);
			view_render.print("DONE");
		}
		else {
			view_render.setCursor(40, 19);
			view_render.print("GAME");
			view_render.setCursor(40, 35);
			view_render.print("OVER");
		}
		view_render.setTextSize(1);
		view_render.setCursor(19, DUNGEON_ROW_CAPTION);
		view_render.print("MODE: Continue");
	}
}

void scr_dungeon_game_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case SCREEN_ENTRY: {
		APP_DBG_SIG("SCREEN_ENTRY\n");
		view_render.initialize();
		view_render_display_on();
		dungeon_game_state = GAME_PLAY;
		/* dungeon_lane owns the session lifecycle - post LANE_SETUP first so
		 * dungeon_setup_session() runs before the other tasks initialise.
		 * (This used to also be called directly here, initialising twice.) */
		task_post_pure_msg(DUNGEON_LANE_ID, DUNGEON_LANE_SETUP);
		task_post_pure_msg(DUNGEON_CONTROL_ID, DUNGEON_CONTROL_SETUP);
		task_post_pure_msg(DUNGEON_ACTION_ID, DUNGEON_ACTION_SETUP);
		task_post_pure_msg(DUNGEON_EFFECT_ID, DUNGEON_EFFECT_SETUP);
		task_post_pure_msg(DUNGEON_STATE_ID, DUNGEON_STATE_SETUP);
		dungeon_start_tick_timer();
	}
		break;

	case DUNGEON_TIME_TICK: {
		APP_DBG_SIG("DUNGEON_GAME_TIME_TICK\n");
		/* Order matters: all five game tasks sit at TASK_PRI_LEVEL_4, so they are
		 * served FIFO. Effects decay before the turn machine advances, exactly as
		 * when dungeon_tick() did both itself. */
		task_post_pure_msg(DUNGEON_CONTROL_ID, DUNGEON_CONTROL_UPDATE);
		task_post_pure_msg(DUNGEON_EFFECT_ID, DUNGEON_EFFECT_UPDATE);
		task_post_pure_msg(DUNGEON_LANE_ID, DUNGEON_LANE_LEVEL_UP);
		task_post_pure_msg(DUNGEON_ACTION_ID, DUNGEON_ACTION_RUN);
	}
		break;

	case DUNGEON_LAND_SUCCESS: {
		APP_DBG_SIG("DUNGEON_GAME_FINISH\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, DUNGEON_TIME_TICK);
		dungeon_reset_objects();
		timer_set(AC_TASK_DISPLAY_ID, DUNGEON_EXIT_GAME, DUNGEON_TIME_EXIT_INTERVAL, TIMER_ONE_SHOT);
		dungeon_save_and_reset_score();
		dungeon_game_state = GAME_OVER;
	}
		BUZZER_PlayTones(tones_startup);
		break;

	case DUNGEON_RESET: {
		APP_DBG_SIG("DUNGEON_GAME_RESET\n");
		timer_remove_attr(AC_TASK_DISPLAY_ID, DUNGEON_TIME_TICK);
		dungeon_reset_objects();
		timer_set(AC_TASK_DISPLAY_ID, DUNGEON_EXIT_GAME, DUNGEON_TIME_EXIT_INTERVAL, TIMER_ONE_SHOT);
		dungeon_save_and_reset_score();
		dungeon_game_state = GAME_OVER;
	}
		BUZZER_PlayTones(tones_3beep);
		break;

	case DUNGEON_EXIT_GAME: {
		APP_DBG_SIG("DUNGEON_GAME_EXIT\n");
		dungeon_game_state = GAME_OFF;
		SCREEN_TRAN(scr_game_over_handle, &scr_game_over);
	}
		break;

	case AC_DISPLAY_BUTTON_MODE_RELEASED: {
		APP_DBG_SIG("AC_DISPLAY_BUTTON_MODE_RELEASED\n");
		if (dungeon_game_state == GAME_OVER) {
			SCREEN_TRAN(scr_game_over_handle, &scr_game_over);
			BUZZER_PlayTones(tones_cc);
		}
	}
		break;

	case AC_DISPLAY_BUTTON_UP_RELEASED:
	case AC_DISPLAY_BUTTON_DOWN_RELEASED:
		break;

	default:
		break;
	}
}
