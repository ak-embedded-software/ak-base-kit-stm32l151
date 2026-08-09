/* 
 * MODBUS Library: Win32 port
 * Copyright (c) 2008-2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbporttimer.c,v 1.8 2011-12-04 18:53:49 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdafx.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/

#define MAX_TIMER_HDLS              ( 16 )
#define IDX_INVALID                 ( 255 )
#define TIMER_TIMEOUT_INVALID       ( 65535U )
#define TIMER_TIMEOUT               ( 5 )
#define TIMER_DEBUG                 ( 1 )

#define RESET_HDL( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->usNTimeOutMS = 0; \
    ( x )->dwTimeOut = 0; \
    ( x )->bIsRunning = FALSE; \
    ( x )->xMBMHdl = MB_HDL_INVALID; \
    ( x )->pbMBPTimerExpiredFN = NULL; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    USHORT          usNTimeOutMS;
    DWORD           dwTimeOut;
    BOOL            bIsRunning;
    xMBHandle       xMBMHdl;
    pbMBPTimerExpiredCB pbMBPTimerExpiredFN;
} xTimerInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xTimerInternalHandle arxTimerHdls[MAX_TIMER_HDLS];
STATIC BOOL     bIsRunning = FALSE;
HANDLE          xWinTimerThreadHdl = NULL;
HANDLE          xWinTimerHdl = NULL;

/* ----------------------- Static functions ---------------------------------*/
STATIC int      iMPPTimerExit( void );
STATIC DWORD WINAPI prrvTimerHandlerThread( LPVOID lpParameter );

/* ----------------------- Start implementation -----------------------------*/

void
vMBPTimerDLLClose( void )
{
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    bIsRunning = FALSE;
    MBP_EXIT_CRITICAL_SECTION(  );

    if( NULL != xWinTimerThreadHdl )
    {
        if( WAIT_OBJECT_0 != WaitForSingleObject( xWinTimerThreadHdl, 5000 ) )
        {
            MBP_ASSERT( 0 );
        }
        ( void )CloseHandle( xWinTimerThreadHdl );
        xWinTimerThreadHdl = NULL;
    }

    /* We do not need the lock anymore since the timer thread
     * has shutdown.
     */
    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
    {
        RESET_HDL( &arxTimerHdls[ubIdx] );
    }
}

eMBErrorCode
eMBPTimerInit( xMBPTimerHandle * xTimerHdl, USHORT usTimeOut1ms,
               pbMBPTimerExpiredCB pbMBPTimerExpiredFN, xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EPORTERR;
    UBYTE           ubIdx;
    DWORD           dwThreadId;
    LARGE_INTEGER   liDueTime;

    liDueTime.QuadPart = -( ( long long )TIMER_TIMEOUT * 10000LL );
    MBP_ENTER_CRITICAL_SECTION(  );
    if( ( NULL != xTimerHdl ) && ( NULL != pbMBPTimerExpiredFN ) && ( MB_HDL_INVALID != xHdl ) )
    {
        if( !bIsRunning )
        {
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
            {
                RESET_HDL( &arxTimerHdls[ubIdx] );
            }
            if( NULL == ( xWinTimerHdl = CreateWaitableTimer( NULL, FALSE, NULL ) ) )
            {
            }
            if( !SetWaitableTimer( xWinTimerHdl, &liDueTime, TIMER_TIMEOUT, NULL, NULL, TRUE ) )
            {
                ( void )CloseHandle( xWinTimerHdl );
            }
            else if( NULL ==
                     ( xWinTimerThreadHdl = CreateThread( NULL, 0, prrvTimerHandlerThread, NULL, 0, &dwThreadId ) ) )
            {
                ( void )CloseHandle( xWinTimerHdl );
            }
            else
            {
                bIsRunning = TRUE;
            }
        }
        if( bIsRunning )
        {
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
            {
                if( IDX_INVALID == arxTimerHdls[ubIdx].ubIdx )
                {
                    break;
                }
            }
            if( MAX_TIMER_HDLS != ubIdx )
            {
                arxTimerHdls[ubIdx].ubIdx = ubIdx;
                arxTimerHdls[ubIdx].usNTimeOutMS = usTimeOut1ms;
                arxTimerHdls[ubIdx].dwTimeOut = 0;
                arxTimerHdls[ubIdx].bIsRunning = FALSE;
                arxTimerHdls[ubIdx].xMBMHdl = xHdl;
                arxTimerHdls[ubIdx].pbMBPTimerExpiredFN = pbMBPTimerExpiredFN;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( TIMER_DEBUG == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "Created timer %hu with timeout %hu ms.\r\n",
                                 ( USHORT ) ubIdx, arxTimerHdls[ubIdx].usNTimeOutMS );
                }
#endif
                *xTimerHdl = &arxTimerHdls[ubIdx];
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_ENORES;
            }
        }
        else
        {
            eStatus = MB_ENORES;
        }
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

void
vMBPTimerClose( xMBPTimerHandle xTimerHdl )
{
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    if( ( NULL != xTimerHdl ) && MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( TIMER_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[IDX=%hu] closed timer.\n",
                         ( USHORT ) pxTimerIntHdl->ubIdx );
        }
#endif
        RESET_HDL( pxTimerIntHdl );
    }
}

