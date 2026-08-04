/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @brief:  Shared runtime state for the dungeon game tasks.
 *
 * dungeon_runtime_t is the single source of truth for one run. It used to live
 * inside scr_dungeon_game.cpp, which meant the screen owned the game rules and
 * the five game tasks were empty shells. The struct now lives here so each
 * task can own its slice of the rules and the screen is left as a pure view.
 *
 * Ownership of the fields, by task:
 *   dungeon_state    monster identity, stats, AI turns, status effects
 *   dungeon_lane     level / stage / travel progression, persistence, score
 *   dungeon_control  menu selection, chest rolls, item application
 *   dungeon_action   turn resolution, battle phase machine, per-tick advance
 *   dungeon_effect   damage popups, shake timers, hit sparks
 *
 * Nothing here is protected by a lock: AK dispatches one task at a time on a
 * single stack, so tasks never preempt each other.
 ******************************************************************************
**/
#ifndef __DUNGEON_RUNTIME_H__
#define __DUNGEON_RUNTIME_H__

#include <stdint.h>
#include <stdbool.h>

#include "app_eeprom.h"

enum {
	DUNGEON_VIEW_TRAVEL = 0,
	DUNGEON_VIEW_CHEST,
	DUNGEON_VIEW_MESSAGE,
	DUNGEON_VIEW_BATTLE,
};

enum {
	DUNGEON_SUPPORT_NONE = 0,
	DUNGEON_SUPPORT_CHEST,
	DUNGEON_SUPPORT_TRAP,
	DUNGEON_SUPPORT_SHRINE,
};

enum {
	DUNGEON_ACTION_ATTACK = 0,
	DUNGEON_ACTION_ITEM,
	DUNGEON_ACTION_DEFEND,
	DUNGEON_ACTION_SKILL,
	DUNGEON_ACTION_ESCAPE,
	DUNGEON_ACTION_COUNT,
};

enum {
	DUNGEON_MONSTER_SLIME = 0,
	DUNGEON_MONSTER_GOBLIN,
	DUNGEON_MONSTER_WOLF,
	DUNGEON_MONSTER_GORILLA,
	DUNGEON_MONSTER_DRAGON,
	DUNGEON_MONSTER_EYE,
};

enum {
	DUNGEON_ITEM_SWORD = 0,
	DUNGEON_ITEM_SHIELD,
	DUNGEON_ITEM_HEAL,
	DUNGEON_ITEM_BOMB,
	DUNGEON_ITEM_ANTIDOTE,
	DUNGEON_ITEM_PURIFY,
	DUNGEON_ITEM_POISON,
	DUNGEON_ITEM_COUNT,
};

enum {
	DUNGEON_NEXT_NONE = 0,
	DUNGEON_NEXT_BATTLE,
	DUNGEON_NEXT_STAGE,
	DUNGEON_NEXT_WIN,
	DUNGEON_NEXT_LOSE,
	DUNGEON_NEXT_TRAVEL,
	DUNGEON_NEXT_RETURN,
};

enum {
	DUNGEON_BATTLE_PHASE_INPUT = 0,
	DUNGEON_BATTLE_PHASE_HERO_ATK_LUNGE,
	DUNGEON_BATTLE_PHASE_HERO_ATK_HIT,
	DUNGEON_BATTLE_PHASE_HERO_ATK_APPLY,
	DUNGEON_BATTLE_PHASE_MONSTER_ATK_LUNGE,
	DUNGEON_BATTLE_PHASE_MONSTER_ATK_HIT,
	DUNGEON_BATTLE_PHASE_MONSTER_ATK_APPLY,
	/* Appended, not inserted: keep the numbering of the phases above stable.
	 *
	 * Entered when dungeon_action has asked dungeon_state to resolve the
	 * monster's turn and is waiting for the reply. dungeon_tick() deliberately
	 * does nothing in this phase, so a tick arriving before DUNGEON_STATE_RUN is
	 * dispatched cannot request the same turn twice. */
	DUNGEON_BATTLE_PHASE_MONSTER_ATK_RESOLVE,
};

#define DUNGEON_BATTLE_WAIT_TICKS	(10)
#define DUNGEON_BATTLE_STEP_TICKS	(5)
#define DUNGEON_SHAKE_TICKS		(6)
#define DUNGEON_POPUP_TICKS		(24)

typedef struct {
	const uint8_t* data;
	uint8_t width;
	uint8_t height;
} dungeon_bitmap_t;

/*
 * Runtime snapshot of one dungeon run.
 * This struct is the main source of truth for battle, travel and UI state.
 */
