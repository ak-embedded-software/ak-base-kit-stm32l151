/***********************************************************************
    Interval Timer code for Cortex-M SysTick
 ***********************************************************************/

/* SysTickÇÃê›íËíl */

#define RELOADV     144000	/* Divider ratio per 1msec */
#define TICKPRI     0xf0	/* Priority */

#include "Cortex-M3.h"
