#ifndef __GAME_ULT_WAVE_H__
#define __GAME_ULT_WAVE_H__

#include <stdint.h>
#include <stdbool.h>
#include "game_player.h"

typedef struct {
	int32_t x_scaled;
	int16_t y;
	int16_t speed;
	int8_t dir;
	bool active;
	bool hit_flags[MAX_MONSTERS];
	bool hit_boss;
} UltWave;


void ult_wave_init();
void ult_wave_spawn(int16_t start_x, player_dir_t dir);
void ult_wave_update(uint32_t dt);
void ult_wave_draw();

#endif // __GAME_ULT_WAVE_H__
