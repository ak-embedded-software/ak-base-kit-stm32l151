/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   05/09/2016
 ******************************************************************************
**/
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef struct {
	uint32_t cpu_clock;
	uint32_t tick;
	uint32_t console_baudrate;
	uint32_t flash_used;
	uint32_t ram_used;
	uint32_t data_used;
	uint32_t stack_used;
	uint32_t heap_size;
} system_info_t;

extern system_info_t system_info;

#ifdef __cplusplus
}
#endif

#endif //__SYSTEM_H__
