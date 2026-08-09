/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmtcp.c,v 1.23 2014-08-23 09:44:21 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <stdio.h>
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
#include "mbmtcp.h"

/*lint --e{717} ~ suppress messages: do .. while(0); */
/*lint --e{788} ~ suppress messages: enum constant not used within defaulted switch */
/*lint --e{835} ~ suppress messages: A zero has been given as right argument to operator '+' */

#if MBM_TCP_ENABLED == 1

/* ----------------------- MBAP Header --------------------------------------*/
/*
 *
 * <------------------------ MODBUS TCP/IP ADU(1) ------------------------->
 *              <----------- MODBUS PDU (1') ---------------->
 *  +-----------+---------------+------------------------------------------+
 *  | TID | PID | Length | UID  |Code | Data                               |
 *  +-----------+---------------+------------------------------------------+
 *  |     |     |        |      |                                           
 * (2)   (3)   (4)      (5)    (6)                                          
 *
 * (2)  ... MBM_TCP_TID_OFF     = 0 (Transaction Identifier - 2 Byte) 
 * (3)  ... MBM_TCP_PID_OFF     = 2 (Protocol Identifier - 2 Byte)
 * (4)  ... MBM_TCP_LEN_OFF     = 4 (Number of bytes - 2 Byte)
 * (5)  ... MBM_TCP_UID_OFF     = 6 (Unit Identifier - 1 Byte)
 * (6)  ... MBM_TCP_MB_PDU_OFF  = 7 (MODBUS PDU)
 *
 * (1)  ... MODBUS TCP/IP ADU (application data unit)
 * (1') ... MODBUS PDU (protocol data unit)
 */

#define MBM_TCP_TID_OFF             ( 0 )
#define MBM_TCP_PID_OFF             ( 2 )
#define MBM_TCP_LEN_OFF             ( 4 )
#define MBM_TCP_UID_OFF             ( 6 )
#define MBM_TCP_MBAP_HEADER_SIZE    ( 7 )
#define MBM_TCP_MB_PDU_OFF          ( 7 )

#define MBM_TCP_PROTOCOL_ID         ( 0 )       /* 0 = Modbus Protocol */

/* ----------------------- Defines ------------------------------------------*/
#define MBM_TCP_PDU_SIZE_MIN        ( 7 )
#define MBM_TCP_PDU_SIZE_MAX        ( 260 )

#define MBM_TCP_BUFFER_SIZE         ( MBM_TCP_PDU_SIZE_MAX )
#define IDX_INVALID                 ( 255 )

#define HDL_RESET_CLIENT( x ) do { \
    ( x )->xTCPClientHdl =MBP_TCPHDL_CLIENT_INVALID; \
} while( 0 );

#define HDL_RESET_RX( x ) do { \
    ( x )->usBufferPos = 0; \
    ( x )->eState = STATE_IDLE; \
} while( 0 );

#define HDL_RESET_TX( x ) do { \
    ( x )->usBufferPos = 0; \
    ( x )->eState = STATE_IDLE; \
} while( 0 );

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->usCurrentTID = ( USHORT )-1; \
    HDL_RESET_TX( x ); \
    HDL_RESET_RX( x ); \
    HDL_RESET_CLIENT( x ); \
    memset( ( void * )&( ( x )->arubBuffer[ 0 ] ), 0, MBM_TCP_BUFFER_SIZE ); \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_IDLE,
    STATE_RCV
} eMBMTCPState;

typedef struct
{
    UBYTE           ubIdx;
    eMBMTCPState    eState;
    USHORT          usBufferPos;
    USHORT          usCurrentTID;
    xMBPTCPHandle   xTCPHdl;
    xMBPTCPClientHandle xTCPClientHdl;
    UBYTE           arubBuffer[MBM_TCP_BUFFER_SIZE];
} xMBMTCPFrameHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;
STATIC xMBMTCPFrameHandle xMBMTCPFrameHdl[MBM_TCP_MAX_INSTANCES];

/* ----------------------- Static functions ---------------------------------*/
STATIC eMBErrorCode eMBMTCPFrameSend( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength );
STATIC eMBErrorCode eMBMTCPFrameReceive( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength );
STATIC eMBErrorCode eMBMTCPFrameClose( xMBHandle xHdl );
STATIC eMBErrorCode eMBPTCPClientNewDataCB( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl );
STATIC eMBErrorCode eMBPTCPClientDisconnectedCB( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl );
STATIC BOOL     eMBMTCPFrameIsTransmitting( xMBHandle xHdl );

#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
void            vMBMLogTCPFrame( eMBPortLogLevel eLevel, xMBMInternalHandle * pxIntHdl, char *szMsg, const UBYTE * pubPayload, USHORT usLength );
#endif
/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBMTCPFrameInit( xMBMInternalHandle * pxIntHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMTCPFrameHandle *pxFrameHdl = NULL;
    UBYTE           ubIdx;

    if( NULL != pxIntHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !bIsInitialized )
        {
            for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBMTCPFrameHdl ); ubIdx++ )
            {
                HDL_RESET( &xMBMTCPFrameHdl[ubIdx] );
            }
            bIsInitialized = TRUE;
        }

        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBMTCPFrameHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xMBMTCPFrameHdl[ubIdx].ubIdx )
            {
                pxFrameHdl = &xMBMTCPFrameHdl[ubIdx];
                pxFrameHdl->ubIdx = ubIdx;
                break;
            }
        }

        if( NULL != pxFrameHdl )
        {
            if( MB_ENOERR == eMBPTCPClientInit( &( pxFrameHdl->xTCPHdl ), pxIntHdl, eMBPTCPClientNewDataCB, eMBPTCPClientDisconnectedCB ) )
            {
                MBP_ASSERT( NULL != pxFrameHdl->xTCPHdl );
                /* Attach the frame handle to the protocol stack. */
                pxIntHdl->pubFrameMBPDUBuffer = ( UBYTE * ) & pxFrameHdl->arubBuffer[MBM_TCP_MBAP_HEADER_SIZE];
                pxIntHdl->xFrameHdl = pxFrameHdl;
                pxIntHdl->pFrameSendFN = eMBMTCPFrameSend;
                pxIntHdl->pFrameRecvFN = eMBMTCPFrameReceive;
                pxIntHdl->pFrameCloseFN = eMBMTCPFrameClose;
#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
                pxIntHdl->pFrameIsTransmittingFN = eMBMTCPFrameIsTransmitting;
#endif
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_EPORTERR;

            }
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_TCP ) )
            {
                vMBPPortLog( MB_LOG_INFO, MB_LOG_TCP, "[IDX=" MBP_FORMAT_USHORT "] Creation of new TCP instance %s.\n",
                             ( USHORT ) pxFrameHdl->ubIdx, eStatus == MB_ENOERR ? "okay" : "failed" );
            }
#endif
            if( MB_ENOERR != eStatus )
            {
                HDL_RESET( pxFrameHdl );
            }
        }
        else
        {
            eStatus = MB_ENORES;
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return eStatus;
}

STATIC          BOOL
eMBMTCPFrameIsTransmitting( xMBHandle xHdl )
{
    BOOL            bIsTransmitting = FALSE;
    return bIsTransmitting;
}

