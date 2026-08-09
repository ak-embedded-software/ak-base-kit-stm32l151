/* 
 * MODBUS Library: Example applicaion for LINUX/CYGWIN
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-ser.c,v 1.6 2009-10-24 16:57:18 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <unistd.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/

#define MBM_SERIAL_PORT	          ( 0 )
#define MBM_SERIAL_BAUDRATE       ( 19200 )
#define MBM_PARITY                ( MB_PAR_NONE )
#define MBM_MODE                  ( MB_RTU )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC char    *prvszMBMode2String( eMBSerialMode eMode );
STATIC char    *prvszMBParity2String( eMBSerialParity eParity );

/* ----------------------- Start implementation -----------------------------*/
int
main( int argc, char *argv[] )
{
    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[20];
    USHORT          usRegCnt = 0;
    UBYTE           ubIdx;

    vMBPOtherDLLInit(  );

    do
    {
        if( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMMaster, MBM_MODE, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
        {
            fprintf( stderr, "MODBUS master instance ready (MODE=%s, PORT=%d, BAUDRATE=%d, PARITY=%s)\n",
                     prvszMBMode2String( MBM_MODE ), MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, prvszMBParity2String( MBM_PARITY ) );
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
            fprintf( stderr, "  reading/writing slave registers: " );
            if( MB_ENOERR == eStatus )
            {
                fprintf( stderr, "okay\n" );
            }
            else
            {
                fprintf( stderr, "failed\n" );
            }
            if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
            {
                MBP_ASSERT( 0 );
            }
        }
        else
        {
            fprintf( stderr, "Can't start MODBUS master instance (MODE=%s, PORT=%d, BAUDRATE=%d, PARITY=%s)!\n",
                     prvszMBMode2String( MBM_MODE ), MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, prvszMBParity2String( MBM_PARITY ) );
        }
        /* Wait 100ms before next try. */
    }
    while( TRUE );
    return 0;
}

STATIC char    *
prvszMBMode2String( eMBSerialMode eMode )
{
    char           *szMode;

    switch ( eMode )
    {
    case MB_RTU:
        szMode = "RTU";
        break;
    case MB_ASCII:
        szMode = "ASCII";
        break;
    default:
        szMode = "unknown";
        break;
    }
    return szMode;
}

STATIC char    *
prvszMBParity2String( eMBSerialParity eParity )
{
    char           *szParity;

    switch ( eParity )
    {
    case MB_PAR_EVEN:
        szParity = "RTU";
        break;
    case MB_PAR_NONE:
        szParity = "ASCII";
        break;
    case MB_PAR_ODD:
        szParity = "ODD";
        break;
    default:
        szParity = "unknown";
        break;
    }
    return szParity;
}
