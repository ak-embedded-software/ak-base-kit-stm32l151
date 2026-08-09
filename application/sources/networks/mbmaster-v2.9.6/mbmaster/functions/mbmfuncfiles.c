/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmfuncfiles.c,v 1.1 2011-05-22 22:29:06 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#if defined( __18CXX )
#include <stdio.h>
#endif
#include <string.h>

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
#define MBM_FUNC_RD_FILE_RECORD                         ( 0x14 )
#define MBM_PDU_FUNC_RD_FILE_MAX_SUBREQUESTS            ( 35 )
#define MBM_PDU_FUNC_RD_FILE_REFERENCE_TYPE             ( 6 )
#define MBM_PDU_FUNC_RD_FILE_FILE_NUMBER_MIN            ( 0x0001 )
#define MBM_PDU_FUNC_RD_FILE_FILE_NUMBER_MAX            ( 0xFFFF )
#define MBM_PDU_FUNC_RD_FILE_RECORD_NUMBER_MIN          ( 0x0000 )
#define MBM_PDU_FUNC_RD_FILE_RECORD_NUMBER_MAX          ( 0x270F )
#define MBM_PDU_FUNC_RD_FILE_RECORD_LENGTH_MIN          ( 1 )
#define MBM_PDU_FUNC_RD_FILE_RECORD_LENGTH_MAX          ( 124 )

#define MBM_PDU_FUNC_RD_FILE_RESPSIZE_MIN                       ( 2 )
#define MBM_PDU_FUNC_RD_FILE_RESPSIZE_SUBRESPONSE_LENGTH_MIN    ( 2 )
#define MBM_PDU_FUNC_RD_FILE_RESPSIZE_DATA_LENGTH_OFF           ( MB_PDU_DATA_OFF )
#define MBM_PDU_FUNC_RD_FILE_RESPSIZE_DATA_OFF                  ( MBM_PDU_FUNC_RD_FILE_RESPSIZE_DATA_LENGTH_OFF + 1 )

#define MBM_FUNC_WR_FILE_RECORD                         ( 0x15 )
#define MBM_PDU_FUNC_WR_FILE_MAX_SUBREQUESTS            ( 27 )
#define MBM_PDU_FUNC_WR_FILE_REFERENCE_TYPE             ( 6 )
#define MBM_PDU_FUNC_WR_FILE_FILE_NUMBER_MIN            ( 0x0001 )
#define MBM_PDU_FUNC_WR_FILE_FILE_NUMBER_MAX            ( 0xFFFF )
#define MBM_PDU_FUNC_WR_FILE_RECORD_NUMBER_MIN          ( 0x0000 )
#define MBM_PDU_FUNC_WR_FILE_RECORD_NUMBER_MAX          ( 0x270F )
#define MBM_PDU_FUNC_WR_FILE_RECORD_LENGTH_MIN          ( 1 )
#define MBM_PDU_FUNC_WR_FILE_RECORD_LENGTH_MAX          ( 124 )
#define MBM_PDU_FUNC_WR_FILE_REQ_SIZE_MIN               ( 2 )
#define MBM_PDU_FUNC_WR_FILE_SUBREQ_SIZE_MIN            ( 7 )

