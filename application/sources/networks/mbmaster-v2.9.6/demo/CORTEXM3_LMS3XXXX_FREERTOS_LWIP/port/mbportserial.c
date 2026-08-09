/*
 * MODBUS Library: Luminary Cortex M3, FreeRTOS
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial.c,v 1.4 2010-06-13 17:04:48 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"


/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"
#include "drvserial.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBP_SERIAL_TASK_PRIORITY        	( MBP_TASK_PRIORITY )
#define MBP_SERIAL_TASK_STACKSIZE           ( 128 )
#define MBP_SERIAL_BUFFER_SIZE	            ( 16 )

#define IDX_INVALID                         ( 255 )

#ifndef MBP_SERIAL_DEBUG
#define MBP_SERIAL_DEBUG                    ( 0 )
#endif

#if defined( MBP_SERIAL_DEBUG ) && ( MBP_SERIAL_DEBUG == 1 )
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/gpio.h>

#if defined( PART_LM3S6965 ) 
#define DEBUG_INIT                          do { \
    GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 ); \
    GPIOPinTypeGPIOOutput( GPIO_PORTD_BASE, GPIO_PIN_1 ); \
} while( 0 )

#define DEBUG_RXENABLE                      do { \
    GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0xFF ); \
} while( 0 )

#define DEBUG_RXDISABLE                     do { \
    GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_1, 0x00 ); \
} while( 0 )
#else
#warning "No debug definitions for this part. Consider adding them to mbportserial.c"
#define DEBUG_INIT
#define DEBUG_RXENABLE 
#define DEBUG_RXDISABLE
#endif
#else
#define DEBUG_INIT
#define DEBUG_RXENABLE 
#define DEBUG_RXDISABLE
#endif


#define HDL_RESET( x )						do { \
	( x )->ubIdx = IDX_INVALID; \
	( x )->xMBHdl = FALSE; \
	( x )->bIsRunning = FALSE; \
	( x )->bIsBroken = FALSE; \
	( x )->pbMBPTransmitterEmptyFN = NULL; \
	( x )->pvMBPReceiveFN = NULL; \
} while( 0 )

/* ----------------------- Type definitions ---------------------------------*/

