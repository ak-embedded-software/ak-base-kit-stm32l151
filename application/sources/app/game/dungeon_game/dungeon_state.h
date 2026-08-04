/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 03/05/2026
 *          End:   04/05/2026
 ******************************************************************************
**/
#ifndef __DUNGEON_STATE_H__
#define __DUNGEON_STATE_H__

#include <stdio.h>

#include "fsm.h"
#include "port.h"
#include "message.h"
#include "timer.h"

#include "app.h"
#include "app_dbg.h"
#include "task_list.h"
#include "task_display.h"

#include "buzzer.h"
#include "scr_dungeon_game.h"




extern void dungeon_state_handle(ak_msg_t* msg);

#endif //__DUNGEON_STATE_H__
