/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 04/05/2026
 *          End:   04/05/2026
 ******************************************************************************
**/
#include "dungeon_effect.h"
#include "dungeon_runtime.h"

/*****************************************************************************/
/*  Damage popups, shake timers and hit sparks.
 */
/*****************************************************************************/
void dungeon_register_player_damage(int16_t amount) {
	if (amount <= 0) {
		return;
	}

	if (dungeon_runtime.player_hp_popup_ticks > 0) {
		dungeon_runtime.player_hp_popup_value += amount;
	}
	else {
		dungeon_runtime.player_hp_popup_value = amount;
	}
	dungeon_runtime.player_hp_popup_ticks = DUNGEON_POPUP_TICKS;
	if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_INPUT) {
		dungeon_runtime.player_shake_ticks = DUNGEON_SHAKE_TICKS;
	}
}

void dungeon_register_monster_damage(int16_t amount) {
	if (amount <= 0) {
		return;
	}

	if (dungeon_runtime.monster_hp_popup_ticks > 0) {
		dungeon_runtime.monster_hp_popup_value += amount;
	}
	else {
		dungeon_runtime.monster_hp_popup_value = amount;
	}
	dungeon_runtime.monster_hp_popup_ticks = DUNGEON_POPUP_TICKS;
	if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_INPUT) {
		dungeon_runtime.monster_shake_ticks = DUNGEON_SHAKE_TICKS;
	}
}

void dungeon_register_monster_armor_loss(int16_t amount) {
	if (amount <= 0) {
		return;
	}

	if (dungeon_runtime.monster_armor_popup_ticks > 0) {
		dungeon_runtime.monster_armor_popup_value += amount;
	}
	else {
		dungeon_runtime.monster_armor_popup_value = amount;
	}
	dungeon_runtime.monster_armor_popup_ticks = DUNGEON_POPUP_TICKS;
}

void dungeon_update_visual_effects() {
	if (dungeon_runtime.player_hp_popup_ticks > 0) {
		dungeon_runtime.player_hp_popup_ticks--;
	}
	if (dungeon_runtime.monster_hp_popup_ticks > 0) {
		dungeon_runtime.monster_hp_popup_ticks--;
	}
	if (dungeon_runtime.monster_armor_popup_ticks > 0) {
		dungeon_runtime.monster_armor_popup_ticks--;
	}
	if (dungeon_runtime.player_shake_ticks > 0) {
		dungeon_runtime.player_shake_ticks--;
	}
	if (dungeon_runtime.monster_shake_ticks > 0) {
		dungeon_runtime.monster_shake_ticks--;
	}
}

uint8_t dungeon_effect_damage() {
	uint8_t value = 0;
	if (dungeon_runtime.poison_turns > 0) {
		value += 5;
	}
	if (dungeon_runtime.burn_turns > 0) {
		value += 5;
	}
	return value;
}

/* Clear every popup / shake timer this task owns. */
static void dungeon_effect_clear() {
	dungeon_runtime.player_hp_popup_ticks = 0;
	dungeon_runtime.monster_hp_popup_ticks = 0;
	dungeon_runtime.monster_armor_popup_ticks = 0;
	dungeon_runtime.player_shake_ticks = 0;
	dungeon_runtime.monster_shake_ticks = 0;
}

void dungeon_effect_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case DUNGEON_EFFECT_SETUP: {
		APP_DBG_SIG("DUNGEON_EFFECT_SETUP\n");
		dungeon_effect_clear();
	}
		break;

	case DUNGEON_EFFECT_UPDATE: {
		APP_DBG_SIG("DUNGEON_EFFECT_UPDATE\n");
		/* Same GAME_PLAY guard dungeon_tick() used to apply before calling this. */
		if (dungeon_game_state == GAME_PLAY) {
			dungeon_update_visual_effects();
		}
	}
		break;

	case DUNGEON_EFFECT_RESET: {
		APP_DBG_SIG("DUNGEON_EFFECT_RESET\n");
		dungeon_effect_clear();
	}
		break;

	default:
		break;
	}
}
