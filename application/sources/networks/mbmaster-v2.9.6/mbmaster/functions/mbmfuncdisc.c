/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007-2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmfuncdisc.c,v 1.9 2011-12-04 21:10:57 embedded-solutions.cwalter Exp $
 */


/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#if defined( __18CXX )
#include <stdio.h>
#endif

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

/* ----------------------- Defines ------------------------------------------*/
#define NBYTES_FOR_NINPUTS( usNInputs ) \
    ( UBYTE )( ( ( usNInputs ) + 7U ) / 8U )

#define MBM_FUNC_RD_DISC_INPUT                      ( 0x02 )
#define MBM_PDU_FUNC_RD_DISCCNT_MAX                 ( 0x07D0 )
#define MBM_PDU_FUNC_RD_DISCCNT_RESP_BYTECNT_OFF    ( MB_PDU_DATA_OFF )
#define MBM_PDU_FUNC_RD_DISCCNT_RESP_INPUTS_OFF     ( MBM_PDU_FUNC_RD_DISCCNT_RESP_BYTECNT_OFF + 1 )

#define MBM_PDU_FUNC_RD_DISC_RESPSIZE( usNInputs )  \
    ( UBYTE )( NBYTES_FOR_NINPUTS( usNInputs ) + 1 + 1 )
/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation (Polling functions)----------*/
#if MBM_FUNC_READ_DISC_INPUTS_ENABLED == 1
void
vMBMReadDiscreteInputsPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usInputStartAddress, USHORT usNInputs,
                              UBYTE arubBufferOut[], eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    UBYTE           ubNByteCnt;
    UBYTE           ubNBytesExpected;
    xMBMInternalHandle *pxIntHdl = ( xMBMInternalHandle * ) xHdl;
    UBYTE          *pubCurPtr;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) && ( peState != NULL ) && ( peStatus != NULL ) )
#else
    if( TRUE )
#endif
    {
        switch ( *peState )
        {
            /* In this state we prepare the frame and send it. */
        case MBM_STATE_NONE:
            if( ( NULL != arubBufferOut ) && ( usNInputs > 0 ) && ( usNInputs <= MBM_PDU_FUNC_RD_DISCCNT_MAX ) )
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE,
                                 "[IDX=" MBP_FORMAT_USHORT "] Read discrete inputs (start=" MBP_FORMAT_USHORT
                                 ", length=" MBP_FORMAT_USHORT ").\n", ( USHORT ) pxIntHdl->ubIdx, usInputStartAddress,
                                 ( USHORT ) usNInputs );
                }
#endif
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_RD_DISC_INPUT;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usInputStartAddress >> 8U );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                    ( UBYTE ) ( usInputStartAddress & 0x00FFU );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usNInputs >> 8U );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usNInputs & 0x00FFU );
				pxIntHdl->usFrameMBPDULength++;

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
            ubNBytesExpected = NBYTES_FOR_NINPUTS( usNInputs );
            if( ( MBM_PDU_FUNC_RD_DISC_RESPSIZE( usNInputs ) == pxIntHdl->usFrameMBPDULength ) &&
                ( MBM_FUNC_RD_DISC_INPUT == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) &&
                ( ubNBytesExpected == pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_FUNC_RD_DISCCNT_RESP_BYTECNT_OFF] ) )
            {

                /* A pointer pointing to the first register value in the stream. */
                pubCurPtr = &( pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_FUNC_RD_DISCCNT_RESP_INPUTS_OFF] );
                for( ubNByteCnt = 0; ubNByteCnt < ubNBytesExpected; ubNByteCnt++ )
                {
                    arubBufferOut[ubNByteCnt] = *pubCurPtr++;
                }
                *peStatus = MB_ENOERR;
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF],
                                                  MBM_FUNC_RD_DISC_INPUT ) )
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
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( MBM_STATE_DONE == *peState )
        {
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE,
                             "[IDX=" MBP_FORMAT_USHORT "] Read discrete inputs finished. Status: " MBP_FORMAT_USHORT
                             "\n", ( USHORT ) pxIntHdl->ubIdx, ( USHORT ) * peStatus );
            }
        }
#endif
    }
#if MBM_ENABLE_FULL_API_CHECKS == 1
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
#endif
}
#endif

/* ----------------------- Start implementation (Blocking functions) --------*/
#if MBM_FUNC_READ_DISC_INPUTS_ENABLED == 1
eMBErrorCode
eMBMReadDiscreteInputs( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usInputStartAddress, USHORT usNInputs,
                        UBYTE arubBufferOut[] )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMReadDiscreteInputsPolled( xHdl, ucSlaveAddress, usInputStartAddress, usNInputs, arubBufferOut, &eState,
                                      &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif
