/* 
 * MODBUS Library: LINUX/CYGWIN timer port
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbporttimer.c,v 1.6 2014-08-23 09:36:06 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>

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

#define RESET_HDL( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
	( x )->usNTimeOutMS = 0; \
    ( x )->xMBMHdl = MB_HDL_INVALID; \
    ( x )->pbMBPTimerExpiredFN = NULL; \
    ( x )->xStartTime.tv_nsec = 0; \
    ( x )->xStartTime.tv_sec = 0; \
    ( x )->bIsRunning = FALSE; \
} while( 0 )

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    struct timespec xStartTime;
    USHORT          usNTimeOutMS;
    BOOL            bIsRunning;
    xMBHandle       xMBMHdl;
    pbMBPTimerExpiredCB pbMBPTimerExpiredFN;
} xTimerInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xTimerInternalHandle arxTimerHdls[MAX_TIMER_HDLS];
STATIC BOOL     bIsInitialized = FALSE;
STATIC pthread_t xTimerThread;
STATIC struct itimerval xTimerIntervall;
STATIC sigset_t sigmsk;

/* ----------------------- Static functions ---------------------------------*/
STATIC void    *pvMBPTimerCB( void *pvArg );

STATIC int      timeval_subtract( result, x, y );

/* ----------------------- Start implementation -----------------------------*/

void
vMBPTimerDLLClose( void )
{
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    bIsInitialized = FALSE;
    MBP_EXIT_CRITICAL_SECTION(  );

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "DLL Close.\n" );
    }
#endif

    /* We do not need the lock anymore since the timer thread
     * has shutdown.
     */
    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
    {
        RESET_HDL( &arxTimerHdls[ubIdx] );
    }
}

eMBErrorCode
eMBPTimerInit( xMBPTimerHandle * xTimerHdl, USHORT usTimeOut1ms, pbMBPTimerExpiredCB pbMBPTimerExpiredFN, xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EPORTERR;
    pthread_attr_t  xAttr;
    int             i;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( ( NULL != xTimerHdl ) && ( NULL != pbMBPTimerExpiredFN ) && ( MB_HDL_INVALID != xHdl ) )
    {
        if( !bIsInitialized )
        {
            for( i = 0; i < MAX_TIMER_HDLS; i++ )
            {
                RESET_HDL( &arxTimerHdls[i] );
            }

            sigemptyset( &sigmsk );
            sigaddset( &sigmsk, SIGALRM );
            if( 0 != pthread_sigmask( SIG_BLOCK, &sigmsk, NULL ) )
            {
                eStatus = MB_EPORTERR;
            }
            else if( 0 != pthread_attr_init( &xAttr ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TIMER ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't set thread attributes: %s\n", strerror( errno ) );
                }
#endif
            }
            else if( 0 != pthread_attr_setdetachstate( &xAttr, PTHREAD_CREATE_DETACHED ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TIMER ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't set thread attributes: %s\n", strerror( errno ) );
                }
#endif
            }
            /* Create a handler thread. */
            else if( 0 == pthread_create( &xTimerThread, NULL, pvMBPTimerCB, NULL ) )
            {
                xTimerIntervall.it_interval.tv_sec = 0;
                xTimerIntervall.it_interval.tv_usec = 10000;
                xTimerIntervall.it_value.tv_sec = 0;
                xTimerIntervall.it_value.tv_usec = 10000;

                if( 0 != setitimer( ITIMER_REAL, &xTimerIntervall, NULL ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[XX] setitimer failed.\n" );
                    }
#endif
                }
                else
                {
                    bIsInitialized = TRUE;
                }
            }
            ( void )pthread_attr_destroy( &xAttr );
        }
        for( i = 0; i < MAX_TIMER_HDLS; i++ )
        {
            if( IDX_INVALID == arxTimerHdls[i].ubIdx )
            {
                break;
            }
        }
        if( ( MAX_TIMER_HDLS != i ) )
        {
            arxTimerHdls[i].pbMBPTimerExpiredFN = pbMBPTimerExpiredFN;
            arxTimerHdls[i].xMBMHdl = xHdl;
            arxTimerHdls[i].ubIdx = i;
            if( MB_ENOERR != eMBPTimerSetTimeout( &arxTimerHdls[i], usTimeOut1ms ) )
            {
                vMBPTimerClose( &arxTimerHdls[i] );
                eStatus = MB_EPORTERR;
            }
            else
            {
                *xTimerHdl = &arxTimerHdls[i];
                eStatus = MB_ENOERR;
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

    MBP_ENTER_CRITICAL_SECTION(  );
    if( ( NULL != xTimerHdl ) && MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%u] Closing handle.\n", pxTimerIntHdl->ubIdx );
        }
#endif
        RESET_HDL( pxTimerIntHdl );
        bIsInitialized = FALSE;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}

eMBErrorCode
eMBPTimerSetTimeout( xMBPTimerHandle xTimerHdl, USHORT usTimeOut1ms )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) && ( usTimeOut1ms > 0 ) && ( usTimeOut1ms != TIMER_TIMEOUT_INVALID ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%u] Setting Timeout to %u.\n", pxTimerIntHdl->ubIdx, usTimeOut1ms );
        }
