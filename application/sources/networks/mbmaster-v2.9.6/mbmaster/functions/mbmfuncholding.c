/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007-2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmfuncholding.c,v 1.26 2011-12-04 21:10:57 embedded-solutions.cwalter Exp $
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
#define MBM_FUNC_RD_REGS                        ( 0x03 )
#define MBM_FUNC_RD_REGS_REGCNT_MAX             ( 0x007D )
#define MBM_FUNC_RD_REGS_RESP_BYTECNT_OFF       ( MB_PDU_DATA_OFF )
#define MBM_FUNC_RD_REGS_RESP_REGS_OFF          ( MBM_FUNC_RD_REGS_RESP_BYTECNT_OFF + 1 )
#define MBM_FUNC_RD_REGS_RESP_SIZE( nRegs )     ( USHORT )( 1 + 1 + 2 * ( nRegs ) )

#define MBM_FUNC_WR_REG                         ( 0x06 )
#define MBM_FUNC_WR_REGS_RESP_SIZE              ( 5 )
#define MBM_FUNC_WR_REG_RESP_ADDR_OFF           ( MB_PDU_DATA_OFF )
#define MBM_FUNC_WR_REG_RESP_VALUE_OFF          ( MBM_FUNC_WR_REG_RESP_ADDR_OFF + 2 )

#define MBM_FUNC_WR_MUL_REGS                    ( 0x10 )
#define MBM_FUNC_WR_MUL_REGS_RESP_SIZE          ( 5 )
#define MBM_FUNC_WR_MUL_REGS_RESP_ADDR_OFF      ( MB_PDU_DATA_OFF )
#define MBM_FUNC_WR_MUL_REGS_RESP_REGCNT_OFF    ( MBM_FUNC_WR_MUL_REGS_RESP_ADDR_OFF + 2 )
#define MBM_FUNC_WR_MUL_REGCNT_MAX              ( 0x07B )

#define MBM_FUNC_RDWR_MUL_REGS                  ( 0x17 )
#define MBM_FUNC_RDWR_MUL_REGS_REGCNT_WR_MAX    ( 0x79 )
#define MBM_FUNC_RDWR_MUL_REGS_REGCNT_RD_MAX    ( 0x7D )
#define MBM_FUNC_RDWR_MUL_REGS_RESP_SIZE( nRegs ) \
    ( USHORT )( 1 + 1 + 2 * ( nRegs ) )
#define MBM_FUNC_RDWR_MUL_REGS_RESP_BYTECNT_OFF ( MB_PDU_DATA_OFF )
#define MBM_FUNC_RDWR_MUL_REGS_RESP_REGS_OFF    ( MBM_FUNC_RDWR_MUL_REGS_RESP_BYTECNT_OFF + 1)

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation (Polling functions)----------*/

#if MBM_FUNC_WR_SINGLE_REG_ENABLED == 1
void
vMBMWriteSingleRegisterPolled( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usRegAddress, USHORT usValue,
                               eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    USHORT          usRegAddressWritten;
    USHORT          usValueWritten;

    xMBMInternalHandle *pxIntHdl = ( xMBMInternalHandle * ) xHdl;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) && ( peState != NULL ) && ( peStatus != NULL ) )
#else
    if( TRUE )
