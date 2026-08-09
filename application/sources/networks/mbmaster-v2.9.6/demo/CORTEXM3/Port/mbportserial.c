/*
 * MODBUS Library: ARM Cortex M3 Port
 * Copyright (c) Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial.c,v 1.1 2009-01-01 23:29:18 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"
#include "LM3Sxxxx.H"
#include "rom.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/

#define IDX_INVALID				( 255 )
#define UART_BAUDRATE_MIN		( 300 )
#define UART_BAUDRATE_MAX		( 115200 )


#define UART_0_ENABLED          ( 1 )   /*!< Set this to 1 to enable USART1 */
#define UART_1_ENABLED          ( 1 )   /*!< Set this to 1 to enable USART2 */

#if ( UART_0_ENABLED == 1 ) && ( UART_1_ENABLED == 1 )
#define UART_0_PORT             ( MB_UART_0 )
#define UART_1_PORT             ( MB_UART_1 )
#define UART_0_IDX              ( 0 )
#define UART_1_IDX              ( 1 )
#define NUARTS                  ( 2 )
#elif ( UART_0_ENABLED == 1 )
#define UART_0_PORT             ( MB_UART_0 )
#define UART_0_IDX              ( 0 )
#define NUARTS                  ( 1 )
#elif ( UART_1_ENABLED == 1 )
#define UART_1_PORT             ( MB_UART_1 )
#define UART_1_IDX              ( 0 )
#define NUARTS                  ( 1 )
#else
#define NUARTS                  ( 0 )
#endif

#define RS_485_UART_0_INIT(  )	\
do { \
} while( 0 )

#define RS_485_UART_0_ENABLE_TX(  )	\
do {\
} while( 0 )

#define RS_485_UART_0_DISABLE_TX(  ) \
do { \
} while( 0 )

#define RS_485_UART_1_INIT(  ) \
do { \
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_5); \
	ROM_GPIOPinWrite( GPIO_PORTC_BASE, GPIO_PIN_5, 0x00 ); \
} while( 0 )

#define RS_485_UART_1_ENABLE_TX(  )	\
do { \
	ROM_GPIOPinWrite( GPIO_PORTC_BASE, GPIO_PIN_5, 0xFF ); \
} while( 0 )

#define RS_485_UART_1_DISABLE_TX(  ) \
do { \
	ROM_GPIOPinWrite( GPIO_PORTC_BASE, GPIO_PIN_5, 0x00 ); \
} while( 0 )

/* ----------------------- Defines (Internal - Don't change) ----------------*/
#define HDL_RESET( x ) do { \
	( x )->ubIdx = IDX_INVALID; \
	( x )->pbMBMTransmitterEmptyFN = NULL; \
	( x )->pvMBMReceiveFN = NULL; \
	( x )->xMBHdl = MB_HDL_INVALID; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    pbMBPSerialTransmitterEmptyAPIV1CB pbMBMTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV1CB pvMBMReceiveFN;
    xMBHandle       xMBHdl;
} xSerialHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialHandle xSerialHdls[NUARTS];
STATIC BOOL     bIsInitalized = FALSE;

#if UART_0_ENABLED == 1
STATIC ULONG    ulUART0SoftIntStatus;
#endif

#if UART_1_ENABLED == 1
STATIC ULONG    ulUART1SoftIntStatus;
#endif

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UBYTE           ubIdx;
    ULONG           ulUARTxConfig = 0;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            HDL_RESET( &xSerialHdls[ubIdx] );
        }
#if UART_0_ENABLED == 1
        ROM_SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOA );
        ROM_SysCtlPeripheralEnable( SYSCTL_PERIPH_UART0 );
        RS_485_UART_0_INIT(  );
        RS_485_UART_0_DISABLE_TX(  );

        ROM_GPIOPinTypeUART( GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
#endif
#if UART_1_ENABLED == 1
        ROM_SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOC );
        ROM_SysCtlPeripheralEnable( SYSCTL_PERIPH_UART1 );
        RS_485_UART_1_INIT(  );
        RS_485_UART_1_DISABLE_TX(  );
        ROM_GPIOPinTypeUART( GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7 );
