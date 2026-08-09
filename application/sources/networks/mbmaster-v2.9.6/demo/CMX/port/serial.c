/* 
 * MODBUS Library: Low Level Serial I/O for CMX
 * Copyright (c) 2008 Gerhard Bran
 * All rights reserved.
 *
 * $Id: serial.c,v 1.3 2008-09-01 18:43:14 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "cxfuncs.h"
#include "m523xevb.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define BAUDRATE_VALUE( ulFSYS, ulBaud ) ( ( ulFSYS )/( 32UL * ulBaud ) )
#define arubTXBUFSIZE     256
#define arubRXBUFSIZE     256
#define arubTXBUFMASK     ((USHORT)(arubTXBUFSIZE-1))
#define arubRXBUFMASK     ((USHORT)(arubRXBUFSIZE-1))

#define arubTXBUF_FREE    (arubTXBUFSIZE - ((usTXPut - usTXGet) & arubTXBUFMASK))

#define USE_UART0TX
#define USE_UART0RX
#define RELEASE_UART0RX
#define RELEASE_UART0TX

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
static UBYTE    arubTXBUF[arubTXBUFSIZE];
static UBYTE    arubRXBUF[arubRXBUFSIZE];
static USHORT   usTXPut;
static USHORT   usTXGet;
static USHORT   usRXPut;
static USHORT   usRXGet;
static UBYTE    bTXBusy;

static USHORT   usRXThreshold;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vMBPSerialUART0BufInit( void );

/* ----------------------- Start implementation -----------------------------*/

void
vMBPSerialUART0Init( ULONG ulBaudRate, UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits )
{
    UBYTE           ucUMR1, ucUMR2;

    switch ( eParity )
    {
    case MB_PAR_EVEN:
        ucUMR1 = MCF_UART_UMR_PM( 0x0 );
        break;
    case MB_PAR_ODD:
        ucUMR1 = MCF_UART_UMR_PM( 0x0 ) | MCF_UART_UMR_PT;
        break;
    default:
    case MB_PAR_NONE:
        ucUMR1 = MCF_UART_UMR_PM( 0x2 );
        break;
    }

    switch ( ucDataBits )
    {
    case 7:
        ucUMR1 |= MCF_UART_UMR_BC( 0x2 );
        break;
    default:
    case 8:
        ucUMR1 |= MCF_UART_UMR_BC( 0x3 );
        break;
    }

    switch ( ucStopBits )
    {

    case 2:
        ucUMR2 = MCF_UART_UMR_SB( 0xF );
        break;
    default:
    case 1:
        ucUMR2 = MCF_UART_UMR_SB( 0x7 );
        break;
    }

    MBP_ENTER_CRITICAL_SECTION_ISR(  );

    /* UART 0: Enable pins */
    MCF_GPIO_PAR_UART = MCF_GPIO_PAR_UART_PAR_U0RXD | MCF_GPIO_PAR_UART_PAR_U0TXD;

    /* Mask all UART0 interrupts */
    MCF_UART_UIMR0 = 0;

    /* Disable Receiver and Transmitter */
    MCF_UART_UCR0 = ( MCF_UART_UCR_TX_DISABLED | MCF_UART_UCR_RX_DISABLED );

    /* Reset Receiver, Transmitter and Mode Register */
    MCF_UART_UCR0 = MCF_UART_UCR_RESET_TX;
    MCF_UART_UCR0 = MCF_UART_UCR_RESET_RX;
    MCF_UART_UCR0 = MCF_UART_UCR_RESET_ERROR;
    MCF_UART_UCR0 = MCF_UART_UCR_BKCHGINT;
    MCF_UART_UCR0 = MCF_UART_UCR_RESET_MR;

    /* Enable UART0 interrupt sources (RX, TX) */
    MCF_UART_UIMR0 = MCF_UART_UIMR_RXRDY_FU;

    /* UART 0: Enable interrupts */
    MCF_INTC0_ICR13 = MCF_INTC0_ICRn_IL( 0x2 ) | MCF_INTC0_ICRn_IP( 0x1 );
    MCF_INTC0_IMRL &= ~( MCF_INTC0_IMRL_INT_MASK13 | MCF_INTC0_IMRL_MASKALL );

    /* UART 0 Clocking */
    MCF_UART_UCSR0 = MCF_UART_UCSR_RCS( 0xd ) | MCF_UART_UCSR_TCS( 0xd );
    MCF_UART_UBG10 = ( UCHAR ) ( BAUDRATE_VALUE( SYSTEM_CLOCK * 1000000UL, ulBaudRate ) >> 8U );
    MCF_UART_UBG20 = ( UCHAR ) ( BAUDRATE_VALUE( SYSTEM_CLOCK * 1000000UL, ulBaudRate ) & 0xFFU );

    /* Configure Parity, Databits and Stopbits */
    MCF_UART_UMR0 = ucUMR1;
    MCF_UART_UMR0 = ucUMR2;

    /* Enable Receiver and Transmitter */
    MCF_UART_UCR0 = ( MCF_UART_UCR_TX_ENABLED | MCF_UART_UCR_RX_ENABLED );

    MBP_EXIT_CRITICAL_SECTION_ISR(  );
}


