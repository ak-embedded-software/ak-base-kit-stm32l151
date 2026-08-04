/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 03/05/2026
 *          End:   03/05/2026
 ******************************************************************************
**/
#include "dungeon_lane.h"
#include "dungeon_runtime.h"

uint32_t dungeon_game_score = 0;

/*****************************************************************************/
/*  Level / stage / travel progression, persistence and score.
 */
/*****************************************************************************/
void dungeon_init_player(uint8_t level) {
	memset(&dungeon_runtime, 0, sizeof(dungeon_runtime));
	dungeon_runtime.level = dungeon_clamp_level(level);
	dungeon_runtime.stage = 1;
	dungeon_runtime.total_stages = dungeon_stage_counts[dungeon_runtime.level - 1];
	dungeon_runtime.player_max_hp = 35 + (int16_t)dungeon_runtime.level * 5;
	dungeon_runtime.player_hp = dungeon_runtime.player_max_hp;
	dungeon_runtime.player_atk = 10 + (int16_t)dungeon_runtime.level * 2;
	dungeon_runtime.player_def = 4 + dungeon_runtime.level;
	dungeon_runtime.selected_action = 0;
	dungeon_game_score = 0;
	dungeon_prepare_stage();
}

void dungeon_update_best_progress() {
	dungeon_game_score_t score_data;
	dungeon_score_read(&score_data);

	if (score_data.best_level < 1 || score_data.best_level > 5) {
		score_data.best_level = 0;
		score_data.best_stage = 0;
	}

	if ((dungeon_runtime.level > score_data.best_level) ||
		((dungeon_runtime.level == score_data.best_level) && (dungeon_runtime.stage > score_data.best_stage))) {
		score_data.best_level = dungeon_runtime.level;
		score_data.best_stage = dungeon_runtime.stage;
	}

	if (dungeon_game_score > score_data.best_score) {
		score_data.best_score = dungeon_game_score;
	}

	dungeon_score_write(&score_data);
}

void dungeon_save_progress() {
	if (dungeon_persist_enabled == 0) {
		return;
	}

	dungeon_game_save_t save_data;
	memset(&save_data, 0, sizeof(save_data));
	save_data.magic = DUNGEON_SAVE_MAGIC;
	save_data.valid = 1;
	save_data.level = dungeon_runtime.level;
	save_data.stage = dungeon_runtime.stage;
	save_data.total_stages = dungeon_runtime.total_stages;
	save_data.score = (uint16_t)dungeon_game_score;
	save_data.player_hp = dungeon_runtime.player_hp;
	save_data.player_max_hp = dungeon_runtime.player_max_hp;
	save_data.player_atk = dungeon_runtime.player_atk;
	save_data.player_def = dungeon_runtime.player_def;
	save_data.monster_hp = dungeon_runtime.monster_hp;
	save_data.monster_max_hp = dungeon_runtime.monster_max_hp;
	save_data.monster_dmg = dungeon_runtime.monster_dmg;
	save_data.monster_armor = dungeon_runtime.monster_armor;
	save_data.current_view = dungeon_runtime.current_view;
	save_data.current_monster = dungeon_runtime.current_monster;
	save_data.selected_action = dungeon_runtime.selected_action;
	save_data.selected_support_item = dungeon_runtime.selected_support_item;
	save_data.poison_turns = dungeon_runtime.poison_turns;
	save_data.burn_turns = dungeon_runtime.burn_turns;
	save_data.curse_turns = dungeon_runtime.curse_turns;
	save_data.enemy_poison_turns = dungeon_runtime.enemy_poison_turns;
	save_data.enemy_poison_damage = dungeon_runtime.enemy_poison_damage;
	save_data.monster_dodge_ready = dungeon_runtime.monster_dodge_ready;
	save_data.defend_active = dungeon_runtime.defend_active;
	save_data.support_event = dungeon_runtime.support_event;
	save_data.support_pending = dungeon_runtime.support_pending;
	save_data.battle_turn = dungeon_runtime.battle_turn;
	save_data.travel_progress = dungeon_runtime.travel_progress;
	memcpy(save_data.inventory, dungeon_runtime.inventory, sizeof(save_data.inventory));
	memcpy(save_data.chest_options, dungeon_runtime.chest_options, sizeof(save_data.chest_options));
	/* Không lưu cái này là mất điện ở màn thông báo xong bấm Continue sẽ đứng
	 * hình: view vẫn là MESSAGE nhưng không biết bấm MODE thì đi đâu. */
	save_data.message_next = dungeon_message_next;
	dungeon_save_write(&save_data);
	dungeon_update_best_progress();
}

