/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008-2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmascii.c,v 1.36 2013-05-21 21:04:08 embedded-solutions.cwalter Exp $
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
#include "mbmascii.h"

#if ( defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 ) )
#include <stdio.h>
#endif

/*lint --e{717} ~ suppress messages: do .. while(0); */

#if MBM_ASCII_ENABLED == 1

/* ----------------------- Defines ------------------------------------------*/
#define MBM_SER_PDU_SIZE_MIN        ( 4 )       /*!< Minimum size of a MODBUS ASCII frame. */
#ifndef MBM_SER_PDU_SIZE_MAX
#define MBM_SER_PDU_SIZE_MAX        ( 255 )     /*!< Maximum size of a MODBUS ASCII frame. */
#endif
#define MBM_SER_PDU_SIZE_LRC        ( 1 )       /*!< Size of LRC checksum. */
#define MBM_SER_PDU_ADDR_OFF        ( 0 )       /*!< Offset of slave address in ASCII frame. */
#define MBM_SER_PDU_PDU_OFF         ( 1 )       /*!< Offset of Modbus-PDU in ASCII frame. */

#define MBM_ASCII_DEFAULT_CR        ( 0x0D )    /*!< Default CR character for MODBUS ASCII. */
#define MBM_ASCII_DEFAULT_LF        ( 0x0A )    /*!< Default LF character for MODBUS ASCII. */
#define MBM_ASCII_START             ( 0x3A )    /*!< Start character ':' for MODBUS ASCII. */
#define IDX_INVALID                 ( 255 )

#define HDL_RESET_RX( x ) do { \
    ( x )->eRcvState = STATE_RX_IDLE; \
    ( x )->usRcvBufferPos = 0; \
    ( x )->eBytePos = BYTE_HIGH_NIBBLE; \
} while( 0 );

#define HDL_RESET_TX( x ) do { \
    ( x )->eSndState = STATE_TX_IDLE; \
    ( x )->usSndBufferCnt = 0; \
    ( x )->eBytePos = BYTE_HIGH_NIBBLE; \
    ( x )->pubSndBufferCur = NULL; \
} while( 0 );

#define HDL_RESET_BASIC( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    HDL_RESET_RX( x ); \
    HDL_RESET_TX( x ); \
    ( x )->xTmrHdl = MBP_TIMERHDL_INVALID; \
    ( x )->xSerHdl = MBP_SERIALHDL_INVALID; \
    memset( ( void * )&( ( x )->ubASCIIFrameBuffer[ 0 ] ), 0, MBM_SER_PDU_SIZE_MAX ); \
} while( 0 );

#if MBM_ASCII_WAITAFTERSEND_ENABLED > 0
#define HDL_RESET_WAITAFTERSEND( x ) do { \
	( x )->xWaitTmrHdl = MBP_TIMERHDL_INVALID; \
} while( 0 );
#else
#define HDL_RESET_WAITAFTERSEND( x )
#endif

#if MBM_ASCII_BACKOF_TIME_MS > 0
#define HDL_RESET_BACKOF( x ) do { \
	( x )->xBackOffTmrHdl = MBP_TIMERHDL_INVALID; \
} while( 0 );
#else
#define HDL_RESET_BACKOF( x )
#endif

#define HDL_RESET( x ) do { \
     HDL_RESET_BASIC( x ); \
	 HDL_RESET_WAITAFTERSEND( x ); \
	 HDL_RESET_BACKOF( x ); \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_RX_RCV,               /*!< Frame is beeing received. */
    STATE_RX_WAIT_EOF,          /*!< Wait for End of Frame. */
    STATE_RX_ERROR              /*!< Error during receive. */
} eMBMASCIIRcvState;

typedef enum
{
    STATE_TX_IDLE,              /*!< Transmitter is in idle state. */
    STATE_TX_START,             /*!< Starting transmission (':' sent). */
    STATE_TX_DATA,              /*!< Sending of data (Address, Data, LRC). */
    STATE_TX_END,               /*!< End of transmission. */
    STATE_TX_NOTIFY             /*!< Notify sender that the frame has been sent. */
} eMBMASCIISndState;

typedef enum
{
    BYTE_HIGH_NIBBLE,           /*!< Character for high nibble of byte. */
    BYTE_LOW_NIBBLE             /*!< Character for low nibble of byte. */
} eMBMASCIIBytePos;

typedef struct
{
    UBYTE           ubIdx;
    volatile UBYTE  ubASCIIFrameBuffer[MBM_SER_PDU_SIZE_MAX];
    eMBMASCIIBytePos eBytePos;
    volatile eMBMASCIIRcvState eRcvState;
    volatile USHORT usRcvBufferPos;

    volatile eMBMASCIISndState eSndState;
    volatile USHORT usSndBufferCnt;
    UBYTE          *pubSndBufferCur;

    xMBPTimerHandle xTmrHdl;
#if MBM_ASCII_BACKOF_TIME_MS > 0
    xMBPTimerHandle xBackOffTmrHdl;
#endif
#if MBM_ASCII_WAITAFTERSEND_ENABLED > 0
    xMBPTimerHandle xWaitTmrHdl;
#endif
    xMBPSerialHandle xSerHdl;
} xMBMASCIIFrameHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;
STATIC xMBMASCIIFrameHandle xMBMASCIIFrameHdl[MBM_SERIAL_ASCII_MAX_INSTANCES];

/* ----------------------- Static functions ---------------------------------*/
STATIC UBYTE    ubMBMSerialASCIIBIN2CHAR( UBYTE ubByte );
STATIC UBYTE    ubMBMSerialASCIICHAR2BIN( UBYTE ubCharacter );
STATIC UBYTE    ubMBMSerialASCIILRC( const UBYTE * pubFrame, USHORT usLen );

#if MBM_ASCII_BACKOF_TIME_MS > 0
STATIC BOOL     bMBMSerialASCIIBackOffTimerCB( xMBHandle xHdl );
#endif
#if MBM_ASCII_WAITAFTERSEND_ENABLED > 0
STATIC BOOL     bMBMSerialWaitCB( xMBHandle xHdl );
#endif

