/* 
 * MODBUS Library: Luminary Cortex M3, FreeRTOS and lwIP Example
 * Copyright (c)  2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportevent.c,v 1.4 2010-06-13 17:04:48 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define MAX_EVENT_HDLS          ( 4 )
#define IDX_INVALID             ( 255 )
#define EV_NONE                 ( 0 )

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->xQueueHdl = 0; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    xQueueHandle    xQueueHdl;
} xEventInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;

STATIC xEventInternalHandle arxEventHdls[MAX_EVENT_HDLS];

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPEventCreate( xMBPEventHandle * pxEventHdl )
{
    /* Thread: 
     *  - Multiple MODBUS core threads from different handles
     * Protection: Full 
     */
    eMBErrorCode    eStatus = MB_EINVAL;
    xQueueHandle    xQueueHdl;
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
                xQueueHdl = xQueueCreate( 1, sizeof( xMBPEventType ) );
                if( 0 != xQueueHdl )
                {
                    arxEventHdls[i].ubIdx = i;
                    arxEventHdls[i].xQueueHdl = xQueueHdl;
                    *pxEventHdl = &arxEventHdls[i];
                    eStatus = MB_ENOERR;
                }
                else
                {
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
    /* Thread: 
     *  - Single MODBUS core thread
     *  - Possibly any porting layer thread
     *
     * Protection: Full
     */
    eMBErrorCode    eStatus = MB_EINVAL;
    xEventInternalHandle *pxEventHdl = xEventHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        if( pdTRUE != xQueueSend( pxEventHdl->xQueueHdl, &xEvent, 0 ) )
        {
            eStatus = MB_EPORTERR;
        }
        else
        {
            eStatus = MB_ENOERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}


BOOL
bMBPEventGet( const xMBPEventHandle xEventHdl, xMBPEventType * pxEvent )
{
    /* Thread: 
     *  - Single MODBUS core thread 
     * 
     * Protection: None
     */
    BOOL            bEventInQueue = FALSE;
    xEventInternalHandle *pxEventHdl = xEventHdl;

    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        if( pdTRUE == xQueueReceive( pxEventHdl->xQueueHdl, pxEvent, 50 / portTICK_RATE_MS ) )
        {
            bEventInQueue = TRUE;
        }
    }

    return bEventInQueue;
}

void
vMBPEventDelete( xMBPEventHandle xEventHdl )
{
    /* Thread: 
     *   - MODBUS core thread 
     *
     * Protection: Full (Protection handle reset for parallel new create)
     */
    xEventInternalHandle *pxEventHdl = xEventHdl;

    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        if( NULL != pxEventHdl->xQueueHdl )
        {
            vQueueDelete( pxEventHdl->xQueueHdl );
        }
        MBP_ENTER_CRITICAL_SECTION(  );
        HDL_RESET( pxEventHdl );
        MBP_EXIT_CRITICAL_SECTION(  );
    }
}
