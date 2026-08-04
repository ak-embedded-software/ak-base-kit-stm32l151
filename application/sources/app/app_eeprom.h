/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 28/04/2026
 *          End:   28/04/2026
 ******************************************************************************
**/
#ifndef __APP_EEPROM_H__
#define __APP_EEPROM_H__

#include <stdint.h>
#include "app.h"

/**
  *****************************************************************************
  * EEPROM define address.
  *
  *****************************************************************************
  */
/*
 * EEPROM layout.
 *
 * Records written through app_eeprom.cpp are stored as:
 *     [magic_number:4][payload][check_sum:1]
 *
 * magic_number marks an initialised record so erased or unrelated bytes are
 * never read back as valid app data. check_sum is an 8-bit additive sum over
 * magic_number + payload - lightweight corruption detection, not security.
 * A record that fails either test falls back to compiled-in defaults.
 *
 * There is no wear levelling here. Callers should avoid rewriting a record
 * whose value has not changed.
 *
 *   0x0100  setting     dungeon_game_setting_t   (4B payload  -> 9B record)
 *   0x0120  best score  dungeon_game_score_t     (12B payload -> 17B record)
 *   0x0140  save game   dungeon_game_save_t      (self-validating, own magic)
 *   0x0200  last score  uint32_t                 (4B payload  -> 9B record)
 */
#define EEPROM_START_ADDR           (0X0000)
#define EEPROM_END_ADDR             (0X1000)

#define EEPROM_SETTING_START_ADDR   (0X0100)
#define EEPROM_DUNGEON_SCORE_ADDR   (0X0120)
#define EEPROM_DUNGEON_SAVE_ADDR    (0X0140)
#define EEPROM_SCORE_PLAY_ADDR      (0X0200)

/* "DUN2". Bản ghi save đời 1 ("DUNG") thiếu check_sum và thiếu message_next,
 * nạp lại nó là treo máy. Đổi magic để mọi bản save cũ bị bỏ qua đúng một lần,
 * thay vì cố đọc rồi đoán mò. */
#define DUNGEON_SAVE_MAGIC          (0x44554E32UL)
#define DUNGEON_EEPROM_MAGIC        (0x44474D31UL)  /* "DGM1" */

/* setting bounds / defaults */
#define DUNGEON_SETTING_SILENT_OFF          (0)
#define DUNGEON_SETTING_SILENT_ON           (1)
#define DUNGEON_SETTING_PARTY_SIZE_MIN      (1)
#define DUNGEON_SETTING_PARTY_SIZE_MAX      (5)
#define DUNGEON_SETTING_PARTY_SIZE_DEFAULT  (3)
#define DUNGEON_SETTING_MONSTER_SPEED_MIN   (1)
#define DUNGEON_SETTING_MONSTER_SPEED_MAX   (5)
#define DUNGEON_SETTING_MONSTER_SPEED_DEF   (2)
#define DUNGEON_SETTING_ANIM_SPEED_MIN      (1)
#define DUNGEON_SETTING_ANIM_SPEED_MAX      (10)
#define DUNGEON_SETTING_ANIM_SPEED_DEFAULT  (4)

/******************************************************************************/
/* Dungeon game */
/******************************************************************************/
typedef struct {
  /* setting data */
  bool silent;
  uint8_t party_size;
  uint8_t anim_speed;
  uint8_t monster_speed;
} dungeon_game_setting_t;

typedef struct {
  uint32_t best_score;
  uint8_t best_level;
  uint8_t best_stage;
  uint8_t reserved[3];
} dungeon_game_score_t;

typedef struct {
  uint32_t magic;
  uint8_t valid;
  uint8_t level;
  uint8_t stage;
  uint8_t total_stages;
  uint16_t score;
  int16_t player_hp;
  int16_t player_max_hp;
  int16_t player_atk;
  int16_t player_def;
  int16_t monster_hp;
  int16_t monster_max_hp;
  int16_t monster_dmg;
  int16_t monster_armor;
  uint8_t current_view;
  uint8_t current_monster;
  uint8_t selected_action;
  uint8_t selected_support_item;
  uint8_t poison_turns;
  uint8_t burn_turns;
  uint8_t curse_turns;
  uint8_t enemy_poison_turns;
  uint8_t enemy_poison_damage;
  uint8_t monster_dodge_ready;
  uint8_t defend_active;
  uint8_t support_event;
  uint8_t support_pending;
  uint8_t battle_turn;
  uint8_t travel_progress;
  uint8_t inventory[7];
  uint8_t chest_options[3];
  /* Bấm MODE ở màn thông báo thì đi đâu tiếp. Thiếu trường này là bản save
   * đời cũ treo máy: nạp lại xong current_view vẫn là MESSAGE, nhưng biến RAM
   * dungeon_message_next về 0 (NONE) nên không nhánh nào khớp, bấm MODE mãi
   * cũng không có gì xảy ra. */
  uint8_t message_next;
  /* Tổng cộng dồn 8 bit trên magic + toàn bộ payload. Ghi EEPROM record này
   * mất ~50 byte; mất điện giữa chừng thì magic (4 byte đầu) đã nằm sẵn trong
   * flash rồi, phần đuôi thì chưa. Không có check_sum là nửa cũ nửa mới vẫn
   * được coi là hợp lệ. */
  uint8_t check_sum;
} dungeon_game_save_t;

/*****************************************************************************/
/*  Validated access API (implemented in app_eeprom.cpp)
 *
 *  All read functions return true when a valid stored record was found, and
 *  false when defaults were substituted. Callers that only need the value can
 *  ignore the return code.
 */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

extern dungeon_game_setting_t settingdata;

extern void dungeon_setting_sanitize(dungeon_game_setting_t* data);
extern bool dungeon_setting_read(dungeon_game_setting_t* data);
extern bool dungeon_setting_write(dungeon_game_setting_t* data);

extern bool dungeon_score_read(dungeon_game_score_t* data);
extern bool dungeon_score_write(dungeon_game_score_t* data);

extern bool dungeon_last_score_read(uint32_t* data);
extern bool dungeon_last_score_write(uint32_t data);

/* Bản ghi save của ván đang chơi dở. Trả về false khi bản ghi không có, sai
 * magic, hoặc sai check_sum (mất điện lúc đang ghi). Lúc đó *data đã được xoá
 * sạch về 0, người gọi cứ coi như chưa có save. */
extern bool dungeon_save_read(dungeon_game_save_t* data);
extern bool dungeon_save_write(dungeon_game_save_t* data);
extern bool dungeon_save_exists(void);
extern void dungeon_save_erase(void);

#ifdef __cplusplus
}
#endif

#endif //__APP_EEPROM_H__
