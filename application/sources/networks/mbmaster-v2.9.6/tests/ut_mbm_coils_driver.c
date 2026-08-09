/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_coils_driver.c,v 1.5 2011-05-22 22:29:05 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
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

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    USHORT          usDelayMilliSeconds;
    const UBYTE    *pubPreparedResponse;
    USHORT          usPreparedResponseLength;
} xPreparedRespThreadData;

/* ----------------------- Static variables ---------------------------------*/
static xMBMHandle xMBMHdl;

/* ----------------------- Static functions ---------------------------------*/
STATIC int      iMBMTestInit( void );
STATIC int      iMBMTestClean( void );

STATIC void     vMBMTestReadCoilsExceptions( void );
STATIC void     vMBMTestReadCoilsBasic( void );
STATIC void     vMBMTestReadCoilsPollingInterface( void );
STATIC void     vMBMTestReadCoilsIllegalResponse( void );
STATIC void     vMBMTestReadCoilsAPI( void );

STATIC void     VMBMWriteSingleCoilBasic( void );

STATIC void     VMBMWriteCoils( void );

STATIC /*@null@ */ void *pvMBMDelayedResponse( void *pvThreadData );

/* ----------------------- Start implementation -----------------------------*/

int
iMBM_AddTests( void )
{
    CU_pSuite       pSuite = NULL;

    pSuite = CU_add_suite( "READ HOLDING", iMBMTestInit, iMBMTestClean );

    if( NULL == pSuite )
    {
        return -1;
    }
    if( ( NULL == CU_add_test( pSuite, "READ COILS (BASIC TESTS)", vMBMTestReadCoilsBasic ) )
        || ( NULL == CU_add_test( pSuite, "READ COILS (ILLEGAL RESPONSES)",
                                  vMBMTestReadCoilsIllegalResponse ) )
        || ( NULL == CU_add_test( pSuite, "READ COILS (POLLING INTERFACE)",
                                  vMBMTestReadCoilsPollingInterface ) )
        || ( NULL == CU_add_test( pSuite, "READ COILS (EXCEPTIONS)",
                                  vMBMTestReadCoilsExceptions ) )
        || ( NULL == CU_add_test( pSuite, "READ COILS (API SAFETY)", vMBMTestReadCoilsAPI ) )
        || ( NULL == CU_add_test( pSuite, "WRITE SINGLE COIL (BASIC TESTS)", VMBMWriteSingleCoilBasic ) )
        || ( NULL == CU_add_test( pSuite, "WRITE COILS (BASIC TESTS)", VMBMWriteCoils ) ) )
    {
        return -1;
    }
    return 0;
}

int
iMBMTestInit( void )
{
    eMBErrorCode    eStatus;

    if( MB_ENOERR != ( eStatus = eMBMTestInit( &xMBMHdl ) ) )
    {
        return -1;
    }
    return 0;
}

int
iMBMTestClean( void )
{
    eMBMClose( xMBMHdl );
    return 0;
}

void
VMBMWriteCoils( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;

    const UBYTE     arubExpectedRequest1[] = {
        0x0F, 0x00, 0x13, 0x00, 0x0A, 0x02, 0xCD, 0x01
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x0F, 0x00, 0x13, 0x00, 0x0A
    };
    const UBYTE     ubNCoils[] = { 0xCD, 0x01 };

    const UBYTE     arubExpectedRequest2[] = {
        0x0F, 0x00, 0x13, 0x00, 0x08, 0x01, 0xA5
    };
    const UBYTE     arubPreparedResponse2[] = {
        0x0F, 0x00, 0x13, 0x00, 0x08
    };
    const UBYTE     ubNCoils2[] = { 0xA5 };

    const UBYTE     arubExpectedRequest3[] = {
        0x0F, 0x00, 0x00, 0x00, 0x0A, 0x02, 0x14, 0x00
    };
    const UBYTE     arubPreparedResponse3[] = {
        0x0F, 0x00, 0x00, 0x00, 0x0A
    };
    const UBYTE     ubNCoils3[] = { 0x14, 0x00 };



    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteCoils( xMBMHdl, 1, 19, 10, ubNCoils ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest2, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse2 );
    xThreadData.pubPreparedResponse = arubPreparedResponse2;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteCoils( xMBMHdl, 1, 19, 8, ubNCoils2 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest3, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest3 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse3 );
    xThreadData.pubPreparedResponse = arubPreparedResponse3;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteCoils( xMBMHdl, 1, 0, 10, ubNCoils3 ) );
}

