/*
 * MODBUS Library: MSP430 port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.1 2010-11-28 22:24:38 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"
#include "intrinsics.h"

/* ----------------------- Modbus includes ----------------------------------*/

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

static UBYTE    ubNesting = 0;
static istate_t xLastISRState;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

void
vMBPAssert( void )
{
    volatile BOOL   bBreakOut = FALSE;

    vMBPEnterCritical(  );
    while( !bBreakOut );
}

void
vMBPEnterCritical( void )
{
    istate_t        xCurISRState = __get_interrupt_state(  );

    __disable_interrupt(  );
    if( ubNesting == 0 )
    {
        xLastISRState = xCurISRState;
    }
    ubNesting++;
}

void
vMBPExitCritical( void )
{
    /* Code for disabling interrupts and the scheduler. */
    ubNesting--;
    if( 0 == ubNesting )
    {
        __set_interrupt_state( xLastISRState );
    }
}
