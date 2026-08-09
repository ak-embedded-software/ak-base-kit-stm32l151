/*
 * MODBUS Library: Cortex M3 port
 * Copyright (c) Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.3 2009-01-02 10:44:40 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "LM3Sxxxx.H"
#include "rom.h"
#include "mbport.h"
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Modbus includes ----------------------------------*/

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

static UBYTE    ubNesting = 0;

/* ----------------------- Static functions ---------------------------------*/
__asm void      vMBPSetInterruptMask( void );
__asm void      vMBPClearInterruptMask( void );

/* ----------------------- Start implementation -----------------------------*/

void
__error__( char *pcFilename, unsigned long ulLine )
{
    ( void )pcFilename;
    ( void )ulLine;
    vMBPAssert(  );
}

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
    vMBPSetInterruptMask(  );
    ubNesting++;
}

void
vMBPExitCritical( void )
{
    ubNesting--;
    if( 0 == ubNesting )
    {
        vMBPClearInterruptMask(  );
    }
}

void
vMBPSetDebugPin( eMBPDebugPin ePinName, BOOL bTurnOn )
{
    STATIC BOOL     bIsInitalized = FALSE;

    if( !bIsInitalized )
    {
        vMBPEnterCritical(  );
        ROM_SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOA );
        ROM_GPIOPinWrite( GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7, 0x00 );
        ROM_GPIOPinTypeGPIOOutput( GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7 );
        bIsInitalized = TRUE;
        vMBPExitCritical(  );
    }

    switch ( ePinName )
    {
    case MBP_DEBUGPIN_0:
        if( bTurnOn )
        {
            ROM_GPIOPinWrite( GPIO_PORTA_BASE, GPIO_PIN_6, 0xFF );
        }
        else
        {
            ROM_GPIOPinWrite( GPIO_PORTA_BASE, GPIO_PIN_6, 0x00 );
        }
        break;
    case MBP_DEBUGPIN_1:
        if( bTurnOn )
        {
            ROM_GPIOPinWrite( GPIO_PORTA_BASE, GPIO_PIN_7, 0xFF );
        }
        else
        {
            ROM_GPIOPinWrite( GPIO_PORTA_BASE, GPIO_PIN_7, 0x00 );
        }
        break;
    default:
        break;
    }
}


/**INDENT-OFF* */
__asm void
vMBPSetInterruptMask( void )
{
    PRESERVE8 
	push	{ r0 }
    mov 	r0,	#MB_INTERRUPT_PRIORITY_MAX
    msr 	basepri, r0
	pop		{ r0 }
	bx		r14
}

__asm void
vMBPClearInterruptMask( void )
{
    PRESERVE8 
	push	{r0}
    mov     r0, #0
    msr 	basepri, r0
	pop    	{r0}
	bx		r14
}
/**INDENT-ON* */
