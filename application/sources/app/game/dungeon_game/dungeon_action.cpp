/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 04/05/2026
 *          End:   04/05/2026
 ******************************************************************************
**/
#include "dungeon_action.h"
#include "dungeon_runtime.h"

/* Confirm button. Rejected with a buzz when the run is already over. */
static void dungeon_action_shoot() {
	if (dungeon_game_state != GAME_PLAY) {
		BUZZER_PlayTones(tones_3beep);
		return;
	}

	dungeon_confirm_action();
}

/*****************************************************************************/
/*  Turn resolution, the battle phase machine and the per-tick advance.
 */
/*****************************************************************************/
void dungeon_start_battle() {
	dungeon_set_monster_stats(dungeon_runtime.current_monster);
	dungeon_runtime.current_view = DUNGEON_VIEW_BATTLE;
	dungeon_runtime.selected_action = 0;
	dungeon_save_progress();
}

void dungeon_trigger_support() {
	if (dungeon_runtime.support_event > 0) {
		dungeon_pick_chest_options();
		dungeon_runtime.current_view = DUNGEON_VIEW_CHEST;
	}
	else {
		dungeon_set_message("Monster appears", dungeon_monster_name[dungeon_runtime.current_monster], "MODE TO BATTLE", DUNGEON_NEXT_BATTLE);
	}
	dungeon_save_progress();
}

int16_t dungeon_player_damage(uint8_t skill) {
	int16_t damage = dungeon_runtime.player_atk + (skill ? (4 + dungeon_runtime.level * 2) : 0);
	damage -= skill ? (dungeon_runtime.monster_armor / 3) : (dungeon_runtime.monster_armor / 2);
	return dungeon_max_int16(damage, 1);
}

uint8_t dungeon_use_best_item() {
	int16_t heal_amount;

	if ((dungeon_runtime.curse_turns > 0) && (dungeon_runtime.inventory[DUNGEON_ITEM_PURIFY] > 0)) {
		dungeon_runtime.inventory[DUNGEON_ITEM_PURIFY]--;
		dungeon_runtime.curse_turns = 0;
		return 1;
	}

	if (((dungeon_runtime.poison_turns > 0) || (dungeon_runtime.burn_turns > 0)) && (dungeon_runtime.inventory[DUNGEON_ITEM_ANTIDOTE] > 0)) {
		dungeon_runtime.inventory[DUNGEON_ITEM_ANTIDOTE]--;
		dungeon_runtime.poison_turns = 0;
		dungeon_runtime.burn_turns = 0;
		return 1;
	}

	heal_amount = 5 + ((dungeon_runtime.level - 1) * 2);
	if ((dungeon_runtime.inventory[DUNGEON_ITEM_HEAL] > 0) && (dungeon_runtime.player_hp < dungeon_runtime.player_max_hp)) {
		if (dungeon_runtime.curse_turns > 0) {
			heal_amount /= 2;
		}
		dungeon_runtime.inventory[DUNGEON_ITEM_HEAL]--;
		dungeon_runtime.player_hp = dungeon_min_int16(dungeon_runtime.player_hp + heal_amount, dungeon_runtime.player_max_hp);
		return 1;
	}

	if ((dungeon_runtime.inventory[DUNGEON_ITEM_POISON] > 0) && (dungeon_runtime.enemy_poison_turns == 0)) {
		dungeon_runtime.inventory[DUNGEON_ITEM_POISON]--;
		dungeon_runtime.enemy_poison_turns = 3;
		dungeon_runtime.enemy_poison_damage = 5 + ((dungeon_runtime.level - 1) * 2);
		return 1;
	}

	if (dungeon_runtime.inventory[DUNGEON_ITEM_BOMB] > 0) {
		int16_t before_armor = dungeon_runtime.monster_armor;
		dungeon_runtime.inventory[DUNGEON_ITEM_BOMB]--;
		dungeon_runtime.monster_armor -= 3 + ((dungeon_runtime.level - 1) * 2);
		if (dungeon_runtime.monster_armor < 0) {
			dungeon_runtime.monster_armor = 0;
		}
		dungeon_register_monster_armor_loss(before_armor - dungeon_runtime.monster_armor);
		return 1;
	}

	return 0;
}

void dungeon_queue_enemy_turn() {
	dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_MONSTER_ATK_LUNGE;
	dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_WAIT_TICKS;
}

