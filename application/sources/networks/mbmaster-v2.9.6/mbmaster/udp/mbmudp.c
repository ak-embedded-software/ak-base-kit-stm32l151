/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP/UDP.
 * Copyright (c) 2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmudp.c,v 1.3 2011-12-04 20:51:46 embedded-solutions.cwalter Exp $
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
#include "mbmudp.h"

#if MBM_UDP_ENABLED == 1

/* ----------------------- MBAP Header --------------------------------------*/
/*
 *
 * <------------------------ MODBUS UDP/IP ADU(1) ------------------------->
 *              <----------- MODBUS PDU (1') ---------------->
 *  +-----------+---------------+------------------------------------------+
 *  | TID | PID | Length | UID  |Code | Data                               |
 *  +-----------+---------------+------------------------------------------+
 *  |     |     |        |      |                                           
 * (2)   (3)   (4)      (5)    (6)                                          
 *
 * (2)  ... MBM_UDP_TID_OFF     = 0 (Transaction Identifier - 2 Byte) 
 * (3)  ... MBM_UDP_PID_OFF     = 2 (Protocol Identifier - 2 Byte)
 * (4)  ... MBM_UDP_LEN_OFF     = 4 (Number of bytes - 2 Byte)
 * (5)  ... MBM_UDP_UID_OFF     = 6 (Unit Identifier - 1 Byte)
 * (6)  ... MBM_UDP_MB_PDU_OFF  = 7 (MODBUS PDU)
 *
 * (1)  ... MODBUS UDP/IP ADU (application data unit)
 * (1') ... MODBUS PDU (protocol data unit)
 */

#define MBM_UDP_TID_OFF             ( 0 )
#define MBM_UDP_PID_OFF             ( 2 )
#define MBM_UDP_LEN_OFF             ( 4 )
#define MBM_UDP_UID_OFF             ( 6 )
#define MBM_UDP_MBAP_HEADER_SIZE    ( 7 )
#define MBM_UDP_MB_PDU_OFF          ( 7 )
#define MBM_UDP_PROTOCOL_ID         ( 0 )       /* 0 = Modbus Protocol */

/* ----------------------- Defines ------------------------------------------*/
#define MBM_UDP_PDU_SIZE_MIN        ( 7 )
#define MBM_UDP_PDU_SIZE_MAX        ( 260 )

#define MBM_UDP_BUFFER_SIZE         ( MBM_UDP_PDU_SIZE_MAX )
#define IDX_INVALID                 ( 255 )

#define HDL_RESET_CLIENT( x ) do { \
    MBP_UDP_CLIENTADDR_FREE( ( x )->pcUDPClientAddress ); \
	( x )->pcUDPClientAddress = NULL; \
    ( x )->usClientPort = 0; \
} while( 0 );

#define HDL_RESET_RX( x ) do { \
    ( x )->eState = STATE_IDLE; \
    ( x )->usBufferPos = 0; \
} while( 0 );

#define HDL_RESET_TX( x ) do { \
    ( x )->eState = STATE_IDLE; \
    ( x )->usBufferPos = 0; \
} while( 0 );

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->usCurrentTID = ( USHORT )-1; \
    HDL_RESET_TX( x ); \
    HDL_RESET_RX( x ); \
    HDL_RESET_CLIENT( x ); \
    memset( ( void * )&( ( x )->arubBuffer[ 0 ] ), 0, MBM_UDP_BUFFER_SIZE ); \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_IDLE,
    STATE_RCV
} eMBMUDPState;

typedef struct
{
    UBYTE           ubIdx;
    eMBMUDPState    eState;
    USHORT          usCurrentTID;
    xMBPUDPHandle   xUDPHdl;
    CHAR           *pcUDPClientAddress;
    USHORT          usClientPort;

    UBYTE           arubBuffer[MBM_UDP_BUFFER_SIZE];
    USHORT          usBufferPos;
} xMBMUDPFrameHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;
STATIC xMBMUDPFrameHandle xMBMUDPFrameHdl[MBM_UDP_MAX_INSTANCES];

/* ----------------------- Static functions ---------------------------------*/
STATIC eMBErrorCode eMBMUDPFrameSend( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength );
STATIC eMBErrorCode eMBMUDPFrameReceive( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength );
STATIC eMBErrorCode eMBMUDPFrameClose( xMBHandle xHdl );
STATIC eMBErrorCode eMBPUDPClientNewDataCB( xMBHandle xMBHdll );