void dungeon_clear_save() {
	dungeon_save_erase();
}

uint8_t dungeon_has_save_data() {
	return dungeon_save_exists() ? 1 : 0;
}

/* Vá lại những trạng thái mà nạp nguyên xi vào là không đi tiếp được nữa.
 *
 * Bản save chụp đúng một khoảnh khắc bất kỳ của ván chơi. Mất điện thì khoảnh
 * khắc đó có thể rơi vào chỗ mà game chỉ đi tiếp được nhờ một biến nằm trong
 * RAM, mà RAM thì mất sạch. Hàm này gom hết mấy chỗ như vậy lại, ưu tiên
 * "luôn đi tiếp được" hơn là "khôi phục chính xác từng chi tiết".
 *
 * Trả về 0 nếu bản save không đáng khôi phục nữa (hero đã chết), lúc đó người
 * gọi nên coi như chưa có save và bắt đầu ván mới. */
static uint8_t dungeon_sanitize_restored_state() {
	/* Hero đã gục thì ván đó xong rồi, không có gì để chơi tiếp. */
	if (dungeon_runtime.player_hp <= 0) {
		return 0;
	}

	if (dungeon_runtime.player_max_hp <= 0) {
		dungeon_runtime.player_max_hp = dungeon_runtime.player_hp;
	}
	if (dungeon_runtime.player_hp > dungeon_runtime.player_max_hp) {
		dungeon_runtime.player_hp = dungeon_runtime.player_max_hp;
	}

	if (dungeon_runtime.total_stages == 0) {
		dungeon_runtime.total_stages = 1;
	}
	if (dungeon_runtime.stage == 0) {
		dungeon_runtime.stage = 1;
	}
	if (dungeon_runtime.stage > dungeon_runtime.total_stages) {
		dungeon_runtime.stage = dungeon_runtime.total_stages;
	}
	if (dungeon_runtime.current_monster > DUNGEON_MONSTER_EYE) {
		dungeon_runtime.current_monster = dungeon_monster_for_stage(dungeon_runtime.level, dungeon_runtime.stage);
	}
	if (dungeon_runtime.selected_action >= DUNGEON_ACTION_COUNT) {
		dungeon_runtime.selected_action = DUNGEON_ACTION_ATTACK;
	}
	if (dungeon_runtime.selected_support_item > 2) {
		dungeon_runtime.selected_support_item = 0;
	}
	if (dungeon_runtime.travel_progress > 100) {
		dungeon_runtime.travel_progress = 100;
	}

	if (dungeon_runtime.current_view == DUNGEON_VIEW_MESSAGE) {
		/* message_next không hợp lệ nghĩa là bản save đời cũ hoặc bị hỏng.
		 * Không đoán được đi đâu thì trả hero về đường đi, chỗ nào cũng tới
		 * được từ đó. */
		if ((dungeon_message_next == DUNGEON_NEXT_NONE) ||
			(dungeon_message_next > DUNGEON_NEXT_RETURN)) {
			dungeon_runtime.current_view = DUNGEON_VIEW_TRAVEL;
			dungeon_message_next = DUNGEON_NEXT_NONE;
		}
	}

	if (dungeon_runtime.current_view == DUNGEON_VIEW_CHEST) {
		/* Rương đã nhặt rồi mà view vẫn là CHEST: nhặt nữa thì support_event
		 * đã bằng 0, đếm lùi không được, ván treo tại chỗ. */
		if (dungeon_runtime.support_event == 0) {
			dungeon_runtime.current_view = DUNGEON_VIEW_TRAVEL;
			dungeon_runtime.travel_progress = 0;
			dungeon_runtime.support_pending = 1;
		}
		else {
			uint8_t index;
			for (index = 0; index < 3; index++) {
				if (dungeon_runtime.chest_options[index] >= DUNGEON_ITEM_COUNT) {
					dungeon_pick_chest_options();
					break;
				}
			}
		}
	}

	if (dungeon_runtime.current_view == DUNGEON_VIEW_BATTLE) {
		/* Chưa từng gán chỉ số quái, hoặc quái đã chết mà chưa kịp trả thưởng.
		 * Cả hai đều không đánh tiếp được, nên dựng lại con quái từ đầu. */
		if ((dungeon_runtime.monster_max_hp <= 0) || (dungeon_runtime.monster_hp <= 0)) {
			dungeon_set_monster_stats(dungeon_runtime.current_monster);
		}
		if (dungeon_runtime.monster_hp > dungeon_runtime.monster_max_hp) {
			dungeon_runtime.monster_hp = dungeon_runtime.monster_max_hp;
		}
	}

	if (dungeon_runtime.current_view == DUNGEON_VIEW_TRAVEL) {
		/* Hero đứng ở cuối đường mà không còn sự kiện nào chờ kích hoạt.
		 * dungeon_advance_travel() sẽ kẹp tiến độ ở 100 rồi thôi, không ai
		 * đẩy sang rương hay sang trận nữa. Bật lại cờ chờ là tick sau nó tự
		 * chạy tiếp đúng kịch bản. */
		if ((dungeon_runtime.travel_progress >= 100) && (dungeon_runtime.support_pending == 0)) {
			dungeon_runtime.support_pending = 1;
		}
	}

	/* Mấy biến chỉ sống trong RAM: nạp lại là về 0, phải đặt lại cho khớp. */
	dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_INPUT;
	dungeon_runtime.battle_wait_ticks = 0;
	dungeon_runtime.pre_battle_alert = (dungeon_runtime.travel_progress >= 70) ? 1 : 0;

	return 1;
}

