/* 
 * MODBUS Library: Win32 port
 * Copyright (c) 2008-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial.c,v 1.14 2011-12-04 18:53:49 embedded-solutions.cwalter Exp $
 *
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define IDX_INVALID                         ( 255 )
#define SER_MAX_INSTANCES                   ( 8 )
#define USE_WIN32_COMM_PURGE                ( 0 )
#define SER_WAIT_EVENT_TIMEOUT              ( 100 )
#define SER_DEBUG                           ( 1 )
#define SER_BUFFER_SIZE						( 256 )
#define THREAD_STACKSIZE_MAX                ( 16384 )

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->xSerHdl = INVALID_HANDLE_VALUE; \
    ( x )->xEvHdl = INVALID_HANDLE_VALUE; \
    ( x )->pbMBPTransmitterEmptyFN = NULL; \
    ( x )->pvMBPReceiveFN = NULL; \
	( x )->pbMBPFrameTimeoutFN = NULL; \
	( x )->eMode = MB_RTU; \
    ( x )->xMBMHdl = MB_HDL_INVALID; \
    ( x )->bIsRunning = FALSE; \
} while( 0 );

#define HDL_RESET_FULL( x ) do { \
    HDL_RESET( x ); \
    ( x )->xThreadHdl = NULL; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    HANDLE          xSerHdl;
    HANDLE          xThreadHdl;
    HANDLE          xEvHdl;
    pbMBPSerialTransmitterEmptyAPIV2CB pbMBPTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV2CB pvMBPReceiveFN;
    pbMBPTimerExpiredCB pbMBPFrameTimeoutFN;
    eMBSerialMode   eMode;
    BOOL            bIsRunning;
    xMBHandle       xMBMHdl;
} xSerialHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialHandle xSerialHdls[SER_MAX_INSTANCES];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC DWORD WINAPI prrvSerHandlerThread( LPVOID lpParameter );
STATIC BOOL     xMBPSerialSetTimeout( HANDLE xSerialHandle, ULONG ulBaudRate );
STATIC void     vMBPMyCommPurge( HANDLE hFile, DWORD dwFlags );

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl, pbMBPTimerExpiredCB pbFrameTimeoutFN, eMBSerialMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    CHAR            szDevice[16];
    DCB             dcb;
    UBYTE           ubIdx;
    xSerialHandle  *pxSerHdl = NULL;
    DWORD           dwThreadId;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            HDL_RESET_FULL( &xSerialHdls[ubIdx] );
        }
        bIsInitalized = TRUE;
    }

    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
    {
        if( IDX_INVALID == xSerialHdls[ubIdx].ubIdx )
        {
            pxSerHdl = &( xSerialHdls[ubIdx] );
            /* Cleanup old unused thread data. */
            if( NULL != pxSerHdl->xThreadHdl )
            {
                ( void )CloseHandle( pxSerHdl->xThreadHdl );
                pxSerHdl->xThreadHdl = NULL;
            }
            HDL_RESET_FULL( pxSerHdl );
            pxSerHdl->ubIdx = ubIdx;
            break;
        }
    }

    if( NULL != pxSerHdl )
    {
        _snprintf( szDevice, 16, "\\\\.\\COM%d", ucPort );
        if( INVALID_HANDLE_VALUE == ( pxSerHdl->xSerHdl = CreateFileA( szDevice, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL ) ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't open serial port %s: %s\n", szDevice, Error2String( GetLastError(  ) ) );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        else if( !GetCommState( pxSerHdl->xSerHdl, &dcb ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't not get communication state: %s\n", Error2String( GetLastError(  ) ) );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        else
        {
            /* COM port is now open and we have its current settings in the DCB. This settings are
             * now changed to reflect the desired configuration.
             */

            /* First override possible old settings. We do not want to use any flow control
             * or other options.
             */
            dcb.fBinary = TRUE;
            dcb.fOutxCtsFlow = FALSE;
            dcb.fOutxDsrFlow = FALSE;
            dcb.fDtrControl = DTR_CONTROL_DISABLE;
            dcb.fDsrSensitivity = FALSE;
            dcb.fTXContinueOnXoff = TRUE;
            dcb.fInX = FALSE;
            dcb.fOutX = FALSE;
            dcb.fErrorChar = FALSE;
            dcb.fNull = FALSE;
            dcb.fRtsControl = RTS_CONTROL_DISABLE;
            dcb.fAbortOnError = FALSE;

            /* Now configure parity, databits and stopbits */
            dcb.BaudRate = ulBaudRate;
            switch ( eParity )
            {
            case MB_PAR_NONE:
                dcb.Parity = NOPARITY;
                dcb.fParity = 0;
                break;
            case MB_PAR_EVEN:
                dcb.Parity = EVENPARITY;
                dcb.fParity = 1;
                break;
            case MB_PAR_ODD:
                dcb.Parity = ODDPARITY;
                dcb.fParity = 1;
                break;
            default:
                eStatus = MB_EINVAL;
            }
            switch ( ucDataBits )
            {
            case 8:
                dcb.ByteSize = 8;
                break;
            case 7:
                dcb.ByteSize = 7;
                break;
            default:
                eStatus = MB_EINVAL;
            }
            switch ( ucStopBits )
            {
            case 1:
                dcb.StopBits = ONESTOPBIT;
                break;
            case 2:
                dcb.StopBits = TWOSTOPBITS;
                break;
            default:
                eStatus = MB_EINVAL;
            }

            if( MB_ENOERR == eStatus )
            {
                /* Initialize the data structure. */
                pxSerHdl->xMBMHdl = xMBMHdl;
                pxSerHdl->bIsRunning = TRUE;
                pxSerHdl->pbMBPFrameTimeoutFN = pbFrameTimeoutFN;
                pxSerHdl->eMode = eMode;
                if( !SetCommState( pxSerHdl->xSerHdl, &dcb ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't set settings for serial device %s: %s\n",
                                     szDevice, Error2String( GetLastError(  ) ) );
                    }
#endif
                    eStatus = MB_EPORTERR;
                }
                else if( !xMBPSerialSetTimeout( pxSerHdl->xSerHdl, ulBaudRate ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't set timeouts for serial device: %s\n", Error2String( GetLastError(  ) ) );
                    }
#endif
                    eStatus = MB_EPORTERR;
                }
                else if( !SetCommMask( pxSerHdl->xSerHdl, 0 ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL,
                                     "Can't set communication event mask for serial device %s: %s\n", szDevice, Error2String( GetLastError(  ) ) );
                    }
#endif
                    eStatus = MB_EPORTERR;
                }
                else if( INVALID_HANDLE_VALUE == ( pxSerHdl->xEvHdl = CreateEvent( NULL, FALSE, FALSE, NULL ) ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't create event object: %s\n", Error2String( GetLastError(  ) ) );
                    }
#endif
                    eStatus = MB_EPORTERR;
                }
                else if( NULL == ( pxSerHdl->xThreadHdl = CreateThread( NULL, THREAD_STACKSIZE_MAX, prrvSerHandlerThread, pxSerHdl, 0, &dwThreadId ) ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't create handler thread: %s\n", Error2String( GetLastError(  ) ) );
                    }
#endif
                    eStatus = MB_EPORTERR;
                }
                else
                {
                    *pxSerialHdl = pxSerHdl;
                    eStatus = MB_ENOERR;
                }
            }
        }
        if( MB_ENOERR != eStatus )
        {
            /* No thread must have been started in the error case. */
            MBP_ASSERT( NULL == pxSerHdl->xThreadHdl );
            if( INVALID_HANDLE_VALUE != pxSerHdl->xSerHdl )
            {
                ( void )CloseHandle( pxSerHdl->xSerHdl );
            }
            if( INVALID_HANDLE_VALUE != pxSerHdl->xEvHdl )
            {
                ( void )CloseHandle( pxSerHdl->xEvHdl );
            }
            HDL_RESET_FULL( pxSerHdl );
        }
    }
    else
    {
        eStatus = MB_ENORES;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

STATIC void
vMBPMyCommPurge( HANDLE hFile, DWORD dwFlags )
{
    DWORD           dwError;
    COMSTAT         xComStat;
    BOOL            bHasMoreData;
    UBYTE           arubBuffer[16];
    DWORD           dwBytesToRead;
    DWORD           dwBytesRead;
#if USE_WIN32_COMM_PURGE
    PurgeComm( hFile, dwFlags );
#else
    if( dwFlags & ( PURGE_RXABORT | PURGE_RXCLEAR ) )
    {
        bHasMoreData = TRUE;
        do
        {
            if( !ClearCommError( hFile, &dwError, &xComStat ) )
            {
                bHasMoreData = FALSE;
            }
            else if( xComStat.cbInQue > 0 )
            {
                dwBytesToRead = xComStat.cbInQue > _countof( arubBuffer ) ? _countof( arubBuffer ) : xComStat.cbInQue;
                if( !ReadFile( hFile, arubBuffer, dwBytesToRead, &dwBytesRead, NULL ) )
                {
                    bHasMoreData = FALSE;
                }
            }
            else
            {
                bHasMoreData = FALSE;
            }
        }
        while( bHasMoreData );
    }
    if( dwFlags & ( PURGE_TXABORT | PURGE_TXCLEAR ) )
    {
        bHasMoreData = TRUE;
        do
        {
            if( !ClearCommError( hFile, &dwError, &xComStat ) )
            {
                bHasMoreData = FALSE;
            }
            else if( xComStat.cbOutQue > 0 )
            {
                Sleep( 10 );
            }
            else
            {
                bHasMoreData = FALSE;
            }
        }
        while( bHasMoreData );
    }
#endif
}

STATIC          BOOL
xMBPSerialSetTimeout( HANDLE xSerialHandle, ULONG ulBaudRate )
{
    static const DWORD guard_time_ms = 20;
    BOOL            bStatus;
    COMMTIMEOUTS    cto;

    /* The guard_time_ms does not affect the performance of the stack in case of no errors. It is set
     * quite high to deal with multiple latencies (e.g. USB FTDI polling, ...)
     */
    cto.ReadIntervalTimeout = 0;
    cto.ReadTotalTimeoutConstant = ( 11UL * 1000UL + ulBaudRate / 2 ) / ulBaudRate + guard_time_ms;
    cto.ReadTotalTimeoutMultiplier = 0;
    cto.WriteTotalTimeoutConstant = ( 11UL * 1000UL * SER_BUFFER_SIZE + ulBaudRate / 2 ) / ulBaudRate + guard_time_ms;
    cto.WriteTotalTimeoutMultiplier = 0;

    if( !SetCommTimeouts( xSerialHandle, &cto ) )
    {
        bStatus = FALSE;
    }
    else
    {
        bStatus = TRUE;
    }

    return bStatus;
}

eMBErrorCode
eMBPSerialClose( xMBPSerialHandle xSerialHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = ( xSerialHandle * ) xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        pxSerialIntHdl->bIsRunning = FALSE;
        eStatus = MB_ENOERR;
        if( !SetEvent( pxSerialIntHdl->xEvHdl ) )
        {
            eStatus = MB_EPORTERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialTxEnable( xMBPSerialHandle xSerialHdl, pbMBPSerialTransmitterEmptyCB pbMBPTransmitterEmptyFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = ( xSerialHandle * ) xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pbMBPTransmitterEmptyFN )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[IDX=%hu]: transmitter enabled\n", ( USHORT ) pxSerialIntHdl->ubIdx );
            }
#endif
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = pbMBPTransmitterEmptyFN;
        }
        else
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[IDX=%hu]: transmitter disabled\n", ( USHORT ) pxSerialIntHdl->ubIdx );
            }