#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
void            vMBMLogUDPFrame( eMBPortLogLevel eLevel, xMBMInternalHandle * pxIntHdl, char *szMsg,
                                 const UBYTE * pubPayload, USHORT usLength );
#endif
/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBMUDPFrameInit( xMBMInternalHandle * pxIntHdl, const CHAR * pcUDPBindAddress, LONG uUDPListenPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMUDPFrameHandle *pxFrameHdl = NULL;
    UBYTE           ubIdx;

    if( NULL != pxIntHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !bIsInitialized )
        {
            for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBMUDPFrameHdl ); ubIdx++ )
            {
                /* Bugfix - Initally assign NULL to makre sure porting
                 * layer always can use MBP_UDP_CLIENTADDR_FREE.
                 */
                xMBMUDPFrameHdl[ubIdx].pcUDPClientAddress = NULL;
                HDL_RESET( &xMBMUDPFrameHdl[ubIdx] );
            }
            bIsInitialized = TRUE;
        }

        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBMUDPFrameHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xMBMUDPFrameHdl[ubIdx].ubIdx )
            {
                pxFrameHdl = &xMBMUDPFrameHdl[ubIdx];
                pxFrameHdl->ubIdx = ubIdx;
                break;
            }
        }

        if( NULL != pxFrameHdl )
        {
            if( MB_ENOERR ==
                eMBPUDPClientInit( &( pxFrameHdl->xUDPHdl ), pxIntHdl, pcUDPBindAddress, uUDPListenPort,
                                   eMBPUDPClientNewDataCB ) )
            {
                MBP_ASSERT( NULL != pxFrameHdl->xUDPHdl );

                /* Attach the frame handle to the protocol stack. */
                pxIntHdl->pubFrameMBPDUBuffer = ( UBYTE * ) & pxFrameHdl->arubBuffer[MBM_UDP_MBAP_HEADER_SIZE];
                pxIntHdl->xFrameHdl = pxFrameHdl;
                pxIntHdl->pFrameSendFN = eMBMUDPFrameSend;
                pxIntHdl->pFrameRecvFN = eMBMUDPFrameReceive;
                pxIntHdl->pFrameCloseFN = eMBMUDPFrameClose;
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_EPORTERR;

            }
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_UDP ) )
            {
                vMBPPortLog( MB_LOG_INFO, MB_LOG_UDP, "[IDX=" MBP_FORMAT_USHORT "] Creation of new UDP instance %s.\n",
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

STATIC          eMBErrorCode
eMBMUDPFrameSend( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength )
{
    eMBErrorCode    eStatus = MB_EINVAL, eStatus2;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMUDPFrameHandle *pxFrameHdl;
    USHORT          usPayloadLength;

#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
    xMBAnalyzerFrame xAnalyzerFrame;
    xMBPTimeStamp   xTimeStamp;
#endif

    if( bMBMIsHdlValid( pxIntHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_UDP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_UDP,
                         "[IDX=" MBP_FORMAT_USHORT "] Sending new frame for slave=" MBP_FORMAT_USHORT
                         " with MODBUS PDU length=" MBP_FORMAT_USHORT ".\n", ( USHORT ) pxFrameHdl->ubIdx,
                         ( USHORT ) ucSlaveAddress, usMBPDULength );
        }
#endif
        MBP_ENTER_CRITICAL_SECTION(  );
        if( MB_IS_VALID_HDL( pxFrameHdl, xMBMUDPFrameHdl ) && MB_IS_VALID_TCP_ADDR( ucSlaveAddress ) &&
            ( usMBPDULength <= ( MBM_UDP_PDU_SIZE_MAX - 7 /* MBAP Header */  ) ) )
        {
            if( NULL != pxFrameHdl->pcUDPClientAddress )
            {
                MBP_ASSERT( STATE_IDLE == pxFrameHdl->eState );

                /* Increase transaction identifier. */
                pxFrameHdl->usCurrentTID++;

                /* Add the transaction identifier. */
                pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF] = ( UBYTE ) ( pxFrameHdl->usCurrentTID >> 8 );
                pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF + 1] = ( UBYTE ) ( pxFrameHdl->usCurrentTID & 0xFF );

                /* Set the protocol. */
                pxFrameHdl->arubBuffer[MBM_UDP_PID_OFF] = /*lint -e(572) */ ( UBYTE ) ( MBM_UDP_PROTOCOL_ID >> 8 );
                pxFrameHdl->arubBuffer[MBM_UDP_PID_OFF + 1] = ( UBYTE ) ( MBM_UDP_PROTOCOL_ID & 0xFF );

                /* Set the length of the request. */
                pxFrameHdl->arubBuffer[MBM_UDP_LEN_OFF] = ( UBYTE ) ( ( usMBPDULength + 1 ) >> 8 );
                pxFrameHdl->arubBuffer[MBM_UDP_LEN_OFF + 1] = ( UBYTE ) ( ( usMBPDULength + 1 ) & 0xFF );

                /* Set the UID for the request. */
                pxFrameHdl->arubBuffer[MBM_UDP_UID_OFF] = ( UBYTE ) ucSlaveAddress;

                usPayloadLength = usMBPDULength + ( USHORT ) MBM_UDP_PDU_SIZE_MIN;
                eStatus2 =
                    eMBPUDPConWrite( pxFrameHdl->xUDPHdl, pxFrameHdl->pcUDPClientAddress, pxFrameHdl->usClientPort,
                                     pxFrameHdl->arubBuffer, usPayloadLength );
                HDL_RESET_TX( pxFrameHdl );
                switch ( eStatus2 )
                {
                case MB_ENOERR:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_UDP ) )
                    {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                        vMBMLogUDPFrame( MB_LOG_DEBUG, pxIntHdl, "Sending frame: ",
                                         ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF], usPayloadLength );
#else
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_UDP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Sending new frame with MODBUS TCP PDU length="
                                     MBP_FORMAT_USHORT ".\n", ( USHORT ) pxFrameHdl->ubIdx, usPayloadLength );
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
                    xAnalyzerFrame.eFrameType = MB_FRAME_UDP;
                    /* Its better to take these values directly out of the request 
                     * in case this part changes later because otherwise bugs could
                     * be introduced easily.
                     */
                    xAnalyzerFrame.x.xUDPHeader.usMBAPTransactionId =
                        ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF] << 8U );
                    xAnalyzerFrame.x.xUDPHeader.usMBAPTransactionId |=
                        ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF + 1] );
                    xAnalyzerFrame.x.xUDPHeader.usMBAPProtocolId = MBM_UDP_PROTOCOL_ID;
                    xAnalyzerFrame.x.xUDPHeader.usMBAPLength =
                        ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_UDP_LEN_OFF] << 8U );;
                    xAnalyzerFrame.x.xUDPHeader.usMBAPLength |=
                        ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_UDP_LEN_OFF + 1] );;
                    xAnalyzerFrame.x.xUDPHeader.ubUnitIdentifier = pxFrameHdl->arubBuffer[MBM_UDP_UID_OFF];

                    xAnalyzerFrame.ubDataPayload = ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF];
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
                    if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_UDP ) )
                    {
                        vMBPPortLog( MB_LOG_WARN, MB_LOG_UDP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Sending frame failed with error = " MBP_FORMAT_USHORT
                                     "!\n", ( USHORT ) pxFrameHdl->ubIdx, ( USHORT ) eStatus );
                    }
