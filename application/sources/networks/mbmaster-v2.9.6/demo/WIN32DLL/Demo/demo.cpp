/* 
 * MODBUS Library: Example for Win32
 * Copyright (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo.cpp,v 1.1 2008/06/08 22:27:20 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdafx.h"
#include "mbport.h"
#include "mbm.h"

/* ----------------------- Defines ------------------------------------------*/

#define MBM_SERIAL_PORT	            ( 1 )
#define MBM_SERIAL_BAUDRATE         ( 38400 )
#define MBM_PARITY                  ( MB_PAR_ODD )
#define MBM_MODE                    ( MB_RTU )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
static LPTSTR   prvszMBMode2String( eMBSerialMode eMode );
static LPTSTR   prvszMBParity2String( eMBSerialParity eParity );

/* ----------------------- Start implementation -----------------------------*/

int
_tmain( int argc, _TCHAR * argv[] )
{

    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[20];
    USHORT          usRegCnt = 0;
    UBYTE           ubIdx;

    do
    {
        if( MB_ENOERR ==
            ( eStatus = eMBMSerialInit( &xMBMMaster, MBM_MODE, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
        {
            Sleep( 1000 );
            _ftprintf( stderr, _T( "MODBUS master instance ready (MODE=%s, PORT=%d, BAUDRATE=%d, PARITY=%s)\r\n" ),
                       prvszMBMode2String( MBM_MODE ), MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE,
                       prvszMBParity2String( MBM_PARITY ) );
            eStatus = MB_ENOERR;

            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
            {
                eStatus = eStatus2;
            }

            /* Read holding register from adress 5 - 25, increment them by one and store
             * them at address 10. 
             */
            if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 5, 20, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }
            for( ubIdx = 0; ubIdx < 20; ubIdx++ )
            {
                usNRegs[ubIdx]++;
            }
            if( MB_ENOERR != ( eStatus2 = eMBMWriteMultipleRegisters( xMBMMaster, 1, 5, 20, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }

            /* Read the input registers from address 2 - 5 and write them to the holding
             * registers at address 1 - 4.
             */
            if( MB_ENOERR != ( eStatus2 = eMBMReadInputRegisters( xMBMMaster, 1, 2, 4, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }
            if( MB_ENOERR != ( eStatus2 = eMBMWriteMultipleRegisters( xMBMMaster, 1, 1, 4, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }

            _ftprintf( stderr, _T( "  reading/writing slave registers: " ) );
            if( MB_ENOERR == eStatus )
            {
                _ftprintf( stderr, _T( "okay\r\n" ) );
            }
            else
            {
                _ftprintf( stderr, _T( "failed\r\n" ) );
            }

            if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
            {

            }
        }
        else
        {
            _ftprintf( stderr, _T( "Can't start MODBUS master instance (MODE=%s, PORT=%d, BAUDRATE=%d, PARITY=%s)!" ),
                       prvszMBMode2String( MBM_MODE ), MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE,
                       prvszMBParity2String( MBM_PARITY ) );
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
