/***********************************************************************
    MICRO C CUBE / COMPACT, DEVICE DRIVER
    Interval Timer Driver for Cortex-M
    Copyright (c)  2011-2012, eForce Co., Ltd. All rights reserved.

    Version Information
            2011.09.02: Created.
            2012.11.30: Removed comment in japanese.
 ***********************************************************************/

#include "kernel.h"

#include "DDR_M_SysTick_cfg.h"


/*******************************
    Systick ISR
 *******************************/

void _ddr_m_systick(VP_INT exinf)
{
    if ((REG_SYSTICK.CTRSTS & 0x00010000) != 0) {
        isig_tim();
    }
}

/*******************************
    Initialize
 *******************************/

void _ddr_m_systick_init(void)
{
    REG_SYSTICK.CTRSTS = 0x00000000;
    REG_SYSTICK.CURRENT = 0;
    REG_SNVIC.SYSPRI[2] = (REG_SNVIC.SYSPRI[2] & 0x00FFFFFF) | ((UW)TICKPRI << 24);
    REG_SYSTICK.RELOAD = RELOADV-1;
    REG_SYSTICK.CTRSTS = 0x00000007;
}
