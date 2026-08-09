/*
 * MODBUS Library: AT91SAM7X port
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial.c,v 1.1 2010-05-22 22:31:33 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

#include "AT91SAM7X256.h"
#include "lib_AT91SAM7X256.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/

#define USART_INTERRUPT_LEVEL           ( AT91C_AIC_PRIOR_HIGHEST )
#define NUARTS                          ( 2 )
#define USART_USART0_IDX                ( 0 )
#define USART_USART1_IDX                ( 1 )

#define IDX_INVALID                     ( 255 )
#define UART_BAUDRATE_MIN               ( 300 )
#define UART_BAUDRATE_MAX               ( 115200 )

#define UART_INIT( ubIdx )      do { \
    if( USART_USART0_IDX == ubIdx ) \
    { \
        AT91F_PIO_CfgPeriph( AT91C_BASE_PIOA, AT91C_PA0_RXD0 | AT91C_PA1_TXD0 | AT91C_PA3_RTS0, 0 ); \
    } \
    else if( USART_USART1_IDX == ubIdx ) \
    { \
        AT91F_PIO_CfgPeriph( AT91C_BASE_PIOA, AT91C_PA5_RXD1 | AT91C_PA6_TXD1 | AT91C_PA8_RTS1, 0 ); \
    } \
    else \
    { \
        MBP_ASSERT( 0 ); \
    } \
} while( 0 )

#define HDL_RESET( x ) do { \
	( x )->ubIdx = IDX_INVALID; \
	( x )->pbMBPTransmitterEmptyFN = NULL; \
	( x )->pvMBPReceiveFN = NULL; \
	( x )->xMBHdl = MB_HDL_INVALID; \
} while( 0 );

/* ----------------------- Function prototypes ------------------------------*/
#if defined( __ICCARM__ ) && ( __ICCARM__ == 1 )
extern void     vUSART0ISRWrapper( void );
__arm void      vUSART0ISR( void );
extern void     vUSART1ISRWrapper( void );
__arm void      vUSART1ISR( void );
#endif

/* ----------------------- Type definitions ---------------------------------*/

typedef struct
{
    UBYTE           ubIdx;
    pbMBPSerialTransmitterEmptyAPIV1CB pbMBPTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV1CB pvMBPReceiveFN;
    xMBHandle       xMBHdl;
} xMBPSerialIntHandle;

struct
{
    unsigned int    uiAT91C_ID_USX;
    AT91PS_USART    pxCOM;
    void            ( *pvIRQHandlerFN ) ( void );
}
const           xMBPSerialHW[NUARTS] = {
    {AT91C_ID_US0, AT91C_BASE_US0, vUSART0ISRWrapper},
    {AT91C_ID_US1, AT91C_BASE_US1, vUSART1ISRWrapper}
};

