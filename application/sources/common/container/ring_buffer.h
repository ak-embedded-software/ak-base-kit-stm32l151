#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define RET_RING_BUFFER_OK				(0x01)
#define RET_RING_BUFFER_NG				(0x00)

typedef struct {
	uint16_t tail_index;
	uint16_t head_index;
	uint16_t fill_size;
	uint16_t buffer_size;
	uint16_t element_size;
	uint8_t* buffer;
} ring_buffer_t;

typedef struct {
	uint16_t tail_index;
	uint16_t head_index;
	uint16_t fill_size;
	uint16_t buffer_size;
	uint8_t* buffer;
} ring_buffer_char_t;

extern void		ring_buffer_init(ring_buffer_t* ring_buffer, void* buffer, uint16_t buffer_size, uint16_t element_size);
extern uint16_t	ring_buffer_availble(ring_buffer_t* ring_buffer);
extern bool		ring_buffer_is_empty(ring_buffer_t* ring_buffer);
extern bool		ring_buffer_is_full(ring_buffer_t* ring_buffer);
extern uint8_t	ring_buffer_put(ring_buffer_t* ring_buffer, void* data);
extern uint8_t	ring_buffer_get(ring_buffer_t* ring_buffer, void* data);

extern void		ring_buffer_char_init(ring_buffer_char_t* ring_buffer, void* buffer, uint16_t buffer_size);
extern uint16_t	ring_buffer_char_availble(ring_buffer_char_t* ring_buffer);
extern bool		ring_buffer_char_is_empty(ring_buffer_char_t* ring_buffer);
extern bool		ring_buffer_char_is_full(ring_buffer_char_t* ring_buffer);
extern void		ring_buffer_char_put(ring_buffer_char_t* ring_buffer, uint8_t c);
extern uint8_t	ring_buffer_char_get(ring_buffer_char_t* ring_buffer);

#ifdef __cplusplus
}
#endif

#endif //__RING_BUFFER_H__
