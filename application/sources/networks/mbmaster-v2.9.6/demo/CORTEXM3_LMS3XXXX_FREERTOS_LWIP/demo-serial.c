/*
 * MODBUS Library: Luminary Cortex M3, FreeRTOS and RS485
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-serial.c,v 1.2 2010-04-25 13:48:16 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_ints.h>
#include <inc/hw_sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* ----------------------- Platform includes --------------------------------*/
#include "port/mbport.h"
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Modbus includes ----------------------------------*/

/* ----------------------- Defines ------------------------------------------*/
#define APP2_TASK_PRIORITY              ( tskIDLE_PRIORITY + 2 )
#define APP2_TASK_STACKSIZE             ( configMINIMAL_STACK_SIZE  )
#define APP3_TASK_PRIORITY              ( tskIDLE_PRIORITY + 2 )
#define APP3_TASK_STACKSIZE             ( 256 )

#define MBM_SERIAL_PORT			        ( 0 )
#define MBM_SERIAL_BAUDRATE		        ( 57600 )
#define MBM_SERIAL_PARITY		        ( MB_PAR_NONE )

#define TEST1_SLAVE_ADDRESS             ( 10 )
#define TEST1_HOLDING_REG_START         ( 512 )
#define TEST1_HOLDING_REG_LENGTH        ( 10 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- External prototypes ------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
void            vApp2Task( void *pvParameters );
void            vApp3Task( void *pvParameters );

/* ----------------------- Start implementation -----------------------------*/


void
vTaskSwitchIn( void )
{
}

int
main( void )
{
    if( DEVICE_IS_REVA2 )
    {
        SysCtlLDOSet( SYSCTL_LDO_2_75V );
    }
    /* Set the clocking to run from the PLL at 50 MHz */
    SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );

#if defined( PART_LM3S6965 ) || defined( PART_LM3S8962 )
    SysCtlPeripheralEnable( SYSCTL_PERIPH_UART0 );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_UART1 );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_UART2 );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOA );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOB );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOC );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOD );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOE );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOF );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOG );
#elif defined( PART_LM3S818 )
    SysCtlPeripheralEnable( SYSCTL_PERIPH_UART0 );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_UART1 );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOA );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOB );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOC );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOD );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOE );
#else
#error "unsupported part"
#endif

    if( pdTRUE != xTaskCreate( vApp2Task, ( signed portCHAR * )"APP2", APP2_TASK_STACKSIZE, NULL, APP2_TASK_PRIORITY,
                               NULL ) )
    {
    }
    else if( pdTRUE != xTaskCreate( vApp3Task, "MODBUS", APP3_TASK_STACKSIZE, NULL, APP3_TASK_PRIORITY, NULL ) )
    {
    }
    else
    {
        vMBPInit(  );
        vTaskStartScheduler(  );
    }
    return 0;
}


void
vApp2Task( void *pvParameters )
{
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    static char     arubBuffer[200];
#endif
    for( ;; )
    {
        vTaskDelay( 5000 );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        vTaskList( arubBuffer );
        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "%s", arubBuffer );
#endif
    }
}

void
vApp3Task( void *pvParameters )
{
    xMBMHandle      xMBMHdl;
    eMBErrorCode    eStatus, eStatus2;
    USHORT          usNRegs[10];
    int             iReadCnt;

    for( ;; )
    {
        eStatus = MB_ENOERR;
        if( MB_ENOERR !=
            ( eStatus2 = eMBMSerialInit( &xMBMHdl, MB_RTU, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_SERIAL_PARITY ) ) )
        {
            eStatus = eStatus2;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "Can not initialize MODBUS stack: %d!\n", ( int )eStatus );
#endif
        }
        else
        {
            iReadCnt = 2;
            do
            {
                if( MB_ENOERR !=
                    ( eStatus2 =
                      eMBMReadHoldingRegisters( xMBMHdl, TEST1_SLAVE_ADDRESS, TEST1_HOLDING_REG_START,
                                                TEST1_HOLDING_REG_LENGTH, usNRegs ) ) )
                {
                    eStatus = eStatus2;                  
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER,
                                "Reading of holding registers (start=%hu, length=%hu) from slave %hu failed: %d!\n",
                                 ( USHORT ) TEST1_HOLDING_REG_START, ( USHORT ) TEST1_HOLDING_REG_LENGTH,
                                 ( USHORT ) TEST1_SLAVE_ADDRESS, ( int )eStatus );
#endif

                }
                iReadCnt--;
            }
            while( iReadCnt > 0 );
            if( MB_ENOERR != ( eStatus2 = eMBMClose( xMBMHdl ) ) )
            {
                eStatus = eStatus2;              
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "Can not close MODBUS instance: %d !\n", ( int )eStatus );
#endif
            }
        }
        if( eStatus != MB_ENOERR )
        {
            /* Give other tasks a chance to run. This is necessary since
             * the threads take some time to close themself. Otherwise
             * the system won't crash but there won't be any resources
             * available and the initialization would fail all the time.
             */
            vTaskDelay( 500 );
        }
    }

}

void
vApplicationStackOverflowHook( xTaskHandle * pxTask, signed portCHAR * pcTaskName )
{
    ( void )IntMasterDisable(  );
    /* Let the watchdog trigger a reset here. */
    for( ;; );
}
