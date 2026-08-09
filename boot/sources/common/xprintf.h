/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   13/08/2016
 ******************************************************************************
**/
#ifndef __XPRINTF_H__
#define __XPRINTF_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <stdint.h>

extern void (*xfunc_out)(uint8_t);
extern void xprintf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif //__XPRINTF_H__
