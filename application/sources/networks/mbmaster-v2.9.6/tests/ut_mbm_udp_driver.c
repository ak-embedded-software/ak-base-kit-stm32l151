/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP/UDP.
 * Copyright (c) 2011 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_udp_driver.c,v 1.3 2011-12-04 20:47:18 embedded-solutions.cwalter Exp $
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
#include "ut_mbm_udp_stubs.h"

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC int      iMBMTestInit( void );
STATIC int      iMBMTestClean( void );

STATIC void     vMBMTestUDPBasicReadWrite( void );
STATIC void     vMBMTestUDPAnyAddress( void );
STATIC void     vMBMTestUDPFailOnInit( void );
STATIC void		vMBMTestUDPBasicFailOnWrite( void );
STATIC void		vMBMTestUDPBasicFailOnRead( void );
STATIC void		vMBMTestUDPBasicNoClient( void );
STATIC void		vMBMTestUDPTimeout( void );
STATIC void     vMBMTestUDPWrongTransaction( void );
STATIC void     vMBMTestUDPWrongSlaveID( void );

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
    if( ( NULL == CU_add_test( pSuite, "BASIC READ/WRITE TRANSACTION", vMBMTestUDPBasicReadWrite ) ) ||
        ( NULL == CU_add_test( pSuite, "BASIC READ/WRITE SLAVE ADDRESS", vMBMTestUDPAnyAddress ) ) ||
	    ( NULL == CU_add_test( pSuite, "TRANSACTION FAIL ON INIT", vMBMTestUDPFailOnInit ) ) ||
		(  NULL == CU_add_test( pSuite, "TRANSACTION FAIL ON WRITE", vMBMTestUDPBasicFailOnWrite ) ) ||
		(  NULL == CU_add_test( pSuite, "TRANSACTION FAIL NO CLIENT", vMBMTestUDPBasicNoClient ) ) ||
		(  NULL == CU_add_test( pSuite, "TRANSACTION TIMEOUT", vMBMTestUDPTimeout ) ) ||
        (  NULL == CU_add_test( pSuite, "TRANSACTION WRONG TRANSACTION ID", vMBMTestUDPWrongTransaction ) ) ||
		(  NULL == CU_add_test( pSuite, "TRANSACTION FAIL ON READ", vMBMTestUDPBasicFailOnRead ) ) ||
		(  NULL == CU_add_test( pSuite, "TRANSACTION WRONG SLAVE ID", vMBMTestUDPWrongSlaveID ) ) )
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
vMBMTestUDPBasicReadWrite( void )
{
    eMBErrorCode    eStatus, eStatus2;
    xMBMHandle      xMBMHdl;
    xMBPUDPHandle   xUDPHdl;

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

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 ) ) );
    if( MB_ENOERR == eStatus )
    {
        xUDPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, "127.0.0.1", 1011 ) );
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
        CU_ASSERT_EQUAL( 100, arusReadRegisters1[0] );
        CU_ASSERT_EQUAL( 200, arusReadRegisters1[1] );

        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );
        CU_ASSERT( MB_ENOERR == ( eStatus2 = eMBMReadInputRegisters( xMBMHdl, 1, 4, 2, &arusReadRegisters2[0] ) ) );
        CU_ASSERT_EQUAL( 1, arusReadRegisters2[0] );
        CU_ASSERT_EQUAL( 2, arusReadRegisters2[1] );

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestUDPAnyAddress( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPUDPHandle   xUDPHdl;
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

    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 ) ) );
    if( MB_ENOERR == eStatus )
    {
        xUDPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, "127.0.0.1", 1011 ) );
		for( iCnt = 0; iCnt < MB_UTILS_NARRSIZE( ubSlaveValidAddresses ); iCnt++, usTransactionID++ )
		{
			arubExpectedRequest1[ 0 ] = usTransactionID >> 8;
			arubExpectedRequest1[ 1 ] = usTransactionID & 0xFF;
			arubPreparedResponse1[ 0 ] = usTransactionID >> 8;
			arubPreparedResponse1[ 1 ] = usTransactionID & 0xFF;			
			arubExpectedRequest1[ 6 ] = ubSlaveValidAddresses[iCnt];
			arubPreparedResponse1[ 6 ] = ubSlaveValidAddresses[iCnt];
			vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
			vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
			CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, ubSlaveValidAddresses[iCnt], 1, 2, &arusReadRegisters1[0] ) );
			CU_ASSERT_EQUAL( 100, arusReadRegisters1[0] );
			CU_ASSERT_EQUAL( 200, arusReadRegisters1[1] );
		}
		for( iCnt = 0; iCnt < MB_UTILS_NARRSIZE( ubSlaveFailedAddresses ); iCnt++, usTransactionID++ )
		{
			CU_ASSERT( MB_EINVAL == eMBMReadHoldingRegisters( xMBMHdl, ubSlaveFailedAddresses[iCnt], 1, 2, &arusReadRegisters1[0] ) );
		}		

        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestUDPBasicFailOnWrite( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPUDPHandle   xUDPHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];
    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters2[2];
	
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 ) ) );
    if( MB_ENOERR == eStatus )
    {
        xUDPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, "127.0.0.1", 1011 ) );
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
		vMBMTestFailOnWrite( xUDPHdl, TRUE );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
	
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );	
		vMBMTestFailOnWrite( xUDPHdl, FALSE );
		CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters2[0] ) );
        CU_ASSERT_EQUAL( 100, arusReadRegisters2[0] );
        CU_ASSERT_EQUAL( 200, arusReadRegisters2[1] );
        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestUDPBasicFailOnRead( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPUDPHandle   xUDPHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];
    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters2[2];
	
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 ) ) );
    if( MB_ENOERR == eStatus )
    {
        xUDPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, "127.0.0.1", 1011 ) );
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
		vMBMTestFailOnRead( xUDPHdl, TRUE );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
	
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );	
		vMBMTestFailOnRead( xUDPHdl, FALSE );
		CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters2[0] ) );
        CU_ASSERT_EQUAL( 100, arusReadRegisters2[0] );
        CU_ASSERT_EQUAL( 200, arusReadRegisters2[1] );
        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void     
