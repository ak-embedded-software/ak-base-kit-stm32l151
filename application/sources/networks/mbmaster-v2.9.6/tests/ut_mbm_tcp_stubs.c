/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_tcp_stubs.c,v 1.5 2014-08-23 09:43:51 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <Console.h>
#include <CUnit.h>


/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"
#include "internal/mbmiframe.h"
#include "internal/mbmi.h"
#include "ut_mbm.h"

/* ----------------------- Defines ------------------------------------------*/
#define IDX_INVALID                     ( 255 )
#define EXPECTED_REQUEST_BUFFER_SIZE    ( 512 )
#define PREPARED_RESPONSE_BUFFER_SIZE   ( 512 )

#define HDL_RESET_CLIENT( x ) do { \
    ( x )->usPreparedResponseLen = 0; \
    ( x )->usPreparedResponseCurPos = 0; \
    ( x )->usExpectedRequestLen = 0; \
    memset( &( ( x )->arubPreparedResponse[ 0 ] ), 0, PREPARED_RESPONSE_BUFFER_SIZE ); \
    memset( &( ( x )->arubExpectedRequest[ 0 ] ), 0, PREPARED_RESPONSE_BUFFER_SIZE ); \
    ( x )->bIsConnected = FALSE; \
} while( 0 );

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->bFailOnWrite = FALSE; \
    ( x )->eMBPTCPClientNewDATAFN = NULL; \
    ( x )->eMBPTCPClientDisconnectedFN = NULL; \
    ( x )->xMBHdl = MB_HDL_INVALID; \
    HDL_RESET_CLIENT( x ); \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    BOOL            bFailOnWrite;
    BOOL            bIsConnected;
    peMBPTCPClientNewDataCB eMBPTCPClientNewDATAFN;
    peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN;
    UBYTE           arubPreparedResponse[PREPARED_RESPONSE_BUFFER_SIZE];
    USHORT          usPreparedResponseLen;
    USHORT          usPreparedResponseCurPos;
    UBYTE           arubExpectedRequest[EXPECTED_REQUEST_BUFFER_SIZE];
    USHORT          usExpectedRequestLen;
    xMBHandle       xMBHdl;
} xTCPInternalHandle;

typedef struct
{
    xTCPInternalHandle *pxTCPIntHdl;
} xTCPRXThreadData;

/* ----------------------- Static variables ---------------------------------*/
STATIC xTCPInternalHandle xTCPIntHdl[MBM_TEST_INSTANCES];
STATIC BOOL     bFailOnInit = FALSE;
STATIC BOOL     bIsInitalized = FALSE;
STATIC xMBPTCPHandle xLastTCPHdl;

/* ----------------------- Function prototypes ------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC void    *vMBMTestResponseRXThread( void *pvThreadData );

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBPTCPClientInit( xMBPTCPHandle * pxTCPHdl, /*@shared@ */ xMBHandle xMBHdl,
                   peMBPTCPClientNewDataCB eMBPTCPClientNewDATAFN,
                   peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    UBYTE           ubIdx;

    CU_ASSERT_PTR_NOT_NULL( pxTCPHdl );
    CU_ASSERT_PTR_NOT_NULL( xMBHdl );
    CU_ASSERT_PTR_NOT_NULL( eMBPTCPClientNewDATAFN );
    CU_ASSERT_PTR_NOT_NULL( eMBPTCPClientDisconnectedFN );

    if( !bFailOnInit )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !bIsInitalized )
        {
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xTCPIntHdl ); ubIdx++ )
            {
                HDL_RESET( &xTCPIntHdl[ubIdx] );
            }
            bIsInitalized = TRUE;
        }
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xTCPIntHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xTCPIntHdl[ubIdx].ubIdx )
            {
                xTCPIntHdl[ubIdx].ubIdx = ubIdx;
                xTCPIntHdl[ubIdx].xMBHdl = xMBHdl;
                xTCPIntHdl[ubIdx].eMBPTCPClientNewDATAFN = eMBPTCPClientNewDATAFN;
                xTCPIntHdl[ubIdx].eMBPTCPClientDisconnectedFN = eMBPTCPClientDisconnectedFN;
                *pxTCPHdl = &xTCPIntHdl[ubIdx];
                xLastTCPHdl = *pxTCPHdl;
                eStatus = MB_ENOERR;
                break;
            }
        }
        if( MB_UTILS_NARRSIZE( xTCPIntHdl ) == ubIdx )
        {
            eStatus = MB_ENORES;
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return eStatus;
}

xMBPTCPHandle
xMBMTestGetLastHandle( void )
{
    return xLastTCPHdl;
}

void
vMBMTestRequestTransmit( xMBPTCPHandle xTCPHdl, UBYTE arubExpectedRequest[], USHORT usExpectedRequestLen )
{
    xTCPInternalHandle *pxTCPIntHdl = xTCPHdl;

    CU_ASSERT_FATAL( usExpectedRequestLen < EXPECTED_REQUEST_BUFFER_SIZE );
    if( MB_IS_VALID_HDL( pxTCPIntHdl, xTCPIntHdl ) )
    {
        memcpy( pxTCPIntHdl->arubExpectedRequest, arubExpectedRequest, usExpectedRequestLen );
        pxTCPIntHdl->usExpectedRequestLen = usExpectedRequestLen;
    }
}

void
vMBMTestPreparedResponse( xMBPTCPHandle xTCPHdl, UBYTE arubPreparedResponse[], USHORT usPreparedResponseLen )
{
    xTCPInternalHandle *pxTCPIntHdl = xTCPHdl;

    CU_ASSERT_FATAL( usPreparedResponseLen < PREPARED_RESPONSE_BUFFER_SIZE );
    if( MB_IS_VALID_HDL( pxTCPIntHdl, xTCPIntHdl ) )
    {
        memcpy( pxTCPIntHdl->arubPreparedResponse, arubPreparedResponse, usPreparedResponseLen );
        pxTCPIntHdl->usPreparedResponseLen = usPreparedResponseLen;
        pxTCPIntHdl->usPreparedResponseCurPos = 0;
    }
}

void
VMBMTestFailOnWrite( xMBPTCPHandle xTCPHdl, BOOL bFail )
{
    xTCPInternalHandle *pxTCPIntHdl = xTCPHdl;

    if( MB_IS_VALID_HDL( pxTCPIntHdl, xTCPIntHdl ) )
    {
        pxTCPIntHdl->bFailOnWrite = bFail;
    }
}