#if MBM_TEST_INSTANCES == 0
STATIC
#endif
    BOOL bMBMSerialASCIITimerCB( xMBHandle xHdl );

#if MBM_SERIAL_API_VERSION == 1
STATIC void     vMBMSerialASCIIReceiverAPIV1CB( xMBHandle xHdl, UBYTE ubValue );

STATIC BOOL     bMBMSerialASCIITransmitterEmptyAPIV1CB( xMBHandle xHdl, UBYTE * ubValue );
#elif MBM_SERIAL_API_VERSION == 2
STATIC BOOL     bMBMSerialASCIITransmitterEmptyAPIV2CB( xMBHandle xHdl, UBYTE * pubBufferOut, USHORT usBufferMax, USHORT * pusBufferWritten );
STATIC void     vMBMSerialASCIIReceiverAPIV2CB( xMBHandle xHdl, const UBYTE * pubBufferIn, USHORT usBufferLen );
#endif

STATIC eMBErrorCode eMBMSerialASCIIFrameSend( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength );
STATIC eMBErrorCode eMBMSerialASCIIFrameReceive( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength );
STATIC eMBErrorCode eMBMSerialASCIIFrameClose( xMBHandle xHdl );
STATIC eMBErrorCode eMBMSerialASCIIFrameCloseInternal( xMBMASCIIFrameHandle * pxASCIIHdl );
STATIC BOOL     eMBMSerialASCIIFrameIsTransmitting( xMBHandle xHdl );

#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
void            vMBMLogASCIIFrame( eMBPortLogLevel eLevel, xMBMInternalHandle * pxIntHdl, char *szMsg, const UBYTE * pubPayload, USHORT usLength );
#endif

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBMSerialASCIIInit( xMBMInternalHandle * pxIntHdl, UCHAR ucPort, ULONG ulBaudRate, eMBSerialParity eParity, UCHAR ucStopBits )
{
    eMBErrorCode    eStatus = MB_ENOERR, eStatus2;
    xMBMASCIIFrameHandle *pxFrameHdl = NULL;
    UBYTE           ubIdx;
    USHORT          usTimeoutMS;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( ( NULL != pxIntHdl ) && ( ulBaudRate > 0 ) )
#else
    if( TRUE )
#endif
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !bIsInitialized )
        {
            for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBMASCIIFrameHdl ); ubIdx++ )
            {
                HDL_RESET( &xMBMASCIIFrameHdl[ubIdx] );
            }
            bIsInitialized = TRUE;
        }

        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBMASCIIFrameHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xMBMASCIIFrameHdl[ubIdx].ubIdx )
            {
                pxFrameHdl = &xMBMASCIIFrameHdl[ubIdx];
                pxFrameHdl->ubIdx = ubIdx;
                break;
            }
        }

        if( NULL != pxFrameHdl )
        {
            usTimeoutMS = ( USHORT ) ( MBM_ASCII_TIMEOUT_SEC * 1000U );

            if( MB_ENOERR != ( eStatus2 = eMBPSerialInit( &( pxFrameHdl->xSerHdl ), ucPort, ulBaudRate, 7, eParity, ucStopBits, pxIntHdl
#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 1
                , NULL, MB_ASCII 
#endif                
                ) ) )
            {
                eStatus = eStatus2;
            }
            else if( MB_ENOERR != ( eStatus2 = eMBPTimerInit( &( pxFrameHdl->xTmrHdl ), usTimeoutMS, bMBMSerialASCIITimerCB, pxIntHdl ) ) )
            {
                eStatus = eStatus2;
            }
#if MBM_ASCII_BACKOF_TIME_MS > 0
            else if( MB_ENOERR != ( eStatus2 = eMBPTimerInit( &( pxFrameHdl->xBackOffTmrHdl ), MBM_ASCII_BACKOF_TIME_MS,
                                                              bMBMSerialASCIIBackOffTimerCB, pxIntHdl ) ) )
            {
                eStatus = eStatus2;
            }
#endif
#if MBM_ASCII_WAITAFTERSEND_ENABLED > 0
            else if( MB_ENOERR !=
                     ( eStatus2 =
                       eMBPTimerInit( &( pxFrameHdl->xWaitTmrHdl ), MBM_SERIAL_ASCII_DYNAMIC_WAITAFTERSEND_TIMEOUT_MS( ulBaudRate ), bMBMSerialWaitCB,
                                      pxIntHdl ) ) )
            {
                eStatus = eStatus2;
            }
#endif
            else
            {
                /* Attach the frame handle to the protocol stack. */
                pxIntHdl->pubFrameMBPDUBuffer = ( UBYTE * ) & pxFrameHdl->ubASCIIFrameBuffer[MBM_SER_PDU_PDU_OFF];
                pxIntHdl->xFrameHdl = pxFrameHdl;
                pxIntHdl->pFrameSendFN = eMBMSerialASCIIFrameSend;
                pxIntHdl->pFrameRecvFN = eMBMSerialASCIIFrameReceive;
                pxIntHdl->pFrameCloseFN = eMBMSerialASCIIFrameClose;
#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
                pxIntHdl->pFrameIsTransmittingFN = eMBMSerialASCIIFrameIsTransmitting;
#endif
                eStatus = MB_ENOERR;
            }

#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_ASCII ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_ASCII,
                             "[IDX=" MBP_FORMAT_USHORT "] Creation of new ASCII instance (port=" MBP_FORMAT_USHORT
                             ", baudrate=%lu, parity=" MBP_FORMAT_USHORT "): %s.\n", ( USHORT ) pxFrameHdl->ubIdx,
                             ( USHORT ) ucPort, ulBaudRate, ( USHORT ) eParity, eStatus == MB_ENOERR ? "okay" : "failed" );
            }
#endif

            if( MB_ENOERR != eStatus )
            {
                ( void )eMBMSerialASCIIFrameCloseInternal( pxFrameHdl );
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

STATIC          BOOL
eMBMSerialASCIIFrameIsTransmitting( xMBHandle xHdl )
{
    BOOL            bIsTransmitting = FALSE;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMASCIIFrameHandle *pxASCIIHdl;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) )
#else
    if( TRUE )
