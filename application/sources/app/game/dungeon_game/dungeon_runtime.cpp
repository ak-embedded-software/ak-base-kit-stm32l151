/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @brief:  Shared runtime state + small helpers. See dungeon_runtime.h.
 ******************************************************************************
**/
#include <string.h>

#include "dungeon_runtime.h"

dungeon_runtime_t dungeon_runtime;
uint8_t dungeon_message_next;

const uint8_t dungeon_stage_counts[5] = {4, 5, 6, 7, 8};
const uint8_t dungeon_monster_table[5][8] = {
	{DUNGEON_MONSTER_SLIME, DUNGEON_MONSTER_GOBLIN, DUNGEON_MONSTER_WOLF, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_GORILLA},
	{DUNGEON_MONSTER_SLIME, DUNGEON_MONSTER_SLIME, DUNGEON_MONSTER_GOBLIN, DUNGEON_MONSTER_WOLF, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_GORILLA},
	{DUNGEON_MONSTER_SLIME, DUNGEON_MONSTER_GOBLIN, DUNGEON_MONSTER_GOBLIN, DUNGEON_MONSTER_WOLF, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_DRAGON, DUNGEON_MONSTER_DRAGON, DUNGEON_MONSTER_DRAGON},
	{DUNGEON_MONSTER_SLIME, DUNGEON_MONSTER_GOBLIN, DUNGEON_MONSTER_GOBLIN, DUNGEON_MONSTER_WOLF, DUNGEON_MONSTER_WOLF, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_DRAGON, DUNGEON_MONSTER_DRAGON},
	{DUNGEON_MONSTER_SLIME, DUNGEON_MONSTER_GOBLIN, DUNGEON_MONSTER_GOBLIN, DUNGEON_MONSTER_WOLF, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_GORILLA, DUNGEON_MONSTER_DRAGON, DUNGEON_MONSTER_EYE},
};

const char* dungeon_monster_name[DUNGEON_MONSTER_EYE + 1] = {
	"SLIME",
	"GOBLIN",
	"WOLF",
	"GORILLA",
	"DRAGON",
	"EYE WATCHER",
};

/* Tên rút gọn cho màn BATTLE. Cột thông tin quái ở đó chỉ rộng 43 px, tức 7 ký
 * tự, nên "EYE WATCHER" (11 ký tự) không vừa. Màn MESSAGE vẫn dùng tên đầy đủ
 * ở dungeon_monster_name vì nó có cả chiều rộng màn hình. */
const char* dungeon_monster_name_short[DUNGEON_MONSTER_EYE + 1] = {
	"SLIME",
	"GOBLIN",
	"WOLF",
	"GORILLA",
	"DRAGON",
	"EYE",
};

/* Nhãn nút hành động: tối đa 3 ký tự (18 px) để vừa nút rộng 22 px với lề 2 px
 * mỗi bên. "ITEM" 4 ký tự = 24 px từng bị tràn ra ngoài nút. */
const char* dungeon_action_name[DUNGEON_ACTION_COUNT] = {
	"ATK",
	"ITM",
	"DEF",
	"SKL",
	"ESC",
};

const char* dungeon_item_name[DUNGEON_ITEM_COUNT] = {
	"Sword",
	"Shield",
	"Healing",
	"Bomb",
	"Antidote",
	"Purify",
	"Poison",
};

uint8_t dungeon_clamp_level(uint8_t level) {
	if (level < 1) {
		return 1;
	}
	if (level > 5) {
		return 5;
	}
	return level;
}

int16_t dungeon_max_int16(int16_t a, int16_t b) {
	return (a > b) ? a : b;
}

int16_t dungeon_min_int16(int16_t a, int16_t b) {
	return (a < b) ? a : b;
}

void dungeon_set_message(const char* line_1, const char* line_2, const char* line_3, uint8_t next_state) {
	snprintf(dungeon_runtime.line_1, sizeof(dungeon_runtime.line_1), "%s", (line_1 != 0) ? line_1 : "");
	snprintf(dungeon_runtime.line_2, sizeof(dungeon_runtime.line_2), "%s", (line_2 != 0) ? line_2 : "");
	snprintf(dungeon_runtime.line_3, sizeof(dungeon_runtime.line_3), "%s", (line_3 != 0) ? line_3 : "");
	dungeon_runtime.current_view = DUNGEON_VIEW_MESSAGE;
	dungeon_message_next = next_state;
}

void dungeon_load_message_defaults() {
	snprintf(dungeon_runtime.line_1, sizeof(dungeon_runtime.line_1), "%s", "Forest path ahead");
	snprintf(dungeon_runtime.line_2, sizeof(dungeon_runtime.line_2), "%s", "MODE to continue");
	dungeon_runtime.line_3[0] = '\0';
}
