#include "fsm.h"
#include "port.h"
#include "message.h"

#include "sys_ctrl.h"
#include "sys_dbg.h"

#include "app.h"
#include "app_dbg.h"

#include "task_list.h"
#include "task_life.h"

led_t led_life;

void task_life(ak_msg_t* msg) {
	switch (msg->sig) {
	case AC_LIFE_SYSTEM_CHECK:
		/* reset watchdog */
		sys_ctrl_independent_watchdog_reset();
		sys_ctrl_soft_watchdog_reset();

#if defined(AK_IO_IRQ_ANALYZER)
#else
		/* toggle led indicator */
		led_toggle(&led_life);
#endif
		break;

	default:
		break;
	}
}
