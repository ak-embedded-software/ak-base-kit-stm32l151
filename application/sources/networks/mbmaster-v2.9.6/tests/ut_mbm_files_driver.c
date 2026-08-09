/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_files_driver.c,v 1.1 2011-05-22 22:29:05 embedded-solutions.cwalter Exp $
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

STATIC void     vMBMTestReadFilesBasic( void );
STATIC void     vMBMTestReadFilesException( void );
STATIC void     vMBMTestReadFilesIllegalResponses( void );

STATIC void     vMBMTestWriteFilesBasic( void );

STATIC void    *pvMBMDelayedResponse( void *pvThreadData );

/* ----------------------- Start implementation -----------------------------*/

int
iMBM_AddTests( void )
{
    CU_pSuite       pSuite = NULL;

    pSuite = CU_add_suite( "READ FILES", iMBMTestInit, iMBMTestClean );

    if( NULL == pSuite )
    {
        return -1;
    }
    if( ( NULL == CU_add_test( pSuite, "READ FILES (BASIC TESTS)", vMBMTestReadFilesBasic ) ) ||
        ( NULL == CU_add_test( pSuite, "READ FILES (EXCEPTIONS)", vMBMTestReadFilesException ) ) ||
        ( NULL == CU_add_test( pSuite, "READ FILES (ILLEGAL RESPONSES)", vMBMTestReadFilesIllegalResponses ) ) ||
        ( NULL == CU_add_test( pSuite, "WRITE FILES (BASIC TESTS)", vMBMTestWriteFilesBasic ) ) )
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
vMBMTestReadFilesBasic( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;
    xMBMFileSubReadReq_t arxReadReq1[2];
    xMBMFileSubReadResp_t arxReadResp1[2];
    const UBYTE     arubExpectedRequest1[] = {
        0x14, 0x0E,
        0x06, 0x00, 0x04, 0x00, 0x01, 0x00, 0x02,
        0x06, 0x00, 0x03, 0x00, 0x09, 0x00, 0x02
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x014, 0x0C,
        0x05, 0x06, 0x0D, 0xFE, 0x00, 0x20,
        0x05, 0x06, 0x33, 0xCD, 0x00, 0x40
    };
    xMBMFileSubReadReq_t arxReadReq2[1];
    xMBMFileSubReadResp_t arxReadResp2[1];
    const UBYTE     arubExpectedRequest2[] = {
        0x14, 0x07,
        0x06, 0x00, 0x04, 0x00, 0x01, 0x00, 0x03
    };
    const UBYTE     arubPreparedResponse2[] = {
        0x014, 0x08,
        0x07, 0x06, 0x0D, 0xFE, 0x00, 0x20, 0xAA, 0xBB
    };

    arxReadReq1[0].usFileNumber = 4;
    arxReadReq1[0].usRecordNumber = 1;
    arxReadReq1[0].usRecordLength = 2;
    arxReadReq1[1].usFileNumber = 3;
    arxReadReq1[1].usRecordNumber = 9;
    arxReadReq1[1].usRecordLength = 2;
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMReadFileRecord( xMBMHdl, 1, arxReadReq1, arxReadResp1, 2 ) );

    arxReadReq2[0].usFileNumber = 4;
    arxReadReq2[0].usRecordNumber = 1;
    arxReadReq2[0].usRecordLength = 3;
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest2, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse2 );
    xThreadData.pubPreparedResponse = arubPreparedResponse2;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMReadFileRecord( xMBMHdl, 1, arxReadReq2, arxReadResp2, 1 ) );
}