#endif
        bIsInitalized = TRUE;
    }

    if( ( MB_HDL_INVALID == xMBMHdl ) || ( NULL == pxSerialHdl ) )
    {
        eStatus = MB_EINVAL;
    }
    else
    {
        eStatus = MB_ENORES;

        /* Setup baudrate */
        if( ( ulBaudRate < UART_BAUDRATE_MIN ) || ( ulBaudRate > UART_BAUDRATE_MAX ) )
        {
            eStatus = MB_EINVAL;
        }

        /* Setup stopbits */
        switch ( ucStopBits )
        {
        case 1:
            ulUARTxConfig |= UART_CONFIG_STOP_ONE;
            break;
        case 2:
            ulUARTxConfig |= UART_CONFIG_STOP_TWO;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }

        switch ( eParity )
        {
        case MB_PAR_ODD:
            ulUARTxConfig |= UART_CONFIG_PAR_ODD;
            break;
        case MB_PAR_EVEN:
            ulUARTxConfig |= UART_CONFIG_PAR_EVEN;
            break;
        case MB_PAR_NONE:
            ulUARTxConfig |= UART_CONFIG_PAR_NONE;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }

        switch ( ucDataBits )
        {
        case 8:
            ulUARTxConfig |= UART_CONFIG_WLEN_8;
            break;
        case 7:
            ulUARTxConfig |= UART_CONFIG_WLEN_7;
            break;
        default:
            eStatus = MB_EINVAL;
            break;
        }

        if( eStatus != MB_EINVAL )
        {
            switch ( ucPort )
            {
#if UART_1_ENABLED == 1
            case UART_0_PORT:
                if( IDX_INVALID == xSerialHdls[UART_0_IDX].ubIdx )
                {
                    ROM_UARTConfigSetExpClk( UART0_BASE, ROM_SysCtlClockGet(  ), ulBaudRate, ulUARTxConfig );
                    HWREG( UART0_BASE + UART_O_LCRH ) &= ~0x10;
                    ROM_IntPrioritySet( INT_UART0, MB_INTERRUPT_PRIORITY );
                    ROM_IntEnable( INT_UART0 );

                    /* Setup handle to uart */
                    *pxSerialHdl = &xSerialHdls[UART_0_IDX];
                    xSerialHdls[UART_0_IDX].ubIdx = UART_0_IDX;
                    xSerialHdls[UART_0_IDX].xMBHdl = xMBMHdl;

                    /* Everything is ok */
                    eStatus = MB_ENOERR;
                }
                else
                {
                    eStatus = MB_ENORES;
                }
                break;
#endif
#if UART_1_ENABLED == 1
            case UART_1_PORT:
                if( IDX_INVALID == xSerialHdls[UART_1_IDX].ubIdx )
                {
                    ROM_UARTConfigSetExpClk( UART1_BASE, ROM_SysCtlClockGet(  ), ulBaudRate, ulUARTxConfig );
                    HWREG( UART1_BASE + UART_O_LCRH ) &= ~0x10;
                    ROM_IntPrioritySet( INT_UART1, MB_INTERRUPT_PRIORITY );
                    ROM_IntEnable( INT_UART1 );

                    /* Setup handle to uart */
                    *pxSerialHdl = &xSerialHdls[UART_1_IDX];
                    xSerialHdls[UART_1_IDX].ubIdx = UART_1_IDX;
                    xSerialHdls[UART_1_IDX].xMBHdl = xMBMHdl;

                    /* Everything is ok */
                    eStatus = MB_ENOERR;
                }
                else
                {
                    eStatus = MB_ENORES;
                }
                break;
#endif
            default:
                break;
            }
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
        switch ( pxSerialIntHdl->ubIdx )
        {
#if UART_0_ENABLED == 1
        case UART_0_IDX:
            if( ( NULL == pxSerialIntHdl->pbMBMTransmitterEmptyFN ) && ( NULL == pxSerialIntHdl->pvMBMReceiveFN ) )
            {
                /* Disable UART 0 interrupts. */
                ROM_UARTIntDisable( UART0_BASE, UART_INT_RX | UART_INT_TX );
                /* Close UART 0 */
                ROM_UARTDisable( UART0_BASE );
                /* Force RS485 back to receive mode */
                RS_485_UART_0_DISABLE_TX(  );
                /* Reset handle */
                HDL_RESET( pxSerialIntHdl );
                /* No error */
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_EAGAIN;
            }
            break;
#endif
#if UART_1_ENABLED == 1
        case UART_1_IDX:
            if( ( NULL == pxSerialIntHdl->pbMBMTransmitterEmptyFN ) && ( NULL == pxSerialIntHdl->pvMBMReceiveFN ) )
            {
                /* Disable UART 1 interrupts. */
                ROM_UARTIntDisable( UART1_BASE, UART_INT_RX | UART_INT_TX );
                /* Close UART 1 */
                ROM_UARTDisable( UART1_BASE );
                /* Force RS485 back to receive mode */
                RS_485_UART_1_DISABLE_TX(  );
                /* Reset handle */
                HDL_RESET( pxSerialIntHdl );
                /* No error */
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_EAGAIN;
            }
            break;
#endif
        default:
            MBP_ASSERT( 0 );
            break;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialTxEnable( xMBPSerialHandle xSerialHdl, pbMBPSerialTransmitterEmptyCB pbMBMTransmitterEmptyFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pbMBMTransmitterEmptyFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pbMBMTransmitterEmptyFN );
            pxSerialIntHdl->pbMBMTransmitterEmptyFN = ( pbMBPSerialTransmitterEmptyAPIV1CB ) pbMBMTransmitterEmptyFN;
            switch ( pxSerialIntHdl->ubIdx )
            {
#if UART_0_ENABLED == 1
            case UART_0_IDX:
                /* RS485 transmit mode */
                RS_485_UART_0_ENABLE_TX(  );
                /* Enable UART 0 TX interrupt */
                ROM_UARTIntEnable( UART0_BASE, UART_INT_TX );
                /* Set software interrupt and trigger it. */
                ulUART0SoftIntStatus = UART_MIS_TXMIS;
                HWREG( NVIC_SW_TRIG ) = INT_UART0 - 16;
                break;
#endif
#if UART_1_ENABLED == 1
            case UART_1_IDX:
                /* RS485 transmit mode */
                RS_485_UART_1_ENABLE_TX(  );
                /* Enable USART 1 TX interrupt */
                ROM_UARTIntEnable( UART1_BASE, UART_INT_TX );
                /* Set software interrupt and trigger it. */
                ulUART1SoftIntStatus = UART_MIS_TXMIS;
                HWREG( NVIC_SW_TRIG ) = INT_UART1 - 16;
                break;
#endif
            default:
                MBP_ASSERT( 0 );
            }

        }
        else
        {
            pxSerialIntHdl->pbMBMTransmitterEmptyFN = NULL;
            /* The transmitter is disable when the last frame has been sent.
             * This is necessary for RS485 with a half-duplex bus.
             */
            switch ( pxSerialIntHdl->ubIdx )
            {
#if UART_0_ENABLED == 1
            case UART_0_IDX:
                /* Disable UART 0 TX interrupt */
                ROM_UARTIntDisable( UART0_BASE, UART_INT_TX );
                /* Enable UART 0 receive timeout interrupt. */
                ROM_UARTIntEnable( UART0_BASE, UART_INT_RT );
                break;
#endif
#if UART_1_ENABLED == 1
            case UART_1_IDX:
                /* Disable UART 1 TX interrupt */
                ROM_UARTIntDisable( UART1_BASE, UART_INT_TX );
                /* Enable UART 1 receive timeout interrupt. */
                ROM_UARTIntEnable( UART1_BASE, UART_INT_RT );
                break;
#endif
            default:
                MBP_ASSERT( 0 );
            }
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBMReceiveFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pvMBMReceiveFN )
        {
            MBP_ASSERT( NULL == pxSerialIntHdl->pvMBMReceiveFN );
            pxSerialIntHdl->pvMBMReceiveFN = ( pvMBPSerialReceiverAPIV1CB ) pvMBMReceiveFN;
            switch ( pxSerialIntHdl->ubIdx )
            {
#if UART_0_ENABLED == 1
            case UART_0_IDX:
                /* Enable UART 0 receive interrupt */
                RS_485_UART_0_DISABLE_TX(  );
                ROM_UARTIntEnable( UART0_BASE, UART_INT_RX );
                break;
#endif
#if UART_1_ENABLED == 1
            case UART_1_IDX:
                /* Enable UART 1 receive interrupt */
                RS_485_UART_1_DISABLE_TX(  );
                ROM_UARTIntEnable( UART1_BASE, UART_INT_RX );
                break;
#endif
            default:
                MBP_ASSERT( 0 );
            }
        }
        else
        {
            pxSerialIntHdl->pvMBMReceiveFN = NULL;
            switch ( pxSerialIntHdl->ubIdx )
            {
#if UART_0_ENABLED == 1
            case UART_0_IDX:
                /* Disable UART 0 receive interrupt */
                ROM_UARTIntDisable( UART0_BASE, UART_INT_RX );
                break;
#endif
#if UART_1_ENABLED == 1
            case UART_1_IDX:
                /* Disable UART 1 receive interrupt */
                ROM_UARTIntDisable( UART0_BASE, UART_INT_RX );
                break;
#endif
            default:
                MBP_ASSERT( 0 );
            }
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

#if UART_0_ENABLED == 1
void
vMBPUART0ISR( void )
{
    ULONG           ulIntStatus;
    BOOL            bHasMoreData = FALSE;
    UBYTE           ubBuffer;

    /* Get the interrupt status and combine it with the software
     * interrupt status.
     */
    ulIntStatus = ROM_UARTIntStatus( UART0_BASE, true );
    ulIntStatus |= ulUART0SoftIntStatus;
    ulUART1SoftIntStatus = 0;

    ROM_UARTIntClear( UART0_BASE, ulIntStatus );

    if( ulIntStatus & UART_MIS_TXMIS )
    {
        if( NULL != xSerialHdls[UART_0_IDX].pbMBMTransmitterEmptyFN )
        {
            bHasMoreData = xSerialHdls[UART_0_IDX].pbMBMTransmitterEmptyFN( xSerialHdls[UART_0_IDX].xMBHdl, &ubBuffer );
        }
        if( !bHasMoreData )
        {
            /* Disable UART 0 TX interrupt */
            ROM_UARTIntDisable( UART0_BASE, UART_INT_TX );
            /* Disable the callback. */
            xSerialHdls[UART_0_IDX].pbMBMTransmitterEmptyFN = NULL;
        }
        else
        {
            HWREG( UART0_BASE + UART_O_DR ) = ubBuffer;
        }
    }
    if( ulIntStatus & UART_MIS_RXMIS )
    {
        ubBuffer = HWREG( UART0_BASE + UART_O_DR );
        if( HWREG( UART0_BASE + UART_O_RSR ) != 0 )
        {
            /* Mark this character as invalid. Still the character should
             * be passed to the MODBUS stack to allow for correct end of
             * frame timeouts.
             */
            ubBuffer = 0xFF;
            /* Any write removes all bits. */
            HWREG( UART0_BASE + UART_O_ECR ) = 0;
        }
        if( NULL != xSerialHdls[UART_0_IDX].pvMBMReceiveFN )
        {
            xSerialHdls[UART_0_IDX].pvMBMReceiveFN( xSerialHdls[UART_0_IDX].xMBHdl, ubBuffer );
        }
    }
}
#endif

#if UART_1_ENABLED == 1
void
vMBPUART1ISR( void )
{
    ULONG           ulIntStatus;
    BOOL            bHasMoreData = FALSE;
    UBYTE           ubBuffer;

    /* Get the interrupt status and combine it with the software
     * interrupt status.
     */
    ulIntStatus = ROM_UARTIntStatus( UART1_BASE, true );
    ulIntStatus |= ulUART1SoftIntStatus;
    ulUART1SoftIntStatus = 0;

    ROM_UARTIntClear( UART1_BASE, ulIntStatus );

    if( ulIntStatus & UART_MIS_TXMIS )
    {
        if( NULL != xSerialHdls[UART_1_IDX].pbMBMTransmitterEmptyFN )
        {
            bHasMoreData = xSerialHdls[UART_1_IDX].pbMBMTransmitterEmptyFN( xSerialHdls[UART_1_IDX].xMBHdl, &ubBuffer );
        }
        if( !bHasMoreData )
        {
            /* Disable USART 1 TX interrupt */
            ROM_UARTIntDisable( UART1_BASE, UART_INT_TX );
            /* Disable the callback. */
            xSerialHdls[UART_1_IDX].pbMBMTransmitterEmptyFN = NULL;
        }
        else
        {
            HWREG( UART1_BASE + UART_O_DR ) = ubBuffer;
        }
    }
    if( ulIntStatus & UART_MIS_RXMIS )
    {
        ubBuffer = HWREG( UART1_BASE + UART_O_DR );
        if( HWREG( UART1_BASE + UART_O_RSR ) != 0 )
        {
            /* Mark this character as invalid. Still the character should
             * be passed to the MODBUS stack to allow for correct end of
             * frame timeouts.
             */
            ubBuffer = 0xFF;
            /* Any write removes all bits. */
            HWREG( UART1_BASE + UART_O_ECR ) = 0;
        }
        if( NULL != xSerialHdls[UART_1_IDX].pvMBMReceiveFN )
        {
            xSerialHdls[UART_1_IDX].pvMBMReceiveFN( xSerialHdls[UART_1_IDX].xMBHdl, ubBuffer );
        }
    }
}
#endif
