/*
 * MODBUS Library: Luminary Cortex M3, FreeRTOS, Serial Driver
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: drvserial.c,v 1.4 2010-06-13 17:04:48 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_uart.h>
#include <driverlib/uart.h>
#include <driverlib/gpio.h>
#include "driverlib/interrupt.h"

/* ----------------------- MODBUS -------------------------------------------*/
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"

#include "drvserial.h"

/* ----------------------- Hardware mapping - Must come first ---------------*/

/* ----------------------- Defines ------------------------------------------*/
#define DRV_SERIAL_IS_VALID( ubPort, xHdls )	\
	( ( ubPort < DRV_SERIAL_MAX_INSTANCES ) && ( xHdls[ ubPort ].bIsInitialized  ) )

#define HDL_RESET( x )						do { \
	( x )->bIsInitialized = FALSE; \
	( x )->xSemHdl = NULL; \
	( x )->usEventMask = 0; \
} while( 0 )

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    BOOL            bIsInitialized;
    xSemaphoreHandle xSemHdl;
    USHORT          usEventMask;
} xDrvSerialHandle;

/* ----------------------- Static functions ---------------------------------*/
STATIC portBASE_TYPE xSerialIntHandler( UBYTE ubIdx );

#if DRV_SERIAL_MAX_INSTANCES >= 1
STATIC void     vSerialHdl0Handler( void );
#endif
#if DRV_SERIAL_MAX_INSTANCES >= 2
STATIC void     vSerialHdl1Handler( void );
#endif
#if DRV_SERIAL_MAX_INSTANCES >= 3
STATIC void     vSerialHdl2Handler( void );
#endif

/* ----------------------- Static variables ---------------------------------*/
STATIC xDrvSerialHandle xDrvSerialHdls[DRV_SERIAL_MAX_INSTANCES];

