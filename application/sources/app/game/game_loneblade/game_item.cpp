#include "game_item.h"
#include "game_player.h"
#include "screens.h"

GameItem g_potion;
static uint32_t item_spawn_timer = 15000; 

void game_item_init() {
	g_potion.active = false;
	g_potion.x = 62;
	g_potion.y_scaled = 0;
	g_potion.speed = 12; 
	item_spawn_timer = 15000;
}

void game_item_spawn() {
	if (!g_potion.active) {
		g_potion.active = true;
		g_potion.y_scaled = 0;
		APP_DBG("[Item] Potion spawned!\n");
	}
}

void game_item_update(uint32_t dt) {
	if (g_potion.active) {
		g_potion.y_scaled += g_potion.speed * dt;
		int16_t y = g_potion.y_scaled / 1000;
		if (y > GAME_GROUND_Y) {
			g_potion.active = false; 
			APP_DBG("[Item] Potion hit ground and shattered.\n");
		}
	} else {
		if (item_spawn_timer > dt) {
			item_spawn_timer -= dt;
		} else {
			item_spawn_timer = 15000;
			game_item_spawn();
		}
	}
}

void game_item_draw() {
	if (!g_potion.active) return;
	int16_t y = g_potion.y_scaled / 1000;

	view_render.drawPixel(g_potion.x - 2, y, WHITE);
	view_render.drawPixel(g_potion.x,     y, WHITE);
	view_render.drawPixel(g_potion.x + 2, y, WHITE);

	view_render.drawPixel(g_potion.x - 2, y + 1, WHITE);
	view_render.drawPixel(g_potion.x - 1, y + 1, WHITE);
	view_render.drawPixel(g_potion.x,     y + 1, WHITE);
	view_render.drawPixel(g_potion.x + 1, y + 1, WHITE);
	view_render.drawPixel(g_potion.x + 2, y + 1, WHITE);

	view_render.drawPixel(g_potion.x - 2, y + 2, WHITE);
	view_render.drawPixel(g_potion.x - 1, y + 2, WHITE);
	view_render.drawPixel(g_potion.x,     y + 2, WHITE);
	view_render.drawPixel(g_potion.x + 1, y + 2, WHITE);
	view_render.drawPixel(g_potion.x + 2, y + 2, WHITE);

	view_render.drawPixel(g_potion.x - 1, y + 3, WHITE);
	view_render.drawPixel(g_potion.x,     y + 3, WHITE);
	view_render.drawPixel(g_potion.x + 1, y + 3, WHITE);

	view_render.drawPixel(g_potion.x,     y + 4, WHITE);
}

void game_item_check_collect() {
	if (!g_potion.active) return;
	int16_t y = g_potion.y_scaled / 1000;

	if ((hero.state == PLAYER_STATE_ATTACK1 || hero.state == PLAYER_STATE_ATTACK2) &&
		(y >= 30 && y <= 52)) {
		g_potion.active = false;
		if (hero.hp < 5) {
			hero.hp++;
		}
		BUZZER_PlaySound(BUZZER_SOUND_TONE_1); 
		APP_DBG("[Item] Potion collected! Player HP: %d\n", hero.hp);
	}
}
