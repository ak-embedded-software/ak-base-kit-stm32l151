/* 
 * MODBUS Library: Example for Win32
 * Copyright (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-readonly.cpp,v 1.4 2010-01-13 19:48:06 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <time.h>
#include "stdafx.h"
#include "mbport.h"
#include "mbm.h"

/* ----------------------- Defines ------------------------------------------*/

#define MBM_SERIAL_PORT	            ( 7 )
#define MBM_SERIAL_BAUDRATE         ( 19200 )
#define MBM_PARITY                  ( MB_PAR_NONE )
#define MBM_MODE                    ( MB_RTU )
#define MBM_USE_TCP                 ( 0 )
#define MBM_CLIENT_HOSTNAME         ( "localhost" )
#define MBM_CLIENT_PORT             ( 502 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
static LPTSTR   prvszMBMode2String( eMBSerialMode eMode );
static LPTSTR   prvszMBParity2String( eMBSerialParity eParity );

/* ----------------------- Start implementation -----------------------------*/

int
_tmain( int argc, _TCHAR * argv[] )
{

    UBYTE           ubSlaveAddress[2] = { 1, 2 };
    UBYTE           ubIdx;
    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[125];
    USHORT          usRegCnt = 0;

    do
    {
#if MBM_USE_TCP == 1
        if( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMMaster ) ) )
        {
        }
        else if( MB_ENOERR == eMBMTCPConnect( xMBMMaster, MBM_CLIENT_HOSTNAME, MBM_CLIENT_PORT ) )
        {
            _ftprintf( stderr, _T( "MODBUS master instance ready (MODE=TCP, HOST=%s, PORT=%d)\r\n" ),
                       MBM_CLIENT_HOSTNAME, MBM_CLIENT_PORT );
#else
        if( MB_ENOERR == eMBMSerialInit( &xMBMMaster, MBM_MODE, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) )
        {
            _ftprintf( stderr, _T( "MODBUS master instance ready (MODE=%s, PORT=%d, BAUDRATE=%d, PARITY=%s)\r\n" ),
                       prvszMBMode2String( MBM_MODE ), MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE,
                       prvszMBParity2String( MBM_PARITY ) );
#endif            
            eMBMSetSlaveTimeout( xMBMMaster, 1000 );
            for( ubIdx = 0; ubIdx < 2; ubIdx++ )
            {
                eStatus = MB_ENOERR;
                usRegCnt = ( rand(  ) + 1 ) % ( sizeof( usNRegs ) / sizeof( usNRegs[ 0 ] ) );
                
                if( MB_ENOERR !=
                    ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, ubSlaveAddress[ ubIdx ], 0, usRegCnt, usNRegs ) ) )
                {
                    eStatus = eStatus2;
                }
                _ftprintf( stderr, _T( "%lu: [slave %d] read %d slave registers : " ), ( ULONG ) clock(  ),
                           ubSlaveAddress[ubIdx], usRegCnt );
                if( MB_ENOERR == eStatus )
                {
                    _ftprintf( stderr, _T( "okay\r\n" ) );
                }
                else
                {
                    _ftprintf( stderr, _T( "failed. error = %d\r\n" ), ( int )eStatus );
                }
            }

            if( MB_ENOERR != eMBMClose( xMBMMaster ) )
            {
                _ftprintf( stderr, _T( "Can't close MODBUS master instance!\r\n" ) );
            }
        }
        else
        {
            _ftprintf( stderr, _T( "Can't start MODBUS master instance!\r\n" ) );
        }
        /* Wait 100ms before next try. */
        Sleep( 100 );
    }
    while( TRUE );
    return 0;
}


static          LPTSTR
prvszMBMode2String( eMBSerialMode eMode )
{
    LPTSTR          szMode;

    switch ( eMode )
    {
    case MB_RTU:
        szMode = _T( "RTU" );
        break;
    case MB_ASCII:
        szMode = _T( "ASCII" );
        break;
    default:
        szMode = _T( "unknown" );
        break;
    }
    return szMode;
}

static          LPTSTR
prvszMBParity2String( eMBSerialParity eParity )
{
    LPTSTR          szParity;

    switch ( eParity )
    {
    case MB_PAR_EVEN:
        szParity = _T( "EVEN" );
        break;
    case MB_PAR_NONE:
        szParity = _T( "NONE" );
        break;
    case MB_PAR_ODD:
        szParity = _T( "ODD" );
        break;
    default:
        szParity = _T( "unknown" );
        break;
    }
    return szParity;
}
