/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial_m644p.c,v 1.2 2010-06-22 18:53:33 embedded-so.embedded-solutions.1 Exp $
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


#define UART_0_ENABLED          ( 1 )   /*!< Set this to 1 to enable USART0 */
#define UART_1_ENABLED          ( 1 )   /*!< Set this to 1 to enable USART1 */

#if ( UART_0_ENABLED == 1 ) && ( UART_1_ENABLED == 1 )
#define UART_0_IDX              ( 0 )
#define UART_1_IDX              ( 1 )
#define NUARTS                  ( 2 )
#elif ( UART_0_ENABLED == 1 )
#define UART_0_IDX              ( 0 )
#define NUARTS                  ( 1 )
#elif ( UART_1_ENABLED == 1 )
#define UART_1_IDX              ( 0 )
#define NUARTS                  ( 1 )
#else
#define NUARTS                  ( 0 )
#endif


#define RS_485_UART_0_INIT(  )	\
    do {  } while( 0 )
#define RS_485_UART_0_ENABLE_TX(  )	\
    do {  } while( 0 )
#define RS_485_UART_0_DISABLE_TX(  ) \
	do {  } while( 0 )

#define RS_485_UART_1_INIT(  )	\
    do { DDRD |= _BV( PIND4 ); } while( 0 )
#define RS_485_UART_1_ENABLE_TX(  )	\
    do { PORTD |= _BV( PIND4 ); } while( 0 )
#define RS_485_UART_1_DISABLE_TX(  ) \
	do { PORTD &= ~_BV( PIND4 ); } while( 0 )

#define UART_BAUD_CALC( UART_BAUD_RATE, F_OSC ) \
    ( ( F_OSC + ( ( UART_BAUD_RATE ) * 8UL ) ) / ( ( UART_BAUD_RATE ) * 16UL ) - 1 )

/* ----------------------- Defines ------------------------------------------*/
/* ----------------------- Defines (Internal - Don't change) ----------------*/
#define HDL_RESET( x ) do { \
	( x )->ubIdx = IDX_INVALID; \
	( x )->pbMBMTransmitterEmptyFN = NULL; \
	( x )->pvMBMReceiveFN = NULL; \
	( x )->xMBMHdl = MB_HDL_INVALID; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    pbMBPSerialTransmitterEmptyAPIV1CB pbMBMTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV1CB pvMBMReceiveFN;
    xMBHandle       xMBMHdl;
} xSerialHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialHandle xSerialHdls[NUARTS];
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
#if UART_0_ENABLED == 1
        RS_485_UART_0_INIT(  );
        RS_485_UART_0_DISABLE_TX(  );
#endif
#if UART_1_ENABLED == 1
        RS_485_UART_1_INIT(  );
        RS_485_UART_1_DISABLE_TX(  );
