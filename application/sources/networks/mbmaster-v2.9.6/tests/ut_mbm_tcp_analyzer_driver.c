/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_tcp_analyzer_driver.c,v 1.2 2009-02-24 19:39:08 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include "Console.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "internal/mbmiframe.h"
#include "internal/mbmi.h"
#include "ut_mbm.h"
#include "ut_mbm_tcp_stubs.h"

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC int      iMBMTestInit( void );
STATIC int      iMBMTestClean( void );

STATIC void     vMBMTestTCPAnalyzerSimple( void );
STATIC void     vMBMTestTCPAnalyzerError( void );

/* ----------------------- Start implementation -----------------------------*/

int
iMBM_AddTests( void )
{
    CU_pSuite       pSuite = NULL;

    pSuite = CU_add_suite( "TCP", iMBMTestInit, iMBMTestClean );

    if( NULL == pSuite )
    {
        return -1;
    }
    if( ( NULL == CU_add_test( pSuite, "SIMPLE", vMBMTestTCPAnalyzerSimple ) ) ||
        ( NULL == CU_add_test( pSuite, "ERROR", vMBMTestTCPAnalyzerError ) ) )
    {
        return -1;
    }
    return 0;
}

int
iMBMTestInit( void )
{
    return 0;
}

int
iMBMTestClean( void )
{
    return 0;
}

STATIC const xMBAnalyzerFrame *pxCurSendFrame;
STATIC const xMBAnalyzerFrame *pxCurRecvFrame;
STATIC BOOL     bAnalyzerCBWasCalledSend;
STATIC BOOL     bAnalyzerCBWasCalledRecv;

STATIC void
vMBAnalyzerCallbackCB( xMBMHandle xHdl, void *pvCtx, const xMBPTimeStamp * pxTime, const xMBAnalyzerFrame * pxFrame )
{
    xMBPTimeStamp   xNewTime;

    vMBPGetTimeStamp( &xNewTime );
    CU_ASSERT_FATAL( NULL != pxFrame );
    CU_ASSERT_FATAL( NULL != pxTime );

    CU_ASSERT_EQUAL( xNewTime - 1, *pxTime );
    if( MB_FRAME_SEND == pxFrame->eFrameDir )
    {
        CU_ASSERT_FATAL( NULL != pxCurSendFrame );
        CU_ASSERT_EQUAL( pxCurSendFrame->eFrameDir, pxFrame->eFrameDir );
        CU_ASSERT_EQUAL( pxCurSendFrame->eFrameType, pxFrame->eFrameType );
        if( MB_FRAME_DAMAGED != pxCurRecvFrame->eFrameType )
        {
            CU_ASSERT_EQUAL( pxCurSendFrame->x.xTCPHeader.usMBAPTransactionId,
                             pxFrame->x.xTCPHeader.usMBAPTransactionId );
            CU_ASSERT_EQUAL( pxCurSendFrame->x.xTCPHeader.usMBAPProtocolId, pxFrame->x.xTCPHeader.usMBAPProtocolId );
            CU_ASSERT_EQUAL( pxCurSendFrame->x.xTCPHeader.usMBAPLength, pxFrame->x.xTCPHeader.usMBAPLength );
            CU_ASSERT_EQUAL( pxCurSendFrame->x.xTCPHeader.ubUnitIdentifier, pxFrame->x.xTCPHeader.ubUnitIdentifier );
        }
        CU_ASSERT_EQUAL( pxCurSendFrame->usDataPayloadLength, pxFrame->usDataPayloadLength );
        if( pxCurSendFrame->usDataPayloadLength == pxFrame->usDataPayloadLength )
        {
            CU_ASSERT( 0 == memcmp( pxCurSendFrame->ubDataPayload, pxFrame->ubDataPayload,
                                    pxFrame->usDataPayloadLength ) );
        }
        bAnalyzerCBWasCalledSend = TRUE;
    }
    else if( MB_FRAME_RECEIVE == pxFrame->eFrameDir )
    {
        CU_ASSERT_FATAL( NULL != pxCurRecvFrame );
        CU_ASSERT_EQUAL( pxCurRecvFrame->eFrameDir, pxFrame->eFrameDir );
        CU_ASSERT_EQUAL( pxCurRecvFrame->eFrameType, pxFrame->eFrameType );
        if( MB_FRAME_DAMAGED != pxCurRecvFrame->eFrameType )
        {
            CU_ASSERT_EQUAL( pxCurRecvFrame->x.xTCPHeader.usMBAPTransactionId,
                             pxFrame->x.xTCPHeader.usMBAPTransactionId );
            CU_ASSERT_EQUAL( pxCurRecvFrame->x.xTCPHeader.usMBAPProtocolId, pxFrame->x.xTCPHeader.usMBAPProtocolId );
            CU_ASSERT_EQUAL( pxCurRecvFrame->x.xTCPHeader.usMBAPLength, pxFrame->x.xTCPHeader.usMBAPLength );
            CU_ASSERT_EQUAL( pxCurRecvFrame->x.xTCPHeader.ubUnitIdentifier, pxFrame->x.xTCPHeader.ubUnitIdentifier );
        }
        CU_ASSERT_EQUAL( pxCurRecvFrame->usDataPayloadLength, pxFrame->usDataPayloadLength );
        if( pxCurRecvFrame->usDataPayloadLength == pxFrame->usDataPayloadLength )
        {
            CU_ASSERT( 0 == memcmp( pxCurRecvFrame->ubDataPayload, pxFrame->ubDataPayload,
                                    pxFrame->usDataPayloadLength ) );
        }
        bAnalyzerCBWasCalledRecv = TRUE;
    }
    else
    {
        CU_ASSERT( 0 );
    }
}

