/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   05/09/2016
 ******************************************************************************
**/
#ifndef __SYS_DBG_H__
#define __SYS_DBG_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "xprintf.h"

#if defined(SYS_DBG_EN)
#define SYS_DBG(fmt, ...)       xprintf((const char*)fmt, ##__VA_ARGS__)
#else
#define SYS_DBG(fmt, ...)
#endif

#if defined(SYS_PRINT_EN)
#define SYS_PRINT(fmt, ...)       xprintf((const char*)fmt, ##__VA_ARGS__)
#else
#define SYS_PRINT(fmt, ...)
#endif

#define FATAL(s, c) \
do { \
	DISABLE_INTERRUPTS(); \
	sys_dbg_fatal((const int8_t*)s, (uint8_t)c); \
} while (0);

extern void sys_dbg_fatal(const int8_t* s, uint8_t c);

#ifdef __cplusplus
}
#endif

#endif //__SYS_DBG_H__
