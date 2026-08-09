/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_rtu_driver.c,v 1.12 2007-08-19 12:26:11 cwalter Exp $
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

STATIC void     vMBMTestRTUBasicReadWrite( void );
STATIC void     vMBMTestRTUBasicFailOnTx( void );
STATIC void     vMBMTestRTUBasicFailOnRx( void );
STATIC void     vMBMTestRTUBasicCRCFail( void );
STATIC void     vMBMTestRTUBasicRXOverrun( void );
STATIC void     vMBMTestRTUBasicTimeout( void );
STATIC void     vMBMTestRTUBasicWrongSlaveAnswered( void );
STATIC void     vMBMTestRTUBasicBroadcast( void );
STATIC void     vMBMTestRTUMultiStack( void );

STATIC void     vMBMTestRTUFailOnInit( void );

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
    if( ( NULL == CU_add_test( pSuite, "BASIC READ/WRITE TRANSACTION", vMBMTestRTUBasicReadWrite ) ) ||
        ( NULL == CU_add_test( pSuite, "WRITE ERROR", vMBMTestRTUBasicFailOnTx ) ) ||
        ( NULL == CU_add_test( pSuite, "READ ERROR", vMBMTestRTUBasicFailOnRx ) ) ||
        ( NULL == CU_add_test( pSuite, "CRC ERROR", vMBMTestRTUBasicCRCFail ) ) ||
        ( NULL == CU_add_test( pSuite, "RX OVERRUN (OVERLONG FRAME)", vMBMTestRTUBasicRXOverrun ) ) ||
        ( NULL == CU_add_test( pSuite, "WRONG SLAVE ANSWERED", vMBMTestRTUBasicWrongSlaveAnswered ) ) ||
        ( NULL == CU_add_test( pSuite, "TIMEOUT", vMBMTestRTUBasicTimeout ) ) ||
        ( NULL == CU_add_test( pSuite, "BROADCAST", vMBMTestRTUBasicBroadcast ) ) ||
        ( NULL == CU_add_test( pSuite, "INIT ERRORS", vMBMTestRTUFailOnInit ) ) ||
        ( NULL == CU_add_test( pSuite, "MULTIPLE STACKS", vMBMTestRTUMultiStack ) ) )
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