#endif
    {
        switch ( *peState )
        {
            /* In this state we prepare the frame and sent it. */
        case MBM_STATE_NONE:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
			if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
			{
				vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE,
							 "[IDX=" MBP_FORMAT_USHORT "] Write single holding register request (start="
							 MBP_FORMAT_USHORT ").\n", ( USHORT ) pxIntHdl->ubIdx, usRegAddress );
			}
#endif
			pxIntHdl->usFrameMBPDULength = 0;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_WR_REG;
			pxIntHdl->usFrameMBPDULength++;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usRegAddress >> 8U );
			pxIntHdl->usFrameMBPDULength++;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usRegAddress & 0x00FFU );
			pxIntHdl->usFrameMBPDULength++;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usValue >> 8U );
			pxIntHdl->usFrameMBPDULength++;
			pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usValue & 0x00FFU );
			pxIntHdl->usFrameMBPDULength++;
			/* Frame is assembled. Now send it. */
			*peStatus = MB_EAGAIN;
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
            if( ( MBM_FUNC_WR_REGS_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                ( MBM_FUNC_WR_REG == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) )
            {
                usRegAddressWritten =
                    ( USHORT ) ( ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_REG_RESP_ADDR_OFF] << 8U );

                usRegAddressWritten |= ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_REG_RESP_ADDR_OFF + 1];
                usValueWritten =
                    ( USHORT ) ( ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_REG_RESP_VALUE_OFF] << 8U );
                usValueWritten |= ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_REG_RESP_VALUE_OFF + 1];
                if( ( usRegAddressWritten == usRegAddress ) && ( usValueWritten == usValue ) )
                {
                    *peStatus = MB_ENOERR;
                }
                else
                {
                    *peStatus = MB_EIO;
                }
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF], MBM_FUNC_WR_REG ) )
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
                             "[IDX=" MBP_FORMAT_USHORT "] Write single holding register finished. Status: "
                             MBP_FORMAT_USHORT "\n", ( USHORT ) pxIntHdl->ubIdx, ( USHORT ) * peStatus );
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

#if MBM_FUNC_RD_HOLDING_ENABLED == 1
void
vMBMReadHoldingRegistersPolled( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usRegStartAddress, UBYTE ubNRegs,
                                USHORT arusBufferOut[], eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    UBYTE           ubNRegCnt;
    xMBMInternalHandle *pxIntHdl = ( xMBMInternalHandle * ) xHdl;
    UBYTE          *pubCurPtr;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) && ( peState != NULL ) && ( peStatus != NULL ) && ( NULL != arusBufferOut ) )
#else
    if( TRUE )
#endif
    {
        switch ( *peState )
        {
            /* In this state we prepare the frame and send it. */
        case MBM_STATE_NONE:
            if( ( ( ( ULONG ) usRegStartAddress + ( ULONG ) ubNRegs ) < ( ULONG ) 0x10000 ) &&
                ( ubNRegs > 0 ) && ( ubNRegs <= MBM_FUNC_RD_REGS_REGCNT_MAX ) )
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE,
                                 "[IDX=" MBP_FORMAT_USHORT "] Read holding registers request (start=" MBP_FORMAT_USHORT
                                 ", length=" MBP_FORMAT_USHORT ").\n", ( USHORT ) pxIntHdl->ubIdx, usRegStartAddress,
                                 ( USHORT ) ubNRegs );
                }
#endif
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_RD_REGS;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usRegStartAddress >> 8U );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                    ( UBYTE ) ( usRegStartAddress & 0x00FFU );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = 0;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ubNRegs;
				pxIntHdl->usFrameMBPDULength++;

                /* Frame is assembled. Now send it. */
                *peStatus = MB_EAGAIN;
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
            if( ( MBM_FUNC_RD_REGS_RESP_SIZE( ubNRegs ) == pxIntHdl->usFrameMBPDULength ) &&
                ( MBM_FUNC_RD_REGS == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) &&
                ( ( 2 * ubNRegs ) == pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_RD_REGS_RESP_BYTECNT_OFF] ) )
            {

                /* A pointer pointing to the first register value in the stream. */
                pubCurPtr = &( pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_RD_REGS_RESP_REGS_OFF] );
                for( ubNRegCnt = 0; ubNRegCnt < ubNRegs; ubNRegCnt++ )
                {
                    arusBufferOut[ubNRegCnt] = ( USHORT ) ( ( USHORT ) * pubCurPtr++ << 8U );
                    arusBufferOut[ubNRegCnt] |= ( USHORT ) * pubCurPtr++;
                }
                *peStatus = MB_ENOERR;
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF], MBM_FUNC_RD_REGS ) )
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
                             "[IDX=" MBP_FORMAT_USHORT "] Read holding registers finished. Status: " MBP_FORMAT_USHORT
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

