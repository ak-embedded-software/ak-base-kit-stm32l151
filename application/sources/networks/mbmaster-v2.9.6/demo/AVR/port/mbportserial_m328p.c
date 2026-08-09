/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial_m328p.c,v 1.5 2010-06-22 18:53:33 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define IDX_INVALID				( 255 )
#define UART_BAUDRATE_MIN		( 300 )
#define UART_BAUDRATE_MAX		( 115200 )

#define RS_485_ENABLED          ( 1 )

#if RS_485_ENABLED == 1
#define RS_485_DE_PIN			( PORTD3 )
#define RS_485_PORT             ( PORTD )
#define RS_485_DDR              ( DDRD )
#define RS_485_INIT(  )			do { RS_485_DDR |= _BV( RS_485_DE_PIN ); } while( 0 )
#define RS_485_ENABLE_TX(  )	do { RS_485_PORT |= _BV( RS_485_DE_PIN ); } while( 0 )
#define RS_485_DISABLE_TX(  )	do { RS_485_PORT &= ~_BV( RS_485_DE_PIN ); } while( 0 )
#else
#define RS_485_INIT(  )
#define RS_485_ENABLE_TX(  )
#define RS_485_DISABLE_TX(  )
#endif

#define UART_BAUD_CALC( UART_BAUD_RATE, F_OSC ) \
    ( ( F_OSC ) / ( ( UART_BAUD_RATE ) * 16UL ) - 1 )

#define HDL_RESET( x ) do { \
	( x )->ubIdx = IDX_INVALID; \
	( x )->pbMBPTransmitterEmptyFN = NULL; \
	( x )->pvMBPReceiveFN = NULL; \
	( x )->xMBMHdl = MB_HDL_INVALID; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    pbMBPSerialTransmitterEmptyAPIV1CB pbMBPTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV1CB pvMBPReceiveFN;
    xMBHandle       xMBMHdl;
} xSerialHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialHandle xSerialHdls[1];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UBYTE           ubIdx;
    UCHAR           ucUCSRC = 0;
    USHORT          usUBRR;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            HDL_RESET( &xSerialHdls[ubIdx] );
        }
        RS_485_INIT(  );
        RS_485_DISABLE_TX(  );
        bIsInitalized = TRUE;
    }

    if( NULL == pxSerialHdl )
    {
        eStatus = MB_EINVAL;
    }
    else if( ( 0 == ucPort ) && ( IDX_INVALID == xSerialHdls[0].ubIdx ) )
    {
        *pxSerialHdl = NULL;

        if( ( ulBaudRate >= UART_BAUDRATE_MIN ) && ( ulBaudRate <= UART_BAUDRATE_MAX ) && ( MB_HDL_INVALID != xMBMHdl ) )
        {

            usUBRR = UART_BAUD_CALC( ulBaudRate, F_CPU ) & 0x0FFFU;
            switch ( eParity )
            {
            case MB_PAR_EVEN:
                ucUCSRC |= _BV( UPM01 );
                break;
            case MB_PAR_ODD:
                ucUCSRC |= _BV( UPM01 ) | _BV( UPM00 );
                break;
            case MB_PAR_NONE:
                break;
            default:
                eStatus = MB_EINVAL;
            }

            switch ( ucDataBits )
            {
            case 8:
                ucUCSRC |= _BV( UCSZ00 ) | _BV( UCSZ01 );
                break;
            case 7:
                ucUCSRC |= _BV( UCSZ01 );
                break;
            default:
                eStatus = MB_EINVAL;
            }

            switch ( ucStopBits )
            {
            case 1:
                break;
            case 2:
                ucUCSRC |= _BV( USBS0 );
                break;
            default:
                eStatus = MB_EINVAL;
            }

            if( MB_ENOERR == eStatus )
            {
                *pxSerialHdl = &xSerialHdls[0];
                xSerialHdls[0].ubIdx = 0;
                xSerialHdls[0].xMBMHdl = xMBMHdl;
                UCSR0A = 0;
                UCSR0B = 0;
                UBRR0L = ( UBYTE ) ( usUBRR & 0xFF );
                UBRR0H = ( UBYTE ) ( usUBRR >> 8U );

                UCSR0C = ucUCSRC;
            }
        }
        else
        {
            eStatus = MB_EINVAL;
        }
    }
    else
    {
        eStatus = MB_ENORES;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialClose( xMBPSerialHandle xSerialHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        /* Reset to default values (See ATMega168 datasheet ) */
        UCSR0A = _BV( UDRE0 );
        UCSR0B = 0;
        UCSR0C = _BV( UCSZ01 ) | _BV( UCSZ00 );
        UBRR0H = 0;
        UBRR0L = 0;
        RS_485_DISABLE_TX(  );
        HDL_RESET( pxSerialIntHdl );
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialTxEnable( xMBPSerialHandle xSerialHdl, pbMBPSerialTransmitterEmptyCB pbMBPTransmitterEmptyFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pbMBPTransmitterEmptyFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pbMBPTransmitterEmptyFN );
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = pbMBPTransmitterEmptyFN;
            UCSR0B |= _BV( TXEN0 ) | _BV( UDRIE0 );
            RS_485_ENABLE_TX(  );
        }
        else
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
            RS_485_DISABLE_TX(  );
            UCSR0B &= ~( _BV( UDRIE0 ) );
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBPReceiveFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pvMBPReceiveFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pvMBPReceiveFN );
            pxSerialIntHdl->pvMBPReceiveFN = pvMBPReceiveFN;
            UCSR0B |= _BV( RXEN0 ) | _BV( RXCIE0 );
        }
        else
        {
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
            UCSR0B &= ~( _BV( RXEN0 ) | _BV( RXCIE0 ) );
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

ISR( USART_UDRE_vect, ISR_BLOCK )
{
    MBP_ASSERT( IDX_INVALID != xSerialHdls[0].ubIdx );
    BOOL            bHasMoreData = TRUE;
    UBYTE           ubTxByte;

    if( NULL != xSerialHdls[0].pbMBPTransmitterEmptyFN )
    {
        bHasMoreData = xSerialHdls[0].pbMBPTransmitterEmptyFN( xSerialHdls[0].xMBMHdl, &ubTxByte );
    }
    if( !bHasMoreData )
    {
        xSerialHdls[0].pbMBPTransmitterEmptyFN = NULL;
        /* The transmitter is disabled when the last frame has been sent.
         * This is necessary for RS485 with a hald-duplex bus.
         */
        UCSR0B &= ~( _BV( UDRIE0 ) );
        UCSR0B |= _BV( TXCIE0 );
    }
    else
    {
        UCSR0A |= _BV( TXC0 );
        UDR0 = ubTxByte;
    }
}

ISR( USART_RX_vect, ISR_BLOCK )
{
    UBYTE           ubUDR = UDR0;

    MBP_ASSERT( IDX_INVALID != xSerialHdls[0].ubIdx );
    if( NULL != xSerialHdls[0].pvMBPReceiveFN )
    {
        xSerialHdls[0].pvMBPReceiveFN( xSerialHdls[0].xMBMHdl, ubUDR );
    }
}

ISR( USART_TX_vect, ISR_BLOCK )
{
    RS_485_DISABLE_TX(  );
    UCSR0B &= ~( _BV( TXCIE0 ) | _BV( TXEN0 ) );
}
