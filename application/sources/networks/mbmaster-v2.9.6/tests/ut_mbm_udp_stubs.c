/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP/UDP.
 * Copyright (c) 2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_udp_stubs.c,v 1.3 2011-12-04 20:47:18 embedded-solutions.cwalter Exp $
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
    ( x )->usExpectedRequestLen = 0; \
    memset( &( ( x )->arubPreparedResponse[ 0 ] ), 0, PREPARED_RESPONSE_BUFFER_SIZE ); \
    memset( &( ( x )->arubExpectedRequest[ 0 ] ), 0, PREPARED_RESPONSE_BUFFER_SIZE ); \
} while( 0 );

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->bFailOnWrite = FALSE; \
	( x )->bFailOnRead = FALSE; \
    ( x )->eMBPUDPClientNewDATAFN = NULL; \
    ( x )->xMBHdl = MB_HDL_INVALID; \
    HDL_RESET_CLIENT( x ); \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    BOOL            bFailOnWrite;
	BOOL            bFailOnRead;
    peMBPUDPClientNewDataCB eMBPUDPClientNewDATAFN;
    UBYTE           arubPreparedResponse[PREPARED_RESPONSE_BUFFER_SIZE];
    USHORT          usPreparedResponseLen;
    UBYTE           arubExpectedRequest[EXPECTED_REQUEST_BUFFER_SIZE];
    USHORT          usExpectedRequestLen;
    xMBHandle       xMBHdl;
} xUDPInternalHandle;

typedef struct
{
    xUDPInternalHandle *pxUDPIntHdl;
} xUDPRXThreadData;

/* ----------------------- Static variables ---------------------------------*/
STATIC xUDPInternalHandle xUDPIntHdl[MBM_TEST_INSTANCES];
STATIC BOOL     bFailOnInit = FALSE;
STATIC BOOL     bIsInitalized = FALSE;
STATIC xMBPUDPHandle xLastUDPHdl;

