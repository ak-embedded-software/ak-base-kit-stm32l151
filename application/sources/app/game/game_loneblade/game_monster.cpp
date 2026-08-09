#include "game_monster.h"
#include "game_player.h"
#include "screens.h"
#include "game_boss.h"
#include <stdlib.h>

static Monster monster_pool[MAX_MONSTERS];
static int32_t spawn_timer;
static uint32_t s_pending_score = 0;

uint32_t monster_drain_score() {
	uint32_t s = s_pending_score;
	s_pending_score = 0;
	return s;
}

// Helper to spawn a new monster
void spawn_monster() {
	int slot = -1;
	for (int i = 0; i < MAX_MONSTERS; i++) {
		if (!monster_pool[i].active) {
			slot = i;
			break;
		}
	}
	if (slot == -1) return;

	Monster* m = &monster_pool[slot];
	m->active = true;
	m->state = MONSTER_STATE_WALKING;
	m->state_timer = 0;
	m->anim_timer = 0;
	m->anim_frame = 0;

	if (rand() % 2 == 0) {
		m->x_scaled = -10 * 1000;
		m->dir = 1; 
	} else {
		m->x_scaled = 134 * 1000;
		m->dir = -1; 
	}

	// Randomly choose type: NORMAL (60%), ARMORED (25%), FLYING (15%)
	int r = rand() % 100;
	if (r < 60) {
		m->type = MONSTER_TYPE_NORMAL;
		m->hp = 3;
		m->speed = 36;
		m->y = GAME_GROUND_Y;
	} else if (r < 85) {
		m->type = MONSTER_TYPE_ARMORED;
		m->hp = 5;
		m->speed = 30; 
		m->y = GAME_GROUND_Y;
	} else {
		m->type = MONSTER_TYPE_FLYING;
		m->hp = 1;
		m->speed = 40; 
		m->y = GAME_GROUND_Y - 15; 
	}

	// Adjust speed by difficulty
	if (g_difficulty == 0) { // EASY
		m->speed = m->speed * 7 / 10;
	} else if (g_difficulty == 1) { // NORMAL 
		m->speed = m->speed * 85 / 100;
	} else if (g_difficulty == 2) { // HARD
		m->speed = m->speed * 115 / 100;
	}
}



void monster_init() {
	for (int i = 0; i < MAX_MONSTERS; i++) {
		monster_pool[i].active = false;
		monster_pool[i].x_scaled = 0;
		monster_pool[i].y = GAME_GROUND_Y;
		monster_pool[i].state = MONSTER_STATE_WALKING;
		monster_pool[i].state_timer = 0;
		monster_pool[i].hp = 0;
		monster_pool[i].speed = 0;
		monster_pool[i].dir = 1;
		monster_pool[i].anim_timer = 0;
		monster_pool[i].anim_frame = 0;
	}
	spawn_timer = 1200; 
}

void monster_spawn_trigger() {
	spawn_monster();
}

Monster* monster_get_pool() {
	return monster_pool;
}

uint8_t monster_get_active_count() {
	uint8_t count = 0;
	for (int i = 0; i < MAX_MONSTERS; i++) {
		if (monster_pool[i].active) count++;
	}
	return count;
}