#endif
    {
        pxASCIIHdl = pxIntHdl->xFrameHdl;
        if( pxASCIIHdl->eSndState != STATE_TX_IDLE )
        {
            bIsTransmitting = TRUE;
        }
    }
    return bIsTransmitting;
}

STATIC          eMBErrorCode
eMBMSerialASCIIFrameSend( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMASCIIFrameHandle *pxASCIIHdl;
    UBYTE           ubLRC;

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
        pxASCIIHdl = pxIntHdl->xFrameHdl;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_ASCII ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_ASCII,
                         "[IDX=" MBP_FORMAT_USHORT "] Sending new frame for slave=" MBP_FORMAT_USHORT " with length="
                         MBP_FORMAT_USHORT ".\n", ( USHORT ) pxASCIIHdl->ubIdx, ( USHORT ) ucSlaveAddress, ( USHORT ) usMBPDULength );
        }
#endif
        if( MB_IS_VALID_HDL( pxASCIIHdl, xMBMASCIIFrameHdl ) &&
            MB_IS_VALID_WRITE_ADDR( ucSlaveAddress ) && ( usMBPDULength <= ( MBM_SER_PDU_SIZE_MAX - ( 1 /* Slave Address */  + 1 /* LRC */  ) ) ) )
        {
            MBP_ENTER_CRITICAL_SECTION(  );
            MBP_ASSERT( STATE_TX_IDLE == pxASCIIHdl->eSndState );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_ASCII ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_ASCII,
                             "[IDX=" MBP_FORMAT_USHORT "] Sender state check: receiver ( state = " MBP_FORMAT_USHORT
                             ", pos = " MBP_FORMAT_USHORT " ), sender ( state = " MBP_FORMAT_USHORT ", cnt = "
                             MBP_FORMAT_USHORT " ).\n", ( USHORT ) pxASCIIHdl->ubIdx, ( USHORT ) pxASCIIHdl->eRcvState,
                             pxASCIIHdl->usRcvBufferPos, ( USHORT ) pxASCIIHdl->eSndState, pxASCIIHdl->usSndBufferCnt );
            }
#endif
            MBP_EXIT_CRITICAL_SECTION(  );

            /* Added the MODBUS ASCII header (= slave address) */
            pxASCIIHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF] = ( UBYTE ) ucSlaveAddress;
            pxASCIIHdl->usSndBufferCnt = 1;

            /* MODBUS PDU is already embedded in the frame. */
            pxASCIIHdl->usSndBufferCnt += usMBPDULength;

            ubLRC = ubMBMSerialASCIILRC( ( UBYTE * ) & ( pxASCIIHdl->ubASCIIFrameBuffer[0] ), pxASCIIHdl->usSndBufferCnt );
            pxASCIIHdl->ubASCIIFrameBuffer[pxASCIIHdl->usSndBufferCnt] = ubLRC;
            pxASCIIHdl->usSndBufferCnt++;

            /* Enable transmitter */
            MBP_ENTER_CRITICAL_SECTION(  );
            pxASCIIHdl->eSndState = STATE_TX_START;
            pxASCIIHdl->pubSndBufferCur = ( UBYTE * ) & ( pxASCIIHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF] );
#if MBM_SERIAL_API_VERSION == 1
            if( MB_ENOERR != ( eStatus = eMBPSerialTxEnable( pxASCIIHdl->xSerHdl, ( pbMBPSerialTransmitterEmptyCB ) bMBMSerialASCIITransmitterEmptyAPIV1CB ) ) )
            {
                HDL_RESET_TX( pxASCIIHdl );
            }
#elif MBM_SERIAL_API_VERSION == 2
            if( MB_ENOERR != ( eStatus = eMBPSerialTxEnable( pxASCIIHdl->xSerHdl, ( pbMBPSerialTransmitterEmptyCB ) bMBMSerialASCIITransmitterEmptyAPIV2CB ) ) )
            {
                HDL_RESET_TX( pxASCIIHdl );
            }
#else
#error "Define either MBM_SERIAL_API_VERSION=1 or 2!"
#endif
            else
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_ASCII ) )
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                    vMBMLogASCIIFrame( MB_LOG_DEBUG, pxIntHdl, "Sending frame: ",
                                       ( const UBYTE * )&pxASCIIHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF], pxASCIIHdl->usSndBufferCnt );
#else
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_ASCII,
                                 "[IDX=" MBP_FORMAT_USHORT "] Sending frame with length " MBP_FORMAT_USHORT ".\n",
                                 ( USHORT ) pxASCIIHdl->ubIdx, pxASCIIHdl->usSndBufferCnt );
#endif
                }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                pxIntHdl->xFrameStat.ulNPacketsSent += 1;
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                /* Pass frame to protocol analyzer. */
                xAnalyzerFrame.eFrameDir = MB_FRAME_SEND;
                xAnalyzerFrame.eFrameType = MB_FRAME_ASCII;
                xAnalyzerFrame.x.xASCIIHeader.ubSlaveAddress = ucSlaveAddress;
                xAnalyzerFrame.x.xASCIIHeader.ubLRC = ubLRC;
                xAnalyzerFrame.ubDataPayload = ( const UBYTE * )&pxASCIIHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF];
                xAnalyzerFrame.usDataPayloadLength = pxASCIIHdl->usSndBufferCnt;
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
eMBMSerialASCIIFrameReceive( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength )
{
    eMBErrorCode    eStatus = MB_EINVAL, eStatus2;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMASCIIFrameHandle *pxASCIIHdl;
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
        pxASCIIHdl = pxIntHdl->xFrameHdl;
        if( MB_IS_VALID_HDL( pxASCIIHdl, xMBMASCIIFrameHdl ) && MB_IS_VALID_READ_ADDR( ucSlaveAddress ) )
        {

            if( NULL == pusMBPDULength )
            {
                /* This function might be called because of a timeout. Timers and
                 * receiver/transmitters are disabled at the end of this function.
                 */
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_ASCII ) )
                {
                    vMBPPortLog( MB_LOG_WARN, MB_LOG_ASCII,
                                 "[IDX=" MBP_FORMAT_USHORT "] Receiver aborted due to error or timeout!\n", ( USHORT ) pxASCIIHdl->ubIdx );
                }
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                xAnalyzerFrame.eFrameType = MB_FRAME_DAMAGED;
#endif
            }
            else if( ( pxASCIIHdl->usRcvBufferPos >= MBM_SER_PDU_SIZE_MIN ) &&
                     ( ucSlaveAddress == pxASCIIHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF] ) &&
                     ( ubMBMSerialASCIILRC( ( UBYTE * ) & ( pxASCIIHdl->ubASCIIFrameBuffer[0] ), pxASCIIHdl->usRcvBufferPos ) == 0 ) )
            {
                *pusMBPDULength = ( USHORT ) ( pxASCIIHdl->usRcvBufferPos - ( MBM_SER_PDU_PDU_OFF + MBM_SER_PDU_SIZE_LRC ) );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_ASCII ) )
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                    vMBMLogASCIIFrame( MB_LOG_DEBUG, pxIntHdl, "Received frame: ",
                                       ( const UBYTE * )&pxASCIIHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF], pxASCIIHdl->usRcvBufferPos );
