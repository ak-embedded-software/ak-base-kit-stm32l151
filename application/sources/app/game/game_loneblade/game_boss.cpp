#include "game_boss.h"
#include "game_player.h"
#include "game_item.h"
#include "screens.h"

GameBoss boss;
BossFireball boss_fireballs[MAX_BOSS_FIREBALLS];

extern void spawn_monster();

static uint32_t fireball_timer = 0;
static uint32_t flash_timer = 0;
static bool flash_state = false;

void game_boss_init() {
	boss.active = false;
	boss.state = BOSS_STATE_INACTIVE;
	for (int i = 0; i < MAX_BOSS_FIREBALLS; i++) {
		boss_fireballs[i].active = false;
	}
}

void game_boss_spawn(uint8_t wave) {
	if (boss.active) return;

	boss.active = true;
	boss.state = BOSS_STATE_SPAWNING;
	boss.wave = wave;
	boss.phase2 = false;
	boss.enraged_alert_timer = 0;
	boss.hp = (wave == 5) ? 40 : 25; // Wave 3: 25 HP, Wave 5: 40 HP
	boss.max_hp = boss.hp;
	boss.state_timer = 2500;
	boss.action_timer = 0;
	boss.dir = (rand() % 2 == 0) ? -1 : 1;
	boss.current_attack = 0;

	if (boss.dir == -1) {
		boss.x = -25;
	} else {
		boss.x = 145;
	}
	boss.y = GAME_GROUND_Y;

	for (int i = 0; i < MAX_BOSS_FIREBALLS; i++) {
		boss_fireballs[i].active = false;
	}
}

static void boss_fireball_spawn() {
	int slot = -1;
	for (int i = 0; i < MAX_BOSS_FIREBALLS; i++) {
		if (!boss_fireballs[i].active) {
			slot = i;
			break;
		}
	}
	if (slot == -1) return;

	BossFireball* bf = &boss_fireballs[slot];
	bf->active = true;
	bf->y = GAME_GROUND_Y - 12;
	bf->speed = 35;
	bf->reflected = false;

	if (boss.dir == -1) {
		bf->x_scaled = 10 * 1000;
		bf->dir = 1;
	} else {
		bf->x_scaled = 114 * 1000;
		bf->dir = -1;
	}
	BUZZER_PlaySound(BUZZER_SOUND_TONE_5);
}