void monster_update(uint32_t dt) {
	if (spawn_timer > (int32_t)dt) {
		spawn_timer -= dt;
	} else {
		spawn_timer = 0;
	}

	if (spawn_timer == 0) {
		uint8_t active_count = 0;
		for (int i = 0; i < MAX_MONSTERS; i++) {
			if (monster_pool[i].active) active_count++;
		}
		if (active_count < MAX_MONSTERS && !boss.active) {
			spawn_monster();
		}
		if (g_difficulty == 0) { // EASY
			spawn_timer = 2000 + (rand() % 2001); // 2s to 4s
		} else if (g_difficulty == 1) { // NORMAL
			spawn_timer = 1600 + (rand() % 1601); // 1.6s to 3.2s
		} else if (g_difficulty == 2) { // HARD
			spawn_timer = 900 + (rand() % 1001);  // 0.9s to 1.9s
		}
	}

	// 2. FSM Update for each monster
	for (int i = 0; i < MAX_MONSTERS; i++) {
		Monster* m = &monster_pool[i];
		if (!m->active) continue;

		// Update state timer
		if (m->state_timer > dt) {
			m->state_timer -= dt;
		} else {
			m->state_timer = 0;
		}

		// Update animation frame timer
		m->anim_timer += dt;
		if (m->anim_timer >= 200) {
			m->anim_timer = 0;
			m->anim_frame = !m->anim_frame;
		}

		int16_t x = m->x_scaled / 1000;

		switch (m->state) {
			case MONSTER_STATE_WALKING: {
				if (m->state_timer > 0) {
					break;
				}

				int16_t dist_to_hero = (m->dir == 1) ? (62 - x) : (x - 62);
				if (dist_to_hero <= MONSTER_ATTACK_RANGE) {
					m->state = MONSTER_STATE_PRE_ATTACK;
					m->state_timer = 350; 
				} else {
					m->x_scaled += (int32_t)m->dir * m->speed * dt;
				}
				break;
			}

			case MONSTER_STATE_PRE_ATTACK: {
				if (m->state_timer == 0) {
					player_take_damage();

					m->state = MONSTER_STATE_WALKING;
					m->state_timer = 800; 
				}
				break;
			}

			case MONSTER_STATE_STUNNED: {
				if (m->state_timer == 0) {
					m->state = MONSTER_STATE_WALKING;
					m->state_timer = 200; 
				}
				break;
			}

			case MONSTER_STATE_KNOCKBACK: {
				int32_t kb_speed = 60; 
				m->x_scaled -= (int32_t)m->dir * kb_speed * dt;

				if (m->state_timer == 0) {
					m->state = MONSTER_STATE_WALKING;
					m->state_timer = 200; 
				}
				break;
			}
		}
	}
}

