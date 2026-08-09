/* 
 * MODBUS Library: Nut/OS port
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial.c,v 1.1 2010-02-21 19:23:07 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <sys/thread.h>
#include <sys/event.h>
#include <io.h>
#include <fcntl.h>
#include <dev/board.h>
/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define IDX_INVALID					( 255 )

#define SER_RXTX_BUFFER_SIZE		( 16 )
#define SER_RX_READ_TIME_MIN_MS		( 5 )
#define SER_RX_GUARD_TIME_MS		( 10 )

#define HDL_RESET( x ) do { \
	( x )->ubIdx = IDX_INVALID; \
	( x )->bIsRunning = FALSE; \
	( x )->iUartFD = -1; \
	( x )->pbMBPTransmitterEmptyFN = NULL; \
	( x )->pvMBPReceiveFN = NULL; \
	( x )->xMBHdl = MB_HDL_INVALID; \
	( x )->xWaitHdl = ( HANDLE )0; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    BOOL            bIsRunning;
    int             iUartFD;
    pbMBPSerialTransmitterEmptyAPIV2CB pbMBPTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV2CB pvMBPReceiveFN;
    xMBHandle       xMBHdl;
    HANDLE          xWaitHdl;
    UBYTE           arubRxTxBuffer[SER_RXTX_BUFFER_SIZE];
} xSerialIntHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialIntHandle xSerialHdls[1];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vMBPSerialHandlerThread( void *pvArg );
STATIC ULONG    ulSerialRxTimeMaxMS( ULONG ulBaudRate );
STATIC void     vMBPSerialHardwareInitUART0( void );
STATIC void     vMBPSerialHardwareInitUART1( void );
/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBHdl )
{
    eMBErrorCode    eStatus = MB_ENORES;
    xSerialIntHandle *pxIntHdl = NULL;
    UBYTE           ubIdx;
    int             iFd = -1;
    u_long          ulIOCTLBaudRate;
    u_long          ulIOCTLDataBits;
    u_long          ulIOCTLParity;
    u_long          ulIOCTLStopBits;
    u_long          ulIOCTLTimeout;
    u_long          ulIOCTLFlow = USART_MF_HALFDUPLEX;
    const struct
    {
        const char     *szDevName;
        void            ( *pvInitFN ) ( void );
    } arszUARTDevs[] =
    {
        {
        DEV_UART_NAME, vMBPSerialHardwareInitUART0},
        {
        DEV_UART1_NAME, vMBPSerialHardwareInitUART1}
    };

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            HDL_RESET( &xSerialHdls[ubIdx] );
        }
        bIsInitalized = TRUE;
    }

    if( ( NULL == pxSerialHdl ) || ( MB_HDL_INVALID == xMBHdl ) || ( ucPort >= MB_UTILS_NARRSIZE( arszUARTDevs ) ) )
    {
        eStatus = MB_EINVAL;
    }
    else
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            if( IDX_INVALID == xSerialHdls[ubIdx].ubIdx )
            {
                pxIntHdl = &xSerialHdls[ubIdx];
                pxIntHdl->ubIdx = ubIdx;
                break;
            }
        }
    }

    if( NULL != pxIntHdl )
    {
        ulIOCTLBaudRate = ulBaudRate;
        ulIOCTLDataBits = ucDataBits;
        ulIOCTLStopBits = ucStopBits;
        switch ( eParity )
        {
        default:
        case MB_PAR_NONE:
            ulIOCTLParity = 0;
            break;
        case MB_PAR_ODD:
            ulIOCTLParity = 1;
            break;
        case MB_PAR_EVEN:
            ulIOCTLParity = 2;
            break;
        }
        ulIOCTLTimeout = ulSerialRxTimeMaxMS( ulBaudRate );

        arszUARTDevs[ucPort].pvInitFN(  );
        if( -1 == ( iFd = _open( arszUARTDevs[ucPort].szDevName, _O_RDWR | _O_BINARY ) ) )
        {
            eStatus = MB_EPORTERR;
        }
        else if( -1 == _ioctl( iFd, UART_SETSPEED, &ulIOCTLBaudRate ) )
        {
            eStatus = MB_EPORTERR;
        }
        else if( -1 == _ioctl( iFd, UART_SETPARITY, &ulIOCTLParity ) )
        {
            eStatus = MB_EPORTERR;
        }
        else if( -1 == _ioctl( iFd, UART_SETDATABITS, &ulIOCTLDataBits ) )
        {
            eStatus = MB_EPORTERR;
        }
        else if( -1 == _ioctl( iFd, UART_SETREADTIMEOUT, &ulIOCTLTimeout ) )
        {
            eStatus = MB_EPORTERR;
        }
        else if( -1 == _ioctl( iFd, UART_SETSTOPBITS, &ulIOCTLStopBits ) )
        {
            eStatus = MB_EPORTERR;
        }
        else if( -1 == _ioctl( iFd, UART_SETFLOWCONTROL, &ulIOCTLFlow ) )
        {
            eStatus = MB_EPORTERR;
        }
        else
        {
            pxIntHdl->iUartFD = iFd;
            pxIntHdl->xMBHdl = xMBHdl;
            pxIntHdl->bIsRunning = TRUE;

            if( ( HANDLE ) 0 == NutThreadCreate( "MBP-SER", vMBPSerialHandlerThread, pxIntHdl, 512 ) )
            {
                eStatus = MB_EPORTERR;
            }
            else
            {
                *pxSerialHdl = pxIntHdl;
                eStatus = MB_ENOERR;
            }
        }
        if( MB_ENOERR != eStatus )
        {
            if( -1 != iFd )
            {
                ( void )_close( iFd );
            }
            HDL_RESET( pxIntHdl );
        }
    }
    else
    {
        eStatus = MB_ENORES;
    }
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER, "[IDX=?] Serial init %s.\n",
                     eStatus == MB_ENOERR ? "okay" : "failed" );
    }
#endif
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialClose( xMBPSerialHandle xSerialHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialIntHandle *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER, "[IDX=" MBP_FORMAT_USHORT "] Marking instance for close.\n",
                         pxSerialIntHdl->ubIdx );
        }
#endif
        pxSerialIntHdl->bIsRunning = FALSE;
        NutEventPost( &( pxSerialIntHdl->xWaitHdl ) );
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialTxEnable( xMBPSerialHandle xSerialHdl, pbMBPSerialTransmitterEmptyCB pbMBPTransmitterEmptyFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialIntHandle *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        if( NULL != pbMBPTransmitterEmptyFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pbMBPTransmitterEmptyFN );
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = pbMBPTransmitterEmptyFN;
        }
        else
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
        }
        NutEventPost( &( pxSerialIntHdl->xWaitHdl ) );
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBPReceiveFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialIntHandle *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        if( NULL != pvMBPReceiveFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pvMBPReceiveFN );
            pxSerialIntHdl->pvMBPReceiveFN = pvMBPReceiveFN;
        }
        else
        {
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
        }
        NutEventPost( &( pxSerialIntHdl->xWaitHdl ) );
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

STATIC void
vMBPSerialHandlerThread( void *pvArg )
{
    BOOL            bIsRunning = TRUE;
    xSerialIntHandle *pxSerialIntHdl = pvArg;
    vTmrHandler( TRUE );
    int             iBytesRead, iBytesWritten, iBytesSent;
    USHORT          usBytesToSend;

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER, "[IDX=" MBP_FORMAT_USHORT "] Started handler thread.\n",
                     pxSerialIntHdl->ubIdx );
    }
#endif
    do
    {
        /* Wait here - We might be woken by any of the receiver/transmitter/close functions
         * within the stack.
         */
        ( void )NutEventWait( &( pxSerialIntHdl->xWaitHdl ), SER_RX_GUARD_TIME_MS );
        MBP_ENTER_CRITICAL_SECTION(  );
        /* Make a copy of this variable for the loop exit check. */
        bIsRunning = pxSerialIntHdl->bIsRunning;
        /* Handle receiving of frames */
        if( pxSerialIntHdl->bIsRunning && ( NULL != pxSerialIntHdl->pvMBPReceiveFN ) )
        {
            MBP_EXIT_CRITICAL_SECTION(  );
            iBytesRead = _read( pxSerialIntHdl->iUartFD, pxSerialIntHdl->arubRxTxBuffer, SER_RXTX_BUFFER_SIZE );
            if( iBytesRead > 0 )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER,
                                 "[IDX=" MBP_FORMAT_USHORT "] Received " MBP_FORMAT_USHORT " bytes.\n",
                                 pxSerialIntHdl->ubIdx, ( USHORT ) iBytesRead );
                }