#endif
                }
            }
            else
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_UDP ) )
                {
                    vMBPPortLog( MB_LOG_WARN, MB_LOG_UDP,
                                 "[IDX=" MBP_FORMAT_USHORT "] No client connected. sending not possible!\n",
                                 ( USHORT ) pxFrameHdl->ubIdx );
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
eMBMUDPFrameReceive( xMBHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMUDPFrameHandle *pxFrameHdl;
    USHORT          usMBAPTransactionIDField, usMBAPProtocolID;

#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
    xMBAnalyzerFrame xAnalyzerFrame;
    xMBPTimeStamp   xTimeStamp;
#endif
    if( bMBMIsHdlValid( pxIntHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
        MBP_ENTER_CRITICAL_SECTION(  );
        if( MB_IS_VALID_HDL( pxFrameHdl, xMBMUDPFrameHdl ) && ( STATE_RCV == pxFrameHdl->eState ) )
        {
            if( MB_IS_VALID_TCP_ADDR( ucSlaveAddress ) )
            {
                if( NULL == pusMBPDULength )
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_UDP ) )
                    {
                        vMBPPortLog( MB_LOG_WARN, MB_LOG_UDP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Receiver aborted due to error or timeout!\n",
                                     ( USHORT ) pxFrameHdl->ubIdx );
                    }
#endif
                    /* This function might be called because of a timeout. */
                    eStatus = MB_ENOERR;
                }
                else if( ( ucSlaveAddress == pxFrameHdl->arubBuffer[MBM_UDP_UID_OFF] ) )
                {
                    usMBAPTransactionIDField = ( USHORT ) pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF] << 8;
                    usMBAPTransactionIDField |= ( USHORT ) pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF + 1];

                    usMBAPProtocolID = ( USHORT ) pxFrameHdl->arubBuffer[MBM_UDP_PID_OFF] << 8;
                    usMBAPProtocolID |= ( USHORT ) pxFrameHdl->arubBuffer[MBM_UDP_PID_OFF + 1];

                    if( ( pxFrameHdl->usCurrentTID == usMBAPTransactionIDField )
                        && ( MBM_UDP_PROTOCOL_ID == usMBAPProtocolID ) )
                    {
                        *pusMBPDULength = ( USHORT ) ( pxFrameHdl->usBufferPos - MBM_UDP_MB_PDU_OFF );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_UDP ) )
                        {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                            vMBMLogUDPFrame( MB_LOG_DEBUG, pxIntHdl, "Received frame: ",
                                             ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF],
                                             pxFrameHdl->usBufferPos );
#else
                            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_UDP,
                                         "[IDX=" MBP_FORMAT_USHORT "] Received correct frame with length="
                                         MBP_FORMAT_USHORT ".\n", ( USHORT ) pxFrameHdl->ubIdx,
                                         pxFrameHdl->usBufferPos );
