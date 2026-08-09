#include "game_loneblade.h"
#include "game_player.h"
#include "game_monster.h"
#include "game_arrow.h"
#include "game_ult_wave.h"
#include "screens.h"
#include "game_item.h"
#include "game_boss.h"
#include "flash.h"

struct SaveData {
	uint32_t magic;
	uint32_t high_score;
	uint32_t leaderboard[5];
};

static uint32_t g_score = 0;
static uint32_t g_high_score = 0;
static uint32_t g_wave  = 1;
static uint32_t g_wave_timer = 0;
static bool     g_over_sent = false;
static bool     g_win_sent  = false;
static bool     s_boss_spawned_w3 = false;
static bool     s_boss_spawned_w5 = false;
static bool     g_keep_score_flag = false;


uint8_t g_sound_enabled = 1;
uint8_t g_difficulty = 1; 

// Top 5 High Scores Leaderboard in RAM
static uint32_t g_leaderboard[5] = {0, 0, 0, 0, 0};

static void game_load_flash_data() {
	static bool s_loaded = false;
	if (!s_loaded) {
		s_loaded = true;
		SaveData data;
		flash_read(0x6000, (uint8_t*)&data, sizeof(data));
		if (data.magic == 0xABCD1234) {
			g_high_score = data.high_score;
			for (int i = 0; i < 5; i++) {
				g_leaderboard[i] = data.leaderboard[i];
			}
		} else {
			g_high_score = 0;
			for (int i = 0; i < 5; i++) {
				g_leaderboard[i] = 0;
			}
		}
	}
}

uint32_t game_get_score() { return g_score; }
uint32_t game_get_wave()  { return g_wave;  }

uint32_t game_get_high_score() {
	game_load_flash_data();
	return g_high_score;
}

uint32_t game_get_leaderboard_score(uint8_t index) {
	game_load_flash_data();
	if (index < 5) return g_leaderboard[index];
	return 0;
}

void game_update_high_score() {
	if (g_score > g_high_score) {
		g_high_score = g_score;
	}

	int pos = -1;
	for (int i = 0; i < 5; i++) {
		if (g_score > g_leaderboard[i]) {
			pos = i;
			break;
		}
	}

	if (pos != -1) {
		for (int i = 4; i > pos; i--) {
			g_leaderboard[i] = g_leaderboard[i - 1];
		}
		g_leaderboard[pos] = g_score;
	}

	// Save to Flash (Sector 0x6000)
	SaveData data;
	data.magic = 0xABCD1234;
	data.high_score = g_high_score;
	for (int i = 0; i < 5; i++) {
		data.leaderboard[i] = g_leaderboard[i];
	}
	flash_erase_sector(0x6000);
	flash_write(0x6000, (uint8_t*)&data, sizeof(data));
}

void game_add_score(uint32_t score) {
	g_score += score;
}

void game_set_keep_score_flag(bool keep) {
	g_keep_score_flag = keep;
}


void game_loneblade_init() {
	game_load_flash_data();

	if (!g_keep_score_flag) {
		g_score = 0;
	}
	g_keep_score_flag = false; 

	g_wave       = 1;
	g_wave_timer = 0;
	g_over_sent  = false;
	g_win_sent   = false;
	s_boss_spawned_w3 = false;
	s_boss_spawned_w5 = false;
	player_init();
	monster_init();
	arrow_init();
	ult_wave_init();
	game_item_init();
	game_boss_init();
}

void game_loneblade_update(uint32_t dt) {
	if (!boss.active) {
		g_wave_timer += dt;
		if (g_wave_timer >= 20000) {
			g_wave_timer = 0;
			g_wave++;
			g_score += 100; 
		}
	}

	if (g_wave == 3 && !s_boss_spawned_w3) {
		s_boss_spawned_w3 = true;
		game_boss_spawn(3);
	} else if (g_wave == 5 && !s_boss_spawned_w5) {
		s_boss_spawned_w5 = true;
		game_boss_spawn(5);
	}

	player_update(dt);
	monster_update(dt);
	arrow_update(dt);
	ult_wave_update(dt);
	game_item_update(dt);
	game_boss_update(dt);

	game_add_score(monster_drain_score());

	if (s_boss_spawned_w5 && !boss.active && !g_win_sent) {
		g_win_sent = true;
		game_update_high_score();
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_WIN);
	}

	if (hero.state == PLAYER_STATE_DEFEAT && !g_over_sent) {
		g_over_sent = true;
		task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_OVER);
	}
}

static void draw_hud_heart(int16_t x, int16_t y, bool filled) {
	if (filled) {
		view_render.drawPixel(x+1, y,   WHITE);
		view_render.drawPixel(x+3, y,   WHITE);
		view_render.drawPixel(x,   y+1, WHITE);
		view_render.drawPixel(x+1, y+1, WHITE);
		view_render.drawPixel(x+2, y+1, WHITE);
		view_render.drawPixel(x+3, y+1, WHITE);
		view_render.drawPixel(x+4, y+1, WHITE);
		view_render.drawPixel(x+1, y+2, WHITE);
		view_render.drawPixel(x+2, y+2, WHITE);
		view_render.drawPixel(x+3, y+2, WHITE);
		view_render.drawPixel(x+2, y+3, WHITE);
	} else {
		view_render.drawPixel(x+1, y,   WHITE);
		view_render.drawPixel(x+3, y,   WHITE);
		view_render.drawPixel(x,   y+1, WHITE);
		view_render.drawPixel(x+2, y+1, WHITE);
		view_render.drawPixel(x+4, y+1, WHITE);
		view_render.drawPixel(x+1, y+2, WHITE);
		view_render.drawPixel(x+3, y+2, WHITE);
		view_render.drawPixel(x+2, y+3, WHITE);
	}
}

static void draw_hud_mana_bolt(int16_t x, int16_t y) {
	view_render.drawPixel(x+2, y,   WHITE);
	view_render.drawPixel(x+1, y+1, WHITE);
	view_render.drawPixel(x+2, y+1, WHITE);
	view_render.drawPixel(x,   y+2, WHITE);
	view_render.drawPixel(x+1, y+2, WHITE);
	view_render.drawPixel(x+2, y+2, WHITE);
	view_render.drawPixel(x+3, y+2, WHITE);
	view_render.drawPixel(x+1, y+3, WHITE);
	view_render.drawPixel(x+2, y+3, WHITE);
	view_render.drawPixel(x+1, y+4, WHITE);
}

void game_loneblade_draw() {
	// Full width ground line
	view_render.drawLine(0, GAME_GROUND_Y, 127, GAME_GROUND_Y, WHITE);

	// Thin HUD separator line
	view_render.drawLine(0, 9, 127, 9, WHITE);

	// Subtle background decorations (Moon, Stars, Mountains, Cloud)
	// 1. Stars (single pixels in the sky)
	view_render.drawPixel(12, 16, WHITE);
	view_render.drawPixel(52, 13, WHITE);
	view_render.drawPixel(85, 17, WHITE);

	// 2. A neat little pixel-art cloud floating in the sky
	view_render.drawLine(34, 15, 39, 15, WHITE);
	view_render.drawLine(32, 16, 41, 16, WHITE);
	view_render.drawLine(33, 17, 40, 17, WHITE);

	// 3. Crescent Moon at top-right
	view_render.drawPixel(112, 13, WHITE);
	view_render.drawPixel(113, 13, WHITE);
	view_render.drawPixel(111, 14, WHITE);
	view_render.drawPixel(110, 15, WHITE);
	view_render.drawPixel(111, 16, WHITE);
	view_render.drawPixel(112, 17, WHITE);
	view_render.drawPixel(113, 17, WHITE);

	// 4. Layered Mountains (Left Range)
	// Peak 1 (Far)
	view_render.drawLine(0,  52, 16, 38, WHITE);
	view_render.drawLine(16, 38, 32, 52, WHITE);
	// Peak 2 (Near)
	view_render.drawLine(18, 52, 30, 44, WHITE);
	view_render.drawLine(30, 44, 46, 52, WHITE);

	// 5. Layered Mountains (Right Range)
	// Peak 1 (Far)
	view_render.drawLine(72,  52, 92, 35, WHITE);
	view_render.drawLine(92, 35, 112, 52, WHITE);
	// Peak 2 (Near)
	view_render.drawLine(96,  52, 110, 42, WHITE);
	view_render.drawLine(110, 42, 127, 52, WHITE);

	view_render.setTextSize(1);
	view_render.setTextColor(WHITE);

	// 1. HP: Draw 5 neat heart icons
	for (int i = 0; i < PLAYER_MAX_HP; i++) {
		draw_hud_heart(4 + i*6, 2, i < hero.hp);
	}

	// 2. Mana: Draw bolt and bar
	draw_hud_mana_bolt(38, 2);
	view_render.drawRect(44, 2, 22, 5, WHITE);
	int mana_width = (hero.mana * 20) / PLAYER_MAX_MANA;
	if (mana_width > 0) {
		view_render.fillRect(45, 3, mana_width, 3, WHITE);
	}

	// 3. Wave
	view_render.setCursor(68, 1);
	view_render.print("W:");
	view_render.print(g_wave);

	// 4. Score
	view_render.setTextWrap(false); 
	view_render.setCursor(86, 1);
	view_render.print("S:");
	view_render.print(g_score);

	player_draw();
	monster_draw();
	arrow_draw();
	ult_wave_draw();
	game_item_draw();
	game_boss_draw();
}
