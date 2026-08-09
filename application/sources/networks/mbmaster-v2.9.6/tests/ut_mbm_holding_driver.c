/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_holding_driver.c,v 1.13 2011-11-22 00:41:26 embedded-solutions.cwalter Exp $
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

STATIC void     vMBMTestReadHoldingExceptions( void );
STATIC void     vMBMTestReadHoldingBasic( void );
STATIC void     vMBMTestReadHoldingPollingInterface( void );
STATIC void     vMBMTestReadHoldingIllegalResponse( void );
STATIC void     vMBMTestReadHoldingAPI( void );

STATIC void     vMBMTestWriteSingleRegisterExceptions( void );
STATIC void     vMBMTestWriteSingleRegisterBasic( void );
STATIC void     vMBMTestWriteSingleRegisterIllegalResponse( void );
STATIC void     vMBMTestWriteSingleRegisterPollingInterface( void );
STATIC void     vMBMTestWriteSingleRegisterAPI( void );

STATIC void     vMBMTestWriteMultipleRegistersExceptions( void );
STATIC void     vMBMTestWriteMultipleRegistersBasic( void );
STATIC void     vMBMTestWriteMultipleRegistersPollingInterface( void );
STATIC void     vMBMTestWriteMultipleRegistersIllegalResponse( void );
STATIC void     vMBMTestWriteMultipleRegistersAPI( void );

STATIC void     vMBMTestReadWriteMultipleRegistersBasic( void );
STATIC void     vMBMTestReadWriteMultipleRegistersPollingInterface( void );
STATIC void     vMBMTestReadWriteMultipleRegistersAPI( void );
STATIC void     vMBMTestReadWriteMultipleRegistersExceptions( void );
STATIC void     vMBMTestReadWriteMultipleRegistersIllegalResponse( void );

STATIC /*@null@ */ void *pvMBMDelayedResponse( void *pvThreadData );

STATIC /*@null@ */ void *pvMBMFailOnTX( void *pvThreadData );

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
    if( ( NULL == CU_add_test( pSuite, "READ HOLDING REGISTERS (BASIC TESTS)",
                               vMBMTestReadHoldingBasic ) )
        || ( NULL == CU_add_test( pSuite, "READ HOLDING REGISTERS (ILLEGAL RESPONSES)",
                                  vMBMTestReadHoldingIllegalResponse ) )
        || ( NULL == CU_add_test( pSuite, "READ HOLDING REGISTERS (POLLING INTERFACE)",
                                  vMBMTestReadHoldingPollingInterface ) )
        || ( NULL == CU_add_test( pSuite, "READ HOLDING REGISTERS (EXCEPTIONS)",
                                  vMBMTestReadHoldingExceptions ) )
        || ( NULL == CU_add_test( pSuite, "READ HOLDING REGISTERS (API SAFETY)",
                                  vMBMTestReadHoldingAPI ) )
        || ( NULL == CU_add_test( pSuite, "WRITE SINGLE REGISTER (BASIC TESTS)",
                                  vMBMTestWriteSingleRegisterBasic ) )
        || ( NULL == CU_add_test( pSuite, "WRITE SINGLE REGISTER (ILLEGAL RESPONSES)",
                                  vMBMTestWriteSingleRegisterIllegalResponse ) )
        || ( NULL == CU_add_test( pSuite, "WRITE SINGLE REGISTER (POLLING INTERFACE)",
                                  vMBMTestWriteSingleRegisterPollingInterface ) )
        || ( NULL == CU_add_test( pSuite, "WRITE SINGLE REGISTER (EXCEPTIONS)",
                                  vMBMTestWriteSingleRegisterExceptions ) )
        || ( NULL == CU_add_test( pSuite, "WRITE SINGLE REGISTER (API SAFETY)",
                                  vMBMTestWriteSingleRegisterAPI ) )
        || ( NULL == CU_add_test( pSuite, "WRITE MULTIPLE REGISTERS (BASIC TESTS)",
                                  vMBMTestWriteMultipleRegistersBasic ) )
        || ( NULL == CU_add_test( pSuite, "WRITE MULTIPLE REGISTERS (ILLEGAL RESPONSES)",
                                  vMBMTestWriteMultipleRegistersIllegalResponse ) )
        || ( NULL == CU_add_test( pSuite, "WRITE MULTIPLE REGISTERS (POLLING INTERFACE)",
                                  vMBMTestWriteMultipleRegistersPollingInterface ) )
        || ( NULL == CU_add_test( pSuite, "WRITE MULTIPLE REGISTERS (EXCEPTIONS)",
                                  vMBMTestWriteMultipleRegistersExceptions ) )
        || ( NULL == CU_add_test( pSuite, "WRITE MULTIPLE REGISTERS (API SAFETY)",
                                  vMBMTestWriteMultipleRegistersAPI ) )
        || ( NULL == CU_add_test( pSuite, "READ/WRITE MULTIPLE REGISTERS (BASIC TESTS)",
                                  vMBMTestReadWriteMultipleRegistersBasic ) )
        || ( NULL == CU_add_test( pSuite, "READ/WRITE MULTIPLE REGISTERS (ILLEGAL RESPONES)",
                                  vMBMTestReadWriteMultipleRegistersIllegalResponse ) )
        || ( NULL == CU_add_test( pSuite, "READ/WRITE MULTIPLE REGISTERS (POLLING INTERFACE)",
                                  vMBMTestReadWriteMultipleRegistersPollingInterface ) )
        || ( NULL == CU_add_test( pSuite, "READ/WRITE MULTIPLE REGISTERS (EXCEPTION)",
                                  vMBMTestReadWriteMultipleRegistersExceptions ) )
        || ( NULL == CU_add_test( pSuite, "READ/WRITE MULTIPLE REGISTERS (API SAFETY)",
                                  vMBMTestReadWriteMultipleRegistersAPI ) ) )
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
vMBMTestReadWriteMultipleRegistersBasic( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;

    /* Write Multiple Registers,
     * Write Address = 0x0100, Write Values = 0x0102, 0x0304, 0x0506
     * Read Address = 0x1050, Registers to read = 2
     */
    const USHORT    ubNRegsIn1[] = { 0x0102, 0x0304, 0x0506 };
    USHORT          ubNRegsOut1[2];
    const UBYTE     arubExpectedRequest1[] = {
        /* addr */ 0x17,
        /* read */ 0x10, 0x50, 0x00, 0x02,
        /* write */ 0x01, 0x00, 0x00, 0x03, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x17, 0x04, 0x11, 0x22, 0x33, 0x44
    };

    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );
    CU_ASSERT_EQUAL( ubNRegsOut1[0], 0x1122U );
    CU_ASSERT_EQUAL( ubNRegsOut1[1], 0x3344U );

}

