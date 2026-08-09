/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * Description: This demo performs the following
 *
 *  - Read input register 0x000F from slave 4
 *  - Read input register 0x0200 from slave 4
 *  - Read input registers 0x0004 - 0x0007 from slave 3
 *  - Read input registers 0x0004 - 0x0007 from slave 4
 * 
 * $Id: demo-multi.c,v 1.5 2007-08-18 00:25:44 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/



#define MBM_SERIAL_PORT			  ( 0 )
#define MBM_SERIAL_BAUDRATE		  ( 19200 )
#define MBM_PARITY				  ( MB_PAR_NONE )

#define DEBUG_LED_ERROR           ( 0 )
#define DEBUG_LED_WORKING         ( 1 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vIOSetLED( UBYTE ubIdx, BOOL bTurnOn );

/* ----------------------- Start implementation -----------------------------*/

int
main( int argc, char *argv[] )
{

    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegsSlave3[4];
    USHORT          usNRegsSlave4[6];
    UBYTE           ubCnt;

    sei(  );
    if( MB_ENOERR ==
        ( eStatus = eMBMSerialInit( &xMBMMaster, MB_ASCII, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
    {
        do
        {
            for( ubCnt = 0; ubCnt < 100; ubCnt++ )
            {
                _delay_ms( 10 );
            }

            eStatus = MB_ENOERR;

            /* Read input register 0x000F from slave 4 */
            if( MB_ENOERR != ( eStatus2 = eMBMReadInputRegisters( xMBMMaster, 4, 0x000F, 1, &usNRegsSlave4[0] ) ) )
            {
                eStatus = eStatus2;
            }
            /* Read input register 0x0200 from slave 4 */
            if( MB_ENOERR != ( eStatus2 = eMBMReadInputRegisters( xMBMMaster, 4, 0x0200, 1, &usNRegsSlave4[1] ) ) )
            {
                eStatus = eStatus2;
            }
            /* Read input registers 0x0004 - 0x0007 from slave 3 */
            if( MB_ENOERR != ( eStatus2 = eMBMReadInputRegisters( xMBMMaster, 3, 0x0004, 4, &usNRegsSlave3[0] ) ) )
            {
                eStatus = eStatus2;
            }
            /* Read input registers 0x0004 - 0x0007 from slave 4 */
            if( MB_ENOERR != ( eStatus2 = eMBMReadInputRegisters( xMBMMaster, 4, 0x0004, 4, &usNRegsSlave4[0] ) ) )
            {
                eStatus = eStatus2;
            }

            switch ( eStatus )
            {
            case MB_ENOERR:
                vIOSetLED( DEBUG_LED_WORKING, TRUE );
                vIOSetLED( DEBUG_LED_ERROR, FALSE );
                break;

            default:
                vIOSetLED( DEBUG_LED_ERROR, TRUE );
                vIOSetLED( DEBUG_LED_WORKING, FALSE );
                break;
            }
        }
        while( TRUE );
    }
    else
    {
        MBP_ASSERT( 0 );
    }


    if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
    {
        MBP_ASSERT( 0 );
    }

    return 0;
}

void
vIOSetLED( UBYTE ubIdx, BOOL bTurnOn )
{
    STATIC BOOL     bIsInitalized = FALSE;

    if( !bIsInitalized )
    {
        DDRB |= _BV( PB0 ) | _BV( PB1 );
        bIsInitalized = TRUE;
    }
    switch ( ubIdx )
    {
    case DEBUG_LED_ERROR:
        if( bTurnOn )
        {
            PORTB |= _BV( PB0 );
        }
        else
        {
            PORTB &= ~_BV( PB0 );
        }
        break;

    case DEBUG_LED_WORKING:
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
