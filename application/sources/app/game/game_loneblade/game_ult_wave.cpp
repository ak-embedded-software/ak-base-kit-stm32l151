#include "game_ult_wave.h"
#include "game_monster.h"
#include "game_boss.h"
#include "game_loneblade.h"
#include "screens.h"
#include <stdlib.h>

#define MAX_WAVES 2

static UltWave wave_pool[MAX_WAVES];

void ult_wave_init() {
	for (int i = 0; i < MAX_WAVES; i++) {
		wave_pool[i].active = false;
	}
}

void ult_wave_spawn(int16_t start_x, player_dir_t dir) {
	int slot = -1;
	for (int i = 0; i < MAX_WAVES; i++) {
		if (!wave_pool[i].active) {
			slot = i;
			break;
		}
	}
	if (slot == -1) return;

	UltWave* w = &wave_pool[slot];
	w->active = true;
	w->x_scaled = (int32_t)start_x * 1000;
	w->y = GAME_GROUND_Y;
	w->speed = 100; // Fast-moving wave
	w->dir = (dir == DIR_LEFT) ? -1 : 1;

	for (int j = 0; j < MAX_MONSTERS; j++) {
		w->hit_flags[j] = false;
	}
	w->hit_boss = false;
	BUZZER_PlaySound(BUZZER_SOUND_LETS_GO);
}

void ult_wave_update(uint32_t dt) {
	for (int i = 0; i < MAX_WAVES; i++) {
		UltWave* w = &wave_pool[i];
		if (!w->active) continue;

		w->x_scaled += (int32_t)w->dir * w->speed * dt;
		int16_t x = w->x_scaled / 1000;

		if (x < -20 || x > 148) {
			w->active = false;
			continue;
		}

		Monster* monsters = monster_get_pool();
		for (int m_idx = 0; m_idx < MAX_MONSTERS; m_idx++) {
			Monster* m = &monsters[m_idx];
			if (!m->active) continue;

			if (w->hit_flags[m_idx]) continue;

			int16_t mx = m->x_scaled / 1000;
			if (abs(x - mx) <= 10) {
				w->hit_flags[m_idx] = true;
				monster_take_damage(m, 2);
			}
		}

		if (boss.active && !w->hit_boss) {
			if (abs(x - boss.x) <= 16) {
				w->hit_boss = true;
				if (boss.hp > 3) {
					boss.hp -= 3;
				} else {
					boss.hp = 0;
					boss.state = BOSS_STATE_DEFEAT;
					boss.state_timer = 1500;
				}
				game_add_score(30); 
				BUZZER_PlaySound(BUZZER_SOUND_BANG);
			}
		}
	}
}

void ult_wave_draw() {
	for (int i = 0; i < MAX_WAVES; i++) {
		UltWave* w = &wave_pool[i];
		if (!w->active) continue;

		int16_t x = w->x_scaled / 1000;
		int16_t draw_x = x - 8;
		int16_t draw_y = w->y - 18; 

		const unsigned char* bitmap = (w->dir == 1) ? bitmap_hero_ult_wave_right : bitmap_hero_ult_wave_left;
		view_render.drawBitmap(draw_x, draw_y, bitmap, 16, 16, WHITE);
	}
}