#endif
                        }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                        pxIntHdl->xFrameStat.ulNPacketsReceived += 1;
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
                        xAnalyzerFrame.eFrameType = MB_FRAME_UDP;
                        xAnalyzerFrame.x.xUDPHeader.usMBAPTransactionId = usMBAPTransactionIDField;
                        xAnalyzerFrame.x.xUDPHeader.usMBAPProtocolId = usMBAPProtocolID;
                        xAnalyzerFrame.x.xUDPHeader.usMBAPLength =
                            ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_UDP_LEN_OFF] << 8U );
                        xAnalyzerFrame.x.xUDPHeader.usMBAPLength |=
                            ( USHORT ) ( pxFrameHdl->arubBuffer[MBM_UDP_LEN_OFF + 1] );
                        xAnalyzerFrame.x.xUDPHeader.ubUnitIdentifier = pxFrameHdl->arubBuffer[MBM_UDP_UID_OFF];
#endif
                        eStatus = MB_ENOERR;
                    }
                    else
                    {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_UDP ) )
                        {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                            vMBMLogUDPFrame( MB_LOG_DEBUG, pxIntHdl,
                                             "Received frame with incorrect transaction id or protocol: ",
                                             ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF],
                                             pxFrameHdl->usBufferPos );
#else
                            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_UDP,
                                         "[IDX=" MBP_FORMAT_USHORT
                                         "] Received frame with incorrect transaction id or protocol!\n",
                                         ( USHORT ) pxFrameHdl->ubIdx );
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
                }
                else
                {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_UDP ) )
                    {
#if defined( MBM_ENABLE_DEBUG_FACILITY_HEAVY ) && ( MBM_ENABLE_DEBUG_FACILITY_HEAVY == 1 )
                        vMBMLogTCPFrame( MB_LOG_DEBUG, pxIntHdl,
                                         "Received frame with incorrect length or wrong slave address: ",
                                         ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF],
                                         pxFrameHdl->usBufferPos );
#else
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_UDP,
                                     "[IDX=" MBP_FORMAT_USHORT "] Received frame with incorrect length "
                                     MBP_FORMAT_USHORT "or wrong slave address!\n", ( USHORT ) pxFrameHdl->ubIdx,
                                     pxFrameHdl->usBufferPos );
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
                xAnalyzerFrame.ubDataPayload = ( const UBYTE * )&pxFrameHdl->arubBuffer[MBM_UDP_TID_OFF];
                xAnalyzerFrame.usDataPayloadLength = pxFrameHdl->usBufferPos;
                if( NULL != pxIntHdl->pvMBAnalyzerCallbackFN )
                {
                    vMBPGetTimeStamp( &xTimeStamp );
                    pxIntHdl->pvMBAnalyzerCallbackFN( pxIntHdl, pxIntHdl->pvCtx, &xTimeStamp, &xAnalyzerFrame );
                }
#endif
            }
            else
            {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_UDP ) )
                {
                    vMBPPortLog( MB_LOG_WARN, MB_LOG_UDP,
                                 "[IDX=" MBP_FORMAT_USHORT "] Invalid handle or wrong slave address " MBP_FORMAT_USHORT
                                 "!\n", ( USHORT ) pxFrameHdl->ubIdx, ( USHORT ) ucSlaveAddress );
                }
#endif
                eStatus = MB_EINVAL;
            }

            HDL_RESET_RX( pxFrameHdl );
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }

    return eStatus;
}

