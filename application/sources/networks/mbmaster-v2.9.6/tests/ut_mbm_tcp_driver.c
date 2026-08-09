/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007-2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_tcp_driver.c,v 1.5 2014-08-23 09:43:51 embedded-solutions.cwalter Exp $
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

STATIC void     vMBMTestTCPBasicReadWrite( void );
STATIC void     vMBMTestTCPConnectionError( void );
STATIC void     vMBMTestTCPWrongTransactionID( void );
STATIC void     vMBMTestTCPAnyAddress( void );

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
    if( /*( NULL == CU_add_test( pSuite, "BASIC READ/WRITE TRANSACTION", vMBMTestTCPBasicReadWrite ) ) ||
		( NULL == CU_add_test( pSuite, "TEST ANY ADDRESS", vMBMTestTCPAnyAddress ) ) ||*/
		( NULL == CU_add_test( pSuite, "TEST WRONG TRANSACTION ID", vMBMTestTCPWrongTransactionID ) ) /*||
        ( NULL == CU_add_test( pSuite, "CONNECTION ERRORS", vMBMTestTCPConnectionError ) ) */)
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
vMBMTestTCPBasicReadWrite( void )
{
    eMBErrorCode    eStatus, eStatus2;
    xMBMHandle      xMBMHdl;
    xMBPTCPHandle   xTCPHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

    /* Slave Address = 1, Read Input Register, Start = 5, Length = 2 */
    UBYTE           arubExpectedRequest2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x04, 0x00, 0x04, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x01, 0x04, 0x04, 0x00, 0x01, 0x00, 0x02
    };

    USHORT          arusReadRegisters2[2];

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMHdl ) ) );
    if( MB_ENOERR == eStatus )
    {
        xTCPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );
        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
        CU_ASSERT_EQUAL( 100, arusReadRegisters1[0] );
        CU_ASSERT_EQUAL( 200, arusReadRegisters1[1] );

        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        CU_ASSERT( MB_ENOERR == ( eStatus2 = eMBMReadInputRegisters( xMBMHdl, 1, 4, 2, &arusReadRegisters2[0] ) ) );
        CU_ASSERT_EQUAL( 1, arusReadRegisters2[0] );
        CU_ASSERT_EQUAL( 2, arusReadRegisters2[1] );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestTCPWrongTransactionID( void )
{
    eMBErrorCode    eStatus, eStatus2;
    xMBMHandle      xMBMHdl;
    xMBPTCPHandle   xTCPHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x02, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
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
        CU_ASSERT_EQUAL( 100, arusReadRegisters1[0] );
        CU_ASSERT_EQUAL( 200, arusReadRegisters1[1] );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}


STATIC void
vMBMTestTCPAnyAddress( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPTCPHandle   xTCPHdl;
	USHORT 			usTransactionID = 0;
	int iCnt;
	const STATIC UCHAR ubSlaveValidAddresses[] = { 0, 1, 247, 255 };
	const STATIC UCHAR ubSlaveFailedAddresses[] = { 248, 254 };
	
    /* Slave Address = 255, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xFF, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMHdl ) ) );
    if( MB_ENOERR == eStatus )
    {
        xTCPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );
		for( iCnt = 0; iCnt < MB_UTILS_NARRSIZE( ubSlaveValidAddresses ); iCnt++, usTransactionID++ )
		{
			arubExpectedRequest1[ 0 ] = usTransactionID >> 8;
			arubExpectedRequest1[ 1 ] = usTransactionID & 0xFF;
			arubPreparedResponse1[ 0 ] = usTransactionID >> 8;
			arubPreparedResponse1[ 1 ] = usTransactionID & 0xFF;			
			arubExpectedRequest1[ 6 ] = ubSlaveValidAddresses[iCnt];
			arubPreparedResponse1[ 6 ] = ubSlaveValidAddresses[iCnt];
			vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
			vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
			CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, ubSlaveValidAddresses[iCnt], 1, 2, &arusReadRegisters1[0] ) );
			CU_ASSERT_EQUAL( 100, arusReadRegisters1[0] );
			CU_ASSERT_EQUAL( 200, arusReadRegisters1[1] );
		}
		for( iCnt = 0; iCnt < MB_UTILS_NARRSIZE( ubSlaveFailedAddresses ); iCnt++ )
		{
			CU_ASSERT( MB_EINVAL == eMBMReadHoldingRegisters( xMBMHdl, ubSlaveFailedAddresses[iCnt], 1, 2, &arusReadRegisters1[0] ) );
		}		
		for( iCnt = 0; iCnt < MB_UTILS_NARRSIZE( ubSlaveValidAddresses ); iCnt++, usTransactionID++ )
		{
			arubExpectedRequest1[ 0 ] = usTransactionID >> 8;
			arubExpectedRequest1[ 1 ] = usTransactionID & 0xFF;
			arubPreparedResponse1[ 0 ] = usTransactionID >> 8;
			arubPreparedResponse1[ 1 ] = usTransactionID & 0xFF;			
			arubExpectedRequest1[ 6 ] = ubSlaveValidAddresses[iCnt];
			arubPreparedResponse1[ 6 ] = ubSlaveValidAddresses[iCnt];
			vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
			vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
			CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, ubSlaveValidAddresses[iCnt], 1, 2, &arusReadRegisters1[0] ) );
			CU_ASSERT_EQUAL( 100, arusReadRegisters1[0] );
			CU_ASSERT_EQUAL( 200, arusReadRegisters1[1] );
		}
        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestTCPConnectionError( void )
{
    eMBErrorCode    eStatus, eStatus2;
    xMBMHandle      xMBMHdl;
    xMBPTCPHandle   xTCPHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {

        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMHdl ) ) );
    if( MB_ENOERR == eStatus )
    {
        xTCPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );
        CU_ASSERT( MB_EIO == eMBMTCPConnect( xMBMHdl, "127.0.0.2", 502 ) );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMTCPConnect( xMBMHdl, "127.0.0.1", 502 ) );
        vMBMTestRequestTransmit( xTCPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xTCPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == ( eStatus2 = eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) ) );
        CU_ASSERT_EQUAL( 100, arusReadRegisters1[0] );
        CU_ASSERT_EQUAL( 200, arusReadRegisters1[1] );

        CU_ASSERT( MB_ENOERR == eMBMTCPDisconnect( xMBMHdl ) );
        CU_ASSERT( MB_EIO == ( eStatus2 = eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) ) );
        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}
