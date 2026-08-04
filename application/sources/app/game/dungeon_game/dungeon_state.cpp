/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 03/05/2026
 *          End:   04/05/2026
 ******************************************************************************
**/
#include "dungeon_state.h"
#include "dungeon_runtime.h"

#include "dungeon_effect.h"

/*****************************************************************************/
/*  Monster identity, stats, AI turn selection and status-effect ticking.
 */
/*****************************************************************************/
uint8_t dungeon_monster_for_stage(uint8_t level, uint8_t stage) {
	return dungeon_monster_table[level - 1][stage - 1];
}

void dungeon_set_monster_stats(uint8_t monster) {
	dungeon_runtime.current_monster = monster;
	dungeon_runtime.battle_turn = 1;
	dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_INPUT;
	dungeon_runtime.battle_wait_ticks = 0;
	dungeon_runtime.pre_battle_alert = 0;
	dungeon_runtime.defend_icon_active = 0;
	dungeon_runtime.defend_active = 0;
	dungeon_runtime.enemy_poison_turns = 0;
	dungeon_runtime.enemy_poison_damage = 0;
	dungeon_runtime.monster_dodge_ready = 0;
	dungeon_runtime.pending_attack_damage = 0;
	dungeon_runtime.pending_attack_hit = 0;

	switch (monster) {
	case DUNGEON_MONSTER_SLIME:
		dungeon_runtime.monster_max_hp = 30 + (dungeon_runtime.level - 1) * 3;
		dungeon_runtime.monster_dmg = 5;
		dungeon_runtime.monster_armor = 1;
		break;
	case DUNGEON_MONSTER_GOBLIN:
		dungeon_runtime.monster_max_hp = 50;
		dungeon_runtime.monster_dmg = 10 + (dungeon_runtime.level - 1) * 2;
		dungeon_runtime.monster_armor = 2;
		break;
	case DUNGEON_MONSTER_WOLF:
		dungeon_runtime.monster_max_hp = 70;
		dungeon_runtime.monster_dmg = 30;
		dungeon_runtime.monster_armor = 1;
		break;
	case DUNGEON_MONSTER_GORILLA:
		dungeon_runtime.monster_max_hp = 90;
		dungeon_runtime.monster_dmg = 40;
		dungeon_runtime.monster_armor = 4 + dungeon_runtime.level * 2;
		break;
	case DUNGEON_MONSTER_DRAGON:
		dungeon_runtime.monster_max_hp = 150;
		dungeon_runtime.monster_dmg = 50;
		dungeon_runtime.monster_armor = (dungeon_runtime.level >= 3) ? (dungeon_runtime.level - 2) * 7 : 7;
		break;
	default:
		dungeon_runtime.monster_max_hp = 200;
		dungeon_runtime.monster_dmg = 60;
		dungeon_runtime.monster_armor = 12;
		break;
	}

	dungeon_runtime.monster_hp = dungeon_runtime.monster_max_hp;
	dungeon_runtime.selected_action = 0;
}

void dungeon_enemy_take_damage_internal(int16_t damage, uint8_t trigger_shake) {
	int16_t before_hp = dungeon_runtime.monster_hp;
	dungeon_runtime.monster_hp -= damage;
	if (dungeon_runtime.monster_hp < 0) {
		dungeon_runtime.monster_hp = 0;
	}
	dungeon_register_monster_damage(before_hp - dungeon_runtime.monster_hp);
	if (trigger_shake && (before_hp > dungeon_runtime.monster_hp)) {
		dungeon_runtime.monster_shake_ticks = DUNGEON_SHAKE_TICKS;
	}
}

void dungeon_enemy_take_damage(int16_t damage) {
	dungeon_enemy_take_damage_internal(damage, 1);
}

uint8_t dungeon_turn_matches(uint8_t turn, uint8_t first, uint8_t step) {
	return ((turn >= first) && (((turn - first) % step) == 0));
}

