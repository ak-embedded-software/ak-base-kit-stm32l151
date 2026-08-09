/*
 * MODBUS Library: NXP Cortex M3, FreeRTOS and lwIP Example
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: drvserial.c,v 1.1 2011-01-02 16:16:22 embedded-solutions.cwalter Exp $
 */
/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"
#include "LPC17xx.h"

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
	( x )->bRxEnabled = FALSE; \
	( x )->bTxEnabled = FALSE; \
	( x )->usEventMask = 0; \
} while( 0 )

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    BOOL            bIsInitialized;
    xSemaphoreHandle xSemHdl;
    BOOL            bRxEnabled;
    BOOL            bTxEnabled;
    USHORT          usEventMask;
} xDrvSerialHandle;

/* ----------------------- Static functions ---------------------------------*/
STATIC ULONG       ulprvDrvSerialGetUARTClock( UBYTE ubPort );
STATIC INLINE BOOL bprvDrvSerialGetUARTHasRxData( UBYTE ubPort );
STATIC INLINE BOOL bprvDrvSerialGetUARTTHRIsEmpty( UBYTE ubPort );
STATIC INLINE UBYTE ubprvDrvSerialGetUARTTXFifoLevel( UBYTE ubPort );
STATIC INLINE void vprvDrvSerialUARTRXIntEnable( UBYTE ubPort, BOOL bOn );
STATIC INLINE void vprvDrvSerialUARTTXIntEnable( UBYTE ubPort, BOOL bOn );
STATIC portBASE_TYPE xSerialIntHandler( UBYTE ubIdx );

/* ----------------------- Static variables ---------------------------------*/
STATIC xDrvSerialHandle xDrvSerialHdls[DRV_SERIAL_MAX_INSTANCES];

STATIC const struct
{
    enum eUartType
    { UART_TYPE, UART1_TYPE } eType;
    union
    {
        __IO LPC_UART_TypeDef *pxUARTBase;
        __IO LPC_UART1_TypeDef *pxUART1Base;
    } x;
    __IO uint32_t  *puiPINSEL;      /* Configuration of UART RX/TX Pins */
    ULONG           ulPINSELMask;   /* Used as mask for UART RX/TX Pin configuration */
    ULONG           ulPINSELSet;    /* Used to configure RX/TX Pins */
    ULONG           ulPCON;         /* Bit in peripheral power control register to enable UART */
    ULONG           ulPCLKDIVOff;   /* Bit in peripheral clock control register to determine clock divider */
    IRQn_Type       xIRQNo;         /* Interrupt number */
    LPC_GPIO_TypeDef *pxGPIO;       /* Set != NULL if software DE pin control required */
    ULONG           ulDEPinMask;        /* Index of DE pin in FIOSET/FIODIR registers */
} xDrvSerialHW[DRV_SERIAL_MAX_INSTANCES] =
{
/* *INDENT-OFF* */
    { UART_TYPE,.x.pxUARTBase = ( LPC_UART_TypeDef * ) LPC_UART0, &( LPC_PINCON->PINSEL0 ), 0x000000F0UL, 0x00000050UL, 1 << 3, 6, UART0_IRQn, LPC_GPIO2, 1UL << 5UL},
    { UART1_TYPE,.x.pxUART1Base = LPC_UART1, &( LPC_PINCON->PINSEL4 ), 0x0000000FUL, 0x0000000AUL, 1 << 4, 8, UART1_IRQn, LPC_GPIO2, 1UL << 6UL}
/* *INDENT-ON* */
};

/* ----------------------- Start implementation -----------------------------*/


