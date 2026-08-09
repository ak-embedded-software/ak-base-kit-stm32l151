/* 
 * MODBUS Library: Example for Win32
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo.cpp,v 1.2 2008-11-02 17:44:52 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <assert.h>

#include "stdafx.h"
#include "mbmaster.h"

/* ----------------------- Defines ------------------------------------------*/

#define MBM_SERIAL_PORT	          ( 1 )
#define MBM_SERIAL_BAUDRATE       ( 19200 )
#define MBM_PARITY                ( MB_PAR_NONE )

#define MBM_DLL                     _T( "mbmaster.dll" )
/* ----------------------- Type definitions ---------------------------------*/
typedef         eMBErrorCode( __cdecl * peMBMSerialInit_t ) ( xMBMHandle * pxHdl, eMBSerialMode eMode, UCHAR ucPort,
                                                              ULONG ulBaudRate, eMBSerialParity eParity );
typedef         eMBErrorCode( __cdecl * peMBMClose_t ) ( xMBMHandle xHdl );
typedef         eMBErrorCode( __cdecl * peMBMWriteSingleRegister_t ) ( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                                                       USHORT usRegAddress, USHORT usValue );
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
    peMBMSerialInit_t peMBMSerialInit;
    peMBMClose_t    peMBMClose;
    peMBMWriteSingleRegister_t peMBMWriteSingleRegister;

    usNReloads = 10;
    do
    {
        _ftprintf( stderr, _T( "Loading DLL %s: " ), MBM_DLL );
        if( NULL != ( hMBMLibrary = LoadLibrary( MBM_DLL ) ) )
        {
            _ftprintf( stderr, _T( "okay\r\n" ) );

            peMBMSerialInit = ( peMBMSerialInit_t ) GetProcAddress( hMBMLibrary, "eMBMSerialInit" );
            peMBMClose = ( peMBMClose_t ) GetProcAddress( hMBMLibrary, "eMBMClose" );
            peMBMWriteSingleRegister =
                ( peMBMWriteSingleRegister_t ) GetProcAddress( hMBMLibrary, "eMBMWriteSingleRegister" );

            assert( NULL != peMBMSerialInit );
            assert( NULL != peMBMClose );
            assert( NULL != peMBMWriteSingleRegister );

            usNumberStackRestarts = 5;
            do
            {
                if( MB_ENOERR ==
                    ( eStatus =
                      peMBMSerialInit( &xMBMMaster, MB_RTU, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
                {
                    _ftprintf( stderr, _T( "Started MODBUS master (MODE=%s,PORT=%d,BAUDRATE=%d)\r\n" ),
                               szMBMode2String( MB_RTU ), ( int )MBM_SERIAL_PORT, ( int )MBM_SERIAL_BAUDRATE );
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

                    if( MB_ENOERR != ( eStatus = peMBMClose( xMBMMaster ) ) )
                    {
                    }

                    /* The serial handle is not released instantly and without a
                     * delay an error would occur and the init would fail maybe
                     * for the first time. 
                     */
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
