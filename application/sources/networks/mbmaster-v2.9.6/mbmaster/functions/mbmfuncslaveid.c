/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007-2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmfuncslaveid.c,v 1.1 2010-04-29 21:20:45 embedded-so.embedded-solutions.1 Exp $
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
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "internal/mbmiframe.h"
#include "internal/mbmi.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBM_FUNC_REPORT_SLAVE_ID                ( 0x011 )
#define MBM_PDU_RESP_SIZE_BYTECNT_OFF           ( MB_PDU_DATA_OFF )
#define MBM_PDU_RESP_DATA_OFF                   ( MBM_PDU_RESP_SIZE_BYTECNT_OFF + 1 )
/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation (Polling functions)----------*/
#if MBM_FUNC_REPORT_SLAVEID_ENABLED == 1
void
vMBMReportSlaveID( xMBHandle xHdl, UCHAR ucSlaveAddress, UBYTE arubBufferOut[],
                   UBYTE ubBufferMax, UBYTE * pubLength, eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    UBYTE           ubByteCnt;
    UBYTE           ubRespBytesAvailable;
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
            if( ( NULL != arubBufferOut ) && ( NULL != pubLength ) )
            {
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_REPORT_SLAVE_ID;
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
            if( MBM_FUNC_REPORT_SLAVE_ID == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] )
            {
                ubRespBytesAvailable = pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_RESP_SIZE_BYTECNT_OFF];
                if( ubRespBytesAvailable < ubBufferMax )
                {
                    *pubLength = ubRespBytesAvailable;
                    pubCurPtr = &( pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_RESP_DATA_OFF] );
                    for( ubByteCnt = 0; ubByteCnt < ubRespBytesAvailable; ubByteCnt++, pubCurPtr++ )
                    {

                        arubBufferOut[ubByteCnt] = *pubCurPtr;
                    }
                    *peStatus = MB_ENOERR;
                }
                else
                {
                    *peStatus = MB_ENORES;
                }
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF],
                                                  MBM_FUNC_REPORT_SLAVE_ID ) )
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
#if MBM_FUNC_REPORT_SLAVEID_ENABLED == 1
eMBErrorCode
eMBMReportSlaveID( xMBHandle xHdl, UCHAR ucSlaveAddress, UBYTE arubBufferOut[], UBYTE ubBufferMax, UBYTE * pubLength )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMReportSlaveID( xHdl, ucSlaveAddress, arubBufferOut, ubBufferMax, pubLength, &eState, &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif
