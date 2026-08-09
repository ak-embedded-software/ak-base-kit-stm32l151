/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008-2012 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmrtu.c,v 1.54 2013-05-21 21:04:12 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "internal/mbmiframe.h"
#include "internal/mbmi.h"
#include "mbmrtu.h"
#include "mbmcrc.h"

#if ( defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 ) )
#include <stdio.h>
#endif

/*lint --e{717} ~ suppress messages: do .. while(0); */
/*lint --e{767} ~ suppress messages: macro defined differently */
/*lint --e{788} ~ suppress messages: enum constant not used within defaulted switch */

#if MBM_RTU_ENABLED == 1

/* ----------------------- Defines ------------------------------------------*/
#define MBM_SER_PDU_SIZE_MIN            ( 4 )   /*!< Minimum size of a MODBUS RTU frame. */
#if !defined( MBM_SER_PDU_SIZE_MAX )
#define MBM_SER_PDU_SIZE_MAX            ( 256 ) /*!< Maximum size of a MODBUS RTU frame. */
#endif
#define MBM_SER_PDU_SIZE_CRC            ( 2 )   /*!< Size of CRC field in PDU. */
#define MBM_SER_PDU_ADDR_OFF            ( 0 )   /*!< Offset of slave address in RTU frame. */
#define MBM_SER_PDU_PDU_OFF             ( 1 )   /*!< Offset of Modbus-PDU in RTU frame. */

#define IDX_INVALID                     ( 255 )

#ifndef MBM_TEST_DISABLE_RTU_TIMEOUTS
#define MBM_TEST_DISABLE_RTU_TIMEOUTS   ( 0 )
#endif

/* Work around a bug in MCC18 compiler. */
#if defined(__18CXX)
#define HDL_RESET_RX( x ) do { \
    ( x )->eRcvState = MBM_STATE_RX_IDLE; \
    ( x )->usRcvBufferPos &= 0; \
} while( 0 );
#else
#define HDL_RESET_RX( x ) do { \
    ( x )->eRcvState = MBM_STATE_RX_IDLE; \
    ( x )->usRcvBufferPos = 0; \
} while( 0 );
#endif

#define HDL_RESET_TX( x ) do { \
    ( x )->eSndState = MBM_STATE_TX_IDLE; \
    ( x )->usSndBufferCnt = 0; \
    ( x )->pubSndBufferCur = NULL; \
} while( 0 );

#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
#define HDL_RESET_WAIT( x ) do { \
    ( x )->xTmrWaitHdl = MBP_TIMERHDL_INVALID; \
} while( 0 )
#else
#define HDL_RESET_WAIT( x )
#endif

#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 1
#define HDL_RESET_T35( x )
#else
#define HDL_RESET_T35( x ) do { \
  ( x )->xTmrHdl = MBP_TIMERHDL_INVALID; \
} while( 0 )
#endif

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    HDL_RESET_RX( x ); \
    HDL_RESET_TX( x ); \
    HDL_RESET_T35( x ); \
    ( x )->xSerHdl = MBP_SERIALHDL_INVALID; \
    HDL_RESET_WAIT( x ); \
    memset( ( void * )&( ( x )->ubRTUFrameBuffer[ 0 ] ), 0, MBM_SER_PDU_SIZE_MAX ); \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    MBM_STATE_RX_IDLE,          /*!< Receiver is in idle state. */
    MBM_STATE_RX_RCV,           /*!< Frame is beeing received. */
    MBM_STATE_RX_ERROR          /*!< Receiver error condition. */
} eMBMRTURcvState;

typedef enum
{
    MBM_STATE_TX_IDLE,          /*!< Transmitter is in idle state. */
    MBM_STATE_TX_XMIT           /*!< Transmitter is sending data. */
} eMBMRTUSndState;

typedef struct
{
    UBYTE           ubIdx;
    volatile UBYTE  ubRTUFrameBuffer[MBM_SER_PDU_SIZE_MAX];

    volatile eMBMRTURcvState eRcvState;
    volatile USHORT usRcvBufferPos;

    volatile eMBMRTUSndState eSndState;
    volatile USHORT usSndBufferCnt;
    UBYTE          *pubSndBufferCur;

#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 0
    xMBPTimerHandle xTmrHdl;
#endif
    xMBPSerialHandle xSerHdl;
#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
    xMBPTimerHandle xTmrWaitHdl;
#endif
} xMBMRTUFrameHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;

STATIC xMBMRTUFrameHandle xMBMRTUFrameHdl[MBM_SERIAL_RTU_MAX_INSTANCES];

/* ----------------------- Static functions ---------------------------------*/
#if MBM_TEST_INSTANCES == 0
STATIC
#endif
    BOOL bMBMSerialRTUT35CB( xMBHandle xHdl );
#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
STATIC BOOL     bMBMSerialWaitCB( xMBHandle xHdl );
#endif
#if MBM_SERIAL_API_VERSION == 1
STATIC void     bMBMSerialRTUReceiverAPIV1CB( xMBHandle xHdl, UBYTE ubValue );