#endif
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
        if( 0 == clock_gettime( CLOCK_REALTIME, &pxTimerIntHdl->xStartTime ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%u] Starting timer with a timeout of %u.\n",
                             pxTimerIntHdl->ubIdx, pxTimerIntHdl->usNTimeOutMS );
            }
#endif
            pxTimerIntHdl->bIsRunning = TRUE;
            eStatus = MB_ENOERR;
        }
        else
        {
            eStatus = MB_EPORTERR;
        }
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
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%u] Stopping timer.\n", pxTimerIntHdl->ubIdx );
        }
#endif
        pxTimerIntHdl->bIsRunning = FALSE;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

STATIC void    *
pvMBPTimerCB( void *pvArg )
{
    int             sig;
    BOOL            bIsRunning;
    unsigned int    ubIdx;
    struct timespec xCurTime, xDiffTime;

    do
    {
        if( sigwait( &sigmsk, &sig ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't wait for signal %d: %s.\n", strerror( errno ) );
            }
#endif
        }
        else
        {
            MBP_ENTER_CRITICAL_SECTION(  );
            for( ubIdx = 0; ubIdx < MAX_TIMER_HDLS; ubIdx++ )
            {
                if( ( IDX_INVALID != arxTimerHdls[ubIdx].ubIdx ) && ( arxTimerHdls[ubIdx].bIsRunning != FALSE ) )
                {
                    if( 0 != clock_gettime( CLOCK_REALTIME, &xCurTime ) )
                    {
                        bIsInitialized = FALSE;
                    }
                    else
                    {
                        if( timeval_subtract( &xDiffTime, &xCurTime, &arxTimerHdls[ubIdx].xStartTime ) )
                        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
                            {
                                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%u] %us %uus\n", arxTimerHdls[ubIdx].ubIdx,
                                             xDiffTime.tv_sec, xDiffTime.tv_nsec );
                            }
#endif
                        }
                        /* Time out, execute the handler. Note: struct timespc has values in ns. */
                        else if( ( xDiffTime.tv_nsec > ( ( long )arxTimerHdls[ubIdx].usNTimeOutMS * 1000000 ) ) || ( xDiffTime.tv_sec > 0 ) )
                        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
                            {
                                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%u] Timeout, executing handler.\n", arxTimerHdls[ubIdx].ubIdx );
                            }
#endif
                            arxTimerHdls[ubIdx].bIsRunning = FALSE;
                            ( void )arxTimerHdls[ubIdx].pbMBPTimerExpiredFN( arxTimerHdls[ubIdx].xMBMHdl );
                        }
                    }
                }
            }
            MBP_EXIT_CRITICAL_SECTION(  );
        }
        MBP_ENTER_CRITICAL_SECTION(  );
        bIsRunning = bIsInitialized;
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    while( bIsRunning );
    return NULL;
}

int
timeval_subtract( result, x, y )
     struct timespec *result, *x, *y;
{
    /* Perform the carry for the later subtraction by updating y. */
    if( x->tv_nsec < y->tv_nsec )
    {
        long            nsec = ( y->tv_nsec - x->tv_nsec ) / 1000000000 + 1;

        y->tv_nsec -= 1000000000 * nsec;
        y->tv_sec += nsec;
    }
    if( x->tv_nsec - y->tv_nsec > 1000000000 )
    {
        long            nsec = ( x->tv_nsec - y->tv_nsec ) / 1000000000;

        y->tv_nsec += 1000000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_nsec = x->tv_nsec - y->tv_nsec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}