STATIC          eMBErrorCode
eMBMTCPFrameSend( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength )
{
    eMBErrorCode    eStatus = MB_EINVAL, eStatus2;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMTCPFrameHandle *pxFrameHdl;
    USHORT          usPayloadLength;
#if defined( MBM_TCP_PURGE_BEFORE_SEND ) && ( MBM_TCP_PURGE_BEFORE_SEND == 1 )
    UBYTE           arubTempBuffer[8];
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
    xMBAnalyzerFrame xAnalyzerFrame;
    xMBPTimeStamp   xTimeStamp;
#endif

    if( bMBMIsHdlValid( pxIntHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_TCP,
                         "[IDX=" MBP_FORMAT_USHORT "] Sending new frame for slave=" MBP_FORMAT_USHORT " with length="
                         MBP_FORMAT_USHORT ".\n", ( USHORT ) pxFrameHdl->ubIdx, ( USHORT ) ucSlaveAddress, usMBPDULength );
        }
#endif
        MBP_ENTER_CRITICAL_SECTION(  );
        if( MB_IS_VALID_HDL( pxFrameHdl, xMBMTCPFrameHdl ) && MB_IS_VALID_TCP_ADDR( ucSlaveAddress ) &&
            ( usMBPDULength <= ( MBM_TCP_PDU_SIZE_MAX - 7 /* MBAP Header */  ) ) )
        {
            if( MBP_TCPHDL_CLIENT_INVALID != pxFrameHdl->xTCPClientHdl )
            {
                MBP_ASSERT( STATE_IDLE == pxFrameHdl->eState );

#if defined( MBM_TCP_PURGE_BEFORE_SEND ) && ( MBM_TCP_PURGE_BEFORE_SEND == 1 )
                do
                {
                    eStatus2 =
                        eMBPTCPConRead( pxFrameHdl->xTCPHdl, pxFrameHdl->xTCPClientHdl, arubTempBuffer, &usPayloadLength, MB_UTILS_NARRSIZE( arubTempBuffer ) );
                }
                while( ( MB_ENOERR == eStatus2 ) && ( usPayloadLength > 0 ) );
#endif

                /* Increase transaction identifier. */
                pxFrameHdl->usCurrentTID++;

                /* Add the transaction identifier. */
                pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF] = ( UBYTE ) ( pxFrameHdl->usCurrentTID >> 8 );
                pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF + 1] = ( UBYTE ) ( pxFrameHdl->usCurrentTID & 0xFF );

                /* Set the protocol. */
                pxFrameHdl->arubBuffer[MBM_TCP_PID_OFF] = /*lint -e(572) */ ( UBYTE ) ( MBM_TCP_PROTOCOL_ID >> 8 );
                pxFrameHdl->arubBuffer[MBM_TCP_PID_OFF + 1] = ( UBYTE ) ( MBM_TCP_PROTOCOL_ID & 0xFF );

                /* Set the length of the request. */
                pxFrameHdl->arubBuffer[MBM_TCP_LEN_OFF] = ( UBYTE ) ( ( usMBPDULength + 1 ) >> 8 );
                pxFrameHdl->arubBuffer[MBM_TCP_LEN_OFF + 1] = ( UBYTE ) ( ( usMBPDULength + 1 ) & 0xFF );

                /* Set the UID for the request. */
                pxFrameHdl->arubBuffer[MBM_TCP_UID_OFF] = ( UBYTE ) ucSlaveAddress;

                usPayloadLength = usMBPDULength + ( USHORT ) MBM_TCP_PDU_SIZE_MIN;
                eStatus2 = eMBPTCPConWrite( pxFrameHdl->xTCPHdl, pxFrameHdl->xTCPClientHdl, pxFrameHdl->arubBuffer, usPayloadLength );
                HDL_RESET_TX( pxFrameHdl );
                switch ( eStatus2 )
                {
                case MB_ENOERR:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_TCP ) )
                    {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                        vMBMLogTCPFrame( MB_LOG_DEBUG, pxIntHdl, "Sending frame: ",
                                         ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF], usPayloadLength );
#else
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_TCP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Sending new frame with length=" MBP_FORMAT_USHORT
                                     ".\n", ( USHORT ) pxFrameHdl->ubIdx, usPayloadLength );
