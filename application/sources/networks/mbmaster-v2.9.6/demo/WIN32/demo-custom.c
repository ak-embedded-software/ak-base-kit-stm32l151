/* 
 * MODBUS Library: Example for a custom MODBUS function for Win32
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-custom.c,v 1.1 2008-11-02 17:12:06 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdafx.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/

#define MBM_SERIAL_PORT             ( 1 )
#define MBM_SERIAL_BAUDRATE         ( 19200 )
#define MBM_PARITY                  ( MB_PAR_NONE )

#define TEST_LOOPCNT                ( 1 )
#define TEST_SLAVEADDRESS           ( 1 )
#define TEST_FUNCTIONCODE           ( 65 )
#define TEST_PAYLOADMAX             ( 252 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

int
_tmain( int argc, _TCHAR * argv[] )
{
    int             i;
    BOOL            bUsageError = FALSE;
    UCHAR           ucSerialPort = MBM_SERIAL_PORT;
    UBYTE           arubPayloadIn[TEST_PAYLOADMAX];
    UBYTE           arubPayloadOut[TEST_PAYLOADMAX];
    ULONG           ulNLoopCnt = TEST_LOOPCNT;
    ULONG           ulNLoopCntCur;
    UBYTE           ubBytesReceived;
    eMBErrorCode    eStatus;
    xMBHandle       xMBMMaster;

    memset( arubPayloadIn, 0, sizeof( arubPayloadIn ) / sizeof( arubPayloadIn[0] ) );
    srand( ( unsigned )time( NULL ) );
    for( i = 1; i < argc; i++ )
    {
        if( 0 == _tcscmp( argv[i], _T( "-p" ) ) )
        {
            i++;
            if( i < argc )
            {
                ucSerialPort = ( UCHAR ) _tstof( argv[i] );
            }
            else
            {
                bUsageError = TRUE;
                break;
            }
        }
        else if( 0 == _tcscmp( argv[i], _T( "-c" ) ) )
        {
            i++;
            if( i < argc )
            {
                ulNLoopCnt = ( UCHAR ) _tstof( argv[i] );
            }
            else
            {
                bUsageError = TRUE;
                break;
            }
        }
        else if( 0 == _tcscmp( argv[i], _T( "-h" ) ) )
        {
            bUsageError = TRUE;
        }
        else
        {
            bUsageError = TRUE;
        }
    }
    if( bUsageError )
    {
        _ftprintf( stderr, _T( "Usage: demo.o [-p X] [-h]\r\n\r\n" ) );
        _ftprintf( stderr, _T( " -p X          ... Use X for your COM port\r\n" ) );
        _ftprintf( stderr, _T( " -h            ... Print this information\r\n" ) );
        _ftprintf( stderr, _T( " -c X          ... Repeat X times\r\n" ) );
        _ftprintf( stderr, _T( "(C) 2008, Embedded Solutions\r\n" ) );
    }
    else
    {
        vMBPOtherDLLInit(  );
        _ftprintf( stderr, _T( "INFO: Using COM port %d\r\n" ), ( int )ucSerialPort );
        if( MB_ENOERR !=
            ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, ucSerialPort, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
        {
            _ftprintf( stderr, _T( "ERROR: Can't open communication stack.\r\n" ) );
            eStatus = MB_ENOERR;
        }
        else
        {
            for( ulNLoopCntCur = 0; ulNLoopCntCur < ulNLoopCnt; ulNLoopCntCur++ )
            {
                /* Prepare buffer to send. */
                UBYTE           ubSize = 1 + ( UBYTE ) ( rand(  ) % ( TEST_PAYLOADMAX - 1 ) );

                arubPayloadIn[0] = ( UBYTE ) ( ulNLoopCntCur & 0xFF );

                _ftprintf( stderr, _T( "[%lu]: Send %d bytes: " ), ulNLoopCntCur, ( int )ubSize );
                eStatus = eMBMReadWriteRAWPDU( xMBMMaster, TEST_SLAVEADDRESS, TEST_FUNCTIONCODE,
                                               arubPayloadIn, ubSize,
                                               arubPayloadOut, sizeof( arubPayloadOut ) / sizeof( arubPayloadOut[0] ),
                                               &ubBytesReceived );

                switch ( eStatus )
                {
                case MB_EINVAL:
                    _ftprintf( stderr, _T( "Wrong arguments or wrong slave response.\r\n" ) );
                    break;

                case MB_ENOERR:
                    _ftprintf( stderr, _T( "Received %d bytes.\r\n" ), ( int )ubBytesReceived );
                    break;

                case MB_ETIMEDOUT:
                    _ftprintf( stderr, _T( "Timeout!\r\n" ) );
                    break;
                default:
                    _ftprintf( stderr, _T( "Got error code %02X!\r\n" ), ( int )eStatus );
                    break;
                }
            }
            if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
            {
                MBP_ASSERT( 0 );
            }
        }
        vMBPOtherDLLClose(  );
    }
    return 0;
}
