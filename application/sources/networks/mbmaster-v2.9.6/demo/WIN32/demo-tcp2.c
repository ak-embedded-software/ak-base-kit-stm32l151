/* 
 * MODBUS Library: WIN32
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-tcp2.c,v 1.1 2009-12-19 11:53:14 embedded-so.embedded-solutions.1 Exp $
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
#define CLIENT_HOSTNAME             "192.168.3.2"
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
    int             iPolls = 100;

    vMBPOtherDLLInit(  );

    if( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMMaster ) ) )
    {
		eStatus = MB_EIO;
		while( iPolls-- > 0 )
		{				
			if( MB_ENOERR != eStatus )
			{
				/* First disconnect - Function is safe even when not connected. */
				( void )eMBMTCPDisconnect( xMBMMaster );
				eStatus = eMBMTCPConnect( xMBMMaster, CLIENT_HOSTNAME, CLIENT_PORT );
			}
			if( MB_ENOERR == eStatus )
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
			_ftprintf( stderr, _T( "poll cycle: %s\n" ), eStatus == MB_ENOERR ? _T( "okay" ) : _T( "failed" ) );
		}
    }
	else
	{
		_ftprintf( stderr, _T( "TCP init failed with error = %d\n" ), eStatus );
	}
    if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
    {
        MBP_ASSERT( 0 );
    }

    vMBPOtherDLLClose(  );
    return 0;
}
