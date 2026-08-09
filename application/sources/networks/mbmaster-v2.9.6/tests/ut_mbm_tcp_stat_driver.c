/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_tcp_stat_driver.c,v 1.1 2008-11-16 00:25:13 cwalter Exp $
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
#include "ut_mbm_tcp_stubs.h"

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
    xMBPTCPHandle   xTCPHdl;
    xMBStat         xCurrentStat;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest2[] = {

        0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMHdl ) ) );
    if( MB_ENOERR == eStatus )
    {
        xTCPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNBytesReceived );

        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest1 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( sizeof( arubPreparedResponse1 ), xCurrentStat.ulNBytesReceived );

        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 2, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 2, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest1 ) + sizeof( arubExpectedRequest2 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( sizeof( arubPreparedResponse1 ) + sizeof( arubPreparedResponse2 ),
                         xCurrentStat.ulNBytesReceived );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIBasicLRCFail( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPTCPHandle   xTCPHdl;
    xMBStat         xCurrentStat;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedInvalidResponse1[] = {
        0x00, 0x00, 0x30, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMHdl ) ) );
    if( MB_ENOERR == eStatus )
    {
        xTCPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );

        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedInvalidResponse1,
                                  MB_UTILS_NARRSIZE( arubPreparedInvalidResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );

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
    xMBPTCPHandle   xTCPHdl;
    xMBStat         xCurrentStat;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest2[] = {

        0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMHdl ) ) );
    if( MB_ENOERR == eStatus )
    {
        xTCPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );

        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
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
        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest2 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( sizeof( arubPreparedResponse2 ), xCurrentStat.ulNBytesReceived );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestASCIIimeout( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPTCPHandle   xTCPHdl;
    xMBStat         xCurrentStat;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    USHORT          arusReadRegisters1[2];

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMHdl ) ) );
    if( MB_ENOERR == eStatus )
    {
        xTCPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );

        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        CU_ASSERT( MB_ETIMEDOUT == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
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