void game_boss_update(uint32_t dt) {
	if (!boss.active) return;

	// Update fireballs
	for (int i = 0; i < MAX_BOSS_FIREBALLS; i++) {
		BossFireball* bf = &boss_fireballs[i];
		if (!bf->active) continue;

		bf->x_scaled += (int32_t)bf->dir * bf->speed * dt;
		int16_t x = bf->x_scaled / 1000;

		if (x < -20 || x > 148) {
			bf->active = false;
			continue;
		}

		if (!bf->reflected) {
			if (abs(x - 62) <= 4) {
				bf->active = false;
				if (hero.state == PLAYER_STATE_SHIELD) {
					BUZZER_PlaySound(BUZZER_SOUND_CLICK);
				} else {
					player_take_damage();
				}
			}
		} else {
			if (abs(x - boss.x) <= 12) {
				bf->active = false;
				if (boss.hp > 0) {
					boss.hp--;
					BUZZER_PlaySound(BUZZER_SOUND_BANG);
					if (boss.hp == 0) {
						boss.state = BOSS_STATE_DEFEAT;
						boss.state_timer = 1500;
					}
				}
			}
		}
	}

	// Update flash timer
	flash_timer += dt;
	if (flash_timer >= 150) {
		flash_timer = 0;
		flash_state = !flash_state;
	}

	// Check Phase 2 transition for Wave 5 Final Boss
	if (boss.wave == 5 && !boss.phase2 && boss.hp <= (boss.max_hp / 2) && boss.state != BOSS_STATE_DEFEAT) {
		boss.phase2 = true;
		boss.enraged_alert_timer = 1500;
		BUZZER_PlaySound(BUZZER_SOUND_GOODBYE);
	}

	if (boss.enraged_alert_timer > dt) {
		boss.enraged_alert_timer -= dt;
	} else {
		boss.enraged_alert_timer = 0;
	}

	uint32_t cooldown = boss.phase2 ? 1000 : 2000;

	switch (boss.state) {
	case BOSS_STATE_SPAWNING: {
		if (boss.state_timer > dt) {
			boss.state_timer -= dt;
		} else {
			boss.x = (boss.dir == -1) ? 10 : 114;
			boss.state = BOSS_STATE_IDLE;
			boss.action_timer = cooldown;
		}
	} break;

	case BOSS_STATE_IDLE: {
		if (boss.action_timer > dt) {
			boss.action_timer -= dt;
		} else {
			boss.current_attack = (boss.current_attack + 1) % 3;
			if (boss.current_attack == 0) {
				boss.state = BOSS_STATE_CHARGE;
				boss.state_timer = 0;
			} else if (boss.current_attack == 1) {
				boss.state = BOSS_STATE_FIREBALL;
				boss.state_timer = boss.phase2 ? 1000 : 1200;
				fireball_timer = 0;
			} else {
				spawn_monster();
				if (boss.phase2) spawn_monster();
				BUZZER_PlaySound(BUZZER_SOUND_3BEEP);
				boss.action_timer = cooldown + 1000;
			}
		}
	} break;

	case BOSS_STATE_CHARGE: {
		if (boss.dir == -1) {
			if (boss.x < 44) {
				boss.x += 2;
			} else {
				boss.x = 44;
				boss.state_timer = 1200;
				boss.state = BOSS_STATE_SLASH;
			}
		} else {
			if (boss.x > 80) {
				boss.x -= 2;
			} else {
				boss.x = 80;
				boss.state_timer = 1200;
				boss.state = BOSS_STATE_SLASH;
			}
		}
	} break;

	case BOSS_STATE_SLASH: {
		if (boss.state_timer > dt) {
			uint32_t prev_timer = boss.state_timer;
			boss.state_timer -= dt;
			if (prev_timer > 350 && boss.state_timer <= 350) {
				BUZZER_PlaySound(BUZZER_SOUND_CLICK);
			}
		} else {
			if (hero.state != PLAYER_STATE_SHIELD) {
				player_take_damage();
				player_take_damage();
			} else {
				BUZZER_PlaySound(BUZZER_SOUND_CLICK);
			}
			boss.state = BOSS_STATE_RETREAT;
		}
	} break;

	case BOSS_STATE_RETREAT: {
		if (boss.dir == -1) {
			if (boss.x > 10) {
				boss.x -= 2;
			} else {
				boss.x = 10;
				boss.state = BOSS_STATE_IDLE;
				boss.action_timer = cooldown;
			}
		} else {
			if (boss.x < 114) {
				boss.x += 2;
			} else {
				boss.x = 114;
				boss.state = BOSS_STATE_IDLE;
				boss.action_timer = cooldown;
			}
		}
	} break;

	case BOSS_STATE_FIREBALL: {
		if (boss.state_timer > dt) {
			boss.state_timer -= dt;
			fireball_timer += dt;
			uint32_t interval = boss.phase2 ? 350 : 600;
			if (fireball_timer >= interval) {
				fireball_timer = 0;
				boss_fireball_spawn();
			}
		} else {
			boss.state = BOSS_STATE_IDLE;
			boss.action_timer = cooldown;
		}
	} break;

	case BOSS_STATE_DEFEAT: {
		if (boss.state_timer > dt) {
			boss.state_timer -= dt;
		} else {
			boss.active = false;
			game_item_spawn();
			game_add_score(300);
			BUZZER_PlaySound(BUZZER_SOUND_WELCOME);
		}
	} break;

	default:
		break;
	}
}

