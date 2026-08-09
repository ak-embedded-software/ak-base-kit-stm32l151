/*
 * MODBUS Library: LPC2388/lwIP  port
 * Copyright (c) 2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial.c,v 1.2 2011-06-21 20:52:15 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include "LPC23xx.h"
#include "typedefs.h"
#include "irq.h"
#include "target.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define IER_RBR				0x01
#define IER_THRE			0x02
#define IER_RLS				0x04

#define IIR_PEND			0x01
#define IIR_RLS				0x03
#define IIR_RDA				0x02
#define IIR_CTI				0x06
#define IIR_THRE			0x01

#define LSR_RDR				0x01
#define LSR_OE				0x02
#define LSR_PE				0x04
#define LSR_FE				0x08
#define LSR_BI				0x10
#define LSR_THRE			0x20
#define LSR_TEMT			0x40
#define LSR_RXFE			0x80

/* ----------------------- Defines ------------------------------------------*/
#define IDX_INVALID				( 255 )
#define UART_BAUDRATE_MIN		( 300 )
#define UART_BAUDRATE_MAX		( 115200 )

#define RS_485_DE_PIN			(  )
#define RS_485_INIT(  )			do {  } while( 0 )
#define RS_485_ENABLE_TX(  )	do { \
		FIO3SET2 |= 0x80; \
	} while( 0 )

