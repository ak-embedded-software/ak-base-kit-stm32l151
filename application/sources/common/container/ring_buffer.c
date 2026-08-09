#include <stdbool.h>
#include <stdlib.h>

#include "ring_buffer.h"
#include "sys_dbg.h"


void ring_buffer_init(ring_buffer_t* ring_buffer, void* buffer, uint16_t buffer_size, uint16_t element_size) {
	ring_buffer->tail_index = 0;
	ring_buffer->head_index = 0;
	ring_buffer->fill_size = 0;

	ring_buffer->buffer_size = buffer_size;
	ring_buffer->buffer = buffer;
	ring_buffer->element_size = element_size;
}

uint16_t ring_buffer_availble(ring_buffer_t* ring_buffer) {
	return ring_buffer->fill_size;
}

bool ring_buffer_is_empty(ring_buffer_t* ring_buffer) {
	return (ring_buffer->fill_size == 0) ? true : false;
}

bool ring_buffer_is_full(ring_buffer_t* ring_buffer) {
	return (ring_buffer->fill_size == ring_buffer->buffer_size) ? true : false;
}

uint8_t ring_buffer_put(ring_buffer_t* ring_buffer, void* data) {
	uint16_t next_tail_index;
	uint16_t next_head_index;

	if (data != NULL) {
		memcpy((uint8_t*)(ring_buffer->buffer + ring_buffer->tail_index * ring_buffer->element_size), (uint8_t*)data, ring_buffer->element_size);

		next_tail_index = (++ring_buffer->tail_index) % ring_buffer->buffer_size;
		ring_buffer->tail_index = next_tail_index;

		if (ring_buffer->fill_size == ring_buffer->buffer_size) {
			next_head_index = (++ring_buffer->head_index) % ring_buffer->buffer_size;
			ring_buffer->head_index = next_head_index;
		}
		else {
			ring_buffer->fill_size++;
		}
	}
	else {
		return RET_RING_BUFFER_NG;
	}

	return RET_RING_BUFFER_OK;
}

uint8_t ring_buffer_get(ring_buffer_t* ring_buffer, void* data) {
	uint16_t next_head_index;

	if (ring_buffer_is_empty(ring_buffer)) {
		return RET_RING_BUFFER_NG;
	}

	if (data != NULL) {
		memcpy((uint8_t*)data, (uint8_t*)(ring_buffer->buffer + ring_buffer->head_index * ring_buffer->element_size), ring_buffer->element_size);

		next_head_index = (++ring_buffer->head_index) % ring_buffer->buffer_size;
		ring_buffer->head_index = next_head_index;

		ring_buffer->fill_size--;
	}
	else {
		return RET_RING_BUFFER_NG;
	}

	return RET_RING_BUFFER_OK;
}

void ring_buffer_char_init(ring_buffer_char_t* ring_buffer, void* buffer, uint16_t buffer_size) {
	ring_buffer->tail_index = 0;
	ring_buffer->head_index = 0;
	ring_buffer->fill_size = 0;

	ring_buffer->buffer_size = buffer_size;
	ring_buffer->buffer = buffer;
}

uint16_t ring_buffer_char_availble(ring_buffer_char_t* ring_buffer) {
	return ring_buffer->fill_size;
}

bool ring_buffer_char_is_empty(ring_buffer_char_t* ring_buffer) {
	return (ring_buffer->fill_size == 0) ? true : false;
}

bool ring_buffer_char_is_full(ring_buffer_char_t* ring_buffer) {
	return (ring_buffer->fill_size == ring_buffer->buffer_size) ? true : false;
}

void ring_buffer_char_put(ring_buffer_char_t* ring_buffer, uint8_t c) {
	uint16_t next_tail_index;
	uint16_t next_head_index;

	ring_buffer->buffer[ring_buffer->tail_index] = c;

	next_tail_index = (++ring_buffer->tail_index) % ring_buffer->buffer_size;
	ring_buffer->tail_index = next_tail_index;

	if (ring_buffer->fill_size == ring_buffer->buffer_size) {
		next_head_index = (++ring_buffer->head_index) % ring_buffer->buffer_size;
		ring_buffer->head_index = next_head_index;
	}
	else {
		ring_buffer->fill_size++;
	}
}

uint8_t	ring_buffer_char_get(ring_buffer_char_t* ring_buffer) {
	uint16_t ret = 0;
	uint16_t next_head_index;

	if (ring_buffer->fill_size) {
		ret = ring_buffer->buffer[ring_buffer->head_index];

		next_head_index = (++ring_buffer->head_index) % ring_buffer->buffer_size;
		ring_buffer->head_index = next_head_index;

		ring_buffer->fill_size--;
	}

	return ret;
}