#if MBM_FUNC_WR_MUL_REGS_ENABLED == 1
void
vMBMWriteMultipleRegistersPolled( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usRegStartAddress,
                                  UBYTE ubNRegs, const USHORT arusBufferIn[],
                                  eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    USHORT          usRegAddressWritten;
    USHORT          usRegsWritten;
    UBYTE           ubRegIdx;
    xMBMInternalHandle *pxIntHdl = ( xMBMInternalHandle * ) xHdl;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) && ( peState != NULL ) && ( peStatus != NULL ) && ( NULL != arusBufferIn ) )
#else
    if( TRUE )
#endif
    {
        switch ( *peState )
        {
            /* In this state we prepare the frame and sent it. */
        case MBM_STATE_NONE:
#if MBM_ENABLE_FULL_API_CHECKS == 1
            if( ( ( ( ULONG ) usRegStartAddress + ( ULONG ) ubNRegs ) < 0x10000UL ) &&
                ( ubNRegs > 0 ) && ( ubNRegs <= MBM_FUNC_WR_MUL_REGCNT_MAX ) )
#else
            if( TRUE )
#endif
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE,
                                 "[IDX=" MBP_FORMAT_USHORT "] Write multiple holding registers request (start="
                                 MBP_FORMAT_USHORT ", length=" MBP_FORMAT_USHORT ").\n", ( USHORT ) pxIntHdl->ubIdx,
                                 usRegStartAddress, ( USHORT ) ubNRegs );
                }
#endif
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_WR_MUL_REGS;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( usRegStartAddress >> 8U );
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                    ( UBYTE ) ( usRegStartAddress & 0x00FFU );
                pxIntHdl->usFrameMBPDULength++;					
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = 0;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ubNRegs;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( 2U * ubNRegs );
				pxIntHdl->usFrameMBPDULength++;

                for( ubRegIdx = 0; ubRegIdx < ubNRegs; ubRegIdx++ )
                {
                    pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                        ( UBYTE ) ( arusBufferIn[ubRegIdx] >> 8U );
                    pxIntHdl->usFrameMBPDULength++;						
                    pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                        ( UBYTE ) ( arusBufferIn[ubRegIdx] & 0x00FFU );
                    pxIntHdl->usFrameMBPDULength++;						
                }
                /* Frame is assembled. Now send it. */
                *peStatus = MB_EAGAIN;
                *peState = MBM_STATE_SEND;
            }
#if MBM_ENABLE_FULL_API_CHECKS == 1
            else
            {
                *peStatus = MB_EINVAL;
                *peState = MBM_STATE_DONE;
            }
#endif
            break;

            /* These states are handled by the common state machine. */
        case MBM_STATE_SEND:
        case MBM_STATE_WAITING:
            vMBMMasterTransactionPolled( pxIntHdl, ucSlaveAddress, peState, peStatus );
            break;

            /* We need to disassemble the response here. */
        case MBM_STATE_DISASSEMBLE:
            *peState = MBM_STATE_DONE;
            if( ( MBM_FUNC_WR_MUL_REGS_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                ( MBM_FUNC_WR_MUL_REGS == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) )
            {
                usRegAddressWritten = ( USHORT )
                    ( ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_MUL_REGS_RESP_ADDR_OFF] << 8U );
                usRegAddressWritten |= ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_MUL_REGS_RESP_ADDR_OFF + 1];
                usRegsWritten = ( USHORT )
                    ( ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_MUL_REGS_RESP_REGCNT_OFF] << 8U );
                usRegsWritten |= ( USHORT ) pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_WR_MUL_REGS_RESP_REGCNT_OFF + 1];
                if( ( usRegAddressWritten == usRegStartAddress ) && ( ( USHORT ) ubNRegs == usRegsWritten ) )
                {
                    *peStatus = MB_ENOERR;
                }
                else
                {
                    *peStatus = MB_EIO;
                }
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF],
                                                  MBM_FUNC_WR_MUL_REGS ) )
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
                             "[IDX=" MBP_FORMAT_USHORT "] Write multiple holding registers finished. Status: "
                             MBP_FORMAT_USHORT "\n", ( USHORT ) pxIntHdl->ubIdx, ( USHORT ) * peStatus );
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