STATIC BOOL     bMBMSerialRTUTransmitterEmptyAPIV1CB( xMBHandle xHdl, UBYTE * ubValue );
#elif MBM_SERIAL_API_VERSION == 2
STATIC BOOL     bMBMSerialRTUTransmitterEmptyAPIV2CB( xMBHandle xHdl, UBYTE * pubBufferOut, USHORT usBufferMax, USHORT * pusBufferWritten );
STATIC void     vMBMSerialRTUReceiverAPIV2CB( xMBHandle xHdl, const UBYTE * pubBufferIn, USHORT usBufferLen );
#endif

STATIC eMBErrorCode eMBMSerialRTUFrameSend( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength );
STATIC eMBErrorCode eMBMSerialRTUFrameReceive( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength );
STATIC eMBErrorCode eMBMSerialRTUFrameClose( xMBHandle xHdl );
STATIC eMBErrorCode eMBMSerialRTUFrameCloseInternal( xMBMRTUFrameHandle * pxRTUHdl );
STATIC BOOL     eMBMSerialRTUFrameIsTransmitting( xMBHandle xHdl );

#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
void            vMBMLogRTUFrame( eMBPortLogLevel eLevel, xMBMInternalHandle * pxIntHdl, char *szMsg, const UBYTE * pubPayload, USHORT usLength );
#endif

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBMSerialRTUInit( xMBMInternalHandle * pxIntHdl, UCHAR ucPort, ULONG ulBaudRate, eMBSerialParity eParity, UCHAR ucStopBits )
{
    eMBErrorCode    eStatus = MB_ENOERR, eStatus2;
    xMBMRTUFrameHandle *pxFrameHdl = NULL;
    UBYTE           ubIdx;
    USHORT          usTimeoutMS;

#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
    USHORT          usTimeoutMSWaitAfterSend;
#endif

    ( void )usTimeoutMS;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( ( NULL != pxIntHdl ) && ( ulBaudRate > 0 ) )
#else
    if( TRUE )
#endif
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !bIsInitialized )
        {
            for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBMRTUFrameHdl ); ubIdx++ )
            {
                HDL_RESET( &xMBMRTUFrameHdl[ubIdx] );
            }
            bIsInitialized = TRUE;
        }

        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBMRTUFrameHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xMBMRTUFrameHdl[ubIdx].ubIdx )
            {
                pxFrameHdl = &xMBMRTUFrameHdl[ubIdx];
                pxFrameHdl->ubIdx = ubIdx;
                break;
            }
        }

        if( NULL != pxFrameHdl )
        {
#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 0
#if MBM_SERIAL_API_VERSION == 2
            usTimeoutMS = ( USHORT ) MBM_SERIAL_APIV2_RTU_DYNAMIC_TIMEOUT_MS( ulBaudRate );
#else
            /* If baudrate > 19200 then we should use the fixed timer value 1750us. 
             * We can't match this exactly so we use 2000us. Otherwise use 3.5 timers
             * the character timeout. */
            if( ulBaudRate > 19200 )
            {
                usTimeoutMS = 2;
            }
            else
            {
                /* The number of ticks required for a character is given by 
                 * xTicksCh = TIMER_TICKS_PER_SECOND * 11 / BAUDRATE
                 * The total timeout is given by xTicksCh * 3.5 = xTicksCh * 7/2.
                 */
                usTimeoutMS = ( USHORT ) ( ( 1000UL * 11UL * 7UL ) / ( 2 * ulBaudRate ) );
            }
#endif
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_RTU ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER, "[IDX=" MBP_FORMAT_USHORT "] Using RTU timeout of "
                             MBP_FORMAT_USHORT "ms.\n", pxFrameHdl->ubIdx, usTimeoutMS );
            }
#endif
#endif

#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
            usTimeoutMSWaitAfterSend = MBM_SERIAL_RTU_DYNAMIC_WAITAFTERSEND_TIMEOUT_MS( ulBaudRate );
#endif

            if( MB_ENOERR != ( eStatus2 = eMBPSerialInit( &( pxFrameHdl->xSerHdl ), ucPort, ulBaudRate, 8, eParity, ucStopBits, pxIntHdl
#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 1
                , bMBMSerialRTUT35CB, MB_RTU
#endif                
                ) ) )
            {
                eStatus = eStatus2;
            }
#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 0
            else if( MB_ENOERR != ( eStatus2 = eMBPTimerInit( &( pxFrameHdl->xTmrHdl ), usTimeoutMS, bMBMSerialRTUT35CB, pxIntHdl ) ) )
            {
                eStatus = eStatus2;
            }
#endif
#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
            else if( MB_ENOERR != ( eStatus2 = eMBPTimerInit( &( pxFrameHdl->xTmrWaitHdl ), usTimeoutMSWaitAfterSend, bMBMSerialWaitCB, pxIntHdl ) ) )
            {
                eStatus = eStatus2;
            }
