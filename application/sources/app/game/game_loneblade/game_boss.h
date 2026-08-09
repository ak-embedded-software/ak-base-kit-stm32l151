#ifndef __GAME_BOSS_H__
#define __GAME_BOSS_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	BOSS_STATE_INACTIVE,
	BOSS_STATE_SPAWNING,
	BOSS_STATE_IDLE,
	BOSS_STATE_CHARGE,
	BOSS_STATE_SLASH,
	BOSS_STATE_FIREBALL,
	BOSS_STATE_RETREAT,
	BOSS_STATE_DEFEAT
} boss_state_t;

typedef struct {
	bool active;
	boss_state_t state;
	int16_t x;
	int16_t y;
	int8_t dir;             // -1: left of player, 1: right of player
	uint8_t hp;
	uint8_t max_hp;
	uint32_t state_timer;
	uint32_t action_timer;  // cycles actions
	uint8_t current_attack; // 0: melee smash, 1: fireball salvo, 2: summon minions
	uint8_t wave;           // 3 for Wave 3, 5 for Wave 5
	bool phase2;            // Enraged phase 2 active
	uint32_t enraged_alert_timer; // Banner alert timer for Phase 2
} GameBoss;

typedef struct {
	bool active;
	int32_t x_scaled;
	int16_t y;
	int32_t speed;
	int8_t dir;
	bool reflected;
} BossFireball;

#define MAX_BOSS_FIREBALLS  3

extern GameBoss boss;
extern BossFireball boss_fireballs[MAX_BOSS_FIREBALLS];

extern void game_boss_init();
extern void game_boss_spawn(uint8_t wave);
extern void game_boss_update(uint32_t dt);
extern void game_boss_draw();
extern void game_boss_check_parry();

#endif //__GAME_BOSS_H__