void
vMBMTestReadFilesIllegalResponses( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;
    xMBMFileSubReadReq_t arxReadReq1[2];
    xMBMFileSubReadResp_t arxReadResp1[2];

    const UBYTE     arubExpectedRequest1[] = {
        0x14, 0x0E,
        0x06, 0x00, 0x04, 0x00, 0x01, 0x00, 0x02,
        0x06, 0x00, 0x03, 0x00, 0x09, 0x00, 0x02
    };
    /* Wrong total length */
    const UBYTE     arubPreparedResponse1[] = {
        0x014, 0x0C,
        0x05, 0x06, 0x0D, 0xFE, 0x00, 0x20,
        0x05, 0x06, 0x33, 0xCD, 0x00, 0x40, 0x00
    };

    /* Wrong function code */
    const UBYTE     arubPreparedResponse2[] = {
        0x015, 0x0C,
        0x05, 0x06, 0x0D, 0xFE, 0x00, 0x20,
        0x05, 0x06, 0x33, 0xCD, 0x00, 0x40
    };
    /* Wrong length */
    const UBYTE     arubPreparedResponse3[] = {
        0x014, 0x0F,
        0x05, 0x06, 0x0D, 0xFE, 0x00, 0x20,
        0x05, 0x06, 0x33, 0xCD, 0x00, 0x40
    };
    /* Wrong length */
    const UBYTE     arubPreparedResponse4[] = {
        0x014, 0x0E,
        0x08, 0x06, 0x0D, 0xFE, 0x00, 0x20,
        0x05, 0x06, 0x33, 0xCD, 0x00, 0x40
    };
    /* Wrong length */
    const UBYTE     arubPreparedResponse5[] = {
        0x014, 0x0E,
        0x05, 0x06, 0x0D, 0xFE, 0x00, 0x20,
        0x08, 0x06, 0x33, 0xCD, 0x00, 0x40
    };

    arxReadReq1[0].usFileNumber = 4;
    arxReadReq1[0].usRecordNumber = 1;
    arxReadReq1[0].usRecordLength = 2;
    arxReadReq1[1].usFileNumber = 3;
    arxReadReq1[1].usRecordNumber = 9;
    arxReadReq1[1].usRecordLength = 2;
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadFileRecord( xMBMHdl, 1, arxReadReq1, arxReadResp1, 2 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse2 );
    xThreadData.pubPreparedResponse = arubPreparedResponse2;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadFileRecord( xMBMHdl, 1, arxReadReq1, arxReadResp1, 2 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse3 );
    xThreadData.pubPreparedResponse = arubPreparedResponse3;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadFileRecord( xMBMHdl, 1, arxReadReq1, arxReadResp1, 2 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse4 );
    xThreadData.pubPreparedResponse = arubPreparedResponse4;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadFileRecord( xMBMHdl, 1, arxReadReq1, arxReadResp1, 2 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse5 );
    xThreadData.pubPreparedResponse = arubPreparedResponse5;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EIO == eMBMReadFileRecord( xMBMHdl, 1, arxReadReq1, arxReadResp1, 2 ) );
}


void
vMBMTestReadFilesException( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;
    xMBMFileSubReadReq_t arxReadReq1[2];
    xMBMFileSubReadResp_t arxReadResp1[2];

    const UBYTE     arubExpectedRequest1[] = {
        0x14, 0x0E,
        0x06, 0x00, 0x04, 0x00, 0x01, 0x00, 0x02,
        0x06, 0x00, 0x03, 0x00, 0x09, 0x00, 0x02
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x14 | 0x80, 0x02,
    };
    const UBYTE     arubPreparedResponse2[] = {
        0x14 | 0x80, 0x03,
    };


    arxReadReq1[0].usFileNumber = 4;
    arxReadReq1[0].usRecordNumber = 1;
    arxReadReq1[0].usRecordLength = 2;
    arxReadReq1[1].usFileNumber = 3;
    arxReadReq1[1].usRecordNumber = 9;
    arxReadReq1[1].usRecordLength = 2;
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_ADDRESS == eMBMReadFileRecord( xMBMHdl, 1, arxReadReq1, arxReadResp1, 2 ) );

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse2 );
    xThreadData.pubPreparedResponse = arubPreparedResponse2;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_EX_ILLEGAL_DATA_VALUE == eMBMReadFileRecord( xMBMHdl, 1, arxReadReq1, arxReadResp1, 2 ) );
}

