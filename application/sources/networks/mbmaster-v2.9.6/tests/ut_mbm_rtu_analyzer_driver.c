/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_rtu_analyzer_driver.c,v 1.2 2009-02-24 19:39:08 cwalter Exp $
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
#include "ut_mbm_rtu_stubs.h"

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC int      iMBMTestInit( void );
STATIC int      iMBMTestClean( void );

STATIC void     vMBMTestRTUAnalyzerSimple( void );
STATIC void     vMBMTestRTUAnalyzerError( void );

/* ----------------------- Start implementation -----------------------------*/

int
iMBM_AddTests( void )
{
    CU_pSuite       pSuite = NULL;

    pSuite = CU_add_suite( "RTU", iMBMTestInit, iMBMTestClean );

    if( NULL == pSuite )
    {
        return -1;
    }
    if( ( NULL == CU_add_test( pSuite, "SIMPLE", vMBMTestRTUAnalyzerSimple ) ) ||
        ( NULL == CU_add_test( pSuite, "ERROR", vMBMTestRTUAnalyzerError ) ) )
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
STATIC BOOL     bAnalzerCBWasCalledSend;
STATIC BOOL     bAnalzerCBWasCalledRecv;

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
            CU_ASSERT_EQUAL( pxCurSendFrame->x.xRTUHeader.ubSlaveAddress, pxFrame->x.xRTUHeader.ubSlaveAddress );
            CU_ASSERT_EQUAL( pxCurSendFrame->x.xRTUHeader.usCRC16, pxFrame->x.xRTUHeader.usCRC16 );
        }
        CU_ASSERT_EQUAL( pxCurSendFrame->usDataPayloadLength, pxFrame->usDataPayloadLength );
        if( pxCurSendFrame->usDataPayloadLength == pxFrame->usDataPayloadLength )
        {
            CU_ASSERT( 0 == memcmp( pxCurSendFrame->ubDataPayload, pxFrame->ubDataPayload,
                                    pxFrame->usDataPayloadLength ) );
        }
        bAnalzerCBWasCalledSend = TRUE;
    }
    else if( MB_FRAME_RECEIVE == pxFrame->eFrameDir )
    {
        CU_ASSERT_FATAL( NULL != pxCurRecvFrame );
        CU_ASSERT_EQUAL( pxCurRecvFrame->eFrameDir, pxFrame->eFrameDir );
        CU_ASSERT_EQUAL( pxCurRecvFrame->eFrameType, pxFrame->eFrameType );
        if( MB_FRAME_DAMAGED != pxCurRecvFrame->eFrameType )
        {
            CU_ASSERT_EQUAL( pxCurRecvFrame->x.xRTUHeader.ubSlaveAddress, pxFrame->x.xRTUHeader.ubSlaveAddress );
            CU_ASSERT_EQUAL( pxCurRecvFrame->x.xRTUHeader.usCRC16, pxFrame->x.xRTUHeader.usCRC16 );
        }
        CU_ASSERT_EQUAL( pxCurRecvFrame->usDataPayloadLength, pxFrame->usDataPayloadLength );
        if( pxCurRecvFrame->usDataPayloadLength == pxFrame->usDataPayloadLength )
        {
            CU_ASSERT( 0 == memcmp( pxCurRecvFrame->ubDataPayload, pxFrame->ubDataPayload,
                                    pxFrame->usDataPayloadLength ) );
        }
        bAnalzerCBWasCalledRecv = TRUE;
    }
    else
    {
        CU_ASSERT( 0 );
    }
}

STATIC void
vMBMTestRTUAnalyzerSimple( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBAnalyzerFrame xAnalyzerSendFrame, xAnalyzerRecvFrame;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest1[] = {
        0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0xC5, 0xCD
    };
    UBYTE           arubPreparedResponse1[] = {
        0x01, 0x03, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x19, 0xA5, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37,
        0x1A
    };
    USHORT          arusReadRegisters1[10];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMRegisterProtAnalyzer( xMBMHdl, NULL, vMBAnalyzerCallbackCB ) ) );

        /* Set expected frame within callback. */
        bAnalzerCBWasCalledSend = FALSE;
        xAnalyzerSendFrame.eFrameDir = MB_FRAME_SEND;
        xAnalyzerSendFrame.eFrameType = MB_FRAME_RTU;
        xAnalyzerSendFrame.x.xRTUHeader.ubSlaveAddress = 0x01;
        xAnalyzerSendFrame.x.xRTUHeader.usCRC16 = 0xC5CD;
        xAnalyzerSendFrame.ubDataPayload = arubExpectedRequest1;
        xAnalyzerSendFrame.usDataPayloadLength = sizeof( arubExpectedRequest1 );
        pxCurSendFrame = &xAnalyzerSendFrame;

        bAnalzerCBWasCalledRecv = FALSE;
        xAnalyzerRecvFrame.eFrameDir = MB_FRAME_RECEIVE;
        xAnalyzerRecvFrame.eFrameType = MB_FRAME_RTU;
        xAnalyzerRecvFrame.x.xRTUHeader.ubSlaveAddress = 0x01;
        xAnalyzerRecvFrame.x.xRTUHeader.usCRC16 = 0x371A;
        xAnalyzerRecvFrame.ubDataPayload = arubPreparedResponse1;
        xAnalyzerRecvFrame.usDataPayloadLength = sizeof( arubPreparedResponse1 );
        pxCurRecvFrame = &xAnalyzerRecvFrame;

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 10, &arusReadRegisters1[0] ) );

        CU_ASSERT( bAnalzerCBWasCalledSend );
        CU_ASSERT( bAnalzerCBWasCalledRecv );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestRTUAnalyzerError( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBAnalyzerFrame xAnalyzerSendFrame, xAnalyzerRecvFrame;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest1[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedInvalidResponse1[] = {
        0x01, 0x03, 0x0A, 0x15, 0xB3, 0x1A, 0x0A, 0x1E,
        0x61, 0x22, 0xB8, 0x27, 0x0F, 0xFF, 0xFF
    };
    USHORT          arusReadRegisters2[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMRegisterProtAnalyzer( xMBMHdl, NULL, vMBAnalyzerCallbackCB ) ) );

        /* Set expected frame within callback. */
        bAnalzerCBWasCalledSend = FALSE;
        xAnalyzerSendFrame.eFrameDir = MB_FRAME_SEND;
        xAnalyzerSendFrame.eFrameType = MB_FRAME_RTU;
        xAnalyzerSendFrame.x.xRTUHeader.ubSlaveAddress = 0x01;
        xAnalyzerSendFrame.x.xRTUHeader.usCRC16 = 0xC5CD;
        xAnalyzerSendFrame.ubDataPayload = arubExpectedRequest1;
        xAnalyzerSendFrame.usDataPayloadLength = sizeof( arubExpectedRequest1 );
        pxCurSendFrame = &xAnalyzerSendFrame;

        bAnalzerCBWasCalledRecv = FALSE;
        xAnalyzerRecvFrame.eFrameDir = MB_FRAME_RECEIVE;
        xAnalyzerRecvFrame.eFrameType = MB_FRAME_DAMAGED;
        xAnalyzerRecvFrame.ubDataPayload = arubPreparedInvalidResponse1;
        xAnalyzerRecvFrame.usDataPayloadLength = sizeof( arubPreparedInvalidResponse1 );
        pxCurRecvFrame = &xAnalyzerRecvFrame;

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedInvalidResponse1, MB_UTILS_NARRSIZE( arubPreparedInvalidResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );

        CU_ASSERT( bAnalzerCBWasCalledSend );
        CU_ASSERT( bAnalzerCBWasCalledRecv );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}
