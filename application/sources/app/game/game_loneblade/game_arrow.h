#ifndef __GAME_ARROW_H__
#define __GAME_ARROW_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	int32_t x_scaled;
	int16_t y;
	int16_t speed;
	int8_t dir;
	bool active;
} Arrow;

void arrow_init();
void arrow_update(uint32_t dt);
void arrow_draw();

#endif // __GAME_ARROW_H__