#endif
            else
            {
                /* Attach the frame handle to the protocol stack. */
                pxIntHdl->pubFrameMBPDUBuffer = ( UBYTE * ) & pxFrameHdl->ubRTUFrameBuffer[MBM_SER_PDU_PDU_OFF];
                pxIntHdl->xFrameHdl = pxFrameHdl;
                pxIntHdl->pFrameSendFN = eMBMSerialRTUFrameSend;
                pxIntHdl->pFrameRecvFN = eMBMSerialRTUFrameReceive;
                pxIntHdl->pFrameCloseFN = eMBMSerialRTUFrameClose;
#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
                pxIntHdl->pFrameIsTransmittingFN = eMBMSerialRTUFrameIsTransmitting;
#endif
                eStatus = MB_ENOERR;
            }

#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_RTU ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_RTU,
                             "[IDX=" MBP_FORMAT_USHORT "] Creation of new RTU instance (port=" MBP_FORMAT_USHORT
                             ", baudrate=" MBP_FORMAT_ULONG ", parity=" MBP_FORMAT_USHORT "): %s.\n",
                             ( USHORT ) pxFrameHdl->ubIdx, ( USHORT ) ucPort, ulBaudRate, ( USHORT ) eParity, eStatus == MB_ENOERR ? "okay" : "failed" );
            }
#endif
            if( MB_ENOERR != eStatus )
            {
                ( void )eMBMSerialRTUFrameCloseInternal( pxFrameHdl );
            }
        }
        else
        {
            eStatus = MB_ENORES;
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
#if MBM_ENABLE_FULL_API_CHECKS == 1
    else
    {
        eStatus = MB_EINVAL;
    }
#endif


    return eStatus;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
STATIC          BOOL
eMBMSerialRTUFrameIsTransmitting( xMBHandle xHdl )
{
    BOOL            bIsTransmitting = FALSE;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMRTUFrameHandle *pxRTUHdl;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) )
#else
    if( TRUE )
#endif
    {
        pxRTUHdl = pxIntHdl->xFrameHdl;
        if( MBM_STATE_TX_XMIT == pxRTUHdl->eSndState )
        {
            bIsTransmitting = TRUE;
        }
    }
    return bIsTransmitting;
}
#pragma GCC diagnostic pop

STATIC          eMBErrorCode
eMBMSerialRTUFrameSend( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    USHORT          usCRC16;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMRTUFrameHandle *pxRTUHdl;

#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
    xMBAnalyzerFrame xAnalyzerFrame;

    xMBPTimeStamp   xTimeStamp;
#endif

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) )
#else
    if( TRUE )
#endif
    {
        pxRTUHdl = pxIntHdl->xFrameHdl;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_RTU ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_RTU,
                         "[IDX=" MBP_FORMAT_USHORT "] Sending new frame for slave=" MBP_FORMAT_USHORT " with length="
                         MBP_FORMAT_USHORT ".\n", ( USHORT ) pxRTUHdl->ubIdx, ( USHORT ) ucSlaveAddress, usMBPDULength );
        }
#endif
        if( MB_IS_VALID_HDL( pxRTUHdl, xMBMRTUFrameHdl ) &&
            MB_IS_VALID_WRITE_ADDR( ucSlaveAddress ) && ( usMBPDULength <= ( MBM_SER_PDU_SIZE_MAX - ( 1 /* Slave Address */  + 2 /* CRC16 */  ) ) ) )
        {
            MBP_ENTER_CRITICAL_SECTION(  );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_RTU ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_RTU,
                             "[IDX=" MBP_FORMAT_USHORT "] Sender state check: receiver ( state = " MBP_FORMAT_USHORT
                             ", pos = " MBP_FORMAT_USHORT " ), sender ( state = " MBP_FORMAT_USHORT ", cnt = "
                             MBP_FORMAT_USHORT " ).\n", ( USHORT ) pxRTUHdl->ubIdx, ( USHORT ) pxRTUHdl->eRcvState,
                             pxRTUHdl->usRcvBufferPos, ( USHORT ) pxRTUHdl->eSndState, pxRTUHdl->usSndBufferCnt );
            }
#endif
            MBP_ASSERT( MBM_STATE_TX_IDLE == pxRTUHdl->eSndState );
            MBP_EXIT_CRITICAL_SECTION(  );

            /* Added the MODBUS RTU header (= slave address) */
            pxRTUHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF] = ( UBYTE ) ucSlaveAddress;
            pxRTUHdl->usSndBufferCnt = 1;

            /* MODBUS PDU is already embedded in the frame. */
            pxRTUHdl->usSndBufferCnt += usMBPDULength;

            usCRC16 = usMBMCRC16( ( const UBYTE * )&pxRTUHdl->ubRTUFrameBuffer[0], pxRTUHdl->usSndBufferCnt );
            pxRTUHdl->ubRTUFrameBuffer[pxRTUHdl->usSndBufferCnt] = ( UBYTE ) ( usCRC16 & 0xFFU );
            pxRTUHdl->usSndBufferCnt++;
            pxRTUHdl->ubRTUFrameBuffer[pxRTUHdl->usSndBufferCnt] = ( UBYTE ) ( usCRC16 >> 8U );
            pxRTUHdl->usSndBufferCnt++;

            /* Enable transmitter */
            MBP_ENTER_CRITICAL_SECTION(  );
            pxRTUHdl->eSndState = MBM_STATE_TX_XMIT;
            pxRTUHdl->pubSndBufferCur = ( UBYTE * ) & pxRTUHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF];