void dungeon_finish_monster_turn() {
	dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_INPUT;
	dungeon_runtime.battle_wait_ticks = 0;
	if (dungeon_runtime.defend_active == 0) {
		dungeon_runtime.defend_icon_active = 0;
	}
	dungeon_runtime.current_view = DUNGEON_VIEW_BATTLE;
	dungeon_save_progress();
}

void dungeon_confirm_action() {
	if (dungeon_game_state != GAME_PLAY) {
		return;
	}

	if (dungeon_runtime.current_view == DUNGEON_VIEW_MESSAGE) {
		if (dungeon_message_next == DUNGEON_NEXT_BATTLE) {
			dungeon_start_battle();
		}
		else if (dungeon_message_next == DUNGEON_NEXT_STAGE) {
			dungeon_advance_stage();
		}
		else if (dungeon_message_next == DUNGEON_NEXT_WIN) {
			dungeon_finish_game(DUNGEON_OUTCOME_WIN);
		}
		else if (dungeon_message_next == DUNGEON_NEXT_LOSE) {
			dungeon_finish_game(DUNGEON_OUTCOME_LOSE);
		}
		else if (dungeon_message_next == DUNGEON_NEXT_TRAVEL) {
			dungeon_runtime.current_view = DUNGEON_VIEW_TRAVEL;
		}
		else if (dungeon_message_next == DUNGEON_NEXT_RETURN) {
			dungeon_runtime.current_view = DUNGEON_VIEW_BATTLE;
		}
		BUZZER_PlayTones(tones_cc);
		return;
	}

	if (dungeon_runtime.current_view == DUNGEON_VIEW_CHEST) {
		dungeon_apply_chest_item(dungeon_runtime.chest_options[dungeon_runtime.selected_support_item]);
		BUZZER_PlayTones(tones_cc);
		return;
	}

	if (dungeon_runtime.current_view != DUNGEON_VIEW_BATTLE) {
		BUZZER_PlayTones(tones_3beep);
		return;
	}

	if (dungeon_runtime.battle_phase != DUNGEON_BATTLE_PHASE_INPUT) {
		BUZZER_PlayTones(tones_3beep);
		return;
	}

	switch (dungeon_runtime.selected_action) {
	case DUNGEON_ACTION_ATTACK:
		dungeon_runtime.defend_icon_active = 0;
		dungeon_runtime.pending_attack_damage = dungeon_player_damage(0);
		dungeon_runtime.pending_attack_hit = 1;
		if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_WOLF) && (dungeon_runtime.monster_dodge_ready != 0)) {
			dungeon_runtime.monster_dodge_ready = 0;
			dungeon_runtime.pending_attack_hit = 0;
			dungeon_runtime.pending_attack_damage = 0;
		}
		dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_HERO_ATK_LUNGE;
		dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_STEP_TICKS;
		dungeon_runtime.monster_shake_ticks = 0;
		dungeon_runtime.player_shake_ticks = 0;
		dungeon_save_progress();
		BUZZER_PlayTones(tones_cc);
		return;

	case DUNGEON_ACTION_ITEM:
		dungeon_runtime.defend_icon_active = 0;
		if (dungeon_use_best_item() == 0) {
			dungeon_set_message("No item ready", "Try another action", "MODE TO RETURN", DUNGEON_NEXT_RETURN);
			BUZZER_PlayTones(tones_3beep);
			return;
		}
		break;

	case DUNGEON_ACTION_DEFEND:
		dungeon_runtime.defend_active = 1;
		dungeon_runtime.defend_icon_active = 1;
		break;

	case DUNGEON_ACTION_SKILL:
		dungeon_runtime.defend_icon_active = 0;
		dungeon_enemy_take_damage(dungeon_player_damage(1));
		break;

	default:
		dungeon_runtime.defend_icon_active = 0;
		if ((dungeon_runtime.current_monster == DUNGEON_MONSTER_DRAGON) || (dungeon_runtime.current_monster == DUNGEON_MONSTER_EYE)) {
			dungeon_set_message("Boss blocks path", "Escape failed", "Enemy turn", DUNGEON_NEXT_NONE);
		}
		else if (((dungeon_runtime.level + dungeon_runtime.stage + dungeon_runtime.battle_turn) % 3) != 0) {
			dungeon_game_score += 5;
			dungeon_set_message("Escape succeeded", "Stage skipped", "MODE NEXT STAGE", DUNGEON_NEXT_STAGE);
			dungeon_save_progress();
			BUZZER_PlayTones(tones_cc);
			return;
		}
		else {
			dungeon_set_message("Escape failed", "Enemy attacks", "", DUNGEON_NEXT_NONE);
		}
		break;
	}

	if (dungeon_runtime.monster_hp <= 0) {
		dungeon_after_battle_win();
		BUZZER_PlayTones(tones_startup);
		return;
	}

	dungeon_queue_enemy_turn();
	dungeon_save_progress();
	BUZZER_PlayTones(tones_cc);
}