#else
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_ASCII,
                                 "[IDX=" MBP_FORMAT_USHORT "] Received correct frame with length " MBP_FORMAT_USHORT
                                 "!\n", ( USHORT ) pxASCIIHdl->ubIdx, pxASCIIHdl->usRcvBufferPos );
#endif
                }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                pxIntHdl->xFrameStat.ulNPacketsReceived += 1;
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                xAnalyzerFrame.eFrameType = MB_FRAME_ASCII;
                xAnalyzerFrame.x.xASCIIHeader.ubSlaveAddress = ucSlaveAddress;
                xAnalyzerFrame.x.xASCIIHeader.ubLRC = pxASCIIHdl->ubASCIIFrameBuffer[pxASCIIHdl->usRcvBufferPos - 1];
#endif
                eStatus = MB_ENOERR;
            }
            else
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_ASCII ) )
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                    vMBMLogASCIIFrame( MB_LOG_ASCII, pxIntHdl, "Received frame with incorrect checksum or length: ",
                                       ( const UBYTE * )&pxASCIIHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF], pxASCIIHdl->usRcvBufferPos );
#else
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_ASCII,
                                 "[IDX=" MBP_FORMAT_USHORT "] Received frame with incorrect checksum or length "
                                 MBP_FORMAT_USHORT "!\n", ( USHORT ) pxASCIIHdl->ubIdx, pxASCIIHdl->usRcvBufferPos );
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
            xAnalyzerFrame.ubDataPayload = ( const UBYTE * )&pxASCIIHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF];
            xAnalyzerFrame.usDataPayloadLength = pxASCIIHdl->usRcvBufferPos;
            if( NULL != pxIntHdl->pvMBAnalyzerCallbackFN )
            {
                vMBPGetTimeStamp( &xTimeStamp );
                pxIntHdl->pvMBAnalyzerCallbackFN( pxIntHdl, pxIntHdl->pvCtx, &xTimeStamp, &xAnalyzerFrame );
            }
#endif
            MBP_ENTER_CRITICAL_SECTION(  );
            if( MB_ENOERR != ( eStatus2 = eMBPSerialTxEnable( pxASCIIHdl->xSerHdl, NULL ) ) )
            {
                eStatus = eStatus2;
            }
            if( MB_ENOERR != ( eStatus2 = eMBPSerialRxEnable( pxASCIIHdl->xSerHdl, NULL ) ) )
            {
                eStatus = eStatus2;
            }
#if MBM_ASCII_BACKOF_TIME_MS == 1
            if( MB_ENOERR != ( eStatus2 = eMBPTimerStop( pxASCIIHdl->xBackOffTmrHdl ) ) )
            {
                eStatus = eStatus2;
            }
#endif
#if MBM_ASCII_WAITAFTERSEND_ENABLED == 1
            if( MB_ENOERR != ( eStatus2 = eMBPTimerStop( pxASCIIHdl->xWaitTmrHdl ) ) )
            {
                eStatus = eStatus2;
            }
#endif
            HDL_RESET_TX( pxASCIIHdl );
            HDL_RESET_RX( pxASCIIHdl );
            MBP_EXIT_CRITICAL_SECTION(  );
        }
    }

    return eStatus;
}

STATIC          eMBErrorCode
eMBMSerialASCIIFrameClose( xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( bMBMIsHdlValid( pxIntHdl ) )
#else
    if( TRUE )
#endif
    {
        eStatus = eMBMSerialASCIIFrameCloseInternal( ( xMBMASCIIFrameHandle * ) pxIntHdl->xFrameHdl );
    }
    return eStatus;
}

STATIC          eMBErrorCode
eMBMSerialASCIIFrameCloseInternal( xMBMASCIIFrameHandle * pxASCIIHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
    UBYTE           ubIdx;
#endif

    MBP_ENTER_CRITICAL_SECTION(  );
#if MBM_ENABLE_FULL_API_CHECKS == 1
    if( MB_IS_VALID_HDL( pxASCIIHdl, xMBMASCIIFrameHdl ) )
#else
    if( TRUE )
#endif
    {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_ASCII ) )
        {
            ubIdx = pxASCIIHdl->ubIdx;
        }