#if MBM_SERIAL_API_VERSION == 2
            if( MB_ENOERR != ( eStatus = eMBPSerialTxEnable( pxRTUHdl->xSerHdl, ( pbMBPSerialTransmitterEmptyCB ) bMBMSerialRTUTransmitterEmptyAPIV2CB ) ) )
            {
                HDL_RESET_TX( pxRTUHdl );
            }
#elif MBM_SERIAL_API_VERSION == 1
            if( MB_ENOERR != ( eStatus = eMBPSerialTxEnable( pxRTUHdl->xSerHdl, ( pbMBPSerialTransmitterEmptyCB ) bMBMSerialRTUTransmitterEmptyAPIV1CB ) ) )
            {
                HDL_RESET_TX( pxRTUHdl );
            }
#else
#error "Define either MBM_SERIAL_API_VERSION=1 or 2!"
#endif
            else
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_RTU ) )
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                    vMBMLogRTUFrame( MB_LOG_DEBUG, pxIntHdl, "Sending frame: ",
                                     ( const UBYTE * )&pxRTUHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF], pxRTUHdl->usSndBufferCnt );
#else
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_RTU,
                                 "[IDX=" MBP_FORMAT_USHORT "] Sending frame with length " MBP_FORMAT_USHORT ".\n",
                                 ( USHORT ) pxRTUHdl->ubIdx, pxRTUHdl->usSndBufferCnt );
#endif
                }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                /* Account for sent frame. */
                pxIntHdl->xFrameStat.ulNPacketsSent += 1;
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                /* Pass frame to protocol analyzer. */
                xAnalyzerFrame.eFrameDir = MB_FRAME_SEND;
                xAnalyzerFrame.eFrameType = MB_FRAME_RTU;
                xAnalyzerFrame.x.xRTUHeader.ubSlaveAddress = ucSlaveAddress;
                xAnalyzerFrame.x.xRTUHeader.usCRC16 = ( USHORT ) ( ( usCRC16 & 0xFF ) << 8U ) + ( USHORT ) ( usCRC16 >> 8U );
                xAnalyzerFrame.ubDataPayload = ( const UBYTE * )&pxRTUHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF];
                xAnalyzerFrame.usDataPayloadLength = pxRTUHdl->usSndBufferCnt;
                if( NULL != pxIntHdl->pvMBAnalyzerCallbackFN )
                {
                    vMBPGetTimeStamp( &xTimeStamp );
                    pxIntHdl->pvMBAnalyzerCallbackFN( pxIntHdl, pxIntHdl->pvCtx, &xTimeStamp, &xAnalyzerFrame );
                }
#endif
            }
            MBP_EXIT_CRITICAL_SECTION(  );
        }
    }
    return eStatus;
}

STATIC          eMBErrorCode
eMBMSerialRTUFrameReceive( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength )
{
    eMBErrorCode    eStatus = MB_EINVAL, eStatus2;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMRTUFrameHandle *pxRTUHdl;

#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
    xMBAnalyzerFrame xAnalyzerFrame;

    xMBPTimeStamp   xTimeStamp;
#endif
#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) )
#else
    if( TRUE )
#endif
    {
        pxRTUHdl = pxIntHdl->xFrameHdl;
        if( MB_IS_VALID_HDL( pxRTUHdl, xMBMRTUFrameHdl ) && MB_IS_VALID_READ_ADDR( ucSlaveAddress ) )
        {

            if( NULL == pusMBPDULength )
            {
                /* This function might be called because of a timeout. Timers and
                 * receiver/transmitters are disabled at the end of this function.
                 */
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_RTU ) )
                {
                    vMBPPortLog( MB_LOG_WARN, MB_LOG_RTU,
                                 "[IDX=" MBP_FORMAT_USHORT "] Receiver aborted due to error or timeout!\n", ( USHORT ) pxRTUHdl->ubIdx );
                }
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                xAnalyzerFrame.eFrameType = MB_FRAME_DAMAGED;
#endif
            }
            else if( ( pxRTUHdl->usRcvBufferPos >= MBM_SER_PDU_SIZE_MIN ) &&
                     ( ucSlaveAddress == pxRTUHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF] ) &&
                     ( usMBMCRC16( ( UBYTE * ) & ( pxRTUHdl->ubRTUFrameBuffer[0] ), pxRTUHdl->usRcvBufferPos ) == 0 ) )
            {
                *pusMBPDULength = ( USHORT ) ( pxRTUHdl->usRcvBufferPos - ( MBM_SER_PDU_PDU_OFF + MBM_SER_PDU_SIZE_CRC ) );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_RTU ) )
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                    vMBMLogRTUFrame( MB_LOG_DEBUG, pxIntHdl, "Received frame: ",
                                     ( const UBYTE * )&pxRTUHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF], pxRTUHdl->usRcvBufferPos );
