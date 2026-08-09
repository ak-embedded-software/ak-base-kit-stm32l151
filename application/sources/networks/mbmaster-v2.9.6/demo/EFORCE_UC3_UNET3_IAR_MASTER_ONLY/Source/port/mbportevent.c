/*
 * MODBUS Libary: uC3/uNET port
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportevent.c,v 1.1 2011-06-13 19:17:54 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include "kernel.h"
#include "kernel_id.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define IDX_INVALID             ( 255 )
#define EV_NONE                 ( 0 )
/* The maximum value for an event as we map the events to a bitmask to use
 * the uC3 event handles.
 */
#define EV_MAX_VALUE			( 31 )

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
} while( 0 )

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
} xEventInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;

STATIC const ID arxOSEventHdls[] = {
    ID_MODBUS_FLG1,
    ID_MODBUS_FLG2
};

STATIC xEventInternalHandle arxEventHdls[MB_UTILS_NARRSIZE( arxOSEventHdls )];

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
            for( i = 0; i < MB_UTILS_NARRSIZE( arxOSEventHdls ); i++ )
            {
                HDL_RESET( &arxEventHdls[i] );
            }
            bIsInitialized = TRUE;
        }
        for( i = 0; i < MB_UTILS_NARRSIZE( arxOSEventHdls ); i++ )
        {
            if( IDX_INVALID == arxEventHdls[i].ubIdx )
            {
                arxEventHdls[i].ubIdx = i;
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
    ER              ercd;
    FLGPTN          ptn;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        MBP_ASSERT( xEvent <= EV_MAX_VALUE );
        ptn = 1 << xEvent;
        if( E_OK == ( ercd = set_flg( arxOSEventHdls[pxEventHdl->ubIdx], ptn ) ) )
        {
            eStatus = MB_ENOERR;
        }
        else
        {
            MBP_ASSERT( E_OK == ercd );
            eStatus = MB_EPORTERR;
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
    ER              ercd;
    FLGPTN          waitptn;
    UINT            cnt;

    if( MB_IS_VALID_HDL( pxEventHdl, arxEventHdls ) )
    {
        ercd = twai_flg( arxOSEventHdls[pxEventHdl->ubIdx], 0xFFFFFFFFU, TWF_ORW, &waitptn, 100 );
        switch ( ercd )
        {
        case E_OK:
            ercd = clr_flg( arxOSEventHdls[pxEventHdl->ubIdx], ~waitptn );
            MBP_ASSERT( E_OK == ercd );
            for( cnt = 0; cnt <= EV_MAX_VALUE; cnt++ )
            {
                if( waitptn & ( 1U << cnt ) )
                {
                    *pxEvent = cnt;
                    break;
                }
            }
            bEventInQueue = TRUE;
            break;
        case E_TMOUT:
            *pxEvent = EV_NONE;
            break;
        default:
            *pxEvent = EV_NONE;
            MBP_ASSERT( E_OK == ercd );
            break;
        }
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
        HDL_RESET( pxEventIntHdl );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}
