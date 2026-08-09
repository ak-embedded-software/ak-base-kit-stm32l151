/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: porttest.c,v 1.4 2007-08-18 00:25:44 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbmporttest.h"

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

void
vMBPTestDebugLED( UBYTE ubIdx, BOOL bTurnOn )
{
    switch ( ubIdx )
    {
    case MBM_PORT_DEBUG_LED_ERROR:
        if( bTurnOn )
        {
            PORTB |= _BV( PB0 );
        }
        else
        {
            PORTB &= ~_BV( PB0 );
        }
        break;

    case MBM_PORT_DEBUG_LED_WORKING:
        if( bTurnOn )
        {
            PORTB |= _BV( PB1 );
        }
        else
        {
            PORTB &= ~_BV( PB1 );
        }
        break;

    default:
        break;
    }
}

int
main( int argc, char *argv[] )
{
    DDRB |= _BV( PB0 ) | _BV( PB1 );
    sei(  );
    do
    {
        vMBPTestRun(  );
    }
    while( TRUE );

    return 0;
}
