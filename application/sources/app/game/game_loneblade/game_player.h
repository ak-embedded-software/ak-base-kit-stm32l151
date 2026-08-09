#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

#include <stdint.h>
#include <stdbool.h>
#include "game_loneblade.h"

typedef enum {
	PLAYER_STATE_IDLE,
	PLAYER_STATE_ATTACK1,      
	PLAYER_STATE_ATTACK2,      
	PLAYER_STATE_SHIELD,
	PLAYER_STATE_HURT,
	PLAYER_STATE_DEFEAT
} player_state_t;

typedef enum {
	DIR_LEFT,
	DIR_RIGHT
} player_dir_t;

typedef struct {
	int16_t x;             
	int16_t y;           
	player_state_t state;  
	player_dir_t dir;      
	uint32_t state_timer;  
	uint8_t hp;             
	uint8_t mana;           
	uint32_t ulti_timer;   
} Player;


extern Player hero;

void player_init();
void player_update(uint32_t dt);
void player_draw();

void player_attack(player_dir_t dir);
void player_shield(bool active);
void player_take_damage();
void player_activate_ulti();

#endif // __GAME_PLAYER_H__