uint8_t dungeon_restore_save() {
	dungeon_game_save_t save_data;
	if (dungeon_save_read(&save_data) == false) {
		return 0;
	}

	memset(&dungeon_runtime, 0, sizeof(dungeon_runtime));
	dungeon_runtime.level = dungeon_clamp_level(save_data.level);
	dungeon_runtime.stage = save_data.stage;
	dungeon_runtime.total_stages = save_data.total_stages;
	dungeon_runtime.current_view = save_data.current_view;
	dungeon_runtime.current_monster = save_data.current_monster;
	dungeon_runtime.support_event = save_data.support_event;
	dungeon_runtime.support_pending = save_data.support_pending;
	dungeon_runtime.battle_turn = save_data.battle_turn;
	dungeon_runtime.travel_progress = save_data.travel_progress;
	dungeon_runtime.selected_action = save_data.selected_action;
	dungeon_runtime.selected_support_item = save_data.selected_support_item;
	dungeon_runtime.defend_active = save_data.defend_active;
	dungeon_runtime.poison_turns = save_data.poison_turns;
	dungeon_runtime.burn_turns = save_data.burn_turns;
	dungeon_runtime.curse_turns = save_data.curse_turns;
	dungeon_runtime.enemy_poison_turns = save_data.enemy_poison_turns;
	dungeon_runtime.enemy_poison_damage = save_data.enemy_poison_damage;
	dungeon_runtime.monster_dodge_ready = save_data.monster_dodge_ready;
	dungeon_runtime.player_hp = save_data.player_hp;
	dungeon_runtime.player_max_hp = save_data.player_max_hp;
	dungeon_runtime.player_atk = save_data.player_atk;
	dungeon_runtime.player_def = save_data.player_def;
	dungeon_runtime.monster_hp = save_data.monster_hp;
	dungeon_runtime.monster_max_hp = save_data.monster_max_hp;
	dungeon_runtime.monster_dmg = save_data.monster_dmg;
	dungeon_runtime.monster_armor = save_data.monster_armor;
	memcpy(dungeon_runtime.inventory, save_data.inventory, sizeof(dungeon_runtime.inventory));
	memcpy(dungeon_runtime.chest_options, save_data.chest_options, sizeof(dungeon_runtime.chest_options));
	dungeon_game_score = save_data.score;
	dungeon_selected_level = dungeon_runtime.level;
	dungeon_message_next = save_data.message_next;
	dungeon_load_message_defaults();
	if (dungeon_runtime.current_view > DUNGEON_VIEW_BATTLE) {
		dungeon_runtime.current_view = DUNGEON_VIEW_TRAVEL;
	}

	/* Nạp xong chưa xong. Còn phải kiểm lại xem trạng thái vừa nạp có đi tiếp
	 * được không, không thì vá lại. Xem dungeon_sanitize_restored_state(). */
	if (dungeon_sanitize_restored_state() == 0) {
		dungeon_clear_save();
		return 0;
	}
	return 1;
}

