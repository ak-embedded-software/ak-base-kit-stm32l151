/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_common_stubs.c,v 1.12 2011-05-22 22:29:05 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <Console.h>
#include <CUnit.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbframe.h"
#include "internal/mbmiframe.h"
#include "internal/mbmi.h"
#include "ut_mbm.h"

/* ----------------------- Defines ------------------------------------------*/

/*                                                              
 * +--------+----------------------------------+---------+
 * | HEADER | MODBUS PDU                       | TRAILER |
 * +--------+----------------------------------+---------+
 *
 * HEADER     ... Depends on RTU/ASCII or Modbus TCP. For example slave 
 *                address in RTU mode.
 * MODBUS PDU ... Used by the stack.
 * TRAILER    ... Depends on RTU/ASCII or Modbus TCP. For example CRC16 in
 *                RTU mode.
 */
#define MBM_HEADER_SIZE             ( 7 )
#define MBM_TRAILER_SIZE            ( 2 )
#define MBM_TOTAL_BYTE_SIZE         ( ( MBM_HEADER_SIZE ) + ( MBM_TRAILER_SIZE ) + ( MB_PDU_SIZE_MAX ) )
#define MBM_MODBUS_PDU_OFF          ( MBM_HEADER_SIZE )
#define IDX_INVALID                 ( 255 )

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->usExpectedRequestLen = 0; \
    ( x )->usPreparedResponseLen = 0; \
} while( 0 )

#define STUB_VERBOSE_MODE           ( 0 )

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    UBYTE           arubMBPDUBuffer[MBM_TOTAL_BYTE_SIZE];
    UBYTE           arubExpectedRequest[MB_PDU_SIZE_MAX];
    USHORT          usExpectedRequestLen;
    UBYTE           arubPreparedResponse[MB_PDU_SIZE_MAX];
    USHORT          usPreparedResponseLen;
} xFrameInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitialized = FALSE;
STATIC xFrameInternalHandle arxMBMFrameInstances[MBM_TEST_INSTANCES];

/* ----------------------- Static functions ---------------------------------*/
STATIC eMBErrorCode eMBMFrameSend( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength );
STATIC eMBErrorCode eMBMFrameReceive( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength );
STATIC eMBErrorCode eMBMFrameClose( xMBMHandle xHdl );

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBMTestInit( xMBMHandle * pxHdl )
{
    xMBMInternalHandle *pxIntHdl = NULL;
    eMBErrorCode    eStatus = MB_ENORES;
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitialized )
    {
        for( ubIdx = 0; ubIdx < MBM_TEST_INSTANCES; ubIdx++ )
        {
            HDL_RESET( &arxMBMFrameInstances[ubIdx] );
        }
        bIsInitialized = TRUE;
    }

    for( ubIdx = 0; ubIdx < MBM_TEST_INSTANCES; ubIdx++ )
    {
        if( IDX_INVALID == arxMBMFrameInstances[ubIdx].ubIdx )
        {
            if( NULL != ( pxIntHdl = pxMBMGetNewHdl(  ) ) )
            {
                arxMBMFrameInstances[ubIdx].ubIdx = ubIdx;
                pxIntHdl->pFrameRecvFN = eMBMFrameReceive;
                pxIntHdl->pFrameSendFN = eMBMFrameSend;
                pxIntHdl->pFrameCloseFN = eMBMFrameClose;
                /*@i1@ */ pxIntHdl->pubFrameMBPDUBuffer =
                    &( arxMBMFrameInstances[ubIdx].arubMBPDUBuffer[MBM_HEADER_SIZE] );
                pxIntHdl->usFrameMBPDULength = 0;
                /*@i1@ */ pxIntHdl->xFrameHdl = &arxMBMFrameInstances[ubIdx];
                *pxHdl = pxIntHdl;
                eStatus = MB_ENOERR;
            }
            break;
        }
    }

    if( eStatus != MB_ENOERR )
    {
        if( pxIntHdl != NULL )
        {
            if( MBM_FRAME_HANDLE_INVALID != pxIntHdl->xFrameHdl )
            {
                HDL_RESET( ( xFrameInternalHandle * ) pxIntHdl->xFrameHdl );
            }
            ( void )eMBMReleaseHdl( pxIntHdl );
        }
    }

    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBMFrameSend( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xFrameInternalHandle *pxFrameHdl;
#if STUB_VERBOSE_MODE == 1	
    int             iIdx;
#endif	

    CU_ASSERT_FATAL( bMBMIsHdlValid( pxIntHdl ) );
    pxFrameHdl = pxIntHdl->xFrameHdl;

#if STUB_VERBOSE_MODE == 1
    fprintf( stderr, "eMBMFrameSend: " );
    for( iIdx = 0; iIdx < usMBPDULength; iIdx++ )
    {
        fprintf( stderr, "%02X ", pxFrameHdl->arubMBPDUBuffer[MBM_MODBUS_PDU_OFF + iIdx] );
    }
    fprintf( stderr, "\n" );
	fprintf( stderr, "eMBMFrameSend: usMBPDULength=%d, expected len=%d\n", usMBPDULength, pxFrameHdl->usExpectedRequestLen );
#endif
    /*@i@ */ CU_ASSERT_EQUAL( usMBPDULength, pxFrameHdl->usExpectedRequestLen );
    /*@i@ */ CU_ASSERT_TRUE( 0 == memcmp( &( pxFrameHdl->arubMBPDUBuffer[MBM_MODBUS_PDU_OFF] ),
                                          pxFrameHdl->arubExpectedRequest,
                                          ( size_t ) pxFrameHdl->usExpectedRequestLen ) );
    if( MB_SER_BROADCAST_ADDR == ucSlaveAddress )
    {
        eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, MBM_EV_SENT );
    }
    return eStatus;
}