#else
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_RTU,
                                 "[IDX=" MBP_FORMAT_USHORT "] Received correct frame with length " MBP_FORMAT_USHORT
                                 "!\n", ( USHORT ) pxRTUHdl->ubIdx, pxRTUHdl->usRcvBufferPos );
#endif
                }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                pxIntHdl->xFrameStat.ulNPacketsReceived += 1;
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                xAnalyzerFrame.eFrameType = MB_FRAME_RTU;
                xAnalyzerFrame.x.xRTUHeader.ubSlaveAddress = ucSlaveAddress;
                xAnalyzerFrame.x.xRTUHeader.usCRC16 = pxRTUHdl->ubRTUFrameBuffer[pxRTUHdl->usRcvBufferPos - 1];
                xAnalyzerFrame.x.xRTUHeader.usCRC16 |= ( USHORT ) ( pxRTUHdl->ubRTUFrameBuffer[pxRTUHdl->usRcvBufferPos - 2] << 8U );
#endif
                eStatus = MB_ENOERR;
            }
            else
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_RTU ) )
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                    vMBMLogRTUFrame( MB_LOG_WARN, pxIntHdl, "Received frame with incorrect checksum or length: ",
                                     ( const UBYTE * )&pxRTUHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF], pxRTUHdl->usRcvBufferPos );
#else
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_RTU,
                                 "[IDX=" MBP_FORMAT_USHORT "] Received frame with incorrect checksum or length "
                                 MBP_FORMAT_USHORT "!\n", ( USHORT ) pxRTUHdl->ubIdx, pxRTUHdl->usRcvBufferPos );
#endif
                }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                pxIntHdl->xFrameStat.ulNChecksumErrors += 1;
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                xAnalyzerFrame.eFrameType = MB_FRAME_DAMAGED;
#endif
                eStatus = MB_EIO;
            }

#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
            xAnalyzerFrame.eFrameDir = MB_FRAME_RECEIVE;
            xAnalyzerFrame.ubDataPayload = ( const UBYTE * )&pxRTUHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF];
            xAnalyzerFrame.usDataPayloadLength = pxRTUHdl->usRcvBufferPos;
            if( NULL != pxIntHdl->pvMBAnalyzerCallbackFN )
            {
                vMBPGetTimeStamp( &xTimeStamp );
                pxIntHdl->pvMBAnalyzerCallbackFN( pxIntHdl, pxIntHdl->pvCtx, &xTimeStamp, &xAnalyzerFrame );
            }
#endif
            MBP_ENTER_CRITICAL_SECTION(  );
            if( MB_ENOERR != ( eStatus2 = eMBPSerialTxEnable( pxRTUHdl->xSerHdl, NULL ) ) )
            {
                eStatus = eStatus2;
            }
            if( MB_ENOERR != ( eStatus2 = eMBPSerialRxEnable( pxRTUHdl->xSerHdl, NULL ) ) )
            {
                eStatus = eStatus2;
            }
#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 0
            if( MB_ENOERR != ( eStatus2 = eMBPTimerStop( pxRTUHdl->xTmrHdl ) ) )
            {
                eStatus = eStatus2;
            }
#endif
#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
            if( MB_ENOERR != ( eStatus2 = eMBPTimerStop( pxRTUHdl->xTmrWaitHdl ) ) )
            {
                eStatus = eStatus2;
            }
#endif
            HDL_RESET_TX( pxRTUHdl );
            HDL_RESET_RX( pxRTUHdl );
            MBP_EXIT_CRITICAL_SECTION(  );
        }
    }

    return eStatus;
}

STATIC          eMBErrorCode
eMBMSerialRTUFrameClose( xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) )
#else
    if( TRUE )
#endif
    {
        eStatus = eMBMSerialRTUFrameCloseInternal( ( xMBMRTUFrameHandle * ) pxIntHdl->xFrameHdl );
    }
    return eStatus;
}

