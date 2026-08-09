/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-masterslave.c,v 1.1 2008-12-06 18:50:02 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "mbs.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/



#define MBM_SERIAL_PORT			  ( 0 )
#define MBM_SERIAL_BAUDRATE		  ( 19200 )
#define MBM_PARITY				  ( MB_PAR_NONE )

#define DEBUG_LED_ERROR           ( 0 )
#define DEBUG_LED_WORKING         ( 1 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vIOSetLED( UBYTE ubIdx, BOOL bTurnOn );

/* ----------------------- Start implementation -----------------------------*/

int
main( int argc, char *argv[] )
{

    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster, xMBSSlave;
	eMBMQueryState	eQueryState ;
    USHORT          usNRegs[10];    

    sei(  );
    if( MB_ENOERR != ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
    {
		MBP_ASSERT( 0 );
	}
	else if( MB_ENOERR != ( eStatus = eMBSSerialInit( &xMBSSlave, MB_RTU, 1, MBM_SERIAL_PORT + 1, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
	{
		MBP_ASSERT( 0 );
	}
	else
	{
		/* This is a code fragment where both the master and the slave stack
		 * are active at the same time. This code works fine on a system
		 * without or with an RTOS. Please note that with an RTOS and
		 * threads you should better create a seperate thread for the
		 * slave stack and let it run there. This will make the code a
		 * lot easier.
		 */
        do
        {
			/* Please note that we need to poll the slave while we are
			 * handling the master because otherwise there will be 
			 * timeouts from devices accessing the slave.
			 */
			eQueryState = MBM_STATE_NONE;

			do
			{
				eStatus2 = eMBSPoll( xMBSSlave );
				vMBMReadHoldingRegistersPolled( xMBMMaster, 1, 0, 10, usNRegs, &eQueryState, &eStatus );
			} while( MBM_STATE_DONE != eQueryState );
			
			/* Now we can access the eStatus variable from the stack and
			 * check if the request was successfull.
			 */
			if( MB_ENOERR != eStatus )
			{
				/* Do something useful here. */
				_delay_ms( 10 );
			}

			/* Check for any slave errors. */
			if( MB_ENOERR != eStatus2 )
			{
				/* Do something useful here. */
				_delay_ms( 10 );
			}
        }
        while( TRUE );
    }


    return 0;
}