/* ----------------------- Static variables ---------------------------------*/
STATIC xMBPSerialIntHandle xSerialHdls[NUARTS];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
#if defined( __ICCARM__ ) && ( __ICCARM__ == 1 )
__arm STATIC portBASE_TYPE xUSARTIRQHandler( UBYTE ubIdx );
#endif

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate, UCHAR ucDataBits,
                eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    xMBPSerialIntHandle *pxSerialIntHdl;
    unsigned int    uiUARTMode;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        HDL_RESET( &xSerialHdls[USART_USART0_IDX] );
        HDL_RESET( &xSerialHdls[USART_USART1_IDX] );
        bIsInitalized = TRUE;
    }

    pxSerialIntHdl = NULL;
    if( NULL != pxSerialHdl )
    {
        uiUARTMode = AT91C_US_USMODE_RS485 | AT91C_US_CLKS_CLOCK;
        switch ( eParity )
        {
        case MB_PAR_NONE:
            uiUARTMode |= AT91C_US_PAR_NONE;
            break;
        case MB_PAR_EVEN:
            uiUARTMode |= AT91C_US_PAR_EVEN;
            break;
        case MB_PAR_ODD:
            uiUARTMode |= AT91C_US_PAR_ODD;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }
        switch ( ucDataBits )
        {
        case 8:
            uiUARTMode |= AT91C_US_CHRL_8_BITS;
            break;
        case 7:
            uiUARTMode |= AT91C_US_CHRL_7_BITS;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }
        switch ( ucStopBits )
        {
        case 1:
            uiUARTMode |= AT91C_US_NBSTOP_1_BIT;
            break;
        case 2:
            uiUARTMode |= AT91C_US_NBSTOP_2_BIT;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }

        if( MB_ENOERR == eStatus )
        {
            pxSerialIntHdl = &xSerialHdls[ucPort];
            pxSerialIntHdl->ubIdx = ucPort;
            if( NULL != pxSerialIntHdl )
            {
                AT91F_PMC_EnablePeriphClock( AT91C_BASE_PMC, 1 << xMBPSerialHW[ucPort].uiAT91C_ID_USX );
                AT91F_US_Configure( xMBPSerialHW[ucPort].pxCOM, configCPU_CLOCK_HZ, uiUARTMode, ulBaudRate, 0 );
                xMBPSerialHW[ucPort].pxCOM->US_CR = AT91C_US_TXEN | AT91C_US_RXEN;
                AT91F_AIC_ConfigureIt( AT91C_BASE_AIC, xMBPSerialHW[ucPort].uiAT91C_ID_USX, USART_INTERRUPT_LEVEL,
                                       AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, xMBPSerialHW[ucPort].pvIRQHandlerFN );
                AT91F_AIC_EnableIt( AT91C_BASE_AIC, xMBPSerialHW[ucPort].uiAT91C_ID_USX );
                UART_INIT( pxSerialIntHdl->ubIdx );

                pxSerialIntHdl->xMBHdl = xMBHdl;
                *pxSerialHdl = pxSerialIntHdl;
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_ENORES;
            }
        }
    }
    else
    {
        eStatus = MB_EINVAL;
    }

    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialClose( xMBPSerialHandle xSerialHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPSerialIntHandle *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        if( ( pxSerialIntHdl->pbMBPTransmitterEmptyFN == NULL ) && ( pxSerialIntHdl->pvMBPReceiveFN == NULL ) )
        {
            AT91F_AIC_DisableIt( AT91C_BASE_AIC, xMBPSerialHW[pxSerialIntHdl->ubIdx].uiAT91C_ID_USX );
            AT91F_US_Close( xMBPSerialHW[pxSerialIntHdl->ubIdx].pxCOM );
            HDL_RESET( pxSerialIntHdl );
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
    xMBPSerialIntHandle *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pbMBPTransmitterEmptyFN )
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = ( pbMBPSerialTransmitterEmptyAPIV1CB ) pbMBPTransmitterEmptyFN;
            AT91F_US_EnableIt( xMBPSerialHW[pxSerialIntHdl->ubIdx].pxCOM, AT91C_US_TXRDY );
        }
        else
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
            AT91F_US_DisableIt( xMBPSerialHW[pxSerialIntHdl->ubIdx].pxCOM, AT91C_US_TXRDY );
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBPReceiveFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPSerialIntHandle *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pvMBPReceiveFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pvMBPReceiveFN );
            pxSerialIntHdl->pvMBPReceiveFN = ( pvMBPSerialReceiverAPIV1CB ) pvMBPReceiveFN;
            AT91F_US_EnableIt( xMBPSerialHW[pxSerialIntHdl->ubIdx].pxCOM, AT91C_US_RXRDY );
        }
        else
        {
            AT91F_US_DisableIt( xMBPSerialHW[pxSerialIntHdl->ubIdx].pxCOM, AT91C_US_RXRDY );
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}


#if defined( __ICCARM__ ) && ( __ICCARM__ == 1 )
__arm STATIC    portBASE_TYPE
xUSARTIRQHandler( UBYTE ubIdx )
#endif
{
    BOOL            bHasData;
    UBYTE           ubValue;
    unsigned int    uiUSARTStatus = xMBPSerialHW[ubIdx].pxCOM->US_CSR;

    MBP_ASSERT( ubIdx == xSerialHdls[ubIdx].ubIdx );

    if( uiUSARTStatus & AT91C_US_RXRDY )
    {
        if( NULL != xSerialHdls[ubIdx].pvMBPReceiveFN )
        {
            ubValue = AT91F_US_GetChar( xMBPSerialHW[ubIdx].pxCOM );
            xSerialHdls[ubIdx].pvMBPReceiveFN( xSerialHdls[ubIdx].xMBHdl, ubValue );
        }
        else
        {
            AT91F_US_DisableIt( xMBPSerialHW[ubIdx].pxCOM, AT91C_US_RXRDY );
        }
    }
    if( uiUSARTStatus & AT91C_US_TXRDY )
    {
        bHasData = FALSE;
        if( NULL != xSerialHdls[ubIdx].pbMBPTransmitterEmptyFN )
        {
            bHasData = xSerialHdls[ubIdx].pbMBPTransmitterEmptyFN( xSerialHdls[ubIdx].xMBHdl, &ubValue );
            if( bHasData )
            {
                AT91F_US_PutChar( xMBPSerialHW[ubIdx].pxCOM, ubValue );
            }
        }
        if( !bHasData )
        {
            AT91F_US_DisableIt( xMBPSerialHW[ubIdx].pxCOM, AT91C_US_TXRDY );
            xSerialHdls[ubIdx].pbMBPTransmitterEmptyFN = NULL;
        }
    }
    return pdFALSE;
}

#if defined( __ICCARM__ ) && ( __ICCARM__ == 1 )
__arm void
vUSART0ISR( void )
#endif
{
    portBASE_TYPE   xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = xUSARTIRQHandler( USART_USART0_IDX );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    AT91C_BASE_AIC->AIC_EOICR = 0;
}

#if defined( __ICCARM__ ) && ( __ICCARM__ == 1 )
__arm void
vUSART1ISR( void )
#endif
{
    portBASE_TYPE   xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = xUSARTIRQHandler( USART_USART1_IDX );
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    AT91C_BASE_AIC->AIC_EOICR = 0;
}
