/* 
 * MODBUS Library: ATxmega port
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * This examples uses USARTC0 and PORTC 2/3. The RS485 is controlled by
 * PORTC pin 4. If your hardware is different please modify the following
 * elements. No further modifications are necessary
 *
 *  - xSerialHWHandle
 *  - Defines for HW_X_RXVECT, HW_X_TXVECT, HW_X_DREVECT
 *
 * $Id: mbportserial.c,v 1.1 2014-03-09 12:51:23 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <asf.h>
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define IDX_INVALID				( 255 )

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Defines (Internal - Don't change) ----------------*/
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

typedef struct
{
    USART_t        *pxUSART;
    ioport_pin_t    xIOPortTX;
    ioport_pin_t    xIOPortRX;
    BOOL            bHasRS485Line;
    ioport_pin_t    xIOPortRS485;
} xSerialHWHandle_t;

/* ----------------------- Static variables ---------------------------------*/

STATIC const xSerialHWHandle_t xSerialHWHandle[] = {
    {&USARTC0, IOPORT_CREATE_PIN( PORTC, 3 ), IOPORT_CREATE_PIN( PORTC, 2 ), TRUE, IOPORT_CREATE_PIN( PORTC, 4 )}
};

#define HW_0_RXVECT		USARTC0_RXC_vect
#define HW_0_TXVECT		USARTC0_TXC_vect
#define HW_0_DREVECT	USARTC0_DRE_vect
#define HW_0_HDL_IDX	0

