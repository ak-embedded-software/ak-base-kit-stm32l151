/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @brief:  Validated access layer over the raw EEPROM driver.
 *
 * Every record is stored as [magic_number][payload][check_sum] so that an
 * erased, half-written or unrelated region is never handed back to the game as
 * if it were real data. On any failure the caller receives compiled-in
 * defaults and a false return value.
 *
 * See app_eeprom.h for the address map.
 ******************************************************************************
**/

#include <stddef.h>

#include "app_eeprom.h"

#include "eeprom.h"

dungeon_game_setting_t settingdata;

/*****************************************************************************/
/*  Record wrappers
 */
/*****************************************************************************/
typedef struct {
	uint32_t magic_number;
	dungeon_game_setting_t data;
	uint8_t check_sum;
} dungeon_setting_eeprom_t;

typedef struct {
	uint32_t magic_number;
	dungeon_game_score_t data;
	uint8_t check_sum;
} dungeon_score_eeprom_t;

typedef struct {
	uint32_t magic_number;
	uint32_t data;
	uint8_t check_sum;
} dungeon_last_score_eeprom_t;

#define DUNGEON_SETTING_CHECKSUM_SIZE    (sizeof(uint32_t) + sizeof(dungeon_game_setting_t))
#define DUNGEON_SCORE_CHECKSUM_SIZE      (sizeof(uint32_t) + sizeof(dungeon_game_score_t))
#define DUNGEON_LAST_SCORE_CHECKSUM_SIZE (sizeof(uint32_t) + sizeof(uint32_t))

/*****************************************************************************/
/*  Checksum helpers
 */
/*****************************************************************************/
static uint8_t dungeon_eeprom_checksum(uint8_t* data, uint32_t size) {
	uint8_t check_sum = 0;

	for (uint32_t i = 0; i < size; i++) {
		check_sum += data[i];
	}

	return check_sum;
}

static void dungeon_eeprom_seal(uint32_t* magic_number, uint8_t* check_sum, uint32_t check_sum_size) {
	*magic_number = DUNGEON_EEPROM_MAGIC;
	*check_sum    = dungeon_eeprom_checksum((uint8_t*)magic_number, check_sum_size);
}

static bool dungeon_eeprom_is_valid(uint32_t* magic_number, uint8_t check_sum, uint32_t check_sum_size) {
	return (*magic_number == DUNGEON_EEPROM_MAGIC) && \
	       (check_sum == dungeon_eeprom_checksum((uint8_t*)magic_number, check_sum_size));
}

/*****************************************************************************/
/*  Setting
 */
/*****************************************************************************/
static void dungeon_setting_set_default(dungeon_game_setting_t* data) {
	data->silent        = DUNGEON_SETTING_SILENT_OFF;
	data->party_size    = DUNGEON_SETTING_PARTY_SIZE_DEFAULT;
	data->anim_speed    = DUNGEON_SETTING_ANIM_SPEED_DEFAULT;
	data->monster_speed = DUNGEON_SETTING_MONSTER_SPEED_DEF;
}

/* Clamp a record that passed magic+checksum but holds an out-of-range value
 * (for example after a firmware change narrowed the allowed range). */
void dungeon_setting_sanitize(dungeon_game_setting_t* data) {
	if ((data->party_size < DUNGEON_SETTING_PARTY_SIZE_MIN) || \
		(data->party_size > DUNGEON_SETTING_PARTY_SIZE_MAX)) {
		data->party_size = DUNGEON_SETTING_PARTY_SIZE_DEFAULT;
	}

	if ((data->monster_speed < DUNGEON_SETTING_MONSTER_SPEED_MIN) || \
		(data->monster_speed > DUNGEON_SETTING_MONSTER_SPEED_MAX)) {
		data->monster_speed = DUNGEON_SETTING_MONSTER_SPEED_DEF;
	}

	if ((data->anim_speed < DUNGEON_SETTING_ANIM_SPEED_MIN) || \
		(data->anim_speed > DUNGEON_SETTING_ANIM_SPEED_MAX)) {
		data->anim_speed = DUNGEON_SETTING_ANIM_SPEED_DEFAULT;
	}
}

bool dungeon_setting_read(dungeon_game_setting_t* data) {
	dungeon_setting_eeprom_t eeprom_data;

	uint8_t ret = eeprom_read(EEPROM_SETTING_START_ADDR, (uint8_t*)&eeprom_data, sizeof(eeprom_data));

	if ((ret == EEPROM_DRIVER_OK) && \
		dungeon_eeprom_is_valid(&eeprom_data.magic_number, eeprom_data.check_sum, DUNGEON_SETTING_CHECKSUM_SIZE)) {
		*data = eeprom_data.data;
		dungeon_setting_sanitize(data);
		return true;
	}

	dungeon_setting_set_default(data);
	return false;
}

bool dungeon_setting_write(dungeon_game_setting_t* data) {
	dungeon_setting_eeprom_t eeprom_data;

	eeprom_data.data = *data;
	dungeon_eeprom_seal(&eeprom_data.magic_number, &eeprom_data.check_sum, DUNGEON_SETTING_CHECKSUM_SIZE);

	return eeprom_write(EEPROM_SETTING_START_ADDR, (uint8_t*)&eeprom_data, sizeof(eeprom_data)) == EEPROM_DRIVER_OK;
}

/*****************************************************************************/
/*  Best score / progress
 */
/*****************************************************************************/
static void dungeon_score_set_default(dungeon_game_score_t* data) {
	data->best_score = 0;
	data->best_level = 0;
	data->best_stage = 0;
	data->reserved[0] = 0;
	data->reserved[1] = 0;
	data->reserved[2] = 0;
}