#endif
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
            ( void )vMBPMyCommPurge( pxSerialIntHdl->xSerHdl, ( PURGE_TXABORT | PURGE_TXCLEAR ) );
        }
        if( !SetEvent( pxSerialIntHdl->xEvHdl ) )
        {
            eStatus = MB_EPORTERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBPReceiveFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = ( xSerialHandle * ) xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pvMBPReceiveFN )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[IDX=%hu]: receiver enabled\n", ( USHORT ) pxSerialIntHdl->ubIdx );
            }
#endif
            pxSerialIntHdl->pvMBPReceiveFN = pvMBPReceiveFN;
        }
        else
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[IDX=%hu]: receiver disabled\n", ( USHORT ) pxSerialIntHdl->ubIdx );
            }
#endif
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
            ( void )vMBPMyCommPurge( pxSerialIntHdl->xSerHdl, ( PURGE_RXABORT | PURGE_RXCLEAR ) );
        }
        if( !SetEvent( pxSerialIntHdl->xEvHdl ) )
        {
            eStatus = MB_EPORTERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

DWORD           WINAPI
prrvSerHandlerThread( LPVOID lpParameter )
{
    UBYTE           arubBuffer[SER_BUFFER_SIZE];
    xSerialHandle  *pxSerHdl = ( xSerialHandle * ) lpParameter;
    HANDLE          xSerDevHdl;
    pbMBPSerialTransmitterEmptyAPIV2CB pbMBPTransmitterEmptyFN = NULL;
    pvMBPSerialReceiverAPIV2CB pvMBPReceiveFN = NULL;
    BOOL            bIsRunning = TRUE;
    DWORD           dwBytesReadTotal, dwBytesRead;
    DWORD           dwBytesToSend, dwBytesSent;
    USHORT          usBytesToSend;
    BOOL            bFrameIsComplete;
    BOOL            bTimeout;
    BOOL            bIsFirstRead;
    USHORT          usInhibitEOFDetectionCode = 0;
    USHORT          usCRC16;

    DWORD           dwFrameReceived;

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
    DWORD           dwReadStart, dwReadEnd;
    DWORD           dwWriteStart, dwWriteEnd;
    DWORD           dwPollOld, dwPollCur, dwPollDelta;

    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
    {
        dwPollOld = GetTickCount(  );
    }
#endif

    if( !SetThreadPriority( GetCurrentThread(  ), THREAD_PRIORITY_TIME_CRITICAL ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_SERIAL ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't increase thread priority: %s\n", Error2String( GetLastError(  ) ) );
        }
#endif
    }

    do
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
        {
            dwPollCur = GetTickCount(  );
            dwPollDelta = dwPollCur - dwPollOld;
            dwPollOld = dwPollCur;
        }
#endif

        /* Make a copy of the variables since they are shared. */
        MBP_ENTER_CRITICAL_SECTION(  );
        bIsRunning = pxSerHdl->bIsRunning;
        xSerDevHdl = pxSerHdl->xSerHdl;
        pvMBPReceiveFN = pxSerHdl->pvMBPReceiveFN;
        pbMBPTransmitterEmptyFN = pxSerHdl->pbMBPTransmitterEmptyFN;
        MBP_EXIT_CRITICAL_SECTION(  );

        /* Make a dummy wait if no event has happend. */
        if( bIsRunning && ( NULL == pbMBPTransmitterEmptyFN ) && ( NULL == pvMBPReceiveFN ) )
        {
            switch ( WaitForSingleObject( pxSerHdl->xEvHdl, SER_WAIT_EVENT_TIMEOUT ) )
            {
            default:
            case WAIT_OBJECT_0:
                break;
            case WAIT_FAILED:
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't wait on event object%s\n", Error2String( GetLastError(  ) ) );
                }
#endif
                break;
            }
        }

        /* If receiver is enabled poll for data. */
        if( pvMBPReceiveFN )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
            {
                dwReadStart = GetTickCount(  );
            }
#endif
            /* First read a single byte so that only total timeouts are used. This 
             * is required by the porting layer so that it does not block.
             */
            if( ReadFile( xSerDevHdl, &arubBuffer[0], 1, &dwBytesRead, NULL ) && ( dwBytesRead > 0 ) )
            {
                dwBytesReadTotal = 0;
                bFrameIsComplete = FALSE;
                bTimeout = FALSE;
                bIsFirstRead = TRUE;
                usCRC16 = 0xFFFFU;
                do
                {
                    /* dwBytesRead handles the first case as the Read has already been executed */
                    if( !bIsFirstRead && !ReadFile( xSerDevHdl, &arubBuffer[dwBytesReadTotal], 1, &dwBytesRead, NULL ) )
                    {
                        /* The read failed - Abort the loop */
                        break;
                    }
                    bIsFirstRead = FALSE;

                    if( 0 == dwBytesRead )
                    {
                        bTimeout = TRUE;
                    }
                    else
                    {
                        switch ( pxSerHdl->eMode )
                        {
                        case MB_ASCII:
                            if( arubBuffer[dwBytesReadTotal] == '\n' )
                            {
                                bFrameIsComplete = TRUE;
                            }
                            break;
                        case MB_RTU:
                            if( 0x0000U == ( usCRC16 = prvCRC16Update( usCRC16, arubBuffer[dwBytesReadTotal] ) ) )
                            {
                                if( 0 == usInhibitEOFDetectionCode )
                                {
                                    if( bMBGuessRTUFrameIsComplete( arubBuffer, dwBytesReadTotal + 1 ) )
                                    {
                                        bFrameIsComplete = TRUE;
                                    }
                                }
                            }
                            break;
                        }
                        /* We have read one more character */
                        dwBytesReadTotal += dwBytesRead;
                    }
                }
                while( ( dwBytesReadTotal < SER_BUFFER_SIZE ) && !bFrameIsComplete && !bTimeout );

                /* In case of a CRC16 error we always return with a timeout. This can happen as 
                 * an result of an incorrect guessed EOF (Very unlikely but still can happen) or
                 * due to an communication error. To recover from this case we disable the performance
                 * improvement until we have a proper frame with standard EOF timeout and no CRC error.
                 */
                if( bTimeout )
                {
                    if( 0x0000U != usCRC16 )
                    {
                         usInhibitEOFDetectionCode = 1;
                    }
                    else
                    {
                        usInhibitEOFDetectionCode = 0;
                    }
                }

                
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
                {
                    dwReadEnd = GetTickCount(  );
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL,
                                    "[IDX=%hu]: read: start=%lu, end=%lu, bytes read=%lu\n",
                                    ( USHORT ) pxSerHdl->ubIdx, dwReadStart, dwReadEnd, dwBytesReadTotal );
                }
#endif

                if( dwBytesReadTotal > 0 )
                {
                    MBP_ENTER_CRITICAL_SECTION(  );
                    if( pxSerHdl->bIsRunning && ( NULL != pxSerHdl->pvMBPReceiveFN ) )
                    {
                        pxSerHdl->pvMBPReceiveFN( pxSerHdl->xMBMHdl, arubBuffer, ( USHORT ) dwBytesReadTotal );
                        if( NULL != pxSerHdl->pbMBPFrameTimeoutFN )
                        {
                            pxSerHdl->pbMBPFrameTimeoutFN( pxSerHdl->xMBMHdl );
                        }
                    }
                    MBP_EXIT_CRITICAL_SECTION(  );
                    dwFrameReceived = GetTickCount(  );
                }
            }
        }

        do
        {
            dwBytesSent = 0;
            dwBytesToSend = 0;
            MBP_ENTER_CRITICAL_SECTION(  );
            if( pxSerHdl->bIsRunning && ( NULL != pxSerHdl->pbMBPTransmitterEmptyFN ) )
            {
                if( !pxSerHdl->pbMBPTransmitterEmptyFN( pxSerHdl->xMBMHdl, arubBuffer, SER_BUFFER_SIZE, &usBytesToSend ) )
                {
                    pxSerHdl->pbMBPTransmitterEmptyFN = NULL;
                    dwBytesToSend = 0;
                }
                else
                {
                    dwBytesToSend = usBytesToSend;
                }
            }
            MBP_EXIT_CRITICAL_SECTION(  );

            while( dwBytesToSend > 0 )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
                {
                    dwWriteStart = GetTickCount(  );
                }
#endif
                /* Ensure we do not send frames to fast */
                if( ( GetTickCount( ) - dwFrameReceived ) < 2U )
                {
                    Sleep( 2 );
                }
                if( !WriteFile( xSerDevHdl, arubBuffer, dwBytesToSend, &dwBytesSent, NULL ) )
                {
                    /* I/O failed. There is nothing we can do about it. The
                     * caller will notice anyway since it will not receive
                     * any reply.
                     */
                    break;
                }
                else
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( SER_DEBUG == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
                    {
                        dwWriteEnd = GetTickCount(  );
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL,
                                     "[IDX=%hu]: write: start=%lu, end=%lu, sent=%lu of %lu\n",
                                     ( USHORT ) pxSerHdl->ubIdx, dwWriteStart, dwWriteEnd, dwBytesSent, dwBytesToSend );
                    }
#endif
                    dwBytesToSend -= dwBytesSent;
                }
            }
        }
        while( dwBytesSent > 0 );
    }
    while( bIsRunning );

    ( void )vMBPMyCommPurge( pxSerHdl->xSerHdl, ( PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) );
    ( void )CloseHandle( pxSerHdl->xSerHdl );
    ( void )CloseHandle( pxSerHdl->xEvHdl );
    MBP_ENTER_CRITICAL_SECTION(  );
    HDL_RESET( pxSerHdl );
    MBP_EXIT_CRITICAL_SECTION(  );

    ExitThread( 0 );
}