#endif
                    }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                    /* Account for sent frame and bytes. */
                    pxIntHdl->xFrameStat.ulNBytesSent += usMBPDULength + MBM_TCP_PDU_SIZE_MIN;
                    pxIntHdl->xFrameStat.ulNPacketsSent += 1;
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                    /* Pass frame to protocol analyzer. */
                    xAnalyzerFrame.eFrameDir = MB_FRAME_SEND;
                    xAnalyzerFrame.eFrameType = MB_FRAME_TCP;
                    /* Its better to take these values directly out of the request 
                     * in case this part changes later because otherwise bugs could
                     * be introduced easily.
                     */
                    xAnalyzerFrame.x.xTCPHeader.usMBAPTransactionId = ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF] << 8U );
                    xAnalyzerFrame.x.xTCPHeader.usMBAPTransactionId |= ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF + 1] );
                    xAnalyzerFrame.x.xTCPHeader.usMBAPProtocolId = MBM_TCP_PROTOCOL_ID;
                    xAnalyzerFrame.x.xTCPHeader.usMBAPLength = ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_TCP_LEN_OFF] << 8U );;
                    xAnalyzerFrame.x.xTCPHeader.usMBAPLength |= ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_TCP_LEN_OFF + 1] );;
                    xAnalyzerFrame.x.xTCPHeader.ubUnitIdentifier = pxFrameHdl->arubBuffer[MBM_TCP_UID_OFF];

                    xAnalyzerFrame.ubDataPayload = ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF];
                    xAnalyzerFrame.usDataPayloadLength = usPayloadLength;
                    if( NULL != pxIntHdl->pvMBAnalyzerCallbackFN )
                    {
                        vMBPGetTimeStamp( &xTimeStamp );
                        pxIntHdl->pvMBAnalyzerCallbackFN( pxIntHdl, pxIntHdl->pvCtx, &xTimeStamp, &xAnalyzerFrame );
                    }
#endif
                    eStatus = MB_ENOERR;
                    pxFrameHdl->eState = STATE_RCV;
                    break;
                case MB_EIO:
                    eStatus = MB_EIO;
                    break;
                default:

                    eStatus = MB_EPORTERR;
                    break;
                }
                if( MB_ENOERR != eStatus )
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_WARN, MB_LOG_TCP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Sending frame failed with error = " MBP_FORMAT_USHORT
                                     "!\n", ( USHORT ) pxFrameHdl->ubIdx, ( USHORT ) eStatus );
                    }
#endif
                }
            }
            else
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_TCP ) )
                {
                    vMBPPortLog( MB_LOG_WARN, MB_LOG_TCP,
                                 "[IDX=" MBP_FORMAT_USHORT "] No client connected. sending not possible!\n", ( USHORT ) pxFrameHdl->ubIdx );
                }
#endif
                eStatus = MB_EIO;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return eStatus;
}



STATIC          eMBErrorCode
eMBMTCPFrameReceive( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMTCPFrameHandle *pxFrameHdl;
    USHORT          usMBAPTransactionIDField, usMBAPProtocolID;

#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
    xMBAnalyzerFrame xAnalyzerFrame;

    xMBPTimeStamp   xTimeStamp;
#endif
    if( bMBMIsHdlValid( pxIntHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
        MBP_ENTER_CRITICAL_SECTION(  );
        if( STATE_RCV == pxFrameHdl->eState )
        {
            if( MB_IS_VALID_HDL( pxFrameHdl, xMBMTCPFrameHdl ) && MB_IS_VALID_TCP_ADDR( ucSlaveAddress ) )
            {
                if( NULL == pusMBPDULength )
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_WARN, MB_LOG_TCP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Receiver aborted due to error or timeout!\n", ( USHORT ) pxFrameHdl->ubIdx );
                    }
#endif
                    /* This function might be called because of a timeout. */
                    eStatus = MB_ENOERR;
                }
                else if( ( pxFrameHdl->usBufferPos >= MBM_TCP_PDU_SIZE_MIN ) && ( ucSlaveAddress == pxFrameHdl->arubBuffer[MBM_TCP_UID_OFF] ) )
                {
                    usMBAPTransactionIDField = ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF] << 8;
                    usMBAPTransactionIDField |= ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF + 1];

                    usMBAPProtocolID = ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_PID_OFF] << 8;
                    usMBAPProtocolID |= ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_PID_OFF + 1];

                    /* We can now assert this as this is already checked in the receive function */
                    MBP_ASSERT( pxFrameHdl->usCurrentTID == usMBAPTransactionIDField );
                    MBP_ASSERT( MBM_TCP_PROTOCOL_ID == usMBAPProtocolID );
                    *pusMBPDULength = ( USHORT ) ( pxFrameHdl->usBufferPos - MBM_TCP_MB_PDU_OFF );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_TCP ) )
                    {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                        vMBMLogTCPFrame( MB_LOG_DEBUG, pxIntHdl, "Received frame: ",
                                         ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF], pxFrameHdl->usBufferPos );