typedef struct
{
    UBYTE           ubIdx;
    xMBHandle       xMBHdl;
    BOOL            bIsRunning;
    BOOL            bIsBroken;
    pbMBPSerialTransmitterEmptyAPIV2CB pbMBPTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV2CB pvMBPReceiveFN;
} xSerialHandle;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vMBPSerialHandlerTask( void *pvArg );

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitalized = FALSE;
STATIC xSerialHandle xSerialHdls[DRV_SERIAL_MAX_INSTANCES];

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        DEBUG_INIT;
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            HDL_RESET( &xSerialHdls[ubIdx] );
        }
        bIsInitalized = TRUE;
    }

    if( ( ucPort < DRV_SERIAL_MAX_INSTANCES ) && ( IDX_INVALID == xSerialHdls[ucPort].ubIdx ) )
    {
        HDL_RESET( &xSerialHdls[ucPort] );
        xSerialHdls[ucPort].xMBHdl = xMBMHdl;
        xSerialHdls[ucPort].ubIdx = ucPort;
        xSerialHdls[ucPort].bIsRunning = TRUE;

        if( MB_ENOERR != eDrvSerialInit( ucPort, ulBaudRate, ucDataBits, eParity, ucStopBits ) )
        {
            eStatus = MB_EPORTERR;
        }
        else if( pdPASS !=
                 xTaskCreate( vMBPSerialHandlerTask, "MBP-SER", MBP_SERIAL_TASK_STACKSIZE, &xSerialHdls[ucPort],
                              MBP_SERIAL_TASK_PRIORITY, NULL ) )
        {
            eStatus = MB_EPORTERR;
        }
        else
        {
            *pxSerialHdl = &xSerialHdls[ucPort];
            eStatus = MB_ENOERR;
        }

        if( MB_ENOERR != eStatus )
        {
            HDL_RESET( &xSerialHdls[ucPort] );
        }
    }
    else
    {
        eStatus = MB_ENORES;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialClose( xMBPSerialHandle xSerialHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( ( NULL != pxSerialIntHdl ) && MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        pxSerialIntHdl->bIsRunning = FALSE;
        ( void )eDrvSerialAbort( pxSerialIntHdl->ubIdx );
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialTxEnable( xMBPSerialHandle xSerialHdl, pbMBPSerialTransmitterEmptyCB pbMBPTransmitterEmptyFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        if( pxSerialIntHdl->bIsBroken )
        {
            eStatus = MB_EIO;
        }
        else
        {
            if( NULL != pbMBPTransmitterEmptyFN )
            {
                pxSerialIntHdl->pbMBPTransmitterEmptyFN = pbMBPTransmitterEmptyFN;
                eStatus = eDrvSerialTransmitEnable( pxSerialIntHdl->ubIdx );
            }
            else
            {
                eStatus = eDrvSerialTransmitDisable( pxSerialIntHdl->ubIdx );
                pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
            }
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

USHORT
usMPSerialTimeout( ULONG ulBaudRate )
{
    USHORT          usTimeoutMS;
    /* Timeout is size of FIFO (assume maximum fill) multiplied by
     * 11 bit times divided by baudrate in milliseconds.
     */
    usTimeoutMS = MB_INTDIV_CEIL( 1000UL * 11UL * 16UL, ulBaudRate );
    if( usTimeoutMS <= 2 )
    {
        usTimeoutMS = 2;
    }
    return usTimeoutMS;
}

eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBPReceiveFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        if( pxSerialIntHdl->bIsBroken )
        {
            eStatus = MB_EIO;
        }
        else
        {
            if( NULL != pvMBPReceiveFN )
            {
                ( void )eDrvSerialReceiveReset( pxSerialIntHdl->ubIdx );
                pxSerialIntHdl->pvMBPReceiveFN = pvMBPReceiveFN;
                eStatus = eDrvSerialReceiveEnable( pxSerialIntHdl->ubIdx );
                DEBUG_RXENABLE;                
            }
            else
            {             
                eStatus = eDrvSerialReceiveDisable( pxSerialIntHdl->ubIdx );
                pxSerialIntHdl->pvMBPReceiveFN = NULL;
                DEBUG_RXDISABLE;                
            }
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

STATIC void
vMBPSerialHandlerTask( void *pvArg )
{
    xSerialHandle  *pxSerialIntHdl = pvArg;
    UBYTE           arubBuffer[16];
    USHORT          usCnt;
    USHORT          usByteMax;
    BOOL            bIsRunning;
    USHORT          usEvents;
    eMBErrorCode    eStatus;
    do
    {
        eStatus = eDrvSerialWaitEvent( pxSerialIntHdl->ubIdx, &usEvents, 5000 );
        MBP_ASSERT( MB_ENOERR == eStatus );
        if( ( usEvents & DRV_SERIAL_EVENT_RXRDY ) > 0 )
        {
            eStatus = eDrvSerialReceive( pxSerialIntHdl->ubIdx, arubBuffer, MB_UTILS_NARRSIZE( arubBuffer ), &usCnt );
            MBP_ENTER_CRITICAL_SECTION(  );
            if( MB_ENOERR == eStatus )
            {
                if( NULL != pxSerialIntHdl->pvMBPReceiveFN )
                {
                    pxSerialIntHdl->pvMBPReceiveFN( pxSerialIntHdl->xMBHdl, arubBuffer, usCnt );
                }
            }
            else
            {
                pxSerialIntHdl->bIsBroken = TRUE;
            }
            MBP_EXIT_CRITICAL_SECTION(  );
        }
        if( ( usEvents & DRV_SERIAL_EVENT_TXRDY ) > 0 )
        {
            eStatus = eDrvSerialTransmitFree( pxSerialIntHdl->ubIdx, &usByteMax );
            if( MB_ENOERR == eStatus )
            {
                MBP_ENTER_CRITICAL_SECTION(  );
                if( ( usByteMax > 0 ) && ( NULL != pxSerialIntHdl->pbMBPTransmitterEmptyFN ) )
                {
                    /* Make sure that we do not overrun our buffer. */
                    usByteMax =
                        usByteMax > MB_UTILS_NARRSIZE( arubBuffer ) ? MB_UTILS_NARRSIZE( arubBuffer ) : usByteMax;
                    /* Fetch the data from the stack and transmit it. */
                    if( !pxSerialIntHdl->
                        pbMBPTransmitterEmptyFN( pxSerialIntHdl->xMBHdl, arubBuffer, usByteMax, &usCnt ) )
                    {
                        pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
                        MBP_EXIT_CRITICAL_SECTION(  );
                        if( MB_ENOERR != eDrvSerialTransmitDisable( pxSerialIntHdl->ubIdx ) )
                        {
                            MBP_ENTER_CRITICAL_SECTION(  );
                            pxSerialIntHdl->bIsBroken = TRUE;
                            MBP_EXIT_CRITICAL_SECTION(  );
                        }
                    }
                    else
                    {
                        MBP_EXIT_CRITICAL_SECTION(  );
                        if( MB_ENOERR != eDrvSerialTransmit( pxSerialIntHdl->ubIdx, arubBuffer, usCnt ) )
                        {
                            MBP_ENTER_CRITICAL_SECTION(  );
                            pxSerialIntHdl->bIsBroken = TRUE;
                            MBP_EXIT_CRITICAL_SECTION(  );
                        }
                    }
                }
                else
                {
                    MBP_EXIT_CRITICAL_SECTION(  );
                }
            }
            else
            {
                pxSerialIntHdl->bIsBroken = TRUE;
            }

        }
        MBP_ENTER_CRITICAL_SECTION(  );
        bIsRunning = pxSerialIntHdl->bIsRunning;
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    while( bIsRunning );

    eDrvSerialClose( pxSerialIntHdl->ubIdx );
    HDL_RESET( pxSerialIntHdl );
    vTaskDelete( NULL );
}
