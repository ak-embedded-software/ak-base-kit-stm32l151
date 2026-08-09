/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_input_driver.c,v 1.9 2011-05-22 22:29:05 embedded-solutions.cwalter Exp $
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

STATIC void     vMBMTestReadInputBasic( void );
STATIC void     vMBMTestReadInputIllegalResponse( void );
STATIC void     vMBMTestReadInputExceptions( void );
STATIC void     vMBMTestReadInputPollingInterface( void );
STATIC void     vMBMTestReadInputAPI( void );

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
    if( ( NULL == CU_add_test( pSuite, "READ INPUT REGISTERS (BASIC TESTS)", vMBMTestReadInputBasic ) ) ||
        ( NULL == CU_add_test( pSuite, "READ INPUT REGISTERS (ILLEGAL RESPONSES)", vMBMTestReadInputIllegalResponse ) )
        || ( NULL ==
             CU_add_test( pSuite, "READ INPUT REGISTERS (POLLING INTERFACE)", vMBMTestReadInputPollingInterface ) )
        || ( NULL == CU_add_test( pSuite, "READ INPUT REGISTERS (API SAFETY)", vMBMTestReadInputAPI ) )
        || ( NULL == CU_add_test( pSuite, "READ INPUT REGISTERS (EXCEPTIONS)", vMBMTestReadInputExceptions ) ) )
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
vMBMTestReadInputBasic( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;

    USHORT          usRegBuf1[5];

    /* Read Input Registers, Start Address = 0x0000, Registers = 5 */
    static const UBYTE arubExpectedRequest1[] = { 0x04, 0x00, 0x00, 0x00, 0x05 };
    static const UBYTE arubPreparedResponse1[] = {
        0x04, 0x0A, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x0, 0x04, 0x00, 0x5
    };

    xThreadData.usDelayMilliSeconds = 100;
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMReadInputRegisters( xMBMHdl, 1, 0x0000, 5, usRegBuf1 ) );
}

void
vMBMTestReadInputPollingInterface( void )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState = MBM_STATE_NONE;
    USHORT          pusRegBuffer[1];
    static const UBYTE arubExpectedRequest[] = { 0x04, 0x00, 0x00, 0x00, 0x01 };
    static const UBYTE arubPreparedResponse[] = { 0x04, 0x02, 0xA5, 0x5A };

    /* First the frame is assembled. After assembling we should be in the 
     * state MBM_STATE_SEND. 
     */
    vMBMReadInputRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_SEND );

    /* Calling the function again will pass the prepared buffer to the 
     * transmitter. We check the contents by using a fixed pattern. After
     * transmitting we are in the state waiting and wait for the reception
     * of a frame.
     */
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );
    vMBMReadInputRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_WAITING );

    /* Supply the prepared response and post an event to the queue indicating
     * that a frame has been received. */
    eMBMTestSetPreparedResponse( xMBMHdl, arubPreparedResponse, ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse ) );
    CU_ASSERT( MB_ENOERR == eMBPEventPost( ( ( xMBMInternalHandle * ) xMBMHdl )->xFrameEventHdl, MBM_EV_RECEIVED ) );
    vMBMReadInputRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DISASSEMBLE );

    /* Now the frame should be disassembled by the stack. */
    vMBMReadInputRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );
    CU_ASSERT( eStatus == MB_ENOERR );

    /* This must fail and is a violation of the API. */
    vMBMReadInputRegistersPolled( xMBMHdl, 1, 0, 1, pusRegBuffer, &eState, &eStatus );
    CU_ASSERT( eState == MBM_STATE_DONE );
    CU_ASSERT( eStatus = MB_EILLSTATE );
}