void
vMBMTestReadWriteMultipleRegistersPollingInterface( void )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    /* Write Multiple Registers,
     * Write Address = 0x0100, Write Values = 0x0102, 0x0304, 0x0506
     * Read Address = 0x1050, Registers to read = 2
     */
    const USHORT    ubNRegsIn1[] = { 0x0102, 0x0304, 0x0506 };
    USHORT          ubNRegsOut1[2];
    const UBYTE     arubExpectedRequest1[] = {
        /* addr */ 0x17,
        /* read */ 0x10, 0x50, 0x00, 0x02,
        /* write */ 0x01, 0x00, 0x00, 0x03, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x17, 0x04, 0x11, 0x22, 0x33, 0x44
    };

    vMBMReadWriteMultipleRegistersPolled( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1, &eState,
                                          &eStatus );
    CU_ASSERT( eState == MBM_STATE_SEND );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    vMBMReadWriteMultipleRegistersPolled( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1, &eState,
                                          &eStatus );
    CU_ASSERT( eState == MBM_STATE_WAITING );

    eMBMTestSetPreparedResponse( xMBMHdl, arubPreparedResponse1,
                                 ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_RECEIVED ) );
    vMBMReadWriteMultipleRegistersPolled( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1, &eState,
                                          &eStatus );
    CU_ASSERT( eState == MBM_STATE_DISASSEMBLE );

    vMBMReadWriteMultipleRegistersPolled( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1, &eState,
                                          &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );

    vMBMReadWriteMultipleRegistersPolled( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1, &eState,
                                          &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );
    CU_ASSERT( eStatus == MB_EILLSTATE );
}

void
vMBMTestReadWriteMultipleRegistersAPI( void )
{

    const USHORT    ubNRegsIn1[3] = { 0, 0, 0 };
    USHORT          ubNRegsOut1[2];
    xMBMInternalHandle xMBMInvalidHdl;

    CU_ASSERT( MB_EINVAL ==
               eMBMReadWriteMultipleRegisters( &xMBMInvalidHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );
    CU_ASSERT( MB_EINVAL == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, NULL, 0x1050, 2, ubNRegsOut1 ) );
    CU_ASSERT( MB_EINVAL == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, NULL ) );
    CU_ASSERT( MB_EINVAL ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 0, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );
    CU_ASSERT( MB_EINVAL ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 248, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );
    CU_ASSERT( MB_EINVAL ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0xFFFF, 1, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );
    CU_ASSERT( MB_EINVAL ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0xFFFF, 1, ubNRegsOut1 ) );
    CU_ASSERT( MB_EINVAL ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 0x7A, ubNRegsIn1, 0xFFFF, 1, ubNRegsOut1 ) );
    CU_ASSERT( MB_EINVAL ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0xFFFF, 0x7E, ubNRegsOut1 ) );
}