STATIC const struct
{
    unsigned long   ulUARTBase;
    unsigned long   ulIntNumber;
    unsigned long   ulRS485DriverEnablePortBase;
    unsigned long   ulDS485DriverEnablePortPin;
    unsigned long   ulRS485UARTPortBase;
    unsigned long   ulRS485UARTPortPins;
    void            ( *pvUARTIsr ) ( void );
} xDrvSerialHW[DRV_SERIAL_MAX_INSTANCES] =
{
#if defined( PART_LM3S818 )
    { UART0_BASE, INT_UART0, GPIO_PORTD_BASE, GPIO_PIN_5, GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1, vSerialHdl0Handler},
    { UART1_BASE, INT_UART1, 0, 0, GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3, vSerialHdl1Handler},    
#else  
    { UART2_BASE, INT_UART2, GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1, vSerialHdl0Handler},
    { UART0_BASE, INT_UART0, GPIO_PORTD_BASE, GPIO_PIN_5, GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1, vSerialHdl1Handler}
#endif    
};

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eDrvSerialInit( UBYTE ubPort, ULONG ulBaudRate, UBYTE ucDataBits, eMBSerialParity eParity, UBYTE ucStopBits )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    unsigned long   ulConfig = 0;
    MBP_ENTER_CRITICAL_SECTION(  );

    if( ( ubPort < DRV_SERIAL_MAX_INSTANCES ) && ( !xDrvSerialHdls[ubPort].bIsInitialized ) )
    {
        HDL_RESET( &xDrvSerialHdls[ubPort] );
        xDrvSerialHdls[ubPort].bIsInitialized = TRUE;

        eStatus = MB_ENOERR;
        switch ( ucDataBits )
        {
        case 8:
            ulConfig |= UART_CONFIG_WLEN_8;
            break;
        case 7:
            ulConfig |= UART_CONFIG_WLEN_7;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }
        switch ( ucStopBits )
        {
        case 2:
            ulConfig |= UART_CONFIG_STOP_TWO;
            break;
        case 1:
            ulConfig |= UART_CONFIG_STOP_ONE;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }
        switch ( eParity )
        {
        case MB_PAR_EVEN:
            ulConfig |= UART_CONFIG_PAR_EVEN;
            break;
        case MB_PAR_ODD:
            ulConfig |= UART_CONFIG_PAR_ODD;
            break;
        case MB_PAR_NONE:
            ulConfig |= UART_CONFIG_PAR_NONE;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }
        if( MB_EINVAL != eStatus )
        {
            vSemaphoreCreateBinary( xDrvSerialHdls[ubPort].xSemHdl );
            if( NULL == xDrvSerialHdls[ubPort].xSemHdl )
            {
                eStatus = MB_ENORES;
            }
            else
            {
                vPortEnterCritical(  );

                /* Configure GPIO Pins for UART */
                GPIOPinTypeUART( xDrvSerialHW[ubPort].ulRS485UARTPortBase, xDrvSerialHW[ubPort].ulRS485UARTPortPins );
                if( 0 != xDrvSerialHW[ubPort].ulRS485DriverEnablePortBase )
                {
                    GPIOPinTypeGPIOOutput( xDrvSerialHW[ubPort].ulRS485DriverEnablePortBase,
                                           xDrvSerialHW[ubPort].ulDS485DriverEnablePortPin );
                    GPIOPinWrite( xDrvSerialHW[ubPort].ulRS485DriverEnablePortBase,
                                  xDrvSerialHW[ubPort].ulDS485DriverEnablePortPin, 0 );                    
                }
                
                /* Reconfigure everything of the UART peripheral because some other
                 * task could have used it.
                 */
                UARTConfigSetExpClk( xDrvSerialHW[ubPort].ulUARTBase, SysCtlClockGet(  ), ulBaudRate, ulConfig );
                UARTFlowControlSet( xDrvSerialHW[ubPort].ulUARTBase, UART_FLOWCONTROL_NONE );
                UARTFIFOLevelSet( xDrvSerialHW[ubPort].ulUARTBase, UART_FIFO_TX4_8, UART_FIFO_RX4_8 );
                UARTFIFOEnable( xDrvSerialHW[ubPort].ulUARTBase );
                UARTDisableSIR( xDrvSerialHW[ubPort].ulUARTBase );
                UARTDMADisable( xDrvSerialHW[ubPort].ulUARTBase, UART_DMA_ERR_RXSTOP | UART_DMA_TX | UART_DMA_RX );
                UARTRxErrorClear( xDrvSerialHW[ubPort].ulUARTBase );
                UARTIntDisable( xDrvSerialHW[ubPort].ulUARTBase,
                                UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | UART_INT_RT | UART_INT_TX |
                                UART_INT_RX | UART_INT_DSR | UART_INT_DCD | UART_INT_CTS | UART_INT_RI );
                UARTIntClear( xDrvSerialHW[ubPort].ulUARTBase,
                              UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | UART_INT_RT | UART_INT_TX |
                              UART_INT_RX | UART_INT_DSR | UART_INT_DCD | UART_INT_CTS | UART_INT_RI );
                IntPrioritySet( xDrvSerialHW[ubPort].ulIntNumber, configKERNEL_INTERRUPT_PRIORITY );
                UARTIntRegister( xDrvSerialHW[ubPort].ulUARTBase, xDrvSerialHW[ubPort].pvUARTIsr );
                HWREG( xDrvSerialHW[ubPort].ulUARTBase + UART_O_CTL ) &= ~UART_CTL_RXE;
                HWREG( xDrvSerialHW[ubPort].ulUARTBase + UART_O_CTL ) &= ~UART_CTL_TXE;
                IntEnable( xDrvSerialHW[ubPort].ulIntNumber );

                vPortExitCritical(  );
                eStatus = MB_ENOERR;
            }
        }

        if( MB_ENOERR != eStatus )
        {
            if( NULL != xDrvSerialHdls[ubPort].xSemHdl )
            {
                vQueueDelete( xDrvSerialHdls[ubPort].xSemHdl );
            }
            HDL_RESET( &xDrvSerialHdls[ubPort] );
        }
    }

    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eDrvSerialClose( UBYTE ubPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    MBP_ENTER_CRITICAL_SECTION(  );
    if( DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        vPortEnterCritical(  );
        UARTDisable( xDrvSerialHW[ubPort].ulUARTBase );
        UARTIntDisable( xDrvSerialHW[ubPort].ulUARTBase,
                        UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | UART_INT_RT | UART_INT_TX | UART_INT_RX
                        | UART_INT_DSR | UART_INT_DCD | UART_INT_CTS | UART_INT_RI );
        UARTIntClear( xDrvSerialHW[ubPort].ulUARTBase,
                      UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE | UART_INT_RT | UART_INT_TX | UART_INT_RX |
                      UART_INT_DSR | UART_INT_DCD | UART_INT_CTS | UART_INT_RI );
        UARTIntUnregister( xDrvSerialHW[ubPort].ulUARTBase );
        vPortExitCritical(  );
        vQueueDelete( xDrvSerialHdls[ubPort].xSemHdl );
        HDL_RESET( &xDrvSerialHdls[ubPort] );
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eDrvSerialWaitEvent( UBYTE ubPort, USHORT * pusEvents, USHORT usTimeOut )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    if( DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        if( pdTRUE == xSemaphoreTake( xDrvSerialHdls[ubPort].xSemHdl, usTimeOut / portTICK_RATE_MS ) )
        {
            vPortEnterCritical(  );
            *pusEvents = xDrvSerialHdls[ubPort].usEventMask;
            if( ( ( HWREG( xDrvSerialHW[ubPort].ulUARTBase + UART_O_CTL ) & UART_CTL_RXE ) > 0 ) &&
                UARTCharsAvail( xDrvSerialHW[ubPort].ulUARTBase ) )
            {
                *pusEvents |= DRV_SERIAL_EVENT_RXRDY;
            }
            /* Note: Transmitter is always ready since transmit function is blocking.
             */
            if( ( HWREG( xDrvSerialHW[ubPort].ulUARTBase + UART_O_CTL ) & UART_CTL_TXE ) > 0 )
            {
                *pusEvents |= DRV_SERIAL_EVENT_TXRDY;
            }
            xDrvSerialHdls[ubPort].usEventMask = 0;
            vPortExitCritical(  );
        }
        else
        {
            *pusEvents = 0;
        }
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eDrvSerialAbort( UBYTE ubPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    if( DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        vPortEnterCritical(  );
        xDrvSerialHdls[ubPort].usEventMask |= DRV_SERIAL_EVENT_ABORT;
        vPortExitCritical(  );
        ( void )xSemaphoreGive( xDrvSerialHdls[ubPort].xSemHdl );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eDrvSerialTransmitEnable( UBYTE ubPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    if( DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        HWREG( xDrvSerialHW[ubPort].ulUARTBase + UART_O_CTL ) |= UART_CTL_TXE;
        if( 0 != xDrvSerialHW[ubPort].ulRS485DriverEnablePortBase )
        {
            GPIOPinWrite( xDrvSerialHW[ubPort].ulRS485DriverEnablePortBase, xDrvSerialHW[ubPort].ulDS485DriverEnablePortPin, 0xFF );
        }            
        /* Note: Transmitter is always ready since transmit function is blocking.
         * Signal waiting task.
         */
        ( void )xSemaphoreGive( xDrvSerialHdls[ubPort].xSemHdl );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eDrvSerialTransmitDisable( UBYTE ubPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    if( DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        /* Small delay loop to disable RS485 driver line. This is a limitation
         * of some Luminary Cortex M3 UARTs that we can not detect the end
         * of the transmission.
         */
        while( HWREG( xDrvSerialHW[ubPort].ulUARTBase + UART_O_FR ) & UART_FR_BUSY )
        {
            vTaskDelay( 1 );
        }

        vPortEnterCritical(  );
        if( 0 != xDrvSerialHW[ubPort].ulRS485DriverEnablePortBase )
        {
            GPIOPinWrite( xDrvSerialHW[ubPort].ulRS485DriverEnablePortBase, xDrvSerialHW[ubPort].ulDS485DriverEnablePortPin, 0x00 );
        }            
        HWREG( xDrvSerialHW[ubPort].ulUARTBase + UART_O_CTL ) &= ~UART_CTL_TXE;
        vPortExitCritical(  );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eDrvSerialTransmitFree( UBYTE ubPort, USHORT * pusNFreeBytes )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    if( ( NULL != pusNFreeBytes ) && DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        /* We can transmit an infinite number of bytes */
        *pusNFreeBytes = 0xFFFFU;
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eDrvSerialTransmit( UBYTE ubPort, const UBYTE * pubBuffer, USHORT usLength )
{
    USHORT          usIdx;
    eMBErrorCode    eStatus = MB_EINVAL;
    if( ( NULL != pubBuffer ) && DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        UARTIntEnable( xDrvSerialHW[ubPort].ulUARTBase, UART_INT_TX );
        for( usIdx = 0; usIdx < usLength; )
        {
            if( !UARTCharPutNonBlocking( xDrvSerialHW[ubPort].ulUARTBase, pubBuffer[usIdx] ) )
            {
                ( void )xSemaphoreTake( xDrvSerialHdls[ubPort].xSemHdl, 0 );
            }
            else
            {
                usIdx++;
            }
        }
        UARTIntDisable( xDrvSerialHW[ubPort].ulUARTBase, UART_INT_TX );
        UARTIntClear( xDrvSerialHW[ubPort].ulUARTBase, UART_INT_TX );

        /* We can transmit more data - Signal immediately */
        ( void )xSemaphoreGive( xDrvSerialHdls[ubPort].xSemHdl );

        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eDrvSerialReceiveEnable( UBYTE ubPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    if( DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        HWREG( xDrvSerialHW[ubPort].ulUARTBase + UART_O_CTL ) |= UART_CTL_RXE;
        UARTIntEnable( xDrvSerialHW[ubPort].ulUARTBase, UART_INT_RX | UART_INT_RT );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eDrvSerialReceiveDisable( UBYTE ubPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    if( DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        vPortEnterCritical(  );
        UARTIntDisable( xDrvSerialHW[ubPort].ulUARTBase, UART_INT_RX | UART_INT_RT );
        UARTIntClear( xDrvSerialHW[ubPort].ulUARTBase, UART_INT_RX | UART_INT_RT );
        HWREG( xDrvSerialHW[ubPort].ulUARTBase + UART_O_CTL ) &= ~UART_CTL_RXE;
        vPortExitCritical(  );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eDrvSerialReceive( UBYTE ubPort, UBYTE * pubBuffer, USHORT usLengthMax, USHORT * pusNBytesReceived )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    if( ( NULL != pubBuffer ) && ( NULL != pusNBytesReceived ) && DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        for( *pusNBytesReceived = 0;
             ( *pusNBytesReceived < usLengthMax ) && UARTCharsAvail( xDrvSerialHW[ubPort].ulUARTBase );
             ( *pusNBytesReceived )++ )
        {
            pubBuffer[*pusNBytesReceived] = ( UBYTE ) UARTCharGet( xDrvSerialHW[ubPort].ulUARTBase );
        }
        if( UARTCharsAvail( xDrvSerialHW[ubPort].ulUARTBase ) )
        {
            ( void )xSemaphoreGive( xDrvSerialHdls[ubPort].xSemHdl );
        }
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eDrvSerialReceiveReset( UBYTE ubPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    if( DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) )
    {
        while( UARTCharsAvail( xDrvSerialHW[ubPort].ulUARTBase ) )
        {
            ( void )UARTCharGet( xDrvSerialHW[ubPort].ulUARTBase );
        }
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

STATIC portBASE_TYPE
xSerialIntHandler( UBYTE ubIdx )
{
    signed portBASE_TYPE xHigherPriorityTaskWoken;

    unsigned long   ulStatus;
    ulStatus = UARTIntStatus( xDrvSerialHW[ubIdx].ulUARTBase, true );
    UARTIntClear( xDrvSerialHW[ubIdx].ulUARTBase, ulStatus );

    if( ( ( UART_INT_RT | UART_INT_RX ) & ulStatus ) > 0 )
    {
        xSemaphoreGiveFromISR( xDrvSerialHdls[ubIdx].xSemHdl, &xHigherPriorityTaskWoken );
    }
    if( ( UART_INT_TX & ulStatus ) > 0 )
    {
        xSemaphoreGiveFromISR( xDrvSerialHdls[ubIdx].xSemHdl, &xHigherPriorityTaskWoken );
    }
    return xHigherPriorityTaskWoken;
}

#if DRV_SERIAL_MAX_INSTANCES >= 1
STATIC void
vSerialHdl0Handler( void )
{
    portBASE_TYPE   xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = xSerialIntHandler( 0 );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
#endif

#if DRV_SERIAL_MAX_INSTANCES >= 2
STATIC void
vSerialHdl1Handler( void )
{
    portBASE_TYPE   xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = xSerialIntHandler( 1 );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
#endif

#if DRV_SERIAL_MAX_INSTANCES >= 3
STATIC void
vSerialHdl2Handler( void )
{
    portBASE_TYPE   xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = xSerialIntHandler( 2 );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
#endif
