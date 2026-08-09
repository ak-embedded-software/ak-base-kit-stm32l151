/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   17/11/2016
 ******************************************************************************
**/

#include "log_queue.h"
#include "app_dbg.h"

uint8_t log_queue_init(log_queue_t* q, uint32_t b, uint32_t q_l, uint32_t e_l, q_ctrl f_w, q_ctrl f_r) {
	q->base		= b;
	q->head		= b;
	q->tail		= b;

	q->q_len	= q_l;
	q->e_len	= e_l;

	q->counter	= 0;
	q->end		= b + ((q_l - 1) * e_l);

	if (f_w) {
		q->write = f_w;
	}
	else {
		return LOG_QUEUE_RET_NG;
	}

	if (f_r) {
		q->read = f_r;
	}
	else {
		return LOG_QUEUE_RET_NG;
	}

	return LOG_QUEUE_RET_OK;
}

uint32_t log_queue_len(log_queue_t* q) {
	return q->counter;
}

uint8_t log_queue_put(log_queue_t* q, void* d) {

	/* check invalib address */
	if ((q->head < q->base) || (q->head > q->end)) {
		return LOG_QUEUE_RET_NG;
	}

	/* write data to queue */
	if (q->write) {
		q->write(q->head, (uint8_t*)d, q->e_len);
	}

	/* overwrite tail */
	if (q->tail == q->head && q->counter == q->q_len) {
		if (q->tail == q->end) {
			q->tail = q->base;
		}
		else {
			q->tail = q->tail + q->e_len;
		}
	}

	/* determine next address of head */
	if (q->head == q->end) {
		q->head = q->base;
	}
	else {
		q->head = q->head + q->e_len;
	}

	/* increase counter */
	q->counter++;
	if (q->counter > q->q_len) {
		q->counter = q->q_len;
	}

	return LOG_QUEUE_RET_OK;
}

uint8_t log_queue_get(log_queue_t* q, void* d) {

	/* check data exist */
	if (!q->counter) {
		return LOG_QUEUE_RET_NG;
	}

	/* read data from queue */
	if (q->read) {
		q->read(q->tail, (uint8_t*)d, q->e_len);
	}
	if (q->tail == q->end) {
		q->tail = q->base;
	}
	else {
		q->tail = q->tail + q->e_len;
	}

	/* descrease counter */
	q->counter--;

	return LOG_QUEUE_RET_OK;
}
