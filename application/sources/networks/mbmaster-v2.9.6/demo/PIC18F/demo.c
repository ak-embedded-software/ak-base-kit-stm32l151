/* 
 * MODBUS Library: PIC18F port 
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo.c,v 1.2 2008-02-26 22:03:34 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
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

#if defined( __18CXX )
void main( void )
#else
int
main( int argc, char *argv[] )
#endif
{

    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[5];
    USHORT          usRegCnt = 0;
    UBYTE           ubIdx;
    UBYTE           ubCnt;

    if( MB_ENOERR ==
        ( eStatus = eMBMSerialInit( &xMBMMaster, MB_ASCII, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
    {
        do
        {

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
            switch ( eStatus )
            {
            case MB_ENOERR:
                vIOSetLED( DEBUG_LED_WORKING, TRUE );
                vIOSetLED( DEBUG_LED_ERROR, FALSE );
                break;

            default:
                vIOSetLED( DEBUG_LED_ERROR, TRUE );
                vIOSetLED( DEBUG_LED_WORKING, FALSE );
                break;
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

#if !defined( __18CXX )
    return 0;
#endif
}


STATIC void
vIOSetLED( UBYTE ubIdx, BOOL bTurnOn )
{
    STATIC BOOL     bIsInitalized = FALSE;

    if( !bIsInitalized )
    {
        bIsInitalized = TRUE;
    }
    switch ( ubIdx )
    {
    case DEBUG_LED_ERROR:
        if( bTurnOn )
        {
        }
        else
        {
        }
        break;

    case DEBUG_LED_WORKING:
        if( bTurnOn )
        {
        }
        else
        {
        }
        break;

    default:
        break;
    }
}