#if MBM_FUNC_RDWR_MUL_REGS_ENABLED == 1
void
vMBMReadWriteMultipleRegistersPolled( xMBHandle xHdl, UCHAR ucSlaveAddress,
                                      USHORT usWriteRegStartAddress, UBYTE ubWriteNRegs,
                                      const USHORT arusBufferIn[],
                                      USHORT usReadRegStartAddress, UBYTE ubReadNRegs,
                                      USHORT arusBufferOut[], eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    UBYTE          *pubCurPtr;
    UBYTE           ubRegIdx;
    xMBMInternalHandle *pxIntHdl = ( xMBMInternalHandle * ) xHdl;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) && ( peState != NULL ) && ( peStatus != NULL ) &&
        ( NULL != arusBufferIn ) && ( NULL != arusBufferOut ) )
#else
    if( TRUE )
#endif
    {
        switch ( *peState )
        {
            /* In this state we prepare the frame and sent it. */
        case MBM_STATE_NONE:
            if( MB_IS_VALID_READ_ADDR( ucSlaveAddress ) &&
                ( ( ( ULONG ) usWriteRegStartAddress + ( ULONG ) ubWriteNRegs ) < 0x10000UL ) &&
                ( ubWriteNRegs > 0 ) && ( ubWriteNRegs <= MBM_FUNC_RDWR_MUL_REGS_REGCNT_WR_MAX ) &&
                ( ( ( ULONG ) usReadRegStartAddress + ( ULONG ) ubReadNRegs ) < 0x10000UL ) &&
                ( ubReadNRegs > 0 ) && ( ubReadNRegs <= MBM_FUNC_RDWR_MUL_REGS_REGCNT_RD_MAX ) )
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE,
                                 "[IDX=" MBP_FORMAT_USHORT
                                 "] Write/read holding multiple registers request (write start=" MBP_FORMAT_USHORT
                                 ", write length=" MBP_FORMAT_USHORT ", read start=" MBP_FORMAT_USHORT ", read length="
                                 MBP_FORMAT_USHORT ").\n", ( USHORT ) pxIntHdl->ubIdx, usWriteRegStartAddress,
                                 ( USHORT ) ubWriteNRegs, usReadRegStartAddress, ( USHORT ) ubReadNRegs );
                }
#endif
                pxIntHdl->usFrameMBPDULength = 0;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = MBM_FUNC_RDWR_MUL_REGS;
                pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                    ( UBYTE ) ( usReadRegStartAddress >> 8U );
				pxIntHdl->usFrameMBPDULength++;	
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =				
                    ( UBYTE ) ( usReadRegStartAddress & 0x00FFU );
				pxIntHdl->usFrameMBPDULength++;	
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = 0;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ubReadNRegs;
				pxIntHdl->usFrameMBPDULength++;

                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                    ( UBYTE ) ( usWriteRegStartAddress >> 8U );
				pxIntHdl->usFrameMBPDULength++;	
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                    ( UBYTE ) ( usWriteRegStartAddress & 0x00FFU );
				pxIntHdl->usFrameMBPDULength++;	
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = 0;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ubWriteNRegs;
				pxIntHdl->usFrameMBPDULength++;
                pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] = ( UBYTE ) ( 2U * ubWriteNRegs );
				pxIntHdl->usFrameMBPDULength++;

                for( ubRegIdx = 0; ubRegIdx < ubWriteNRegs; ubRegIdx++ )
                {
                    pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                        ( UBYTE ) ( arusBufferIn[ubRegIdx] >> 8U );
					pxIntHdl->usFrameMBPDULength++;	
                    pxIntHdl->pubFrameMBPDUBuffer[pxIntHdl->usFrameMBPDULength] =
                        ( UBYTE ) ( arusBufferIn[ubRegIdx] & 0x00FFU );
					pxIntHdl->usFrameMBPDULength++;	
                }
                /* Frame is assembled. Now send it. */
                *peStatus = MB_EAGAIN;
                *peState = MBM_STATE_SEND;
            }
#if MBM_ENABLE_FULL_API_CHECKS == 1
            else
            {
                *peStatus = MB_EINVAL;
                *peState = MBM_STATE_DONE;
            }
