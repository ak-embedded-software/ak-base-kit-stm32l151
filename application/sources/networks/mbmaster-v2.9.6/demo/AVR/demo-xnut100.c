/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-xnut100.c,v 1.3 2010-02-28 09:10:30 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#if defined( __IMAGECRAFT__  )
#include <iccioavr.h>
#include <AVRdef.h>
#else
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#endif

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBM_SERIAL_PORT			  ( 0 )
#define MBM_SERIAL_BAUDRATE		  ( 19200 )
#define MBM_PARITY				  ( MB_PAR_NONE )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC void		vMBPSetLED( UBYTE ubLED, UBYTE bState );

/* ----------------------- Start implementation -----------------------------*/

int
main( int argc, char *argv[] )
{
    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[20];
    USHORT          usRegCnt = 0;
    UBYTE           ubIdx;
    UBYTE           ubCnt;


    sei(  );
    if( MB_ENOERR ==
        ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
    {
        if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, 0x33AA ) ) )
        {
            eStatus = eStatus2;
        }

        do
        {
#if defined( __IMAGECRAFT__  )        
            _StackCheck(  );
#endif            
            
            eStatus = MB_ENOERR;

            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
            {
                eStatus = eStatus2;
            }

            /* Read holding register from adress 5 - 10, increment them by one and store
             * them at address 10. 
             */
            if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 5, 5, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }
            for( ubIdx = 0; ubIdx < 5; ubIdx++ )
            {
                usNRegs[ubIdx]++;
            }
            if( MB_ENOERR != ( eStatus2 = eMBMWriteMultipleRegisters( xMBMMaster, 1, 10, 5, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }

            /* Read the input registers from address 2 - 5 and write them to the holding
             * registers at address 1 - 4.
             */
            if( MB_ENOERR != ( eStatus2 = eMBMReadInputRegisters( xMBMMaster, 1, 2, 4, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }
            if( MB_ENOERR != ( eStatus2 = eMBMWriteMultipleRegisters( xMBMMaster, 1, 1, 4, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }
            switch ( eStatus )
            {
            case MB_ENOERR:
				vMBPSetLED( 0, 1 );
                break;

            default:
				vMBPSetLED( 0, 2 );
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

STATIC void
vMBPSetLED( UBYTE ubLED, UBYTE bState )
{
	STATIC BOOL bIsInitialized = FALSE;
	if( !bIsInitialized )
	{
	    /* Initialize DEBUG LEDs */
	    DDRF |= _BV( PF2 ) | _BV( PF3 ) | _BV( PF1 ) | _BV( PF0 );
	    PORTF &= ~( _BV( PF2 ) | _BV( PF3 ) | _BV( PF1 ) | _BV( PF0 ) );
		bIsInitialized = TRUE;
	}

    switch ( ubLED )
    {
    case 0:
        PORTF &= ~( _BV( PF2 ) | _BV( PF3 ) );
        switch ( bState )
        {
        case 2:
            PORTF |= _BV( PF3 );
            break;
        case 1:
            PORTF |= _BV( PF2 );
            break;
        default:
        case 0:
            break;
        }
        break;
    case 1:
        PORTF &= ~( _BV( PF1 ) | _BV( PF0 ) );
        switch ( bState )
        {
        case 2:
            PORTF |= _BV( PF1 );
            break;
        case 1:
            PORTF |= _BV( PF0 );
            break;
        default:
        case 0:
            break;
        }
        break;
    default:
        break;
    }
}

void
_StackOverflowed( char cOverFlowed )
{    
    vMBPAssert(  );
} 

