/* 
 * MODBUS Library: Example for Win32
 * Copyright (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-tcp.cpp,v 1.2 2009-12-07 18:03:53 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <assert.h>

#include "stdafx.h"
#include "mbport.h"
#include "mbm.h"

/* ----------------------- Defines ------------------------------------------*/

#define CLIENT_HOSTNAME           "127.0.0.1"
#define CLIENT_PORT               ( 502 )

#define MBM_DLL                     _T( "mbmaster.dll" )
/* ----------------------- Type definitions ---------------------------------*/
typedef         eMBErrorCode( __cdecl * peMBMClose_t ) ( xMBMHandle xHdl );
typedef         eMBErrorCode( __cdecl * peMBMWriteSingleRegister_t ) ( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                                                       USHORT usRegAddress, USHORT usValue );
typedef         eMBErrorCode( __cdecl * peMBMTCPInit_t ) ( xMBMHandle * pxHdl );
typedef         eMBErrorCode( __cdecl * peMBMTCPConnect_t ) ( xMBMHandle xHdl, const CHAR * pcTCPClientAddress,
                                                              USHORT usTCPPort );
typedef         eMBErrorCode( __cdecl * peMBMTCPDisconnect_t ) ( xMBMHandle xHdl );

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

LPTSTR
szMBMode2String( eMBSerialMode eMode )
{
    switch ( eMode )
    {
    default:
        assert( 0 );
    case MB_RTU:
        return _T( "RTU" );
        break;
    case MB_ASCII:
        return _T( "ASCII" );
        break;
    }
}

int
_tmain( int argc, _TCHAR * argv[] )
{

    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usRegCnt = 0;
    USHORT          usNumberStackRestarts;
    USHORT          usNReloads;

    HINSTANCE       hMBMLibrary;
    peMBMClose_t    peMBMClose;
    peMBMWriteSingleRegister_t peMBMWriteSingleRegister;
    peMBMTCPInit_t  peMBMTCPInit;
    peMBMTCPConnect_t peMBMTCPConnect;
    peMBMTCPDisconnect_t peMBMTCPDisconnect;

    usNReloads = 10;
    do
    {
        _ftprintf( stderr, _T( "Loading DLL %s: " ), MBM_DLL );
        if( NULL != ( hMBMLibrary = LoadLibrary( MBM_DLL ) ) )
        {
            _ftprintf( stderr, _T( "okay\r\n" ) );

            peMBMClose = ( peMBMClose_t ) GetProcAddress( hMBMLibrary, "eMBMClose" );
            peMBMWriteSingleRegister =
                ( peMBMWriteSingleRegister_t ) GetProcAddress( hMBMLibrary, "eMBMWriteSingleRegister" );
            peMBMTCPInit = ( peMBMTCPInit_t ) GetProcAddress( hMBMLibrary, "eMBMTCPInit" );
            peMBMTCPConnect = ( peMBMTCPConnect_t ) GetProcAddress( hMBMLibrary, "eMBMTCPConnect" );
            peMBMTCPDisconnect = ( peMBMTCPDisconnect_t ) GetProcAddress( hMBMLibrary, "eMBMTCPDisconnect" );

            assert( NULL != peMBMClose );
            assert( NULL != peMBMWriteSingleRegister );
            assert( NULL != peMBMTCPInit );
            assert( NULL != peMBMTCPConnect );
            assert( NULL != peMBMTCPDisconnect );

            usNumberStackRestarts = 5;
            do
            {
                if( MB_ENOERR == ( eStatus = peMBMTCPInit( &xMBMMaster ) ) )
                {
                    _ftprintf( stderr, _T( "connecting to %s:%hu: " ), _T( CLIENT_HOSTNAME ), CLIENT_PORT );
                    if( MB_ENOERR == peMBMTCPConnect( xMBMMaster, CLIENT_HOSTNAME, CLIENT_PORT ) )
                    {
                        _ftprintf( stderr, _T( "okay\r\n" ) );
                        /* Write an incrementing counter to register address 0. */
                        if( MB_ENOERR != ( eStatus2 = peMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
                        {
                            eStatus = eStatus2;
                        }
                        /* Write an incrementing counter to register address 0. */
                        if( MB_ENOERR != ( eStatus2 = peMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
                        {
                            eStatus = eStatus2;
                        }
                        /* Now disconnect. */
                        if( MB_ENOERR != ( eStatus2 = peMBMTCPDisconnect( xMBMMaster ) ) )
                        {
                            eStatus = eStatus2;
                        }
                    }
                    else
                    {
                        _ftprintf( stderr, _T( "failed\r\n" ) );
                    }

                    if( MB_ENOERR != ( eStatus = peMBMClose( xMBMMaster ) ) )
                    {
                        assert( 0 );
                    }
                }
            }
            while( usNumberStackRestarts-- > 0 );

            _ftprintf( stderr, _T( "Unloading DLL %s: " ), MBM_DLL );
            if( FreeLibrary( hMBMLibrary ) )
            {
                _ftprintf( stderr, _T( "okay\r\n" ) );
            }
            else
            {
                _ftprintf( stderr, _T( "failed\r\n" ) );
            }

        }
        else
        {
            _ftprintf( stderr, _T( "failed\r\n" ) );
        }
    }
    while( usNReloads-- > 0 );

    return 0;
}
