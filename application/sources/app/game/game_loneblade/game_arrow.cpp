#include "game_arrow.h"
#include "game_player.h"
#include "game_loneblade.h"
#include "screens.h"
#include "game_boss.h"
#include <stdlib.h>

#define MAX_ARROWS 3

static Arrow arrow_pool[MAX_ARROWS];
static int32_t arrow_spawn_timer = 0;

void arrow_init() {
	for (int i = 0; i < MAX_ARROWS; i++) {
		arrow_pool[i].active = false;
	}
	arrow_spawn_timer = 1500 + (rand() % 1500);
}

static void spawn_arrow() {
	int slot = -1;
	for (int i = 0; i < MAX_ARROWS; i++) {
		if (!arrow_pool[i].active) {
			slot = i;
			break;
		}
	}
	if (slot == -1) return;

	Arrow* a = &arrow_pool[slot];
	a->active = true;
	a->y = GAME_GROUND_Y - 15;
	a->speed = 55;

	if (g_difficulty == 2) {
		a->speed = a->speed * 115 / 100;
	}

	if (rand() % 2 == 0) {
		a->x_scaled = -10 * 1000;
		a->dir = 1;
	} else {
		a->x_scaled = 134 * 1000;
		a->dir = -1;
	}
}

void arrow_update(uint32_t dt) {
	if (g_difficulty == 0) return;

	if (arrow_spawn_timer > (int32_t)dt) {
		arrow_spawn_timer -= dt;
	} else {
		arrow_spawn_timer = 0;
	}

	if (arrow_spawn_timer == 0) {
		if (!boss.active) {
			spawn_arrow();
		}
		if (g_difficulty == 2) {
			arrow_spawn_timer = 1500 + (rand() % 1201);
		} else {
			arrow_spawn_timer = 3000 + (rand() % 2001); // 3s to 5s interval for NORMAL
		}
	}

	for (int i = 0; i < MAX_ARROWS; i++) {
		Arrow* a = &arrow_pool[i];
		if (!a->active) continue;

		a->x_scaled += (int32_t)a->dir * a->speed * dt;
		int16_t x = a->x_scaled / 1000;

		if (x < -20 || x > 148) {
			a->active = false;
		} else if (abs(x - 62) <= 4) {
			player_take_damage();
			a->active = false;
		}
	}
}

void arrow_draw() {
	if (g_difficulty == 0) return;

	for (int i = 0; i < MAX_ARROWS; i++) {
		Arrow* a = &arrow_pool[i];
		if (!a->active) continue;

		int16_t x = a->x_scaled / 1000;
		int16_t draw_y = a->y - 4;

		view_render.drawLine(x - 3, draw_y, x + 3, draw_y, WHITE);

		if (a->dir == 1) {
			view_render.drawPixel(x + 2, draw_y - 1, WHITE);
			view_render.drawPixel(x + 2, draw_y + 1, WHITE);
		} else {
			view_render.drawPixel(x - 2, draw_y - 1, WHITE);
			view_render.drawPixel(x - 2, draw_y + 1, WHITE);
		}
	}
}
