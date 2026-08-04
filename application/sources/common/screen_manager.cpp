/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   31/11/2016
 ******************************************************************************
**/

#include "ak.h"
#include "message.h"

#include "screen_manager.h"

#include "sys_ctrl.h"
#include "sys_dbg.h"
#include "task_list.h"
#include "timer.h"

static ak_msg_t screen_msg_entry;

static scr_mng_t* screen_manager = SCREEN_MANAGER_NULL;
static view_screen_t* view_screen = VIEW_SCREEN_NULL;

static bool screen_render_started = true;
static uint32_t screen_last_render_ms = 0;

/* Rate limit the OLED refresh.
 *
 * A full refresh pushes the whole 1024-byte framebuffer over bit-banged I2C and
 * blocks the calling task while it does so. Rendering once per dispatched
 * message means a burst of messages turns into a burst of full-frame writes.
 *
 * Instead: render immediately if the minimum interval has elapsed, otherwise
 * arm a one-shot timer for the remainder. Bursts within one interval collapse
 * into a single render because timer_set() replaces the pending attribute. */
static void scr_mng_render_screen() {
	uint32_t current_ms = sys_ctrl_millis();
	uint32_t time_diff = current_ms - screen_last_render_ms;

	if (screen_render_started || \
		(time_diff >= AC_DISPLAY_MINIMUM_SCREEN_RENDER_INTERVAL_MS)) \
	{
		screen_render_started = false;
		screen_last_render_ms = current_ms;
		view_render_screen(view_screen);
	}
	else {
		timer_set(	AC_TASK_DISPLAY_ID, \
					AC_DISPLAY_RENDER_SCREEN, \
					AC_DISPLAY_MINIMUM_SCREEN_RENDER_INTERVAL_MS - time_diff, \
					TIMER_ONE_SHOT);
	}
}

void scr_mng_ctor(scr_mng_t* scr_mng, screen_f init_scr, view_screen_t* scr_obj) {
	/* init entry message */
	screen_msg_entry.sig = SCREEN_ENTRY;

	view_screen = scr_obj;				/* point to current screen object */
	screen_manager = scr_mng;			/* init singleton screen manager */
	screen_manager->screen = init_scr;	/* assign init handler */
}

void scr_mng_dispatch(ak_msg_t* msg) {
	if (screen_manager == SCREEN_MANAGER_NULL) {
		FATAL("SCR_MNG", 0x01);
		return;
	}

	screen_manager->screen(msg);
	scr_mng_render_screen();
}

void scr_mng_tran(screen_f target,  view_screen_t* scr_obj) {
	if (screen_manager == SCREEN_MANAGER_NULL) {
		FATAL("SCR_MNG", 0x01);
		return;
	}

	/* change new screen */
	view_screen = scr_obj;
	screen_manager->screen = target;

	/* entry new screen */
	screen_manager->screen(&screen_msg_entry);
	view_render_screen(view_screen);
}

/* Cancel a render that a previous dispatch deferred - used by screens that
 * know the frame they just produced is already stale. */
void scr_mng_contain_screen_none_update_mark() {
	timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_RENDER_SCREEN);
}

screen_f scr_mng_get_current_screen() {
	return screen_manager->screen;
}
