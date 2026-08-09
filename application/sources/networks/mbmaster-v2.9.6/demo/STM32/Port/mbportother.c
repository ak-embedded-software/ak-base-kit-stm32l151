/*
 * MODBUS Library: ARM STM32 Port (FWLIB 2.0x)
 * Copyright (c) Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * ARM STM32 Port by Niels Andersen, Elcanic A/S <niels.andersen.elcanic@gmail.com>
 *
 * $Id: mbportother.c,v 1.1 2008-12-14 19:33:32 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

#include "platform.h"
#include "sys_dbg.h"

/* ----------------------- Modbus includes ----------------------------------*/

/* ----------------------- Defines ------------------------------------------*/
#define PORT_INTERRUPT_PRIORITY_MAX     ( 1 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
#if 0
static UBYTE    ubNesting = 0;
#endif
/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

void
assert_failed( uint8_t * file, uint32_t line )
{
    ( void )file;
    ( void )line;
    vMBPAssert(  );
}

void
vMBPAssert( void )
{
    FATAL("MB", 0x01);
}

#if 1
void
vMBPEnterCritical( void )
{
    ENTRY_CRITICAL();
}

void
vMBPExitCritical( void )
{
    EXIT_CRITICAL();
}
#else
void
vMBPEnterCritical( void )
{
    __asm volatile  ( "mov r0, %0\n" "msr basepri, r0\n"::"i" ( PORT_INTERRUPT_PRIORITY_MAX ):"r0" );

    ubNesting++;
}

void
vMBPExitCritical( void )
{
    ubNesting--;
    if( 0 == ubNesting )
    {
        __asm volatile  ( "mov r0, #0\n" "msr basepri, r0\n":::"r0" );

        NVIC_RESETPRIMASK(  );
    }
}
#endif

void
vMBPSetDebugPin( eMBPDebugPin ePinName, BOOL bTurnOn )
{
    switch (ePinName)
    {
    case MBP_DEBUGPIN_0:        
        break;
    
    case MBP_DEBUGPIN_1: 
        break;
        
    default:
        break;
    }
    
}