#endif
        if( MBP_SERIALHDL_INVALID != pxASCIIHdl->xSerHdl )
        {
            if( MB_ENOERR != eMBPSerialClose( pxASCIIHdl->xSerHdl ) )
            {
                eStatus = MB_EPORTERR;
            }
            else
            {
                if( MBP_TIMERHDL_INVALID != pxASCIIHdl->xTmrHdl )
                {
                    vMBPTimerClose( pxASCIIHdl->xTmrHdl );
                }
#if MBM_ASCII_BACKOF_TIME_MS > 0
                if( MBP_TIMERHDL_INVALID != pxASCIIHdl->xBackOffTmrHdl )
                {
                    vMBPTimerClose( pxASCIIHdl->xBackOffTmrHdl );
                }
#endif
#if MBM_ASCII_WAITAFTERSEND_ENABLED > 0
                if( MBP_TIMERHDL_INVALID != pxASCIIHdl->xWaitTmrHdl )
                {
                    vMBPTimerClose( pxASCIIHdl->xWaitTmrHdl );
                }
#endif
                HDL_RESET( pxASCIIHdl );
                eStatus = MB_ENOERR;

            }
        }
        else
        {
            MBP_ASSERT( MBP_TIMERHDL_INVALID == pxASCIIHdl->xTmrHdl );
#if MBM_ASCII_BACKOF_TIME_MS == 1
            MBP_ASSERT( MBP_TIMERHDL_INVALID == pxASCIIHdl->xBackOffTmrHdl );
#endif
#if MBM_ASCII_WAITAFTERSEND_ENABLED > 0
            MBP_ASSERT( MBP_TIMERHDL_INVALID == pxASCIIHdl->xWaitTmrHdl );
#endif
            HDL_RESET( pxASCIIHdl );
            eStatus = MB_ENOERR;
        }
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_ASCII ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_ASCII, "[IDX=" MBP_FORMAT_USHORT "] closed ASCII instance: %s.\n",
                         ( USHORT ) ubIdx, eStatus == MB_ENOERR ? "okay" : "failed" );
        }
#endif

    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

#if MBM_SERIAL_API_VERSION == 1
STATIC void
vMBMSerialASCIIReceiverAPIV1CB( xMBHandle xHdl, UBYTE ubValue )
{
    eMBErrorCode    eStatus;
    BOOL            bEnableTimer = TRUE;
    UBYTE           ubBinValue;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMASCIIFrameHandle *pxASCIIFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxASCIIFrameHdl->eSndState == STATE_TX_IDLE );

    switch ( pxASCIIFrameHdl->eRcvState )
    {

        /* A new character is received. If the character is a ':' the
         * input buffers are cleared. A CR character signals the end of the 
         * data block. Other characters are real data bytes.
         */
    case STATE_RX_RCV:
        if( MBM_ASCII_START == ubValue )
        {
            /* Empty receive buffer. */
            pxASCIIFrameHdl->eBytePos = BYTE_HIGH_NIBBLE;
            pxASCIIFrameHdl->usRcvBufferPos = 0;
        }
        else if( MBM_ASCII_DEFAULT_CR == ubValue )
        {
            pxASCIIFrameHdl->eRcvState = STATE_RX_WAIT_EOF;
        }
        else
        {
            ubBinValue = ubMBMSerialASCIICHAR2BIN( ubValue );
            switch ( pxASCIIFrameHdl->eBytePos )
            {
            case BYTE_HIGH_NIBBLE:
                /* High nibble comes first. We need to check for an overflow. */
                if( pxASCIIFrameHdl->usRcvBufferPos < MBM_SER_PDU_SIZE_MAX )
                {
                    pxASCIIFrameHdl->ubASCIIFrameBuffer[pxASCIIFrameHdl->usRcvBufferPos] = ( UBYTE ) ( ubBinValue << 4 );
                    pxASCIIFrameHdl->eBytePos = BYTE_LOW_NIBBLE;
                }
                else
                {
                    pxASCIIFrameHdl->eRcvState = STATE_RX_ERROR;
                }
                break;

            case BYTE_LOW_NIBBLE:
                pxASCIIFrameHdl->ubASCIIFrameBuffer[pxASCIIFrameHdl->usRcvBufferPos] |= ubBinValue;
                pxASCIIFrameHdl->usRcvBufferPos++;
                pxASCIIFrameHdl->eBytePos = BYTE_HIGH_NIBBLE;
                break;
            }
        }
        break;

    case STATE_RX_WAIT_EOF:
        if( MBM_ASCII_DEFAULT_LF == ubValue )
        {
            bEnableTimer = FALSE;
            eStatus = eMBPSerialRxEnable( pxASCIIFrameHdl->xSerHdl, NULL );
            MBP_ASSERT( MB_ENOERR == eStatus );
            eStatus = eMBPTimerStop( pxASCIIFrameHdl->xTmrHdl );
            MBP_ASSERT( MB_ENOERR == eStatus );

#if MBM_ASCII_BACKOF_TIME_MS > 0
            eStatus = eMBPTimerStart( pxASCIIFrameHdl->xBackOffTmrHdl );
            MBP_ASSERT( MB_ENOERR == eStatus );
#else
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECEIVED );
            MBP_ASSERT( MB_ENOERR == eStatus );
#endif
        }
        else if( MBM_ASCII_START == ubValue )
        {
            HDL_RESET_RX( pxASCIIFrameHdl );
            pxASCIIFrameHdl->eRcvState = STATE_RX_RCV;
        }
        else
        {
            pxASCIIFrameHdl->eRcvState = STATE_RX_ERROR;
        }
        break;

    case STATE_RX_IDLE:
        if( MBM_ASCII_START == ubValue )
        {
            /* Others settings initialized on the transistion from TX to RX. */
            pxASCIIFrameHdl->eRcvState = STATE_RX_RCV;
        }
        break;

        /* Receiver stays in error. The timeout from the MODBUS stack will 
         * abort this instruction.
         */
    case STATE_RX_ERROR:
    default:
        pxASCIIFrameHdl->eRcvState = STATE_RX_ERROR;
        break;
    }
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
    pxIntHdl->xFrameStat.ulNBytesReceived += 1;
#endif
    if( bEnableTimer )
    {
        if( MB_ENOERR != eMBPTimerStart( pxASCIIFrameHdl->xTmrHdl ) )
        {
            /* We can only abort here because or timers failed. */
            eStatus = eMBPSerialRxEnable( pxASCIIFrameHdl->xSerHdl, NULL );
            MBP_ASSERT( MB_ENOERR == eStatus );
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, MBM_EV_RECV_ERROR );
            MBP_ASSERT( MB_ENOERR == eStatus );
            pxASCIIFrameHdl->eRcvState = STATE_RX_ERROR;
        }
    }
}

