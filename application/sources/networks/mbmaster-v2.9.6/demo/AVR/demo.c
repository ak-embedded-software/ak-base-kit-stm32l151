/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo.c,v 1.13 2009-02-22 12:57:32 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define RS232_ENABLE_PORT		PORTC
#define RS232_ENABLE_DDR		DDRC
#define RS232_ENABLE_PIN		PC3

#define RS485_ENABLE_PORT		PORTC
#define RS485_ENABLE_DDR		DDRC
#define RS485_ENABLE_PIN		PC2

#define RS232_MUX_PORT			PORTC
#define RS232_MUX_DDR			DDRC
#define RS232_MUX_PIN			PC0

#define MBM_SERIAL_PORT			  ( 1 )
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
    USHORT          usNRegs[5];
    USHORT          usRegCnt = 0;
    UBYTE           ubIdx;
    UBYTE           ubCnt;

	/* Safe initialization settings for hardware. */
	RS232_ENABLE_PORT |= _BV( RS232_ENABLE_PIN );
	RS232_ENABLE_DDR |= _BV( RS232_ENABLE_PIN );
	RS232_MUX_PORT |= _BV( RS232_MUX_PIN );
	RS232_MUX_DDR |= _BV( RS232_MUX_PIN );
	RS485_ENABLE_PORT |= _BV( RS485_ENABLE_PIN );
	RS485_ENABLE_DDR |= _BV( RS485_ENABLE_PIN );

	/* For RS232 mode comment out these lines. */
#if 0
	RS232_ENABLE_PORT &= ~( _BV( RS232_ENABLE_PIN ) );
#endif

	/* For RS485 mode enable the lines below. */

	RS485_ENABLE_PORT &= ~( _BV( RS485_ENABLE_PIN ) );


    sei(  );
    if( MB_ENOERR ==
        ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
    {
        do
        {
            for( ubCnt = 0; ubCnt < 100; ubCnt++ )
            {
                _delay_ms( 10 );
            }

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
#if defined(__AVR_ATmega328P__)
        DDRB |= _BV( PORTB0 ) | _BV( PORTB1 );
#else
        DDRB |= _BV( PB0 ) | _BV( PB1 );
#endif
        bIsInitalized = TRUE;
    }
    switch ( ubIdx )
    {
    case DEBUG_LED_ERROR:
        if( bTurnOn )
        {
#if defined(__AVR_ATmega328P__)
			PORTB |= _BV( PORTB0 );
#else
            PORTB |= _BV( PB0 );
#endif
        }
        else
        {
#if defined(__AVR_ATmega328P__)
			PORTB &= ~_BV( PORTB0 );
#else
            PORTB &= ~_BV( PB0 );
#endif
        }
        break;

    case DEBUG_LED_WORKING:
        if( bTurnOn )
        {
#if defined(__AVR_ATmega328P__)
            PORTB |= _BV( PORTB1 );
#else
            PORTB |= _BV( PB1 );
#endif
        }
        else
        {
#if defined(__AVR_ATmega328P__)
            PORTB &= ~_BV( PORTB1 );
#else
            PORTB &= ~_BV( PB1 );
#endif
        }
        break;

    default:
        break;
    }
}