#endif
                MBP_ENTER_CRITICAL_SECTION(  );
                if( pxSerialIntHdl->bIsRunning && ( NULL != pxSerialIntHdl->pvMBPReceiveFN ) )
                {
                    pxSerialIntHdl->pvMBPReceiveFN( pxSerialIntHdl->xMBHdl, pxSerialIntHdl->arubRxTxBuffer,
                                                    ( USHORT ) iBytesRead );
                }
                MBP_EXIT_CRITICAL_SECTION(  );
            }
            NutEventPost( &( pxSerialIntHdl->xWaitHdl ) );
        }
        else
        {
            MBP_EXIT_CRITICAL_SECTION(  );
        }

        /* Handle transmission of frames */
        do
        {
            usBytesToSend = 0;
            MBP_ENTER_CRITICAL_SECTION(  );
            if( pxSerialIntHdl->bIsRunning && ( NULL != pxSerialIntHdl->pbMBPTransmitterEmptyFN ) )
            {
                if( !pxSerialIntHdl->pbMBPTransmitterEmptyFN( pxSerialIntHdl->xMBHdl, pxSerialIntHdl->arubRxTxBuffer,
                                                              SER_RXTX_BUFFER_SIZE, &usBytesToSend ) )
                {
                    /* Warning: Undocumented use of API. First flush transmit buffer and then
                     * empty receive buffer.
                     */
                    _write( pxSerialIntHdl->iUartFD, 0, 0 );
                    /* Warning: There is still one buffer in the output queue. The reason is that
                     * Nut/OS simply checks its internal buffer for zero size but not the underlying
                     * buffering in the UART device. Therefore we wait simply 1 millisecond which
                     * is good enough for baudrates starting from 9600.
                     */
                    NutDelay( 1 );
                    _read( pxSerialIntHdl->iUartFD, 0, 0 );
                    pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
                }
                NutEventPost( &( pxSerialIntHdl->xWaitHdl ) );
            }
            MBP_EXIT_CRITICAL_SECTION(  );
            if( usBytesToSend > 0 )
            {
                iBytesSent = 0;
                do
                {
                    if( -1 ==
                        ( iBytesWritten =
                          _write( pxSerialIntHdl->iUartFD, &pxSerialIntHdl->arubRxTxBuffer[iBytesSent],
                                  ( int )usBytesToSend ) ) )
                    {
                        break;
                    }
                    else
                    {
                        iBytesSent += iBytesWritten;
                    }
                }
                while( iBytesSent < usBytesToSend );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER,
                                 "[IDX=" MBP_FORMAT_USHORT "] Sent " MBP_FORMAT_USHORT " of " MBP_FORMAT_USHORT
                                 " bytes.\n", pxSerialIntHdl->ubIdx, ( USHORT ) iBytesSent, usBytesToSend );
                }
