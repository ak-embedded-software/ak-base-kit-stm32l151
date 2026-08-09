#ifndef __GAME_LONEBLADE_H__
#define __GAME_LONEBLADE_H__

#include <stdint.h>

// Game configurations
#define GAME_GROUND_Y          53
#define GAME_SCREEN_WIDTH      128
#define GAME_SCREEN_HEIGHT     64

#define PLAYER_MAX_HP          (5)
#define PLAYER_MAX_MANA        (100)
#define PLAYER_ATTACK_RANGE_1  (25)
#define PLAYER_ATTACK_RANGE_2  (25)

#define MAX_MONSTERS           (5)
#define MONSTER_ATTACK_RANGE   (20)
#define MONSTER_QUEUE_DISTANCE (15)

// Game controller API
void game_loneblade_init();
void game_loneblade_update(uint32_t dt);
void game_loneblade_draw();

uint32_t game_get_score();
uint32_t game_get_wave();
uint32_t game_get_high_score();
void game_add_score(uint32_t score);
void game_update_high_score();
void game_set_keep_score_flag(bool keep);

#endif // __GAME_LONEBLADE_H__
