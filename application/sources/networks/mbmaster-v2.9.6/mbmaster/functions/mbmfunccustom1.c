/* 
 * MODBUS Library: Example for custom functions
 * Copyright (c) 2008-2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmfunccustom1.c,v 1.4 2011-01-02 16:15:32 embedded-solutions.cwalter Exp $
 */


/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <stdio.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"
#include "internal/mbmiframe.h"
#include "internal/mbmi.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBM_FUNC_CUSTOM                             ( 0x32 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation (Polling functions)----------*/
void
vMBMCustom1Polled( xMBMHandle xHdl, UCHAR ucSlaveAddress, UBYTE ubLength, UBYTE * pubPayload, UBYTE * pubState,
                   /*@out@ */ eMBMQueryState * peState,
                   /*@out@ */ eMBErrorCode * peStatus )
{
    xMBMInternalHandle *pxIntHdl = ( xMBMInternalHandle * ) xHdl;
    UBYTE           ubIdx;

    if( bMBMIsHdlValid( pxIntHdl ) && ( peState != NULL ) && ( peStatus != NULL ) )
    {
        switch ( *peState )
        {
            /* In this state we prepare the frame and send it. */
        case MBM_STATE_NONE:
            /* ----------------------- TO BE CUSTOMIZED BY USER ---------------------------------- */
            if( ( NULL != pubPayload ) && ( ubLength > 0 ) && ( ubLength <= 100 ) )
            {
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_CUSTOM;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ubLength;
				pxIntHdl->usFrameMBPDULength++;
                for( ubIdx = 0; ubIdx < ubLength; ubIdx++ )
                {
                    pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = *pubPayload++;
					pxIntHdl->usFrameMBPDULength++;
                }
                /* Frame is assembled. Now send it. */
                *peState = MBM_STATE_SEND;
            }
            else
            {
                *peStatus = MB_EINVAL;
                *peState = MBM_STATE_DONE;
            }
            break;
            /* ----------------------- TO BE CUSTOMIZED BY USER ---------------------------------- */

            /* These states are handled by the common state machine. */
        case MBM_STATE_SEND:
        case MBM_STATE_WAITING:
            vMBMMasterTransactionPolled( pxIntHdl, ucSlaveAddress, peState, peStatus );
            break;

            /* We need to disassemble the response here. */
        case MBM_STATE_DISASSEMBLE:
            *peState = MBM_STATE_DONE;
            /* ----------------------- TO BE CUSTOMIZED BY USER ---------------------------------- */
            if( MBM_FUNC_CUSTOM == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] )
            {
                *pubState = pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF];
                *peStatus = MB_ENOERR;
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF], MBM_FUNC_CUSTOM ) )
            {
                *peStatus = eMBExceptionToErrorcode( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_EX_CODE_OFF] );
            }
            else
            {
                *peStatus = MB_EIO;
            }
            /* ----------------------- TO BE CUSTOMIZED BY USER ---------------------------------- */
            break;

        case MBM_STATE_ERROR:
            /* No cleanup required. */
            *peState = MBM_STATE_DONE;
            break;

        default:
            *peState = MBM_STATE_DONE;
            *peStatus = MB_EILLSTATE;
        }
    }
    else
    {
        if( NULL != peState )
        {
            *peState = MBM_STATE_DONE;
        }
        if( NULL != peStatus )
        {
            *peStatus = MB_EINVAL;
        }
    }
}

/* ----------------------- Start implementation (Blocking functions) --------*/
eMBErrorCode
eMBMCustom1Polled( xMBMHandle xHdl, UCHAR ucSlaveAddress, UBYTE ubLength, UBYTE * pubPayload, UBYTE * pubState )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMCustom1Polled( xHdl, ucSlaveAddress, ubLength, pubPayload, pubState, &eState, &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