void
vMBMTestWriteFilesBasic( void )
{
    xPreparedRespThreadData xThreadData;
    pthread_t       xThreadHdl;
    xMBMFileSubWriteReq_t arxWriteReq1[1];
    const UBYTE     arubRequest1Data[] = {
        0x06, 0x0AF, 0x04, 0xBE, 0x10, 0x0D
    };
    const UBYTE     arubExpectedRequest1[] = {
        0x15, 0x0D,
        0x06, 0x00, 0x04, 0x00, 0x07, 0x00, 0x03, 0x06, 0x0AF, 0x04, 0xBE, 0x10, 0x0D
    };
    const UBYTE     arubPreparedResponse1[] = {
        0x15, 0x0D,
        0x06, 0x00, 0x04, 0x00, 0x07, 0x00, 0x03, 0x06, 0x0AF, 0x04, 0xBE, 0x10, 0x0D
    };

    xMBMFileSubWriteReq_t arxWriteReq2[2];
    const UBYTE     arubRequest2DataA[] = {
        0x06, 0x0AF, 0x04, 0xBE, 0x10, 0x0D
    };
    const UBYTE     arubRequest2DataB[] = {
        0x01, 0x02, 0x03, 0x04
    };
    const UBYTE     arubExpectedRequest2[] = {
        0x15, 0x18,
        0x06, 0x00, 0x04, 0x00, 0x07, 0x00, 0x03, 0x06, 0x0AF, 0x04, 0xBE, 0x10, 0x0D,
        0x06, 0x00, 0x01, 0x00, 0x02, 0x00, 0x02, 0x01, 0x02, 0x03, 0x04
    };
    const UBYTE     arubPreparedResponse2[] = {
        0x15, 0x18,
        0x06, 0x00, 0x04, 0x00, 0x07, 0x00, 0x03, 0x06, 0x0AF, 0x04, 0xBE, 0x10, 0x0D,
        0x06, 0x00, 0x01, 0x00, 0x02, 0x00, 0x02, 0x01, 0x02, 0x03, 0x04
    };


    arxWriteReq1[0].usFileNumber = 4;
    arxWriteReq1[0].usRecordNumber = 7;
    arxWriteReq1[0].usRecordLength = 3;
    arxWriteReq1[0].pubRecordData = arubRequest1Data;
    xThreadData.usDelayMilliSeconds = 100;

    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest1, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest1 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse1 );
    xThreadData.pubPreparedResponse = arubPreparedResponse1;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteFileRecord( xMBMHdl, 1, arxWriteReq1, 1 ) );

    arxWriteReq2[0].usFileNumber = 4;
    arxWriteReq2[0].usRecordNumber = 7;
    arxWriteReq2[0].usRecordLength = 3;
    arxWriteReq2[0].pubRecordData = arubRequest2DataA;
    arxWriteReq2[1].usFileNumber = 1;
    arxWriteReq2[1].usRecordNumber = 2;
    arxWriteReq2[1].usRecordLength = 2;
    arxWriteReq2[1].pubRecordData = arubRequest2DataB;
    eMBMTestSetExpectedRequest( xMBMHdl, arubExpectedRequest2, ( USHORT ) MB_UTILS_NARRSIZE( arubExpectedRequest2 ) );
    xThreadData.usPreparedResponseLength = ( USHORT ) MB_UTILS_NARRSIZE( arubPreparedResponse2 );
    xThreadData.pubPreparedResponse = arubPreparedResponse2;
    CU_ASSERT_FATAL( 0 == pthread_create( &xThreadHdl, NULL, pvMBMDelayedResponse, &xThreadData ) );
    CU_ASSERT( MB_ENOERR == eMBMWriteFileRecord( xMBMHdl, 1, arxWriteReq2, 2 ) );
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
