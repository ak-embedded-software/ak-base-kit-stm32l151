#ifndef __VIEW_ITEM_H__
#define __VIEW_ITEM_H__

#include <stdint.h>

/* globals screen define */
#define NUMBER_SCREEN_ITEMS_MAX		3
#define VIEW_SCREEN_NULL			((view_screen_t*)0)

#define ITEM_NULL					((view_item_t*)0)

/* globals item define */
#define ITEM_TYPE_RECTANGLE			(0x00)
#define ITEM_TYPE_DYNAMIC			(0x01)
#define NUMBERS_ITEM_TYPE			(0x02)

/* define rectangle item */
#define RECTANGLE_TEXT_LENGH_MAX	9


/* define dynamic item */
typedef void (*dyn_render)();

typedef struct {
	uint8_t item_type;
} view_item_t;

typedef struct {
	view_item_t item;

	char	text[RECTANGLE_TEXT_LENGH_MAX];
	uint8_t text_color;
	uint8_t	font_size;

	uint8_t	focus_cursor;
	uint8_t	focus_size;

	uint8_t	type;

	uint8_t	x;
	uint8_t	y;
	uint8_t	width;
	uint8_t	height;

	uint8_t	border_width;
} view_rectangle_t;


typedef struct {
	view_item_t item;
	dyn_render render;
} view_dynamic_t;


typedef struct {
	void* item[NUMBER_SCREEN_ITEMS_MAX];
	uint8_t focus_item;
} view_screen_t;

#endif //__VIEW_ITEM_H__
