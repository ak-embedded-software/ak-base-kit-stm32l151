/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_ascii_analyzer_driver.c,v 1.2 2009-02-24 19:39:08 cwalter Exp $
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
#include "ut_mbm_ascii_stubs.h"

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC int      iMBMTestInit( void );
STATIC int      iMBMTestClean( void );

STATIC void     vMBMTestASCIIAnalyzerSimple( void );
STATIC void     vMBMTestASCIIAnalyzerError( void );

/* ----------------------- Start implementation -----------------------------*/

int
iMBM_AddTests( void )
{
    CU_pSuite       pSuite = NULL;

    pSuite = CU_add_suite( "ASCII", iMBMTestInit, iMBMTestClean );

    if( NULL == pSuite )
    {
        return -1;
    }
    if( ( NULL == CU_add_test( pSuite, "SIMPLE", vMBMTestASCIIAnalyzerSimple ) ) ||
        ( NULL == CU_add_test( pSuite, "ERROR", vMBMTestASCIIAnalyzerError ) ) )
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
            CU_ASSERT_EQUAL( pxCurSendFrame->x.xASCIIHeader.ubSlaveAddress, pxFrame->x.xASCIIHeader.ubSlaveAddress );
            CU_ASSERT_EQUAL( pxCurSendFrame->x.xASCIIHeader.ubLRC, pxFrame->x.xASCIIHeader.ubLRC );
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
            CU_ASSERT_EQUAL( pxCurRecvFrame->x.xASCIIHeader.ubSlaveAddress, pxFrame->x.xASCIIHeader.ubSlaveAddress );
            CU_ASSERT_EQUAL( pxCurRecvFrame->x.xASCIIHeader.ubLRC, pxFrame->x.xASCIIHeader.ubLRC );
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

STATIC          UBYTE
ubMBMSerialASCIICHAR2BIN( UBYTE ubCharacter )
{
    if( ( ubCharacter >= 0x30 /* ASCII '0' */  ) && ( ubCharacter <= 0x39 /* ASCII '9' */  ) )
    {
        return ( UBYTE ) ( ubCharacter - 0x30 /* ASCII '0' */  );
    }
    else if( ( ubCharacter >= 0x41 /* ASCII 'A' */  ) && ( ubCharacter <= 0x46 /* ASCII 'F' */  ) )
    {
        return ( UBYTE ) ( ubCharacter - 0x41 /* ASCII 'A' */  + 0x0A );
    }
    else
    {
        return 0xFF;
    }
}

STATIC void
vEncodeFrameASCII( const UBYTE * pubASCIIFrame, USHORT usLength, UBYTE * pubBinaryFrame, USHORT * pusBinaryLength )
{
    USHORT          usCnt;
    enum
    { NIBBLE_HIGH, NIBBLE_LOW } eCurNibble = NIBBLE_HIGH;

    *pusBinaryLength = 0;
    CU_ASSERT_FATAL( NULL != pubASCIIFrame );
    CU_ASSERT_FATAL( NULL != pubBinaryFrame );
    /* Start after ":" and strip out CR/LF */
    for( usCnt = 1; usCnt < usLength - 2; usCnt++ )
    {
        switch ( eCurNibble )
        {
        case NIBBLE_HIGH:
            pubBinaryFrame[*pusBinaryLength] = ubMBMSerialASCIICHAR2BIN( pubASCIIFrame[usCnt] ) << 4;
            eCurNibble = NIBBLE_LOW;
            break;
        case NIBBLE_LOW:
            pubBinaryFrame[*pusBinaryLength] |= ubMBMSerialASCIICHAR2BIN( pubASCIIFrame[usCnt] );
            *pusBinaryLength += 1;
            eCurNibble = NIBBLE_HIGH;
        }
    }
}

STATIC void
vMBMTestASCIIAnalyzerSimple( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBAnalyzerFrame xAnalyzerSendFrame, xAnalyzerRecvFrame;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubExpectedRequest1Binary[sizeof( arubExpectedRequest1 ) / 2];
    USHORT          arusExpectedRequest1BinaryLength;

    UBYTE           arubPreparedResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D, 0x0A
    };
    UBYTE           arubPreparedResponse1Binary[sizeof( arubPreparedResponse1 ) / 2];
    USHORT          arubPreparedResponse1BinaryLength;

    USHORT          arusReadRegisters1[5];

    vEncodeFrameASCII( arubExpectedRequest1, sizeof( arubExpectedRequest1 ), arubExpectedRequest1Binary,
                       &arusExpectedRequest1BinaryLength );
    vEncodeFrameASCII( arubPreparedResponse1, sizeof( arubPreparedResponse1 ), arubPreparedResponse1Binary,
                       &arubPreparedResponse1BinaryLength );

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMRegisterProtAnalyzer( xMBMHdl, NULL, vMBAnalyzerCallbackCB ) ) );

        /* Set expected frame within callback. */
        bAnalzerCBWasCalledSend = FALSE;
        xAnalyzerSendFrame.eFrameDir = MB_FRAME_SEND;
        xAnalyzerSendFrame.eFrameType = MB_FRAME_ASCII;
        xAnalyzerSendFrame.x.xASCIIHeader.ubSlaveAddress = 0x01;
        xAnalyzerSendFrame.x.xASCIIHeader.ubLRC = 0xF7;
        xAnalyzerSendFrame.ubDataPayload = arubExpectedRequest1Binary;
        xAnalyzerSendFrame.usDataPayloadLength = arusExpectedRequest1BinaryLength;
        pxCurSendFrame = &xAnalyzerSendFrame;

        bAnalzerCBWasCalledRecv = FALSE;
        xAnalyzerRecvFrame.eFrameDir = MB_FRAME_RECEIVE;
        xAnalyzerRecvFrame.eFrameType = MB_FRAME_ASCII;
        xAnalyzerRecvFrame.x.xASCIIHeader.ubSlaveAddress = 0x01;
        xAnalyzerRecvFrame.x.xASCIIHeader.ubLRC = 0x62;
        xAnalyzerRecvFrame.ubDataPayload = arubPreparedResponse1Binary;
        xAnalyzerRecvFrame.usDataPayloadLength = arubPreparedResponse1BinaryLength;
        pxCurRecvFrame = &xAnalyzerRecvFrame;

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) ) );

        CU_ASSERT( bAnalzerCBWasCalledSend );
        CU_ASSERT( bAnalzerCBWasCalledRecv );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIAnalyzerError( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBAnalyzerFrame xAnalyzerSendFrame, xAnalyzerRecvFrame;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubExpectedRequest1Binary[sizeof( arubExpectedRequest1 ) / 2];
    USHORT          arusExpectedRequest1BinaryLength;

    UBYTE           arubPreparedResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x38, 0x32, 0x0D, 0x0A
    };
    UBYTE           arubPreparedResponse1Binary[sizeof( arubPreparedResponse1 ) / 2];
    USHORT          arubPreparedResponse1BinaryLength;
    USHORT          arusReadRegisters1[5];

    vEncodeFrameASCII( arubExpectedRequest1, sizeof( arubExpectedRequest1 ), arubExpectedRequest1Binary,
                       &arusExpectedRequest1BinaryLength );
    vEncodeFrameASCII( arubPreparedResponse1, sizeof( arubPreparedResponse1 ), arubPreparedResponse1Binary,
                       &arubPreparedResponse1BinaryLength );

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMRegisterProtAnalyzer( xMBMHdl, NULL, vMBAnalyzerCallbackCB ) ) );

        /* Set expected frame within callback. */
        bAnalzerCBWasCalledSend = FALSE;
        xAnalyzerSendFrame.eFrameDir = MB_FRAME_SEND;
        xAnalyzerSendFrame.eFrameType = MB_FRAME_ASCII;
        xAnalyzerSendFrame.x.xASCIIHeader.ubSlaveAddress = 0x01;
        xAnalyzerSendFrame.x.xASCIIHeader.ubLRC = 0xF7;
        xAnalyzerSendFrame.ubDataPayload = arubExpectedRequest1Binary;
        xAnalyzerSendFrame.usDataPayloadLength = arusExpectedRequest1BinaryLength;
        pxCurSendFrame = &xAnalyzerSendFrame;

        bAnalzerCBWasCalledRecv = FALSE;
        xAnalyzerRecvFrame.eFrameDir = MB_FRAME_RECEIVE;
        xAnalyzerRecvFrame.eFrameType = MB_FRAME_DAMAGED;
        xAnalyzerRecvFrame.ubDataPayload = arubPreparedResponse1Binary;
        xAnalyzerRecvFrame.usDataPayloadLength = arubPreparedResponse1BinaryLength;
        pxCurRecvFrame = &xAnalyzerRecvFrame;

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR != ( eStatus = eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) ) );

        CU_ASSERT( bAnalzerCBWasCalledSend );
        CU_ASSERT( bAnalzerCBWasCalledRecv );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}