STATIC          eMBErrorCode
eMBMSerialRTUFrameCloseInternal( xMBMRTUFrameHandle * pxRTUHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;

    MBP_ENTER_CRITICAL_SECTION(  );
#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( MB_IS_VALID_HDL( pxRTUHdl, xMBMRTUFrameHdl ) )
#else
    if( TRUE )
#endif
    {
        eStatus = MB_ENOERR;
        if( MBP_SERIALHDL_INVALID != pxRTUHdl->xSerHdl )
        {
            if( MB_ENOERR != eMBPSerialClose( pxRTUHdl->xSerHdl ) )
            {
                eStatus = MB_EPORTERR;
            }
            else
            {
#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 0
                if( MBP_TIMERHDL_INVALID != pxRTUHdl->xTmrHdl )
                {
                    vMBPTimerClose( pxRTUHdl->xTmrHdl );
                }
#endif
#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
                if( MBP_TIMERHDL_INVALID != pxRTUHdl->xTmrWaitHdl )
                {
                    vMBPTimerClose( pxRTUHdl->xTmrWaitHdl );
                }
#endif
                HDL_RESET( pxRTUHdl );
                eStatus = MB_ENOERR;
            }
        }
        else
        {
#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 0
            MBP_ASSERT( MBP_TIMERHDL_INVALID == pxRTUHdl->xTmrHdl );
#endif
#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
            MBP_ASSERT( MBP_TIMERHDL_INVALID == pxRTUHdl->xTmrWaitHdl );
#endif
            HDL_RESET( pxRTUHdl );
            eStatus = MB_ENOERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

#if MBM_SERIAL_API_VERSION == 1
STATIC void
bMBMSerialRTUReceiverAPIV1CB( xMBHandle xHdl, UBYTE ubValue )
{
    eMBErrorCode    eStatus;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMRTUFrameHandle *pxRTUFrameHdl;

    ( void )eStatus;
    MBP_ENTER_CRITICAL_SECTION(  );
    pxRTUFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxRTUFrameHdl->eSndState == MBM_STATE_TX_IDLE );

    switch ( pxRTUFrameHdl->eRcvState )
    {
    case MBM_STATE_RX_IDLE:
#if defined(__18CXX)
        pxRTUFrameHdl->usRcvBufferPos &= 0;
#else
        pxRTUFrameHdl->usRcvBufferPos = 0;
#endif
        pxRTUFrameHdl->ubRTUFrameBuffer[pxRTUFrameHdl->usRcvBufferPos] = ubValue;
        pxRTUFrameHdl->usRcvBufferPos++;
        pxRTUFrameHdl->eRcvState = MBM_STATE_RX_RCV;

        break;

    case MBM_STATE_RX_RCV:
        if( pxRTUFrameHdl->usRcvBufferPos < MBM_SER_PDU_SIZE_MAX )
        {
            pxRTUFrameHdl->ubRTUFrameBuffer[pxRTUFrameHdl->usRcvBufferPos] = ubValue;
            pxRTUFrameHdl->usRcvBufferPos++;
        }
        else
        {
            pxRTUFrameHdl->eRcvState = MBM_STATE_RX_ERROR;
        }
        break;

    default:
    case MBM_STATE_RX_ERROR:
        pxRTUFrameHdl->eRcvState = MBM_STATE_RX_ERROR;
        break;
    }

#if MBM_ENABLE_STATISTICS_INTERFACE == 1
    pxIntHdl->xFrameStat.ulNBytesReceived += 1;
#endif

#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 0
#if MBM_TEST_DISABLE_RTU_TIMEOUTS != 1
    if( MB_ENOERR != eMBPTimerStart( pxRTUFrameHdl->xTmrHdl ) )
    {
        /* We can only abort here because or timers failed. */
        eStatus = eMBPSerialRxEnable( pxRTUFrameHdl->xSerHdl, NULL );
        MBP_ASSERT( MB_ENOERR == eStatus );
        eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECV_ERROR );
        MBP_ASSERT( MB_ENOERR == eStatus );
        pxRTUFrameHdl->eRcvState = MBM_STATE_RX_ERROR;
    }
#endif
#endif
    MBP_EXIT_CRITICAL_SECTION(  );
}


STATIC          BOOL
bMBMSerialRTUTransmitterEmptyAPIV1CB( xMBHandle xHdl, UBYTE * pubValue )
{
    eMBErrorCode    eStatus;
    BOOL            bMoreTXData = FALSE;
    BOOL            bEnableRx = FALSE;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMRTUFrameHandle *pxRTUFrameHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    pxRTUFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxRTUFrameHdl->eRcvState == MBM_STATE_RX_IDLE );
    MBP_ASSERT( pxRTUFrameHdl->eSndState == MBM_STATE_TX_XMIT );
    MBP_ASSERT( pxRTUFrameHdl->pubSndBufferCur != NULL );

    switch ( pxRTUFrameHdl->eSndState )
    {
        /* We send characters until all outstanding bytes in the send buffer
         * has been delivered. If all characters have been sent and we sent
         * to a broadcast we are done. This is handled by delivering an
         * MBM_EV_SENT event to the main state machine. Otherwise we enable
         * the receiver or abort using an error.
         */
    case MBM_STATE_TX_XMIT:
        if( pxRTUFrameHdl->usSndBufferCnt > 0 )
        {
            *pubValue = *( pxRTUFrameHdl->pubSndBufferCur );
            pxRTUFrameHdl->pubSndBufferCur++;
            pxRTUFrameHdl->usSndBufferCnt--;
            bMoreTXData = TRUE;
        }
        else
        {
            if( MB_SER_BROADCAST_ADDR == pxRTUFrameHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF] )
            {
                eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SENT );
                MBP_ASSERT( MB_ENOERR == eStatus );
            }
            else
            {
                bEnableRx = TRUE;
            }
        }
        break;

        /* Default case which aborts the transmitter. */
    default:
        eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SEND_ERROR );
        MBP_ASSERT( MB_ENOERR == eStatus );
        break;
    }
    if( !bMoreTXData )
    {
        HDL_RESET_TX( pxRTUFrameHdl );
    }
    else
    {
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
        pxIntHdl->xFrameStat.ulNBytesSent += 1;
#endif
    }
    if( bEnableRx )
    {
#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
        if( MB_ENOERR != eMBPTimerStart( pxRTUFrameHdl->xTmrWaitHdl ) )
        {
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SEND_ERROR );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
#else
        if( MB_ENOERR != eMBPSerialRxEnable( pxRTUFrameHdl->xSerHdl, ( pvMBPSerialReceiverCB ) bMBMSerialRTUReceiverAPIV1CB ) )
        {
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SEND_ERROR );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
#endif
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return bMoreTXData;
}
#endif

