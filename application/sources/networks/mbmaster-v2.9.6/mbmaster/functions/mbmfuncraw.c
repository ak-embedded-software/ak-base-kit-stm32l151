/* 
 * MODBUS Library: Example for custom functions
 * Copyright (c) 2008-2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmfuncraw.c,v 1.9 2011-06-18 19:17:13 embedded-solutions.cwalter Exp $
 */


/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>
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

/*lint --e{788} ~ suppress messages: enum constant not used within defaulted switch */
/*lint --e{835} ~ suppress messages: A zero has been given as right argument to operator '+' */

/* ----------------------- Defines ------------------------------------------*/
#if MBM_FUNC_READWRITE_RAWPDU_ENABLED == 1

#define MBM_PAYLOAD_MAX                 ( 252 ) /* PDU MAX SIZE - Function Code */
#define MBM_FUNCCODE_MIN                ( 1 )
#define MBM_FUNCCODE_MAX                ( 127 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation (Polling functions)----------*/
void
vMBMReadWriteRAWPDUPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress, UCHAR ucFunctionCode,
                           const UBYTE arubPayloadIn[], UBYTE ubPayloadInLength,
                           UBYTE arubPayloadOut[], UBYTE pubPayloadOutLengthMax, UBYTE * pubPayloadOutLength,
                           eMBMQueryState * peState,
                           eMBErrorCode * peStatus )
{
    xMBMInternalHandle *pxIntHdl = ( xMBMInternalHandle * ) xHdl;

    if( bMBMIsHdlValid( pxIntHdl ) && ( peState != NULL ) && ( peStatus != NULL ) )
    {
        switch ( *peState )
        {
            /* In this state we prepare the frame and send it. */
        case MBM_STATE_NONE:
            if( ( ( ( NULL != arubPayloadIn ) && ( ubPayloadInLength <= MBM_PAYLOAD_MAX ) ) ||
                  ( ( NULL == arubPayloadIn ) && ( 0 == ubPayloadInLength ) ) ) &&
                ( ucFunctionCode >= MBM_FUNCCODE_MIN ) && ( ucFunctionCode <= MBM_FUNCCODE_MAX ) &&
                ( ( arubPayloadOut != NULL ) || pubPayloadOutLengthMax == 0 ) )
            {
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ucFunctionCode;
				pxIntHdl->usFrameMBPDULength++;
                if( ubPayloadInLength > 0 )
                {
                    MBP_ASSERT( NULL != arubPayloadIn );
                    /* Now copy the RAW payload into the MODBUS PDU and adjust the length. */
                    memcpy( &( pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] ), arubPayloadIn,
                            ubPayloadInLength );
                }
                pxIntHdl->usFrameMBPDULength += ( USHORT ) ubPayloadInLength;
                /* Frame is assembled. Now send it. */
                *peState = MBM_STATE_SEND;
            }
            else
            {
                *peStatus = MB_EINVAL;
                *peState = MBM_STATE_DONE;
            }
            break;

            /* These states are handled by the common state machine. */
        case MBM_STATE_SEND:
        case MBM_STATE_WAITING:
            vMBMMasterTransactionPolled( pxIntHdl, ucSlaveAddress, peState, peStatus );
            break;

            /* We need to disassemble the response here. */
        case MBM_STATE_DISASSEMBLE:
            *peState = MBM_STATE_DONE;
            if( ucFunctionCode == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] )
            {
                MBP_ASSERT( pxIntHdl->usFrameMBPDULength >= MB_PDU_SIZE_MIN );
                if( ( pxIntHdl->usFrameMBPDULength - 1 ) <= pubPayloadOutLengthMax )
                {
                    memcpy( arubPayloadOut, &( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF + 1] ),
                            pxIntHdl->usFrameMBPDULength - 1 );
                    *pubPayloadOutLength = ( UBYTE ) ( pxIntHdl->usFrameMBPDULength - 1 );
                }
                *peStatus = MB_ENOERR;
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF], ucFunctionCode ) )
            {
                *peStatus = eMBExceptionToErrorcode( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_EX_CODE_OFF] );
            }
            else
            {
                *peStatus = MB_EIO;
            }
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
eMBMReadWriteRAWPDU( xMBMHandle xHdl, UCHAR ucSlaveAddress, UCHAR ucFunctionCode,
                     const UBYTE arubPayloadIn[], UBYTE ubPayloadInLength,
                     UBYTE arubPayloadOut[], UBYTE pubPayloadOutLengthMax, UBYTE * pubPayloadOutLength )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMReadWriteRAWPDUPolled( xHdl, ucSlaveAddress, ucFunctionCode, arubPayloadIn, ubPayloadInLength,
                                   arubPayloadOut, pubPayloadOutLengthMax, pubPayloadOutLength, &eState, &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}

#endif