eMBErrorCode
eMBMUDPFrameSetClient( xMBMInternalHandle * pxIntHdl, const CHAR * pcUDPClientAddress, USHORT usUDPPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMUDPFrameHandle *pxFrameHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( bMBMIsHdlValid( pxIntHdl ) && MB_IS_VALID_HDL( ( xMBMUDPFrameHandle * ) pxIntHdl->xFrameHdl, xMBMUDPFrameHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
        MBP_UDP_CLIENTADDR_FREE( pxFrameHdl->pcUDPClientAddress );
        MBP_UDP_CLIENTADDR_COPY( pcUDPClientAddress, pxFrameHdl->pcUDPClientAddress );
        pxFrameHdl->usClientPort = usUDPPort;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

STATIC          eMBErrorCode
eMBPUDPClientNewDataCB( xMBHandle xMBHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL, eStatus2;
    xMBMInternalHandle *pxIntHdl = xMBHdl;
    xMBMUDPFrameHandle *pxFrameHdl;
    USHORT          usBytesRead;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( bMBMIsHdlValid( pxIntHdl ) && MB_IS_VALID_HDL( ( xMBMUDPFrameHandle * ) pxIntHdl->xFrameHdl, xMBMUDPFrameHdl ) )
    {
        eStatus = MB_ENOERR;
        pxFrameHdl = pxIntHdl->xFrameHdl;
        eStatus2 =
            eMBPUDPConRead( pxFrameHdl->xUDPHdl, &( pxFrameHdl->arubBuffer[0] ), &usBytesRead, MBM_UDP_BUFFER_SIZE );
        switch ( eStatus2 )
        {
        case MB_ENOERR:
            pxFrameHdl->usBufferPos = usBytesRead;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_UDP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_UDP,
                             "[IDX=" MBP_FORMAT_USHORT "] Received " MBP_FORMAT_USHORT " bytes.\n",
                             ( USHORT ) pxFrameHdl->ubIdx, usBytesRead );
            }
#endif
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
            pxIntHdl->xFrameStat.ulNBytesReceived += usBytesRead;
#endif
            break;
        default:
        case MB_EPORTERR:
            eStatus = MB_EPORTERR;
            break;
        }

        if( MB_ENOERR == eStatus2 )
        {
            if( STATE_IDLE != pxFrameHdl->eState )
            {
                pxFrameHdl->usBufferPos = usBytesRead;
                if( MB_ENOERR != eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_RECEIVED ) )
                {
                    eStatus = MB_EPORTERR;
                }
            }
        }
        else
        {
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_UDP ) )
            {
                vMBPPortLog( MB_LOG_WARN, MB_LOG_UDP,
                             "[IDX=" MBP_FORMAT_USHORT "] Read failed with error = " MBP_FORMAT_USHORT "!\n",
                             ( USHORT ) pxFrameHdl->ubIdx, ( USHORT ) eStatus2 );
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
eMBMUDPFrameClose( xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xMBMUDPFrameHandle *pxFrameHdl;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
    UBYTE           ubOldIdx;
#endif

    MBP_ENTER_CRITICAL_SECTION(  );
    if( bMBMIsHdlValid( pxIntHdl ) && MB_IS_VALID_HDL( ( xMBMUDPFrameHandle * ) pxIntHdl->xFrameHdl, xMBMUDPFrameHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        ubOldIdx = pxFrameHdl->ubIdx;
#endif
        eStatus = eMBPUDPClientClose( pxFrameHdl->xUDPHdl );
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_UDP ) )
        {
            vMBPPortLog( MB_LOG_INFO, MB_LOG_UDP, "[IDX=" MBP_FORMAT_USHORT "] Closed UDP master instance %s.\n",
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
vMBMLogUDPFrame( eMBPortLogLevel eLevel, xMBMInternalHandle * pxIntHdl, char *szMsg, const UBYTE * pubPayload,
                 USHORT usLength )
{
    USHORT          usIdx;
    CHAR            arubBuffer[MBM_UDP_BUFFER_SIZE * 2U + 1];
    xMBMUDPFrameHandle *pxFrameHdl = pxIntHdl->xFrameHdl;

    MBP_ASSERT( usLength < MBM_UDP_BUFFER_SIZE );
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
    vMBPPortLog( eLevel, MB_LOG_UDP, "[IDX=" MBP_FORMAT_USHORT "] %s%s\n", ( USHORT ) pxFrameHdl->ubIdx, szMsg,
                 arubBuffer );
}
#endif

#endif
