/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   13/08/2016
 ******************************************************************************
**/

#include "ak.h"
#include "ak_dbg.h"
#include "message.h"
#include "task.h"

#include "sys_dbg.h"

/* pure pool memory */
static ak_msg_pure_t msg_pure_pool[AK_PURE_MSG_POOL_SIZE];
static ak_msg_t* free_list_pure_msg_pool;
static uint32_t free_list_pure_used;
static uint32_t free_list_pure_used_max;

/* common pool memory */
static ak_msg_common_t msg_common_pool[AK_COMMON_MSG_POOL_SIZE];
static ak_msg_t* free_list_common_msg_pool;
static uint32_t free_list_common_used;
static uint32_t free_list_common_used_max;

/* dynamic pool memory */
static ak_msg_dynamic_t msg_dynamic_pool[AK_DYNAMIC_MSG_POOL_SIZE];
static ak_msg_t* free_list_dynamic_msg_pool;
static uint32_t free_list_dynamic_used;
static uint32_t free_list_dynamic_used_max;

static void pure_msg_pool_init();
static void common_msg_pool_init();
static void dynamic_msg_pool_init();

static void free_pure_msg(ak_msg_t* msg);
static void free_common_msg(ak_msg_t* msg);
static void free_dynamic_msg(ak_msg_t* msg);

void msg_init() {
	pure_msg_pool_init();
	common_msg_pool_init();
	dynamic_msg_pool_init();
}

void msg_free(ak_msg_t* msg) {
	uint8_t pool_type;

	msg_dec_ref_count(msg);

	if (get_msg_ref_count(msg) == 0) {

		pool_type = get_msg_type(msg);

		switch (pool_type) {
		case COMMON_MSG_TYPE:
			free_common_msg(msg);
			break;

		case PURE_MSG_TYPE:
			free_pure_msg(msg);
			break;

		case DYNAMIC_MSG_TYPE:
			free_dynamic_msg(msg);
			break;

		default:
			FATAL("MF", 0x20);
			break;
		}
	}
}

void msg_force_free(ak_msg_t* msg) {
	uint8_t pool_type = get_msg_type(msg);

	switch (pool_type) {
	case COMMON_MSG_TYPE:
		free_common_msg(msg);
		break;

	case PURE_MSG_TYPE:
		free_pure_msg(msg);
		break;

	case DYNAMIC_MSG_TYPE:
		free_dynamic_msg(msg);
		break;

	default:
		FATAL("MF", 0x27);
		break;
	}
}

void msg_inc_ref_count(ak_msg_t* msg) {
	if (get_msg_ref_count(msg) < AK_MSG_REF_COUNT_MAX) {
		msg->ref_count++;
	}
	else {
		FATAL("MF", 0x61);
	}
}

void msg_dec_ref_count(ak_msg_t* msg) {
	if (get_msg_ref_count(msg) > 0) {
		msg->ref_count--;
	}
	else {
		FATAL("MF", 0x28);
	}
}

void* ak_malloc(size_t size) {
	extern uint32_t __heap_end__;
	static uint8_t* ak_heap = NULL;

	if (ak_heap != NULL) {
		if (((uint32_t)ak_heap + size + __AK_MALLOC_CTRL_SIZE) > ((uint32_t)&__heap_end__)) {
			FATAL("ak_malloc", 0x01);
		}
	}

	ak_heap = malloc(size);

	if (ak_heap == NULL) {
		FATAL("ak_malloc", 0x02);
	}

	return ak_heap;
}

void ak_free(void* ptr) {
	free(ptr);
}

/*****************************************************************************
 * pure message function define.
 *****************************************************************************/
void pure_msg_pool_init() {
	uint32_t index;

	ENTRY_CRITICAL();

	free_list_pure_msg_pool = (ak_msg_t*)msg_pure_pool;

	for (index = 0; index < AK_PURE_MSG_POOL_SIZE; index++) {
		msg_pure_pool[index].msg_header.ref_count |= PURE_MSG_TYPE;
		if (index == (AK_PURE_MSG_POOL_SIZE - 1)) {
			msg_pure_pool[index].msg_header.next = AK_MSG_NULL;
		}
		else {
			msg_pure_pool[index].msg_header.next = (ak_msg_t*)&msg_pure_pool[index + 1];
		}
	}

	free_list_pure_used = 0;
	free_list_pure_used_max = 0;

	EXIT_CRITICAL();
}