eMBErrorCode
eMBPTCPClientOpen( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle * pxTCPClientHdl, const CHAR * pcConnectAddress,
                   USHORT usTCPPort )
{
    eMBErrorCode    eStatus = MB_EIO;
    xTCPInternalHandle *pxTCPIntHdl = xTCPHdl;

    if( MB_IS_VALID_HDL( pxTCPIntHdl, xTCPIntHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !pxTCPIntHdl->bIsConnected )
        {
            if( 0 == strcmp( "127.0.0.1", pcConnectAddress ) )
            {
                /* Note: For testing we use the same handle. */
                *pxTCPClientHdl = pxTCPIntHdl;
                pxTCPIntHdl->bIsConnected = TRUE;
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_EIO;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}

eMBErrorCode
eMBPTCPClientClose( xMBPTCPHandle xTCPHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    xTCPInternalHandle *pxTCPIntHdl = xTCPHdl;

    if( MB_IS_VALID_HDL( pxTCPIntHdl, xTCPIntHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        HDL_RESET( pxTCPIntHdl );
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}

eMBErrorCode
eMBPTCPConRead( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, /*@out@ */ UBYTE * pubBuffer, /*@out@ */
                USHORT * pusBufferLen, USHORT usBufferMax )
{
    eMBErrorCode    eStatus = MB_EIO;
    xTCPInternalHandle *pxTCPIntHdl = xTCPClientHdl;
    USHORT          usBytesMax;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTCPIntHdl, xTCPIntHdl ) )
    {
        usBytesMax = pxTCPIntHdl->usPreparedResponseLen - pxTCPIntHdl->usPreparedResponseCurPos;
        if( usBytesMax > 4 )
        {
            usBytesMax = 4;
        }
        usBytesMax = usBytesMax > usBufferMax ? usBufferMax : usBytesMax;
        memcpy( pubBuffer, &( pxTCPIntHdl->arubPreparedResponse[pxTCPIntHdl->usPreparedResponseCurPos] ), usBytesMax );
        *pusBufferLen = usBytesMax;
        pxTCPIntHdl->usPreparedResponseCurPos += usBytesMax;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "eMBPTCPConRead: Returned %d bytes\n", usBytesMax );
#endif
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}


eMBErrorCode
eMBPTCPConWrite( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, const UBYTE * pubBuffer, USHORT usBufferLen )
{
    pthread_t       xThreadHdl;
    eMBErrorCode    eStatus = MB_EIO;
    BOOL            bIsEqual;
    UBYTE           ubBuffer1[PREPARED_RESPONSE_BUFFER_SIZE * 3 + 1];
    UBYTE           ubBuffer2[PREPARED_RESPONSE_BUFFER_SIZE * 3 + 1];
    int             i, iCurPos, iRes;

    xTCPInternalHandle *pxTCPIntHdl = xTCPClientHdl;

    if( MB_IS_VALID_HDL( pxTCPIntHdl, xTCPIntHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        CU_ASSERT( usBufferLen == pxTCPIntHdl->usExpectedRequestLen );
        bIsEqual = memcmp( pxTCPIntHdl->arubExpectedRequest, pubBuffer, pxTCPIntHdl->usExpectedRequestLen ) == 0;
        if( !bIsEqual )
        {
            iCurPos = 0;
            for( i = 0; i < pxTCPIntHdl->usExpectedRequestLen; i++ )
            {
                iCurPos +=
                    snprintf( &ubBuffer1[iCurPos], MB_UTILS_NARRSIZE( ubBuffer1 ) - iCurPos, "%02X ",
                              pxTCPIntHdl->arubExpectedRequest[i] );
            }
            iCurPos = 0;
            for( i = 0; i < usBufferLen; i++ )
            {
                iCurPos +=
                    snprintf( &ubBuffer2[iCurPos], MB_UTILS_NARRSIZE( ubBuffer2 ) - iCurPos, "%02X ", pubBuffer[i] );
            }
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "eMBPTCPConWrite: Expected Buffer : %s\n", ubBuffer1 );
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "eMBPTCPConWrite: Sent Buffer     : %s\n", ubBuffer2 );
#endif
        }
        else
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "eMBPTCPConWrite: send %d bytes\n", usBufferLen );
#endif
        }
        CU_ASSERT( 0 == memcmp( pxTCPIntHdl->arubExpectedRequest, pubBuffer, pxTCPIntHdl->usExpectedRequestLen ) );

        if( bIsEqual )
        {
            eStatus = MB_ENOERR;
            CU_ASSERT_FATAL( 0 ==
                             ( iRes = pthread_create( &xThreadHdl, NULL, vMBMTestResponseRXThread, pxTCPIntHdl ) ) );
        }
        MBP_EXIT_CRITICAL_SECTION(  );

    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}

eMBErrorCode
eMBPTCPConClose( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    xTCPInternalHandle *pxTCPIntHdl = xTCPClientHdl;

    if( MB_IS_VALID_HDL( pxTCPIntHdl, xTCPIntHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        HDL_RESET_CLIENT( pxTCPIntHdl );
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return eStatus;
}

STATIC void    *
vMBMTestResponseRXThread( void *pvThreadData )
{
    xTCPInternalHandle *pxTCPIntHdl = pvThreadData;
    BOOL            bFinished = FALSE;
    eMBErrorCode    eStatus;

    /* Wait some time. */
    usleep( 1000 );

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "vMBMTestResponseRXThread: started..\n" );
#endif
    /* Now start the transmission. */
    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( ( pxTCPIntHdl->usPreparedResponseCurPos >= pxTCPIntHdl->usPreparedResponseLen ) ||
            !pxTCPIntHdl->bIsConnected )
        {
            bFinished = TRUE;
        }
        MBP_EXIT_CRITICAL_SECTION(  );
        if( !bFinished )
        {
            if( NULL != pxTCPIntHdl->eMBPTCPClientNewDATAFN )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "vMBMTestResponseRXThread: still %d of %d bytes available.\n",
                             pxTCPIntHdl->usPreparedResponseLen - pxTCPIntHdl->usPreparedResponseCurPos,
                             pxTCPIntHdl->usPreparedResponseLen );
#endif
                eStatus = pxTCPIntHdl->eMBPTCPClientNewDATAFN( pxTCPIntHdl->xMBHdl, pxTCPIntHdl );
                assert( MB_ENOERR == eStatus );
            }
            usleep( 1000 );
        }
    }
    while( !bFinished );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "vMBMTestResponseRXThread: done..\n" );
#endif
    return NULL;
}
