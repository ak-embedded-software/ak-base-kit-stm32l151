/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_rtu_v2_stat_driver.c,v 1.1 2008-11-16 17:55:13 cwalter Exp $
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

STATIC void     vMBMTestRTUBasicSimple( void );
STATIC void     vMBMTestRTUBasicReset( void );
STATIC void     vMBMTestRTUBasicCRCFail( void );
STATIC void     vMBMTestRTUTimeout( void );

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
    if( ( NULL == CU_add_test( pSuite, "SIMPLE", vMBMTestRTUBasicSimple ) ) ||
        ( NULL == CU_add_test( pSuite, "CRC", vMBMTestRTUBasicCRCFail ) ) ||
        ( NULL == CU_add_test( pSuite, "RESET", vMBMTestRTUBasicReset ) ) ||
        ( NULL == CU_add_test( pSuite, "TIMEOUT", vMBMTestRTUTimeout ) ) )
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
vMBMTestRTUBasicSimple( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBStat         xCurrentStat;

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
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNBytesReceived );

        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 10, &arusReadRegisters1[0] ) );
        CU_ASSERT( MB_ENOERR == ( eStatus = eMBMGetStatistics( xMBMHdl, &xCurrentStat ) ) );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsSent );
        CU_ASSERT_EQUAL( 1, xCurrentStat.ulNPacketsReceived );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNTimeouts );
        CU_ASSERT_EQUAL( 0, xCurrentStat.ulNChecksumErrors );
        CU_ASSERT_EQUAL( sizeof( arubExpectedRequest1 ), xCurrentStat.ulNBytesSent );
        CU_ASSERT_EQUAL( sizeof( arubPreparedResponse1 ), xCurrentStat.ulNBytesReceived );

        vMBMTestRequestTransmit( 0, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );
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
vMBMTestRTUBasicCRCFail( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBStat         xCurrentStat;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest1[] = {
        0x01, 0x03, 0x00, 0x05, 0x00, 0x05, 0x95, 0xC8
    };
    UBYTE           arubPreparedInvalidResponse1[] = {
        0x01, 0x03, 0x0A, 0x15, 0xB3, 0x1A, 0x0A, 0x1E,
        0x61, 0x22, 0xB8, 0x27, 0x0F, 0xFF, 0xFF
    };
    USHORT          arusReadRegisters2[5];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT_FATAL( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedInvalidResponse1, MB_UTILS_NARRSIZE( arubPreparedInvalidResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 5, 5, &arusReadRegisters2[0] ) );

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
vMBMTestRTUBasicReset( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBStat         xCurrentStat;

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

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        /* Send first request and check */
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( 0, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 10, &arusReadRegisters1[0] ) );
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
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 10, &arusReadRegisters1[0] ) );
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
vMBMTestRTUTimeout( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBStat         xCurrentStat;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 10 */
    UBYTE           arubExpectedRequest1[] = {
        0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0xC5, 0xCD
    };
    USHORT          arusReadRegisters1[10];

    vMBMTestSetFailOnInit( FALSE );
    vMBMTestSetExpectedSerialInit( 0, 38400, 8, 2, MB_PAR_NONE );
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) );
    if( MB_ENOERR == eStatus )
    {
        /* Send first request and check */
        vMBMTestRequestTransmit( 0, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        CU_ASSERT( MB_ETIMEDOUT == eMBMReadHoldingRegisters( xMBMHdl, 1, 0, 10, &arusReadRegisters1[0] ) );
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