void dungeon_prepare_continue() {
	dungeon_start_mode = DUNGEON_START_CONTINUE;
}

void dungeon_prepare_new_game() {
	dungeon_start_mode = DUNGEON_START_NEW_GAME;
	dungeon_selected_level = 1;
	dungeon_clear_save();
}

void dungeon_prepare_level(uint8_t level) {
	dungeon_start_mode = DUNGEON_START_LEVEL;
	dungeon_selected_level = dungeon_clamp_level(level);
}

void dungeon_prepare_creator_mode(uint8_t level) {
	dungeon_start_mode = DUNGEON_START_CREATOR;
	dungeon_selected_level = dungeon_clamp_level(level);
}

uint8_t dungeon_is_creator_mode() {
	return (dungeon_start_mode == DUNGEON_START_CREATOR);
}

void dungeon_prepare_stage() {
	dungeon_runtime.travel_progress = 0;
	dungeon_runtime.pre_battle_alert = 0;
	dungeon_runtime.current_view = DUNGEON_VIEW_TRAVEL;
	/* Every stage meets a chest before battle, boss stage gets 2 chests. */
	dungeon_runtime.support_event = (dungeon_runtime.stage >= dungeon_runtime.total_stages) ? 2 : 1;
	dungeon_runtime.support_pending = 1;
	dungeon_runtime.current_monster = dungeon_monster_for_stage(dungeon_runtime.level, dungeon_runtime.stage);
	dungeon_load_message_defaults();
	dungeon_save_progress();
}

/* Advance the hero along the forest path. Driven per tick by
 * DUNGEON_LANE_LEVEL_UP. Only meaningful in the travel view - the battle view
 * is handled entirely by dungeon_action. */
void dungeon_advance_travel() {
	if (dungeon_game_state != GAME_PLAY) {
		return;
	}

	if (dungeon_runtime.current_view != DUNGEON_VIEW_TRAVEL) {
		return;
	}

	/* Slow the hero down a bit so the path animation feels less rushed. */
	uint8_t travel_step = 1;
	if (dungeon_control.action_image == 1) {
		travel_step = 0;
	}
	dungeon_runtime.travel_progress += travel_step;

	if ((dungeon_runtime.support_event == 0) && (dungeon_runtime.pre_battle_alert == 0) && (dungeon_runtime.travel_progress >= 70)) {
		dungeon_runtime.pre_battle_alert = 1;
	}

	if (dungeon_runtime.travel_progress >= 100) {
		dungeon_runtime.travel_progress = 100;
		if (dungeon_runtime.support_pending) {
			dungeon_runtime.support_pending = 0;
			dungeon_trigger_support();
		}
	}
}

/* Terminal check for the run, requested by dungeon_state when the hero drops. */
void dungeon_check_game_over() {
	if (dungeon_runtime.player_hp > 0) {
		return;
	}

	dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_INPUT;
	dungeon_runtime.battle_wait_ticks = 0;
	dungeon_runtime.defend_icon_active = 0;
	dungeon_set_message("Hero has fallen", "GAME OVER", "MODE TO EXIT", DUNGEON_NEXT_LOSE);
	BUZZER_PlayTones(tones_3beep);
}