void
VMBMWriteSingleCoilBasic( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;

    const UBYTE     arubExpectedRequest1[] = {
        0x05, 0x00, 0x10, 0x00, 0x00
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x05, 0x00, 0x10, 0x00, 0x00
    };
    const UBYTE     arubExpectedRequest2[] = {
        0x05, 0x00, 0x10, 0xFF, 0x00
    };
    const UBYTE     arubPreparedResponse2[] = {
        0x05, 0x00, 0x10, 0xFF, 0x00
    };

    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleCoil( xMBMHdl, 1, 16, FALSE ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest2, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse2 );
    xThreadData.pubPreparedResponse = arubPreparedResponse2;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleCoil( xMBMHdl, 1, 16, TRUE ) );


}

void
vMBMTestReadCoilsBasic( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;

    UBYTE           ubCoilsOut1[3];
    const UBYTE     arubExpectedRequest1[] = {
        0x01, 0x00, 0x13, 0x00, 0x13
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x01, 0x03, 0xCD, 0x6B, 0x05
    };
    UBYTE           ubCoilsOut2[2];
    const UBYTE     arubExpectedRequest2[] = {
        0x01, 0x00, 0x00, 0x00, 0x0A
    };
    const UBYTE     arubPreparedResponse2[] = {
        0x01, 0x02, 0x20, 0x00
    };

    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );
    CU_ASSERT_EQUAL( 0xCD, ubCoilsOut1[0] );
    CU_ASSERT_EQUAL( 0x6B, ubCoilsOut1[1] );
    CU_ASSERT_EQUAL( 0x05, ubCoilsOut1[2] );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest2, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse2 );
    xThreadData.pubPreparedResponse = arubPreparedResponse2;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMReadCoils( xMBMHdl, 1, 0, 10, ubCoilsOut2 ) );
    CU_ASSERT_EQUAL( 0x20, ubCoilsOut2[0] );
    CU_ASSERT_EQUAL( 0x00, ubCoilsOut2[1] );
}


void
vMBMTestReadCoilsExceptions( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;

    UBYTE           ubCoilsOut1[3];
    const UBYTE     arubExpectedRequest1[] = {
        0x01, 0x00, 0x13, 0x00, 0x13
    };

    const UBYTE     arubExIllegalDataAddressResponse[] = { 0x81, 0x02 };
    const UBYTE     arubExIllegalDataValueResponse[] = { 0x81, 0x03 };
    const UBYTE     arubExSlaveDeviceFailure[] = { 0x81, 0x04 };
    const UBYTE     arubExAcknowledge[] = { 0x81, 0x05 };
    const UBYTE     arubExSlaveDeviceBusy[] = { 0x81, 0x06 };
    const UBYTE     arubExMemoryParityError[] = { 0x81, 0x08 };
    const UBYTE     arubExGatewayPathUnavailable[] = { 0x81, 0x0A };
    const UBYTE     arubExGatewayTargetFailed[] = { 0x81, 0x0B };
    const UBYTE     arubExUnknown[] = { 0x81, 0x55 };
    const UBYTE     arubExWrongCode[] = { 0x82, 0x02 };
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataAddressResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataAddressResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_ADDRESS == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataValueResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataValueResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_VALUE == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceFailure;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceFailure );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_DEVICE_FAILURE == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubExAcknowledge;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExAcknowledge );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ACKNOWLEDGE == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceBusy;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceBusy );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_BUSY == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubExMemoryParityError;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExMemoryParityError );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_MEMORY_PARITY_ERROR == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubExGatewayPathUnavailable;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayPathUnavailable );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_PATH_UNAVAILABLE == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubExGatewayTargetFailed;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayTargetFailed );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_TARGET_FAILED == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubExUnknown;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExUnknown );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubExWrongCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExWrongCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );
}

