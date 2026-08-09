/*
 * MODBUS Library: NXP Cortex M3, FreeRTOS and lwIP Example
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: main.c,v 1.1 2011-01-02 16:31:44 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "LPC17xx.h"

#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "ethernetif/LPC17xx/lpc17xx_netif.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "port/mbport.h"
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define DEBUG_STACK                     ( 0 )

#define APP1_TASK_PRIORITY              ( tskIDLE_PRIORITY + 2 )
#define APP1_TASK_STACKSIZE             ( 256 )
#define APP2_TASK_PRIORITY              ( tskIDLE_PRIORITY + 2 )
#define APP2_TASK_STACKSIZE             ( 196 )
#define APP3_TASK_PRIORITY              ( tskIDLE_PRIORITY + 2 )
#define APP3_TASK_STACKSIZE             ( 196 )

#define TEST1_TCP_SLAVE                 "192.168.0.200"
#define TEST1_TCP_PORT                  502
#define TEST1_SLAVE_ADDRESS             ( 1 )
#define TEST1_HOLDING_REG_START         ( 0 )
#define TEST1_HOLDING_REG_LENGTH        ( 10 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
STATIC struct netif netif;

/* ----------------------- External prototypes ------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
void vApp3Task( void *pvParameters );
void vApp2Task( void *pvParameters );
void vApp1Task( void *pvParameters );
void LwIP_Init( void );

/* ----------------------- Start implementation -----------------------------*/

int main(void)
{
    /* Call SystemCoreClockUpdate such that SystemCoreClock holds correct
     * value.
     */
    SystemCoreClockUpdate(  );

    if( NULL == sys_thread_new( "MODBUS", vApp1Task, NULL, APP1_TASK_STACKSIZE, APP1_TASK_PRIORITY ) )
    {
    }
    else if( pdTRUE != xTaskCreate( vApp2Task, ( signed portCHAR * )"APP2", APP2_TASK_STACKSIZE, NULL, APP2_TASK_PRIORITY,
                               NULL ) )
    {
    }
    else if( pdTRUE != xTaskCreate( vApp3Task, ( signed portCHAR * )"APP3", APP3_TASK_STACKSIZE, NULL, APP3_TASK_PRIORITY,
                               NULL ) )
    {
    }
    else
    {
    	vMBPInit(  );
        vTaskStartScheduler(  );
    }
	return 0 ;
}

void
vApp1Task( void *pvParameters )
{
    xMBMHandle      xMBMHdl;
    eMBErrorCode    eStatus, eStatus2;
    USHORT          usNRegs[10];
    int             iConnectCnt;
    int             iReadCnt;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_OTHER, "Starting lwIP\n" );
#endif
    LwIP_Init(  );
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
vApp2Task( void *pvParameters )
{
    xMBMHandle      xMBMHdl;
    eMBErrorCode    eStatus, eStatus2;
    USHORT          usNRegs[10];
    int             iReadCnt;

    for( ;; )
    {
        eStatus = MB_ENOERR;
        if( MB_ENOERR !=
            ( eStatus2 = eMBMSerialInit( &xMBMHdl, MB_RTU, 0, 38400, MB_PAR_NONE ) ) )
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
vApp3Task( void *pvParameters )
{
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
#if defined( DEBUG_STACK ) && ( DEBUG_STACK == 1 )
    static char     arubBuffer[200];
#endif
#endif
    for( ;; )
    {
        vMBPSetLed( 0, FALSE );
        vTaskDelay( 2500 );
        vMBPSetLed( 0, TRUE );
        vTaskDelay( 2500 );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
#if defined( DEBUG_STACK ) && ( DEBUG_STACK == 1 )
        vTaskList( arubBuffer );
        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "%s", arubBuffer );
#endif
#endif
    }
}

void LwIP_Init( void )
{
    struct ip_addr  ipaddr;
    struct ip_addr  netmask;
    struct ip_addr  gw;

    tcpip_init( NULL, NULL );

    IP4_ADDR( &ipaddr, 192, 168, 0, 8 );
    IP4_ADDR( &netmask, 255, 255, 255, 0 );
    IP4_ADDR( &gw, 192, 168, 0, 1 );
    netif_add( &netif, &ipaddr, &netmask, &gw, NULL, &lpc17xx_netif_init, &tcpip_input );
    netif_set_default( &netif );
    netif_set_up( &netif );
    while( 0 == netif_is_up( &netif ) )
    {
        vTaskDelay( 5000 / portTICK_RATE_MS );
    }
}

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
	/* This function will get called if a task overflows its stack. */

	( void ) pxTask;
	( void ) pcTaskName;

	for( ;; );
}