uint32_t get_pure_msg_pool_used() {
	return free_list_pure_used;
}

uint32_t get_pure_msg_pool_used_max() {
	return free_list_pure_used_max;
}

ak_msg_t* get_pure_msg() {
	ak_msg_t* allocate_message;

	ENTRY_CRITICAL();

	allocate_message = free_list_pure_msg_pool;

	if (allocate_message == AK_MSG_NULL) {
		FATAL("MF", 0x31);
	}
	else {
		free_list_pure_msg_pool = allocate_message->next;

		free_list_pure_used++;
		if (free_list_pure_used >= free_list_pure_used_max) {
			free_list_pure_used_max = free_list_pure_used;
		}
	}

	reset_msg_ref_count(allocate_message);

	allocate_message->ref_count++;
	allocate_message->src_task_id = get_current_task_id();

	EXIT_CRITICAL();

	return allocate_message;
}

void free_pure_msg(ak_msg_t* msg) {

	ENTRY_CRITICAL();

	msg->next = free_list_pure_msg_pool;
	free_list_pure_msg_pool = msg;

	free_list_pure_used--;

	EXIT_CRITICAL();
}

/*****************************************************************************
 * common message function define.
 *****************************************************************************/
void common_msg_pool_init() {
	uint32_t index;

	ENTRY_CRITICAL();

	free_list_common_msg_pool = (ak_msg_t*)msg_common_pool;

	for (index = 0; index < AK_COMMON_MSG_POOL_SIZE; index++) {
		msg_common_pool[index].msg_header.ref_count |= COMMON_MSG_TYPE;
		if (index == (AK_COMMON_MSG_POOL_SIZE - 1)) {
			msg_common_pool[index].msg_header.next = AK_MSG_NULL;
		}
		else {
			msg_common_pool[index].msg_header.next = (ak_msg_t*)&msg_common_pool[index + 1];
		}
	}

	free_list_common_used = 0;
	free_list_common_used_max = 0;

	EXIT_CRITICAL();
}

uint32_t get_common_msg_pool_used() {
	return free_list_common_used;
}

uint32_t get_common_msg_pool_used_max() {
	return free_list_common_used_max;
}

ak_msg_t* get_common_msg() {
	ak_msg_t* allocate_message;

	ENTRY_CRITICAL();

	allocate_message = free_list_common_msg_pool;

	if (allocate_message == AK_MSG_NULL) {
		FATAL("MF", 0x21);
	}
	else {
		free_list_common_msg_pool = allocate_message->next;

		free_list_common_used++;
		if (free_list_common_used >= free_list_common_used_max) {
			free_list_common_used_max = free_list_common_used;
		}
	}

	reset_msg_ref_count(allocate_message);

	allocate_message->ref_count++;
	allocate_message->src_task_id = get_current_task_id();

	((ak_msg_common_t*)allocate_message)->len = 0;

	EXIT_CRITICAL();

	return allocate_message;
}

void free_common_msg(ak_msg_t* msg) {

	ENTRY_CRITICAL();

	msg->next = free_list_common_msg_pool;
	free_list_common_msg_pool = msg;

	free_list_common_used--;

	EXIT_CRITICAL();
}

uint8_t set_data_common_msg(ak_msg_t* msg, uint8_t* data, uint8_t size) {
	if (get_msg_type(msg) != COMMON_MSG_TYPE) {
		FATAL("MF", 0x23);
	}

	if (size > AK_COMMON_MSG_DATA_SIZE) {
		FATAL("MF", 0x24);
	}

	((ak_msg_common_t*)msg)->len = size;
	memcpy(((ak_msg_common_t*)msg)->data, data, size);

	return AK_MSG_OK;
}

uint8_t* get_data_common_msg(ak_msg_t* msg) {

	if (get_msg_type(msg) != COMMON_MSG_TYPE) {
		FATAL("MF", 0x26);
	}

	return ((ak_msg_common_t*)msg)->data;
}

