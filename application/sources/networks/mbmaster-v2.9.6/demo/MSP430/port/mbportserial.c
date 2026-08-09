/*
 * MODBUS Library: MSP430 port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial.c,v 1.1 2010-11-28 22:24:38 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include "io430x16xm.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define IDX_INVALID				( 255 )
#define UART_BAUDRATE_MIN		( 300 )
#define UART_BAUDRATE_MAX		( 57600 )

#define DEBUG_PERFORMANCE       ( 1 )

#if DEBUG_PERFORMANCE == 1
#define DEBUG_PIN_RX            ( 5 )
#define DEBUG_PIN_TX            ( 6 )
#define DEBUG_PORT_DIR          ( P5DIR )
#define DEBUG_PORT_OUT          ( P5OUT )
#define DEBUG_INIT(  )          do \
  { \
    DEBUG_PORT_DIR |= ( 1 << DEBUG_PIN_RX ) | ( 1 << DEBUG_PIN_TX ); \
    DEBUG_PORT_OUT &= ~( ( 1 << DEBUG_PIN_RX ) | ( 1 << DEBUG_PIN_TX ) ); \
  } while( 0 );
#define DEBUG_TOGGLE_RX(  )     DEBUG_PORT_OUT ^= ( 1 << DEBUG_PIN_RX )
#define DEBUG_TOGGLE_TX(  )     DEBUG_PORT_OUT ^= ( 1 << DEBUG_PIN_TX )
#else
#define DEBUG_INIT(  )
#define DEBUG_TOGGLE_RX(  )
#define DEBUG_TOGGLE_TX(  )
#endif

#define HDL_RESET( x ) do { \
	( x )->ubIdx = IDX_INVALID; \
	( x )->pbMBPTransmitterEmptyFN = NULL; \
	( x )->pvMBPReceiveFN = NULL; \
	( x )->xMBMHdl = MB_HDL_INVALID; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    pbMBPSerialTransmitterEmptyAPIV1CB pbMBPTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV1CB pvMBPReceiveFN;
    xMBHandle       xMBMHdl;
} xSerialHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialHandle xSerialHdls[1];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UBYTE           ubIdx;
    USHORT          UxCTL;
    USHORT          UxBR = ( USHORT ) ( SMCLK / ulBaudRate );

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            HDL_RESET( &xSerialHdls[ubIdx] );
        }
        DEBUG_INIT(  );
        bIsInitalized = TRUE;
    }

    if( NULL == pxSerialHdl )
    {
        eStatus = MB_EINVAL;
    }
    else if( ( 0 == ucPort ) && ( IDX_INVALID == xSerialHdls[0].ubIdx ) )
    {
        *pxSerialHdl = NULL;

        if( ( ulBaudRate > UART_BAUDRATE_MIN ) && ( ulBaudRate < UART_BAUDRATE_MAX ) && ( MB_HDL_INVALID != xMBMHdl ) )
        {
            UxCTL = 0;
            switch ( eParity )
            {
            case MB_PAR_NONE:
                break;
            case MB_PAR_ODD:
                UxCTL |= PENA;
                break;
            case MB_PAR_EVEN:
                UxCTL |= PENA | PEV;
                break;
            }

            switch ( ucDataBits )
            {
            case 7:
                break;
            default:
            case 8:
                UxCTL |= CHAR2;
                break;
            }

            switch ( ucStopBits )
            {

            case 2:
                UxCTL |= SPB;
                break;
            default:
            case 1:
                break;
            }
            U0CTL |= SWRST;
            /* Initialize all UART registers */
            U0CTL = UxCTL | SWRST;
            /* SSELx = 11 = SMCLK. Use only if PLL is synchronized ! */
            U0TCTL = SSEL1 | SSEL0;
            U0RCTL = URXEIE;
            /* Configure USART0 Baudrate Registers. */
            U0BR0 = ( UxBR & 0xFF );
            U0BR1 = ( UxBR >> 8 );
            U0MCTL = 0;
            /* Enable UART */
            ME1 |= UTXE0 | URXE0;
            /* Clear reset flag. */
            U0CTL &= ~SWRST;

            /* USART0 TXD/RXD */
            P3SEL |= 0x30;
            P3DIR |= 0x10;

            xSerialHdls[0].ubIdx = 0;
            xSerialHdls[0].xMBMHdl = xMBMHdl;
            *pxSerialHdl = &xSerialHdls[0];
            eStatus = MB_ENOERR;
        }
        else
        {
            eStatus = MB_EINVAL;
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
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        if( ( pxSerialIntHdl->pbMBPTransmitterEmptyFN == NULL ) && ( pxSerialIntHdl->pvMBPReceiveFN == NULL ) )
        {
            U0CTL = SWRST;
            U0TCTL = TXEPT;
            U0RCTL = 0;
            U0BR0 = U0BR1 = 0;
            U0MCTL = 0;
            eStatus = MB_ENOERR;
        }
        else
        {
            eStatus = MB_EAGAIN;
        }
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
        eStatus = MB_ENOERR;
        if( NULL != pbMBPTransmitterEmptyFN )
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = ( pbMBPSerialTransmitterEmptyAPIV1CB ) pbMBPTransmitterEmptyFN;
            IE1 |= UTXIE0;
            IFG1 |= UTXIFG0;
        }
        else
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
            IE1 &= ~UTXIE0;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBPReceiveFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pvMBPReceiveFN )
        {
            pxSerialIntHdl->pvMBPReceiveFN = ( pvMBPSerialReceiverAPIV1CB ) pvMBPReceiveFN;
            IE1 |= URXIE0;
        }
        else
        {
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
            IE1 &= ~URXIE0;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

#pragma vector = USART0TX_VECTOR
__interrupt STATIC void
prrvUSARTTxISR( void )
{
    BOOL            bHasMoreData = TRUE;
    UBYTE           ubTxByte;

    DEBUG_TOGGLE_TX(  );
    if( NULL != xSerialHdls[0].pbMBPTransmitterEmptyFN )
    {
        bHasMoreData = xSerialHdls[0].pbMBPTransmitterEmptyFN( xSerialHdls[0].xMBMHdl, &ubTxByte );
    }
    if( !bHasMoreData )
    {
        xSerialHdls[0].pbMBPTransmitterEmptyFN = NULL;
        IE1 &= ~UTXIE0;
    }
    else
    {
        TXBUF0 = ubTxByte;
    }
    DEBUG_TOGGLE_TX(  );
}

#pragma vector = USART0RX_VECTOR
__interrupt STATIC void
prrvUSARTRxISR( void )
{
    UBYTE           ubUDR = RXBUF0;

    DEBUG_TOGGLE_RX(  );
    if( NULL != xSerialHdls[0].pvMBPReceiveFN )
    {
        xSerialHdls[0].pvMBPReceiveFN( xSerialHdls[0].xMBMHdl, ubUDR );
    }
    DEBUG_TOGGLE_RX(  );
}