STATIC xSerialHandle xSerialHdls[MB_UTILS_NARRSIZE( xSerialHWHandle )];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate, UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    usart_rs232_options_t xSerialOptions;
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            HDL_RESET( &xSerialHdls[ubIdx] );
        }
        bIsInitalized = TRUE;
    }

    if( NULL == pxSerialHdl )
    {
        eStatus = MB_EINVAL;
    }
    else
    {
        if( ( ucPort < MB_UTILS_NARRSIZE( xSerialHWHandle ) ) && ( xSerialHdls[ucPort].ubIdx == IDX_INVALID ) )
        {
            xSerialOptions.baudrate = ulBaudRate;
            xSerialOptions.charlength = ucDataBits;
            switch ( ucDataBits )
            {
            default:
            case 8:
                xSerialOptions.charlength = USART_CHSIZE_8BIT_gc;
                break;
            case 7:
                xSerialOptions.charlength = USART_CHSIZE_7BIT_gc;
                break;
            }
            switch ( eParity )
            {
            default:
            case MB_PAR_NONE:
                xSerialOptions.paritytype = USART_PMODE_DISABLED_gc;
                break;
            case MB_PAR_ODD:
                xSerialOptions.paritytype = USART_PMODE_ODD_gc;
                break;
            case MB_PAR_EVEN:
                xSerialOptions.paritytype = USART_PMODE_EVEN_gc;
                break;
            }
            switch ( ucStopBits )
            {
            default:
            case 1:
                xSerialOptions.stopbits = false;
                break;
            case 2:
                xSerialOptions.stopbits = true;
                break;
            }

            if( usart_init_rs232( xSerialHWHandle[ucPort].pxUSART, &xSerialOptions ) )
            {
                ioport_configure_pin( xSerialHWHandle[ucPort].xIOPortTX, IOPORT_DIR_OUTPUT | IOPORT_INIT_HIGH );
                ioport_configure_pin( xSerialHWHandle[ucPort].xIOPortRX, IOPORT_DIR_INPUT );
                ioport_configure_pin( xSerialHWHandle[ucPort].xIOPortRS485, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW );
                usart_tx_enable( xSerialHWHandle[ucPort].pxUSART );
                usart_rx_enable( xSerialHWHandle[ucPort].pxUSART );

                xSerialHdls[ucPort].pbMBPTransmitterEmptyFN = NULL;
                xSerialHdls[ucPort].pvMBPReceiveFN = NULL;
                xSerialHdls[ucPort].xMBMHdl = xMBMHdl;
                xSerialHdls[ucPort].ubIdx = ucPort;
                *pxSerialHdl = &xSerialHdls[ucPort];
            }
            else
            {
                eStatus = MB_EPORTERR;
            }
        }
        else
        {
            eStatus = MB_ENORES;
        }
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
        usart_set_rx_interrupt_level( xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART, USART_INT_LVL_OFF );
        usart_set_tx_interrupt_level( xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART, USART_INT_LVL_OFF );
        usart_set_dre_interrupt_level( xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART, USART_INT_LVL_OFF );
        usart_tx_disable( xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART );
        usart_rx_disable( xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART );
		HDL_RESET( pxSerialIntHdl );
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
        eStatus = MB_ENOERR;
        if( NULL != pbMBPTransmitterEmptyFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pbMBPTransmitterEmptyFN );
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = pbMBPTransmitterEmptyFN;
            usart_set_dre_interrupt_level( xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART, USART_INT_LVL_LO );
        }
        else
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
            usart_set_tx_interrupt_level( xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART, USART_INT_LVL_LO );
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
            ( void )xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART->STATUS;
            ( void )xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART->DATA;
            usart_set_rx_interrupt_level( xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART, USART_INT_LVL_LO );
        }
        else
        {
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
            usart_set_rx_interrupt_level( xSerialHWHandle[pxSerialIntHdl->ubIdx].pxUSART, USART_INT_LVL_OFF );
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

ISR( HW_0_RXVECT )
{
    UBYTE           ubUDR = usart_get( xSerialHWHandle[HW_0_HDL_IDX].pxUSART );

    MBP_ASSERT( IDX_INVALID != xSerialHdls[HW_0_HDL_IDX].ubIdx );
    if( NULL != xSerialHdls[HW_0_HDL_IDX].pvMBPReceiveFN )
    {
        xSerialHdls[HW_0_HDL_IDX].pvMBPReceiveFN( xSerialHdls[HW_0_HDL_IDX].xMBMHdl, ubUDR );
    }
}

ISR( HW_0_TXVECT )
{
    if( xSerialHWHandle[HW_0_HDL_IDX].bHasRS485Line )
    {
        ioport_set_pin_level( xSerialHWHandle[HW_0_HDL_IDX].xIOPortRS485, false );
    }
    usart_set_tx_interrupt_level( xSerialHWHandle[HW_0_HDL_IDX].pxUSART, USART_INT_LVL_OFF );
}

ISR( HW_0_DREVECT )
{
    BOOL            bHasMoreData = TRUE;
    UBYTE           ubTxByte;
    MBP_ASSERT( IDX_INVALID != xSerialHdls[HW_0_HDL_IDX].ubIdx );

    if( NULL != xSerialHdls[HW_0_HDL_IDX].pbMBPTransmitterEmptyFN )
    {
        bHasMoreData = xSerialHdls[HW_0_HDL_IDX].pbMBPTransmitterEmptyFN( xSerialHdls[HW_0_HDL_IDX].xMBMHdl, &ubTxByte );
    }
    if( !bHasMoreData )
    {
        xSerialHdls[0].pbMBPTransmitterEmptyFN = NULL;
        usart_set_dre_interrupt_level( xSerialHWHandle[HW_0_HDL_IDX].pxUSART, USART_INT_LVL_OFF );
        usart_set_tx_interrupt_level( xSerialHWHandle[HW_0_HDL_IDX].pxUSART, USART_INT_LVL_LO );
    }
    else
    {
        if( xSerialHWHandle[HW_0_HDL_IDX].bHasRS485Line )
        {
            ioport_set_pin_level( xSerialHWHandle[HW_0_HDL_IDX].xIOPortRS485, true );
        }
        xSerialHWHandle[HW_0_HDL_IDX].pxUSART->DATA = ubTxByte;
    }
}