BOOL
bMBPSerialUART0Send( UBYTE * pubBuffer, USHORT usLength )
{
    BOOL            bStatus = FALSE;
    USHORT          usIdx;

    MBP_ENTER_CRITICAL_SECTION_ISR(  );
    if( usLength < arubTXBUF_FREE )
    {
        for( usIdx = 0; usIdx < usLength; usIdx++ )
        {
            arubTXBUF[usTXPut++] = pubBuffer[usIdx];
            usTXPut &= arubTXBUFMASK;
        }

        /* If transmission is currently running, do nothing */
        if( MCF_UART_USR0 & MCF_UART_USR_TXEMP )
        {
            /* Trigger new transmission */
            usIdx = usTXGet++;
            usTXGet &= arubTXBUFMASK;
            /* Copy first byte to UTB */
            MCF_UART_UTB0 = arubTXBUF[usIdx];
            bTXBusy = 1;
            MCF_UART_UIMR0 = ( MCF_UART_UIMR_RXRDY_FU | MCF_UART_UIMR_TXRDY );
        }
        bStatus = TRUE;
    }
    MBP_EXIT_CRITICAL_SECTION_ISR(  );
    return bStatus;
}

USHORT
vMBPSerialUART0Receive( UBYTE * pubBuffer, USHORT usLengthMax )
{
    USHORT          usBytesReceived = 0;

    MBP_ENTER_CRITICAL_SECTION_ISR(  );
    for( usBytesReceived = 0; ( ( usBytesReceived < usLengthMax ) && ( usRXPut != usRXGet ) ); usBytesReceived++ )
    {
        pubBuffer[usBytesReceived] = arubRXBUF[usRXGet++];
        usRXGet &= arubRXBUFMASK;
    }
    MBP_EXIT_CRITICAL_SECTION_ISR(  );

    if( usBytesReceived > usRXThreshold )
    {
        usRXThreshold = usBytesReceived;
    }
    return usBytesReceived;
}

STATIC void
vMBPSerialUART0BufInit( void )
{
    usTXPut = 0;
    usTXGet = 0;
    usRXPut = 0;
    usRXGet = 0;
    bTXBusy = 0;
}

void
vprvMBPSerialIRQHandler( void )
{
    USHORT          usIdx;
    byte            bStatus;

    if( MCF_UART_UISR0 & MCF_UART_UISR_RXRDY_FU )
    {
        /* Received character available in FIFO (FFULL) */
        usIdx = usRXPut++;
        usRXPut &= arubRXBUFMASK;
        arubRXBUF[usIdx] = MCF_UART_URB0;
        /* Signal the application that there is some data available. */
        bStatus = K_Intrp_Event_Signal( 0, g_bSerialTaskSlot, MB_SERRX_EVENT );
        MBP_ASSERT( K_OK == bStatus );
    }

    /* If transmitting and if the transmitter holding register is empty. */
    if( bTXBusy && ( MCF_UART_UISR0 & MCF_UART_UISR_TXRDY ) )
    {
        if( usTXPut != usTXGet )
        {
            /* characters to transmit are available */
            /* increment get pointer before copying the byte to UTB (to prevent a race condition) */
            usIdx = usTXGet++;
            usTXGet &= arubTXBUFMASK;
            /* load the transmitter holding register with a character */
            MCF_UART_UTB0 = arubTXBUF[usIdx];
        }
        else
        {
            /* TX buffer empty, transmission finished */
            bTXBusy = 0;
            /* Enable only RX interrupt, Disable TX interrupt */
            MCF_UART_UIMR0 = MCF_UART_USR_FFULL;
        }
    }
}
