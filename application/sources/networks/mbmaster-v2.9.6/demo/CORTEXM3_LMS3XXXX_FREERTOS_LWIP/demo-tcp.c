/*
 * MODBUS Library: Luminary Cortex M3, FreeRTOS and lwIP Example
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-tcp.c,v 1.2 2010-04-25 10:01:54 embedded-so.embedded-solutions.1 Exp $
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
#include "lwip/api.h"

/* ----------------------- Platform includes --------------------------------*/
#include "netif/LWIPStack.h"
#include "netif/ETHIsr.h"
#include "utils/uartstdio.h"
#include "port/mbport.h"
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Modbus includes ----------------------------------*/

/* ----------------------- Defines ------------------------------------------*/
#define APP1_TASK_PRIORITY              ( tskIDLE_PRIORITY + 2 )
#define APP1_TASK_STACKSIZE             ( configMINIMAL_STACK_SIZE )
#define APP2_TASK_PRIORITY              ( tskIDLE_PRIORITY + 2 )
#define APP2_TASK_STACKSIZE             ( 256 )
#define APP3_TASK_PRIORITY              ( tskIDLE_PRIORITY + 2 )
#define APP3_TASK_STACKSIZE             ( 384 )

#define TEST1_TCP_SLAVE                 "192.168.100.100"
#define TEST1_TCP_PORT                  502
#define TEST1_SLAVE_ADDRESS             ( 1 )
#define TEST1_HOLDING_REG_START         ( 0 )
#define TEST1_HOLDING_REG_LENGTH        ( 10 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- External prototypes ------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
void            vApp1Task( void *pvParameters );
void            vApp2Task( void *pvParameters );
void            vApp3Task( void *pvParameters );

/* ----------------------- Start implementation -----------------------------*/

int
main( void )
{
    if( DEVICE_IS_REVA2 )
    {
        SysCtlLDOSet( SYSCTL_LDO_2_75V );
    }
    /* Set the clocking to run from the PLL at 50 MHz */
    SysCtlClockSet( SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOF );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_UART0 );
    SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOA );

    if( pdTRUE !=
        xTaskCreate( vApp1Task, ( signed portCHAR * )"APP1", APP1_TASK_STACKSIZE, NULL, APP1_TASK_PRIORITY, NULL ) )
    {
    }
    else if( pdTRUE !=
             xTaskCreate( vApp2Task, ( signed portCHAR * )"APP2", APP2_TASK_STACKSIZE, NULL, APP2_TASK_PRIORITY,
                          NULL ) )
    {
    }
    else if( NULL == sys_thread_new( "MODBUS", vApp3Task, NULL, APP3_TASK_STACKSIZE, APP3_TASK_PRIORITY ) )
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
vApp1Task( void *pvParameters )
{
    for( ;; )
    {
        vTaskDelay( 1000 );
    }
}

void
vApp2Task( void *pvParameters )
{
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    static signed char arubBuffer[512];
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
    IP_CONFIG       ipconfig;
    xMBMHandle      xMBMHdl;
    eMBErrorCode    eStatus, eStatus2;
    USHORT          usNRegs[10];
    int             iConnectCnt;
    int             iReadCnt;

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER, "Starting lwIP\n" );
#endif

    ETHServiceTaskInit( 0 );
    ETHServiceTaskFlush( 0, ETH_FLUSH_RX | ETH_FLUSH_TX );
    ipconfig.IPMode = IPADDR_USE_STATIC;
    ipconfig.IPAddr = 0xC0A86465UL;
    ipconfig.NetMask = 0xFFFFFF00UL;
    ipconfig.GWAddr = 0x00000000UL;
    LWIPServiceTaskInit( &ipconfig );

    for( ;; )
    {
        eStatus = MB_ENOERR;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "Initializing MODBUS stack!\n" );
#endif
        if( MB_ENOERR != ( eStatus2 = eMBMTCPInit( &xMBMHdl ) ) )
        {
            eStatus = eStatus2;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "Can not initialize MODBUS stack: %d!\n", eStatus );
#endif
        }
        else
        {
            iConnectCnt = 10;
            do
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "Connecting to MODBUS slave!\n" );
#endif
                if( MB_ENOERR == ( eStatus = eMBMTCPConnect( xMBMHdl, TEST1_TCP_SLAVE, TEST1_TCP_PORT ) ) )
                {
                    iReadCnt = 2;
                    do
                    {
                        if( MB_ENOERR !=
                            ( eStatus =
                              eMBMReadHoldingRegisters( xMBMHdl, TEST1_SLAVE_ADDRESS, TEST1_HOLDING_REG_START,
                                                        TEST1_HOLDING_REG_LENGTH, usNRegs ) ) )
                        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER,
                                         "Reading of holding registers (start=%hu, length=%hu) from slave %hu failed!\n",
                                         ( USHORT ) TEST1_HOLDING_REG_START, ( USHORT ) TEST1_HOLDING_REG_LENGTH,
                                         ( USHORT ) TEST1_SLAVE_ADDRESS );
#endif
                            eStatus = eStatus2;
                        }
                        iReadCnt--;
                    }
                    while( iReadCnt > 0 );
                    if( MB_ENOERR != ( eStatus = eMBMTCPDisconnect( xMBMHdl ) ) )
                    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "Can not disconnect from slave!\n" );
#endif
                        eStatus = eStatus2;
                    }
                }
                else
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "Can not connect to MODBUS slave %s:%hu!\n",
                                 TEST1_TCP_SLAVE, ( USHORT ) TEST1_TCP_PORT );
#endif
                    eStatus = eStatus2;
                }
                iConnectCnt--;
            }
            while( ( MB_ENOERR == eStatus ) && ( iConnectCnt > 0 ) );
            if( MB_ENOERR != ( eStatus = eMBMClose( xMBMHdl ) ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "Can not close MODBUS instance!\n" );
#endif
                eStatus = eStatus2;
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
