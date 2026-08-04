#ifndef __XPRINTF_H__
#define __XPRINTF_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#define xprintf(...)   printf(__VA_ARGS__)
#define xputc(c)       putchar(c)
#define xputs(s)       fputs(s, stdout)
#define xsprintf       sprintf
#define xvprintf       vprintf
#ifdef __cplusplus
}
#endif
#endif
