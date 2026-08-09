/* 
 * MODBUS Library: Example for MODBUS master with RTU/ASCII
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmdemo-ser.c,v 1.1 2008-04-06 07:46:23 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
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

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

int
main( int argc, char *argv[] )
{

    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[5];
    USHORT          usRegCnt = 0;
    UBYTE           ubIdx;

    if( MB_ENOERR ==
        ( eStatus = eMBMSerialInit( &xMBMMaster, MB_ASCII, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
    {
        do
        {
            sleep( 1 );

            eStatus = MB_ENOERR;

            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
            {
                eStatus = eStatus2;
            }

            /* Read holding register from adress 5 - 10, increment them by one and store
             * them at address 10. 
             */
            if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 5, 5, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }
            for( ubIdx = 0; ubIdx < 5; ubIdx++ )
            {
                usNRegs[ubIdx]++;
            }
            if( MB_ENOERR != ( eStatus2 = eMBMWriteMultipleRegisters( xMBMMaster, 1, 10, 5, usNRegs ) ) )
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
        }
        while( TRUE );
    }
    else
    {
        MBP_ASSERT( 0 );
    }

    if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
    {
        MBP_ASSERT( 0 );
    }

    return 0;
}