void
vMBPSerialDLLClose( void )
{
    BOOL            bThreadsRunning = FALSE;
    UBYTE           ubIdx;
    HANDLE          hThreadHandle;

    /* If this code is called we know that no MODBUS instances are
     * newly created (Because of the INIT lock) and that all
     * still running serial threads are going to shut down since
     * their bIsRunning flag has been set to FALSE.
     */
    do
    {
        hThreadHandle = NULL;
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            if( NULL != xSerialHdls[ubIdx].xThreadHdl )
            {
                hThreadHandle = xSerialHdls[ubIdx].xThreadHdl;
                /* If still running make sure this instance is shutdown. This
                 * should not happen wo we print a log message here.
                 */
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) && xSerialHdls[ubIdx].bIsRunning )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL,
                                 "[IDX=%hu]: thread still running. forcing shutdown (please report).\n", xSerialHdls[ubIdx].ubIdx );
                }
#endif
                xSerialHdls[ubIdx].bIsRunning = FALSE;
                xSerialHdls[ubIdx].xThreadHdl = NULL;
                break;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
        if( NULL != hThreadHandle )
        {
            if( WAIT_OBJECT_0 != WaitForSingleObject( hThreadHandle, 10000 ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "serial thread not shut down. leaving thread running (possibly holding serial device)!\n" );
                }
#endif
            }
            ( void )CloseHandle( hThreadHandle );
        }
    }
    while( NULL != hThreadHandle );
}