#define MBM_PDU_FUNC_WR_FILE_RESPSIZE_DATA_LENGTH_OFF           ( MB_PDU_DATA_OFF )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation (Polling functions)----------*/
#if MBM_FUNC_RD_FILES_ENABLED == 1
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wtype-limits"
void
vMBMReadFileRecordPolled( xMBHandle xHdl, UCHAR ucSlaveAddress,
                          const xMBMFileSubReadReq_t arxSubRequests[], xMBMFileSubReadResp_t arxSubResponses[],
                          USHORT usNSubRequests, eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    UBYTE           ubIdx;
    BOOL            bSubRequestsValid;
    USHORT          usExpectedResponseLength = 0;
    UBYTE           ubRespLength;
    UBYTE           ubRespType;
    UBYTE           ubNBytesLeft;
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
            if( ( NULL != arxSubRequests ) && ( NULL != arxSubResponses ) &&
                ( usNSubRequests > 0 ) && ( usNSubRequests <= MBM_PDU_FUNC_RD_FILE_MAX_SUBREQUESTS ) )
            {
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_RD_FILE_RECORD;
                pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = 7 * usNSubRequests;
                pxIntHdl->usFrameMBPDULength++;
                for( usExpectedResponseLength = MBM_PDU_FUNC_RD_FILE_RESPSIZE_MIN, bSubRequestsValid = TRUE, ubIdx = 0;
                     ubIdx < usNSubRequests; ubIdx++ )
                {
                    if( ( arxSubRequests[ubIdx].usFileNumber < MBM_PDU_FUNC_RD_FILE_FILE_NUMBER_MIN ) ||
                        ( arxSubRequests[ubIdx].usFileNumber > MBM_PDU_FUNC_RD_FILE_FILE_NUMBER_MAX ) )
                    {
                        bSubRequestsValid = FALSE;
                        break;
                    }
                    else if( ( arxSubRequests[ubIdx].usRecordNumber < MBM_PDU_FUNC_RD_FILE_RECORD_NUMBER_MIN ) ||
                             ( arxSubRequests[ubIdx].usRecordNumber > MBM_PDU_FUNC_RD_FILE_RECORD_NUMBER_MAX ) )
                    {
                        bSubRequestsValid = FALSE;
                        break;
                    }
                    else if( ( arxSubRequests[ubIdx].usRecordLength < MBM_PDU_FUNC_RD_FILE_RECORD_LENGTH_MIN ) ||
                             ( arxSubRequests[ubIdx].usRecordLength > MBM_PDU_FUNC_RD_FILE_RECORD_LENGTH_MAX ) )
                    {
                        bSubRequestsValid = FALSE;
                        break;
                    }
                    else
                    {
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            MBM_PDU_FUNC_RD_FILE_REFERENCE_TYPE;
                        pxIntHdl->usFrameMBPDULength++;
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usFileNumber >> 8U );
                        pxIntHdl->usFrameMBPDULength++;
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usFileNumber & 0xFFU );
                        pxIntHdl->usFrameMBPDULength++;
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usRecordNumber >> 8U );
                        pxIntHdl->usFrameMBPDULength++;
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usRecordNumber & 0xFFU );
                        pxIntHdl->usFrameMBPDULength++;
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usRecordLength >> 8U );
                        pxIntHdl->usFrameMBPDULength++;
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usRecordLength & 0xFFU );
                        pxIntHdl->usFrameMBPDULength++;

                        usExpectedResponseLength +=
                            MBM_PDU_FUNC_RD_FILE_RESPSIZE_SUBRESPONSE_LENGTH_MIN +
                            2 * arxSubRequests[ubIdx].usRecordLength;
                    }
                }
                if( bSubRequestsValid && usExpectedResponseLength < MB_PDU_SIZE_MAX )
                {
                    *peState = MBM_STATE_SEND;
                }
                else
                {
                    *peStatus = MB_EINVAL;
                    *peState = MBM_STATE_DONE;
                }
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
            /* First check for minimum frame length and if response is of correct function type */
            if( ( pxIntHdl->usFrameMBPDULength >= MBM_PDU_FUNC_RD_FILE_RESPSIZE_MIN ) &&
                ( MBM_FUNC_RD_FILE_RECORD == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) )
            {
                /* Now get the number of bytes in the response and compare to the actual number
                 * of bytes received. 
                 */
                ubNBytesLeft = pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_FUNC_RD_FILE_RESPSIZE_DATA_LENGTH_OFF];
                if( ubNBytesLeft == ( pxIntHdl->usFrameMBPDULength - MBM_PDU_FUNC_RD_FILE_RESPSIZE_MIN ) )
                {
                    pubCurPtr = &pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_FUNC_RD_FILE_RESPSIZE_DATA_OFF];
                    /* Now decode the requests */
                    for( bSubRequestsValid = TRUE, ubIdx = 0; ubIdx < usNSubRequests; ubIdx++ )
                    {
                        if( ubNBytesLeft < 2 )
                        {
                            bSubRequestsValid = FALSE;
                            break;
                        }
                        else
                        {
                            ubRespLength = *pubCurPtr;
                            pubCurPtr++;
                            ubRespType = *pubCurPtr;
                            pubCurPtr++;
                            if( ubNBytesLeft < ubRespLength )
                            {
                                bSubRequestsValid = FALSE;
                                break;
                            }
                            if( ubRespType != MBM_PDU_FUNC_RD_FILE_REFERENCE_TYPE )
                            {
                                bSubRequestsValid = FALSE;
                                break;
                            }
                            else if( ubRespLength != ( 2 * arxSubRequests[ubIdx].usRecordLength + 1 ) )
                            {
                                bSubRequestsValid = FALSE;
                                break;
                            }
                            else
                            {
                                arxSubResponses[ubIdx].usResponseLength = ubRespLength - 1;
                                arxSubResponses[ubIdx].pubResponse = pubCurPtr;
                                pubCurPtr += arxSubResponses[ubIdx].usResponseLength;
                                ubNBytesLeft -= ubRespLength;
                            }
                        }
                    }
                    if( bSubRequestsValid )
                    {
                        *peStatus = MB_ENOERR;
                    }
                    else
                    {
                        *peStatus = MB_EIO;
                    }
                }
                else
                {
                    *peStatus = MB_EIO;
                }
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF],
                                                  MBM_FUNC_RD_FILE_RECORD ) )
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