#else
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_TCP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Received correct frame with length="
                                     MBP_FORMAT_USHORT ".\n", ( USHORT ) pxFrameHdl->ubIdx, pxFrameHdl->usBufferPos );
#endif
                    }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                    pxIntHdl->xFrameStat.ulNPacketsReceived += 1;
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                    xAnalyzerFrame.eFrameType = MB_FRAME_TCP;
                    xAnalyzerFrame.x.xTCPHeader.usMBAPTransactionId = usMBAPTransactionIDField;
                    xAnalyzerFrame.x.xTCPHeader.usMBAPProtocolId = usMBAPProtocolID;
                    xAnalyzerFrame.x.xTCPHeader.usMBAPLength = ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_TCP_LEN_OFF] << 8U );
                    xAnalyzerFrame.x.xTCPHeader.usMBAPLength |= ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_TCP_LEN_OFF + 1] );
                    xAnalyzerFrame.x.xTCPHeader.ubUnitIdentifier = pxFrameHdl->arubBuffer[MBM_TCP_UID_OFF];
#endif
                    eStatus = MB_ENOERR;
                }
                else
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_TCP ) )
                    {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                        vMBMLogTCPFrame( MB_LOG_DEBUG, pxIntHdl,
                                         "Received frame with incorrect length or wrong slave address: ",
                                         ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF], pxFrameHdl->usBufferPos );
#else
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_TCP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Received frame with incorrect length "
                                     MBP_FORMAT_USHORT "or wrong slave address!\n", ( USHORT ) pxFrameHdl->ubIdx, pxFrameHdl->usBufferPos );
#endif
                    }

#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                    /* Account for checksum failure. */
                    pxIntHdl->xFrameStat.ulNChecksumErrors += 1;
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                    xAnalyzerFrame.eFrameType = MB_FRAME_DAMAGED;
#endif
                    eStatus = MB_EIO;
                }
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                xAnalyzerFrame.eFrameDir = MB_FRAME_RECEIVE;
                xAnalyzerFrame.ubDataPayload = ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF];
                xAnalyzerFrame.usDataPayloadLength = pxFrameHdl->usBufferPos;
                if( NULL != pxIntHdl->pvMBAnalyzerCallbackFN )
                {
                    vMBPGetTimeStamp( &xTimeStamp );
                    pxIntHdl->pvMBAnalyzerCallbackFN( pxIntHdl, pxIntHdl->pvCtx, &xTimeStamp, &xAnalyzerFrame );
                }
#endif
                HDL_RESET_RX( pxFrameHdl );
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }

    return eStatus;
}

eMBErrorCode
eMBMTCPFrameConnect( xMBMInternalHandle * pxIntHdl, const CHAR * pcTCPClientAddress, USHORT usTCPPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMTCPFrameHandle *pxFrameHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( bMBMIsHdlValid( pxIntHdl ) && MB_IS_VALID_HDL( ( xMBMTCPFrameHandle * ) pxIntHdl->xFrameHdl, xMBMTCPFrameHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
        if( MBP_TCPHDL_CLIENT_INVALID != pxFrameHdl->xTCPClientHdl )
        {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_TCP ) )
            {
                vMBPPortLog( MB_LOG_WARN, MB_LOG_TCP,
                             "[IDX=" MBP_FORMAT_USHORT
                             "] Connect but client connection is already active. Dropping current client!\n", ( USHORT ) pxFrameHdl->ubIdx );
            }
#endif
            ( void )eMBMTCPFrameDisconnect( pxIntHdl );
            eStatus = MB_EIO;
            MBP_EXIT_CRITICAL_SECTION(  );
        }
        else
        {
            MBP_EXIT_CRITICAL_SECTION(  );
            /* We should not spwan the critical section over the client
             * open because this makes it overally complicated for a user
             * implementing the porting layer because <connect> will block
             * in case the remote site is down. This would lock the complete stack
             * for about 30 seconds (On WIN32). Moving the client connect
             * to a seperate thread does not make it better because the master
             * stack would then assume the connection is ready and would already
             * start transmitting data.
             *
             * Solution: We move the client open out of the global lock. No
             *  callbacks will be made by the client because he has no socket
             *  where to get callbacks from (Assumes correct client implementation).
             * Existing races: There is one existing race when a seperate thread
             *  from the thread which calls the <eMBMTCPConnect> calls MODBUS
             *  functions with the same handle.
             */
            eStatus = eMBPTCPClientOpen( pxFrameHdl->xTCPHdl, &( pxFrameHdl->xTCPClientHdl ), pcTCPClientAddress, usTCPPort );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_TCP,
                             "[IDX=" MBP_FORMAT_USHORT "] Connecting to client %s:" MBP_FORMAT_USHORT ": %s!\n",
                             ( USHORT ) pxFrameHdl->ubIdx, pcTCPClientAddress, usTCPPort, eStatus == MB_ENOERR ? "okay" : "failed" );
            }