void dungeon_after_battle_win() {
	dungeon_runtime.battle_phase = DUNGEON_BATTLE_PHASE_INPUT;
	dungeon_runtime.battle_wait_ticks = 0;
	dungeon_runtime.defend_icon_active = 0;
	dungeon_game_score += 20 + (dungeon_runtime.level * 10);
	if (dungeon_runtime.stage >= dungeon_runtime.total_stages) {
		dungeon_set_message("LEVEL COMPLETE", "All monsters down", "MODE TO FINISH", DUNGEON_NEXT_WIN);
	}
	else {
		dungeon_set_message("Monster defeated", "Stage cleared", "MODE NEXT STAGE", DUNGEON_NEXT_STAGE);
	}
	dungeon_save_progress();
}

void dungeon_finish_game(uint8_t outcome) {
	dungeon_last_outcome = outcome;
	if (dungeon_persist_enabled) {
		dungeon_clear_save();
	}
	if (outcome == DUNGEON_OUTCOME_WIN) {
		task_post_pure_msg(DUNGEON_SCREEN_ID, DUNGEON_LAND_SUCCESS);
	}
	else {
		task_post_pure_msg(DUNGEON_SCREEN_ID, DUNGEON_RESET);
	}
}

void dungeon_advance_stage() {
	dungeon_runtime.stage++;
	if (dungeon_runtime.stage > dungeon_runtime.total_stages) {
		dungeon_finish_game(DUNGEON_OUTCOME_WIN);
		return;
	}
	dungeon_prepare_stage();
}

void dungeon_setup_session() {
	dungeon_last_outcome = DUNGEON_OUTCOME_NONE;
	dungeon_load_setting();
	dungeon_persist_enabled = 1;

	if ((dungeon_start_mode == DUNGEON_START_CONTINUE) && dungeon_restore_save()) {
		dungeon_game_score = dungeon_get_score_value();
		return;
	}

	if (dungeon_start_mode == DUNGEON_START_CREATOR) {
		dungeon_persist_enabled = 0;
		dungeon_init_player(dungeon_selected_level);
	}
	else if (dungeon_start_mode == DUNGEON_START_LEVEL) {
		dungeon_init_player(dungeon_selected_level);
	}
	else {
		dungeon_init_player(1);
		dungeon_selected_level = 1;
	}
}

void dungeon_reset_session() {
	memset(&dungeon_runtime, 0, sizeof(dungeon_runtime));
	dungeon_message_next = DUNGEON_NEXT_NONE;
}

uint8_t dungeon_get_current_stage() {
	return dungeon_runtime.stage;
}

uint8_t dungeon_get_total_stages() {
	return dungeon_runtime.total_stages;
}

uint8_t dungeon_get_level_value() {
	return dungeon_runtime.level;
}

uint32_t dungeon_get_score_value() {
	return dungeon_game_score;
}

void dungeon_save_and_reset_score() {
	if (dungeon_persist_enabled) {
		dungeon_last_score_write(dungeon_game_score);
	}
	dungeon_game_score = 0;
}

void dungeon_reset_objects() {
	task_post_pure_msg(DUNGEON_CONTROL_ID, DUNGEON_CONTROL_RESET);
	task_post_pure_msg(DUNGEON_ACTION_ID, DUNGEON_ACTION_RESET);
	task_post_pure_msg(DUNGEON_STATE_ID, DUNGEON_STATE_RESET);
	task_post_pure_msg(DUNGEON_EFFECT_ID, DUNGEON_EFFECT_RESET);
	task_post_pure_msg(DUNGEON_LANE_ID, DUNGEON_LANE_RESET);
}

void dungeon_lane_handle(ak_msg_t* msg) {
	switch (msg->sig) {
	case DUNGEON_LANE_SETUP: {
		APP_DBG_SIG("DUNGEON_LANE_SETUP\n");
		dungeon_setup_session();
	}
		break;

	case DUNGEON_LANE_LEVEL_UP: {
		APP_DBG_SIG("DUNGEON_LANE_LEVEL_UP\n");
		dungeon_advance_travel();
	}
		break;

	case DUNGEON_LANE_CHECK_GAME_OVER: {
		APP_DBG_SIG("DUNGEON_LANE_CHECK_GAME_OVER\n");
		dungeon_check_game_over();
	}
		break;

	case DUNGEON_LANE_RESET: {
		APP_DBG_SIG("DUNGEON_LANE_RESET\n");
		dungeon_reset_session();
	}
		break;

	default:
		break;
	}
}
