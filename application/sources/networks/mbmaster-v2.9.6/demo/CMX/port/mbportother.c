/* 
 * MODBUS Library: CMX port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.4 2008-09-01 18:44:19 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include "cxfuncs.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"
#include "m523xevb.h"

/* ----------------------- Modbus includes ----------------------------------*/

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

STATIC BYTE     ubNestingISR = 0;
STATIC BYTE     ubNesting = 0;
STATIC unsigned char ucRecResOwner = 255;
STATIC int      usOldIPL;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Functions ----------------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
void
vMBPDebugLED( UBYTE ubLED, BOOL bStatus )
{
    STATIC BOOL     bIsInitialized = FALSE;

    if( !bIsInitialized )
    {
        MCF_GPIO_PAR_FECI2C = MCF_GPIO_PAR_FECI2C_PAR_SDA( 0 ) | MCF_GPIO_PAR_FECI2C_PAR_SCL( 0 );
        MCF_GPIO_PDDR_FECI2C = MCF_GPIO_PDDR_FECI2C_PDDR_FECI2C0 | MCF_GPIO_PDDR_FECI2C_PDDR_FECI2C1;
    }
    switch ( ubLED )
    {
    case 0:
        if( bStatus )
        {
            MCF_GPIO_PCLRR_FECI2C = ( UBYTE ) ~ MCF_GPIO_PDDR_FECI2C_PDDR_FECI2C0;
        }
        else
        {
            MCF_GPIO_PPDSDR_FECI2C = ( UBYTE ) MCF_GPIO_PDDR_FECI2C_PDDR_FECI2C0;
        }
    case 1:
        if( bStatus )
        {
            MCF_GPIO_PCLRR_FECI2C = ( UBYTE ) ~ MCF_GPIO_PDDR_FECI2C_PDDR_FECI2C1;
        }
        else
        {
            MCF_GPIO_PPDSDR_FECI2C = ( UBYTE ) MCF_GPIO_PDDR_FECI2C_PDDR_FECI2C1;
        }
    default:
        break;
    }

}

void
vMBPAssert( void )
{
    volatile BOOL   bBreakOut = FALSE;

    K_OS_Disable_Interrupts(  );
    while( !bBreakOut )
    {

    }
}

void
vMBPEnterCritical( void )
{
    unsigned char   ucStatus;
    unsigned char   ucMyTaskSlot = K_OS_Task_Slot_Get(  );

    if( ucMyTaskSlot == ucRecResOwner )
    {
        ubNesting++;
    }
    else
    {
        ucStatus = K_Resource_Wait( MB_RESOURCE, 0 );
        MBP_ASSERT( K_OK == ucStatus );
        ubNesting = 1;
        ucRecResOwner = ucMyTaskSlot;
    }
}

void
vMBPExitCritical( void )
{
    unsigned char   ucStatus;

    if( --ubNesting == 0 )
    {
        ucRecResOwner = 255;
        ucStatus = K_Resource_Release( MB_RESOURCE );
        MBP_ASSERT( K_OK == ucStatus );
    }
}

void
vMBPEnterCriticalISR( void )
{
    int             usCurrentIPL = asm_set_ipl( 7 );

    if( ubNestingISR == 0 )
    {
        /* First caller preservs IPL. */
        usOldIPL = usCurrentIPL;
    }
    ubNestingISR++;
}

void
vMBPExitCriticalISR( void )
{
    ubNestingISR--;
    if( 0 == ubNestingISR )
    {
        asm_set_ipl( ( unsigned long )usOldIPL );
    }
}