#endif
        bIsInitalized = TRUE;
    }

    if( NULL == pxSerialHdl )
    {
        eStatus = MB_EINVAL;
    }
    else
    {
        eStatus = MB_ENORES;
        switch ( ucPort )
        {
#if UART_0_ENABLED == 1
        case UART_0_IDX:
            if( IDX_INVALID == xSerialHdls[UART_0_IDX].ubIdx )
            {
                *pxSerialHdl = NULL;

                if( ( ulBaudRate >= UART_BAUDRATE_MIN ) && ( ulBaudRate <= UART_BAUDRATE_MAX )
                    && ( MB_HDL_INVALID != xMBMHdl ) )
                {
                    eStatus = MB_ENOERR;
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
                        *pxSerialHdl = &xSerialHdls[UART_0_IDX];
                        xSerialHdls[UART_0_IDX].ubIdx = UART_0_IDX;
                        xSerialHdls[UART_0_IDX].xMBMHdl = xMBMHdl;
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
                eStatus = MB_EINVAL;
            }
            break;
#endif
#if UART_1_ENABLED == 1
        case UART_1_IDX:
            if( IDX_INVALID == xSerialHdls[UART_1_IDX].ubIdx )
            {
                *pxSerialHdl = NULL;

                if( ( ulBaudRate >= UART_BAUDRATE_MIN ) && ( ulBaudRate <= UART_BAUDRATE_MAX )
                    && ( MB_HDL_INVALID != xMBMHdl ) )
                {
                    eStatus = MB_ENOERR;
                    usUBRR = UART_BAUD_CALC( ulBaudRate, F_CPU ) & 0x0FFFU;
                    switch ( eParity )
                    {
                    case MB_PAR_EVEN:
                        ucUCSRC |= _BV( UPM11 );
                        break;
                    case MB_PAR_ODD:
                        ucUCSRC |= _BV( UPM11 ) | _BV( UPM10 );
                        break;
                    case MB_PAR_NONE:
                        break;
                    default:
                        eStatus = MB_EINVAL;
                    }

                    switch ( ucDataBits )
                    {
                    case 8:
                        ucUCSRC |= _BV( UCSZ10 ) | _BV( UCSZ11 );
                        break;
                    case 7:
                        ucUCSRC |= _BV( UCSZ11 );
                        break;
                    default:
                        eStatus = MB_EINVAL;
                    }

                    switch ( ucStopBits )
                    {
                    case 1:
                        break;
                    case 2:
                        ucUCSRC |= _BV( USBS1 );
                        break;
                    default:
                        eStatus = MB_EINVAL;
                    }

                    if( MB_ENOERR == eStatus )
                    {
                        *pxSerialHdl = &xSerialHdls[UART_1_IDX];
                        xSerialHdls[UART_1_IDX].ubIdx = UART_1_IDX;
                        xSerialHdls[UART_1_IDX].xMBMHdl = xMBMHdl;
                        UCSR1A = 0;
                        UCSR1B = 0;
                        UBRR1L = ( UBYTE ) ( usUBRR & 0xFF );
                        UBRR1H = ( UBYTE ) ( usUBRR >> 8U );

                        UCSR1C = ucUCSRC;
                    }
                }
                else
                {
                    eStatus = MB_EINVAL;
                }
            }

#endif
        default:
            break;
        }
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
        switch ( pxSerialIntHdl->ubIdx )
        {
#if UART_0_ENABLED == 1
        case UART_0_IDX:
            if( ( UCSR0B & ( _BV( TXEN0 ) | _BV( RXEN0 ) ) ) != 0 )
            {
                eStatus = MB_EAGAIN;
            }
            else if( ( NULL == pxSerialIntHdl->pbMBMTransmitterEmptyFN ) && ( NULL == pxSerialIntHdl->pvMBMReceiveFN ) )
            {
                UCSR0A = 0;
                UCSR0B = 0;
                UCSR0C = 0;
                UBRR0H = 0;
                UBRR0L = 0;
                RS_485_UART_0_DISABLE_TX(  );
                HDL_RESET( pxSerialIntHdl );
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_EIO;
            }
            break;
#endif
#if UART_1_ENABLED == 1
        case UART_1_IDX:
            if( ( UCSR1B & ( _BV( TXEN1 ) | _BV( RXEN1 ) ) ) != 0 )
            {
                eStatus = MB_EAGAIN;
            }
            else if( ( NULL == pxSerialIntHdl->pbMBMTransmitterEmptyFN ) && ( NULL == pxSerialIntHdl->pvMBMReceiveFN ) )
            {
                UCSR1A = 0;
                UCSR1B = 0;
                UCSR1C = 0;
                UBRR1H = 0;
                UBRR1L = 0;
                RS_485_UART_0_DISABLE_TX(  );
                HDL_RESET( pxSerialIntHdl );
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_EIO;
            }
            break;
#endif
        default:
            MBP_ASSERT( 0 );
            break;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialTxEnable( xMBPSerialHandle xSerialHdl, pbMBPSerialTransmitterEmptyCB pbMBMTransmitterEmptyFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pbMBMTransmitterEmptyFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pbMBMTransmitterEmptyFN );
            pxSerialIntHdl->pbMBMTransmitterEmptyFN = pbMBMTransmitterEmptyFN;
            switch ( pxSerialIntHdl->ubIdx )
            {
#if UART_0_ENABLED == 1
            case UART_0_IDX:				
                UCSR0B |= _BV( TXEN0 ) | _BV( UDRE0 );
                RS_485_UART_0_ENABLE_TX(  );
                break;
#endif
#if UART_1_ENABLED == 1
            case UART_1_IDX:
                UCSR1B |= _BV( TXEN1 ) | _BV( UDRE1 );
                RS_485_UART_1_ENABLE_TX(  );
                break;
#endif
            default:
                MBP_ASSERT( 0 );
            }

        }
        else
        {
            pxSerialIntHdl->pbMBMTransmitterEmptyFN = NULL;
            /* The transmitter is disable when the last frame has been sent.
             * This is necessary for RS485 with a half-duplex bus.
             */
            switch ( pxSerialIntHdl->ubIdx )
            {
#if UART_0_ENABLED == 1
            case UART_0_IDX:
                UCSR0B &= ~( _BV( UDRE0 ) );
				RS_485_UART_0_DISABLE_TX(  );
                break;
#endif
#if UART_1_ENABLED == 1
            case UART_1_IDX:
                UCSR1B &= ~( _BV( UDRE1 ) );
				RS_485_UART_1_DISABLE_TX(  );
                break;
#endif
            default:
                MBP_ASSERT( 0 );
            }
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBMReceiveFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pvMBMReceiveFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pvMBMReceiveFN );
            pxSerialIntHdl->pvMBMReceiveFN = pvMBMReceiveFN;
            switch ( pxSerialIntHdl->ubIdx )
            {
#if UART_0_ENABLED == 1
            case UART_0_IDX:
                UCSR0B |= _BV( RXEN0 ) | _BV( RXCIE0 );
                break;
#endif
#if UART_1_ENABLED == 1
            case UART_1_IDX:
                UCSR1B |= _BV( RXEN1 ) | _BV( RXCIE1 );
                break;
#endif
            default:
                MBP_ASSERT( 0 );
            }
        }
        else
        {
            pxSerialIntHdl->pvMBMReceiveFN = NULL;
            switch ( pxSerialIntHdl->ubIdx )
            {
#if UART_0_ENABLED == 1
            case UART_0_IDX:
                UCSR0B &= ~( _BV( RXEN0 ) | _BV( RXCIE0 ) );
                break;
#endif
#if UART_1_ENABLED == 1
            case UART_1_IDX:
                UCSR1B &= ~( _BV( RXEN1 ) | _BV( RXCIE1 ) );
                break;
#endif
            default:
                MBP_ASSERT( 0 );
            }
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

#if UART_0_ENABLED == 1
ISR( USART0_UDRE_vect, ISR_BLOCK )
{
    MBP_ASSERT( IDX_INVALID != xSerialHdls[UART_0_IDX].ubIdx );
    BOOL            bHasMoreData = TRUE;
    UBYTE           ubTxByte;

    if( NULL != xSerialHdls[UART_0_IDX].pbMBMTransmitterEmptyFN )
    {
        bHasMoreData = xSerialHdls[UART_0_IDX].pbMBMTransmitterEmptyFN( xSerialHdls[UART_0_IDX].xMBMHdl, &ubTxByte );
    }
    if( !bHasMoreData )
    {
        xSerialHdls[UART_0_IDX].pbMBMTransmitterEmptyFN = NULL;
        /* The transmitter is disabled when the last frame has been sent.
         * This is necessary for RS485 with a hald-duplex bus.
         */
        UCSR0B &= ~( _BV( UDRE0 ) );
        UCSR0B |= _BV( TXCIE0 );
    }
    else
    {
		UCSR0A |= _BV( TXC0 );
        UDR0 = ubTxByte;
    }
}
#endif

#if UART_0_ENABLED == 1
ISR( USART0_RX_vect, ISR_BLOCK )
{
    UBYTE           ubUDR = UDR0;

    MBP_ASSERT( IDX_INVALID != xSerialHdls[UART_0_IDX].ubIdx );
    if( NULL != xSerialHdls[UART_0_IDX].pvMBMReceiveFN )
    {
        xSerialHdls[UART_0_IDX].pvMBMReceiveFN( xSerialHdls[UART_0_IDX].xMBMHdl, ubUDR );
    }
}
#endif

#if UART_0_ENABLED == 1
ISR( USART0_TX_vect, ISR_BLOCK )
{
    RS_485_UART_0_DISABLE_TX(  );
    UCSR0B &= ~( _BV( TXCIE0 ) | _BV( TXEN0 ) );
}
#endif

#if UART_1_ENABLED == 1
ISR( USART1_UDRE_vect, ISR_BLOCK )
{
    MBP_ASSERT( IDX_INVALID != xSerialHdls[UART_1_IDX].ubIdx );
    BOOL            bHasMoreData = TRUE;
    UBYTE           ubTxByte;

    if( NULL != xSerialHdls[UART_1_IDX].pbMBMTransmitterEmptyFN )
    {
        bHasMoreData = xSerialHdls[UART_1_IDX].pbMBMTransmitterEmptyFN( xSerialHdls[UART_1_IDX].xMBMHdl, &ubTxByte );
    }
    if( !bHasMoreData )
    {
        xSerialHdls[UART_1_IDX].pbMBMTransmitterEmptyFN = NULL;
        /* The transmitter is disabled when the last frame has been sent.
         * This is necessary for RS485 with a hald-duplex bus.
         */
        UCSR1B &= ~( _BV( UDRE1 ) );
        UCSR1B |= _BV( TXCIE1 );
    }
    else
    {
		UCSR1A |= _BV( TXC0 );
        UDR1 = ubTxByte;
    }
}
#endif

#if UART_1_ENABLED == 1
ISR( USART1_RX_vect, ISR_BLOCK )
{
    UBYTE           ubUDR = UDR1;

    MBP_ASSERT( IDX_INVALID != xSerialHdls[UART_1_IDX].ubIdx );
    if( NULL != xSerialHdls[UART_1_IDX].pvMBMReceiveFN )
    {
        xSerialHdls[UART_1_IDX].pvMBMReceiveFN( xSerialHdls[UART_1_IDX].xMBMHdl, ubUDR );
    }
}
#endif

#if UART_1_ENABLED == 1
ISR( USART1_TX_vect, ISR_BLOCK )
{
    UCSR1B &= ~( _BV( TXCIE1 ) | _BV( TXEN1 ) );
	RS_485_UART_1_DISABLE_TX(  );
}
#endif