#endif
            }
        }
        while( usBytesToSend > 0 );

        /* Process all Timers */
        vTmrHandler( FALSE );
    }
    while( bIsRunning );


    MBP_ENTER_CRITICAL_SECTION(  );
    ( void )_close( pxSerialIntHdl->iUartFD );
    HDL_RESET( pxSerialIntHdl );
    MBP_EXIT_CRITICAL_SECTION(  );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER, "[IDX=" MBP_FORMAT_USHORT "] Closed handler thread.\n",
                     pxSerialIntHdl->ubIdx );
    }
#endif
    NutThreadExit(  );
}

STATIC          ULONG
ulSerialRxTimeMaxMS( ULONG ulBaudRate )
{
    ULONG           ulTimeOut;
    ulTimeOut = ( SER_RXTX_BUFFER_SIZE * 11UL * 1000UL + ulBaudRate / 2 ) / ulBaudRate;
    if( ulTimeOut < SER_RX_READ_TIME_MIN_MS )
    {
        ulTimeOut = SER_RX_READ_TIME_MIN_MS;
    }
    return ulTimeOut;
}

USHORT
usMBPSerialRTUV2Timeout( ULONG ulBaudRate )
{
    ULONG           ulBasicTimeoutMS;

    /* The minimum time for the RTUV2 timeout is the time required to fill the
     * complete RX buffer plus the guard time to work arround inaccuracies in
     * scheduling.
     */
    ulBasicTimeoutMS = ulSerialRxTimeMaxMS( ulBaudRate ) + SER_RX_GUARD_TIME_MS;
    return ( USHORT ) ulBasicTimeoutMS;
}

STATIC void
vMBPSerialHardwareInitUART0( void )
{
    DDRB |= _BV( PB0 ) | _BV( PB1 );
    PORTB &= ~( _BV( PB0 ) | _BV( PB1 ) );
    ( void )NutRegisterDevice( &DEV_UART, 0, 0 );
}

STATIC void
vMBPSerialHardwareInitUART1( void )
{
    DDRB |= _BV( PB2 ) | _BV( PB3 );
    PORTB &= ~( _BV( PB2 ) | _BV( PB3 ) );
    ( void )NutRegisterDevice( &DEV_UART1, 0, 0 );
}