void
vMBMTestReadWriteMultipleRegistersExceptions( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;

    /* Write Multiple Registers,
     * Write Address = 0x0100, Write Values = 0x0102, 0x0304, 0x0506
     * Read Address = 0x1050, Registers to read = 2
     */
    const USHORT    ubNRegsIn1[] = { 0x0102, 0x0304, 0x0506 };
    USHORT          ubNRegsOut1[2];
    const UBYTE     arubExpectedRequest[] = {
        /* addr */ 0x17,
        /* read */ 0x10, 0x50, 0x00, 0x02,
        /* write */ 0x01, 0x00, 0x00, 0x03, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };

    const UBYTE     arubExIllegalDataAddressResponse[] = { 0x97, 0x02 };
    const UBYTE     arubExIllegalDataValueResponse[] = { 0x97, 0x03 };
    const UBYTE     arubExSlaveDeviceFailure[] = { 0x97, 0x04 };
    const UBYTE     arubExAcknowledge[] = { 0x97, 0x05 };
    const UBYTE     arubExSlaveDeviceBusy[] = { 0x97, 0x06 };
    const UBYTE     arubExMemoryParityError[] = { 0x97, 0x08 };
    const UBYTE     arubExGatewayPathUnavailable[] = { 0x97, 0x0A };
    const UBYTE     arubExGatewayTargetFailed[] = { 0x97, 0x0B };
    const UBYTE     arubExUnknown[] = { 0x97, 0x55 };
    const UBYTE     arubExWrongCode[] = { 0x98, 0x02 };
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataAddressResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataAddressResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_ADDRESS ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataValueResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataValueResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_VALUE ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceFailure;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceFailure );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_DEVICE_FAILURE ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubExAcknowledge;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExAcknowledge );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ACKNOWLEDGE ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceBusy;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceBusy );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_BUSY ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubExMemoryParityError;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExMemoryParityError );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_MEMORY_PARITY_ERROR ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubExGatewayPathUnavailable;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayPathUnavailable );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_PATH_UNAVAILABLE ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubExGatewayTargetFailed;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayTargetFailed );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_TARGET_FAILED ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubExUnknown;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExUnknown );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubExWrongCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExWrongCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

}

void
vMBMTestReadWriteMultipleRegistersIllegalResponse( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;

    /* Write Multiple Registers,
     * Write Address = 0x0100, Write Values = 0x0102, 0x0304, 0x0506
     * Read Address = 0x1050, Registers to read = 2
     */
    const USHORT    ubNRegsIn1[] = { 0x0102, 0x0304, 0x0506 };
    USHORT          ubNRegsOut1[2];
    const UBYTE     arubExpectedRequest[] = {
        /* addr */ 0x17,
        /* read */ 0x10, 0x50, 0x00, 0x02,
        /* write */ 0x01, 0x00, 0x00, 0x03, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };
    const UBYTE     arubPreparedResponse[] = {
        0x17, 0x04, 0x11, 0x22, 0x33, 0x44
    };
    xThreadData.usDelayMilliSeconds = 100;

    const UBYTE     arubRespEmptyFrame[] = { };
    const UBYTE     arubRespFrameToShort1[] = { 0x17 };
    const UBYTE     arubRespFrameToShort2[] = { 0x17, 0x04 };
    const UBYTE     arubRespFrameToShort3[] = { 0x17, 0x04 };
    const UBYTE     arubRespFrameToShort4[] = { 0x17, 0x04, 0x11 };
    const UBYTE     arubRespFrameToShort5[] = { 0x17, 0x04, 0x11, 0x22 };
    const UBYTE     arubRespFrameToShort6[] = { 0x17, 0x04, 0x11, 0x22 };
    const UBYTE     arubRespFrameToShort7[] = { 0x17, 0x04, 0x11, 0x22, 0x33 };
    const UBYTE     arubRespIllegalFunctionCode[] = { 0x16, 0x04, 0x11, 0x22, 0x33, 0x44 };
    const UBYTE     arubRespIllegalLength1[] = { 0x17, 0x03, 0x11, 0x22, 0x33, 0x44 };
    const UBYTE     arubRespIllegalLength2[] = { 0x03, 0x05, 0x11, 0x22, 0x33, 0x44 };
    const UBYTE     arubRespFrameToLong[] = { 0x03, 0x04, 0x11, 0x22, 0x33, 0x44, 0x55 };

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubRespEmptyFrame;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespEmptyFrame );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort3;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort3 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort4;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort4 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort5;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort5 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort6;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort6 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort7;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort7 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalFunctionCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalFunctionCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalLength1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalLength1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalLength2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalLength2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToLong;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToLong );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMFailOnTX, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );

    xThreadData.pubPreparedResponse = arubPreparedResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR ==
               eMBMReadWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegsIn1, 0x1050, 2, ubNRegsOut1 ) );
}

void
vMBMTestWriteMultipleRegistersBasic( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;

    /* Write Multiple Registers, Address = 0x0100, Values = 0x0102, 0x0304, 0x0506 */
    static const USHORT ubNRegs1[] = { 0x0102, 0x0304, 0x0506 };
    static const UBYTE arubExpectedRequest1[] = {
        0x10, 0x01, 0x00, 0x00, 0x03, 0x06,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };
    static const UBYTE arubPreparedResponse1[] = {
        0x10, 0x01, 0x00, 0x00, 0x03
    };

    /* Write Multiple Registers, Address = 0x0001, Values = 0x0809, 0x0A0B */
    static const USHORT ubNRegs2[] = { 0x0809, 0x0A0B };
    static const UBYTE arubExpectedRequest2[] = {
        0x10, 0x00, 0x01, 0x00, 0x02, 0x04,
        0x08, 0x09, 0x0A, 0x0B
    };
    static const UBYTE arubPreparedResponse2[] = {
        0x10, 0x00, 0x01, 0x00, 0x02
    };

    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs1 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest2, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse2 );
    xThreadData.pubPreparedResponse = arubPreparedResponse2;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0001, 2, ubNRegs2 ) );
}

