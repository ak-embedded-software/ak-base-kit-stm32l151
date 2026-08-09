/* 
 * MODBUS Library: Win32 port
 * Copyright (c) 2008-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportevent.c,v 1.7 2011-12-04 18:53:49 embedded-solutions.cwalter Exp $
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
#define MAX_EVENT_HDLS          ( 8 )
#define IDX_INVALID             ( 255 )
#define EV_NONE                 ( 0 )

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->xEvHdl = NULL; \
    ( x )->xType = EV_NONE; \
    ( x )->bIsEventInQueue = FALSE; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    HANDLE          xEvHdl;
    xMBPEventType   xType;
    BOOL            bIsEventInQueue;
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
            for( i = 0; i < MAX_EVENT_HDLS; i++ )
            {
                if( IDX_INVALID == arxEventHdls[i].ubIdx )
                {
                    arxEventHdls[i].xEvHdl = CreateEvent( NULL, FALSE, FALSE, NULL );
                    if( NULL != arxEventHdls[i].xEvHdl )
                    {
                        arxEventHdls[i].ubIdx = i;
                        arxEventHdls[i].xType = EV_NONE;
                        *pxEventHdl = &arxEventHdls[i];
                        eStatus = MB_ENOERR;
                    }
                    else
                    {
                        eStatus = MB_ENORES;
                    }
                    break;
                }
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
        if( !SetEvent( pxEventHdl->xEvHdl ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "[EV=%hu] Can not set event %d!.\n",
                             ( USHORT ) pxEventHdl->ubIdx, ( int )xEvent );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        else
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_EVENT ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_EVENT, "[EV=%hu] Set event %d.\n",
                             ( USHORT ) pxEventHdl->ubIdx, ( int )xEvent );
            }
#endif
            pxEventHdl->xType = xEvent;
            pxEventHdl->bIsEventInQueue = TRUE;
            eStatus = MB_ENOERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

BOOL
bMBPEventGet( const xMBPEventHandle xEventHdl, xMBPEventType * pxEvent )
{
    BOOL            bEventInQueue = FALSE;
    xEventInternalHandle *pxEventHdl = xEventHdl;
    HANDLE          xWinEvHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        xWinEvHdl = pxEventHdl->xEvHdl;
        MBP_EXIT_CRITICAL_SECTION(  );
        switch ( WaitForSingleObject( xWinEvHdl, 500 ) )
        {
        case WAIT_OBJECT_0:
            MBP_ENTER_CRITICAL_SECTION(  );
            if( pxEventHdl->bIsEventInQueue )
            {
                bEventInQueue = TRUE;
                *pxEvent = pxEventHdl->xType;
                pxEventHdl->bIsEventInQueue = FALSE;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_EVENT ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_EVENT, "[EV=%hu] Received event %d.\n",
                                 ( USHORT ) pxEventHdl->ubIdx, ( int )*pxEvent );
                }
#endif
            }

            MBP_EXIT_CRITICAL_SECTION(  );
        default:
            break;
        }
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
        if( NULL != pxEventIntHdl->xEvHdl )
        {
            ( void )CloseHandle( pxEventIntHdl->xEvHdl );
        }
        HDL_RESET( pxEventIntHdl );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}