STATIC          BOOL
bMBMSerialASCIITransmitterEmptyAPIV1CB( xMBHandle xHdl, UBYTE * pubValue )
{
    eMBErrorCode    eStatus;
    BOOL            bMoreTXData = FALSE;
    BOOL            bEnableRx = FALSE;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMASCIIFrameHandle *pxASCIIFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxASCIIFrameHdl->eRcvState == STATE_RX_IDLE );

    switch ( pxASCIIFrameHdl->eSndState )
    {
        /* Start of the transmission. the start of a frame is defined by
         * sending the character ':'. */
    case STATE_TX_START:
        *pubValue = MBM_ASCII_START;
        pxASCIIFrameHdl->eSndState = STATE_TX_DATA;
        pxASCIIFrameHdl->eBytePos = BYTE_HIGH_NIBBLE;
        bMoreTXData = TRUE;
        break;

        /* Send the data block. Each data byte is encoded as a character hex
         * stream where the high nibble is sent first. If all data bytes have
         * been sent we send a CR character to end the transmission.
         */
    case STATE_TX_DATA:
        if( pxASCIIFrameHdl->usSndBufferCnt > 0 )
        {
            switch ( pxASCIIFrameHdl->eBytePos )
            {
            case BYTE_HIGH_NIBBLE:
                /*@i2@ */ *pubValue =
                    ubMBMSerialASCIIBIN2CHAR( ( UBYTE ) ( *( pxASCIIFrameHdl->pubSndBufferCur ) >> 4 ) );
                pxASCIIFrameHdl->eBytePos = BYTE_LOW_NIBBLE;
                break;

            case BYTE_LOW_NIBBLE:
                /*@i2@ */ *pubValue =
                    ubMBMSerialASCIIBIN2CHAR( ( UBYTE ) ( *( pxASCIIFrameHdl->pubSndBufferCur ) & 0x0F ) );
                pxASCIIFrameHdl->eBytePos = BYTE_HIGH_NIBBLE;
                pxASCIIFrameHdl->pubSndBufferCur++;
                pxASCIIFrameHdl->usSndBufferCnt--;
                break;
            }
        }
        else
        {
            *pubValue = MBM_ASCII_DEFAULT_CR;
            pxASCIIFrameHdl->eSndState = STATE_TX_END;
        }
        bMoreTXData = TRUE;
        break;

        /* Finish the frame by sending an LF character. */
    case STATE_TX_END:
        *pubValue = MBM_ASCII_DEFAULT_LF;
        pxASCIIFrameHdl->eSndState = STATE_TX_NOTIFY;
        bMoreTXData = TRUE;
        break;

        /* Notify the caller that the frame has been sent. */
    case STATE_TX_NOTIFY:
        pxASCIIFrameHdl->eSndState = STATE_TX_IDLE;
        bMoreTXData = FALSE;
        if( MB_SER_BROADCAST_ADDR == pxASCIIFrameHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF] )
        {
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, MBM_EV_SENT );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
        else
        {
            bEnableRx = TRUE;
        }
        break;

        /* In this case the transmitter is disabled. */
    case STATE_TX_IDLE:
    default:
        break;
    }

    if( !bMoreTXData )
    {
        HDL_RESET_TX( pxASCIIFrameHdl );
    }
    else
    {
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
        pxIntHdl->xFrameStat.ulNBytesSent += 1;
#endif
    }
    if( bEnableRx )
    {
        HDL_RESET_RX( pxASCIIFrameHdl );
#if MBM_ASCII_WAITAFTERSEND_ENABLED > 0
        if( MB_ENOERR != eMBPTimerStart( pxASCIIFrameHdl->xWaitTmrHdl ) )
        {
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, MBM_EV_SEND_ERROR );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
#else

        if( MB_ENOERR != eMBPSerialRxEnable( pxASCIIFrameHdl->xSerHdl, ( pvMBPSerialReceiverCB ) vMBMSerialASCIIReceiverAPIV1CB ) )
        {
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, MBM_EV_SEND_ERROR );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
#endif
    }
    return bMoreTXData;
}
#endif

