/* 
 * MODBUS Master Library: ATxmega demo application
 * Copyright (c) 2014 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: main.c,v 1.1 2014-03-09 12:51:19 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <asf.h>
#include <util/delay.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"

#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBM_SERIAL_PORT			  	( 0 )
#define MBM_SERIAL_BAUDRATE		  	( 19200 )
#define MBM_PARITY				  	( MB_PAR_NONE )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
int
main( void )
{
	UBYTE ubCnt;
    eMBErrorCode    eStatus, eStatus2;
    xMBMHandle      xMBMMaster;
	    USHORT          usNRegs[5];
	    USHORT          usRegCnt = 0;
	    UBYTE           ubIdx;

    board_init(  );
    sysclk_init(  );
    pmic_init(  );
    cpu_irq_enable(  );
    if( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
    {
        do
        {
            for( ubCnt = 0; ubCnt < 100; ubCnt++ )
            {
                _delay_ms( 10 );
            }

            eStatus = MB_ENOERR;

            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
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
}
