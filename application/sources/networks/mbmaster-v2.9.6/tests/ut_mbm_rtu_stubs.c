/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_rtu_stubs.c,v 1.11 2007-09-02 14:57:32 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <pthread.h>
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

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->bIsTxEnabled = FALSE; \
    ( x )->bFailOnTxStart = FALSE; \
    ( x )->bIsRxEnabled = FALSE; \
    ( x )->bFailOnRxStart = FALSE; \
    ( x )->bTimeoutOnRx = FALSE; \
    ( x )->pbMBPTransmitterEmptyFN = NULL; \
    ( x )->pvMBPReceiveFN = NULL; \
    ( x )->xMBHdl = MB_HDL_INVALID; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    BOOL            bIsTxEnabled;
    BOOL            bFailOnTxStart;
    BOOL            bIsRxEnabled;
    BOOL            bFailOnRxStart;
    BOOL            bTimeoutOnRx;
    pbMBPSerialTransmitterEmptyAPIV1CB pbMBPTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV1CB pvMBPReceiveFN;
    xMBHandle       xMBHdl;
} xSerialInternalHandle;

typedef struct
{
    xSerialInternalHandle *pxSerialIntHdl;
    UBYTE           arubExpectedRequest[EXPECTED_REQUEST_BUFFER_SIZE];
    USHORT          usExpectedRequestLen;
    USHORT          usExpectedRequestCurPos;
} xSerialTxThreadData;

typedef struct
{
    xSerialInternalHandle *pxSerialIntHdl;
    UBYTE           arubPreparedResponse[PREPARED_RESPONSE_BUFFER_SIZE];
    USHORT          usPreparedResponseLen;
} xSerialRxThreadData;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialInternalHandle xSerialHdl[MBM_TEST_INSTANCES];

STATIC BOOL     bIsInitalized = FALSE;
STATIC UCHAR    ucExpectedPort = 0;
STATIC ULONG    ulExpectedBaudRate = 19200;
STATIC UCHAR    ucExpectedDataBits = 8;
STATIC UCHAR    ucExpectedStopBits = 1;
STATIC eMBSerialParity eExpectedParity = MB_PAR_EVEN;
STATIC BOOL     bFailOnInit = TRUE;

/* ----------------------- Function prototypes ------------------------------*/
BOOL            bMBMSerialRTUT35CB( xMBHandle xHdl );

/* ----------------------- Static functions ---------------------------------*/
STATIC void    *vMBMTestRequestTXThread( void *pvThreadData );
STATIC void    *vMBMTestResponseRXThread( void *pvThreadData );

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl )
{
    eMBErrorCode    eStatus = MB_EPORTERR;
    UBYTE           ubIdx;

    CU_ASSERT_PTR_NOT_NULL( pxSerialHdl );
    CU_ASSERT_EQUAL( ucPort, ucExpectedPort );
    CU_ASSERT_EQUAL( ulBaudRate, ulExpectedBaudRate );
    CU_ASSERT_EQUAL( ucDataBits, ucExpectedDataBits );
    CU_ASSERT_EQUAL( ucStopBits, ucExpectedStopBits );
    CU_ASSERT_EQUAL( eParity, eExpectedParity );
    CU_ASSERT_PTR_NOT_NULL( xMBMHdl );

    if( !bFailOnInit )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( !bIsInitalized )
        {
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdl ); ubIdx++ )
            {
                HDL_RESET( &xSerialHdl[ubIdx] );
            }
            bIsInitalized = TRUE;
        }
        if( ( ucPort < MB_UTILS_NARRSIZE( xSerialHdl ) ) && ( IDX_INVALID == xSerialHdl[ucPort].ubIdx ) )
        {
            xSerialHdl[ucPort].ubIdx = ucPort;
            xSerialHdl[ucPort].xMBHdl = xMBMHdl;
            *pxSerialHdl = &xSerialHdl[ucPort];
            eStatus = MB_ENOERR;
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    return eStatus;
}

eMBErrorCode
eMBPSerialClose( xMBPSerialHandle xSerialHdlArg )
{
    eMBErrorCode    eStatus = MB_ENOERR, eStatus2;
    xSerialInternalHandle *pxSerialIntHdl = xSerialHdlArg;

    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( MB_ENOERR != ( eStatus2 = eMBPSerialRxEnable( pxSerialIntHdl, NULL ) ) )
        {
            eStatus = eStatus2;
        }
        if( MB_ENOERR != ( eStatus2 = eMBPSerialTxEnable( pxSerialIntHdl, NULL ) ) )
        {
            eStatus = eStatus2;
        }
        HDL_RESET( pxSerialIntHdl );
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}