eMBErrorCode
eMBMFrameReceive( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xFrameInternalHandle *pxFrameHdl;

    CU_ASSERT_FATAL( bMBMIsHdlValid( pxIntHdl ) );

    pxFrameHdl = pxIntHdl->xFrameHdl;
    if( NULL != pusMBPDULength )
    {
        if( 0 != pxFrameHdl->usPreparedResponseLen )
        {
            memcpy( &( pxFrameHdl->arubMBPDUBuffer[MBM_MODBUS_PDU_OFF] ), pxFrameHdl->arubPreparedResponse,
                    ( size_t ) pxFrameHdl->usPreparedResponseLen );
            *pusMBPDULength = pxFrameHdl->usPreparedResponseLen;
            eStatus = MB_ENOERR;
        }
        else
        {
            eStatus = MB_EIO;
        }
    }
    return eStatus;
}

eMBErrorCode
eMBMFrameClose( xMBMHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;
    xFrameInternalHandle *pxFrameHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( bMBMIsHdlValid( pxIntHdl ) )
    {
        pxFrameHdl = pxIntHdl->xFrameHdl;
        if( MB_IS_VALID_HDL( pxFrameHdl, arxMBMFrameInstances ) )
        {
            HDL_RESET( pxFrameHdl );
            eStatus = MB_ENOERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

void
eMBMTestSetExpectedRequest( const xMBMHandle xHdl, const UBYTE arubExpectedRequestArg[], USHORT usExpectedRequestLen )
{
    xFrameInternalHandle *pxFrameHdl = ( ( xMBMInternalHandle * ) xHdl )->xFrameHdl;

    /*@i@ */ CU_ASSERT_PTR_NOT_NULL( pxFrameHdl );
    /*@i@ */ CU_ASSERT( usExpectedRequestLen < MB_PDU_SIZE_MAX );
    if( NULL != arubExpectedRequestArg )
    {
        memcpy( pxFrameHdl->arubExpectedRequest, arubExpectedRequestArg, ( size_t ) usExpectedRequestLen );
        pxFrameHdl->usExpectedRequestLen = usExpectedRequestLen;
    }
    else
    {
        pxFrameHdl->usExpectedRequestLen = 0;
    }
}

void
eMBMTestSetPreparedResponse( const xMBMHandle xHdl, const UBYTE arubPreparedResponseArg[],
                             USHORT usPreparedResponseLen )
{
    xFrameInternalHandle *pxFrameHdl = ( ( xMBMInternalHandle * ) xHdl )->xFrameHdl;

    /*@i@ */ CU_ASSERT_PTR_NOT_NULL( pxFrameHdl );
    /*@i@ */ CU_ASSERT( usPreparedResponseLen < MB_PDU_SIZE_MAX );
    if( NULL != arubPreparedResponseArg )
    {
        memcpy( pxFrameHdl->arubPreparedResponse, arubPreparedResponseArg, ( size_t ) usPreparedResponseLen );
        pxFrameHdl->usPreparedResponseLen = usPreparedResponseLen;
    }
    else
    {
        pxFrameHdl->usPreparedResponseLen = 0;
    }
}
