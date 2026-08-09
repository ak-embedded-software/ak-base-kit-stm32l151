/* 
 * MODBUS Library: CMX port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportevent.c,v 1.3 2008-08-27 18:02:15 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include "cxfuncs.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define MAX_EVENT_HDLS          ( 1 )
#define IDX_INVALID             ( 255 )
#define EV_NONE                 ( 0 )
#define EV_WAIT_TICKS           ( 100 )

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->xType = EV_NONE; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    xMBPEventType   xType;
} xEventInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;
STATIC xEventInternalHandle arxEventHdls[MAX_EVENT_HDLS];

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPEventCreate( xMBPEventHandle * pxEventHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    UBYTE           i;

    if( NULL != pxEventHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !bIsInitialized )
        {
            for( i = 0; i < MAX_EVENT_HDLS; i++ )
            {
                HDL_RESET( &arxEventHdls[i] );
            }
            bIsInitialized = TRUE;
        }
        for( i = 0; i < MAX_EVENT_HDLS; i++ )
        {
            if( IDX_INVALID == arxEventHdls[i].ubIdx )
            {
                arxEventHdls[i].ubIdx = i;
                arxEventHdls[i].xType = EV_NONE;
                *pxEventHdl = &arxEventHdls[i];
                eStatus = MB_ENOERR;
                break;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return eStatus;
}

eMBErrorCode
eMBPEventPost( const xMBPEventHandle xEventHdl, xMBPEventType xEvent )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xEventInternalHandle *pxEventHdl = xEventHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        K_Event_Signal( 0, g_bMODBUSTaskSlot, ( word16 ) ( MB_QUEUE_EVENT << pxEventHdl->ubIdx ) );
        pxEventHdl->xType = xEvent;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

BOOL
bMBPEventGet( const xMBPEventHandle xEventHdl, xMBPEventType * pxEvent )
{
    BOOL            bEventInQueue = FALSE;
    UBYTE           ubIdx;

#if !MB_USE_SINGLETASK
    word16          uiEvents;
#endif
    xEventInternalHandle *pxEventHdl = xEventHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        ubIdx = pxEventHdl->ubIdx;
#if MB_USE_SINGLETASK
        if( EV_NONE != pxEventHdl->xType )
        {
            bEventInQueue = TRUE;
            *pxEvent = pxEventHdl->xType;
            pxEventHdl->xType = EV_NONE;
        }
        MBP_EXIT_CRITICAL_SECTION(  );
#else
        MBP_EXIT_CRITICAL_SECTION(  );
        uiEvents = K_Event_Wait( ( word16 ) ( MB_QUEUE_EVENT << ubIdx ), EV_WAIT_TICKS, 2 );
        if( uiEvents )
        {
            MBP_ENTER_CRITICAL_SECTION(  );
            if( EV_NONE != pxEventHdl->xType )
            {
                bEventInQueue = TRUE;
                *pxEvent = pxEventHdl->xType;
                pxEventHdl->xType = EV_NONE;
            }
            MBP_EXIT_CRITICAL_SECTION(  );
        }
#endif
    }
    else
    {
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return bEventInQueue;
}

void
vMBPEventDelete( xMBPEventHandle xEventHdl )
{
    xEventInternalHandle *pxEventIntHdl = xEventHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventIntHdl, arxEventHdls ) )
    {
        ( void )K_Event_Reset( g_bMODBUSTaskSlot, ( word16 ) ( MB_QUEUE_EVENT << pxEventIntHdl->ubIdx ) );
        HDL_RESET( pxEventIntHdl );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}
