/* 
 * MODBUS Library: CUNIT framework port
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbporttimer.c,v 1.2 2007-08-19 12:26:11 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define MAX_TIMER_HDLS          ( 8 )
#define IDX_INVALID             ( 255 )
#define EV_NONE                 ( 0 )

#define RESET_HDL( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->bFailOnStart = FALSE; \
    ( x )->bFailOnStop = FALSE; \
    ( x )->bFailOnSetTimeout = FALSE; \
    ( x )->xMBPHdl = MB_HDL_INVALID; \
    ( x )->pbMBPTimerExpiredFN = NULL; \
    memset( &( ( x )->xTimerPosixHdl ), 0, sizeof( timer_t ) ); \
    memset( &( ( x )->xTimerSpec ), 0, sizeof( struct itimerspec ) ); \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    BOOL            bFailOnStart;
    BOOL            bFailOnStop;
    BOOL            bFailOnSetTimeout;
    timer_t         xTimerPosixHdl;
    struct itimerspec xTimerSpec;
    xMBHandle       xMBPHdl;
    pbMBPTimerExpiredCB pbMBPTimerExpiredFN;
} xTimerInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xTimerInternalHandle arxTimerHdls[MAX_TIMER_HDLS];
STATIC BOOL     bIsInitalized = FALSE;
STATIC BOOL     bFailOnInit = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void
eMBPTimerCB( union sigval xSigVal )
{
    xTimerInternalHandle *pxTimerIntHdl = xSigVal.sival_ptr;

    if( ( NULL != pxTimerIntHdl ) && ( NULL != pxTimerIntHdl->pbMBPTimerExpiredFN ) )
    {
        pxTimerIntHdl->pbMBPTimerExpiredFN( pxTimerIntHdl->xMBPHdl );
    }
}

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPTimerInit( xMBPTimerHandle * xTimerHdl, USHORT usTimeOut1ms,
               pbMBPTimerExpiredCB pbMBPTimerExpiredFN, xMBHandle xHdl )
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
        if( !bFailOnInit && ( MAX_TIMER_HDLS != i ) )
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
vMBPTimerFailOnInit( BOOL bFail )
{
    bFailOnInit = bFail;
}


eMBErrorCode
eMBPTimerSetFailOnStart( xMBPTimerHandle xTimerHdl, BOOL bFail )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxIntHdl, arxTimerHdls ) )
    {
        pxIntHdl->bFailOnStart = bFail;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPTimerSetFailOnStop( xMBPTimerHandle xTimerHdl, BOOL bFail )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxIntHdl, arxTimerHdls ) )
    {
        pxIntHdl->bFailOnStop = bFail;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPTimerSetFailOnSetTimeout( xMBPTimerHandle xTimerHdl, BOOL bFail )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxIntHdl, arxTimerHdls ) )
    {
        pxIntHdl->bFailOnSetTimeout = bFail;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

void
vMBPTimerClose( xMBPTimerHandle xTimerHdl )
{
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    if( ( MBP_TIMERHDL_INVALID != xTimerHdl ) &&
        ( pxTimerIntHdl->ubIdx < MAX_TIMER_HDLS ) && ( pxTimerIntHdl == &arxTimerHdls[pxTimerIntHdl->ubIdx] ) )
    {
        ( void )timer_delete( pxTimerIntHdl->xTimerPosixHdl );
        RESET_HDL( pxTimerIntHdl );
    }
}

eMBErrorCode
eMBPTimerSetTimeout( xMBPTimerHandle xTimerHdl, USHORT usTimeOut1ms )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        if( !pxTimerIntHdl->bFailOnSetTimeout )
        {
            pxTimerIntHdl->xTimerSpec.it_interval.tv_nsec = 0;
            pxTimerIntHdl->xTimerSpec.it_interval.tv_sec = 0;
            pxTimerIntHdl->xTimerSpec.it_value.tv_sec = usTimeOut1ms / 1000;
            pxTimerIntHdl->xTimerSpec.it_value.tv_nsec = 1000000UL * ( usTimeOut1ms % 1000 );
            eStatus = MB_ENOERR;
        }
        else
        {
            eStatus = MB_EPORTERR;
        }
    }
    return eStatus;
}

eMBErrorCode
eMBPTimerStart( xMBPTimerHandle xTimerHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        if( !pxTimerIntHdl->bFailOnStart )
        {
            if( ( 0 == timer_settime( pxTimerIntHdl->xTimerPosixHdl, 0, &( pxTimerIntHdl->xTimerSpec ), NULL ) ) )
            {
                eStatus = MB_ENOERR;
            }
        }
        else
        {
            eStatus = MB_EPORTERR;
        }
    }
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

    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        if( !pxTimerIntHdl->bFailOnStop )
        {
            if( ( 0 == timer_settime( pxTimerIntHdl->xTimerPosixHdl, 0, &( xTimerSpec ), NULL ) ) )
            {
                eStatus = MB_ENOERR;
            }
        }
        else
        {
            eStatus = MB_EPORTERR;
        }
    }
    return eStatus;
}
