/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_ascii_v2_driver.c,v 1.2 2011-05-22 22:29:05 embedded-solutions.cwalter Exp $
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

STATIC void     vMBMTestASCIIBasicReadWrite( void );
STATIC void     vMBMTestASCIIBasicTimeout( void );
STATIC void     vMBMTestASCIIReadWriteRetry( void );
STATIC void     vMBMTestASCIIBasicRXOverrun( void );
STATIC void     vMBMTestASCIIBasicLRCFail( void );
STATIC void     vMBMTestASCIIBasicFailOnTx( void );
STATIC void     vMBMTestASCIIBasicFailOnRx( void );
STATIC void     vMBMTestASCIIFailOnInit( void );

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
    if( ( NULL == CU_add_test( pSuite, "BASIC READ/WRITE TRANSACTION", vMBMTestASCIIBasicReadWrite ) ) ||
        ( NULL == CU_add_test( pSuite, "LRC ERROR", vMBMTestASCIIBasicLRCFail ) ) ||
        ( NULL == CU_add_test( pSuite, "WRITE ERROR", vMBMTestASCIIBasicFailOnTx ) ) ||
        ( NULL == CU_add_test( pSuite, "TRANSMISSION RETRY", vMBMTestASCIIReadWriteRetry ) ) ||
        ( NULL == CU_add_test( pSuite, "RX OVERRUN (OVERLONG FRAME)", vMBMTestASCIIBasicRXOverrun ) ) ||
        ( NULL == CU_add_test( pSuite, "READ ERROR", vMBMTestASCIIBasicFailOnRx ) ) ||
        ( NULL == CU_add_test( pSuite, "TIMEOUT", vMBMTestASCIIBasicTimeout ) ) ||
        ( NULL == CU_add_test( pSuite, "INIT ERRORS", vMBMTestASCIIFailOnInit ) ) )
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
vMBMTestASCIIBasicLRCFail( void )
{
    eMBErrorCode    eStatus;
    xMBHandle       xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubPreparedResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x22, 0x0D, 0x0A
    };
    USHORT          arusReadRegisters1[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIBasicReadWrite( void )
{
    eMBErrorCode    eStatus;
    xMBHandle       xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubPreparedResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D, 0x0A
    };

    /* Slave Address = 0, Write Single Register, Address = 0, Value = 0 */
    UBYTE           arubExpectedRequest2[] = {
        0x3A, 0x30, 0x30, 0x30, 0x36, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x30, 0x46, 0x41, 0x0D,
        0x0A
    };

    USHORT          arusReadRegisters1[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        vMBMTestRequestTransmit( 0, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        CU_ASSERT( MB_ENOERR == eMBMWriteSingleRegister( xMBMHdl, 0, 0, 0 ) );
        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIReadWriteRetry( void )
{
    eMBErrorCode    eStatus;
    xMBHandle       xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubPreparedResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34,

        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D, 0x0A
    };
    UBYTE           arubPreparedResponse2[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D,
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D, 0x0A
    };
    USHORT          arusReadRegisters1[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIBasicRXOverrun( void )
{
    eMBErrorCode    eStatus;
    xMBHandle       xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubPreparedResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D, 0x0A
    };
    UBYTE           arubPreparedResponse2[514];
    USHORT          arusReadRegisters1[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        memset( arubPreparedResponse2, 0, sizeof( arubPreparedResponse2 ) );
        arubPreparedResponse2[0] = 0x3A;
        arubPreparedResponse2[512] = 0x0D;
        arubPreparedResponse2[513] = 0x0A;

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse2, 514 );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        memset( arubPreparedResponse2, 0, sizeof( arubPreparedResponse2 ) );
        arubPreparedResponse2[0] = 0x3A;
        arubPreparedResponse2[512] = 0x00;
        arubPreparedResponse2[513] = 0x00;

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse2, 514 );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}


STATIC void
vMBMTestASCIIBasicFailOnTx( void )
{
    eMBErrorCode    eStatus;
    xMBHandle       xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubPreparedResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D, 0x0A
    };
    USHORT          arusReadRegisters1[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        /* Fail on a TX. */
        vMBMTestFailOnTxStart( 0, TRUE );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        /* Must still work after fail on TX on next request. */
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        vMBMTestFailOnTxStart( 0, FALSE );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );
        CU_ASSERT_EQUAL( 0000, arusReadRegisters1[0] );
        CU_ASSERT_EQUAL( 1111, arusReadRegisters1[1] );
        CU_ASSERT_EQUAL( 2222, arusReadRegisters1[2] );
        CU_ASSERT_EQUAL( 3333, arusReadRegisters1[3] );
        CU_ASSERT_EQUAL( 4444, arusReadRegisters1[4] );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIBasicFailOnRx( void )
{
    eMBErrorCode    eStatus;
    xMBHandle       xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubPreparedResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D, 0x0A
    };
    USHORT          arusReadRegisters1[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestFailOnRxStart( 0, TRUE );
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        /* Must still work after fail on RX on next request. */
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        vMBMTestFailOnRxStart( 0, FALSE );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );
        CU_ASSERT_EQUAL( 0000, arusReadRegisters1[0] );
        CU_ASSERT_EQUAL( 1111, arusReadRegisters1[1] );
        CU_ASSERT_EQUAL( 2222, arusReadRegisters1[2] );
        CU_ASSERT_EQUAL( 3333, arusReadRegisters1[3] );
        CU_ASSERT_EQUAL( 4444, arusReadRegisters1[4] );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIBasicTimeout(  )
{
    eMBErrorCode    eStatus;
    xMBHandle       xMBMHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubPreparedResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D, 0x0A
    };
    UBYTE           arubPreparedResponse2[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D
    };
    USHORT          arusReadRegisters1[5];

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest3[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    /* Illegal response - Instead of an LF at the end an illegal character
     * is received.
     */
    UBYTE           arubPreparedResponse3[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x32, 0x0D, 0xFF
    };

    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        /* This timeout is due because no response is received from the slave. */
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestSetResponseTimeoutOnRx( 0, TRUE );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ETIMEDOUT == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );
        vMBMTestSetResponseTimeoutOnRx( 0, FALSE );

        /* This timeout is due to an intercharacter timeout. */
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        CU_ASSERT( MB_ENOERR == eMBMSetSlaveTimeout( xMBMHdl, 10000 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        /* This timeout is due to a missing start character where the receiver is
         * put into the error state. */
        vMBMTestRequestTransmit( 0, arubExpectedRequest3, MB_UTILS_NARRSIZE( arubExpectedRequest3 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse3, MB_UTILS_NARRSIZE( arubPreparedResponse3 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}


STATIC void
vMBMTestASCIIFailOnInit( void )
{
    xMBHandle       xMBMHdl1, xMBMHdl2, xMBMHdl3;

    /* Illegal arguments (baudrate = 0 ) */
    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 0, 7, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR != eMBMSerialInit( &xMBMHdl1, MB_ASCII, 0, 0, MB_PAR_NONE ) );

    /* Fail on init. */
    vMBMTestSetFailOnInit( TRUE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR != eMBMSerialInit( &xMBMHdl1, MB_ASCII, 0, 38400, MB_PAR_NONE ) );

    /* Allocate to many stacks. */
    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMSerialInit( &xMBMHdl1, MB_ASCII, 0, 38400, MB_PAR_NONE ) );
    vMBMTestSetExpectedSerialInit( 1, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMSerialInit( &xMBMHdl2, MB_ASCII, 1, 38400, MB_PAR_NONE ) );
    vMBMTestSetExpectedSerialInit( 2, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENORES == eMBMSerialInit( &xMBMHdl3, MB_ASCII, 1, 38400, MB_PAR_NONE ) );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl1 ) );
    CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl2 ) );
}