eMBErrorCode
eMBPTimerSetTimeout( xMBPTimerHandle xTimerHdl, USHORT usTimeOut1ms )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) &&
        ( usTimeOut1ms > 0 ) && ( usTimeOut1ms != TIMER_TIMEOUT_INVALID ) )
    {
        pxTimerIntHdl->usNTimeOutMS = usTimeOut1ms;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPTimerStart( xMBPTimerHandle xTimerHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        pxTimerIntHdl->dwTimeOut = GetTickCount(  );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( TIMER_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[IDX=%hu] started at %u\r\n",
                         ( USHORT ) pxTimerIntHdl->ubIdx, pxTimerIntHdl->dwTimeOut );
        }
#endif
        pxTimerIntHdl->bIsRunning = TRUE;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPTimerStop( xMBPTimerHandle xTimerHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        pxTimerIntHdl->dwTimeOut = 0;
        pxTimerIntHdl->bIsRunning = FALSE;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

STATIC DWORD    WINAPI
prrvTimerHandlerThread( LPVOID lpParameter )
{
    UBYTE           ubIdx;
    DWORD           dwCurrentTime = GetTickCount(  );
    BOOL            bIsStillRunning;

    ( void )lpParameter;

    if( !SetThreadPriority( GetCurrentThread(  ), THREAD_PRIORITY_TIME_CRITICAL ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_TIMER ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't increase thread priority: %s",
                         Error2String( GetLastError(  ) ) );
        }
#endif
    }

    do
    {
        switch ( WaitForSingleObject( xWinTimerHdl, INFINITE ) )
        {
        case WAIT_OBJECT_0:
            MBP_ENTER_CRITICAL_SECTION(  );
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
            {
                if( ( IDX_INVALID != arxTimerHdls[ubIdx].ubIdx ) && ( arxTimerHdls[ubIdx].bIsRunning ) )
                {
                    dwCurrentTime = GetTickCount(  );
                    if( ( dwCurrentTime - arxTimerHdls[ubIdx].dwTimeOut ) >=
                        ( DWORD ) arxTimerHdls[ubIdx].usNTimeOutMS )
                    {
                        arxTimerHdls[ubIdx].bIsRunning = 0;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( TIMER_DEBUG == 1 )
                        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
                        {
                            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[IDX=%hu] timeout at %u\r\n",
                                         ( USHORT ) ubIdx, dwCurrentTime );
                        }
#endif
                        ( void )arxTimerHdls[ubIdx].pbMBPTimerExpiredFN( arxTimerHdls[ubIdx].xMBMHdl );
                    }
                }
            }
            MBP_EXIT_CRITICAL_SECTION(  );
            break;
        default:
            MBP_ASSERT( 0 );
            break;
        }
        MBP_ENTER_CRITICAL_SECTION(  );
        bIsStillRunning = bIsRunning;
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    while( bIsStillRunning );

    if( NULL != xWinTimerHdl )
    {
        ( void )CloseHandle( xWinTimerHdl );
        xWinTimerHdl = NULL;
    }
    ExitThread( 0 );
}