void monster_draw() {
	for (int i = 0; i < MAX_MONSTERS; i++) {
		Monster* m = &monster_pool[i];
		if (!m->active) continue;

		int16_t x = m->x_scaled / 1000;
		int16_t y = m->y;

		switch (m->type) {
			case MONSTER_TYPE_NORMAL: {
				int16_t draw_x = x - 6;
				int16_t draw_y = y - 14;
				uint8_t draw_w = 12;
				uint8_t draw_h = 14;

				const unsigned char* bitmap = bitmap_monster_normal_walk1_left;
				if (m->dir == 1) { // Facing right
					if (m->state == MONSTER_STATE_WALKING) {
						bitmap = m->anim_frame ? bitmap_monster_normal_walk1_right : bitmap_monster_normal_walk2_right;
					} else if (m->state == MONSTER_STATE_PRE_ATTACK) {
						if (m->state_timer > 150) {
							bitmap = bitmap_monster_normal_attack_windup_right;
						} else {
							bitmap = bitmap_monster_normal_attack_strike_right;
							draw_w = 16;
						}
					} else if (m->state == MONSTER_STATE_STUNNED || m->state == MONSTER_STATE_KNOCKBACK) {
						bitmap = bitmap_monster_normal_hurt_right;
					}
				} else { // Facing left
					if (m->state == MONSTER_STATE_WALKING) {
						bitmap = m->anim_frame ? bitmap_monster_normal_walk1_left : bitmap_monster_normal_walk2_left;
					} else if (m->state == MONSTER_STATE_PRE_ATTACK) {
						if (m->state_timer > 150) {
							bitmap = bitmap_monster_normal_attack_windup_left;
						} else {
							bitmap = bitmap_monster_normal_attack_strike_left;
							draw_w = 16;
							draw_x = x - 10;
						}
					} else if (m->state == MONSTER_STATE_STUNNED || m->state == MONSTER_STATE_KNOCKBACK) {
						bitmap = bitmap_monster_normal_hurt_left;
					}
				}

				view_render.drawBitmap(draw_x, draw_y, bitmap, draw_w, draw_h, WHITE);

				if (m->state == MONSTER_STATE_STUNNED) {
					view_render.drawPixel(x - 2, draw_y - 2, WHITE);
					view_render.drawPixel(x + 2, draw_y - 2, WHITE);
				}
				break;
			}

			case MONSTER_TYPE_ARMORED: {
				int16_t draw_x = x - 8;
				int16_t draw_y = y - 19;
				uint8_t draw_w = 16;
				uint8_t draw_h = 19;

				const unsigned char* bitmap = bitmap_monster_armored_walk1_left;
				if (m->dir == 1) { // Facing right
					if (m->state == MONSTER_STATE_WALKING) {
						bitmap = m->anim_frame ? bitmap_monster_armored_walk1_right : bitmap_monster_armored_walk2_right;
					} else if (m->state == MONSTER_STATE_PRE_ATTACK) {
						if (m->state_timer > 150) {
							bitmap = bitmap_monster_armored_attack_windup_right;
						} else {
							bitmap = bitmap_monster_armored_attack_strike_right;
							draw_w = 24;
						}
					} else if (m->state == MONSTER_STATE_STUNNED || m->state == MONSTER_STATE_KNOCKBACK) {
						bitmap = bitmap_monster_armored_hurt_right;
					}
				} else { // Facing left
					if (m->state == MONSTER_STATE_WALKING) {
						bitmap = m->anim_frame ? bitmap_monster_armored_walk1_left : bitmap_monster_armored_walk2_left;
					} else if (m->state == MONSTER_STATE_PRE_ATTACK) {
						if (m->state_timer > 150) {
							bitmap = bitmap_monster_armored_attack_windup_left;
						} else {
							bitmap = bitmap_monster_armored_attack_strike_left;
							draw_w = 24;
							draw_x = x - 16;
						}
					} else if (m->state == MONSTER_STATE_STUNNED || m->state == MONSTER_STATE_KNOCKBACK) {
						bitmap = bitmap_monster_armored_hurt_left;
					}
				}

				view_render.drawBitmap(draw_x, draw_y, bitmap, draw_w, draw_h, WHITE);

				if (m->hp < 2) {
					int16_t body_left = x - 8;
					view_render.drawLine(body_left + 3, draw_y + 3, body_left + 13, draw_y + 16, BLACK);
					view_render.drawLine(body_left + 13, draw_y + 3, body_left + 3, draw_y + 16, BLACK);
				}

				if (m->state == MONSTER_STATE_STUNNED) {
					view_render.drawPixel(x - 3, draw_y - 2, WHITE);
					view_render.drawPixel(x + 3, draw_y - 2, WHITE);
				}
				break;
			}

			case MONSTER_TYPE_FLYING: {
				int16_t draw_x = x - 6;
				int16_t draw_y = y - 8;

				const unsigned char* bitmap = m->anim_frame ? bitmap_monster_fly_frame1 : bitmap_monster_fly_frame2;
				view_render.drawBitmap(draw_x, draw_y, bitmap, 12, 8, WHITE);

				if (m->state == MONSTER_STATE_STUNNED) {
					view_render.drawPixel(x - 2, draw_y - 2, WHITE);
					view_render.drawPixel(x + 2, draw_y - 2, WHITE);
				}
				break;
			}
		}
	}
}

void monster_take_damage(Monster* m, uint8_t damage) {
	if (!m || !m->active) return;

	if (m->hp > damage) {
		m->hp -= damage;
		if (rand() % 2 == 0) {
			m->state = MONSTER_STATE_STUNNED;
			m->state_timer = 400;
		} else {
			m->state = MONSTER_STATE_KNOCKBACK;
			m->state_timer = 200;
		}
		BUZZER_PlaySound(BUZZER_SOUND_CLICK);
	} else {
		m->hp = 0;
		m->active = false;
		if (m->type == MONSTER_TYPE_FLYING)       s_pending_score += 5;
		else if (m->type == MONSTER_TYPE_ARMORED) s_pending_score += 20;
		else                                       s_pending_score += 10;

		if (hero.ulti_timer == 0) {
			hero.mana += 15;
			if (hero.mana > PLAYER_MAX_MANA) {
				hero.mana = PLAYER_MAX_MANA;
			}
		}
		BUZZER_PlaySound(BUZZER_SOUND_BANG);
	}
}
