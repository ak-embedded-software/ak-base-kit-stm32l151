/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_disc_driver.c,v 1.2 2008-03-20 19:39:24 cwalter Exp $
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

STATIC void     vMBMTestReadDiscreteExceptions( void );
STATIC void     vMBMTestReadDiscreteBasic( void );
STATIC void     vMBMTestReadDiscretePollingInterface( void );
STATIC void     vMBMTestReadDiscreteIllegalResponse( void );
STATIC void     vMBMTestReadDiscreteAPI( void );

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
    if( ( NULL == CU_add_test( pSuite, "READ DISCRETE INPUTS (BASIC TESTS)",
                               vMBMTestReadDiscreteBasic ) )
        || ( NULL == CU_add_test( pSuite, "READ DISCRETE INPUTS (ILLEGAL RESPONSES)",
                                  vMBMTestReadDiscreteIllegalResponse ) )
        || ( NULL == CU_add_test( pSuite, "READ DISCRETE INPUTS (POLLING INTERFACE)",
                                  vMBMTestReadDiscretePollingInterface ) )
        || ( NULL == CU_add_test( pSuite, "READ DISCRETE INPUTS (EXCEPTIONS)",
                                  vMBMTestReadDiscreteExceptions ) )
        || ( NULL == CU_add_test( pSuite, "READ DISCRTE INPUTS (API SAFETY)", vMBMTestReadDiscreteAPI ) ) )
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
vMBMTestReadDiscreteBasic( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;

    UBYTE           ubDiscOut1[3];
    const UBYTE     arubExpectedRequest1[] = {
        0x02, 0x00, 0xC4, 0x00, 0x16
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x02, 0x03, 0xAC, 0xDB, 0x35
    };

    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );
    CU_ASSERT_EQUAL( 0xAC, ubDiscOut1[0] );
    CU_ASSERT_EQUAL( 0xDB, ubDiscOut1[1] );
    CU_ASSERT_EQUAL( 0x35, ubDiscOut1[2] );
}

void
vMBMTestReadDiscreteExceptions( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;

    UBYTE           ubDiscOut1[3];
    const UBYTE     arubExpectedRequest1[] = {
        0x02, 0x00, 0xC4, 0x00, 0x16
    };

    const UBYTE     arubExIllegalDataAddressResponse[] = { 0x82, 0x02 };
    const UBYTE     arubExIllegalDataValueResponse[] = { 0x82, 0x03 };
    const UBYTE     arubExSlaveDeviceFailure[] = { 0x82, 0x04 };
    const UBYTE     arubExAcknowledge[] = { 0x82, 0x05 };
    const UBYTE     arubExSlaveDeviceBusy[] = { 0x82, 0x06 };
    const UBYTE     arubExMemoryParityError[] = { 0x82, 0x08 };
    const UBYTE     arubExGatewayPathUnavailable[] = { 0x82, 0x0A };
    const UBYTE     arubExGatewayTargetFailed[] = { 0x82, 0x0B };
    const UBYTE     arubExUnknown[] = { 0x82, 0x55 };
    const UBYTE     arubExWrongCode[] = { 0x83, 0x02 };
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataAddressResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataAddressResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_ADDRESS == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataValueResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataValueResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_VALUE == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceFailure;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceFailure );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_DEVICE_FAILURE == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubExAcknowledge;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExAcknowledge );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ACKNOWLEDGE == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceBusy;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceBusy );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_BUSY == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubExMemoryParityError;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExMemoryParityError );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_MEMORY_PARITY_ERROR == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubExGatewayPathUnavailable;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayPathUnavailable );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_PATH_UNAVAILABLE == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubExGatewayTargetFailed;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayTargetFailed );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_TARGET_FAILED == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubExUnknown;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExUnknown );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubExWrongCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExWrongCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );
}

void
vMBMTestReadDiscretePollingInterface( void )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    /* Write Multiple Registers, Address = 0x0100, Values = 0x0102, 0x0304, 0x0506 */
    UBYTE           ubDiscOut1[3];
    const UBYTE     arubExpectedRequest1[] = {
        0x02, 0x00, 0xC4, 0x00, 0x16
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x02, 0x03, 0xAC, 0xDB, 0x35
    };

    vMBMReadDiscreteInputsPolled( xMBMHdl, 1, 196, 22, ubDiscOut1, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_SEND );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    vMBMReadDiscreteInputsPolled( xMBMHdl, 1, 196, 22, ubDiscOut1, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_WAITING );

    eMBMTestSetPreparedResponse( xMBMHdl, arubPreparedResponse1,
                                 ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_RECEIVED ) );
    vMBMReadDiscreteInputsPolled( xMBMHdl, 1, 196, 22, ubDiscOut1, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DISASSEMBLE );

    vMBMReadDiscreteInputsPolled( xMBMHdl, 1, 196, 22, ubDiscOut1, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );

}

void
vMBMTestReadDiscreteIllegalResponse( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;
    UBYTE           ubDiscOut1[3];
    const UBYTE     arubExpectedRequest1[] = {
        0x02, 0x00, 0xC4, 0x00, 0x16
    };
    xThreadData.usDelayMilliSeconds = 100;

    const UBYTE     arubRespEmptyFrame[] = { };
    const UBYTE     arubRespFrameToShort1[] = { 0x02 };
    const UBYTE     arubRespFrameToShort2[] = { 0x02, 0x03 };
    const UBYTE     arubRespFrameToShort3[] = { 0x02, 0x03, 0xAC };
    const UBYTE     arubRespFrameToShort4[] = { 0x02, 0x03, 0xAC, 0xDB };

    const UBYTE     arubRespIllegalFunctionCode[] = { 0x03, 0x03, 0xAC, 0xDB, 0x35 };
    const UBYTE     arubRespIllegalLength[] = { 0x02, 0x04, 0xAC, 0xDB, 0x35 };
    const UBYTE     arubRespFrameToLong1[] = { 0x02, 0x03, 0xAC, 0xDB, 0x35, 0xAA };
    const UBYTE     arubRespFrameOkay[] = { 0x02, 0x03, 0xAC, 0xDB, 0x35 };

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );

    xThreadData.pubPreparedResponse = arubRespEmptyFrame;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespEmptyFrame );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort3;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort3 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort4;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort4 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalFunctionCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalFunctionCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalLength;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalLength );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToLong1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToLong1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameOkay;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameOkay );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMReadDiscreteInputs( xMBMHdl, 1, 196, 22, ubDiscOut1 ) );
}

void
vMBMTestReadDiscreteAPI( void )
{
    xMBMInternalHandle xMBMInvalidHdl;
    UBYTE           ubDiscOut1[3];

    CU_ASSERT( MB_EINVAL == eMBMReadDiscreteInputs( NULL, 1, 0, 0, ubDiscOut1 ) );
    CU_ASSERT( MB_EINVAL == eMBMReadDiscreteInputs( xMBMHdl, 1, 0, 0, ubDiscOut1 ) );
    CU_ASSERT( MB_EINVAL == eMBMReadDiscreteInputs( xMBMHdl, 1, 1, 0, ubDiscOut1 ) );
    CU_ASSERT( MB_EINVAL == eMBMReadDiscreteInputs( xMBMHdl, 1, 1, 1, NULL ) );
    CU_ASSERT( MB_EINVAL == eMBMReadDiscreteInputs( &xMBMInvalidHdl, 1, 1, 1, ubDiscOut1 ) );
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