uint8_t get_data_len_common_msg(ak_msg_t* msg) {

	if (get_msg_type(msg) != COMMON_MSG_TYPE) {
		FATAL("MF", 0x38);
	}

	return ((ak_msg_common_t*)msg)->len;
}

/*****************************************************************************
 * dynamic message function define.
 *****************************************************************************/
void dynamic_msg_pool_init() {
	uint32_t index;

	ENTRY_CRITICAL();

	free_list_dynamic_msg_pool = (ak_msg_t*)msg_dynamic_pool;

	for (index = 0; index < AK_DYNAMIC_MSG_POOL_SIZE; index++) {
		msg_dynamic_pool[index].msg_header.ref_count |= DYNAMIC_MSG_TYPE;
		if (index == (AK_DYNAMIC_MSG_POOL_SIZE - 1)) {
			msg_dynamic_pool[index].msg_header.next = AK_MSG_NULL;
		}
		else {
			msg_dynamic_pool[index].msg_header.next = (ak_msg_t*)&msg_dynamic_pool[index + 1];
		}
	}

	free_list_dynamic_used = 0;
	free_list_dynamic_used_max = 0;

	EXIT_CRITICAL();
}

uint32_t get_dynamic_msg_pool_used() {
	return free_list_dynamic_used;
}

uint32_t get_dynamic_msg_pool_used_max() {
	return free_list_dynamic_used_max;
}

void free_dynamic_msg(ak_msg_t* msg) {

	ENTRY_CRITICAL();

	msg->next = free_list_dynamic_msg_pool;
	free_list_dynamic_msg_pool = msg;

	ak_free(((ak_msg_dynamic_t*)msg)->data);

	free_list_dynamic_used--;

	EXIT_CRITICAL();
}

ak_msg_t* get_dynamic_msg() {
	ak_msg_t* allocate_message;

	ENTRY_CRITICAL();

	allocate_message = free_list_dynamic_msg_pool;

	if (allocate_message == AK_MSG_NULL) {
		FATAL("MF", 0x41);
	}
	else {
		free_list_dynamic_msg_pool = allocate_message->next;

		free_list_dynamic_used++;
		if (free_list_dynamic_used >= free_list_dynamic_used_max) {
			free_list_dynamic_used_max = free_list_dynamic_used;
		}
	}

	reset_msg_ref_count(allocate_message);

	allocate_message->ref_count++;
	allocate_message->src_task_id = get_current_task_id();

	((ak_msg_dynamic_t*)allocate_message)->len = 0;
	((ak_msg_dynamic_t*)allocate_message)->data = ((uint8_t*)0);

	EXIT_CRITICAL();

	return allocate_message;
}

uint8_t set_data_dynamic_msg(ak_msg_t* msg, uint8_t* data, uint32_t size) {
	if (get_msg_type(msg) != DYNAMIC_MSG_TYPE) {
		FATAL("MF", 0x43);
	}

	((ak_msg_dynamic_t*)msg)->len = size;
	((ak_msg_dynamic_t*)msg)->data = (uint8_t*)ak_malloc(size);
	memcpy(((ak_msg_dynamic_t*)msg)->data, data, size);

	return AK_MSG_OK;
}

uint8_t* get_data_dynamic_msg(ak_msg_t* msg) {
	if (get_msg_type(msg) != DYNAMIC_MSG_TYPE) {
		FATAL("MF", 0x46);
	}

	return ((ak_msg_dynamic_t*)msg)->data;
}

uint32_t get_data_len_dynamic_msg(ak_msg_t* msg) {
	return ((ak_msg_dynamic_t*)msg)->len;
}

/*****************************************************************************
 * debug message function define.
 *****************************************************************************/
void msg_dbg_dum(ak_msg_t* msg) {
	xprintf("stid:%d dtid:%d rfc:%02X sig:%d Istid:%d Idtid:%d Ist:%d Idt:%d Isig:%d\n",	\
			msg->src_task_id,	\
			msg->des_task_id,	\
			msg->ref_count,		\
			msg->sig,			\
			msg->if_src_task_id,\
			msg->if_des_task_id,\
			msg->if_src_type,	\
			msg->if_des_type,	\
			msg->if_sig			\
			);
}
