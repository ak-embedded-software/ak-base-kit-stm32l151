/* 
 * MODBUS Library: AVR port for ATMega2561
 * Copyright (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial_m2561.c,v 1.4 2010-06-22 18:53:33 embedded-so.embedded-solutions.1 Exp $
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

/* ----------------------- Defines ( User modifiable )-----------------------*/
#define USART0_ENABLED          ( 1 )
#define USART1_ENABLED          ( 1 )

#define USART0_RS485_ENABLED    ( 1 )
#define USART0_RS485_INIT_TX    do { /* customize */ } while( 0 )
#define USART0_RS485_ENABLE_TX  do { /* customize */ } while( 0 )
#define USART0_RS485_DISABLE_TX do { /* customize */ } while( 0 )

#define USART1_RS485_ENABLED    ( 1 )
#define USART1_RS485_INIT_TX    do { DDRE |= _BV( PORTE2 ); } while( 0 )
#define USART1_RS485_ENABLE_TX  do { PORTE |= _BV( PORTE2 ); } while( 0 )
#define USART1_RS485_DISABLE_TX do { PORTE &= ~_BV( PORTE2 ); } while( 0 )

/* ----------------------- Defines ------------------------------------------*/
#define SERIAL_DEBUG            ( 0 )
#if ( SERIAL_DEBUG == 1 )
#define SERIAL_DEBUG_INIT       do { DDRE |= _BV( PORTE4 ); } while( 0 )
#define SERIAL_DEBUG_SET        do { PORTE |= _BV( PORTE4 ); } while( 0 )
#define SERIAL_DEBUG_CLEAR      do { PORTE &= ~_BV( PORTE4 ); } while( 0 )
#else
#define SERIAL_DEBUG_INIT        
#define SERIAL_DEBUG_SET         
#define SERIAL_DEBUG_CLEAR       
#endif

#define IDX_INVALID				( 255 )
#define UART_BAUDRATE_MIN		( 300 )
#define UART_BAUDRATE_MAX		( 115200 )

#define USART0_IDX              ( 0 )
#define USART1_IDX              ( USART0_ENABLED * 1 + USART0_IDX )