#define RS_485_DISABLE_TX(  )	do { \
		vShortDelay( 10 ); \
		FIO3CLR2 |= 0x80; \
	} while( 0 )

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
STATIC void     vShortDelay( USHORT usCntArg );
void __attribute__ ( ( interrupt( ( "IRQ" ) ) ) ) UART3Handler( void );

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate, UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UBYTE           ubIdx, ubLCR;
    DWORD           Fdiv;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            HDL_RESET( &xSerialHdls[ubIdx] );
        }
        RS_485_INIT(  );
        RS_485_DISABLE_TX(  );
        bIsInitalized = TRUE;
    }

    if( NULL == pxSerialHdl )
    {
        eStatus = MB_EINVAL;
    }
    else if( ( 0 == ucPort ) && ( IDX_INVALID == xSerialHdls[0].ubIdx ) )
    {
        *pxSerialHdl = NULL;

        if( ( ulBaudRate >= UART_BAUDRATE_MIN ) && ( ulBaudRate <= UART_BAUDRATE_MAX ) && ( MB_HDL_INVALID != xMBMHdl ) )
        {
            switch ( eParity )
            {
            case MB_PAR_EVEN:
                ubLCR = ( 0x01 << 4 ) | ( 1 << 3 );
                break;
            case MB_PAR_ODD:
                ubLCR = ( 0x00 << 4 ) | ( 1 << 3 );
                break;
            case MB_PAR_NONE:
                ubLCR = 0x00;
                break;
            default:
                eStatus = MB_EINVAL;
            }

            switch ( ucDataBits )
            {
            case 8:
                ubLCR |= 0x03;
                break;
            case 7:
                ubLCR |= 0x02;
                break;
            default:
                eStatus = MB_EINVAL;
            }

            switch ( ucStopBits )
            {
            case 1:
                break;
            case 2:
                ubLCR |= 1 << 2;
                break;
            default:
                eStatus = MB_EINVAL;
            }

            if( InstallIRQ( UART3_INT, ( void * )UART3Handler, HIGHEST_PRIORITY ) == FALSE )
            {
                eStatus = MB_EPORTERR;
            }
            else if( MB_ENOERR == eStatus )
            {
                PINSEL9 |= 0x03000000;  /* Pin function select register 9: P4.28=>bits 25,24='11' => TxD3 */
                PINSEL9 |= 0x0C000000;  /* Pin function select register 9: P4.29=>bits 27,26='11' => RxD3 */
                PCONP |= 0x02000000;    /* Power Control for Peripherals register: power on UART3 (not powered by default) */
                U3ICR = 0x0;    /* IrDA Control Register for UART3 only: disable IrDA => normal UART */
                U3LCR = 0x80 | ubLCR;   /* UART3 Line Control Register + Enable access to Divisor Latches */
                /* UART baud rate = PCLK / (16 x (256 x U3DLM + U3DLL)) */
                Fdiv = ( Fpclk / 16 ) / ulBaudRate;
                U3DLM = Fdiv / 256;     /* UART3 Divisor Latch LSB Register: lower 8 bits of the divisor */
                U3DLL = Fdiv % 256;     /* UART3 Divisor Latch MSB Register: higher 8 bits of the divisor */
                U3LCR = 0x03;   /* UART3 Line Control Register (DLAB bit): disable access to Divisor Latches */
                U3FCR = 0x07;   /* UART3 FIFO Control Register: enable and reset TX and RX FIFOs */
                *pxSerialHdl = &xSerialHdls[0];
                xSerialHdls[0].ubIdx = 0;
                xSerialHdls[0].xMBMHdl = xMBMHdl;
            }
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

            RS_485_DISABLE_TX(  );
            HDL_RESET( pxSerialIntHdl );
            eStatus = MB_ENOERR;
        }
        else
        {
            eStatus = MB_EIO;
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
            MBP_ASSERT( NULL == pxSerialIntHdl->pbMBPTransmitterEmptyFN );
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = pbMBPTransmitterEmptyFN;
            RS_485_ENABLE_TX(  );
            U3IER = IER_THRE;
            VICSoftInt |= 1 << 29;
        }
        else
        {
            U3IER &= ~IER_THRE;
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
            RS_485_DISABLE_TX(  );

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
            MBP_ASSERT( NULL == pxSerialIntHdl->pvMBPReceiveFN );
            pxSerialIntHdl->pvMBPReceiveFN = pvMBPReceiveFN;
            U3IER = IER_RBR | IER_RLS;
        }
        else
        {
            U3IER &= ~( IER_RBR | IER_RLS );
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

void __attribute__ ( ( interrupt( ( "IRQ" ) ) ) ) UART3Handler( void )
{
    BYTE            IIRValue, LSRValue;
    UBYTE           ubTxByte = 0;
    BOOL            bHasMoreData;

    IIRValue = U3IIR;
    IIRValue >>= 1;
    IIRValue &= 0x07;

    if( ( IIRValue == IIR_RLS ) || ( IIRValue == IIR_RDA ) )
    {
        LSRValue = U3LSR;
        ubTxByte = U3RBR;
        if( NULL != xSerialHdls[0].pvMBPReceiveFN )
        {
            xSerialHdls[0].pvMBPReceiveFN( xSerialHdls[0].xMBMHdl, ubTxByte );
        }
    }
    if( VICSoftInt & ( 1 << 29 ) )
    {
        VICSoftIntClr = ( 1 << 29 );
        IIRValue = IIR_THRE;
    }
    if( IIRValue == IIR_THRE )
    {
        if( NULL != xSerialHdls[0].pbMBPTransmitterEmptyFN )
        {
            bHasMoreData = xSerialHdls[0].pbMBPTransmitterEmptyFN( xSerialHdls[0].xMBMHdl, &ubTxByte );
        }
        if( !bHasMoreData )
        {
            U3IER &= ~IER_THRE;
            xSerialHdls[0].pbMBPTransmitterEmptyFN = NULL;
            while( !( U3LSR & 0x40 ) );
            RS_485_DISABLE_TX(  );
        }
        else
        {
            U3THR = ubTxByte;
        }
    }

    DisableIRQ(  );
    VICVectAddr = 0;            /* Acknowledge Interrupt */
}

STATIC void
vShortDelay( USHORT usCntArg )
{
    volatile USHORT usCnt = usCntArg;
    while( usCnt-- );
}