void
vMBMTestWriteMultipleRegistersPollingInterface( void )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    /* Write Multiple Registers, Address = 0x0100, Values = 0x0102, 0x0304, 0x0506 */
    static const USHORT ubNRegs[] = { 0x0102, 0x0304, 0x0506 };
    static const UBYTE arubExpectedRequest[] = {
        0x10, 0x01, 0x00, 0x00, 0x03, 0x06,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };
    static const UBYTE arubPreparedResponse[] = {
        0x10, 0x01, 0x00, 0x00, 0x03
    };

    vMBMWriteMultipleRegistersPolled( xMBMHdl, 1, 0x0100, 3, ubNRegs, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_SEND );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );
    vMBMWriteMultipleRegistersPolled( xMBMHdl, 1, 0x0100, 3, ubNRegs, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_WAITING );

    eMBMTestSetPreparedResponse( xMBMHdl, arubPreparedResponse, ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse ) );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_RECEIVED ) );
    vMBMWriteMultipleRegistersPolled( xMBMHdl, 1, 0x0100, 3, ubNRegs, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DISASSEMBLE );

    vMBMWriteMultipleRegistersPolled( xMBMHdl, 1, 0x0100, 3, ubNRegs, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );

    /* Same test but we send to the broadcast address. */
    eState = MBM_STATE_NONE;
    vMBMWriteMultipleRegistersPolled( xMBMHdl, 0, 0x0100, 3, ubNRegs, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_SEND );

    /* Now the frame should be transmitted. */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );
    vMBMWriteMultipleRegistersPolled( xMBMHdl, 0, 0x0100, 3, ubNRegs, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_WAITING );

    /* No response therefore we should be finished. */
    vMBMWriteMultipleRegistersPolled( xMBMHdl, 0, 0x0100, 3, ubNRegs, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );
    CU_ASSERT( eStatus == MB_ENOERR );

    /* No response therefore we should be finished. */
    vMBMWriteMultipleRegistersPolled( xMBMHdl, 0, 0x0100, 3, ubNRegs, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );
    CU_ASSERT( eStatus == MB_EILLSTATE );
}

void
vMBMTestWriteMultipleRegistersExceptions( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;

    /* Write Multiple Registers, Address = 0x0100, Values = 0x0102, 0x0304, 0x0506 */
    static const USHORT ubNRegs[] = { 0x0102, 0x0304, 0x0506 };
    static const UBYTE arubExpectedRequest[] = {
        0x10, 0x01, 0x00, 0x00, 0x03, 0x06,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };

    const UBYTE     arubExIllegalDataAddressResponse[] = { 0x90, 0x02 };
    const UBYTE     arubExIllegalDataValueResponse[] = { 0x90, 0x03 };
    const UBYTE     arubExSlaveDeviceFailure[] = { 0x90, 0x04 };
    const UBYTE     arubExAcknowledge[] = { 0x90, 0x05 };
    const UBYTE     arubExSlaveDeviceBusy[] = { 0x90, 0x06 };
    const UBYTE     arubExMemoryParityError[] = { 0x90, 0x08 };
    const UBYTE     arubExGatewayPathUnavailable[] = { 0x90, 0x0A };
    const UBYTE     arubExGatewayTargetFailed[] = { 0x90, 0x0B };
    const UBYTE     arubExUnknown[] = { 0x90, 0x55 };
    const UBYTE     arubExWrongCode[] = { 0xA0, 0x02 };
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataAddressResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataAddressResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_ADDRESS == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataValueResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataValueResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_VALUE == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceFailure;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceFailure );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_DEVICE_FAILURE == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubExAcknowledge;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExAcknowledge );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ACKNOWLEDGE == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceBusy;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceBusy );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_BUSY == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubExMemoryParityError;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExMemoryParityError );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_MEMORY_PARITY_ERROR == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubExGatewayPathUnavailable;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayPathUnavailable );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_PATH_UNAVAILABLE == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubExGatewayTargetFailed;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayTargetFailed );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_TARGET_FAILED == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubExUnknown;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExUnknown );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubExWrongCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExWrongCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );
}


