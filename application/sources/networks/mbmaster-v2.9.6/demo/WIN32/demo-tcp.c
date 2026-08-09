/* 
 * MODBUS Library: WIN32
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-tcp.c,v 1.2 2008-11-02 17:12:07 cwalter Exp $
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
#define CLIENT_PORT                 502

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
    int             iNConnects;
    int             iPolls = 2;
    int             i;

    vMBPOtherDLLInit(  );

    do
    {
        if( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMMaster ) ) )
        {
            iNConnects = 5;
            do
            {
                if( MB_ENOERR == eMBMTCPConnect( xMBMMaster, CLIENT_HOSTNAME, CLIENT_PORT ) )
                {
                    eStatus = MB_ENOERR;

                    for( i = 0; i < 10; i++ )
                    {
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
                    }

                    /* Now disconnect. */
                    if( MB_ENOERR != ( eStatus2 = eMBMTCPDisconnect( xMBMMaster ) ) )
                    {
                        eStatus = eStatus2;
                    }

                    _ftprintf( stderr, _T( "poll cycle: %s\r\n" ),
                               eStatus == MB_ENOERR ? _T( "okay" ) : _T( "failed" ) );
                }
                else
                {
                    _ftprintf( stderr, _T( "failed to connect to %s:%hu!\r\n" ), _T( CLIENT_HOSTNAME ), CLIENT_PORT );
                }
            }
            while( iNConnects-- > 0 );
        }
        else
        {
            MBP_ASSERT( 0 );
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