#define USART_NUARTS            ( USART0_ENABLED + USART1_ENABLED )

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
    volatile uint8_t *pubUCSRXA;
    volatile uint8_t *pubUCSRXB;
    volatile uint8_t *pubUCSRXC;
    volatile uint8_t *pubUBRRXL;
    volatile uint8_t *pubUBRRXH;
} xSerialHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialHandle xSerialHdls[USART_NUARTS];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void
vMBPSerialTransmitCtl( UBYTE ubPort, BOOL bDriverEnable )
{
    switch ( ubPort )
    {
#if ( USART0_ENABLED == 1 ) && ( USART0_RS485_ENABLED == 1 )
    case USART0_IDX:
        if( bDriverEnable )
        {
            USART0_RS485_ENABLE_TX;
        }
        else
        {
            USART0_RS485_DISABLE_TX;
        }
        break;
#endif
#if ( USART1_ENABLED == 1 ) && ( USART1_RS485_ENABLED == 1 )
    case USART1_IDX:
        if( bDriverEnable )
        {
            USART1_RS485_ENABLE_TX;
        }
        else
        {
            USART1_RS485_DISABLE_TX;
        }
        break;
#endif
    }
}

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    xSerialHandle  *pxSerialIntHdl = NULL;
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
        SERIAL_DEBUG_INIT;
        bIsInitalized = TRUE;
    }

    if( NULL == pxSerialHdl )
    {
        eStatus = MB_EINVAL;
    }
    else if( ( USART0_IDX == ucPort ) && ( IDX_INVALID == xSerialHdls[USART0_IDX].ubIdx ) )
    {
#if ( USART0_ENABLED == 1 ) && ( USART0_RS485_ENABLED == 1 )
        USART0_RS485_INIT_TX;
        USART0_RS485_DISABLE_TX;
#endif
        xSerialHdls[USART0_IDX].ubIdx = USART0_IDX;
        xSerialHdls[USART0_IDX].pubUCSRXA = &UCSR0A;
        xSerialHdls[USART0_IDX].pubUCSRXB = &UCSR0B;
        xSerialHdls[USART0_IDX].pubUCSRXC = &UCSR0C;
        xSerialHdls[USART0_IDX].pubUBRRXL = &UBRR0L;
        xSerialHdls[USART0_IDX].pubUBRRXH = &UBRR0H;
        pxSerialIntHdl = &xSerialHdls[USART0_IDX];
    }
    else if( ( USART1_IDX == ucPort ) && ( IDX_INVALID == xSerialHdls[USART1_IDX].ubIdx ) )
    {
#if ( USART1_ENABLED == 1 ) && ( USART1_RS485_ENABLED == 1 )
        USART1_RS485_INIT_TX;
        USART1_RS485_DISABLE_TX;
#endif
        xSerialHdls[USART1_IDX].ubIdx = USART1_IDX;
        xSerialHdls[USART1_IDX].pubUCSRXA = &UCSR1A;
        xSerialHdls[USART1_IDX].pubUCSRXB = &UCSR1B;
        xSerialHdls[USART1_IDX].pubUCSRXC = &UCSR1C;
        xSerialHdls[USART1_IDX].pubUBRRXL = &UBRR1L;
        xSerialHdls[USART1_IDX].pubUBRRXH = &UBRR1H;
        pxSerialIntHdl = &xSerialHdls[USART1_IDX];
    }
    if( NULL != pxSerialIntHdl )
    {
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
                *pxSerialHdl = pxSerialIntHdl;
                pxSerialIntHdl->xMBMHdl = xMBMHdl;
                *( pxSerialIntHdl->pubUCSRXA ) = 0;
                *( pxSerialIntHdl->pubUCSRXB ) = 0;
                *( pxSerialIntHdl->pubUBRRXL ) = ( UBYTE ) ( usUBRR & 0xFF );
                *( pxSerialIntHdl->pubUBRRXH ) = ( UBYTE ) ( usUBRR >> 8U );
                *( pxSerialIntHdl->pubUCSRXC ) = ucUCSRC;
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
        if( ( *( pxSerialIntHdl->pubUCSRXB ) & ( _BV( TXEN0 ) | _BV( RXEN0 ) ) ) != 0 )
        {
            eStatus = MB_EAGAIN;
        }
        else if( ( pxSerialIntHdl->pbMBPTransmitterEmptyFN == NULL ) && ( pxSerialIntHdl->pvMBPReceiveFN == NULL ) )
        {
            *( pxSerialIntHdl->pubUCSRXA ) = _BV( UDRE0 );
            *( pxSerialIntHdl->pubUCSRXB ) = 0;
            *( pxSerialIntHdl->pubUCSRXC ) = _BV( UCSZ01 ) | _BV( UCSZ00 );
            *( pxSerialIntHdl->pubUBRRXL ) = 0;
            *( pxSerialIntHdl->pubUBRRXH ) = 0;
            vMBPSerialTransmitCtl( pxSerialIntHdl->ubIdx, FALSE );
            HDL_RESET( pxSerialIntHdl );
            eStatus = MB_ENOERR;
        }
        else
        {
            eStatus = MB_EIO;
        }
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
            *( pxSerialIntHdl->pubUCSRXB ) |= _BV( TXEN0 ) | _BV( UDRIE0 );
            vMBPSerialTransmitCtl( pxSerialIntHdl->ubIdx, TRUE );
        }
        else
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
            vMBPSerialTransmitCtl( pxSerialIntHdl->ubIdx, FALSE );
            *( pxSerialIntHdl->pubUCSRXB ) &= ~( _BV( UDRIE0 ) );
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
            *( pxSerialIntHdl->pubUCSRXB ) |= _BV( RXEN0 ) | _BV( RXCIE0 );
        }
        else
        {
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
            *( pxSerialIntHdl->pubUCSRXB ) &= ~( _BV( RXEN0 ) | _BV( RXCIE0 ) );
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

#if ( USART0_ENABLED == 1 )
ISR( USART0_UDRE_vect, ISR_BLOCK )
{
    BOOL            bHasMoreData = TRUE;
    UBYTE           ubTxByte;
    
    SERIAL_DEBUG_SET;
    MBP_ASSERT( IDX_INVALID != xSerialHdls[USART0_IDX].ubIdx );
    if( NULL != xSerialHdls[USART0_IDX].pbMBPTransmitterEmptyFN )
    {
        bHasMoreData = xSerialHdls[USART0_IDX].pbMBPTransmitterEmptyFN( xSerialHdls[USART0_IDX].xMBMHdl, &ubTxByte );
    }
    if( !bHasMoreData )
    {
        xSerialHdls[USART0_IDX].pbMBPTransmitterEmptyFN = NULL;
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
    SERIAL_DEBUG_CLEAR;
}

ISR( USART0_RX_vect, ISR_BLOCK )
{
    UBYTE           ubUDR = UDR0;

    SERIAL_DEBUG_SET;
    MBP_ASSERT( IDX_INVALID != xSerialHdls[0].ubIdx );
    if( NULL != xSerialHdls[USART0_IDX].pvMBPReceiveFN )
    {
        xSerialHdls[USART0_IDX].pvMBPReceiveFN( xSerialHdls[USART0_IDX].xMBMHdl, ubUDR );
    }
    SERIAL_DEBUG_CLEAR;
}

ISR( USART0_TX_vect, ISR_BLOCK )
{
    SERIAL_DEBUG_SET;
    UCSR0B &= ~( _BV( TXCIE0 ) | _BV( TXEN0 ) );
    vMBPSerialTransmitCtl( USART0_IDX, FALSE );
    SERIAL_DEBUG_CLEAR;
}
#endif

#if ( USART1_ENABLED == 1 )
ISR( USART1_UDRE_vect, ISR_BLOCK )
{
    BOOL            bHasMoreData = TRUE;
    UBYTE           ubTxByte;

    SERIAL_DEBUG_SET;
    MBP_ASSERT( IDX_INVALID != xSerialHdls[USART1_IDX].ubIdx );
    if( NULL != xSerialHdls[USART1_IDX].pbMBPTransmitterEmptyFN )
    {
        bHasMoreData = xSerialHdls[USART1_IDX].pbMBPTransmitterEmptyFN( xSerialHdls[USART1_IDX].xMBMHdl, &ubTxByte );
    }
    if( !bHasMoreData )
    {
        xSerialHdls[USART1_IDX].pbMBPTransmitterEmptyFN = NULL;
        /* The transmitter is disabled when the last frame has been sent.
         * This is necessary for RS485 with a hald-duplex bus.
         */
        UCSR1B &= ~( _BV( UDRIE0 ) );
        UCSR1B |= _BV( TXCIE0 );
    }
    else
    {
        UCSR1A |= _BV( TXC0 );
        UDR1 = ubTxByte;
    }
    SERIAL_DEBUG_CLEAR;
}

ISR( USART1_RX_vect, ISR_BLOCK )
{
    UBYTE           ubUDR = UDR1;

    SERIAL_DEBUG_SET;
    MBP_ASSERT( IDX_INVALID != xSerialHdls[1].ubIdx );
    if( NULL != xSerialHdls[USART1_IDX].pvMBPReceiveFN )
    {
        xSerialHdls[USART1_IDX].pvMBPReceiveFN( xSerialHdls[USART1_IDX].xMBMHdl, ubUDR );
    }
    SERIAL_DEBUG_CLEAR;
}

ISR( USART1_TX_vect, ISR_BLOCK )
{
    SERIAL_DEBUG_SET;
    UCSR1B &= ~( _BV( TXCIE0 ) | _BV( TXEN0 ) );
    vMBPSerialTransmitCtl( USART1_IDX, FALSE );
    SERIAL_DEBUG_CLEAR;
}
#endif