void
vMBMTestWriteMultipleRegistersIllegalResponse( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;

    /* Write Multiple Registers, Address = 0x0100, Values = 0x0102, 0x0304, 0x0506 */
    static const USHORT ubNRegs[] = { 0x0102, 0x0304, 0x0506 };
    static const UBYTE arubExpectedRequest[] = {
        0x10, 0x01, 0x00, 0x00, 0x03, 0x06,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };

    const UBYTE     arubRespEmptyFrame[] = { };
    const UBYTE     arubRespFrameToShort1[] = { 0x10 };
    const UBYTE     arubRespFrameToShort2[] = { 0x10, 0x01 };
    const UBYTE     arubRespFrameToShort3[] = { 0x10, 0x01, 0x00 };
    const UBYTE     arubRespFrameToShort4[] = { 0x10, 0x01, 0x00, 0x00 };
    const UBYTE     arubRespFrameToShort5[] = { 0x10, 0x01, 0x00, 0x00, 0x05 };
    const UBYTE     arubRespIllegalAddress[] = { 0x10, 0x01, 0x01, 0x00, 0x03 };
    const UBYTE     arubRespIllegalNRegs[] = { 0x10, 0x01, 0x00, 0x00, 0x02 };
    const UBYTE     arubRespIllegalFunctionCode[] = { 0x11, 0x01, 0x00, 0x00, 0x03 };
    const UBYTE     arubRespFrameOkay[] = { 0x10, 0x01, 0x00, 0x00, 0x03 };

    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubRespEmptyFrame;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespEmptyFrame );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort3;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort3 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort4;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort4 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort5;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort5 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubRespIllegalAddress;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalAddress );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubRespIllegalNRegs;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalNRegs );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubRespIllegalFunctionCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalFunctionCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );

    xThreadData.pubPreparedResponse = arubRespFrameOkay;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameOkay );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0x0100, 3, ubNRegs ) );
}

void
vMBMTestWriteMultipleRegistersAPI( void )
{
    static const USHORT usRegs[2];
    xMBMInternalHandle xMBMInvalidHdl;

	/* no longer possible cause correct address is now checked deeper in the
	 * chain.
	 */
    /* CU_ASSERT( MB_EINVAL == eMBMWriteMultipleRegisters( xMBMHdl, 248, 0, 2, usRegs ) ); */
    CU_ASSERT( MB_EINVAL == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0, 2, NULL ) );
    CU_ASSERT( MB_EINVAL == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0, 0, usRegs ) );
    CU_ASSERT( MB_EINVAL == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0, 124, usRegs ) );
    CU_ASSERT( MB_EINVAL == eMBMWriteMultipleRegisters( xMBMHdl, 1, 0xFFFFU, 2, usRegs ) );
    CU_ASSERT( MB_EINVAL == eMBMWriteMultipleRegisters( &xMBMInvalidHdl, 1, 0, 2, usRegs ) );
}

void
vMBMTestWriteSingleRegisterBasic( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;

    /* Write Single Register, Address = 0x0000, Value = 0xFFFF (Boundary) */
    static const UBYTE arubExpectedRequest1[] = { 0x06, 0x00, 0x00, 0xFF, 0xFF };
    static const UBYTE arubPreparedResponse1[] = { 0x06, 0x00, 0x00, 0xFF, 0xFF };

    /* Write Single Register, Address = 0x0000, Value = 0x0000 (Boundary) */
    static const UBYTE arubExpectedRequest2[] = { 0x06, 0x00, 0x00, 0x00, 0x00 };
    static const UBYTE arubPreparedResponse2[] = { 0x06, 0x00, 0x00, 0x00, 0x00 };

    /* Write Single Register, Address = 0xFFFF, Value = 0xFFFF (Boundary) */
    static const UBYTE arubExpectedRequest3[] = { 0x06, 0xFF, 0xFF, 0xFF, 0xFF };
    static const UBYTE arubPreparedResponse3[] = { 0x06, 0xFF, 0xFF, 0xFF, 0xFF };

    /* Write Single Register, Address = 0xFFFF, Value = 0x0000 (Boundary) */
    static const UBYTE arubExpectedRequest4[] = { 0x06, 0xFF, 0xFF, 0x00, 0x00 };
    static const UBYTE arubPreparedResponse4[] = { 0x06, 0xFF, 0xFF, 0x00, 0x00 };

    /* Write Single Register, Address = 0x2178, Value = 0x3827 */
    static const UBYTE arubExpectedRequest5[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };
    static const UBYTE arubPreparedResponse5[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };

    /* Write Single Register, Address = 0x2178, Value = 0x3827. For broadcast */
    static const UBYTE arubExpectedRequest6[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0xFFFF ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest2, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse2 );
    xThreadData.pubPreparedResponse = arubPreparedResponse2;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x0000 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest3, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest3 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse3 );
    xThreadData.pubPreparedResponse = arubPreparedResponse3;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 1, 0xFFFF, 0xFFFF ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest4, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest4 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse4 );
    xThreadData.pubPreparedResponse = arubPreparedResponse4;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 1, 0xFFFF, 0x0000 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest5, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest5 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse5 );
    xThreadData.pubPreparedResponse = arubPreparedResponse5;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 1, 0x2178, 0x3827 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest6, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest6 ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 0, 0x2178, 0x3827 ) );
}