#endif
        }
    }
    else
    {
        MBP_EXIT_CRITICAL_SECTION(  );
    }


    return eStatus;
}

/*lint -e{818} ~ suppress message: pxIntHdl could be declared as pointing to const */
eMBErrorCode
eMBMTCPFrameDisconnect( xMBMInternalHandle * pxIntHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMTCPFrameHandle *pxFrameHdl;

    if( bMBMIsHdlValid( pxIntHdl ) && MB_IS_VALID_HDL( ( xMBMTCPFrameHandle * ) pxIntHdl->xFrameHdl, xMBMTCPFrameHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
        if( MBP_TCPHDL_CLIENT_INVALID != pxFrameHdl->xTCPClientHdl )
        {
            ( void )eMBPTCPConClose( pxFrameHdl->xTCPHdl, pxFrameHdl->xTCPClientHdl );
            HDL_RESET_RX( pxFrameHdl );
            HDL_RESET_TX( pxFrameHdl );
            HDL_RESET_CLIENT( pxFrameHdl );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_TCP, "[IDX=" MBP_FORMAT_USHORT "] Closed client connection.\n", ( USHORT ) pxFrameHdl->ubIdx );
            }
#endif
        }
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

STATIC          eMBErrorCode
eMBPTCPClientNewDataCB( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL, eStatus2;
    xMBMInternalHandle *pxIntHdl = xMBHdl;
    xMBMTCPFrameHandle *pxFrameHdl;
    USHORT          usBytesRead;
    USHORT          usMaxBytesToRead;
    USHORT          usMBAPLengthField;
    USHORT          usMBAPTransactionIDField;
    USHORT          usMBAPProtocolID;
    BOOL            bClientBroken = FALSE;
    UBYTE           arubBuffer[8];
    ( void )xTCPClientHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( bMBMIsHdlValid( pxIntHdl ) && MB_IS_VALID_HDL( ( xMBMTCPFrameHandle * ) pxIntHdl->xFrameHdl, xMBMTCPFrameHdl ) )
    {
        eStatus = MB_ENOERR;
        pxFrameHdl = pxIntHdl->xFrameHdl;
        if( pxFrameHdl->eState != STATE_RCV )
        {
            /* Unexpected data - We just read and drop it (!) */
            if( MB_ENOERR != eMBPTCPConRead( pxFrameHdl->xTCPHdl, pxFrameHdl->xTCPClientHdl, arubBuffer, &usBytesRead, MB_UTILS_NARRSIZE( arubBuffer ) ) )
            {
                bClientBroken = TRUE;
            }
        }
        else if( pxFrameHdl->usBufferPos < MBM_TCP_MBAP_HEADER_SIZE )
        {
            usMaxBytesToRead = ( USHORT ) ( MBM_TCP_MBAP_HEADER_SIZE - pxFrameHdl->usBufferPos );
            eStatus2 =
                eMBPTCPConRead( pxFrameHdl->xTCPHdl, pxFrameHdl->xTCPClientHdl,
                                &( pxFrameHdl->arubBuffer[pxFrameHdl->usBufferPos] ), &usBytesRead, usMaxBytesToRead );
            switch ( eStatus2 )
            {
            case MB_ENOERR:
                pxFrameHdl->usBufferPos += usBytesRead;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_TCP ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_TCP,
                                 "[IDX=" MBP_FORMAT_USHORT "] Received " MBP_FORMAT_USHORT " bytes.\n", ( USHORT ) pxFrameHdl->ubIdx, usBytesRead );
                }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                pxIntHdl->xFrameStat.ulNBytesReceived += usBytesRead;
#endif
                break;
            case MB_EIO:
                bClientBroken = TRUE;
                break;
            default:
            case MB_EPORTERR:
                eStatus = MB_EPORTERR;
                break;
            }

#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( MB_ENOERR != eStatus2 )
            {
                if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_TCP ) )
                {
                    vMBPPortLog( MB_LOG_WARN, MB_LOG_TCP,
                                 "[IDX=" MBP_FORMAT_USHORT "] Read failed with error = " MBP_FORMAT_USHORT "!\n",
                                 ( USHORT ) pxFrameHdl->ubIdx, ( USHORT ) eStatus2 );
                }
            }