void dungeon_enemy_action() {
	int16_t hp_before = dungeon_runtime.player_hp;
	int16_t damage = dungeon_runtime.monster_dmg - (dungeon_runtime.player_def / 2);
	if (dungeon_runtime.defend_active) {
		damage /= 2;
		dungeon_runtime.defend_active = 0;
	}
	damage = dungeon_max_int16(damage, 1);

	if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_SLIME) && ((dungeon_runtime.battle_turn % 2) == 0)) {
		dungeon_runtime.monster_hp = dungeon_min_int16(dungeon_runtime.monster_hp + 5, dungeon_runtime.monster_max_hp);
	}
	else if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_GOBLIN) && dungeon_turn_matches(dungeon_runtime.battle_turn, 2, 3)) {
		dungeon_runtime.poison_turns = 3;
	}
	else if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_WOLF) && dungeon_turn_matches(dungeon_runtime.battle_turn, 3, 4)) {
		dungeon_runtime.monster_dodge_ready = 1;
	}
	else if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_GORILLA) && dungeon_turn_matches(dungeon_runtime.battle_turn, 2, 3)) {
		dungeon_runtime.monster_armor += 5;
	}
	else if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_DRAGON) && dungeon_turn_matches(dungeon_runtime.battle_turn, 3, 4)) {
		dungeon_runtime.player_hp -= 10;
		dungeon_runtime.burn_turns = 3;
	}
	else if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_EYE) && dungeon_turn_matches(dungeon_runtime.battle_turn, 3, 3)) {
		dungeon_runtime.curse_turns = 3;
	}

	dungeon_runtime.player_hp -= damage;
	if (dungeon_runtime.player_hp < 0) {
		dungeon_runtime.player_hp = 0;
	}
	dungeon_register_player_damage(hp_before - dungeon_runtime.player_hp);
}

void dungeon_status_tick() {
	int16_t player_before = dungeon_runtime.player_hp;
	int16_t monster_before = dungeon_runtime.monster_hp;

	if (dungeon_runtime.poison_turns > 0) {
		dungeon_runtime.player_hp -= 5;
		dungeon_runtime.poison_turns--;
	}
	if (dungeon_runtime.burn_turns > 0) {
		dungeon_runtime.player_hp -= 5;
		dungeon_runtime.burn_turns--;
	}
	if (dungeon_runtime.curse_turns > 0) {
		dungeon_runtime.curse_turns--;
	}
	if (dungeon_runtime.enemy_poison_turns > 0) {
		dungeon_runtime.monster_hp -= dungeon_runtime.enemy_poison_damage;
		dungeon_runtime.enemy_poison_turns--;
	}

	if (dungeon_runtime.player_hp < 0) {
		dungeon_runtime.player_hp = 0;
	}
	if (dungeon_runtime.monster_hp < 0) {
		dungeon_runtime.monster_hp = 0;
	}

	dungeon_register_player_damage(player_before - dungeon_runtime.player_hp);
	dungeon_register_monster_damage(monster_before - dungeon_runtime.monster_hp);
}

/* One full monster turn: AI action, status effects, then the outcome.
 *
 * Requested by dungeon_action via DUNGEON_STATE_RUN once the monster's attack
 * animation reaches its hit frame. Advancing the battle phase from here is what
 * releases dungeon_action from DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE. */
static void dungeon_monster_turn() {
	dungeon_enemy_action();
	dungeon_status_tick();
	dungeon_runtime.battle_turn++;

	if (dungeon_runtime.monster_hp <= 0) {
		/* Poison finished the monster off during its own turn. */
		dungeon_after_battle_win();
		BUZZER_PlayTones(tones_startup);
		return;
	}

	if (dungeon_runtime.player_hp <= 0) {
		/* dungeon_lane owns the run lifecycle, so it decides how a run ends. */
		task_post_pure_msg(DUNGEON_LANE_ID, DUNGEON_LANE_CHECK_GAME_OVER);
		return;
	}

	dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_MONSTER_ATK_APPLY;
	dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_STEP_TICKS;
	dungeon_save_progress();
}

void dungeon_state_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case DUNGEON_STATE_SETUP: {
		APP_DBG_SIG("DUNGEON_STATE_SETUP\n");
		/* Deliberately does NOT call dungeon_set_monster_stats(): the monster is
		 * established either by dungeon_prepare_stage() for a fresh stage or by
		 * dungeon_restore_save() when continuing, and re-rolling it here would
		 * heal a wounded monster back to full HP on every continue. */
	}
		break;

	case DUNGEON_STATE_RUN: {
		APP_DBG_SIG("DUNGEON_STATE_RUN\n");
		dungeon_monster_turn();
	}
		break;

	case DUNGEON_STATE_DETONATOR: {
	}
		break;

	case DUNGEON_STATE_RESET: {
		APP_DBG_SIG("DUNGEON_STATE_RESET\n");
	}
		break;

	default:
		break;
	}
}
