/* 
 * MODBUS Library: WIN32
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-tcp-multi.c,v 1.2 2008-11-02 17:12:07 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
/* 
 * MODBUS Library: Example for Win32
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-tcp-multi.c,v 1.2 2008-11-02 17:12:07 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdafx.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/

#define MASTER1_PORT	            ( 1 )
#define MASTER1_BAUDRATE            ( 19200 )
#define MASTER1_PARITY              ( MB_PAR_NONE )

#define CLIENT2_HOSTNAME            "127.0.0.1"
#define CLIENT2_PORT                502
/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
STATIC CRITICAL_SECTION xCritSection;
STATIC struct
{
    BOOL            bMaster1Okay;
    BOOL            bMaster2Okay;
    BOOL            bFinished;
} xThreadData;

/* ----------------------- Static functions ---------------------------------*/
STATIC DWORD WINAPI pvHandlerThread1( LPVOID lpParameter );
STATIC DWORD WINAPI pvHandlerThread2( LPVOID lpParameter );

/* ----------------------- Start implementation -----------------------------*/

int
_tmain( int argc, _TCHAR * argv[] )
{
    DWORD           dwThread1Id, dwThread2Id;
    HANDLE          xThread1Hdl = NULL;
    HANDLE          xThread2Hdl = NULL;
    TCHAR           cCmd;

    xThreadData.bMaster1Okay = FALSE;
    xThreadData.bMaster2Okay = FALSE;
    xThreadData.bFinished = FALSE;
    InitializeCriticalSection( &xCritSection );

    vMBPOtherDLLInit(  );

    if( NULL == ( xThread1Hdl = CreateThread( NULL, 0, pvHandlerThread1, NULL, 0, &dwThread1Id ) ) )
    {
    }
    else if( NULL == ( xThread2Hdl = CreateThread( NULL, 0, pvHandlerThread2, NULL, 0, &dwThread2Id ) ) )
    {
    }
    else
    {
        do
        {
            _tprintf( _T( ">" ) );
            cCmd = _gettchar(  );
            if( _T( 'q' ) == cCmd )
            {
                EnterCriticalSection( &xCritSection );
                xThreadData.bFinished = TRUE;
                LeaveCriticalSection( &xCritSection );
            }
            if( _T( 's' ) == cCmd )
            {
                EnterCriticalSection( &xCritSection );
                _tprintf( _T( "MASTER 1 RUNNING: %d\tMASTER 2 RUNNING: %d\r\n" ), xThreadData.bMaster1Okay,
                          xThreadData.bMaster2Okay );
                LeaveCriticalSection( &xCritSection );
            }

        }
        while( !xThreadData.bFinished );
    }
    EnterCriticalSection( &xCritSection );
    xThreadData.bFinished = TRUE;
    LeaveCriticalSection( &xCritSection );
    if( NULL != xThread1Hdl )
    {
        if( WAIT_OBJECT_0 != WaitForSingleObject( xThread1Hdl, 5000 ) )
        {
            assert( 0 );
        }
    }
    if( NULL != xThread2Hdl )
    {
        if( WAIT_OBJECT_0 != WaitForSingleObject( xThread2Hdl, 5000 ) )
        {
            assert( 0 );
        }
    }

    vMBPOtherDLLClose(  );
    return 0;
}

STATIC DWORD    WINAPI
pvHandlerThread1( LPVOID lpParameter )
{
    xMBHandle       xMBMMaster;
    BOOL            bIsRunning;
    eMBErrorCode    eStatus = MB_ENOERR, eStatus2;
    USHORT          usLoopCnt = 0;
    USHORT          usNRegs[25];

    do
    {
        if( MB_ENOERR ==
            ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MASTER1_PORT, MASTER1_BAUDRATE, MASTER1_PARITY ) ) )
        {

            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usLoopCnt++ ) ) )
            {
                eStatus = eStatus2;
            }
            if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 1, _countof( usNRegs ), usNRegs ) ) )
            {
                eStatus = eStatus2;
            }
            if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
            {
                MBP_ASSERT( 0 );
            }
        }
        /* Wait 50ms before next try. */
        Sleep( 50 );

        EnterCriticalSection( &xCritSection );
        bIsRunning = !xThreadData.bFinished;
        xThreadData.bMaster1Okay = eStatus == MB_ENOERR ? TRUE : FALSE;
        LeaveCriticalSection( &xCritSection );
    }
    while( bIsRunning );

    ExitThread( 0 );
}

STATIC DWORD    WINAPI
pvHandlerThread2( LPVOID lpParameter )
{
    xMBHandle       xMBMMaster;
    eMBErrorCode    eStatus, eStatus2;
    USHORT          usLoopCnt = 0;
    USHORT          usNRegs[1];
    BOOL            bIsRunning;

    do
    {
        if( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMMaster ) ) )
        {
            if( MB_ENOERR == eMBMTCPConnect( xMBMMaster, CLIENT2_HOSTNAME, CLIENT2_PORT ) )
            {
                if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usLoopCnt++ ) ) )
                {
                    eStatus = eStatus2;
                }
                if( MB_ENOERR !=
                    ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 1, _countof( usNRegs ), usNRegs ) ) )
                {
                    eStatus = eStatus2;
                }
                /* Now disconnect. */
                if( MB_ENOERR != ( eStatus2 = eMBMTCPDisconnect( xMBMMaster ) ) )
                {
                    eStatus = eStatus2;
                }
            }
            if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
            {
                MBP_ASSERT( 0 );
            }

            /* Wait 50ms before next try. */
            Sleep( 50 );

            EnterCriticalSection( &xCritSection );
            bIsRunning = !xThreadData.bFinished;
            xThreadData.bMaster2Okay = eStatus == MB_ENOERR ? TRUE : FALSE;
            LeaveCriticalSection( &xCritSection );
        }
    }
    while( bIsRunning );

    ExitThread( 0 );
}
