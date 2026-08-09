/* 
 * MODBUS Library: LINUX/CYGWIN timer port
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbporttimerposix.c,v 1.7 2014-08-23 09:36:06 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <signal.h>
#include <time.h>
#include <string.h>

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
    ( x )->xMBPHdl = MB_HDL_INVALID; \
    ( x )->pbMBPTimerExpiredFN = NULL; \
    ( x )->bIsRunning = FALSE; \
    memset( &( ( x )->xTimerPosixHdl ), 0, sizeof( timer_t ) ); \
    memset( &( ( x )->xTimerSpec ), 0, sizeof( struct itimerspec ) ); \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    timer_t         xTimerPosixHdl;
    struct itimerspec xTimerSpec;
    BOOL            bIsRunning;
    xMBHandle       xMBPHdl;
    pbMBPTimerExpiredCB pbMBPTimerExpiredFN;
} xTimerInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xTimerInternalHandle arxTimerHdls[MAX_TIMER_HDLS];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     eMBPTimerCB( union sigval xSigVal );

/* ----------------------- Start implementation -----------------------------*/

void
vMBPTimerDLLClose( void )
{
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    bIsInitalized = FALSE;
    MBP_EXIT_CRITICAL_SECTION(  );

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
    int             iRes;
    int             i;
    struct timespec xTimerResolution;
    struct sigevent xTimerSigEvent;
    timer_t         xTimerPosixHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( ( NULL != xTimerHdl ) && ( NULL != pbMBPTimerExpiredFN ) && ( MB_HDL_INVALID != xHdl ) )
    {
        if( !bIsInitalized )
        {
            for( i = 0; i < MAX_TIMER_HDLS; i++ )
            {
                RESET_HDL( &arxTimerHdls[i] );
            }
            bIsInitalized = TRUE;
        }
        for( i = 0; i < MAX_TIMER_HDLS; i++ )
        {
            if( IDX_INVALID == arxTimerHdls[i].ubIdx )
            {
                break;
            }
        }
        if( MAX_TIMER_HDLS != i )
        {
            xTimerSigEvent.sigev_notify = SIGEV_THREAD;
            xTimerSigEvent.sigev_signo = 0;
            xTimerSigEvent.sigev_value.sival_ptr = &arxTimerHdls[i];
            xTimerSigEvent.sigev_notify_attributes = NULL;
            xTimerSigEvent.sigev_notify_function = eMBPTimerCB;

            if( 0 != timer_create( CLOCK_REALTIME, &xTimerSigEvent, &xTimerPosixHdl ) )
            {
                eStatus = MB_EPORTERR;
            }
            if( 0 != ( iRes = clock_getres( CLOCK_REALTIME, &xTimerResolution ) ) )
            {
                eStatus = MB_EPORTERR;
            }
            else
            {
                arxTimerHdls[i].pbMBPTimerExpiredFN = pbMBPTimerExpiredFN;
                arxTimerHdls[i].xMBPHdl = xHdl;
                arxTimerHdls[i].ubIdx = i;
                arxTimerHdls[i].xTimerPosixHdl = xTimerPosixHdl;
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
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%d] deleted\n", ( int )pxTimerIntHdl->ubIdx );
        }
#endif
        ( void )timer_delete( pxTimerIntHdl->xTimerPosixHdl );
        RESET_HDL( pxTimerIntHdl );
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
        pxTimerIntHdl->xTimerSpec.it_interval.tv_nsec = 0;
        pxTimerIntHdl->xTimerSpec.it_interval.tv_sec = 0;
        pxTimerIntHdl->xTimerSpec.it_value.tv_sec = usTimeOut1ms / 1000;
        pxTimerIntHdl->xTimerSpec.it_value.tv_nsec = 1000000UL * ( usTimeOut1ms % 1000 );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%d] set new timeout %hu\n", ( int )pxTimerIntHdl->ubIdx, usTimeOut1ms );
        }
#endif
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
        if( 0 == timer_settime( pxTimerIntHdl->xTimerPosixHdl, 0, &( pxTimerIntHdl->xTimerSpec ), NULL ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%d] started\n", ( int )pxTimerIntHdl->ubIdx );
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
    struct itimerspec xTimerSpec;

    xTimerSpec.it_interval.tv_nsec = 0;
    xTimerSpec.it_interval.tv_sec = 0;
    xTimerSpec.it_value.tv_sec = 0;
    xTimerSpec.it_value.tv_nsec = 0;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        if( 0 == timer_settime( pxTimerIntHdl->xTimerPosixHdl, 0, &( xTimerSpec ), NULL ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%d] stopped\n", ( int )pxTimerIntHdl->ubIdx );
            }
#endif
            pxTimerIntHdl->bIsRunning = FALSE;
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

STATIC void
eMBPTimerCB( union sigval xSigVal )
{
    xTimerInternalHandle *pxTimerIntHdl = xSigVal.sival_ptr;

    if( ( NULL != pxTimerIntHdl ) && ( NULL != pxTimerIntHdl->pbMBPTimerExpiredFN ) && ( pxTimerIntHdl->bIsRunning ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TIMER ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TIMER, "[%d] expired\n", ( int )pxTimerIntHdl->ubIdx );
        }
#endif
        pxTimerIntHdl->pbMBPTimerExpiredFN( pxTimerIntHdl->xMBPHdl );
    }
}