eMBErrorCode
eMBMReadFileRecord( xMBHandle xHdl, UCHAR ucSlaveAddress,
                    const xMBMFileSubReadReq_t arxSubRequests[],
                    xMBMFileSubReadResp_t arxSubResponses[], USHORT usNSubRequests )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMReadFileRecordPolled( xHdl, ucSlaveAddress, arxSubRequests, arxSubResponses, usNSubRequests, &eState,
                                  &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif

#if MBM_FUNC_WR_FILES_ENABLED == 1
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wtype-limits"
void
vMBMWriteFileRecordPolled( xMBHandle xHdl, UCHAR ucSlaveAddress,
                           const xMBMFileSubWriteReq_t arxSubRequests[], USHORT usNSubRequests,
                           eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    UBYTE           ubIdx;
    BOOL            bSubRequestsValid;
    USHORT          usExpectedResponseLength = 0;
    UBYTE           ubNBytesLeft, ubNBytesWritten;
    UBYTE           ubRefType;
    USHORT          usFileNumber;
    USHORT          usRecordNumber;
    USHORT          usRecordLength;
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
            if( ( NULL != arxSubRequests ) && ( usNSubRequests > 0 ) && ( usNSubRequests <= MBM_PDU_FUNC_WR_FILE_MAX_SUBREQUESTS ) )
            {
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_WR_FILE_RECORD;
                pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->usFrameMBPDULength++;

                for( usExpectedResponseLength = MBM_PDU_FUNC_WR_FILE_REQ_SIZE_MIN, bSubRequestsValid = TRUE, ubIdx = 0;
                     ubIdx < usNSubRequests; ubIdx++ )
                {
                    usExpectedResponseLength += MBM_PDU_FUNC_WR_FILE_SUBREQ_SIZE_MIN +  2 * arxSubRequests[ubIdx].usRecordLength;
                    if( ( arxSubRequests[ubIdx].usFileNumber < MBM_PDU_FUNC_WR_FILE_FILE_NUMBER_MIN ) ||
                        ( arxSubRequests[ubIdx].usFileNumber > MBM_PDU_FUNC_WR_FILE_FILE_NUMBER_MAX ) )
                    {
                        bSubRequestsValid = FALSE;
                        break;
                    }
                    else if( ( arxSubRequests[ubIdx].usRecordNumber < MBM_PDU_FUNC_WR_FILE_RECORD_NUMBER_MIN ) ||
                             ( arxSubRequests[ubIdx].usRecordNumber > MBM_PDU_FUNC_WR_FILE_RECORD_NUMBER_MAX ) )
                    {
                        bSubRequestsValid = FALSE;
                        break;
                    }
                    else if( ( arxSubRequests[ubIdx].usRecordLength < MBM_PDU_FUNC_WR_FILE_RECORD_LENGTH_MIN ) ||
                             ( arxSubRequests[ubIdx].usRecordLength > MBM_PDU_FUNC_WR_FILE_RECORD_LENGTH_MAX ) ||
                             ( usExpectedResponseLength > MB_PDU_SIZE_MAX ) )
                    {
                        bSubRequestsValid = FALSE;
                        break;
                    }
                    else
                    {
                        /* Reference type */
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            MBM_PDU_FUNC_RD_FILE_REFERENCE_TYPE;
                        pxIntHdl->usFrameMBPDULength++;

                        /* File number */
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usFileNumber >> 8U );
                        pxIntHdl->usFrameMBPDULength++;
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usFileNumber & 0xFFU );
                        pxIntHdl->usFrameMBPDULength++;

                        /* Record number */
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usRecordNumber >> 8U );
                        pxIntHdl->usFrameMBPDULength++;
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usRecordNumber & 0xFFU );
                        pxIntHdl->usFrameMBPDULength++;

                        /* Record length */
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usRecordLength >> 8U );
                        pxIntHdl->usFrameMBPDULength++;
                        pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                            ( UBYTE ) ( arxSubRequests[ubIdx].usRecordLength & 0xFFU );
                        pxIntHdl->usFrameMBPDULength++;

                        for( ubNBytesWritten = 0; ubNBytesWritten < 2*arxSubRequests[ubIdx].usRecordLength; ubNBytesWritten+=2 )
                        {
                            pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = arxSubRequests[ubIdx].pubRecordData[ubNBytesWritten];
                            pxIntHdl->usFrameMBPDULength++;
                            pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = arxSubRequests[ubIdx].pubRecordData[ubNBytesWritten+1];
                            pxIntHdl->usFrameMBPDULength++;
                        }
                    }
                }
                if( bSubRequestsValid && usExpectedResponseLength < MB_PDU_SIZE_MAX )
                {
                    pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_FUNC_RD_FILE_RESPSIZE_DATA_LENGTH_OFF]=usExpectedResponseLength-MBM_PDU_FUNC_WR_FILE_REQ_SIZE_MIN;
                    *peState = MBM_STATE_SEND;
                }
                else
                {
                    *peStatus = MB_EINVAL;
                    *peState = MBM_STATE_DONE;
                }
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
            /* First check for minimum frame length and if response is of correct function type */
            if( ( pxIntHdl->usFrameMBPDULength >= MBM_PDU_FUNC_WR_FILE_REQ_SIZE_MIN ) &&
                ( MBM_FUNC_WR_FILE_RECORD == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) )
            {
                /* Now get the number of bytes in the response and compare to the actual number
                 * of bytes received. 
                 */
                ubNBytesLeft = pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_FUNC_WR_FILE_RESPSIZE_DATA_LENGTH_OFF];
                if( ubNBytesLeft == ( pxIntHdl->usFrameMBPDULength - MBM_PDU_FUNC_WR_FILE_REQ_SIZE_MIN ) )
                {
                    pubCurPtr = &pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_FUNC_WR_FILE_RESPSIZE_DATA_LENGTH_OFF+1];
                    /* Now decode the requests */
                    for( bSubRequestsValid = TRUE, ubIdx = 0; ubIdx < usNSubRequests; ubIdx++ )
                    {
                        if( ubNBytesLeft <  MBM_PDU_FUNC_WR_FILE_SUBREQ_SIZE_MIN +  2 * arxSubRequests[ubIdx].usRecordLength )
                        {         
                            bSubRequestsValid = FALSE;
                            break;
                        }
                        else
                        {
                            ubRefType = *pubCurPtr;
                            pubCurPtr++;
                            usFileNumber = ( USHORT )*pubCurPtr << 8U;
                            pubCurPtr++;
                            usFileNumber |= ( USHORT )*pubCurPtr;        
                            pubCurPtr++;
                            usRecordNumber = ( USHORT )*pubCurPtr << 8U;
                            pubCurPtr++;
                            usRecordNumber |= ( USHORT )*pubCurPtr;        
                            pubCurPtr++;
                            usRecordLength = ( USHORT )*pubCurPtr << 8U;
                            pubCurPtr++;
                            usRecordLength |= ( USHORT )*pubCurPtr;        
                            pubCurPtr++;
                            ubNBytesLeft -= MBM_PDU_FUNC_WR_FILE_SUBREQ_SIZE_MIN;

                            if( ( usFileNumber != arxSubRequests[ubIdx].usFileNumber ) ||
                                ( usRecordNumber != arxSubRequests[ubIdx].usRecordNumber ) ||
                                ( usRecordLength != arxSubRequests[ubIdx].usRecordLength ) )
                            {
                                bSubRequestsValid = FALSE;
                                break;
                            }
                            else if( 0 != memcmp(  arxSubRequests[ubIdx].pubRecordData, pubCurPtr, 2 * usRecordLength ) )
                            {
                                bSubRequestsValid = FALSE;
                                break;
                            }
                            else
                            {
                                pubCurPtr += 2 * usRecordLength;
                                ubNBytesLeft -= 2 * usRecordLength;
                            }
                        }
                    }
                    if( bSubRequestsValid )
                    {
                        *peStatus = MB_ENOERR;
                    }
                    else
                    {
                        *peStatus = MB_EIO;
                    }
                }
                else
                {
                    *peStatus = MB_EIO;
                }
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF],
                                                  MBM_FUNC_WR_FILE_RECORD ) )
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
#pragma GCC diagnostic pop

eMBErrorCode 
eMBMWriteFileRecord( xMBHandle xHdl, UCHAR ucSlaveAddress, 
                     const xMBMFileSubWriteReq_t arxSubRequests[],
                     USHORT usNSubRequests )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMWriteFileRecordPolled( xHdl, ucSlaveAddress, arxSubRequests, usNSubRequests, &eState, &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif
