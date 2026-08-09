#ifndef __APP_DBG_H__
#define __APP_DBG_H__

#include "xprintf.h"

#if defined(APP_DBG_EN)
#define APP_DBG(fmt, ...)       xprintf("[DBG] " fmt, ##__VA_ARGS__)
#else
#define APP_DBG(fmt, ...)
#endif

#if defined(APP_PRINT_EN)
#define APP_PRINT(fmt, ...)       xprintf("[PRINT] " fmt, ##__VA_ARGS__)
#else
#define APP_PRINT(fmt, ...)
#endif

#if defined(LOGIN_PRINT_EN)
#define LOGIN_PRINT(fmt, ...)       xprintf(fmt, ##__VA_ARGS__)
#else
#define LOGIN_PRINT(fmt, ...)
#endif

#if defined(APP_DBG_SIG_EN)
#define APP_DBG_SIG(fmt, ...)       xprintf("-SIG-> " fmt, ##__VA_ARGS__)
#else
#define APP_DBG_SIG(fmt, ...)
#endif

#endif //__APP_DBG_H__
