#include "view_render.h"
Adafruit_oled_drv view_render;

static int view_render_rectangle(void* rectangle);
static int view_render_dynamic(void* dynamic);

static uint8_t number_item;

static view_render_item render_list[] = {
	view_render_rectangle,
	view_render_dynamic
};

void view_render_init() {
	number_item = sizeof(render_list)/sizeof(view_render_item);
	view_render.initialize();
}

int view_render_rectangle(void* rectangle) {
	uint8_t x, y, len;
	uint8_t x_forcus, y_forcus, w_forcus, h_forcus;

	view_rectangle_t* rect = (view_rectangle_t*)rectangle;
	if (rect == NULL) {
		return (-1);
	}

	/*paint back ground of object screen on lcd*/
	switch (rect->type) {
	case BACK_GND_STYLE_FILL:
		view_render.fillRect(rect->x, rect->y, rect->width, rect->height, WHITE);
		view_render.setTextColor(BLACK);
		break;

	case BACK_GND_STYLE_OUTLINE:
		view_render.drawRect(rect->x, rect->y, rect->width, rect->height, WHITE);
		view_render.setTextColor(WHITE);
		break;

	case BACK_GND_STYLE_NONE_OUTLINE:
		view_render.setTextColor(WHITE);
		break;

	default:
		break;
	}

	len = strlen((const char*)rect->text);

	x = rect->x + (rect->width - len * X_SIZE_FONT * rect->font_size )/2 - len;
	y = rect->y + (rect->height - rect->font_size * Y_SIZE_FONT) / 2;

	/*set font size for object screen*/
	view_render.setTextSize(rect->font_size);

	/*set cursor on lcd for object screen*/
	view_render.setCursor(x,y);

	/*print data of object screen on lcd*/
	view_render.print(rect->text);

	/*paint back ground of object screen when setting*/
	if (rect->border_width !=  0) {
		x_forcus = rect->focus_cursor * X_SIZE_FONT * rect->font_size + rect->focus_cursor* 2 + x - 2;
		y_forcus = rect->y + 1;

		w_forcus = rect->focus_size * (X_SIZE_FONT * rect->font_size + 3);
		h_forcus = rect->height - 2;

		if (rect->type == BACK_GND_STYLE_FILL) {
			view_render.drawRect(x_forcus, y_forcus, w_forcus, h_forcus, BLACK);
		}
		else if (rect->type == BACK_GND_STYLE_OUTLINE) {
			view_render.drawRect(x_forcus, y_forcus, w_forcus, h_forcus, WHITE);
		}
	}

	return 0;
}

int view_render_dynamic(void* dynamic) {
	((view_dynamic_t*)dynamic)->render();
	return 0;
}

int view_render_screen(view_screen_t* screen) {
	view_render.clear();

	if ((view_item_t*)screen->item[screen->focus_item] != ITEM_NULL) {
		if (((view_item_t*)screen->item[screen->focus_item])->item_type == ITEM_TYPE_RECTANGLE) {
			((view_rectangle_t*) screen->item[screen->focus_item])->type = BACK_GND_STYLE_FILL;
		}
	}

	for (uint8_t i = 0; i < NUMBER_SCREEN_ITEMS_MAX; i++ ) {
		if ((view_item_t*)screen->item[i] != ITEM_NULL) {
			if (((view_item_t*)screen->item[i])->item_type == ITEM_TYPE_RECTANGLE) {
				if (i != screen->focus_item && ((view_rectangle_t*) screen->item[i])->type != BACK_GND_STYLE_NONE_OUTLINE) {
					((view_rectangle_t*) screen->item[i])->type = BACK_GND_STYLE_OUTLINE;
				}
			}
		}
	}

	for (int i = 0; i < NUMBER_SCREEN_ITEMS_MAX; i++) {
		if ((view_item_t*)screen->item[i] != ITEM_NULL) {
			render_list[((view_item_t*)screen->item[i])->item_type]((view_item_t*)screen->item[i]);
		}
	}

	view_render.update();

	return 0;
}

void view_render_display_on() {
	view_render.display_on();
}

void view_render_display_off() {
	view_render.display_off();
}