void
vMBMTestFailOnTxStart( UCHAR ucExpectedPortArg, BOOL bFail )
{
    CU_ASSERT_FATAL( ucExpectedPortArg < MB_UTILS_NARRSIZE( xSerialHdl ) );
    xSerialInternalHandle *pxSerialIntHdl = &xSerialHdl[ucExpectedPortArg];

    CU_ASSERT_FATAL( IDX_INVALID != pxSerialIntHdl->ubIdx );
    pxSerialIntHdl->bFailOnTxStart = bFail;
}

void
vMBMTestFailOnRxStart( UCHAR ucExpectedPortArg, BOOL bFail )
{
    CU_ASSERT_FATAL( ucExpectedPortArg < MB_UTILS_NARRSIZE( xSerialHdl ) );
    xSerialInternalHandle *pxSerialIntHdl = &xSerialHdl[ucExpectedPortArg];

    CU_ASSERT_FATAL( IDX_INVALID != pxSerialIntHdl->ubIdx );
    pxSerialIntHdl->bFailOnRxStart = bFail;
}


eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdlArg, pvMBPSerialReceiverCB pvMBPReceiveFN )
{
    eMBErrorCode    eStatus = MB_EIO;
    xSerialInternalHandle *pxSerialIntHdl = xSerialHdlArg;

    CU_ASSERT_FATAL( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdl ) );

    if( !pxSerialIntHdl->bFailOnRxStart )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( NULL != pvMBPReceiveFN )
        {
            pxSerialIntHdl->bIsRxEnabled = TRUE;
            pxSerialIntHdl->pvMBPReceiveFN = pvMBPReceiveFN;
        }
        else
        {
            pxSerialIntHdl->bIsRxEnabled = FALSE;
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
        }
        MBP_EXIT_CRITICAL_SECTION(  );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}


eMBErrorCode
eMBPSerialTxEnable( xMBPSerialHandle xSerialHdlArg, pbMBPSerialTransmitterEmptyCB pbMBPTransmitterEmptyFN )
{
    eMBErrorCode    eStatus = MB_EIO;
    xSerialInternalHandle *pxSerialIntHdl = xSerialHdlArg;

    CU_ASSERT_FATAL( NULL != xSerialHdlArg );

    if( !pxSerialIntHdl->bFailOnTxStart )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( NULL != pbMBPTransmitterEmptyFN )
        {
            pxSerialIntHdl->bIsTxEnabled = TRUE;
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = pbMBPTransmitterEmptyFN;
        }
        else
        {
            pxSerialIntHdl->bIsTxEnabled = FALSE;
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
        }
        MBP_EXIT_CRITICAL_SECTION(  );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

STATIC void    *
vMBMTestRequestTXThread( void *pvData )
{
    xSerialTxThreadData *pxThreadData = pvData;
    BOOL            isTxEnabled = FALSE;
    BOOL            bMoreTXData;
    UBYTE           ubExpectedCharacter;

    /* Wait until the transmitter is enabled. */
    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        isTxEnabled = pxThreadData->pxSerialIntHdl->bIsTxEnabled;
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    while( !isTxEnabled );

    /* Now start the transmission. */
    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        isTxEnabled = pxThreadData->pxSerialIntHdl->bIsTxEnabled;
        MBP_EXIT_CRITICAL_SECTION(  );
        if( isTxEnabled )
        {
            CU_ASSERT_FATAL( NULL != pxThreadData->pxSerialIntHdl->pbMBPTransmitterEmptyFN );
            bMoreTXData =
                pxThreadData->pxSerialIntHdl->pbMBPTransmitterEmptyFN( pxThreadData->pxSerialIntHdl->xMBHdl,
                                                                       &ubExpectedCharacter );
            if( !bMoreTXData )
            {
                ( void )eMBPSerialTxEnable( pxThreadData->pxSerialIntHdl, NULL );
            }
            else
            {
                CU_ASSERT_EQUAL( ubExpectedCharacter,
                                 pxThreadData->arubExpectedRequest[pxThreadData->usExpectedRequestCurPos] );
                pxThreadData->usExpectedRequestCurPos++;
            }
        }
    }
    while( isTxEnabled );

    return NULL;
}


void
vMBMTestRequestTransmit( UCHAR ucExpectedPortArg, UBYTE arubExpectedRequest[], USHORT usExpectedRequestLen )
{
    pthread_t       xThreadHdl;
    int             iRes;

    CU_ASSERT_FATAL( usExpectedRequestLen < EXPECTED_REQUEST_BUFFER_SIZE );
    CU_ASSERT_FATAL( ucExpectedPortArg < MB_UTILS_NARRSIZE( xSerialHdl ) );

    xSerialTxThreadData *pxThreadData = malloc( sizeof( xSerialTxThreadData ) );

    CU_ASSERT_FATAL( pxThreadData != NULL );

    MBP_ENTER_CRITICAL_SECTION(  );
    pxThreadData->usExpectedRequestLen = usExpectedRequestLen;
    pxThreadData->usExpectedRequestCurPos = 0;
    pxThreadData->pxSerialIntHdl = &xSerialHdl[ucExpectedPortArg];
    memcpy( &( pxThreadData->arubExpectedRequest[0] ), arubExpectedRequest, usExpectedRequestLen );
    CU_ASSERT_FATAL( 0 == ( iRes = pthread_create( &xThreadHdl, NULL, vMBMTestRequestTXThread, pxThreadData ) ) );
    MBP_EXIT_CRITICAL_SECTION(  );
}