#if MBM_SERIAL_API_VERSION == 2
STATIC          BOOL
bMBMSerialRTUTransmitterEmptyAPIV2CB( xMBHandle xHdl, UBYTE * pubBufferOut, USHORT usBufferMax, USHORT * pusBufferWritten )
{
    eMBErrorCode    eStatus;
    USHORT          usBytesToSend;
    BOOL            bMoreTXData = FALSE;
    BOOL            bEnableRx = FALSE;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMRTUFrameHandle *pxRTUFrameHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    pxRTUFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxRTUFrameHdl->eRcvState == MBM_STATE_RX_IDLE );
    MBP_ASSERT( pxRTUFrameHdl->eSndState == MBM_STATE_TX_XMIT );
    MBP_ASSERT( pxRTUFrameHdl->pubSndBufferCur != NULL );

    switch ( pxRTUFrameHdl->eSndState )
    {
        /* We send characters until all outstanding bytes in the send buffer
         * has been delivered. If all characters have been sent and we sent
         * to a broadcast we are done. This is handled by delivering an
         * MBM_EV_SENT event to the main state machine. Otherwise we enable
         * the receiver or abort using an error.
         */
    case MBM_STATE_TX_XMIT:
        if( pxRTUFrameHdl->usSndBufferCnt > 0 )
        {
            usBytesToSend = pxRTUFrameHdl->usSndBufferCnt < usBufferMax ? pxRTUFrameHdl->usSndBufferCnt : usBufferMax;
            memcpy( pubBufferOut, pxRTUFrameHdl->pubSndBufferCur, ( size_t ) usBytesToSend );
            pxRTUFrameHdl->pubSndBufferCur += usBytesToSend;
            pxRTUFrameHdl->usSndBufferCnt -= usBytesToSend;
            *pusBufferWritten = usBytesToSend;
            bMoreTXData = TRUE;
        }
        else
        {
            if( MB_SER_BROADCAST_ADDR == pxRTUFrameHdl->ubRTUFrameBuffer[MBM_SER_PDU_ADDR_OFF] )
            {
                eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SENT );
                MBP_ASSERT( MB_ENOERR == eStatus );
            }
            else
            {
                bEnableRx = TRUE;
            }
        }
        break;

        /* Default case which aborts the transmitter. */
    default:
        eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SEND_ERROR );
        MBP_ASSERT( MB_ENOERR == eStatus );
        break;
    }
    if( !bMoreTXData )
    {
        HDL_RESET_TX( pxRTUFrameHdl );
    }
    else
    {
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
        pxIntHdl->xFrameStat.ulNBytesSent += *pusBufferWritten;
#endif
    }
    if( bEnableRx )
    {
#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
        if( MB_ENOERR != eMBPTimerStart( pxRTUFrameHdl->xTmrWaitHdl ) )
        {
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SEND_ERROR );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
#else
        if( MB_ENOERR != eMBPSerialRxEnable( pxRTUFrameHdl->xSerHdl, ( pvMBPSerialReceiverCB ) vMBMSerialRTUReceiverAPIV2CB ) )
        {
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SEND_ERROR );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
#endif
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return bMoreTXData;
}

STATIC void
vMBMSerialRTUReceiverAPIV2CB( xMBHandle xHdl, const UBYTE * pubBufferIn, USHORT usBufferLen )
{
    eMBErrorCode    eStatus;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMRTUFrameHandle *pxRTUFrameHdl;

    ( void )eStatus;
    MBP_ENTER_CRITICAL_SECTION(  );
    pxRTUFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxRTUFrameHdl->eSndState == MBM_STATE_TX_IDLE );    
    switch ( pxRTUFrameHdl->eRcvState )
    {
    case MBM_STATE_RX_IDLE:
        if( usBufferLen < MBM_SER_PDU_SIZE_MAX )
        {
            memcpy( ( void * )&pxRTUFrameHdl->ubRTUFrameBuffer[0], pubBufferIn, ( size_t ) usBufferLen );
            pxRTUFrameHdl->usRcvBufferPos = usBufferLen;
            pxRTUFrameHdl->eRcvState = MBM_STATE_RX_RCV;
        }
        else
        {
            pxRTUFrameHdl->eRcvState = MBM_STATE_RX_ERROR;
        }
        break;

    case MBM_STATE_RX_RCV:
        if( ( pxRTUFrameHdl->usRcvBufferPos + usBufferLen ) < MBM_SER_PDU_SIZE_MAX )
        {
            memcpy( ( void * )&( pxRTUFrameHdl->ubRTUFrameBuffer[pxRTUFrameHdl->usRcvBufferPos] ), pubBufferIn, ( size_t ) usBufferLen );
            pxRTUFrameHdl->usRcvBufferPos += usBufferLen;
        }
        else
        {
            pxRTUFrameHdl->eRcvState = MBM_STATE_RX_ERROR;
        }
        break;

    default:
    case MBM_STATE_RX_ERROR:
        pxRTUFrameHdl->eRcvState = MBM_STATE_RX_ERROR;
        break;
    }
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
    pxIntHdl->xFrameStat.ulNBytesReceived += usBufferLen;
