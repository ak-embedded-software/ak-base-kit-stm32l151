#ifndef __GAME_MONSTER_H__
#define __GAME_MONSTER_H__

#include <stdint.h>
#include <stdbool.h>
#include "game_loneblade.h"

typedef enum {
	MONSTER_TYPE_NORMAL,
	MONSTER_TYPE_ARMORED,
	MONSTER_TYPE_FLYING
} monster_type_t;

typedef enum {
	MONSTER_STATE_WALKING,      
	MONSTER_STATE_PRE_ATTACK,  
	MONSTER_STATE_STUNNED,  
	MONSTER_STATE_KNOCKBACK     
} monster_state_t;

typedef struct {
	int32_t x_scaled;      
	int16_t y;               
	monster_type_t type;
	monster_state_t state;
	uint32_t state_timer;    
	int16_t speed;            
	int8_t dir;              
	bool active;
	uint8_t hp;             
	uint32_t anim_timer;     
	uint8_t anim_frame;
} Monster;

void monster_init();
void monster_update(uint32_t dt);
void monster_draw();
void monster_spawn_trigger();
Monster* monster_get_pool();
uint8_t monster_get_active_count();
void monster_take_damage(Monster* m, uint8_t damage);
uint32_t monster_drain_score();

#endif // __GAME_MONSTER_H__
