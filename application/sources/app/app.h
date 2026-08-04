/**
 ******************************************************************************
 * @author: An Nguyen Khanh
 * @date:   13/08/2016
 ******************************************************************************
**/

#ifndef __APP_H__
#define __APP_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "ak.h"


#include "app_if.h"
#include "app_eeprom.h"
#include "app_data.h"

/*****************************************************************************/
/* SYSTEM task define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	SYSTEM_AK_FLASH_UPDATE_REQ = AK_USER_DEFINE_SIG,
};

/*****************************************************************************/
/* FIRMWARE task define
 */
/*****************************************************************************/
/* define timer */
#define FW_PACKED_TIMEOUT_INTERVAL			(5000)
#define FW_UPDATE_REQ_INTERVAL				(5000)

/* define signal */
enum {
	FW_CRENT_APP_FW_INFO_REQ = AK_USER_DEFINE_SIG,
	FW_CRENT_BOOT_FW_INFO_REQ,
	FW_UPDATE_REQ,
	FW_UPDATE_SM_OK,
	FW_TRANSFER_REQ,
	FW_INTERNAL_UPDATE_APP_RES_OK,
	FW_INTERNAL_UPDATE_BOOT_RES_OK,
	FW_SAFE_MODE_RES_OK,
	FW_UPDATE_SM_BUSY,
	FW_PACKED_TIMEOUT,
	FW_CHECKING_REQ
};

/*****************************************************************************/
/*  LIFE task define
 */
/*****************************************************************************/
/* define timer */
#define AC_LIFE_TASK_TIMER_LED_LIFE_INTERVAL		(1000)

/* define signal */
enum {
	AC_LIFE_SYSTEM_CHECK = AK_USER_DEFINE_SIG,
};

/*****************************************************************************/
/*  SHELL task define
 */
/*****************************************************************************/
/* define timer */

/* define signal */
enum {
	AC_SHELL_LOGIN_CMD = AK_USER_DEFINE_SIG,
	AC_SHELL_REMOTE_CMD,
};

/*****************************************************************************/
/* IF task define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	AC_IF_PURE_MSG_IN = AK_USER_DEFINE_SIG,
	AC_IF_PURE_MSG_OUT,
	AC_IF_COMMON_MSG_IN,
	AC_IF_COMMON_MSG_OUT,
	AC_IF_DYNAMIC_MSG_IN,
	AC_IF_DYNAMIC_MSG_OUT,
};

/*****************************************************************************/
/* UART_IF task define
 */
/*****************************************************************************/
/* timer signal */
/* define signal */

enum {
	AC_UART_IF_INIT = AK_USER_DEFINE_SIG,
	AC_UART_IF_PURE_MSG_OUT,
	AC_UART_IF_COMMON_MSG_OUT,
	AC_UART_IF_DYNAMIC_MSG_OUT,
	AC_UART_IF_PURE_MSG_IN,
	AC_UART_IF_COMMON_MSG_IN,
	AC_UART_IF_DYNAMIC_MSG_IN,
};

/*****************************************************************************/
/*  SCREENS task define
 */
/*****************************************************************************/
/* define timer */
#define AC_DISPLAY_INITIAL_INTERVAL									(100)
#define AC_DISPLAY_STARTUP_INTERVAL									(2000)
#define AC_DISPLAY_IDLE_INTERVAL									(20000)
#define AC_DISPLAY_LOGO_INTERVAL									(10000)
#define AC_DISPLAY_SHOW_IDLE_BALL_MOVING_UPDATE_INTERAL				(150)
#define AC_DISPLAY_MINIMUM_SCREEN_RENDER_INTERVAL_MS				(50) /* 50ms => Max 20 FPS */
#define AC_DISPLAY_TITLE_BLINK_INTERVAL								(500) /* nửa chu kỳ nhấp nháy */

/* define signal
 *
 * Signal space of the display task:
 *   [0]      AK_SYS_DEFINE_SIG  - internal render request
 *   [1..9]                      - button events (posted from app_bsp callbacks)
 *   [10..]   AK_USER_DEFINE_SIG - screen level signals
 */
enum {
	/* Render screen signal */
	AC_DISPLAY_RENDER_SCREEN = AK_SYS_DEFINE_SIG,

