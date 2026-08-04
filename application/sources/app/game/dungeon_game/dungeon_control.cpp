/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 05/05/2026
 *          End:   05/05/2026
 ******************************************************************************
**/
#include "dungeon_control.h"
#include "dungeon_runtime.h"

dungeon_control_t dungeon_control;

static void dungeon_control_setup() {
	dungeon_control.action_image = 1;
}

static void dungeon_control_update() {
	if (dungeon_game_state != GAME_PLAY) {
		return;
	}

	dungeon_control.action_image++;
	if (dungeon_control.action_image > 3) {
		dungeon_control.action_image = 1;
	}
}

static void dungeon_control_move_up() {
	if (dungeon_game_state != GAME_PLAY) {
		return;
	}
	dungeon_move_selection(-1);
}

static void dungeon_control_move_down() {
	if (dungeon_game_state != GAME_PLAY) {
		return;
	}
	dungeon_move_selection(1);
}

static void dungeon_control_reset() {
	dungeon_control.action_image = 0;
}

/*****************************************************************************/
/*  Menu selection, chest rolls and item application.
 */
/*****************************************************************************/
void dungeon_pick_chest_options() {
	uint8_t pool[DUNGEON_ITEM_COUNT];
	uint8_t pool_len = 0;
	uint8_t seed = (uint8_t)(dungeon_runtime.level + dungeon_runtime.stage);

	for (uint8_t item = 0; item < DUNGEON_ITEM_COUNT; item++) {
		if ((item == DUNGEON_ITEM_PURIFY) && (dungeon_runtime.level < 5)) {
			continue;
		}
		pool[pool_len++] = item;
	}

	for (uint8_t index = 0; index < 3; index++) {
		dungeon_runtime.chest_options[index] = pool[(seed + (index * 2)) % pool_len];
		for (uint8_t prev = 0; prev < index; prev++) {
			if (dungeon_runtime.chest_options[index] == dungeon_runtime.chest_options[prev]) {
				dungeon_runtime.chest_options[index] = pool[(seed + index + prev + 1) % pool_len];
			}
		}
	}

	dungeon_runtime.selected_support_item = 0;
}

void dungeon_apply_chest_item(uint8_t item) {
	int16_t level_bonus = dungeon_runtime.level - 1;
	switch (item) {
	case DUNGEON_ITEM_SWORD:
		dungeon_runtime.player_atk += 5 + (level_bonus * 3);
		break;
	case DUNGEON_ITEM_SHIELD:
		dungeon_runtime.player_def += 5 + (level_bonus * 4);
		break;
	default:
		dungeon_runtime.inventory[item]++;
		break;
	}

	if (dungeon_runtime.support_event > 0) {
		dungeon_runtime.support_event--;
	}

	dungeon_game_score += 5;
	dungeon_runtime.travel_progress = 0;
	dungeon_runtime.support_pending = 1;
	dungeon_runtime.current_view = DUNGEON_VIEW_TRAVEL;
	dungeon_set_message("You picked item", dungeon_item_name[item], "Keep moving", DUNGEON_NEXT_TRAVEL);
	dungeon_save_progress();
}

void dungeon_move_selection(int8_t delta) {
	if (dungeon_game_state != GAME_PLAY) {
		return;
	}

	if (dungeon_runtime.current_view == DUNGEON_VIEW_CHEST) {
		int16_t next_index = (int16_t)dungeon_runtime.selected_support_item + delta;
		if (next_index < 0) {
			next_index = 0;
		}
		if (next_index > 2) {
			next_index = 2;
		}
		dungeon_runtime.selected_support_item = (uint8_t)next_index;
	}
	else if (dungeon_runtime.current_view == DUNGEON_VIEW_BATTLE) {
		int16_t next_index = (int16_t)dungeon_runtime.selected_action + delta;
		if (next_index < 0) {
			next_index = 0;
		}
		if (next_index >= DUNGEON_ACTION_COUNT) {
			next_index = DUNGEON_ACTION_COUNT - 1;
		}
		dungeon_runtime.selected_action = (uint8_t)next_index;
	}
}

void dungeon_control_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case DUNGEON_CONTROL_SETUP: {
		APP_DBG_SIG("DUNGEON_CONTROL_SETUP\n");
		dungeon_control_setup();
	}
		break;

	case DUNGEON_CONTROL_UPDATE: {
		APP_DBG_SIG("DUNGEON_CONTROL_UPDATE\n");
		dungeon_control_update();
	}
		break;

	case DUNGEON_CONTROL_UP: {
		APP_DBG_SIG("DUNGEON_CONTROL_UP\n");
		dungeon_control_move_up();
	}
		break;

	case DUNGEON_CONTROL_DOWN: {
		APP_DBG_SIG("DUNGEON_CONTROL_DOWN\n");
		dungeon_control_move_down();
	}
		break;

	case DUNGEON_CONTROL_RESET: {
		APP_DBG_SIG("DUNGEON_CONTROL_RESET\n");
		dungeon_control_reset();
	}
		break;

	default:
		break;
	}
}
