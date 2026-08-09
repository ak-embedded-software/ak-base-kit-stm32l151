/* 
 * MODBUS Library: WIN32
 * Copyright (c) 2008-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-udp.c,v 1.3 2011-12-04 21:11:46 embedded-solutions.cwalter Exp $
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

#define CLIENT_RETRIES              10
#define CLIENT_HOSTNAME             "127.0.0.1"
#define CLIENT_PORT                 503
#define SLAVE_ADDRESS				255

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
    int             iPolls = 5;
    int             i;

    vMBPOtherDLLInit(  );

    do
    {
        if( MB_ENOERR != ( eStatus = eMBMUDPInit( &xMBMMaster, "0.0.0.0", -1 ) ) )
        {
			_ftprintf( stderr, _T( "Can not initialize MODBUS stack: %d\n" ), eStatus );
        }
        else if( MB_ENOERR != ( eStatus = eMBMUDPSetSlave( xMBMMaster, CLIENT_HOSTNAME, CLIENT_PORT ) ) )
        {
			_ftprintf( stderr, _T( "Can not set slave: %d\n" ), eStatus );
        }
        else
        {
            for( i = 0; i < 10; i++ )
            {
                /* Write an incrementing counter to register address 1. */
                if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, SLAVE_ADDRESS, 1, usRegCnt++ ) ) )
                {
                    eStatus = eStatus2;
                }

                /* Read holding register from adress 5 - 10, increment them by one and store
                 * them at address 10. 
                 */
                if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, SLAVE_ADDRESS, 5, 5, usNRegs ) ) )
                {
                    eStatus = eStatus2;
                }
                for( ubIdx = 0; ubIdx < 5; ubIdx++ )
                {
                    usNRegs[ubIdx]++;
                }
                if( MB_ENOERR != ( eStatus2 = eMBMWriteMultipleRegisters( xMBMMaster, SLAVE_ADDRESS, 10, 5, usNRegs ) ) )
                {
                    eStatus = eStatus2;
                }
            }

            _ftprintf( stderr, _T( "poll cycle: %s\n" ), eStatus == MB_ENOERR ? _T( "okay" ) : _T( "failed" ) );
        }

        if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
        {
            MBP_ASSERT( 0 );
        }
    }
    while( iPolls-- > 0 );

    vMBPOtherDLLClose(  );
    return 0;
}