void
vMBMTestWriteSingleRegisterIllegalResponse( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;
    const UBYTE     arubExpectedRequest[] = { 0x06, 0x12, 0x34, 0x56, 0x78 };
    xThreadData.usDelayMilliSeconds = 100;

    const UBYTE     arubRespEmptyFrame[] = { };
    const UBYTE     arubRespFrameToShort1[] = { 0x06 };
    const UBYTE     arubRespFrameToShort2[] = { 0x06, 0x12 };
    const UBYTE     arubRespFrameToShort3[] = { 0x06, 0x12 };
    const UBYTE     arubRespFrameToShort4[] = { 0x06, 0x12, 0x34 };
    const UBYTE     arubRespFrameToShort5[] = { 0x06, 0x12, 0x34, 0x56 };
    const UBYTE     arubRespIllegalFunctionCode[] = { 0x04, 0x12, 0x34, 0x56, 0x78 };
    const UBYTE     arubRespIllegalAddress[] = { 0x06, 0x12, 0x35, 0x56, 0x78 };
    const UBYTE     arubRespIllegalValue[] = { 0x06, 0x12, 0x35, 0x56, 0x78 };
    const UBYTE     arubRespFrameToLong1[] = { 0x06, 0x12, 0x34, 0x56, 0x78, 0x78 };
    const UBYTE     arubRespFrameOkay[] = { 0x06, 0x12, 0x34, 0x56, 0x78 };

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubRespEmptyFrame;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespEmptyFrame );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort3;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort3 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort4;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort4 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort5;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort5 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalFunctionCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalFunctionCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalAddress;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalAddress );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespIllegalValue;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalValue );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespFrameToLong1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToLong1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    xThreadData.pubPreparedResponse = arubRespFrameOkay;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameOkay );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x05678 ) );

    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMFailOnTX, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x1234, 0x5678 ) );
}

void
vMBMTestWriteSingleRegisterPollingInterface( void )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;

    /* Write Single Register, Address = 0x2178, Value = 0x3827 */
    static const UBYTE arubExpectedRequest[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };
    static const UBYTE arubPreparedResponse[] = { 0x06, 0x21, 0x78, 0x38, 0x27 };

    /* First the frame is assembled. After assembling we should be in the 
     * state MBM_STATE_SEND. 
     */
    vMBMWriteSingleRegisterPolled( xMBMHdl, 1, 0x2178, 0x3827, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_SEND );

    /* Calling the function again will pass the prepared buffer to the 
     * transmitter. We check the contents by using a fixed pattern. After
     * transmitting we are in the state waiting and wait for the reception
     * of a frame.
     */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );
    vMBMWriteSingleRegisterPolled( xMBMHdl, 1, 0x2178, 0x3827, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_WAITING );

    /* Supply the prepared response and post an event to the queue indicating
     * that a frame has been received. */
    eMBMTestSetPreparedResponse( xMBMHdl, arubPreparedResponse, ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse ) );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_RECEIVED ) );
    vMBMWriteSingleRegisterPolled( xMBMHdl, 1, 0x2178, 0x3827, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DISASSEMBLE );

    /* Now the frame should be disassembled by the stack. */
    vMBMWriteSingleRegisterPolled( xMBMHdl, 1, 0x2178, 0x3827, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );

    /* Same test but we send to the broadcast address. Therefore we will
     * get no response.
     */
    eState = MBM_STATE_NONE;
    vMBMWriteSingleRegisterPolled( xMBMHdl, 0, 0x2178, 0x3827, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_SEND );

    /* Now the frame should be transmitted. */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );
    vMBMWriteSingleRegisterPolled( xMBMHdl, 0, 0x2178, 0x3827, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_WAITING );

    /* No response therefore we should be finished. */
    vMBMWriteSingleRegisterPolled( xMBMHdl, 0, 0x2178, 0x3827, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );
    CU_ASSERT( eStatus == MB_ENOERR );

    /* No response therefore we should be finished. */
    vMBMWriteSingleRegisterPolled( xMBMHdl, 0, 0x2178, 0x3827, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );
    CU_ASSERT( eStatus == MB_EILLSTATE );
}

void
vMBMTestWriteSingleRegisterAPI( void )
{
    xMBMInternalHandle xMBMInvalidHdl;

    CU_ASSERT( MB_EINVAL == eMBMWriteSingleRegister( NULL, 1, 0, 0 ) );
	/* no longer possible cause correct address is now checked deeper in the
	 * chain.
	 */
    /* CU_ASSERT( MB_EINVAL == eMBMWriteSingleRegister( xMBMHdl, 248, 0, 0 ) ); */
    CU_ASSERT( MB_EINVAL == eMBMWriteSingleRegister( &xMBMInvalidHdl, 1, 0, 0 ) );
}

