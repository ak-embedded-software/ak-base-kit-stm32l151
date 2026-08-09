/* 
 * MODBUS Library: CMX port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbporttimer.c,v 1.2 2008-08-26 18:36:32 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "cxfuncs.h"
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define IDX_INVALID                 ( 255 )
#define EV_NONE                     ( 0 )

#define TIMER_TIMEOUT_INVALID	    ( 65535U )
#define TIMER_MS2TIMER_TICKS( x)    ( x)

#define RESET_HDL( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->usNTimeOutMS = 0; \
    ( x )->xMBMHdl = MB_HDL_INVALID; \
    ( x )->pbMBPTimerExpiredFN = NULL; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    USHORT          usNTimeOutMS;
    ULONG           dwTimeOut;
    BOOL            bIsRunning;
    xMBHandle       xMBMHdl;
    pbMBPTimerExpiredCB pbMBPTimerExpiredFN;
} xTimerInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xTimerInternalHandle arxTimerHdls[MB_TIMERS_COUNT];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBPTimerInit( xMBPTimerHandle * xTimerHdl, USHORT usTimeOut1ms,
               pbMBPTimerExpiredCB pbMBPTimerExpiredFN, xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EPORTERR;
    UBYTE           ubIdx;


    MBP_ENTER_CRITICAL_SECTION(  );
    if( ( NULL != xTimerHdl ) && ( NULL != pbMBPTimerExpiredFN ) && ( MB_HDL_INVALID != xHdl ) )
    {
        if( !bIsInitalized )
        {

            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
            {
                RESET_HDL( &arxTimerHdls[ubIdx] );
            }

            bIsInitalized = TRUE;

        }
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
        {
            if( IDX_INVALID == arxTimerHdls[ubIdx].ubIdx )
            {
                break;
            }
        }
        if( MB_TIMERS_COUNT != ubIdx )
        {
            if( K_OK != K_Timer_Create( ( unsigned char )( ubIdx + MB_TIMERS_IDX_FIRST ), 0, g_bSerialTaskSlot,
                                        ( unsigned short )( 1 << ( MB_TIMERS_EVENT_IDX_FIRST + ubIdx ) ) ) )
            {
                eStatus = MB_ENORES;
            }
            else
            {
                arxTimerHdls[ubIdx].ubIdx = ubIdx;
                arxTimerHdls[ubIdx].usNTimeOutMS = usTimeOut1ms;
                arxTimerHdls[ubIdx].dwTimeOut = 0;
                arxTimerHdls[ubIdx].bIsRunning = FALSE;
                arxTimerHdls[ubIdx].xMBMHdl = xHdl;
                arxTimerHdls[ubIdx].pbMBPTimerExpiredFN = pbMBPTimerExpiredFN;
                *xTimerHdl = &arxTimerHdls[ubIdx];
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
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        /* There is no way to delete a CMX timer. */
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
        if( K_OK != K_Timer_Start( ( unsigned char )( MB_TIMERS_IDX_FIRST + pxTimerIntHdl->ubIdx ),
                                   TIMER_MS2TIMER_TICKS( pxTimerIntHdl->usNTimeOutMS ), 0 ) )
        {
            eStatus = MB_EPORTERR;
            /* Only for testing - Remove later. */
            MBP_ASSERT( 0 );
        }
        else
        {
            eStatus = MB_ENOERR;
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
        if( K_OK != K_Timer_Stop( ( unsigned char )( MB_TIMERS_IDX_FIRST + pxTimerIntHdl->ubIdx ) ) )
        {
            eStatus = MB_EPORTERR;
            /* Only for testing - Remove later. */
            MBP_ASSERT( 0 );
        }
        else
        {
            eStatus = MB_ENOERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

void
prvvTimerHandler( unsigned char ucTmrIdx )
{
    MBP_ENTER_CRITICAL_SECTION(  );
    if( IDX_INVALID != arxTimerHdls[ucTmrIdx].ubIdx )
    {
        ( void )arxTimerHdls[ucTmrIdx].pbMBPTimerExpiredFN( arxTimerHdls[ucTmrIdx].xMBMHdl );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}
