/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_driver.c,v 1.8 2007-08-19 12:26:11 cwalter Exp $
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

typedef struct
{
    USHORT          usDelayMilliSeconds;
    xMBPEventType   xEvent;
} xEventThreadData;

/* ----------------------- Static variables ---------------------------------*/
static xMBMHandle xMBMHdl;

/* ----------------------- Static functions ---------------------------------*/
STATIC int      iMBMTestInit( void );
STATIC int      iMBMTestClean( void );

STATIC void     vMBMTestResponseTimeout( void );
STATIC void     vMBMTestTimerErrors( void );
STATIC void     vMBMTestEventErrors( void );
STATIC void     vMBMTestInitErrors( void );
STATIC void     vMBMTestFunctions( void );
STATIC void     vMBMTestExceptions( void );
STATIC void     vMBMTestSendReceiveErrors( void );

STATIC void    *pvMBMDelayedResponse( void *pvThreadData );
STATIC void    *pvMBMDelayedEvent( void *pvThreadData );

/* ----------------------- Start implementation -----------------------------*/

int
iMBM_AddTests( void )
{
    CU_pSuite       pSuite = NULL;

    pSuite = CU_add_suite( "CORE STACK", iMBMTestInit, iMBMTestClean );

    if( NULL == pSuite )
    {
        return -1;
    }
    if( ( NULL == CU_add_test( pSuite, "TIMEOUT TEST (BASIC TESTS)", vMBMTestResponseTimeout ) ) ||
        ( NULL == CU_add_test( pSuite, "INIT ERRORS", vMBMTestInitErrors ) ) ||
        ( NULL == CU_add_test( pSuite, "TIMER ERROR", vMBMTestTimerErrors ) ) ||
        ( NULL == CU_add_test( pSuite, "EVENT ERRORS", vMBMTestEventErrors ) ) ||
        ( NULL == CU_add_test( pSuite, "EXCEPTION", vMBMTestExceptions ) ) ||
        ( NULL == CU_add_test( pSuite, "SEND/RECEIVE ERRORS", vMBMTestSendReceiveErrors ) ) ||
        ( NULL == CU_add_test( pSuite, "SIMPLE FUNCTIONS", vMBMTestFunctions ) ) )
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

STATIC void
vMBMTestResponseTimeout( void )
{
    USHORT          pusRegBuffer[1];
    const UBYTE     arubExpectedRequest[] = { 0x03, 0x00, 0x00, 0x00, 0x01 };
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    CU_ASSERT( MB_ENOERR == eMBMSetSlaveTimeout( xMBMHdl, 1000 ) );
    CU_ASSERT( MB_ETIMEDOUT == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );
}

STATIC void
vMBMTestSendReceiveErrors( void )
{
    xEventThreadData xThreadData;
    xPreparedRespThreadData xThreadData2;
    pthread_t       xThreadHdl;
    eMBMQueryState  eState;
    eMBErrorCode    eStatus;

    /* Write Single Register, Address = 0x2178, Value = 0x3827 */
    static const UBYTE arubExpectedRequest1[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };

    xThreadData.usDelayMilliSeconds = 100;
    /* Error in sender. */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.xEvent = MBM_EV_SEND_ERROR;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedEvent, &xThreadData ) );
    CU_ASSERT( MB_ENOERR != eMBMWriteSingleRegister( xMBMHdl, 1, 0x2178, 0x3827 ) );

    /* Error in receiver. */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.xEvent = MBM_EV_RECV_ERROR;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedEvent, &xThreadData ) );
    CU_ASSERT( MB_ENOERR != eMBMWriteSingleRegister( xMBMHdl, 1, 0x2178, 0x3827 ) );

    /* Illegal event. */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.xEvent = 78;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedEvent, &xThreadData ) );
    CU_ASSERT( MB_ENOERR != eMBMWriteSingleRegister( xMBMHdl, 1, 0x2178, 0x3827 ) );

    /* Illegal state. */
    eState = ( eMBMQueryState ) 57;
    vMBMMasterTransactionPolled( ( xMBMInternalHandle * ) xMBMHdl, 1, &eState, &eStatus );
    CU_ASSERT( MB_EILLSTATE == eStatus );

    xThreadData2.usDelayMilliSeconds = 100;
    xThreadData2.pubPreparedResponse = NULL;
    xThreadData2.usPreparedResponseLength = 0;
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.xEvent = 78;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData2 ) );
    CU_ASSERT( MB_ENOERR != eMBMWriteSingleRegister( xMBMHdl, 1, 0x2178, 0x3827 ) );
}

STATIC void
vMBMTestFunctions( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;

    /* Write Single Register, Address = 0x2178, Value = 0x3827 */
    static const UBYTE arubExpectedRequest1[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };
    static const UBYTE arubPreparedResponse1[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };

    /* Write Single Register, Address = 0x2178, Value = 0x3827. For broadcast */
    static const UBYTE arubExpectedRequest2[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };
    xThreadData.usDelayMilliSeconds = 100;

    /* Write Single Register */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 1, 0x2178, 0x3827 ) );

    /* Write Single Register on broadcast address. */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest2, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 0, 0x2178, 0x3827 ) );
}