void
vMBMTestReadInputIllegalResponse( void )
{
    pthread_t       xThreadHdl;
    USHORT          pusRegBuffer[1];
    xPreparedRespThreadData xThreadData;
    const UBYTE     arubExpectedRequest[] = { 0x04, 0x00, 0x00, 0x00, 0x01 };
    xThreadData.usDelayMilliSeconds = 100;

    const UBYTE     arubRespEmptyFrame[] = { };
    const UBYTE     arubRespFrameToShort1[] = { 0x04 };
    const UBYTE     arubRespFrameToShort2[] = { 0x04, 0x02 };
    const UBYTE     arubRespFrameToShort3[] = { 0x04, 0x02 };
    const UBYTE     arubRespFrameToShort4[] = { 0x04, 0x02, 0xA5 };
    const UBYTE     arubRespIllegalFunctionCode[] = { 0x03, 0x02, 0xA5, 0x5A };
    const UBYTE     arubRespFrameOkay[] = { 0x04, 0x02, 0xA5, 0x5A };
    const UBYTE     arubRespIllegalLength1[] = { 0x04, 0x03, 0xA5, 0x5A };
    const UBYTE     arubRespIllegalLength2[] = { 0x04, 0x04, 0xA5, 0x5A };
    const UBYTE     arubRespFrameToLong[] = { 0x04, 0x02, 0xA5, 0x5A, 0xA5 };

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubRespEmptyFrame;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespEmptyFrame );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort3;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort3 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToShort4;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToShort4 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespIllegalFunctionCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalFunctionCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespIllegalLength1;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalLength1 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespIllegalLength2;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespIllegalLength2 );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameToLong;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameToLong );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMFailOnTX, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubRespFrameOkay;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubRespFrameOkay );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

}

void
vMBMTestReadInputExceptions( void )
{
    pthread_t       xThreadHdl;
    USHORT          pusRegBuffer[1];
    xPreparedRespThreadData xThreadData;
    const UBYTE     arubExpectedRequest[] = { 0x04, 0x00, 0x00, 0x00, 0x01 };
    const UBYTE     arubExIllegalDataAddressResponse[] = { 0x84, 0x02 };
    const UBYTE     arubExIllegalDataValueResponse[] = { 0x84, 0x03 };
    const UBYTE     arubExSlaveDeviceFailure[] = { 0x84, 0x04 };
    const UBYTE     arubExAcknowledge[] = { 0x84, 0x05 };
    const UBYTE     arubExSlaveDeviceBusy[] = { 0x84, 0x06 };
    const UBYTE     arubExMemoryParityError[] = { 0x84, 0x08 };
    const UBYTE     arubExGatewayPathUnavailable[] = { 0x84, 0x0A };
    const UBYTE     arubExGatewayTargetFailed[] = { 0x84, 0x0B };
    const UBYTE     arubExUnknown[] = { 0x84, 0x55 };
    const UBYTE     arubExWrongCode[] = { 0x83, 0x02 };
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataAddressResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataAddressResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_ADDRESS == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExIllegalDataValueResponse;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExIllegalDataValueResponse );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_VALUE == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceFailure;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceFailure );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_DEVICE_FAILURE == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExAcknowledge;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExAcknowledge );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ACKNOWLEDGE == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExSlaveDeviceBusy;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExSlaveDeviceBusy );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_SLAVE_BUSY == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExMemoryParityError;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExMemoryParityError );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_MEMORY_PARITY_ERROR == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExGatewayPathUnavailable;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayPathUnavailable );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_PATH_UNAVAILABLE == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExGatewayTargetFailed;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExGatewayTargetFailed );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_GATEWAY_TARGET_FAILED == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExUnknown;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExUnknown );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );

    xThreadData.pubPreparedResponse = arubExWrongCode;
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubExWrongCode );
    CU_ASSERT( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadInputRegisters( xMBMHdl, 1, 0, 1, pusRegBuffer ) );
}

void
vMBMTestReadInputAPI( void )
{
    xMBMInternalHandle xMBMInvalidHdl;
    USHORT          pusRegBuffer[1];

    CU_ASSERT( MB_EINVAL == eMBMReadInputRegisters( NULL, 1, 0, 1, pusRegBuffer ) );
    CU_ASSERT( MB_EINVAL == eMBMReadInputRegisters( xMBMHdl, 1, 65535U, 1, pusRegBuffer ) );
    CU_ASSERT( MB_EINVAL == eMBMReadInputRegisters( xMBMHdl, 1, 0, 126, pusRegBuffer ) );
    CU_ASSERT( MB_EINVAL == eMBMReadInputRegisters( xMBMHdl, 1, 0, 0, pusRegBuffer ) );
    CU_ASSERT( MB_EINVAL == eMBMReadInputRegisters( &xMBMInvalidHdl, 1, 0, 1, pusRegBuffer ) );
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