vMBMTestUDPWrongTransaction( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPUDPHandle   xUDPHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

	
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 ) ) );
    if( MB_ENOERR == eStatus )
    {
        xUDPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, "127.0.0.1", 1011 ) );
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
		CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
	}
}

STATIC void     
vMBMTestUDPWrongSlaveID( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPUDPHandle   xUDPHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x02, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];

	
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 ) ) );
    if( MB_ENOERR == eStatus )
    {
        xUDPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, "127.0.0.1", 1011 ) );
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
		CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
	}
}

STATIC void
vMBMTestUDPBasicNoClient( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPUDPHandle   xUDPHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters1[2];
	
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 ) ) );
    if( MB_ENOERR == eStatus )
    {
        xUDPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, NULL, 1011 ) );
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
	
		CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, "127.0.0.1", 1011 ) );
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse1, MB_UTILS_NARRSIZE( arubPreparedResponse1 ) );	
		CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
        CU_ASSERT_EQUAL( 100, arusReadRegisters1[0] );
        CU_ASSERT_EQUAL( 200, arusReadRegisters1[1] );
        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestUDPTimeout( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;
    xMBPUDPHandle   xUDPHdl;

    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest1[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    USHORT          arusReadRegisters1[2];
    /* Slave Address = 1, Read Holding Register, Start = 0, Length = 2 */
    UBYTE           arubExpectedRequest2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00, 0x02
    };
    UBYTE           arubPreparedResponse2[] = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x01, 0x03, 0x04, 0x00, 0x64, 0x00, 0xC8
    };
    USHORT          arusReadRegisters2[2];
	
    CU_ASSERT( MB_ENOERR == ( eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 ) ) );
    if( MB_ENOERR == eStatus )
    {
        xUDPHdl = xMBMTestGetLastHandle(  );
        CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, "127.0.0.1", 1011 ) );
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest1, MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
		vMBMTestPreparedResponse( xUDPHdl, NULL, 0 );	
        CU_ASSERT( MB_ENOERR != eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters1[0] ) );
	
		CU_ASSERT_FATAL( MB_ENOERR == eMBMUDPSetSlave( xMBMHdl, "127.0.0.1", 1011 ) );
        vMBMTestRequestTransmit( xUDPHdl, arubExpectedRequest2, MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
        vMBMTestPreparedResponse( xUDPHdl, arubPreparedResponse2, MB_UTILS_NARRSIZE( arubPreparedResponse2 ) );	
		CU_ASSERT( MB_ENOERR == eMBMReadHoldingRegisters( xMBMHdl, 1, 1, 2, &arusReadRegisters2[0] ) );
        CU_ASSERT_EQUAL( 100, arusReadRegisters2[0] );
        CU_ASSERT_EQUAL( 200, arusReadRegisters2[1] );
        CU_ASSERT( MB_ENOERR == eMBMClose( xMBMHdl ) );
    }
}

STATIC void
vMBMTestUDPFailOnInit( void )
{
    eMBErrorCode    eStatus;
    xMBMHandle      xMBMHdl;

	vMBMTestFailOnInit( TRUE );
	eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 );
	CU_ASSERT( MB_ENOERR != eStatus );
	eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 );
	CU_ASSERT( MB_ENOERR != eStatus );
	eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 );
	CU_ASSERT( MB_ENOERR != eStatus );

	vMBMTestFailOnInit( FALSE );
	eStatus = eMBMUDPInit( &xMBMHdl, "0.0.0.0", 512 );
	CU_ASSERT( MB_ENOERR == eStatus );
	eStatus = eMBMClose( xMBMHdl );
	CU_ASSERT( MB_ENOERR == eStatus );
}