#if MBM_SERIAL_API_VERSION == 2
STATIC          BOOL
bMBMSerialASCIITransmitterEmptyAPIV2CB( xMBHandle xHdl, UBYTE * pubBufferOut, USHORT usBufferMax, USHORT * pusBufferWritten )
{
    eMBErrorCode    eStatus;
    BOOL            bMoreTxData = FALSE;
    BOOL            bEnableRx = FALSE;
    BOOL            bAppendMoreData = TRUE;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMASCIIFrameHandle *pxASCIIFrameHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    pxASCIIFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( NULL != pxASCIIFrameHdl );
    MBP_ASSERT( pxASCIIFrameHdl->eRcvState == STATE_RX_IDLE );
    MBP_ASSERT( usBufferMax > 0 );

    *pusBufferWritten = 0;
    do
    {
        switch ( pxASCIIFrameHdl->eSndState )
        {
            /* Start of the transmission. the start of a frame is defined by
             * sending the character ':'. */
        case STATE_TX_START:
            *pubBufferOut++ = MBM_ASCII_START;
            *pusBufferWritten = ( USHORT ) ( *pusBufferWritten + 1 );
            pxASCIIFrameHdl->eSndState = STATE_TX_DATA;
            pxASCIIFrameHdl->eBytePos = BYTE_HIGH_NIBBLE;
            bMoreTxData = TRUE;
            break;

            /* Send the data block. Each data byte is encoded as a character hex
             * stream where the high nibble is sent first. If all data bytes have
             * been sent we send a CR character to end the transmission.
             */
        case STATE_TX_DATA:
            if( pxASCIIFrameHdl->usSndBufferCnt > 0 )
            {
                MBP_ASSERT( NULL != pxASCIIFrameHdl->pubSndBufferCur );
                switch ( pxASCIIFrameHdl->eBytePos )
                {
                case BYTE_HIGH_NIBBLE:
                    *pubBufferOut++ = ubMBMSerialASCIIBIN2CHAR( ( UBYTE ) ( *( pxASCIIFrameHdl->pubSndBufferCur ) >> 4 ) );
                    *pusBufferWritten = ( USHORT ) ( *pusBufferWritten + 1 );
                    pxASCIIFrameHdl->eBytePos = BYTE_LOW_NIBBLE;
                    break;

                case BYTE_LOW_NIBBLE:
                    *pubBufferOut++ = ubMBMSerialASCIIBIN2CHAR( ( UBYTE ) ( *( pxASCIIFrameHdl->pubSndBufferCur ) & 0x0F ) );
                    *pusBufferWritten = ( USHORT ) ( *pusBufferWritten + 1 );
                    pxASCIIFrameHdl->eBytePos = BYTE_HIGH_NIBBLE;
                    pxASCIIFrameHdl->pubSndBufferCur++;
                    pxASCIIFrameHdl->usSndBufferCnt--;
                    break;
                }
                bMoreTxData = TRUE;
            }
            else
            {
                *pubBufferOut++ = MBM_ASCII_DEFAULT_CR;
                *pusBufferWritten = ( USHORT ) ( *pusBufferWritten + 1 );
                bMoreTxData = TRUE;
                pxASCIIFrameHdl->eSndState = STATE_TX_END;
            }
            break;

            /* Finish the frame by sending an LF character. */
        case STATE_TX_END:
            *pubBufferOut++ = MBM_ASCII_DEFAULT_LF;
            *pusBufferWritten = ( USHORT ) ( *pusBufferWritten + 1 );
            bMoreTxData = TRUE;
            pxASCIIFrameHdl->eSndState = STATE_TX_NOTIFY;
            bAppendMoreData = FALSE;
            break;

            /* Notify the caller that the frame has been sent. */
        case STATE_TX_NOTIFY:
            pxASCIIFrameHdl->eSndState = STATE_TX_IDLE;
            if( MB_SER_BROADCAST_ADDR == pxASCIIFrameHdl->ubASCIIFrameBuffer[MBM_SER_PDU_ADDR_OFF] )
            {
                eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SENT );
                MBP_ASSERT( MB_ENOERR == eStatus );
            }
            else
            {
                bEnableRx = TRUE;
            }
            bAppendMoreData = FALSE;

            /* In this case the transmitter is disabled. */
            /*lint -fallthrough */
        case STATE_TX_IDLE:
        default:
            bAppendMoreData = FALSE;
            break;
        }
    }
    while( bAppendMoreData && ( *pusBufferWritten < usBufferMax ) );

    if( !bMoreTxData )
    {
        HDL_RESET_TX( pxASCIIFrameHdl );
    }
    else
    {
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
        pxIntHdl->xFrameStat.ulNBytesSent += *pusBufferWritten;
#endif
    }
    if( bEnableRx )
    {
        HDL_RESET_RX( pxASCIIFrameHdl );
        if( MB_ENOERR != eMBPSerialRxEnable( pxASCIIFrameHdl->xSerHdl, ( pvMBPSerialReceiverCB ) vMBMSerialASCIIReceiverAPIV2CB ) )
        {
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SEND_ERROR );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return bMoreTxData;
}

STATIC void
vMBMSerialASCIIReceiverAPIV2CB( xMBHandle xHdl, const UBYTE * pubBufferIn, USHORT usBufferLen )
{
    eMBErrorCode    eStatus;
    USHORT          usBufferLeft = usBufferLen;
    BOOL            bEnableTimer = TRUE;
    UBYTE           ubBinValue;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMASCIIFrameHandle *pxASCIIFrameHdl;
    UBYTE           ubValue;

    MBP_ENTER_CRITICAL_SECTION(  );
    MBP_ASSERT( NULL != pxIntHdl );
    pxASCIIFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( NULL != pxASCIIFrameHdl );
    MBP_ASSERT( pxASCIIFrameHdl->eSndState == STATE_TX_IDLE );

    while( usBufferLeft > 0 )
    {
        ubValue = *pubBufferIn++;
        usBufferLeft--;
        switch ( pxASCIIFrameHdl->eRcvState )
        {
            /* A new character is received. If the character is a ':' the
             * input buffers are cleared. A CR character signals the end of the 
             * data block. Other characters are real data bytes.
             */
        case STATE_RX_RCV:
            if( MBM_ASCII_START == ubValue )
            {
                /* Empty receive buffer. */
                pxASCIIFrameHdl->eBytePos = BYTE_HIGH_NIBBLE;
                pxASCIIFrameHdl->usRcvBufferPos = 0;
            }
            else if( MBM_ASCII_DEFAULT_CR == ubValue )
            {
                pxASCIIFrameHdl->eRcvState = STATE_RX_WAIT_EOF;
            }
            else
            {
                ubBinValue = ubMBMSerialASCIICHAR2BIN( ubValue );
                switch ( pxASCIIFrameHdl->eBytePos )
                {
                case BYTE_HIGH_NIBBLE:
                    /* High nibble comes first. We need to check for an overflow. */
                    if( pxASCIIFrameHdl->usRcvBufferPos < MBM_SER_PDU_SIZE_MAX )
                    {
                        pxASCIIFrameHdl->ubASCIIFrameBuffer[pxASCIIFrameHdl->usRcvBufferPos] = ( UBYTE ) ( ubBinValue << 4 );
                        pxASCIIFrameHdl->eBytePos = BYTE_LOW_NIBBLE;
                    }
                    else
                    {
                        pxASCIIFrameHdl->eRcvState = STATE_RX_ERROR;
                    }
                    break;

                case BYTE_LOW_NIBBLE:
                    pxASCIIFrameHdl->ubASCIIFrameBuffer[pxASCIIFrameHdl->usRcvBufferPos] |= ubBinValue;
                    pxASCIIFrameHdl->usRcvBufferPos++;
                    pxASCIIFrameHdl->eBytePos = BYTE_HIGH_NIBBLE;
                    break;
                }
            }
            break;

        case STATE_RX_WAIT_EOF:
            if( MBM_ASCII_DEFAULT_LF == ubValue )
            {
                bEnableTimer = FALSE;
                eStatus = eMBPSerialRxEnable( pxASCIIFrameHdl->xSerHdl, NULL );
                MBP_ASSERT( MB_ENOERR == eStatus );
                eStatus = eMBPTimerStop( pxASCIIFrameHdl->xTmrHdl );
                MBP_ASSERT( MB_ENOERR == eStatus );

#if MBM_ASCII_BACKOF_TIME_MS > 0
                eStatus = eMBPTimerStart( pxASCIIFrameHdl->xBackOffTmrHdl );
                MBP_ASSERT( MB_ENOERR == eStatus );
#else
                eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECEIVED );
                MBP_ASSERT( MB_ENOERR == eStatus );
#endif
            }
            else if( MBM_ASCII_START == ubValue )
            {
                HDL_RESET_RX( pxASCIIFrameHdl );
                pxASCIIFrameHdl->eRcvState = STATE_RX_RCV;
            }
            else
            {
                pxASCIIFrameHdl->eRcvState = STATE_RX_ERROR;
            }
            break;

        case STATE_RX_IDLE:
            if( MBM_ASCII_START == ubValue )
            {
                /* Others settings initialized on the transistion from TX to RX. */
                pxASCIIFrameHdl->eRcvState = STATE_RX_RCV;
            }
            break;

            /* Receiver stays in error. The timeout from the MODBUS stack will 
             * abort this instruction.
             */
        case STATE_RX_ERROR:
        default:
            pxASCIIFrameHdl->eRcvState = STATE_RX_ERROR;
            break;
        }
    }