void game_boss_draw() {
	if (!boss.active) return;

	if (boss.state == BOSS_STATE_SPAWNING) {
		if (flash_state) {
			view_render.setCursor(34, 25);
			view_render.print("BOSS ALERT");
		}
		return;
	}

	if (boss.state != BOSS_STATE_DEFEAT) {
		view_render.drawRect(14, 56, 96, 4, WHITE);
		int hp_width = (boss.hp * 94) / boss.max_hp;
		if (hp_width > 0) {
			view_render.fillRect(15, 57, hp_width, 2, WHITE);
		}

		if (boss.enraged_alert_timer > 0) {
			if (flash_state) {
				view_render.setCursor(20, 10);
				view_render.print("!!! ENRAGED BOSS !!!");
			}
		}
	}

	bool draw_boss = true;
	if (boss.state == BOSS_STATE_SLASH && boss.state_timer > 0 && boss.state_timer <= 350) {
		draw_boss = flash_state;
	}

	if (boss.state == BOSS_STATE_SLASH || boss.state == BOSS_STATE_CHARGE) {
		if (boss.state_timer > 350 || flash_state) {
			view_render.setCursor(boss.x - 7, GAME_GROUND_Y - 34);
			view_render.print("[!]");
		}
	}

	if (draw_boss) {
		const unsigned char* bitmap = bitmap_boss_idle_left;
		int w = 24;
		int h = 28;
		bool is_right = (boss.dir == -1);

		switch (boss.state) {
		case BOSS_STATE_IDLE:
			bitmap = is_right ? bitmap_boss_idle_right : bitmap_boss_idle_left;
			w = 24; h = 28;
			break;

		case BOSS_STATE_CHARGE:
		case BOSS_STATE_RETREAT: {
			bool walk_frame = (boss.x / 4) % 2;
			bitmap = walk_frame ? (is_right ? bitmap_boss_walk1_right : bitmap_boss_walk1_left)
			                    : (is_right ? bitmap_boss_walk2_right : bitmap_boss_walk2_left);
			w = 24; h = 28;
		} break;

		case BOSS_STATE_SLASH:
			if (boss.state_timer > 200) {
				bitmap = is_right ? bitmap_boss_windup_right : bitmap_boss_windup_left;
				w = 24; h = 28;
			} else {
				bitmap = is_right ? bitmap_boss_strike_right : bitmap_boss_strike_left;
				w = 32; h = 28;
			}
			break;

		case BOSS_STATE_FIREBALL:
			bitmap = is_right ? bitmap_boss_summon_right : bitmap_boss_summon_left;
			w = 28; h = 28;
			break;

		case BOSS_STATE_DEFEAT:
			bitmap = is_right ? bitmap_boss_hurt_right : bitmap_boss_hurt_left;
			w = 28; h = 20;
			break;

		default:
			bitmap = is_right ? bitmap_boss_idle_right : bitmap_boss_idle_left;
			w = 24; h = 28;
			break;
		}

		view_render.drawBitmap(boss.x - w/2, GAME_GROUND_Y - h, bitmap, w, h, WHITE);
	}

	for (int i = 0; i < MAX_BOSS_FIREBALLS; i++) {
		BossFireball* bf = &boss_fireballs[i];
		if (!bf->active) continue;

		int16_t fx = bf->x_scaled / 1000;
		const unsigned char* fb_bitmap = (bf->dir == 1) ? bitmap_boss_fireball_right : bitmap_boss_fireball_left;
		view_render.drawBitmap(fx - 5, bf->y - 3, fb_bitmap, 10, 6, WHITE);
	}
}

void game_boss_check_parry() {
	if (!boss.active) return;

	for (int i = 0; i < MAX_BOSS_FIREBALLS; i++) {
		BossFireball* bf = &boss_fireballs[i];
		if (!bf->active || bf->reflected) continue;

		int16_t fx = bf->x_scaled / 1000;
		if (abs(fx - 62) <= 16) {
			if ((bf->dir == 1 && hero.dir == DIR_LEFT) || (bf->dir == -1 && hero.dir == DIR_RIGHT)) {
				bf->dir = -bf->dir;
				bf->reflected = true;
				bf->speed = 60;
				BUZZER_PlaySound(BUZZER_SOUND_HIGHSCORE);
			}
		}
	}

	if (boss.state == BOSS_STATE_SLASH || boss.state == BOSS_STATE_RETREAT) {
		if ((boss.x < 62 && hero.dir == DIR_LEFT) || (boss.x > 62 && hero.dir == DIR_RIGHT)) {
			if (boss.hp > 0) {
				boss.hp--;
				BUZZER_PlaySound(BUZZER_SOUND_BANG);
				if (boss.hp == 0) {
					boss.state = BOSS_STATE_DEFEAT;
					boss.state_timer = 1500;
				}
			}
		}
	}
}
