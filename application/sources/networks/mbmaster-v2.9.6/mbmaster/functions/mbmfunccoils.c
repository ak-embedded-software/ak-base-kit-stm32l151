/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008-2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmfunccoils.c,v 1.15 2011-12-04 21:10:57 embedded-solutions.cwalter Exp $
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

#define MBM_FUNC_RD_COILS                           ( 0x01 )
#define MBM_PDU_FUNC_RD_COILSCNT_MAX                ( 0x07D0 )
#define MBM_PDU_FUNC_RD_COILS_RESP_BYTECNT_OFF      ( MB_PDU_DATA_OFF )
#define MBM_PDU_FUNC_RD_COILS_RESP_INPUTS_OFF       ( MBM_PDU_FUNC_RD_COILS_RESP_BYTECNT_OFF + 1 )

#define MBM_PDU_FUNC_RD_COILS_RESPSIZE( usNCoils )  \
    ( UBYTE )( NBYTES_FOR_NINPUTS( usNCoils ) + 1 + 1 )

#define MBM_FUNC_WR_SINGLE_COIL                     ( 0x05 )
#define MBM_FUNC_WR_SINGLE_COIL_RESPSIZE            ( 5 )
#define MBM_FUNC_WR_SINGLE_COIL_RESP_ADDR_OFF       ( MB_PDU_DATA_OFF )
#define MBM_FUNC_WR_SINGLE_COIL_RESP_VALUE_OFF      ( MBM_FUNC_WR_SINGLE_COIL_RESP_ADDR_OFF + 2 )

#define NBYTES_FOR_NCOILS( usNCoils ) \
    ( UBYTE )( ( ( usNCoils ) + 7U ) / 8U )

#define MBM_FUNC_WR_COILS                           ( 0x0F )
#define MBM_FUNC_WR_COILS_COILSCNT_MAX              ( 0x07D0 )
#define MBM_FUNC_WR_COILD_RESPSIZE                  ( 5 )
#define MBM_FUNC_WR_COILS_RESP_ADDR_OFF             ( MB_PDU_DATA_OFF )
#define MBM_FUNC_WR_COILS_NCOILS_OFF                ( MBM_FUNC_WR_COILS_RESP_ADDR_OFF + 2 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation (Polling functions)----------*/
#if MBM_FUNC_READ_COILS_ENABLED == 1
void
vMBMReadCoilsPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usCoilStartAddress, USHORT usNCoils,
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
            if( ( NULL != arubBufferOut ) && ( usNCoils > 0 ) && ( usNCoils <= MBM_PDU_FUNC_RD_COILSCNT_MAX ) )
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE,
                                 "[IDX=" MBP_FORMAT_USHORT "] Read coils (start=" MBP_FORMAT_USHORT ", length="
                                 MBP_FORMAT_USHORT ").\n", ( USHORT ) pxIntHdl->ubIdx, usCoilStartAddress,
                                 ( USHORT ) usNCoils );
                }
#endif
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_RD_COILS;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usCoilStartAddress >> 8U );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usCoilStartAddress & 0x00FFU );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usNCoils >> 8U );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usNCoils & 0x00FFU );
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
            ubNBytesExpected = NBYTES_FOR_NINPUTS( usNCoils );
            if( ( MBM_PDU_FUNC_RD_COILS_RESPSIZE( usNCoils ) == pxIntHdl->usFrameMBPDULength ) &&
                ( MBM_FUNC_RD_COILS == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) &&
                ( ubNBytesExpected == pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_FUNC_RD_COILS_RESP_BYTECNT_OFF] ) )
            {

                /* A pointer pointing to the first register value in the stream. */
                pubCurPtr = &( pxIntHdl->pubFrameMBPDUBuffer[MBM_PDU_FUNC_RD_COILS_RESP_INPUTS_OFF] );
                for( ubNByteCnt = 0; ubNByteCnt < ubNBytesExpected; ubNByteCnt++ )
                {
                    arubBufferOut[ubNByteCnt] = *pubCurPtr++;
                }
                *peStatus = MB_ENOERR;
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF], MBM_FUNC_RD_COILS ) )
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
                             "[IDX=" MBP_FORMAT_USHORT "] Read coils finished. Status: " MBP_FORMAT_USHORT "\n",
                             ( USHORT ) pxIntHdl->ubIdx, ( USHORT ) * peStatus );
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