void
vMBMTestWriteSingleRegisterExceptions( void )
{
    pthread_t       xThreadHdl;
    xPreparedRespThreadData xThreadData;
    const UBYTE     arubExpectedRequest[] = { 0x06, 0x00, 0x00, 0x12, 0x34 };
    const UBYTE     arubExIllegalDataAddressResponse[] = { 0x86, 0x02 };
    const UBYTE     arubExIllegalDataValueResponse[] = { 0x86, 0x03 };
    const UBYTE     arubExSlaveDeviceFailure[] = { 0x86, 0x04 };
    const UBYTE     arubExAcknowledge[] = { 0x86, 0x05 };
    const UBYTE     arubExSlaveDeviceBusy[] = { 0x86, 0x06 };
    const UBYTE     arubExMemoryParityError[] = { 0x86, 0x08 };
    const UBYTE     arubExGatewayPathUnavailable[] = { 0x86, 0x0A };
    const UBYTE     arubExGatewayTargetFailed[] = { 0x86, 0x0B };
    const UBYTE     arubExUnknown[] = { 0x86, 0x55 };
    const UBYTE     arubExWrongCode[] = { 0x87, 0x02 };
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataAddressResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataAddressResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_ADDRESS == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataValueResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataValueResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_VALUE == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceFailure;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceFailure );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_DEVICE_FAILURE == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );

    xThreadData.pubPreparedResponse = arubExAcknowledge;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExAcknowledge );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ACKNOWLEDGE == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceBusy;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceBusy );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_BUSY == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );

    xThreadData.pubPreparedResponse = arubExMemoryParityError;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExMemoryParityError );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_MEMORY_PARITY_ERROR == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );

    xThreadData.pubPreparedResponse = arubExGatewayPathUnavailable;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayPathUnavailable );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_PATH_UNAVAILABLE == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );

    xThreadData.pubPreparedResponse = arubExGatewayTargetFailed;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayTargetFailed );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_TARGET_FAILED == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );

    xThreadData.pubPreparedResponse = arubExUnknown;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExUnknown );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );

    xThreadData.pubPreparedResponse = arubExWrongCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExWrongCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMWriteSingleRegister( xMBMHdl, 1, 0x0000, 0x1234 ) );
}

void
vMBMTestReadHoldingPollingInterface( void )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;
    USHORT          pusRegBuffer[1];
    static const UBYTE arubExpectedRequest[] = { 0x03, 0x00, 0x00, 0x00, 0x01 };
    static const UBYTE arubPreparedResponse[] = { 0x03, 0x02, 0xA5, 0x5A };

    /* First the frame is assembled. After assembling we should be in the 
     * state MBM_STATE_SEND. 
     */
    vMBMReadHoldingRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_SEND );

    /* Calling the function again will pass the prepared buffer to the 
     * transmitter. We check the contents by using a fixed pattern. After
     * transmitting we are in the state waiting and wait for the reception
     * of a frame.
     */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );
    vMBMReadHoldingRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_WAITING );

    /* Supply the prepared response and post an event to the queue indicating
     * that a frame has been received. */
    eMBMTestSetPreparedResponse( xMBMHdl, arubPreparedResponse, ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse ) );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_RECEIVED ) );
    vMBMReadHoldingRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DISASSEMBLE );

    /* Now the frame should be disassembled by the stack. */
    vMBMReadHoldingRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );
    CU_ASSERT( eStatus == MB_ENOERR );

    /* This must fail and is a violation of the API. */
    vMBMReadHoldingRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );
    CU_ASSERT( eStatus = MB_EILLSTATE );
}

void
vMBMTestReadHoldingBasic( void )
{
    pthread_t       xThreadHdl;
    USHORT          pusRegBuffer[125];
    xPreparedRespThreadData xThreadData;

    const UBYTE     nReadSizeTests[] = { 1, 125 };

    UBYTE           arubExpectedRequest[5];
    UBYTE           arubPreparedResponse[1 + 2 * 125];
    int             i, j;

    xThreadData.usDelayMilliSeconds = 100;

    for( i = 0; i < MB_UTILS_NARRSIZE( nReadSizeTests ); i++ )
    {
        arubExpectedRequest[0] = 0x03;
        arubExpectedRequest[1] = 0x00;
        arubExpectedRequest[2] = 0x10;  /* start address = 0x10 */
        arubExpectedRequest[3] = 0x00;
        arubExpectedRequest[4] = nReadSizeTests[i];
        eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

        arubPreparedResponse[0] = 0x03;
        arubPreparedResponse[1] = 2 * nReadSizeTests[i];
        for( j = 0; j < nReadSizeTests[i]; j++ )
        {
            arubPreparedResponse[2 + 2 * j] = j;
            arubPreparedResponse[3 + 2 * j] = j + 1;
        }
        xThreadData.pubPreparedResponse = arubPreparedResponse;
        xThreadData.usPreparedResponseLength = 2 + 2 * nReadSizeTests[i];
        CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
        memset( pusRegBuffer, 0xFF, sizeof( pusRegBuffer ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0x10, nReadSizeTests[i], pusRegBuffer ) );
        for( j = 0; j < nReadSizeTests[i]; j++ )
        {
        CU_ASSERT_EQUAL( pusRegBuffer[j], ( USHORT ) ( ( j << 8 ) | ( ( j + 1 ) & 0xFF ) ) )}
    }

}