void dungeon_tick() {
	if (dungeon_game_state != GAME_PLAY) {
		return;
	}

	/* Popup / shake decay is owned by dungeon_effect (DUNGEON_EFFECT_UPDATE),
	 * which the screen posts immediately before DUNGEON_ACTION_RUN. */

	if (dungeon_runtime.current_view == DUNGEON_VIEW_BATTLE) {
		if (dungeon_runtime.battle_phase != DUNGEON_BATTLE_PHASE_INPUT) {
			if (dungeon_runtime.battle_wait_ticks > 0) {
				dungeon_runtime.battle_wait_ticks--;
			}

			if (dungeon_runtime.battle_wait_ticks == 0) {
				if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_HERO_ATK_LUNGE) {
					dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_HERO_ATK_HIT;
					dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_STEP_TICKS;
					if (dungeon_runtime.pending_attack_hit) {
						dungeon_runtime.monster_shake_ticks = DUNGEON_BATTLE_STEP_TICKS;
					}
					dungeon_save_progress();
				}
				else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_HERO_ATK_HIT) {
					if (dungeon_runtime.pending_attack_hit) {
						dungeon_enemy_take_damage_internal(dungeon_runtime.pending_attack_damage, 0);
					}

					if (dungeon_runtime.monster_hp <= 0) {
						dungeon_after_battle_win();
						BUZZER_PlayTones(tones_startup);
						return;
					}

					dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_HERO_ATK_APPLY;
					dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_STEP_TICKS;
					dungeon_save_progress();
				}
				else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_HERO_ATK_APPLY) {
					dungeon_runtime.pending_attack_damage = 0;
					dungeon_runtime.pending_attack_hit = 0;
					dungeon_queue_enemy_turn();
					dungeon_save_progress();
				}
				else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_LUNGE) {
					dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_MONSTER_ATK_HIT;
					dungeon_runtime.battle_wait_ticks = DUNGEON_BATTLE_STEP_TICKS;
					dungeon_runtime.player_shake_ticks = DUNGEON_BATTLE_STEP_TICKS;
					dungeon_save_progress();
				}
				else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_HIT) {
					/* The monster's turn belongs to dungeon_state: it owns the
					 * per-monster AI and the status effects. Hand it over and
					 * park in RESOLVE until it reports back. */
					dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE;
					task_post_pure_msg(DUNGEON_STATE_ID, DUNGEON_STATE_RUN);
				}
				else if (dungeon_runtime.battle_phase == DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE) {
					/* Waiting on dungeon_state. */
				}
				else {
					dungeon_finish_monster_turn();
				}
			}
		}
		return;
	}

	/* Travel movement is dungeon_lane's job - it is driven by
	 * DUNGEON_LANE_LEVEL_UP. Battle and travel are mutually exclusive views,
	 * so the two tasks never touch the same fields in the same tick. */
}

void dungeon_action_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case DUNGEON_ACTION_SETUP: {
		APP_DBG_SIG("DUNGEON_ACTION_SETUP\n");
		dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_INPUT;
		dungeon_runtime.battle_wait_ticks = 0;
	}
		break;

	case DUNGEON_ACTION_RUN: {
		APP_DBG_SIG("DUNGEON_ACTION_RUN\n");
		dungeon_tick();
	}
		break;

	case DUNGEON_ACTION_SHOOT: {
		APP_DBG_SIG("DUNGEON_ACTION_SHOOT\n");
		dungeon_action_shoot();
	}
		break;

	case DUNGEON_ACTION_RESET: {
		APP_DBG_SIG("DUNGEON_ACTION_RESET\n");
		dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_INPUT;
		dungeon_runtime.battle_wait_ticks = 0;
	}
		break;

	default:
		break;
	}
}