void
vMBMTestReadCoilsPollingInterface( void )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    UBYTE           ubCoilsOut1[3];
    const UBYTE     arubExpectedRequest1[] = {
        0x01, 0x00, 0x13, 0x00, 0x13
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x01, 0x03, 0xCD, 0x6B, 0x05
    };

    vMBMReadCoilsPolled( xMBMHdl, 1, 19, 19, ubCoilsOut1, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_SEND );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    vMBMReadCoilsPolled( xMBMHdl, 1, 19, 19, ubCoilsOut1, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_WAITING );

    eMBMTestSetPreparedResponse( xMBMHdl, arubPreparedResponse1,
                                 ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_RECEIVED ) );
    vMBMReadCoilsPolled( xMBMHdl, 1, 19, 19, ubCoilsOut1, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DISASSEMBLE );

    vMBMReadCoilsPolled( xMBMHdl, 1, 19, 19, ubCoilsOut1, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );

}

void
vMBMTestReadCoilsIllegalResponse( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;
    UBYTE           ubCoilsOut1[3];
    const UBYTE     arubExpectedRequest1[] = {
        0x01, 0x00, 0x13, 0x00, 0x13
    };

    xThreadData.usDelayMilliSeconds = 100;

    const UBYTE     arubRespEmptyFrame[] = { };
    const UBYTE     arubRespFrameToShort1[] = { 0x01 };
    const UBYTE     arubRespFrameToShort2[] = { 0x01, 0x03 };
    const UBYTE     arubRespFrameToShort3[] = { 0x01, 0x03, 0xCD };
    const UBYTE     arubRespFrameToShort4[] = { 0x01, 0x03, 0xCD, 0x6B };

    const UBYTE     arubRespIllegalFunctionCode[] = { 0x02, 0x03, 0xCD, 0x6B, 0x0 };
    const UBYTE     arubRespIllegalLength[] = { 0x01, 0x04, 0xCD, 0x6B, 0x0 };
    const UBYTE     arubRespFrameToLong1[] = { 0x01, 0x03, 0xCD, 0x6B, 0x0, 0xAA };
    const UBYTE     arubRespFrameOkay[] = { 0x01, 0x03, 0xCD, 0x6B, 0x0 };

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );

    xThreadData.pubPreparedResponse = arubRespEmptyFrame;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespEmptyFrame );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort3;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort3 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort4;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort4 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalFunctionCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalFunctionCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalLength;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalLength );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToLong1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToLong1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameOkay;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameOkay );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMReadCoils( xMBMHdl, 1, 19, 19, ubCoilsOut1 ) );
}

void
vMBMTestReadCoilsAPI( void )
{
    xMBMInternalHandle xMBMInvalidHdl;
    UBYTE           ubCoilsOut1[3];

    CU_ASSERT( MB_EINVAL == eMBMReadCoils( NULL, 1, 0, 0, ubCoilsOut1 ) );
    CU_ASSERT( MB_EINVAL == eMBMReadCoils( xMBMHdl, 1, 0, 0, ubCoilsOut1 ) );
    CU_ASSERT( MB_EINVAL == eMBMReadCoils( xMBMHdl, 1, 1, 0, ubCoilsOut1 ) );
    CU_ASSERT( MB_EINVAL == eMBMReadCoils( xMBMHdl, 1, 1, 1, NULL ) );
    CU_ASSERT( MB_EINVAL == eMBMReadCoils( &xMBMInvalidHdl, 1, 1, 1, ubCoilsOut1 ) );
}

void           *
pvMBMDelayedResponse( void *pvThreadData )
{
    xPreparedRespThreadData *pxData = pvThreadData;

    eMBMTestSetPreparedResponse( xMBMHdl, pxData->pubPreparedResponse, pxData->usPreparedResponseLength );
    ( void )usleep( pxData->usDelayMilliSeconds * 1000 );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_RECEIVED ) );
    pthread_exit( NULL );
    return NULL;
}
