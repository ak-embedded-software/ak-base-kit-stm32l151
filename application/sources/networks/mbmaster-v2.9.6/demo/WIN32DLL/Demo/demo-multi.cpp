/* 
 * MODBUS Library: Example for Win32
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-multi.cpp,v 1.4 2009-12-07 18:26:16 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <assert.h>
#include "stdafx.h"
#include "mbport.h"
#include "mbm.h"

/* ----------------------- Defines ------------------------------------------*/
#define STATIC static

#define MBM1_SERIAL_PORT	        ( 5 )
#define MBM1_SERIAL_BAUDRATE      ( 19200 )
#define MBM1_PARITY               ( MB_PAR_NONE )

#define MBM2_SERIAL_PORT	        ( 1 )
#define MBM2_SERIAL_BAUDRATE      ( 38400 )
#define MBM2_PARITY               ( MB_PAR_NONE )

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
STATIC DWORD WINAPI prrvMBTask1( LPVOID lpParameter );
STATIC DWORD WINAPI prrvMBTask2( LPVOID lpParameter );

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

    if( NULL == ( xThread1Hdl = CreateThread( NULL, 0, prrvMBTask1, NULL, 0, &dwThread1Id ) ) )
    {
    }
    else if( NULL == ( xThread2Hdl = CreateThread( NULL, 0, prrvMBTask2, NULL, 0, &dwThread2Id ) ) )
    {
    }
    else
    {
        _tprintf( _T( "Press 's' to get status information.\r\n" ) );
        _tprintf( _T( "Press 'q' to quit.\r\n\r\n" ) );

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
            ( void )fflush( stdin );
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

    return 0;
}

STATIC DWORD    WINAPI
prrvMBTask1( LPVOID lpParameter )
{
    eMBErrorCode    eStatus, eStatus2;
    xMBMHandle      xMBMMaster;
    BOOL            bIsRunning;
    USHORT          usRegCnt = 0;

    do
    {
        if( MB_ENOERR ==
            ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MBM1_SERIAL_PORT, MBM1_SERIAL_BAUDRATE, MBM1_PARITY ) ) )
        {
            eStatus = MB_ENOERR;

            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
            {
                eStatus = eStatus2;
            }

            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
            {
                eStatus = eStatus2;
            }

            if( MB_ENOERR != ( eStatus2 = eMBMClose( xMBMMaster ) ) )
            {
                eStatus = eStatus2;
            }
        }
        /* Update thread status */
        EnterCriticalSection( &xCritSection );
        bIsRunning = !xThreadData.bFinished;
        xThreadData.bMaster1Okay = eStatus == MB_ENOERR ? TRUE : FALSE;
        LeaveCriticalSection( &xCritSection );
        /* Wait some time */
        Sleep( 50 );
    }
    while( bIsRunning );

    return NULL;
}

STATIC DWORD    WINAPI
prrvMBTask2( LPVOID lpParameter )
{
    eMBErrorCode    eStatus, eStatus2;
    xMBMHandle      xMBMMaster;
    BOOL            bIsRunning;
    USHORT          usRegCnt = 0;

    do
    {
        if( MB_ENOERR ==
            ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MBM2_SERIAL_PORT, MBM2_SERIAL_BAUDRATE, MBM2_PARITY ) ) )
        {
            eStatus = MB_ENOERR;

            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
            {
                eStatus = eStatus2;
            }

            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
            {
                eStatus = eStatus2;
            }

            if( MB_ENOERR != ( eStatus2 = eMBMClose( xMBMMaster ) ) )
            {
                eStatus = eStatus2;
            }
        }
        /* Update thread status */
        EnterCriticalSection( &xCritSection );
        bIsRunning = !xThreadData.bFinished;
        xThreadData.bMaster2Okay = eStatus == MB_ENOERR ? TRUE : FALSE;
        LeaveCriticalSection( &xCritSection );
        /* Wait some time */
        Sleep( 50 );
    }
    while( bIsRunning );

    return NULL;
}
