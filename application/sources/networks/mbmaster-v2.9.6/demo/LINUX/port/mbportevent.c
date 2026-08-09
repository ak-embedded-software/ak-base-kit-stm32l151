/* 
 * MODBUS Library: LINUX/CYGWIN port
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportevent.c,v 1.6 2014-08-23 09:36:06 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define MAX_EVENT_HDLS          ( 8 )
#define IDX_INVALID             ( 255 )
#define EV_NONE                 ( 0 )

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->xEvType = EV_NONE; \
} while( 0 )

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    xMBPEventType   xEvType;
} xEventInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;
STATIC xEventInternalHandle arxEventHdls[MAX_EVENT_HDLS];
STATIC sem_t    post_sem[MAX_EVENT_HDLS], get_sem[MAX_EVENT_HDLS];

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
            for( i = 0; i < MAX_EVENT_HDLS; i++ )
            {
                bIsInitialized = TRUE;
                if( sem_init( &post_sem[i], 0, 0 ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't initialize post semaphore %d: %s.\n", i, strerror( errno ) );
                    }
#endif
                    bIsInitialized = FALSE;
                    break;
                }
                else if( sem_init( &get_sem[i], 0, 1 ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't initialize get semaphore %d: %s.\n", i, strerror( errno ) );
                    }
#endif
                    bIsInitialized = FALSE;
                    break;
                }
            }
        }
        if( bIsInitialized )
        {
            for( i = 0; i < MAX_EVENT_HDLS; i++ )
            {
                if( IDX_INVALID == arxEventHdls[i].ubIdx )
                {
                    arxEventHdls[i].ubIdx = i;
                    *pxEventHdl = &arxEventHdls[i];
                    eStatus = MB_ENOERR;
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

    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        if( sem_wait( &get_sem[pxEventHdl->ubIdx] ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't lock semaphore %d: %s.\n", strerror( errno ) );
            }
#endif
        }
        else
        {
            if( EV_NONE != pxEventHdl->xEvType )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't post event in queue, filled.\n" );
                }
#endif
                sem_post( &get_sem[pxEventHdl->ubIdx] );
                eStatus = MB_EPORTERR;
            }
            else
            {
                pxEventHdl->xEvType = xEvent;

                if( sem_post( &post_sem[pxEventHdl->ubIdx] ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't unlock semaphore: %s.\n", strerror( errno ) );
                    }
#endif
                }
                else
                {
                    eStatus = MB_ENOERR;
                }
            }
        }
    }

    return eStatus;
}

BOOL
bMBPEventGet( const xMBPEventHandle xEventHdl, xMBPEventType * pxEvent )
{
    BOOL            bEventInQueue = FALSE;
    xEventInternalHandle *pxEventHdl = xEventHdl;

    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        if( sem_wait( &post_sem[pxEventHdl->ubIdx] ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't lock semaphore %d: %s.\n", strerror( errno ) );
            }
#endif
        }

        if( EV_NONE != pxEventHdl->xEvType )
        {
            *pxEvent = pxEventHdl->xEvType;
            pxEventHdl->xEvType = EV_NONE;
            bEventInQueue = TRUE;
        }

        if( sem_post( &get_sem[pxEventHdl->ubIdx] ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't unlock semaphore: %s.\n", strerror( errno ) );
            }
#endif
        }
    }

    return bEventInQueue;
}

void
vMBPEventDelete( xMBPEventHandle xEventHdl )
{
    BYTE            x;
    xEventInternalHandle *pxEventIntHdl = xEventHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventIntHdl, arxEventHdls ) )
    {
        HDL_RESET( pxEventIntHdl );

        for( x = 0; x < MAX_EVENT_HDLS; x++ )
        {
            if( sem_destroy( &post_sem[x] ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't destroy post semaphore %d: %s.\n", x, strerror( errno ) );
                }
#endif
            }
            if( sem_destroy( &get_sem[x] ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_EVENT ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't destroy get semaphore %d: %s.\n", x, strerror( errno ) );
                }
#endif
            }
        }
    }

    MBP_EXIT_CRITICAL_SECTION(  );
}