#if MBM_FUNC_WRITE_SINGLE_COIL_ENABLED == 1
void
vMBMWriteSingleCoilPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usOutputAddress, BOOL bOn,
                           eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    xMBMInternalHandle *pxIntHdl = ( xMBMInternalHandle * ) xHdl;
    USHORT          usOutputAddressResp, usOutputValueResp, usOutputValueExpected;

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
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
			if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
			{
				vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE,
							 "[IDX=" MBP_FORMAT_USHORT "] Write coil (start=" MBP_FORMAT_USHORT ").\n",
							 ( USHORT ) pxIntHdl->ubIdx, usOutputAddress );
			}
#endif
			pxIntHdl->usFrameMBPDULength = 0;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_WR_SINGLE_COIL;
			pxIntHdl->usFrameMBPDULength++;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usOutputAddress >> 8U );
			pxIntHdl->usFrameMBPDULength++;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usOutputAddress & 0x00FFU );
			pxIntHdl->usFrameMBPDULength++;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( bOn ? 0xFF : 0x00 );
			pxIntHdl->usFrameMBPDULength++;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = 0x00;
			pxIntHdl->usFrameMBPDULength++;

			/* Frame is assembled. Now send it. */
			*peState = MBM_STATE_SEND;
            break;

            /* These states are handled by the common state machine. */
        case MBM_STATE_SEND:
        case MBM_STATE_WAITING:
            vMBMMasterTransactionPolled( pxIntHdl, ucSlaveAddress, peState, peStatus );
            break;

            /* We need to disassemble the response here. */
        case MBM_STATE_DISASSEMBLE:
            *peState = MBM_STATE_DONE;
            if( ( MBM_FUNC_WR_SINGLE_COIL_RESPSIZE == pxIntHdl->usFrameMBPDULength ) &&
                ( MBM_FUNC_WR_SINGLE_COIL == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) )
            {
                /* Additional casts a for PIC MCC18 compiler to fix a bug when -Oi is not used. 
                 * This is required because it does not enforce ANSI c integer promotion
                 * rules.
                 */
                usOutputAddressResp =
                    ( USHORT ) ( ( USHORT ) pxIntHdl->
                                 pubFrameMBPDUBuffer[MBM_FUNC_WR_SINGLE_COIL_RESP_ADDR_OFF] << 8U );
                usOutputAddressResp |=
                    ( USHORT ) ( ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_SINGLE_COIL_RESP_ADDR_OFF + 1] );
                usOutputValueResp =
                    ( USHORT ) ( ( USHORT ) pxIntHdl->
                                 pubFrameMBPDUBuffer[MBM_FUNC_WR_SINGLE_COIL_RESP_VALUE_OFF] << 8U );
                usOutputValueResp |=
                    ( USHORT ) ( ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_SINGLE_COIL_RESP_VALUE_OFF + 1] );
                usOutputValueExpected = ( USHORT ) ( bOn ? 0xFF00 : 0x0000 );
                /* Check if the response matches what we have sent to the slave. */
                if( usOutputAddressResp != usOutputAddress )
                {
                    *peStatus = MB_EIO;
                }
                else if( usOutputValueResp != usOutputValueExpected )
                {
                    *peStatus = MB_EIO;
                }
                else
                {
                    *peStatus = MB_ENOERR;
                }
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF], MBM_FUNC_RD_COILS ) )
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
                             "[IDX=" MBP_FORMAT_USHORT "] Write coil finished. Status: " MBP_FORMAT_USHORT "\n",
                             ( USHORT ) pxIntHdl->ubIdx, ( USHORT ) * peStatus );
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

