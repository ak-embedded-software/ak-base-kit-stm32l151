#ifndef __GAME_ITEM_H__
#define __GAME_ITEM_H__

#include <stdint.h>

typedef struct {
	bool active;
	int32_t x;
	int32_t y_scaled;
	int32_t speed;
} GameItem;

extern GameItem g_potion;

extern void game_item_init();
extern void game_item_spawn();
extern void game_item_update(uint32_t dt);
extern void game_item_draw();
extern void game_item_check_collect();

#endif //__GAME_ITEM_H__