bool dungeon_score_read(dungeon_game_score_t* data) {
	dungeon_score_eeprom_t eeprom_data;

	uint8_t ret = eeprom_read(EEPROM_DUNGEON_SCORE_ADDR, (uint8_t*)&eeprom_data, sizeof(eeprom_data));

	if ((ret == EEPROM_DRIVER_OK) && \
		dungeon_eeprom_is_valid(&eeprom_data.magic_number, eeprom_data.check_sum, DUNGEON_SCORE_CHECKSUM_SIZE)) {
		*data = eeprom_data.data;
		return true;
	}

	dungeon_score_set_default(data);
	return false;
}

bool dungeon_score_write(dungeon_game_score_t* data) {
	dungeon_score_eeprom_t eeprom_data;

	eeprom_data.data = *data;
	dungeon_eeprom_seal(&eeprom_data.magic_number, &eeprom_data.check_sum, DUNGEON_SCORE_CHECKSUM_SIZE);

	return eeprom_write(EEPROM_DUNGEON_SCORE_ADDR, (uint8_t*)&eeprom_data, sizeof(eeprom_data)) == EEPROM_DRIVER_OK;
}

/*****************************************************************************/
/*  Score of the last finished run
 */
/*****************************************************************************/
bool dungeon_last_score_read(uint32_t* data) {
	dungeon_last_score_eeprom_t eeprom_data;

	uint8_t ret = eeprom_read(EEPROM_SCORE_PLAY_ADDR, (uint8_t*)&eeprom_data, sizeof(eeprom_data));

	if ((ret == EEPROM_DRIVER_OK) && \
		dungeon_eeprom_is_valid(&eeprom_data.magic_number, eeprom_data.check_sum, DUNGEON_LAST_SCORE_CHECKSUM_SIZE)) {
		*data = eeprom_data.data;
		return true;
	}

	*data = 0;
	return false;
}

bool dungeon_last_score_write(uint32_t data) {
	dungeon_last_score_eeprom_t eeprom_data;

	eeprom_data.data = data;
	dungeon_eeprom_seal(&eeprom_data.magic_number, &eeprom_data.check_sum, DUNGEON_LAST_SCORE_CHECKSUM_SIZE);

	return eeprom_write(EEPROM_SCORE_PLAY_ADDR, (uint8_t*)&eeprom_data, sizeof(eeprom_data)) == EEPROM_DRIVER_OK;
}

/*****************************************************************************/
/*  Ván chơi dở
 *
 *  Bản ghi này mang magic riêng (DUNGEON_SAVE_MAGIC) chứ không dùng chung
 *  DUNGEON_EEPROM_MAGIC, vì nó còn có cờ `valid` để xoá save mà không phải xoá
 *  cả vùng. Nhưng cách tính check_sum thì giống hệt mấy bản ghi trên: cộng dồn
 *  8 bit từ byte đầu tới ngay trước ô check_sum.
 */
/*****************************************************************************/
/* Phải dùng offsetof chứ KHÔNG được lấy sizeof - 1.
 *
 * Struct này căn theo 4 byte (vì có uint32_t magic ở đầu), nên trình biên dịch
 * chèn thêm mấy byte đệm ở đuôi: check_sum nằm ở offset 52 nhưng sizeof lại là
 * 56. Lấy sizeof - 1 = 55 thì phép cộng dồn nuốt luôn cả ô check_sum lẫn mấy
 * byte đệm, đọc lại không bao giờ khớp -> lúc nào cũng tưởng save hỏng. */
#define DUNGEON_SAVE_CHECKSUM_SIZE  ((uint32_t)offsetof(dungeon_game_save_t, check_sum))

bool dungeon_save_read(dungeon_game_save_t* data) {
	dungeon_game_save_t eeprom_data;

	uint8_t ret = eeprom_read(EEPROM_DUNGEON_SAVE_ADDR, (uint8_t*)&eeprom_data, sizeof(eeprom_data));

	if ((ret == EEPROM_DRIVER_OK) && \
		(eeprom_data.magic == DUNGEON_SAVE_MAGIC) && \
		(eeprom_data.valid == 1) && \
		(eeprom_data.check_sum == dungeon_eeprom_checksum((uint8_t*)&eeprom_data, DUNGEON_SAVE_CHECKSUM_SIZE))) {
		*data = eeprom_data;
		return true;
	}

	/* Hỏng thì trả về bản ghi rỗng, đừng để người gọi đọc trúng rác. */
	for (uint32_t i = 0; i < sizeof(dungeon_game_save_t); i++) {
		((uint8_t*)data)[i] = 0;
	}
	return false;
}

bool dungeon_save_write(dungeon_game_save_t* data) {
	data->magic = DUNGEON_SAVE_MAGIC;
	data->valid = 1;
	data->check_sum = dungeon_eeprom_checksum((uint8_t*)data, DUNGEON_SAVE_CHECKSUM_SIZE);

	return eeprom_write(EEPROM_DUNGEON_SAVE_ADDR, (uint8_t*)data, sizeof(dungeon_game_save_t)) == EEPROM_DRIVER_OK;
}

bool dungeon_save_exists(void) {
	dungeon_game_save_t eeprom_data;
	return dungeon_save_read(&eeprom_data);
}

void dungeon_save_erase(void) {
	dungeon_game_save_t eeprom_data;

	for (uint32_t i = 0; i < sizeof(eeprom_data); i++) {
		((uint8_t*)&eeprom_data)[i] = 0;
	}

	eeprom_write(EEPROM_DUNGEON_SAVE_ADDR, (uint8_t*)&eeprom_data, sizeof(eeprom_data));
}