#if MBM_ENABLE_STATISTICS_INTERFACE == 1
    pxIntHdl->xFrameStat.ulNBytesReceived += usBufferLen;
#endif
    if( bEnableTimer )
    {
        if( MB_ENOERR != eMBPTimerStart( pxASCIIFrameHdl->xTmrHdl ) )
        {
            /* We can only abort here because or timers failed. */
            eStatus = eMBPSerialRxEnable( pxASCIIFrameHdl->xSerHdl, NULL );
            MBP_ASSERT( MB_ENOERR == eStatus );
            eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECV_ERROR );
            MBP_ASSERT( MB_ENOERR == eStatus );
            pxASCIIFrameHdl->eRcvState = STATE_RX_ERROR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}
#endif

#if MBM_TEST_INSTANCES == 0
STATIC
#endif
    BOOL
bMBMSerialASCIITimerCB( xMBHandle xHdl )
{
    eMBErrorCode    eStatus;
    BOOL            bNeedCtxSwitch = TRUE;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMASCIIFrameHandle *pxASCIIFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxASCIIFrameHdl->eSndState == STATE_TX_IDLE );

    eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECV_ERROR );
    MBP_ASSERT( MB_ENOERR == eStatus );

    return bNeedCtxSwitch;
}

STATIC          UBYTE
ubMBMSerialASCIICHAR2BIN( UBYTE ubCharacter )
{
    if( ( ubCharacter >= 0x30 /* ASCII '0' */  ) && ( ubCharacter <= 0x39 /* ASCII '9' */  ) )
    {
        return ( UBYTE ) ( ubCharacter - 0x30 /* ASCII '0' */  );
    }
    else if( ( ubCharacter >= 0x41 /* ASCII 'A' */  ) && ( ubCharacter <= 0x46 /* ASCII 'F' */  ) )
    {
        return ( UBYTE ) ( ( ubCharacter - 0x41 ) /* ASCII 'A' */  + 0x0A );
    }
    else
    {
        return 0xFF;
    }
}

STATIC          UBYTE
ubMBMSerialASCIIBIN2CHAR( UBYTE ubByte )
{
    if( ubByte <= 0x09 )
    {
        return ( UBYTE ) ( 0x30 + ubByte );
    }
    else if( ( ubByte >= 0x0A ) && ( ubByte <= 0x0F ) )
    {
        return ( UBYTE ) ( ( ubByte - 0x0A ) + 0x41 /* ASCII 'A' */  );
    }
    MBP_ASSERT( 0 );
    /*lint -e(527) */ return 0xFF;
}


STATIC          UBYTE
ubMBMSerialASCIILRC( const UBYTE * pubFrame, USHORT usLen )
{
    UBYTE           ubLRC = 0;  /* LRC char initialized */

    while( usLen-- > 0 )
    {
        ubLRC += *pubFrame++;   /* Add buffer byte without carry */
    }

    /* Return twos complement */
    ubLRC = ( UBYTE ) ( -( ( UBYTE ) ubLRC ) );
    return ubLRC;
}

#if MBM_ASCII_BACKOF_TIME_MS > 0
STATIC          BOOL
bMBMSerialASCIIBackOffTimerCB( xMBHandle xHdl )
{
    xMBMInternalHandle *pxIntHdl = xHdl;
    eMBErrorCode    eStatus;

    eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECEIVED );
    MBP_ASSERT( MB_ENOERR == eStatus );
    return TRUE;
}
#endif

#if MBM_ASCII_WAITAFTERSEND_ENABLED > 0
STATIC          BOOL
bMBMSerialWaitCB( xMBHandle xHdl )
{
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMASCIIFrameHandle *pxASCIIFrameHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    pxASCIIFrameHdl = pxIntHdl->xFrameHdl;
    MBP_ASSERT( pxASCIIFrameHdl->eSndState == STATE_TX_IDLE );
#if MBM_SERIAL_API_VERSION == 2
    if( MB_ENOERR != eMBPSerialRxEnable( pxASCIIFrameHdl->xSerHdl, ( pvMBPSerialReceiverCB ) vMBMSerialASCIIReceiverAPIV2CB ) )
#elif MBM_SERIAL_API_VERSION == 1
    if( MB_ENOERR != eMBPSerialRxEnable( pxASCIIFrameHdl->xSerHdl, ( pvMBPSerialReceiverCB ) vMBMSerialASCIIReceiverAPIV1CB ) )
#endif
    {
        ( void )eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_SEND_ERROR );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return FALSE;
}
#endif

#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
void
vMBMLogASCIIFrame( eMBPortLogLevel eLevel, xMBMInternalHandle * pxIntHdl, char *szMsg, const UBYTE * pubPayload, USHORT usLength )
{
    USHORT          usIdx;
    CHAR            arubBuffer[MBM_SER_PDU_SIZE_MAX * 2U + 1];

    xMBMASCIIFrameHandle *pxFrameHdl = pxIntHdl->xFrameHdl;

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
    vMBPPortLog( eLevel, MB_LOG_ASCII, "[IDX=" MBP_FORMAT_USHORT "] %s%s\n", ( USHORT ) pxFrameHdl->ubIdx, szMsg, arubBuffer );
}

#endif

#endif