#endif
        }
        else
        {
            usMBAPLengthField = ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_LEN_OFF] << 8;
            usMBAPLengthField |= ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_LEN_OFF + 1];
            usMaxBytesToRead = ( USHORT ) ( ( usMBAPLengthField + MBM_TCP_UID_OFF ) - pxFrameHdl->usBufferPos );
            if( ( pxFrameHdl->usBufferPos + usMaxBytesToRead ) < MBM_TCP_BUFFER_SIZE )
            {
                eStatus2 =
                    eMBPTCPConRead( pxFrameHdl->xTCPHdl, pxFrameHdl->xTCPClientHdl,
                                    &( pxFrameHdl->arubBuffer[pxFrameHdl->usBufferPos] ), &usBytesRead, usMaxBytesToRead );
                switch ( eStatus2 )
                {
                case MB_ENOERR:
                    pxFrameHdl->usBufferPos += usBytesRead;
                    MBP_ASSERT( pxFrameHdl->usBufferPos <= ( USHORT ) ( usMBAPLengthField + MBM_TCP_UID_OFF ) );
                    /* Check if MODBUS frame is complete. */
                    if( ( USHORT ) ( usMBAPLengthField + MBM_TCP_UID_OFF ) == pxFrameHdl->usBufferPos )
                    {
                        usMBAPTransactionIDField = ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF] << 8;
                        usMBAPTransactionIDField |= ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_TID_OFF + 1];
                        usMBAPProtocolID = ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_PID_OFF] << 8;
                        usMBAPProtocolID |= ( USHORT ) pxFrameHdl->arubBuffer[MBM_TCP_PID_OFF + 1];
                        if( ( pxFrameHdl->usCurrentTID == usMBAPTransactionIDField ) && ( MBM_TCP_PROTOCOL_ID == usMBAPProtocolID ) )
                        {
                            if( MB_ENOERR != eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECEIVED ) )
                            {
                                eStatus = MB_EPORTERR;
                            }
                        }
                        else
                        {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_TCP ) )
                            {
                                vMBPPortLog( MB_LOG_WARN, MB_LOG_TCP,
                                             "[IDX=" MBP_FORMAT_USHORT "] received transaction ID " MBP_FORMAT_USHORT " does not match " MBP_FORMAT_USHORT "!\n",
                                             ( USHORT ) pxFrameHdl->ubIdx, usMBAPTransactionIDField, pxFrameHdl->usCurrentTID );
                            }
#endif
                            HDL_RESET_RX( pxFrameHdl );
                            pxFrameHdl->eState = STATE_RCV;
                        }
                    }
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                    pxIntHdl->xFrameStat.ulNBytesReceived += usBytesRead;
#endif
                    break;
                case MB_EIO:
                    bClientBroken = TRUE;
                    break;
                default:
                case MB_EPORTERR:
                    eStatus = MB_EPORTERR;
                }
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( MB_ENOERR != eStatus2 )
                {
                    if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_WARN, MB_LOG_TCP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Read failed with error = " MBP_FORMAT_USHORT "!\n",
                                     ( USHORT ) pxFrameHdl->ubIdx, ( USHORT ) eStatus2 );
                    }
                }
