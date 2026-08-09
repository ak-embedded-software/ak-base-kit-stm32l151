/* 
 * MODBUS Library: LINUX/CYGWIN port
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbporteventposix.c,v 1.5 2014-08-23 09:36:06 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <mqueue.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
#define MESSAGE_PRIORITY        ( 1 )
#define MESSAGE_QUEUE_NAME_FMT  "/MBM_%d_%d"

#define HDL_RESET( x ) do { \
    ( x )->xMQHdl = ( mqd_t ) - 1; \
    ( x )->ubIdx = IDX_INVALID; \
} while( 0 )

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    mqd_t           xMQHdl;
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
    mqd_t           xMQHdlTemp;
    CHAR            arucMQName[32];
    struct mq_attr  xMQAttr;

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
                xMQAttr.mq_flags = 0;
                xMQAttr.mq_maxmsg = 1;
                xMQAttr.mq_msgsize = sizeof( xMBPEventType );
                ( void )snprintf( arucMQName, MB_UTILS_NARRSIZE( arucMQName ), MESSAGE_QUEUE_NAME_FMT, ( int )getpid(  ), i );
                xMQHdlTemp = mq_open( arucMQName, O_RDWR | O_CREAT | O_EXCL, 0700, &xMQAttr );
                if( ( mqd_t ) - 1 != xMQHdlTemp )
                {
                    arxEventHdls[i].ubIdx = i;
                    arxEventHdls[i].xMQHdl = xMQHdlTemp;
                    *pxEventHdl = &arxEventHdls[i];
                    eStatus = MB_ENOERR;
                }
                else
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_EVENT ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't create event queue '%s': %s", arucMQName, strerror( errno ) );
                    }
#endif
                    eStatus = MB_EPORTERR;
                }
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
    mqd_t           xMQHdlTemp;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        /* Handle the very unlikely case that we are in the callback and someone
         * shutsdown the stack.
         */
        xMQHdlTemp = pxEventHdl->xMQHdl;
        MBP_EXIT_CRITICAL_SECTION(  );

        if( 0 != mq_send( xMQHdlTemp, ( char * )&xEvent, sizeof( xMBPEventType ), MESSAGE_PRIORITY ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_EVENT ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't post event in queue %d: %s", ( int )xMQHdlTemp, strerror( errno ) );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        else
        {
            eStatus = MB_ENOERR;
        }
    }
    else
    {
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return eStatus;
}

BOOL
bMBPEventGet( const xMBPEventHandle xEventHdl, xMBPEventType * pxEvent )
{
    BOOL            bEventInQueue = FALSE;
    xEventInternalHandle *pxEventHdl = xEventHdl;
    mqd_t           xMQHdlTemp;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        /* Handle the very unlikely case that we are in the callback and someone
         * shutsdown the stack.
         */
        xMQHdlTemp = pxEventHdl->xMQHdl;
        MBP_EXIT_CRITICAL_SECTION(  );
        if( sizeof( xMBPEventType ) == mq_receive( xMQHdlTemp, ( char * )pxEvent, sizeof( xMBPEventType ), NULL ) )
        {
            bEventInQueue = TRUE;
        }
        else
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_EVENT ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_EVENT, "Can't receive event from queue %d: %s", ( int )xMQHdlTemp, strerror( errno ) );
            }
#endif
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
    CHAR            arucMQName[32];
    xEventInternalHandle *pxEventIntHdl = xEventHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventIntHdl, arxEventHdls ) )
    {
        if( ( mqd_t ) - 1 != pxEventIntHdl->xMQHdl )
        {
            ( void )snprintf( arucMQName, MB_UTILS_NARRSIZE( arucMQName ), MESSAGE_QUEUE_NAME_FMT, ( int )getpid(  ), pxEventIntHdl->ubIdx );
            ( void )mq_close( pxEventIntHdl->xMQHdl );
            ( void )mq_unlink( arucMQName );
        }
        HDL_RESET( pxEventIntHdl );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}
