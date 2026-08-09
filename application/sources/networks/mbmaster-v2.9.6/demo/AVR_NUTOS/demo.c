/* 
 * MODBUS Library: Example for Nut/OS for XNUT-100
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo.c,v 1.1 2010-02-21 19:23:06 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <cfg/crt.h>
#include <io.h>
#include <sys/timer.h>

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
STATIC void     vStatusLEDSet( BOOL bError );

/* ----------------------- Start implementation -----------------------------*/
int
main( void )
{

    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[20];
    USHORT          usRegCnt = 0;
    UBYTE           ubIdx;

    vMBPInit(  );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER, "starting...\n" );
#endif
    NutSleep( 2000 );

    do
    {
        if( MB_ENOERR ==
            ( eStatus = eMBMSerialInit( &xMBMMaster, MBM_MODE, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
        {
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

            if( MB_ENOERR == eStatus )
            {
                vStatusLEDSet( FALSE );
            }
            else
            {
                vStatusLEDSet( TRUE );
            }
            if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
            {
                MBP_ASSERT( 0 );
            }
        }
    }
    while( TRUE );
    return 0;
}

STATIC void
vStatusLEDSet( BOOL bError )
{
    if( bError )
    {
        vMBPSetLED( 0, 2 );
    }
    else
    {
        vMBPSetLED( 0, 1 );
    }
}