#endif
            break;

            /* These states are handled by the common state machine. */
        case MBM_STATE_SEND:
        case MBM_STATE_WAITING:
            vMBMMasterTransactionPolled( pxIntHdl, ucSlaveAddress, peState, peStatus );
            break;

            /* We need to disassemble the response here. */
        case MBM_STATE_DISASSEMBLE:
            *peState = MBM_STATE_DONE;
            if( ( MBM_FUNC_RDWR_MUL_REGS_RESP_SIZE( ubReadNRegs ) == pxIntHdl->usFrameMBPDULength ) &&
                ( MBM_FUNC_RDWR_MUL_REGS == pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF] ) &&
                ( ( 2 * ubReadNRegs ) == pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_RDWR_MUL_REGS_RESP_BYTECNT_OFF] ) )
            {

                /* A pointer pointing to the first register value in the stream. */
                pubCurPtr = &( pxIntHdl->pubFrameMBPDUBuffer[MBM_FUNC_RDWR_MUL_REGS_RESP_REGS_OFF] );
                for( ubRegIdx = 0; ubRegIdx < ubReadNRegs; ubRegIdx++ )
                {
                    arusBufferOut[ubRegIdx] = ( USHORT ) ( ( USHORT ) * pubCurPtr++ << 8U );
                    arusBufferOut[ubRegIdx] |= ( USHORT ) * pubCurPtr++;
                }
                *peStatus = MB_ENOERR;
            }
            /* Check for exception frame. */
            else if( ( MB_PDU_EX_RESP_SIZE == pxIntHdl->usFrameMBPDULength ) &&
                     MB_PDU_FUNC_ISEXCEPTION_FOR( pxIntHdl->pubFrameMBPDUBuffer[MB_PDU_FUNC_OFF],
                                                  MBM_FUNC_RDWR_MUL_REGS ) )
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
                             "[IDX=" MBP_FORMAT_USHORT "] Write/read holding multiple registers done. Status: "
                             MBP_FORMAT_USHORT "\n", ( USHORT ) pxIntHdl->ubIdx, ( USHORT ) * peStatus );
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
#if MBM_FUNC_WR_SINGLE_REG_ENABLED == 1
eMBErrorCode
eMBMWriteSingleRegister( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usRegAddress, USHORT usValue )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMWriteSingleRegisterPolled( xHdl, ucSlaveAddress, usRegAddress, usValue, &eState, &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif

#if MBM_FUNC_RD_HOLDING_ENABLED == 1
eMBErrorCode
eMBMReadHoldingRegisters( xMBHandle xHdl, UCHAR ucSlaveAddress,
                          USHORT usRegStartAddress, UBYTE ubNRegs, USHORT arusBufferOut[] )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMReadHoldingRegistersPolled( xHdl, ucSlaveAddress, usRegStartAddress, ubNRegs, arusBufferOut, &eState,
                                        &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif

#if MBM_FUNC_WR_MUL_REGS_ENABLED == 1
eMBErrorCode
eMBMWriteMultipleRegisters( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usRegStartAddress,
                            UBYTE ubNRegs, const USHORT arusBufferIn[] )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMWriteMultipleRegistersPolled( xHdl, ucSlaveAddress, usRegStartAddress, ubNRegs, arusBufferIn, &eState,
                                          &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif

#if MBM_FUNC_RDWR_MUL_REGS_ENABLED == 1
eMBErrorCode
eMBMReadWriteMultipleRegisters( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usWriteRegStartAddress, UBYTE ubWriteNRegs,
                                const USHORT arusBufferIn[],
                                USHORT usReadRegStartAddress, UBYTE ubReadNRegs, USHORT arusBufferOut[] )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    do
    {
        vMBMReadWriteMultipleRegistersPolled( xHdl, ucSlaveAddress,
                                              usWriteRegStartAddress, ubWriteNRegs, arusBufferIn,
                                              usReadRegStartAddress, ubReadNRegs, arusBufferOut, &eState, &eStatus );
    }
    while( eState != MBM_STATE_DONE );
    return eStatus;
}
#endif
