#include "game_player.h"
#include "game_combat.h"
#include "game_ult_wave.h"
#include "screens.h"
#include "button.h"
#include "app_bsp.h"
#include "game_item.h"
#include "game_boss.h"

static uint32_t s_mode_press_duration = 0;
static bool s_ulti_triggered = false;

Player hero;

void player_init() {
	hero.x = 62;      
	hero.y = GAME_GROUND_Y;       
	hero.state = PLAYER_STATE_IDLE;
	hero.dir = DIR_RIGHT;
	hero.state_timer = 0;
	hero.hp = PLAYER_MAX_HP;
	hero.mana = 0;
	hero.ulti_timer = 0;
	s_mode_press_duration = 0;
	s_ulti_triggered = false;
}


void player_attack(player_dir_t dir) {
	if (hero.state == PLAYER_STATE_IDLE) {
		hero.state = PLAYER_STATE_ATTACK1;
		hero.dir = dir;
		hero.state_timer = 250; 
		combat_register_hit(dir, PLAYER_ATTACK_RANGE_1);
	} else if (hero.state == PLAYER_STATE_ATTACK1 && hero.dir == dir) {
		hero.state = PLAYER_STATE_ATTACK2;
		hero.state_timer = 150;
		combat_register_hit(dir, PLAYER_ATTACK_RANGE_2);

		if (hero.ulti_timer > 0) {
			ult_wave_spawn(hero.x, dir);
		}
	}

	game_item_check_collect();
	game_boss_check_parry();
}

void player_shield(bool active) {
	if (active) {
		if (hero.state == PLAYER_STATE_IDLE) {
			hero.state = PLAYER_STATE_SHIELD;
			hero.state_timer = 500; // Block for 500ms
		}
	} else {
		if (hero.state == PLAYER_STATE_SHIELD) {
			hero.state = PLAYER_STATE_IDLE;
			hero.state_timer = 0;
		}
	}
}

void player_take_damage() {
	if (hero.state == PLAYER_STATE_SHIELD) {
		BUZZER_PlaySound(BUZZER_SOUND_CLICK); 
		return;
	}

	if (hero.state != PLAYER_STATE_DEFEAT) {
		hero.state = PLAYER_STATE_HURT;
		hero.state_timer = 300; 
		
		if (hero.hp > 0) {
			hero.hp--;
		}
		
		if (hero.hp == 0) {
			hero.state = PLAYER_STATE_DEFEAT;
			hero.state_timer = 0; 
		}
		
		BUZZER_PlaySound(BUZZER_SOUND_3BEEP);
	}
}

void player_activate_ulti() {
	if (hero.mana >= PLAYER_MAX_MANA && (hero.state == PLAYER_STATE_IDLE || hero.state == PLAYER_STATE_SHIELD)) {
		hero.mana = 0;
		hero.ulti_timer = 6000; 
		hero.state = PLAYER_STATE_IDLE; 
		BUZZER_PlaySound(BUZZER_SOUND_GOODBYE);
	}
}


void player_update(uint32_t dt) {
	if (btn_mode.read() == BUTTON_HW_STATE_PRESSED) {
		if (!s_ulti_triggered && hero.mana >= PLAYER_MAX_MANA) {
			s_mode_press_duration += dt;
			if (s_mode_press_duration >= 250) { 
				player_activate_ulti();
				s_ulti_triggered = true;
			}
		}
	} else {
		s_mode_press_duration = 0;
		s_ulti_triggered = false;
	}

	if (hero.ulti_timer > 0) {
		if (hero.ulti_timer >= dt) {
			hero.ulti_timer -= dt;
		} else {
			hero.ulti_timer = 0;
		}
	}

	if (hero.state_timer > 0) {
		if (hero.state_timer >= dt) {
			hero.state_timer -= dt;
		} else {
			hero.state_timer = 0;
		}

		if (hero.state_timer == 0) {
			if (hero.state == PLAYER_STATE_ATTACK1 || 
				hero.state == PLAYER_STATE_ATTACK2 || 
				hero.state == PLAYER_STATE_HURT ||
				hero.state == PLAYER_STATE_SHIELD) {
				hero.state = PLAYER_STATE_IDLE;
			}
		}
	}
}

void player_draw() {
	const unsigned char* bitmap = bitmap_hero_idle_right;
	uint8_t w = 22;
	uint8_t h = 19;
	int16_t draw_x = hero.x;
	int16_t draw_y = hero.y - 19; 

	if (btn_mode.read() == BUTTON_HW_STATE_PRESSED && hero.mana >= PLAYER_MAX_MANA && !s_ulti_triggered && s_mode_press_duration >= 100) {
		w = 23;
		h = 34;
		draw_y = hero.y - 34;
		if (hero.dir == DIR_LEFT) {
			bitmap = bitmap_hero_ulti_left;
			draw_x = hero.x - 11;
		} else {
			bitmap = bitmap_hero_ulti_right;
			draw_x = hero.x - 11;
		}
	} else {
		switch (hero.state) {
			case PLAYER_STATE_IDLE:
				w = 22;
				if (hero.dir == DIR_LEFT) {
					bitmap = bitmap_hero_idle_left;
					draw_x = hero.x - 11;
				} else {
					bitmap = bitmap_hero_idle_right;
					draw_x = hero.x - 11;
				}
				break;

		case PLAYER_STATE_ATTACK1:
			w = 32;
			if (hero.dir == DIR_LEFT) {
				bitmap = bitmap_hero_attack_left;
				draw_x = hero.x - 20;
			} else {
				bitmap = bitmap_hero_attack_right;
				draw_x = hero.x - 11;
			}
			break;

			case PLAYER_STATE_ATTACK2:
				w = 35;
				if (hero.dir == DIR_LEFT) {
					bitmap = bitmap_hero_attack2_left;
					draw_x = hero.x - 20; 
				} else {
					bitmap = bitmap_hero_attack2_right;
					draw_x = hero.x - 11;
				}
				break;

			case PLAYER_STATE_SHIELD:
				w = 16;
				if (hero.dir == DIR_LEFT) {
					bitmap = bitmap_hero_shield_left;
					draw_x = hero.x - 8;
				} else {
					bitmap = bitmap_hero_shield_right;
					draw_x = hero.x - 8;
				}
				break;

			case PLAYER_STATE_HURT:
				if (hero.state_timer > 150) {
					return; 
				}
				w = 22;
				if (hero.dir == DIR_LEFT) {
					bitmap = bitmap_hero_idle_left;
					draw_x = hero.x - 11;
				} else {
					bitmap = bitmap_hero_idle_right;
					draw_x = hero.x - 11;
				}
				break;

			case PLAYER_STATE_DEFEAT:
				w = 42;
				if (hero.dir == DIR_LEFT) {
					bitmap = bitmap_hero_defeat_left;
					draw_x = hero.x - 21;
				} else {
					bitmap = bitmap_hero_defeat_right;
					draw_x = hero.x - 21;
				}
				break;
		}
	}

	view_render.drawBitmap(draw_x, draw_y, bitmap, w, h, WHITE);
}