void
vMBMTestReadHoldingIllegalResponse( void )
{
    pthread_t       xThreadHdl;
    USHORT          pusRegBuffer[1];
    xPreparedRespThreadData xThreadData;
    const UBYTE     arubExpectedRequest[] = { 0x03, 0x00, 0x00, 0x00, 0x01 };
    xThreadData.usDelayMilliSeconds = 100;

    const UBYTE     arubRespEmptyFrame[] = { };
    const UBYTE     arubRespFrameToShort1[] = { 0x03 };
    const UBYTE     arubRespFrameToShort2[] = { 0x03, 0x02 };
    const UBYTE     arubRespFrameToShort3[] = { 0x03, 0x02 };
    const UBYTE     arubRespFrameToShort4[] = { 0x03, 0x02, 0xA5 };
    const UBYTE     arubRespIllegalFunctionCode[] = { 0x04, 0x02, 0xA5, 0x5A };
    const UBYTE     arubRespIllegalLength1[] = { 0x03, 0x03, 0xA5, 0x5A };
    const UBYTE     arubRespIllegalLength2[] = { 0x03, 0x04, 0xA5, 0x5A };
    const UBYTE     arubRespFrameToLong[] = { 0x03, 0x02, 0xA5, 0x5A, 0xA5 };

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubRespEmptyFrame;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespEmptyFrame );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort3;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort3 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort4;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort4 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespIllegalFunctionCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalFunctionCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespIllegalLength1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalLength1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespIllegalLength2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalLength2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToLong;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToLong );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMFailOnTX, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );
}

void
vMBMTestReadHoldingAPI( void )
{
    xMBMInternalHandle xMBMInvalidHdl;
    USHORT          pusRegBuffer[1];

    CU_ASSERT( MB_EINVAL == eMBMReadHoldingRegisters( NULL, 1, 0, 1, pusRegBuffer ) );
    CU_ASSERT( MB_EINVAL == eMBMReadHoldingRegisters( xMBMHdl, 1, 65535U, 1, pusRegBuffer ) );
    CU_ASSERT( MB_EINVAL == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 126, pusRegBuffer ) );
    CU_ASSERT( MB_EINVAL == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 0, pusRegBuffer ) );
    CU_ASSERT( MB_EINVAL == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, NULL ) );
	/* no longer possible cause correct address is now checked deeper in the
	 * chain.
	 */
    /* CU_ASSERT( MB_EINVAL == eMBMReadHoldingRegisters( xMBMHdl, 248, 0, 1, pusRegBuffer ) ); */
    CU_ASSERT( MB_EINVAL == eMBMReadHoldingRegisters( &xMBMInvalidHdl, 1, 0, 1, pusRegBuffer ) );
}

void
vMBMTestReadHoldingExceptions( void )
{
    pthread_t       xThreadHdl;
    USHORT          pusRegBuffer[1];
    xPreparedRespThreadData xThreadData;
    const UBYTE     arubExpectedRequest[] = { 0x03, 0x00, 0x00, 0x00, 0x01 };
    const UBYTE     arubExIllegalDataAddressResponse[] = { 0x83, 0x02 };
    const UBYTE     arubExIllegalDataValueResponse[] = { 0x83, 0x03 };
    const UBYTE     arubExSlaveDeviceFailure[] = { 0x83, 0x04 };
    const UBYTE     arubExAcknowledge[] = { 0x83, 0x05 };
    const UBYTE     arubExSlaveDeviceBusy[] = { 0x83, 0x06 };
    const UBYTE     arubExMemoryParityError[] = { 0x83, 0x08 };
    const UBYTE     arubExGatewayPathUnavailable[] = { 0x83, 0x0A };
    const UBYTE     arubExGatewayTargetFailed[] = { 0x83, 0x0B };
    const UBYTE     arubExUnknown[] = { 0x83, 0x55 };
    const UBYTE     arubExWrongCode[] = { 0x84, 0x02 };
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataAddressResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataAddressResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_ADDRESS == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataValueResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataValueResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_VALUE == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceFailure;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceFailure );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_DEVICE_FAILURE == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExAcknowledge;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExAcknowledge );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ACKNOWLEDGE == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceBusy;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceBusy );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_BUSY == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExMemoryParityError;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExMemoryParityError );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_MEMORY_PARITY_ERROR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExGatewayPathUnavailable;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayPathUnavailable );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_PATH_UNAVAILABLE == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExGatewayTargetFailed;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayTargetFailed );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_TARGET_FAILED == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExUnknown;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExUnknown );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExWrongCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExWrongCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );
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

void           *
pvMBMFailOnTX( void *pvThreadData )
{
    ( void )usleep( 1000 );
    CU_ASSERT_FATAL( MB_ENOERR ==
                     eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_SEND_ERROR ) );
    return NULL;
}