#if MBM_FUNC_WRITE_COILS_ENABLED == 1
void
vMBMWriteCoilsPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                      USHORT usCoilStartAddress, USHORT usNCoils, const UBYTE arubCoilsIn[],
                      eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    xMBMInternalHandle *pxIntHdl = ( xMBMInternalHandle * ) xHdl;
    UBYTE           ubIdx;
    const UBYTE    *pubCoilInPtr = arubCoilsIn;
    UBYTE           ubNBytesForCoils = NBYTES_FOR_NCOILS( usNCoils );
    USHORT          usCoilStartAddressResp;
    USHORT          usNCoilsResp;

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
            if( ( NULL != pubCoilInPtr ) && ( usNCoils <= MBM_FUNC_WR_COILS_COILSCNT_MAX ) )
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE,
                                 "[IDX=" MBP_FORMAT_USHORT "] Write coils (start=" MBP_FORMAT_USHORT ", length="
                                 MBP_FORMAT_USHORT ").\n", ( USHORT ) pxIntHdl->ubIdx, usCoilStartAddress, usNCoils );
                }
#endif
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_WR_COILS;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usCoilStartAddress >> 8U );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usCoilStartAddress & 0x00FFU );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usNCoils >> 8U );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usNCoils & 0x00FFU );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ubNBytesForCoils;
				pxIntHdl->usFrameMBPDULength++;


                for( ubIdx = 0; ubIdx < ubNBytesForCoils; ubIdx++ )
                {
                    pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = *pubCoilInPtr++;
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

            /* These states are handled by the common state machine. */
        case MBM_STATE_SEND:
        case MBM_STATE_WAITING:
            vMBMMasterTransactionPolled( pxIntHdl, ucSlaveAddress, peState, peStatus );
            break;

            /* We need to disassemble the response here. */
        case MBM_STATE_DISASSEMBLE:
            *peState = MBM_STATE_DONE;
            if( ( MBM_FUNC_WR_COILD_RESPSIZE == pxIntHdl->usFrameMBPDULength ) &&
                ( MBM_FUNC_WR_COILS == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) )
            {
                usCoilStartAddressResp =
                    ( USHORT ) ( ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_COILS_RESP_ADDR_OFF] << 8U );
                usCoilStartAddressResp |=
                    ( USHORT ) ( pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_COILS_RESP_ADDR_OFF + 1] );
                usNCoilsResp = ( USHORT ) ( pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_COILS_NCOILS_OFF] << 8U );
                usNCoilsResp |= ( USHORT ) ( pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_COILS_NCOILS_OFF + 1] );

                /* Check if the response matches what we have sent to the slave. */
                if( usCoilStartAddressResp != usCoilStartAddress )
                {
                    *peStatus = MB_EIO;
                }
                else if( usNCoilsResp != usNCoils )
                {
                    *peStatus = MB_EIO;
                }
                else
                {
                    *peStatus = MB_ENOERR;
                }
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF], MBM_FUNC_WR_COILS ) )
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
                             "[IDX=" MBP_FORMAT_USHORT "] Write coils finished. Status: " MBP_FORMAT_USHORT "\n",
                             ( USHORT ) pxIntHdl->ubIdx, ( USHORT ) * peStatus );
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
#if MBM_FUNC_READ_COILS_ENABLED == 1
eMBErrorCode
eMBMReadCoils( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usCoilStartAddress, USHORT usNCoils,
               /*@out@ */ UBYTE arubBufferOut[] )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMReadCoilsPolled( xHdl, ucSlaveAddress, usCoilStartAddress, usNCoils, arubBufferOut, &eState, &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif

#if MBM_FUNC_WRITE_SINGLE_COIL_ENABLED == 1
eMBErrorCode
eMBMWriteSingleCoil( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usOutputAddress, BOOL bOn )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMWriteSingleCoilPolled( xHdl, ucSlaveAddress, usOutputAddress, bOn, &eState, &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif

#if MBM_FUNC_WRITE_COILS_ENABLED == 1
eMBErrorCode
eMBMWriteCoils( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usCoilStartAddress, USHORT usNCoils,
                const UBYTE arubCoilsIn[] )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMWriteCoilsPolled( xHdl, ucSlaveAddress, usCoilStartAddress, usNCoils, arubCoilsIn, &eState, &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif
