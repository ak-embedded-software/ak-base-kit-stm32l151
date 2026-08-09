#ifndef __SCREENS_BITMAP_H__
#define __SCREENS_BITMAP_H__

#include "view_render.h"

// scr_welcome
extern const unsigned char PROGMEM bitmap_dolphin[];

// Hero Sprites (New)
// bitmap_hero_idle_left: 22x19px
extern const unsigned char PROGMEM bitmap_hero_idle_left[];
// bitmap_hero_idle_right: 22x19px
extern const unsigned char PROGMEM bitmap_hero_idle_right[];
// bitmap_hero_attack_left: 32x19px
extern const unsigned char PROGMEM bitmap_hero_attack_left[];
// bitmap_hero_attack_right: 32x19px
extern const unsigned char PROGMEM bitmap_hero_attack_right[];
// bitmap_hero_attack2_left: 28x19px
extern const unsigned char PROGMEM bitmap_hero_attack2_left[];
// bitmap_hero_attack2_right: 28x19px
extern const unsigned char PROGMEM bitmap_hero_attack2_right[];
// bitmap_hero_shield_left: 16x19px
extern const unsigned char PROGMEM bitmap_hero_shield_left[];
// bitmap_hero_shield_right: 16x19px
extern const unsigned char PROGMEM bitmap_hero_shield_right[];
// bitmap_hero_defeat_left: 42x19px
extern const unsigned char PROGMEM bitmap_hero_defeat_left[];
// bitmap_hero_defeat_right: 42x19px
extern const unsigned char PROGMEM bitmap_hero_defeat_right[];
// bitmap_hero_ulti_left: 23x34px
extern const unsigned char PROGMEM bitmap_hero_ulti_left[];
// bitmap_hero_ulti_right: 23x34px
extern const unsigned char PROGMEM bitmap_hero_ulti_right[];

// Hero Ult Wave (16x16px)
extern const unsigned char PROGMEM bitmap_hero_ult_wave_left[];
extern const unsigned char PROGMEM bitmap_hero_ult_wave_right[];

// Monster Bitmaps
extern const unsigned char PROGMEM bitmap_monster_normal_walk1_left[];
extern const unsigned char PROGMEM bitmap_monster_normal_walk2_left[];
extern const unsigned char PROGMEM bitmap_monster_normal_attack_windup_left[];
extern const unsigned char PROGMEM bitmap_monster_normal_attack_strike_left[];
extern const unsigned char PROGMEM bitmap_monster_normal_hurt_left[];

extern const unsigned char PROGMEM bitmap_monster_normal_walk1_right[];
extern const unsigned char PROGMEM bitmap_monster_normal_walk2_right[];
extern const unsigned char PROGMEM bitmap_monster_normal_attack_windup_right[];
extern const unsigned char PROGMEM bitmap_monster_normal_attack_strike_right[];
extern const unsigned char PROGMEM bitmap_monster_normal_hurt_right[];

extern const unsigned char PROGMEM bitmap_monster_armored_walk1_left[];
extern const unsigned char PROGMEM bitmap_monster_armored_walk2_left[];
extern const unsigned char PROGMEM bitmap_monster_armored_attack_windup_left[];
extern const unsigned char PROGMEM bitmap_monster_armored_attack_strike_left[];
extern const unsigned char PROGMEM bitmap_monster_armored_hurt_left[];

extern const unsigned char PROGMEM bitmap_monster_armored_walk1_right[];
extern const unsigned char PROGMEM bitmap_monster_armored_walk2_right[];
extern const unsigned char PROGMEM bitmap_monster_armored_attack_windup_right[];
extern const unsigned char PROGMEM bitmap_monster_armored_attack_strike_right[];
extern const unsigned char PROGMEM bitmap_monster_armored_hurt_right[];

extern const unsigned char PROGMEM bitmap_monster_fly_frame1[];
extern const unsigned char PROGMEM bitmap_monster_fly_frame2[];

// Boss Bitmaps (Generated)
extern const unsigned char PROGMEM bitmap_boss_idle_left[];
extern const unsigned char PROGMEM bitmap_boss_idle_right[];
extern const unsigned char PROGMEM bitmap_boss_walk1_left[];
extern const unsigned char PROGMEM bitmap_boss_walk1_right[];
extern const unsigned char PROGMEM bitmap_boss_walk2_left[];
extern const unsigned char PROGMEM bitmap_boss_walk2_right[];
extern const unsigned char PROGMEM bitmap_boss_windup_left[];
extern const unsigned char PROGMEM bitmap_boss_windup_right[];
extern const unsigned char PROGMEM bitmap_boss_strike_left[];
extern const unsigned char PROGMEM bitmap_boss_strike_right[];
extern const unsigned char PROGMEM bitmap_boss_hurt_left[];
extern const unsigned char PROGMEM bitmap_boss_hurt_right[];
extern const unsigned char PROGMEM bitmap_boss_summon_left[];
extern const unsigned char PROGMEM bitmap_boss_summon_right[];

// Boss Fireball Bitmaps
extern const unsigned char PROGMEM bitmap_boss_fireball_left[];
extern const unsigned char PROGMEM bitmap_boss_fireball_right[];

#endif //__SCREENS_BITMAP_H__