STATIC void
vMBMTestRTUBasicReadWrite( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;

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

    /* Slave Address = 1, Read Holding Register, Start = 5, Length = 5 */
    UBYTE           arubExpectedRequest2[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedResponse2[] = {
        0x01, 0x03, 0x0A, 0x15, 0xB3, 0x1A, 0x0A, 0x1E,
        0x61, 0x22, 0xB8, 0x27, 0x0F, 0x50, 0xA8
    };
    USHORT          arusReadRegisters2[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 10, &arusReadRegisters1[0] ) );
        CU_ASSERT_EQUAL( 0x19A5, arusReadRegisters1[3] );

        vMBMTestRequestTransmit( 0, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );
        CU_ASSERT_EQUAL( 5555, arusReadRegisters2[0] );
        CU_ASSERT_EQUAL( 6666, arusReadRegisters2[1] );
        CU_ASSERT_EQUAL( 7777, arusReadRegisters2[2] );
        CU_ASSERT_EQUAL( 8888, arusReadRegisters2[3] );
        CU_ASSERT_EQUAL( 9999, arusReadRegisters2[4] );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestRTUBasicFailOnTx( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest2[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedResponse2[] = {
        0x01, 0x03, 0x0A, 0x15, 0xB3, 0x1A, 0x0A, 0x1E,
        0x61, 0x22, 0xB8, 0x27, 0x0F, 0x50, 0xA8
    };
    USHORT          arusReadRegisters2[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        /* Fail on a TX. */
        vMBMTestFailOnTxStart( 0, TRUE );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );

        /* Must still work after fail on TX on next request. */
        vMBMTestRequestTransmit( 0, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        vMBMTestFailOnTxStart( 0, FALSE );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );
        CU_ASSERT_EQUAL( 5555, arusReadRegisters2[0] );
        CU_ASSERT_EQUAL( 6666, arusReadRegisters2[1] );
        CU_ASSERT_EQUAL( 7777, arusReadRegisters2[2] );
        CU_ASSERT_EQUAL( 8888, arusReadRegisters2[3] );
        CU_ASSERT_EQUAL( 9999, arusReadRegisters2[4] );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestRTUBasicFailOnRx( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest1[] = {
        0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0xC5, 0xCD
    };

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest2[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedResponse2[] = {
        0x01, 0x03, 0x0A, 0x15, 0xB3, 0x1A, 0x0A, 0x1E,
        0x61, 0x22, 0xB8, 0x27, 0x0F, 0x50, 0xA8
    };
    USHORT          arusReadRegisters2[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestFailOnRxStart( 0, TRUE );
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 10, &arusReadRegisters2[0] ) );

        /* Must still work after fail on RX on next request. */
        vMBMTestRequestTransmit( 0, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        vMBMTestFailOnRxStart( 0, FALSE );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );
        CU_ASSERT_EQUAL( 5555, arusReadRegisters2[0] );
        CU_ASSERT_EQUAL( 6666, arusReadRegisters2[1] );
        CU_ASSERT_EQUAL( 7777, arusReadRegisters2[2] );
        CU_ASSERT_EQUAL( 8888, arusReadRegisters2[3] );
        CU_ASSERT_EQUAL( 9999, arusReadRegisters2[4] );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestRTUBasicCRCFail( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest2[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedInvalidResponse2[] = {
        0x01, 0x03, 0x0A, 0x15, 0xB3, 0x1A, 0x0A, 0x1E,
        0x61, 0x22, 0xB8, 0x27, 0x0F, 0xFF, 0xFF
    };
    USHORT          arusReadRegisters2[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( 0, arubPreparedInvalidResponse2, MB_UTILS_NARRSIZE( arubPreparedInvalidResponse2 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestRTUBasicRXOverrun( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest1[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedResponse1[258];

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest2[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedResponse2[] = {
        0x01, 0x03, 0x0A, 0x15, 0xB3, 0x1A, 0x0A, 0x1E,
        0x61, 0x22, 0xB8, 0x27, 0x0F, 0x50, 0xA8
    };
    USHORT          arusReadRegisters2[5];

    memset( arubPreparedResponse1, 0xFF, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, 257 );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, 258 );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );

        vMBMTestRequestTransmit( 0, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestRTUBasicWrongSlaveAnswered( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest1[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedResponse1[] = {
        0x02, 0x03, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x75
    };
    USHORT          arusReadRegisters2[5];

    memset( arubPreparedResponse1, 0xFF, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestRTUBasicTimeout(  )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedResponse[] = {
        0x01, 0x03, 0x0A, 0x15, 0xB3, 0x1A, 0x0A, 0x1E,
        0x61, 0x22, 0xB8, 0x27, 0x0F, 0x50, 0xA8
    };
    USHORT          arusReadRegisters[5];

    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest, MB_UTILS_NARRSIZE( arubExpectedRequest ) );
        vMBMTestSetResponseTimeoutOnRx( 0, TRUE );
        vMBMTestPreparedResponse( 0, arubPreparedResponse, MB_UTILS_NARRSIZE( arubPreparedResponse ) );
        CU_ASSERT( MB_ETIMEDOUT == eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters[0] ) );

        vMBMTestSetResponseTimeoutOnRx( 0, FALSE );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestRTUBasicBroadcast(  )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;

    /* Slave Address = 0, Write Single Register = 0, Value = 0 */
    UBYTE           arubExpectedRequest[] = {
        0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x88, 0x1B
    };

    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest, MB_UTILS_NARRSIZE( arubExpectedRequest ) );
        CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 0, 0x0000, 0x0000 ) );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void    *
vMBMTestRTUMultiStackThread1( void *pvArg )
{
    xMBMHandle      xMBMHdl = pvArg;
    int             iCnt = 10;
    UBYTE           arubExpectedRequest[] = {
        0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0xC5, 0xCD
    };
    UBYTE           arubPreparedResponse[] = {
        0x01, 0x03, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x19, 0xA5, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37,
        0x1A
    };
    USHORT          arusReadRegisters[10];

    do
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest, MB_UTILS_NARRSIZE( arubExpectedRequest ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse, MB_UTILS_NARRSIZE( arubPreparedResponse ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 10, &arusReadRegisters[0] ) );
        CU_ASSERT_EQUAL( 0x19A5, arusReadRegisters[3] );
        usleep( 50 );
    }
    while( iCnt-- > 0 );
    return NULL;
}

STATIC void    *
vMBMTestRTUMultiStackThread2( void *pvArg )
{
    xMBMHandle      xMBMHdl = pvArg;
    int             iCnt = 10;
    UBYTE           arubExpectedRequest[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedResponse[] = {
        0x01, 0x03, 0x0A, 0x15, 0xB3, 0x1A, 0x0A, 0x1E,
        0x61, 0x22, 0xB8, 0x27, 0x0F, 0x50, 0xA8
    };
    USHORT          arusReadRegisters[5];

    do
    {
        vMBMTestRequestTransmit( 1, arubExpectedRequest, MB_UTILS_NARRSIZE( arubExpectedRequest ) );
        vMBMTestPreparedResponse( 1, arubPreparedResponse, MB_UTILS_NARRSIZE( arubPreparedResponse ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters[0] ) );
        CU_ASSERT_EQUAL( 5555, arusReadRegisters[0] );
        CU_ASSERT_EQUAL( 6666, arusReadRegisters[1] );
        CU_ASSERT_EQUAL( 7777, arusReadRegisters[2] );
        CU_ASSERT_EQUAL( 8888, arusReadRegisters[3] );
        CU_ASSERT_EQUAL( 9999, arusReadRegisters[4] );
        usleep( 100 );
    }
    while( iCnt-- > 0 );
    return NULL;
}

STATIC void
vMBMTestRTUMultiStack( void )
{
    xMBMHandle      xMBMHdl1, xMBMHdl2;
    pthread_t       xThreadHdl1, xThreadHdl2;

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMSerialInit( &xMBMHdl1, MB_RTU, 0, 38400, MB_PAR_NONE ) );
    vMBMTestSetExpectedSerialInit( 1, 19200, 8, 1, MB_PAR_ODD );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMSerialInit( &xMBMHdl2, MB_RTU, 1, 19200, MB_PAR_ODD ) );

    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl1, NULL, vMBMTestRTUMultiStackThread1, xMBMHdl1 ) );
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl2, NULL, vMBMTestRTUMultiStackThread2, xMBMHdl2 ) );


    CU_ASSERT( 0 == pthread_join( xThreadHdl1, NULL ) );
    CU_ASSERT( 0 == pthread_join( xThreadHdl2, NULL ) );

    CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl1 ) );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl2 ) );
}

STATIC void
vMBMTestRTUFailOnInit( void )
{
    xMBMHandle      xMBMHdl1, xMBMHdl2, xMBMHdl3;

    /* Illegal arguments (baudrate = 0 ) */
    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 0, 8, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR != eMBMSerialInit( &xMBMHdl1, MB_RTU, 0, 0, MB_PAR_NONE ) );

    /* Fail on init. */
    vMBMTestSetFailOnInit( TRUE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR != eMBMSerialInit( &xMBMHdl1, MB_RTU, 0, 38400, MB_PAR_NONE ) );

    /* Allocate to many stacks. */
    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMSerialInit( &xMBMHdl1, MB_RTU, 0, 38400, MB_PAR_NONE ) );
    vMBMTestSetExpectedSerialInit( 1, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMSerialInit( &xMBMHdl2, MB_RTU, 1, 38400, MB_PAR_NONE ) );
    vMBMTestSetExpectedSerialInit( 2, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENORES == eMBMSerialInit( &xMBMHdl3, MB_RTU, 1, 38400, MB_PAR_NONE ) );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl1 ) );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl2 ) );
}