STATIC void
vMBMTestTCPAnalyzerSimple( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPTCPHandle   xTCPHdl;
    xMBAnalyzerFrame xAnalyzerSendFrame, xAnalyzerRecvFrame;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMHdl ) ) );
    if( MB_ENOERR == eStatus )
    {
        xTCPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMRegisterProtAnalyzer( xMBMHdl, NULL, vMBAnalyzerCallbackCB ) ) );

        /* Set expected frame within callback. */
        bAnalyzerCBWasCalledSend = FALSE;
        xAnalyzerSendFrame.eFrameDir = MB_FRAME_SEND;
        xAnalyzerSendFrame.eFrameType = MB_FRAME_TCP;
        xAnalyzerSendFrame.x.xTCPHeader.usMBAPTransactionId = 0;
        xAnalyzerSendFrame.x.xTCPHeader.usMBAPProtocolId = 0;
        xAnalyzerSendFrame.x.xTCPHeader.usMBAPLength = 6;
        xAnalyzerSendFrame.x.xTCPHeader.ubUnitIdentifier = 1;
        xAnalyzerSendFrame.ubDataPayload = arubExpectedRequest1;
        xAnalyzerSendFrame.usDataPayloadLength = sizeof( arubExpectedRequest1 );
        pxCurSendFrame = &xAnalyzerSendFrame;

        bAnalyzerCBWasCalledRecv = FALSE;
        xAnalyzerRecvFrame.eFrameDir = MB_FRAME_RECEIVE;
        xAnalyzerRecvFrame.eFrameType = MB_FRAME_TCP;
        xAnalyzerRecvFrame.x.xTCPHeader.usMBAPTransactionId = 0;
        xAnalyzerRecvFrame.x.xTCPHeader.usMBAPProtocolId = 0;
        xAnalyzerRecvFrame.x.xTCPHeader.usMBAPLength = 7;
        xAnalyzerRecvFrame.x.xTCPHeader.ubUnitIdentifier = 1;
        xAnalyzerRecvFrame.ubDataPayload = arubPreparedResponse1;
        xAnalyzerRecvFrame.usDataPayloadLength = sizeof( arubPreparedResponse1 );
        pxCurRecvFrame = &xAnalyzerRecvFrame;

        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );

        CU_ASSERT( bAnalyzerCBWasCalledSend );
        CU_ASSERT( bAnalyzerCBWasCalledRecv );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestTCPAnalyzerError( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPTCPHandle   xTCPHdl;
    xMBAnalyzerFrame xAnalyzerSendFrame, xAnalyzerRecvFrame;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedInvalidResponse1[] = {
        0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMHdl ) ) );
    if( MB_ENOERR == eStatus )
    {
        xTCPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMRegisterProtAnalyzer( xMBMHdl, NULL, vMBAnalyzerCallbackCB ) ) );

        /* Set expected frame within callback. */
        bAnalyzerCBWasCalledSend = FALSE;
        xAnalyzerSendFrame.eFrameDir = MB_FRAME_SEND;
        xAnalyzerSendFrame.eFrameType = MB_FRAME_TCP;
        xAnalyzerSendFrame.x.xTCPHeader.usMBAPTransactionId = 0;
        xAnalyzerSendFrame.x.xTCPHeader.usMBAPProtocolId = 0;
        xAnalyzerSendFrame.x.xTCPHeader.usMBAPLength = 6;
        xAnalyzerSendFrame.x.xTCPHeader.ubUnitIdentifier = 1;
        xAnalyzerSendFrame.ubDataPayload = arubExpectedRequest1;
        xAnalyzerSendFrame.usDataPayloadLength = sizeof( arubExpectedRequest1 );
        pxCurSendFrame = &xAnalyzerSendFrame;

        bAnalyzerCBWasCalledRecv = FALSE;
        xAnalyzerRecvFrame.eFrameDir = MB_FRAME_RECEIVE;
        xAnalyzerRecvFrame.eFrameType = MB_FRAME_DAMAGED;
        xAnalyzerRecvFrame.ubDataPayload = arubPreparedInvalidResponse1;
        xAnalyzerRecvFrame.usDataPayloadLength = sizeof( arubPreparedInvalidResponse1 );
        pxCurRecvFrame = &xAnalyzerRecvFrame;

        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedInvalidResponse1,
                                  MB_UTILS_NARRSIZE( arubPreparedInvalidResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );

        CU_ASSERT( bAnalyzerCBWasCalledSend );
        CU_ASSERT( bAnalyzerCBWasCalledRecv );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}