	/* Button signals */
	AC_DISPLAY_BUTTON_MODE_PRESSED,
	AC_DISPLAY_BUTTON_MODE_LONG_PRESSED,
	AC_DISPLAY_BUTTON_MODE_RELEASED,
	AC_DISPLAY_BUTTON_UP_PRESSED,
	AC_DISPLAY_BUTTON_UP_LONG_PRESSED,
	AC_DISPLAY_BUTTON_UP_RELEASED,
	AC_DISPLAY_BUTTON_DOWN_PRESSED,
	AC_DISPLAY_BUTTON_DOWN_LONG_PRESSED,
	AC_DISPLAY_BUTTON_DOWN_RELEASED,

	/* Screen signals */
	AC_DISPLAY_INITIAL = AK_USER_DEFINE_SIG,
	AC_DISPLAY_SHOW_LOGO,
	AC_DISPLAY_SHOW_IDLE,
	AC_DISPLAY_SHOW_IDLE_BALL_MOVING_UPDATE,
	AC_DISPLAY_SHOW_FW_UPDATE,
	AC_DISPLAY_SHOW_FW_UPDATE_ERR,

	/* Thêm ở CUỐI enum, đừng chèn vào giữa.
	 * Mấy giá trị trên đã nằm trong firmware đang chạy ngoài bo, chèn giữa là
	 * đổi số của tất cả signal đứng sau nó. */
	AC_DISPLAY_TITLE_BLINK,
};

/*****************************************************************************/
/*  Dungeon game 'SCREEN' task define
 */
/*****************************************************************************/
/* define timer */
#define DUNGEON_TIME_TICK_INTERVAL									(100)
#define DUNGEON_TIME_EXIT_INTERVAL									(3000)
/* define signal */
enum {
	DUNGEON_INITIAL_SETUP = AK_USER_DEFINE_SIG,
	DUNGEON_TIME_TICK = DUNGEON_DEFINE_SIG,
	DUNGEON_RESET,
	DUNGEON_LAND_SUCCESS,
	DUNGEON_EXIT_GAME,
};

/*****************************************************************************/
/*  Dungeon game control task define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	DUNGEON_CONTROL_SETUP = DUNGEON_DEFINE_SIG,
	DUNGEON_CONTROL_UPDATE,
	DUNGEON_CONTROL_UP,
	DUNGEON_CONTROL_DOWN,
	DUNGEON_CONTROL_RESET,
};

/*****************************************************************************/
/*  Dungeon game action task define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	DUNGEON_ACTION_SETUP = DUNGEON_DEFINE_SIG,
	DUNGEON_ACTION_RUN,
	DUNGEON_ACTION_SHOOT,
	DUNGEON_ACTION_RESET,
};

/*****************************************************************************/
/*  Dungeon game effect task define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	DUNGEON_EFFECT_SETUP = DUNGEON_DEFINE_SIG,
	DUNGEON_EFFECT_UPDATE,
	DUNGEON_EFFECT_RESET,
};

/*****************************************************************************/
/*  Dungeon game lane task define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	DUNGEON_LANE_SETUP = DUNGEON_DEFINE_SIG,
	DUNGEON_LANE_LEVEL_UP,
	DUNGEON_LANE_CHECK_GAME_OVER,
	DUNGEON_LANE_RESET,
};

/*****************************************************************************/
/*  Dungeon game state task define
 */
/*****************************************************************************/
/* define timer */
/* define signal */
enum {
	DUNGEON_STATE_SETUP = DUNGEON_DEFINE_SIG,
	DUNGEON_STATE_RUN,
	DUNGEON_STATE_DETONATOR,
	DUNGEON_STATE_RESET,
};

/*****************************************************************************/
/*  app function declare
 */
/*****************************************************************************/
#define APP_MAGIC_NUMBER	0xAABBCCDD
#define APP_VER				{0, 0, 0, 3}

typedef struct {
	uint32_t magic_number;
	uint8_t version[4];
} app_info_t;

extern const app_info_t app_info;

extern void* app_get_boot_share_data();
extern int  main_app();

#ifdef __cplusplus
}
#endif

#endif //__APP_H__