#endif

#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 0
#if MBM_TEST_DISABLE_RTU_TIMEOUTS != 1
    if( MB_ENOERR != eMBPTimerStart( pxRTUFrameHdl->xTmrHdl ) )
    {
        /* We can only abort here because or timers failed. */
        eStatus = eMBPSerialRxEnable( pxRTUFrameHdl->xSerHdl, NULL );
        MBP_ASSERT( MB_ENOERR == eStatus );
        eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECV_ERROR );
        MBP_ASSERT( MB_ENOERR == eStatus );
        pxRTUFrameHdl->eRcvState = MBM_STATE_RX_ERROR;
    }
#endif
#endif
    MBP_EXIT_CRITICAL_SECTION(  );
}
#endif

#if MBM_TEST_INSTANCES == 0
STATIC
#endif
    BOOL
bMBMSerialRTUT35CB( xMBHandle xHdl )
{
    eMBErrorCode    eStatus;
    BOOL            bNeedCtxSwitch = TRUE;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMRTUFrameHandle *pxRTUFrameHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    pxRTUFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxRTUFrameHdl->eSndState == MBM_STATE_TX_IDLE );

    switch ( pxRTUFrameHdl->eRcvState )
    {
    case MBM_STATE_RX_RCV:
        eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECEIVED );
        MBP_ASSERT( MB_ENOERR == eStatus );
        break;

    default:
        eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECV_ERROR );
        MBP_ASSERT( MB_ENOERR == eStatus );
    }
    pxRTUFrameHdl->eRcvState = MBM_STATE_RX_IDLE;
    /* Disable the receive and the timers after a timeout. */
    eStatus = eMBPSerialRxEnable( pxRTUFrameHdl->xSerHdl, NULL );
    MBP_ASSERT( MB_ENOERR == eStatus );
#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 0
    eStatus = eMBPTimerStop( pxRTUFrameHdl->xTmrHdl );
    MBP_ASSERT( MB_ENOERR == eStatus );
#endif
    MBP_EXIT_CRITICAL_SECTION(  );
    return bNeedCtxSwitch;
}

#if MBM_RTU_WAITAFTERSEND_ENABLED == 1
STATIC          BOOL
bMBMSerialWaitCB( xMBHandle xHdl )
{
    eMBErrorCode    eStatus;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMRTUFrameHandle *pxRTUFrameHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    pxRTUFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxRTUFrameHdl->eSndState == MBM_STATE_TX_IDLE );
#if MBM_SERIAL_API_VERSION == 2
    if( MB_ENOERR != eMBPSerialRxEnable( pxRTUFrameHdl->xSerHdl, ( pvMBPSerialReceiverCB ) vMBMSerialRTUReceiverAPIV2CB ) )
#elif MBM_SERIAL_API_VERSION == 1
    if( MB_ENOERR != eMBPSerialRxEnable( pxRTUFrameHdl->xSerHdl, ( pvMBPSerialReceiverCB ) bMBMSerialRTUReceiverAPIV1CB ) )
#endif
    {
        eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SEND_ERROR );
        MBP_ASSERT( MB_ENOERR == eStatus );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return FALSE;
}
#endif

#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
void
vMBMLogRTUFrame( eMBPortLogLevel eLevel, xMBMInternalHandle * pxIntHdl, char *szMsg, const UBYTE * pubPayload, USHORT usLength )
{
    USHORT          usIdx;
    CHAR            arubBuffer[MBM_SER_PDU_SIZE_MAX * 2U + 1];
    xMBMRTUFrameHandle *pxFrameHdl = pxIntHdl->xFrameHdl;

    MBP_ASSERT( usLength < MBM_SER_PDU_SIZE_MAX );
    if( usLength > 0 )
    {
        for( usIdx = 0; usIdx < usLength; usIdx++ )
        {
            sprintf( &arubBuffer[usIdx * 2], MBP_FORMAT_UINT_AS_HEXBYTE, ( unsigned int )pubPayload[usIdx] );
        }
    }
    else
    {
        strcpy( arubBuffer, "empty" );
    }
    vMBPPortLog( eLevel, MB_LOG_RTU, "[IDX=" MBP_FORMAT_USHORT "] %s%s\n", ( USHORT ) pxFrameHdl->ubIdx, szMsg, arubBuffer );
}
#endif

#endif