STATIC void
vMBMTestEventErrors( void )
{
    xMBMInternalHandle *pxIntHdl = xMBMHdl;

    /* Write Single Register, Address = 0x2178, Value = 0x3827. For broadcast */
    static const UBYTE arubExpectedRequest2[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };

    /* Write Single Register on broadcast address. */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest2, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
    CU_ASSERT_FATAL( MB_ENOERR == eMBPEventFailOnPost( pxIntHdl->xFrameEventHdl, TRUE ) );
    CU_ASSERT( MB_ENOERR != eMBMWriteSingleRegister( xMBMHdl, 0, 0x2178, 0x3827 ) );
    CU_ASSERT_FATAL( MB_ENOERR == eMBPEventFailOnPost( pxIntHdl->xFrameEventHdl, FALSE ) );
}

STATIC void
vMBMTestInitErrors( void )
{
    xMBMHandle      xHdl;

    vMBPTimerFailOnInit( TRUE );
    CU_ASSERT( MB_ENOERR != eMBMTestInit( &xHdl ) );
    vMBPTimerFailOnInit( FALSE );
    CU_ASSERT( MB_ENOERR == eMBMTestInit( &xHdl ) );
    CU_ASSERT( MB_ENOERR == eMBMClose( xHdl ) );

    vMBPEventFailOnInit( TRUE );
    CU_ASSERT( MB_ENOERR != eMBMTestInit( &xHdl ) );
    vMBPEventFailOnInit( FALSE );
    CU_ASSERT( MB_ENOERR == eMBMTestInit( &xHdl ) );
    CU_ASSERT( MB_ENOERR == eMBMClose( xHdl ) );
}

STATIC void
vMBMTestTimerErrors( void )
{
    USHORT          pusRegBuffer[1];
    const UBYTE     arubExpectedRequest[] = { 0x03, 0x00, 0x00, 0x00, 0x01 };
    xMBMInternalHandle *pxIntHdl = xMBMHdl;

    CU_ASSERT_FATAL( MB_ENOERR == eMBPTimerSetFailOnSetTimeout( pxIntHdl->xRespTimeoutHdl, TRUE ) );
    CU_ASSERT( MB_ENOERR != eMBMSetSlaveTimeout( xMBMHdl, 500 ) );
    CU_ASSERT_FATAL( MB_ENOERR == eMBPTimerSetFailOnSetTimeout( pxIntHdl->xRespTimeoutHdl, FALSE ) );

    CU_ASSERT_FATAL( MB_ENOERR == eMBPTimerSetFailOnStart( pxIntHdl->xRespTimeoutHdl, TRUE ) );
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );
    CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );
    CU_ASSERT_FATAL( MB_ENOERR == eMBPTimerSetFailOnStart( pxIntHdl->xRespTimeoutHdl, FALSE ) );
}

STATIC void
vMBMTestExceptions( void )
{
    CU_ASSERT( MB_EX_ILLEGAL_FUNCTION == eMBExceptionToErrorcode( 0x01 ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_ADDRESS == eMBExceptionToErrorcode( 0x02 ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_VALUE == eMBExceptionToErrorcode( 0x03 ) );
    CU_ASSERT( MB_EX_SLAVE_DEVICE_FAILURE == eMBExceptionToErrorcode( 0x04 ) );
    CU_ASSERT( MB_EX_ACKNOWLEDGE == eMBExceptionToErrorcode( 0x05 ) );
    CU_ASSERT( MB_EX_SLAVE_BUSY == eMBExceptionToErrorcode( 0x06 ) );
    CU_ASSERT( MB_EX_MEMORY_PARITY_ERROR == eMBExceptionToErrorcode( 0x08 ) );
    CU_ASSERT( MB_EX_GATEWAY_PATH_UNAVAILABLE == eMBExceptionToErrorcode( 0x0A ) );
    CU_ASSERT( MB_EX_GATEWAY_TARGET_FAILED == eMBExceptionToErrorcode( 0x0B ) );
    CU_ASSERT( MB_EIO == eMBExceptionToErrorcode( 0xFF ) );
}

STATIC void    *
pvMBMDelayedResponse( void *pvThreadData )
{
    xPreparedRespThreadData *pxData = pvThreadData;

    eMBMTestSetPreparedResponse( xMBMHdl, pxData->pubPreparedResponse, pxData->usPreparedResponseLength );
    ( void )usleep( pxData->usDelayMilliSeconds * 1000 );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_RECEIVED ) );
    return NULL;
}


STATIC void    *
pvMBMDelayedEvent( void *pvThreadData )
{
    xEventThreadData *pxData = pvThreadData;

    ( void )usleep( pxData->usDelayMilliSeconds * 1000 );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, pxData->xEvent ) );
    return NULL;
}
