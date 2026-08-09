/* 
 * MODBUS Library: STM32 port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo.c,v 1.2 2009-01-01 23:37:23 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBM_SERIAL_PORT           ( MB_UART_2 )
#define MBM_SERIAL_BAUDRATE       ( 9600 )
#define MBM_PARITY                ( MB_PAR_NONE )

#define DEBUG_LED_ERROR           ( 0 )
#define DEBUG_LED_WORKING         ( 1 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
STATIC ErrorStatus HSEStartUpStatus;
const u32       _Main_Crystal = 8000000;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vRCCConfiguration( void );
STATIC void     vNVICConfiguration( void );
STATIC void     vGPIOConfiguration( void );
STATIC void     vIOSetLED( UBYTE ubIdx, BOOL bTurnOn );

/* ----------------------- Start implementation -----------------------------*/

int
main( int argc, char *argv[] )
{

    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[5];
    USHORT          usRegCnt = 0;
    UBYTE           ubIdx;
    UBYTE           ubCnt;

#if defined( LIBDEBUG ) & ( LIBDEBUG == 1 )
    libdebug(  );
#endif
    vRCCConfiguration(  );
    vGPIOConfiguration(  );
    vNVICConfiguration(  );

    if( MB_ENOERR ==
        ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
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

    return 0;
}

void
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

STATIC void
vRCCConfiguration( void )
{
    /* RCC system reset(for debug purpose) */
    RCC_DeInit(  );

    /* Enable HSE */
    RCC_HSEConfig( RCC_HSE_ON );

    /* Wait till HSE is ready */
    HSEStartUpStatus = RCC_WaitForHSEStartUp(  );

    if( HSEStartUpStatus == SUCCESS )
    {
        /* HCLK = SYSCLK */
        RCC_HCLKConfig( RCC_SYSCLK_Div1 );
        /* PCLK2 = HCLK */
        RCC_PCLK2Config( RCC_HCLK_Div1 );
        /* PCLK1 = HCLK/2 */
        RCC_PCLK1Config( RCC_HCLK_Div2 );
        /* Flash 2 wait state */
        FLASH_SetLatency( FLASH_Latency_2 );
        /* Enable Prefetch Buffer */
        FLASH_PrefetchBufferCmd( FLASH_PrefetchBuffer_Enable );
        /* PLLCLK = 8MHz * 9 = 72 MHz */
        RCC_PLLConfig( RCC_PLLSource_HSE_Div1, RCC_PLLMul_9 );
        /* Enable PLL */
        RCC_PLLCmd( ENABLE );
        /* Wait till PLL is ready */
        while( RCC_GetFlagStatus( RCC_FLAG_PLLRDY ) == RESET )
        {
        }

        /* Select PLL as system clock source */
        RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK );

        /* Wait till PLL is used as system clock source */
        while( RCC_GetSYSCLKSource(  ) != 0x08 )
        {
        }
    }

    /* clocks */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_USART1, ENABLE );
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM3 | RCC_APB1Periph_USART2, ENABLE );
}

STATIC void
vNVICConfiguration( void )
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_SetVectorTable( NVIC_VectTab_FLASH, 0x0 );

    /* Enable USART1 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQChannel;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MB_PREEMP_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    /* Enable USART2 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQChannel;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MB_PREEMP_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    /* Enable USART2 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQChannel;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = MB_PREEMP_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init( &NVIC_InitStructure );
}

STATIC void
vGPIOConfiguration( void )
{
    GPIO_InitTypeDef GPIO_InitStructure;
    volatile int    i;

    /* Configure all unused GPIO port pins in Analog Input mode (floating input */
    /* trigger OFF), this will reduce the power consumption and increase the device */
    /* immunity against EMI/EMC */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                            RCC_APB2Periph_GPIOE, ENABLE );
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init( GPIOA, &GPIO_InitStructure );
    GPIO_Init( GPIOB, &GPIO_InitStructure );
    GPIO_Init( GPIOC, &GPIO_InitStructure );
    GPIO_Init( GPIOD, &GPIO_InitStructure );
    GPIO_Init( GPIOE, &GPIO_InitStructure );
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                            RCC_APB2Periph_GPIOE, DISABLE );

    /* Enable clocks on GPIOA and GPIOC */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE );

    /* Configure USART1 Tx (PA9) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init( GPIOA, &GPIO_InitStructure );
    /* Configure USART1 Rx (PA10) as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

    /* Configure USART2 Tx (PA.2) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init( GPIOA, &GPIO_InitStructure );
    /* Configure USART2 Rx (PA.3) as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init( GPIOA, &GPIO_InitStructure );

}
