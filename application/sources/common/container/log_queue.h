/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   17/11/2016
 ******************************************************************************
**/

#ifndef __LOG_QUEUE_H__
#define __LOG_QUEUE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define LOG_QUEUE_RET_OK		(0x00)
#define LOG_QUEUE_RET_NG		(0x01)

typedef uint8_t (*q_ctrl)(uint32_t, uint8_t* , uint32_t);

typedef struct {
	/* public */
	uint32_t	base;

	uint32_t	head;
	uint32_t	tail;

	uint32_t	q_len;
	uint32_t	e_len;

	q_ctrl		read;
	q_ctrl		write;

	/* private */
	uint32_t	counter;
	uint32_t	end;

} log_queue_t;

uint8_t		log_queue_init(log_queue_t* q, uint32_t b, uint32_t q_l, uint32_t e_l, q_ctrl f_w, q_ctrl f_r);
uint8_t		log_queue_put(log_queue_t* q, void* d);
uint8_t		log_queue_get(log_queue_t* q, void* d);
uint32_t	log_queue_len(log_queue_t* q);

#ifdef __cplusplus
}
#endif

#endif //__LOG_QUEUE_H__
