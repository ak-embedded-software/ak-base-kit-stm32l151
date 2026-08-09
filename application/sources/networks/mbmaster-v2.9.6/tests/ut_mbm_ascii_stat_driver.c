/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_ascii_stat_driver.c,v 1.1 2008-11-16 00:25:13 cwalter Exp $
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

STATIC void     vMBMTestASCIIBasicSimple( void );
STATIC void     vMBMTestASCIIBasicReset( void );
STATIC void     vMBMTestASCIIBasicLRCFail( void );
STATIC void     vMBMTestASCIIimeout( void );

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
    if( ( NULL == CU_add_test( pSuite, "SIMPLE", vMBMTestASCIIBasicSimple ) ) ||
        ( NULL == CU_add_test( pSuite, "LRC", vMBMTestASCIIBasicLRCFail ) ) ||
        ( NULL == CU_add_test( pSuite, "RESET", vMBMTestASCIIBasicReset ) ) ||
        ( NULL == CU_add_test( pSuite, "TIMEOUT", vMBMTestASCIIimeout ) ) )
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
vMBMTestASCIIBasicSimple( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBStat         xCurrentStat;

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
    USHORT          arusReadRegisters1[10];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNBytesReceived );

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest1 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( sizeof( arubPreparedResponse1 ), xCurrentStat.ulNBytesReceived );

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 2, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 2, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest1 ) + sizeof( arubExpectedRequest1 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( sizeof( arubPreparedResponse1 ) + sizeof( arubPreparedResponse1 ),
                         xCurrentStat.ulNBytesReceived );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIBasicLRCFail( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBStat         xCurrentStat;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    UBYTE           arubPreparedInvalidResponse1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x41, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x34, 0x35, 0x37, 0x30,
        0x38, 0x41, 0x45, 0x30, 0x44, 0x30, 0x35, 0x31,
        0x31, 0x35, 0x43, 0x36, 0x22, 0x0D, 0x0A
    };
    USHORT          arusReadRegisters1[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedInvalidResponse1, MB_UTILS_NARRSIZE( arubPreparedInvalidResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );

        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest1 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( sizeof( arubPreparedInvalidResponse1 ), xCurrentStat.ulNBytesReceived );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIBasicReset( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBStat         xCurrentStat;

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
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        /* Send first request and check */
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest1 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( sizeof( arubPreparedResponse1 ), xCurrentStat.ulNBytesReceived );

        /* Reset statistics */
        CU_ASSERT( MB_ENOERR == eMBMResetStatistics( xMBMHdl ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNBytesReceived );

        /* Verify if still works */
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest1 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( sizeof( arubPreparedResponse1 ), xCurrentStat.ulNBytesReceived );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIimeout( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBStat         xCurrentStat;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 5 */
    UBYTE           arubExpectedRequest1[] = {
        0x3A, 0x30, 0x31, 0x30, 0x33, 0x30, 0x30, 0x30,
        0x30, 0x30, 0x30, 0x30, 0x35, 0x46, 0x37, 0x0D,
        0x0A
    };
    USHORT          arusReadRegisters1[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 7, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_ASCII, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        /* Send first request and check */
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 5, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest1 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNBytesReceived );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}
