/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   31/11/2016
 ******************************************************************************
**/

#ifndef __SCREEN_MANAGER_H__
#define __SCREEN_MANAGER_H__

#include <stdint.h>
#include "ak.h"
#include "message.h"

#include "view_render.h"

#define SCREEN_ENTRY		(0xFE)
#define SCREEN_EXIT			(0xFF)

#define SCREEN_MANAGER_NULL		((scr_mng_t*)0)

typedef void (*screen_f)(ak_msg_t* msg);

typedef struct {
	screen_f	screen;
} scr_mng_t;

#define SCREEN_CTOR(me, init_scr_layout, scr_obj)		\
		scr_mng_ctor(me, init_scr_layout, scr_obj)

#define SCREEN_TRAN(target, scr_obj)		\
		scr_mng_tran(target, scr_obj)

#define SCREEN_BACK()		\
		scr_mng_back()

#define SCREEN_DISPATCH(msg)		\
		scr_mng_dispatch(msg)

#define SCREEN_NONE_UPDATE_MASK()		\
		scr_mng_contain_screen_none_update_mark()

extern void scr_mng_ctor(scr_mng_t* scr_mng, screen_f init_scr, view_screen_t* scr_obj);
extern void scr_mng_tran(screen_f target, view_screen_t* scr_obj);
extern void scr_mng_back();
extern void scr_mng_contain_screen_none_update_mark();
extern void scr_mng_dispatch(ak_msg_t* msg);
extern screen_f scr_mng_get_current_screen();
extern view_screen_t* scr_mng_get_current_view_screen();

#endif //__SCREEN_MANAGER_H__