void
vMBMTestPreparedResponse( UCHAR ucExpectedPortArg, UBYTE arubPreparedResponse[], USHORT usPreparedResponseLen )
{
    pthread_t       xThreadHdl;
    int             iRes;

    CU_ASSERT_FATAL( usPreparedResponseLen < EXPECTED_REQUEST_BUFFER_SIZE );
    CU_ASSERT_FATAL( ucExpectedPortArg < MB_UTILS_NARRSIZE( xSerialHdl ) );

    xSerialRxThreadData *pxThreadData = malloc( sizeof( xSerialRxThreadData ) );

    CU_ASSERT_FATAL( pxThreadData != NULL );
    MBP_ENTER_CRITICAL_SECTION(  );
    pxThreadData->pxSerialIntHdl = &xSerialHdl[ucExpectedPortArg];
    pxThreadData->usPreparedResponseLen = usPreparedResponseLen;
    memcpy( &( pxThreadData->arubPreparedResponse[0] ), arubPreparedResponse, usPreparedResponseLen );
    CU_ASSERT_FATAL( 0 == ( iRes = pthread_create( &xThreadHdl, NULL, vMBMTestResponseRXThread, pxThreadData ) ) );
    MBP_EXIT_CRITICAL_SECTION(  );
}

STATIC void    *
vMBMTestResponseRXThread( void *pvThreadData )
{
    xMBMInternalHandle *pxIntHdl;
    xSerialRxThreadData *pxThreadData = pvThreadData;
    BOOL            isRxEnabled = FALSE;
    USHORT          usRxBufPos = 0;

    /* Wait until the transmitter is enabled. */
    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        isRxEnabled = pxThreadData->pxSerialIntHdl->bIsRxEnabled;
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    while( !isRxEnabled );

    if( pxThreadData->pxSerialIntHdl->bTimeoutOnRx )
    {
        pxIntHdl = pxThreadData->pxSerialIntHdl->xMBHdl;
        CU_ASSERT_FATAL( MB_ENOERR == eMBPEventPost( pxIntHdl->xFrameEventHdl, MBM_EV_TIMEDOUT ) );
    }
    else
    {
        /* Now start the transmission. */
        do
        {
            MBP_ENTER_CRITICAL_SECTION(  );
            isRxEnabled = pxThreadData->pxSerialIntHdl->bIsRxEnabled;
            MBP_EXIT_CRITICAL_SECTION(  );
            if( isRxEnabled )
            {
                CU_ASSERT_FATAL( NULL != pxThreadData->pxSerialIntHdl->pvMBPReceiveFN );
                pxThreadData->pxSerialIntHdl->pvMBPReceiveFN( pxThreadData->pxSerialIntHdl->xMBHdl,
                                                              pxThreadData->arubPreparedResponse[usRxBufPos] );
                usRxBufPos++;
            }
        }
        while( isRxEnabled && usRxBufPos < ( pxThreadData->usPreparedResponseLen ) );

        /* Tell the stack that the frame has been received. */
        bMBMSerialRTUT35CB( pxThreadData->pxSerialIntHdl->xMBHdl );
    }

    return NULL;
}

void
vMBMTestSetResponseTimeoutOnRx( UCHAR ucExpectedPortArg, BOOL bTimeout )
{
    CU_ASSERT_FATAL( ucExpectedPortArg < MB_UTILS_NARRSIZE( xSerialHdl ) );
    xSerialHdl[ucExpectedPortArg].bTimeoutOnRx = bTimeout;
}

void
vMBMTestSetExpectedSerialInit( UCHAR ucExpectedPortArg, ULONG ulExpectedBaudRateArg,
                               UCHAR ucExpectedDataBitsArg, UCHAR ucExpectedStopBitsArg,
                               eMBSerialParity eExpectedParityArg )
{
    ucExpectedPort = ucExpectedPortArg;
    ulExpectedBaudRate = ulExpectedBaudRateArg;
    ucExpectedDataBits = ucExpectedDataBitsArg;
    ucExpectedStopBits = ucExpectedStopBitsArg;
    eExpectedParity = eExpectedParityArg;
}

void
vMBMTestSetFailOnInit( BOOL bFail )
{
    bFailOnInit = bFail;
}