eMBErrorCode
eDrvSerialInit( UBYTE ubPort, ULONG ulBaudRate, UBYTE ucDataBits, eMBSerialParity eParity, UBYTE ucStopBits )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    ULONG           ulLCR = 0;
    ULONG           ulBaudDiv;

    MBP_ENTER_CRITICAL_SECTION(  );

    if( ( ubPort < DRV_SERIAL_MAX_INSTANCES ) && ( !xDrvSerialHdls[ubPort].bIsInitialized ) )
    {
        HDL_RESET( &xDrvSerialHdls[ubPort] );
        xDrvSerialHdls[ubPort].bIsInitialized = TRUE;

        eStatus = MB_ENOERR;
        switch ( ucDataBits )
        {
        case 8:
            ulLCR |= 3UL << 0UL;
            break;
        case 7:
            ulLCR |= 2UL << 0UL;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }
        switch ( ucStopBits )
        {
        case 2:
            ulLCR |= 1UL << 2UL;
            break;
        case 1:
            ulLCR |= 0UL << 2UL;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }
        switch ( eParity )
        {
        case MB_PAR_EVEN:
            ulLCR |= ( 1UL << 4UL ) | ( 1UL << 3UL );
            break;
        case MB_PAR_ODD:
            ulLCR |= ( 0UL << 4UL ) | ( 1UL << 3UL );
            break;
        case MB_PAR_NONE:
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
                if( NULL != xDrvSerialHW[ubPort].pxGPIO )
                {
                    xDrvSerialHW[ubPort].pxGPIO->FIOCLR = xDrvSerialHW[ubPort].ulDEPinMask;
                    xDrvSerialHW[ubPort].pxGPIO->FIODIR = xDrvSerialHW[ubPort].ulDEPinMask;
                }
                LPC_SC->PCONP |= xDrvSerialHW[ubPort].ulPCON;

                *( xDrvSerialHW[ubPort].puiPINSEL ) &= ~xDrvSerialHW[ubPort].ulPINSELMask;
                *( xDrvSerialHW[ubPort].puiPINSEL ) |= xDrvSerialHW[ubPort].ulPINSELSet;
                ulBaudDiv = ulprvDrvSerialGetUARTClock( ubPort ) / ( 16UL * ulBaudRate );
                switch ( xDrvSerialHW[ubPort].eType )
                {
                case UART_TYPE:
                    xDrvSerialHW[ubPort].x.pxUARTBase->LCR = ulLCR | ( 1UL << 7UL );
                    xDrvSerialHW[ubPort].x.pxUARTBase->DLM = ulBaudDiv / 256UL;
                    xDrvSerialHW[ubPort].x.pxUARTBase->DLL = ulBaudDiv % 256UL;
                    xDrvSerialHW[ubPort].x.pxUARTBase->LCR = ulLCR;
                    xDrvSerialHW[ubPort].x.pxUARTBase->FCR = 0x00000007UL | ( 2UL << 6UL );
                    xDrvSerialHW[ubPort].x.pxUARTBase->IER = 0;
                    ( void )xDrvSerialHW[ubPort].x.pxUARTBase->IIR;
                    break;
                case UART1_TYPE:
                    xDrvSerialHW[ubPort].x.pxUART1Base->LCR = ulLCR | ( 1UL << 7UL );
                    xDrvSerialHW[ubPort].x.pxUART1Base->DLM = ulBaudDiv / 256UL;
                    xDrvSerialHW[ubPort].x.pxUART1Base->DLL = ulBaudDiv % 256UL;
                    xDrvSerialHW[ubPort].x.pxUART1Base->LCR = ulLCR;
                    xDrvSerialHW[ubPort].x.pxUART1Base->FCR = 0x00000007UL | ( 2UL << 6UL );
                    xDrvSerialHW[ubPort].x.pxUARTBase->IER = 0;
                    ( void )xDrvSerialHW[ubPort].x.pxUARTBase->IIR;
                    break;
                default:
                    MBP_ASSERT( 0 );
                    break;
                }
                NVIC_SetPriority( xDrvSerialHW[ubPort].xIRQNo, configUART_INTERRUPT_PRIORITY );
                NVIC_EnableIRQ( xDrvSerialHW[ubPort].xIRQNo );
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
        switch ( xDrvSerialHW[ubPort].eType )
        {
        case UART_TYPE:
            xDrvSerialHW[ubPort].x.pxUARTBase->IER = 0x00000000UL;
            xDrvSerialHW[ubPort].x.pxUARTBase->FCR = 0x00000000UL;
            xDrvSerialHW[ubPort].x.pxUARTBase->LCR = 0x00000000UL;
            break;
        case UART1_TYPE:
            xDrvSerialHW[ubPort].x.pxUART1Base->IER = 0x00000000UL;
            xDrvSerialHW[ubPort].x.pxUART1Base->FCR = 0x00000000UL;
            xDrvSerialHW[ubPort].x.pxUART1Base->LCR = 0x00000000UL;
            break;
        default:
            MBP_ASSERT( 0 );
            break;
        }
        LPC_SC->PCONP &= ~xDrvSerialHW[ubPort].ulPCON;
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
            if( xDrvSerialHdls[ubPort].bRxEnabled && bprvDrvSerialGetUARTHasRxData( ubPort ) )
            {
                *pusEvents |= DRV_SERIAL_EVENT_RXRDY;
            }
            if( xDrvSerialHdls[ubPort].bTxEnabled && bprvDrvSerialGetUARTTHRIsEmpty( ubPort ) )
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
        /* Note: Transmitter is always ready since transmit function is blocking.
         * Signal waiting task.
         */
        xDrvSerialHdls[ubPort].bTxEnabled = TRUE;
        ( void )xSemaphoreGive( xDrvSerialHdls[ubPort].xSemHdl );
        if( NULL != xDrvSerialHW[ubPort].pxGPIO )
        {
            xDrvSerialHW[ubPort].pxGPIO->FIOSET = xDrvSerialHW[ubPort].ulDEPinMask;
        }
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
        switch ( xDrvSerialHW[ubPort].eType )
        {
        case UART_TYPE:
            while( !( xDrvSerialHW[ubPort].x.pxUARTBase->LSR && ( 1UL << 6UL ) ) )
            {
                vTaskDelay( 1 );
            }
            break;
        case UART1_TYPE:
            while( !( xDrvSerialHW[ubPort].x.pxUART1Base->LSR && ( 1UL << 6UL ) ) )
            {
                vTaskDelay( 1 );
            }
            break;
        default:
            MBP_ASSERT( 0 );
            break;
        }
        if( NULL != xDrvSerialHW[ubPort].pxGPIO )
        {
            xDrvSerialHW[ubPort].pxGPIO->FIOCLR = xDrvSerialHW[ubPort].ulDEPinMask;
        }
        xDrvSerialHdls[ubPort].bTxEnabled = FALSE;
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
        for( usIdx = 0; usIdx < usLength; )
        {
            /* Check if transmit holding register is empty. If yes
             * place character into buffer.
             */
            if( bprvDrvSerialGetUARTTHRIsEmpty( ubPort ) )
            {
                switch ( xDrvSerialHW[ubPort].eType )
                {
                case UART_TYPE:
                    xDrvSerialHW[ubPort].x.pxUARTBase->THR = pubBuffer[usIdx];
                    break;
                case UART1_TYPE:
                    xDrvSerialHW[ubPort].x.pxUART1Base->THR = pubBuffer[usIdx];
                    break;
                default:
                    MBP_ASSERT( 0 );
                }
                usIdx++;
            }
            else
            {
                /* Enable transmit holding register empty interrupt */
                vprvDrvSerialUARTTXIntEnable( ubPort, TRUE );
                ( void )xSemaphoreTake( xDrvSerialHdls[ubPort].xSemHdl, 0 );
            }
        }
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
        xDrvSerialHdls[ubPort].bRxEnabled = TRUE;
        vprvDrvSerialUARTRXIntEnable( ubPort, TRUE );
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
        vprvDrvSerialUARTRXIntEnable( ubPort, FALSE );
        xDrvSerialHdls[ubPort].bRxEnabled = FALSE;
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
             ( *pusNBytesReceived < usLengthMax ) && bprvDrvSerialGetUARTHasRxData( ubPort ); ( *pusNBytesReceived )++ )
        {
            switch ( xDrvSerialHW[ubPort].eType )
            {
            case UART_TYPE:
                pubBuffer[*pusNBytesReceived] = xDrvSerialHW[ubPort].x.pxUARTBase->RBR;
                break;
            case UART1_TYPE:
                pubBuffer[*pusNBytesReceived] = xDrvSerialHW[ubPort].x.pxUARTBase->RBR;
                break;
            }
        }
        if( bprvDrvSerialGetUARTHasRxData( ubPort ) )
        {
            ( void )xSemaphoreGive( xDrvSerialHdls[ubPort].xSemHdl );
        }
        /* Reenable receive interrupt */
        vprvDrvSerialUARTRXIntEnable( ubPort, TRUE );
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
        switch ( xDrvSerialHW[ubPort].eType )
        {
        case UART_TYPE:
            xDrvSerialHW[ubPort].x.pxUARTBase->FCR |= ( 1UL << 1UL );
            break;
        case UART1_TYPE:
            xDrvSerialHW[ubPort].x.pxUART1Base->FCR |= ( 1UL << 1UL );
            break;
        }
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

STATIC          portBASE_TYPE
xSerialIntHandler( UBYTE ubPort )
{
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    UBYTE           ubIIRValue = 0;

    MBP_ASSERT( DRV_SERIAL_IS_VALID( ubPort, xDrvSerialHdls ) );

    switch ( xDrvSerialHW[ubPort].eType )
    {
    case UART_TYPE:
        ubIIRValue = xDrvSerialHW[ubPort].x.pxUARTBase->IIR;
        ( void )xDrvSerialHW[ubPort].x.pxUARTBase->LSR;
        break;
    case UART1_TYPE:
        ubIIRValue = xDrvSerialHW[ubPort].x.pxUARTBase->IIR;
        ( void )xDrvSerialHW[ubPort].x.pxUARTBase->LSR;
        break;
    default:
        MBP_ASSERT( 0 );
        break;
    }
    ubIIRValue = ( ubIIRValue >> 1UL ) & 0x07UL;
    /* Transmit holding register empty */
    if( 1 == ubIIRValue )
    {
        vprvDrvSerialUARTTXIntEnable( ubPort, FALSE );
        xSemaphoreGiveFromISR( xDrvSerialHdls[ubPort].xSemHdl, &xHigherPriorityTaskWoken );
    }
    /* Receive data ready */
    else if( 2 == ubIIRValue )
    {
        vprvDrvSerialUARTRXIntEnable( ubPort, FALSE );
        xSemaphoreGiveFromISR( xDrvSerialHdls[ubPort].xSemHdl, &xHigherPriorityTaskWoken );
    }
    /* Character timeout */
    else if( 6 == ubIIRValue )
    {
        if( bprvDrvSerialGetUARTHasRxData( ubPort ) )
        {
            vprvDrvSerialUARTRXIntEnable( ubPort, FALSE );
            xSemaphoreGiveFromISR( xDrvSerialHdls[ubPort].xSemHdl, &xHigherPriorityTaskWoken );
        }
    }
    return xHigherPriorityTaskWoken;
}

#if DRV_SERIAL_MAX_INSTANCES >= 1
void
UART0_IRQHandler( void )
{
    portBASE_TYPE   xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = xSerialIntHandler( 0 );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
#endif

#if DRV_SERIAL_MAX_INSTANCES >= 2
void
UART1_IRQHandler( void )
{
    portBASE_TYPE   xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = xSerialIntHandler( 1 );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
#endif

#if DRV_SERIAL_MAX_INSTANCES >= 3
void
UART2_IRQHandler( void )
{
    portBASE_TYPE   xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = xSerialIntHandler( 2 );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
#endif

#if DRV_SERIAL_MAX_INSTANCES >= 3
void
UART3_IRQHandler( void )
{
    portBASE_TYPE   xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = xSerialIntHandler( 3 );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
#endif

STATIC INLINE   BOOL
bprvDrvSerialGetUARTHasRxData( UBYTE ubPort )
{
    BOOL            bHasData = FALSE;
    switch ( xDrvSerialHW[ubPort].eType )
    {
    case UART_TYPE:
        bHasData = xDrvSerialHW[ubPort].x.pxUARTBase->LSR & ( 1 << 0 ) ? TRUE : FALSE;
        break;
    case UART1_TYPE:
        bHasData = xDrvSerialHW[ubPort].x.pxUART1Base->LSR & ( 1 << 0 ) ? TRUE : FALSE;
        break;
    }
    return bHasData;
}

STATIC INLINE BOOL
bprvDrvSerialGetUARTTHRIsEmpty( UBYTE ubPort )
{
    BOOL            bHasData = FALSE;
    switch ( xDrvSerialHW[ubPort].eType )
    {
    case UART_TYPE:
        bHasData = xDrvSerialHW[ubPort].x.pxUARTBase->LSR & ( 1 << 5 ) ? TRUE : FALSE;
        break;
    case UART1_TYPE:
        bHasData = xDrvSerialHW[ubPort].x.pxUART1Base->LSR & ( 1 << 5 ) ? TRUE : FALSE;
        break;
    }
    return bHasData;
}

STATIC INLINE void
vprvDrvSerialUARTRXIntEnable( UBYTE ubPort, BOOL bOn )
{
    switch ( xDrvSerialHW[ubPort].eType )
    {
    case UART_TYPE:
        if( bOn )
        {

            xDrvSerialHW[ubPort].x.pxUARTBase->IER |= ( 1UL << 0UL );
        }
        else
        {
            xDrvSerialHW[ubPort].x.pxUARTBase->IER &= ~( 1UL << 0UL );
        }
        break;
    case UART1_TYPE:
        if( bOn )
        {
            ( void )xDrvSerialHW[ubPort].x.pxUART1Base->LSR;
            xDrvSerialHW[ubPort].x.pxUART1Base->IER |= ( 1UL << 0UL );
        }
        else
        {
            xDrvSerialHW[ubPort].x.pxUART1Base->IER &= ~( 1UL << 0UL );
        }
        break;
    default:
        MBP_ASSERT( 0 );
        break;
    }
}

STATIC INLINE void
vprvDrvSerialUARTTXIntEnable( UBYTE ubPort, BOOL bOn )
{
    switch ( xDrvSerialHW[ubPort].eType )
    {
    case UART_TYPE:
        if( bOn )
        {
            xDrvSerialHW[ubPort].x.pxUARTBase->IER |= ( 1UL << 1UL );
        }
        else
        {
            xDrvSerialHW[ubPort].x.pxUARTBase->IER &= ~( 1UL << 1UL );
        }
        break;
    case UART1_TYPE:
        if( bOn )
        {
            xDrvSerialHW[ubPort].x.pxUART1Base->IER |= ( 1UL << 1UL );
        }
        else
        {
            xDrvSerialHW[ubPort].x.pxUART1Base->IER &= ~( 1UL << 1UL );
        }
        break;
    default:
        MBP_ASSERT( 0 );
        break;
    }
}

STATIC INLINE   UBYTE
ubprvDrvSerialGetUARTTXFifoLevel( UBYTE ubPort )
{
    UBYTE           ubTxFifoLevel;
    switch ( xDrvSerialHW[ubPort].eType )
    {
    case UART_TYPE:
        ubTxFifoLevel = ( xDrvSerialHW[ubPort].x.pxUARTBase->FIFOLVL >> 8UL ) & 0xFUL;
        break;
    case UART1_TYPE:
        ubTxFifoLevel = ( xDrvSerialHW[ubPort].x.pxUART1Base->FIFOLVL >> 8UL ) & 0xFUL;
        break;
    }
    return ubTxFifoLevel;
}

STATIC          ULONG
ulprvDrvSerialGetUARTClock( UBYTE ubPort )
{
    ULONG           ulPCLKDIV = ( LPC_SC->PCLKSEL0 >> xDrvSerialHW[ubPort].ulPCLKDIVOff ) & 0x03;
    ULONG           ulPERCLK;
    switch ( ulPCLKDIV )
    {
    default:
    case 0:
        ulPERCLK = SystemCoreClock / 4;
        break;
    case 1:
        ulPERCLK = SystemCoreClock;
        break;
    case 2:
        ulPERCLK = SystemCoreClock / 2;
        break;
    case 3:
        ulPERCLK = SystemCoreClock / 8;
        break;
    }
    return ulPERCLK;
}
