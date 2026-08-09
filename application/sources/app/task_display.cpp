#include "fsm.h"
#include "port.h"
#include "message.h"
#include "timer.h"

#include "app.h"
#include "app_dbg.h"

#include "task_list.h"
#include "task_display.h"

scr_mng_t scr_mng_app;

void task_display(ak_msg_t* msg) {
	scr_mng_dispatch(msg);
}
