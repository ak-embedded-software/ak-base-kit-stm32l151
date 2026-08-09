/*
 * MODBUS Library: Example applicaion for LINUX/CYGWIN
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-tcp.c,v 1.6 2009-11-22 10:15:08 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"

/* ----------------------- Defines ------------------------------------------*/

#define CLIENT_RETRIES              10
/*
#define CLIENT_HOSTNAME             "213.129.231.170"
#define CLIENT_HOSTNAME             "192.168.56.3"
#define CLIENT_HOSTNAME             "192.168.56.1"
#define CLIENT_HOSTNAME             "192.168.56.3"
*/
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
        printf( "New poll cycle started, %d cycles left.\n", iPolls );
        if( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMMaster ) ) )
        {
            iNConnects = 5;
            do
            {
                if( MB_ENOERR == eMBMTCPConnect( xMBMMaster, CLIENT_HOSTNAME, CLIENT_PORT ) )
                {
                    printf( "New connection established, %d retries left.\n", iNConnects );
                    eStatus = MB_ENOERR;
                    for( i = 0; i < 50; i++ )
                    {
                        /* Write an incrementing counter to register address 0. */
                        if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
                        {
                            fprintf( stderr, "Function <eMBMWriteSingleRegister> failed %d.\n", eStatus2 );
                            eStatus = eStatus2;
                        }
                        /* Read holding register from adress 5 - 10, increment them by one and store
                         * them at address 10. 
                         */
                        if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 5, 5, usNRegs ) ) )
                        {
                            fprintf( stderr, "Function <eMBMReadHoldingRegisters> failed %d.\n", eStatus2 );
                            eStatus = eStatus2;
                        }

                        /* Read input register from adress 5 - 10
                         */
                        if( MB_ENOERR != ( eStatus2 = eMBMReadInputRegisters( xMBMMaster, 1, 5, 5, usNRegs ) ) )
                        {
                            fprintf( stderr, "Function <eMBMReadInputRegisters> failed %d.\n", eStatus2 );
                            eStatus = eStatus2;
                        }
                        for( ubIdx = 0; ubIdx < 5; ubIdx++ )
                        {
                            usNRegs[ubIdx]++;
                        }
#if 0
                        if( MB_ENOERR != ( eStatus2 = eMBMWriteMultipleRegisters( xMBMMaster, 1, 10, 5, usNRegs ) ) )
                        {
                            fprintf( stderr, "Function <eMBMWriteMultipleRegisters> failed %d.\n", eStatus2 );
                            eStatus = eStatus2;
                        }
#endif
                        if( MB_ENOERR != eStatus )
                        {
                            break;
                        }
                    }
                    /* Now disconnect. */
                    if( MB_ENOERR != ( eStatus2 = eMBMTCPDisconnect( xMBMMaster ) ) )
                    {
                        fprintf( stderr, "Function <eMBMTCPDisconnect> failed %d.\n", eStatus2 );
                        eStatus = eStatus2;
                    }
                    fprintf( stderr, "poll cycle: %s\n", eStatus == MB_ENOERR ? "okay" : "failed" );
                }
                else
                {
                    fprintf( stderr, "failed to connect to %s:%hu!\n", CLIENT_HOSTNAME, CLIENT_PORT );
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

        sleep( 1 );
    }
    while( iPolls-- > 0 );
    vMBPOtherDLLClose(  );
    return 0;
}