typedef struct {
	uint8_t level;
	uint8_t stage;
	uint8_t total_stages;
	uint8_t current_view;               /* Travel / Chest / Message / Battle */
	uint8_t current_monster;            /* Monster enum for current stage */
	uint8_t support_event;              /* Remaining chest events before battle */
	uint8_t support_pending;            /* Gate to trigger chest/battle at travel end */
	uint8_t battle_turn;                /* Turn counter for monster behavior patterns */
	uint8_t battle_phase;               /* Fine-grained battle animation phase */
	uint8_t battle_wait_ticks;          /* Delay ticks for current battle phase */
	uint8_t pre_battle_alert;           /* Show monster in travel before entering battle */
	uint8_t defend_icon_active;         /* Shield icon UI flag */
	uint8_t travel_progress;            /* 0..100 travel progress across forest lane */
	uint8_t selected_action;            /* Selected action button index in battle */
	uint8_t selected_support_item;      /* Selected chest option index */
	uint8_t defend_active;              /* Defend status that halves next incoming hit */
	uint8_t poison_turns;               /* Hero poison turns remaining */
	uint8_t burn_turns;                 /* Hero burn turns remaining */
	uint8_t curse_turns;                /* Hero curse turns remaining */
	uint8_t enemy_poison_turns;         /* Monster poison turns remaining */
	uint8_t enemy_poison_damage;        /* Poison damage applied to monster each turn */
	uint8_t monster_dodge_ready;        /* Wolf special dodge flag */
	uint8_t inventory[DUNGEON_ITEM_COUNT]; /* Item counts */
	uint8_t chest_options[3];           /* Rolled item IDs in current chest */
	int16_t player_hp;
	int16_t player_max_hp;
	int16_t player_atk;
	int16_t player_def;
	int16_t monster_hp;
	int16_t monster_max_hp;
	int16_t monster_dmg;
	int16_t monster_armor;
	int16_t pending_attack_damage;      /* Deferred ATK damage applied in APPLY phase */
	uint8_t pending_attack_hit;         /* 0 when dodged/missed, 1 when hit should apply */
	int16_t player_hp_popup_value;      /* Floating text value for hero damage */
	int16_t monster_hp_popup_value;     /* Floating text value for monster damage */
	int16_t monster_armor_popup_value;  /* Floating text value for armor loss */
	uint8_t player_hp_popup_ticks;      /* Popup lifespan */
	uint8_t monster_hp_popup_ticks;     /* Popup lifespan */
	uint8_t monster_armor_popup_ticks;  /* Popup lifespan */
	uint8_t player_shake_ticks;         /* Shake duration */
	uint8_t monster_shake_ticks;        /* Shake duration */
	char line_1[22];                    /* Message view line 1 */
	char line_2[22];                    /* Message view line 2 */
	char line_3[22];                    /* Message view line 3 */
} dungeon_runtime_t;

extern dungeon_runtime_t dungeon_runtime;
extern uint8_t dungeon_message_next;

/* Owned by scr_dungeon_game.cpp: creator mode plays without touching EEPROM. */
extern uint8_t dungeon_persist_enabled;
extern void dungeon_load_setting();

extern const uint8_t dungeon_stage_counts[5];
extern const uint8_t dungeon_monster_table[5][8];
extern const char* dungeon_monster_name[DUNGEON_MONSTER_EYE + 1];
extern const char* dungeon_monster_name_short[DUNGEON_MONSTER_EYE + 1];
extern const char* dungeon_action_name[DUNGEON_ACTION_COUNT];
extern const char* dungeon_item_name[DUNGEON_ITEM_COUNT];

extern uint8_t dungeon_clamp_level(uint8_t level);
extern int16_t dungeon_max_int16(int16_t a, int16_t b);
extern int16_t dungeon_min_int16(int16_t a, int16_t b);
extern void dungeon_set_message(const char* line_1, const char* line_2, const char* line_3, uint8_t next_state);
extern void dungeon_load_message_defaults();

/* --- dungeon_state --- */
extern uint8_t dungeon_monster_for_stage(uint8_t level, uint8_t stage);
extern void dungeon_set_monster_stats(uint8_t monster);
extern void dungeon_enemy_take_damage_internal(int16_t damage, uint8_t trigger_shake);
extern void dungeon_enemy_take_damage(int16_t damage);
extern uint8_t dungeon_turn_matches(uint8_t turn, uint8_t first, uint8_t step);
extern void dungeon_enemy_action();
extern void dungeon_status_tick();

/* --- dungeon_lane --- */
extern void dungeon_advance_travel();
extern void dungeon_check_game_over();
extern void dungeon_init_player(uint8_t level);
extern void dungeon_update_best_progress();
extern void dungeon_save_progress();
extern void dungeon_clear_save();
extern uint8_t dungeon_has_save_data();
extern uint8_t dungeon_restore_save();
extern void dungeon_prepare_continue();
extern void dungeon_prepare_new_game();
extern void dungeon_prepare_level(uint8_t level);
extern void dungeon_prepare_creator_mode(uint8_t level);
extern uint8_t dungeon_is_creator_mode();
extern void dungeon_prepare_stage();
extern void dungeon_after_battle_win();
extern void dungeon_finish_game(uint8_t outcome);
extern void dungeon_advance_stage();
extern void dungeon_setup_session();
extern void dungeon_reset_session();
extern uint8_t dungeon_get_current_stage();
extern uint8_t dungeon_get_total_stages();
extern uint8_t dungeon_get_level_value();
extern uint32_t dungeon_get_score_value();
extern void dungeon_save_and_reset_score();
extern void dungeon_reset_objects();

/* --- dungeon_control --- */
extern void dungeon_pick_chest_options();
extern void dungeon_apply_chest_item(uint8_t item);
extern void dungeon_move_selection(int8_t delta);

/* --- dungeon_action --- */
extern void dungeon_start_battle();
extern void dungeon_trigger_support();
extern int16_t dungeon_player_damage(uint8_t skill);
extern uint8_t dungeon_use_best_item();
extern void dungeon_queue_enemy_turn();
extern void dungeon_finish_monster_turn();
extern void dungeon_confirm_action();
extern void dungeon_tick();

/* --- dungeon_effect --- */
extern void dungeon_register_player_damage(int16_t amount);
extern void dungeon_register_monster_damage(int16_t amount);
extern void dungeon_register_monster_armor_loss(int16_t amount);
extern void dungeon_update_visual_effects();
extern uint8_t dungeon_effect_damage();

#endif //__DUNGEON_RUNTIME_H__