/* ----------------------- Function prototypes ------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC void    *vMBMTestResponseRXThread( void *pvThreadData );

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBPUDPClientInit( xMBPUDPHandle * pxUDPHdl, xMBHandle xMBHdl,
                   const CHAR * pcUDPBindAddress, LONG uUDPListenPort, peMBPUDPClientNewDataCB eMBPUDPClientNewDATAFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    UBYTE           ubIdx;

    CU_ASSERT_PTR_NOT_NULL( pxUDPHdl );
    CU_ASSERT_PTR_NOT_NULL( xMBHdl );
    CU_ASSERT_TRUE( ( -1 == uUDPListenPort ) || ( ( 0 <= uUDPListenPort ) && ( uUDPListenPort <= 65535 ) ) );
    CU_ASSERT_PTR_NOT_NULL( eMBPUDPClientNewDATAFN );

    if( !bFailOnInit )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !bIsInitalized )
        {
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xUDPIntHdl ); ubIdx++ )
            {
                HDL_RESET( &xUDPIntHdl[ubIdx] );
            }
            bIsInitalized = TRUE;
        }
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xUDPIntHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xUDPIntHdl[ubIdx].ubIdx )
            {
                xUDPIntHdl[ubIdx].ubIdx = ubIdx;
                xUDPIntHdl[ubIdx].xMBHdl = xMBHdl;
                xUDPIntHdl[ubIdx].eMBPUDPClientNewDATAFN = eMBPUDPClientNewDATAFN;
                *pxUDPHdl = &xUDPIntHdl[ubIdx];
                xLastUDPHdl = *pxUDPHdl;
                eStatus = MB_ENOERR;
                break;
            }
        }
        if( MB_UTILS_NARRSIZE( xUDPIntHdl ) == ubIdx )
        {
            eStatus = MB_ENORES;
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return eStatus;
}

void			
vMBMTestFailOnInit( BOOL bFail )
{
	bFailOnInit = bFail;
}

void            
vMBMTestFailOnWrite( xMBPUDPHandle xUDPHdl, BOOL bFail )
{
    xUDPInternalHandle *pxUDPIntHdl = xUDPHdl;
    if( MB_IS_VALID_HDL( pxUDPIntHdl, xUDPIntHdl ) )
    {
		pxUDPIntHdl->bFailOnWrite = bFail;
	}		
}

void            
vMBMTestFailOnRead( xMBPUDPHandle xUDPHdl, BOOL bFail )
{
    xUDPInternalHandle *pxUDPIntHdl = xUDPHdl;
    if( MB_IS_VALID_HDL( pxUDPIntHdl, xUDPIntHdl ) )
    {
		pxUDPIntHdl->bFailOnRead = bFail;
	}		
}

xMBPUDPHandle
xMBMTestGetLastHandle( void )
{
    return xLastUDPHdl;
}

void
vMBMTestRequestTransmit( xMBPUDPHandle xUDPHdl, UBYTE arubExpectedRequest[], USHORT usExpectedRequestLen )
{
    xUDPInternalHandle *pxUDPIntHdl = xUDPHdl;

    CU_ASSERT_FATAL( usExpectedRequestLen < EXPECTED_REQUEST_BUFFER_SIZE );
    if( MB_IS_VALID_HDL( pxUDPIntHdl, xUDPIntHdl ) )
    {
#if UDP_STUB_DEBUG == 1
		vMBPPortLog( MBP_LOG_DEBUG, MB_LOG_PORT_UDP, "vMBMTestRequestTransmit: set new request\n" );	
#endif			
        memcpy( pxUDPIntHdl->arubExpectedRequest, arubExpectedRequest, usExpectedRequestLen );
        pxUDPIntHdl->usExpectedRequestLen = usExpectedRequestLen;
    }
}

void
vMBMTestPreparedResponse( xMBPUDPHandle xUDPHdl, UBYTE arubPreparedResponse[], USHORT usPreparedResponseLen )
{
    xUDPInternalHandle *pxUDPIntHdl = xUDPHdl;

    CU_ASSERT_FATAL( usPreparedResponseLen < PREPARED_RESPONSE_BUFFER_SIZE );
    if( MB_IS_VALID_HDL( pxUDPIntHdl, xUDPIntHdl ) )
    {
#if UDP_STUB_DEBUG == 1
		vMBPPortLog( MBP_LOG_DEBUG, MB_LOG_PORT_UDP, "vMBMTestPreparedResponse: set new prepared response\n" );	
#endif	
		if( ( NULL != arubPreparedResponse ) && ( usPreparedResponseLen > 0 ) )
		{
			memcpy( pxUDPIntHdl->arubPreparedResponse, arubPreparedResponse, usPreparedResponseLen );
			pxUDPIntHdl->usPreparedResponseLen = usPreparedResponseLen;
		}
		else
		{
			memset( pxUDPIntHdl->arubPreparedResponse, 0 , sizeof( pxUDPIntHdl->arubPreparedResponse ) );
			pxUDPIntHdl->usPreparedResponseLen = 0;
		}
    }
}

eMBErrorCode
eMBPUDPConWrite( xMBPUDPHandle xUDPHdl, const CHAR * pcUDPClientAddress, USHORT usUDPSlavePort, const UBYTE * pubBuffer,
                 USHORT usBufferLen )
{
    pthread_t       xThreadHdl;
    eMBErrorCode    eStatus = MB_EIO;
    BOOL            bIsEqual;
    UBYTE           ubBuffer1[PREPARED_RESPONSE_BUFFER_SIZE * 3 + 1];
    UBYTE           ubBuffer2[PREPARED_RESPONSE_BUFFER_SIZE * 3 + 1];
    int             i, iCurPos, iRes;

    xUDPInternalHandle *pxUDPIntHdl = xUDPHdl;

#if UDP_STUB_DEBUG == 1
		vMBPPortLog( MBP_LOG_DEBUG, MB_LOG_PORT_UDP, "eMBPUDPConWrite: sent %d bytes\n", usBufferLen );	
#endif		
    if( !MB_IS_VALID_HDL( pxUDPIntHdl, xUDPIntHdl ) )
    {
		eStatus = MB_EINVAL;
	}
	else if ( pxUDPIntHdl->bFailOnWrite )
	{
#if UDP_STUB_DEBUG == 1
		vMBPPortLog( MBP_LOG_DEBUG, MB_LOG_PORT_UDP, "eMBPUDPConWrite: Fail on write is active!\n" );	
		eStatus = MB_EIO;
#endif		
	}
	else
	{
        MBP_ENTER_CRITICAL_SECTION(  );
        CU_ASSERT( usBufferLen == pxUDPIntHdl->usExpectedRequestLen );
        bIsEqual = memcmp( pxUDPIntHdl->arubExpectedRequest, pubBuffer, pxUDPIntHdl->usExpectedRequestLen ) == 0;
        if( !bIsEqual )
        {
            iCurPos = 0;
            for( i = 0; i < pxUDPIntHdl->usExpectedRequestLen; i++ )
            {
                iCurPos +=
                    snprintf( &ubBuffer1[iCurPos], MB_UTILS_NARRSIZE( ubBuffer1 ) - iCurPos, "%02X ",
                              pxUDPIntHdl->arubExpectedRequest[i] );
            }
            iCurPos = 0;
            for( i = 0; i < usBufferLen; i++ )
            {
                iCurPos +=
                    snprintf( &ubBuffer2[iCurPos], MB_UTILS_NARRSIZE( ubBuffer2 ) - iCurPos, "%02X ", pubBuffer[i] );
            }
#if UDP_STUB_DEBUG == 1
            vMBPPortLog( MBP_LOG_DEBUG, MB_LOG_PORT_UDP, "eMBPUDPConWrite: Expected Buffer : %s\n", ubBuffer1 );
            vMBPPortLog( MBP_LOG_DEBUG, MB_LOG_PORT_UDP, "eMBPUDPConWrite: Sent Buffer     : %s\n", ubBuffer2 );
#endif
        }
        CU_ASSERT( 0 == memcmp( pxUDPIntHdl->arubExpectedRequest, pubBuffer, pxUDPIntHdl->usExpectedRequestLen ) );

        if( bIsEqual )
        {
	
            eStatus = MB_ENOERR;
            CU_ASSERT_FATAL( 0 ==
                             ( iRes = pthread_create( &xThreadHdl, NULL, vMBMTestResponseRXThread, pxUDPIntHdl ) ) );
        }
        MBP_EXIT_CRITICAL_SECTION(  );

    }
    return eStatus;
}

eMBErrorCode
eMBPUDPConRead( xMBPUDPHandle xUDPHdl, UBYTE * pubBuffer, USHORT * pusBufferLen, USHORT usBufferMax )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xUDPInternalHandle *pxUDPIntHdl = xUDPHdl;
    CU_ASSERT_PTR_NOT_NULL( pubBuffer );
    CU_ASSERT_PTR_NOT_NULL( pusBufferLen );

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !MB_IS_VALID_HDL( pxUDPIntHdl, xUDPIntHdl ) )
	{
		eStatus = MB_EINVAL;
	}
	else if( pxUDPIntHdl->bFailOnRead )
	{
		eStatus = MB_EIO;
	}
	else
    {
        if( pxUDPIntHdl->usPreparedResponseLen <= usBufferMax )
        {
            memcpy( pubBuffer, &( pxUDPIntHdl->arubPreparedResponse[0] ), pxUDPIntHdl->usPreparedResponseLen );
            *pusBufferLen = pxUDPIntHdl->usPreparedResponseLen;
#if UDP_STUB_DEBUG == 1
            vMBPPortLog( MBP_LOG_DEBUG, MB_LOG_PORT_UDP, "eMBPUDPConRead: Returned %d bytes.\n", *pusBufferLen );
#endif
        }
        else
        {
#if UDP_STUB_DEBUG == 1
            vMBPPortLog( MBP_LOG_DEBUG, MB_LOG_PORT_UDP, "eMBPUDPConRead: Buffer to small for data!\n", *pusBufferLen );
#endif
            eStatus = MB_EIO;
        }

        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPUDPClientClose( xMBPUDPHandle xUDPHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    xUDPInternalHandle *pxUDPIntHdl = xUDPHdl;

    if( MB_IS_VALID_HDL( pxUDPIntHdl, xUDPIntHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        HDL_RESET( pxUDPIntHdl );
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}

STATIC void    *
vMBMTestResponseRXThread( void *pvThreadData )
{
    xUDPInternalHandle *pxUDPIntHdl = pvThreadData;
    eMBErrorCode    eStatus;

    /* Wait some time. */
    usleep( 1000 );

#if UDP_STUB_DEBUG == 1
    vMBPPortLog( MBP_LOG_DEBUG, MB_LOG_PORT_UDP, "vMBMTestResponseRXThread: new data available...\n" );
#endif

    MBP_ENTER_CRITICAL_SECTION(  );
	if( pxUDPIntHdl->usPreparedResponseLen > 0 )
	{
		if( NULL != pxUDPIntHdl->eMBPUDPClientNewDATAFN )
		{
			eStatus = pxUDPIntHdl->eMBPUDPClientNewDATAFN( pxUDPIntHdl->xMBHdl );
			if( !pxUDPIntHdl->bFailOnRead )
			{
				CU_ASSERT_EQUAL( eStatus, MB_ENOERR );
			}
		}
	}
    MBP_EXIT_CRITICAL_SECTION(  );
    return NULL;
}
