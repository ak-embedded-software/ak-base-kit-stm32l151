/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   Start: 04/05/2026
 *          End:   04/05/2026
 ******************************************************************************
**/
#ifndef __DUNGEON_ACTION_H__
#define __DUNGEON_ACTION_H__

#include <stdio.h>

#include "fsm.h"
#include "port.h"
#include "message.h"

#include "app.h"
#include "app_dbg.h"
#include "task_list.h"
#include "task_display.h"

#include "buzzer.h"
#include "scr_dungeon_game.h"




extern void dungeon_action_handle(ak_msg_t* msg);

#endif //__DUNGEON_ACTION_H__
