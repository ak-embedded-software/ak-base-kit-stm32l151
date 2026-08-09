/* 
 * MODBUS Library: Win32 port
 * Copyright (c) 2008-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbporttimerce.c,v 1.5 2011-12-04 18:53:49 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdafx.h"
#include "windows.h"
#include "Mmsystem.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/

#define MAX_TIMER_HDLS                  ( 16 )
#define IDX_INVALID                     ( 255 )
#define TIMER_TIMEOUT_INVALID           ( 65535U )
#define TIMER_TIMEOUT                   ( 10 )
#define TIMER_RESOLUTION                ( 5 )
#define TIMER_DEBUG                     ( 1 )
#define THREAD_STACKSIZE_MAX            ( 16384 )

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
HANDLE          xWinTimerEvHdl = NULL;
STATIC MMRESULT xWinMMTimerId;

/* ----------------------- Static functions ---------------------------------*/
STATIC int      iMPPTimerExit( void );
STATIC DWORD WINAPI prrvTimerHandlerThread( LPVOID lpParameter );
STATIC UINT     wTimerRes;
STATIC int      vMBPTimerExit( void );

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBPTimerInit( xMBPTimerHandle * xTimerHdl, USHORT usTimeOut1ms,
               pbMBPTimerExpiredCB pbMBPTimerExpiredFN, xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EPORTERR;
    UBYTE           ubIdx;
    DWORD           dwThreadId;
    LARGE_INTEGER   liDueTime;
    TIMECAPS        xTC;

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
            if( TIMERR_NOERROR != timeGetDevCaps( &xTC, sizeof( TIMECAPS ) ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TIMER ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't get multimedia timer capabilities: %s\n",
                                 Error2String( GetLastError(  ) ) );
                }
#endif
            }
            else
            {
                wTimerRes = min( max( xTC.wPeriodMin, TIMER_RESOLUTION ), xTC.wPeriodMax );
                if( NULL == ( xWinTimerEvHdl = CreateEvent( NULL, FALSE, FALSE, NULL ) ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TIMER ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't create event object: %s\n",
                                     Error2String( GetLastError(  ) ) );
                    }
#endif
                }
                else if( TIMERR_NOERROR != timeBeginPeriod( wTimerRes ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TIMER ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't set timer period: %s\n",
                                     Error2String( GetLastError(  ) ) );
                    }
#endif
                    ( void )CloseHandle( xWinTimerEvHdl );
                }
                else if( 0 ==
                         ( xWinMMTimerId =
                           timeSetEvent( TIMER_TIMEOUT, wTimerRes, xWinTimerEvHdl, 0,
                                         TIME_PERIODIC | TIME_CALLBACK_EVENT_SET ) ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TIMER ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't start multimedia timer: %s\n",
                                     Error2String( GetLastError(  ) ) );
                    }
#endif
                    ( void )CloseHandle( xWinTimerEvHdl );
                    ( void )timeEndPeriod( wTimerRes );
                }
                else if( NULL ==
                         ( xWinTimerThreadHdl =
                           CreateThread( NULL, THREAD_STACKSIZE_MAX, prrvTimerHandlerThread, NULL, 0, &dwThreadId ) ) )
                {
                    ( void )CloseHandle( xWinTimerEvHdl );
                    ( void )timeKillEvent( xWinMMTimerId );
                    ( void )timeEndPeriod( wTimerRes );
                }
                else
                {
                    bIsRunning = TRUE;
                }

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
#if defined( MBP_LEAK_TEST ) && ( MBP_LEAK_TEST == 1 )
            if( ( double )rand(  ) / ( double )RAND_MAX < MBP_LEAK_RATE )
            {
                eStatus = MB_ENORES;
            }
#else
            if( 0 )
            {
            }
#endif
            else
            {
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
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "Created timer %hu with timeout %hu ms.\n",
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
vMBPTimerDLLClose( void )
{
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    bIsRunning = FALSE;
    MBP_EXIT_CRITICAL_SECTION(  );

    /* Wait for thread to finish. */
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

void
vMBPTimerClose( xMBPTimerHandle xTimerHdl )
{
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    /* Double check for NULL but otherwise fails with VC2005, SP1 */
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
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[IDX=%hu] started at %u\n", ( USHORT ) pxTimerIntHdl->ubIdx,
                         pxTimerIntHdl->dwTimeOut );
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
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't increase thread priority: %s\n",
                         Error2String( GetLastError(  ) ) );
        }
#endif
    }

    do
    {
        switch ( WaitForSingleObject( xWinTimerEvHdl, INFINITE ) )
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
                            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[IDX=%hu] timeout at %u\n", ( USHORT ) ubIdx,
                                         dwCurrentTime );
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

    /* Close all other resourcess. */
    if( NULL != xWinTimerEvHdl )
    {
        ( void )CloseHandle( xWinTimerEvHdl );
        xWinTimerEvHdl = NULL;
    }
    if( 0 != xWinMMTimerId )
    {
        ( void )timeKillEvent( xWinMMTimerId );
        xWinMMTimerId = 0;
    }
    if( 0 != wTimerRes )
    {
        ( void )timeEndPeriod( wTimerRes );
        wTimerRes = 0;
    }
    ExitThread( 0 );
}