#endif
            }
            else
            {
                bClientBroken = TRUE;
            }
        }
        if( bClientBroken )
        {
            ( void )eMBPTCPConClose( pxFrameHdl->xTCPHdl, pxFrameHdl->xTCPClientHdl );
            HDL_RESET_RX( pxFrameHdl );
            HDL_RESET_TX( pxFrameHdl );
            HDL_RESET_CLIENT( pxFrameHdl );
            if( MB_ENOERR != eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECV_ERROR ) )
            {
                /* We can't handle this error. */
                MBP_ASSERT( eStatus == MB_ENOERR );
                eStatus = MB_EPORTERR;
            }
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_TCP ) )
            {
                vMBPPortLog( MB_LOG_WARN, MB_LOG_TCP, "[IDX=" MBP_FORMAT_USHORT "] Dropped client!\n", ( USHORT ) pxFrameHdl->ubIdx );
            }
#endif
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

STATIC          eMBErrorCode
eMBPTCPClientDisconnectedCB( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xMBHdl;
    xMBMTCPFrameHandle *pxFrameHdl;
    ( void )xTCPClientHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( bMBMIsHdlValid( pxIntHdl ) && MB_IS_VALID_HDL( ( xMBMTCPFrameHandle * ) pxIntHdl->xFrameHdl, xMBMTCPFrameHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
        ( void )eMBPTCPConClose( pxFrameHdl->xTCPHdl, pxFrameHdl->xTCPClientHdl );
        HDL_RESET_RX( pxFrameHdl );
        HDL_RESET_TX( pxFrameHdl );
        HDL_RESET_CLIENT( pxFrameHdl );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_TCP, "[IDX=" MBP_FORMAT_USHORT "] Client disconnected.\n", ( USHORT ) pxFrameHdl->ubIdx );
        }
#endif

    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

STATIC          eMBErrorCode
eMBMTCPFrameClose( xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMTCPFrameHandle *pxFrameHdl;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
    UBYTE           ubOldIdx;
#endif

    MBP_ENTER_CRITICAL_SECTION(  );
    if( bMBMIsHdlValid( pxIntHdl ) && MB_IS_VALID_HDL( ( xMBMTCPFrameHandle * ) pxIntHdl->xFrameHdl, xMBMTCPFrameHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        ubOldIdx = pxFrameHdl->ubIdx;
#endif
        if( MBP_TCPHDL_CLIENT_INVALID == pxFrameHdl->xTCPClientHdl )
        {
            ( void )eMBPTCPConClose( pxFrameHdl->xTCPHdl, pxFrameHdl->xTCPClientHdl );
        }
        eStatus = eMBPTCPClientClose( pxFrameHdl->xTCPHdl );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_TCP ) )
        {
            vMBPPortLog( MB_LOG_INFO, MB_LOG_TCP, "[IDX=" MBP_FORMAT_USHORT "] Closed TCP master instance %s.\n",
                         ( USHORT ) ubOldIdx, eStatus == MB_ENOERR ? "okay" : "failed" );
        }
#endif
        HDL_RESET( pxFrameHdl );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
void
vMBMLogTCPFrame( eMBPortLogLevel eLevel, xMBMInternalHandle * pxIntHdl, char *szMsg, const UBYTE * pubPayload, USHORT usLength )
{
    USHORT          usIdx;
    CHAR            arubBuffer[MBM_TCP_BUFFER_SIZE * 2U + 1];
    xMBMTCPFrameHandle *pxFrameHdl = pxIntHdl->xFrameHdl;

    MBP_ASSERT( usLength < MBM_TCP_BUFFER_SIZE );
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
    vMBPPortLog( eLevel, MB_LOG_TCP, "[IDX=" MBP_FORMAT_USHORT "] %s%s\n", ( USHORT ) pxFrameHdl->ubIdx, szMsg, arubBuffer );
}
#endif

#endif
