/* 
 * MODBUS Slave Library: A portable MODBUS slave for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: demo.c,v 1.1 2010-05-22 22:31:29 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

/* ----------------------- Platform includes --------------------------------*/
#include "AT91SAM7X256.h"
#include "lib_AT91SAM7X256.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "port/mbport.h"
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define MODBUS_TASK_PRIORITY            ( tskIDLE_PRIORITY + 1 )
#define MODBUS_TASK_STACKSIZE           ( 256 )
#define APPLICATION_TASK_PRIORITY       ( tskIDLE_PRIORITY + 2 )
#define APPLICATION_TASK_STACKSIZE      ( configMINIMAL_STACK_SIZE )

#define MBM_SERIAL_PORT                 ( 0 )
#define MBM_SERIAL_BAUDRATE             ( 38400 )
#define MBM_PARITY                      ( MB_PAR_NONE )

#define DEBUG_LED_ERROR                 ( 0 )
#define DEBUG_LED_WORKING               ( 1 )
#define LED0_PIN                        AT91C_PIO_PB20
#define LED0_BASE                       AT91C_BASE_PIOB
#define LED1_PIN                        AT91C_PIO_PB21
#define LED1_BASE                       AT91C_BASE_PIOB

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vAppModbusTask( void *pvParameters );
STATIC void     vAppApplicationTask( void *pvParameters );
STATIC void     vSetupHardware( void );
STATIC void     vMBPTestDebugLED( UBYTE ubIdx, BOOL bTurnOn );


/* ----------------------- Start implementation -----------------------------*/
int
main( int argc, char *argv[] )
{
    vSetupHardware(  );

    if( pdPASS !=
        xTaskCreate( vAppApplicationTask, "APPLICATION", APPLICATION_TASK_STACKSIZE, NULL, APPLICATION_TASK_PRIORITY,
                     NULL ) )
    {
    }
    else if( pdPASS !=
             xTaskCreate( vAppModbusTask, "MODBUS", MODBUS_TASK_STACKSIZE, NULL, MODBUS_TASK_STACKSIZE, NULL ) )
    {
    }
    else
    {
        vTaskStartScheduler(  );
    }
}

void
vSetupHardware( void )
{

    vMBPInit(  );

    /* Enable the peripheral clock. */
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA;
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOB;

    /* Setup the LED pins. */
    AT91F_PIO_CfgOutput( LED0_BASE, LED0_PIN );
    AT91F_PIO_SetOutput( LED0_BASE, LED0_PIN );
    AT91F_PIO_CfgOutput( LED1_BASE, LED1_PIN );
    AT91F_PIO_SetOutput( LED1_BASE, LED1_PIN );
}

void
vAppApplicationTask( void *pvParameters )
{
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    static char     arubBuffer[200];
#endif
    for( ;; )
    {
        vTaskDelay( 2000 );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        vTaskList( arubBuffer );
        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "%s", arubBuffer );
#endif
    }
}

void
vAppModbusTask( void *pvParameters )
{
    USHORT          usRegCnt = 0;
    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[5];

    if( MB_ENOERR ==
        ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MBM_SERIAL_PORT, MBM_SERIAL_BAUDRATE, MBM_PARITY ) ) )
    {
        do
        {
#if 1 
            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
            {
                eStatus = eStatus2;
            }
            vTaskDelay( 100 );
            /* Read holding register from adress 5 - 10, increment them by one and store
             * them at address 10. 
             */
            if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 5, 5, usNRegs ) ) )
            {
                eStatus = eStatus2;
            }
#endif            
            switch ( eStatus )
            {
            case MB_ENOERR:
                vMBPTestDebugLED( DEBUG_LED_WORKING, TRUE );
                vMBPTestDebugLED( DEBUG_LED_ERROR, FALSE );
                break;
            default:
                vMBPTestDebugLED( DEBUG_LED_ERROR, TRUE );
                vMBPTestDebugLED( DEBUG_LED_WORKING, FALSE );
                break;
            }
             vTaskDelay( 1 );
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
}

void
vMBPTestDebugLED( UBYTE ubIdx, BOOL bTurnOn )
{
    switch ( ubIdx )
    {
    case DEBUG_LED_ERROR:
        if( bTurnOn )
        {
            AT91F_PIO_ClearOutput( LED0_BASE, LED0_PIN );
        }
        else
        {
            AT91F_PIO_SetOutput( LED0_BASE, LED0_PIN );
        }
        break;

    case DEBUG_LED_WORKING:
        if( bTurnOn )
        {
            AT91F_PIO_ClearOutput( LED0_BASE, LED1_PIN );
        }
        else
        {
            AT91F_PIO_SetOutput( LED0_BASE, LED1_PIN );
        }
        break;

    default:
        break;
    }
}

void 
vApplicationStackOverflowHook( xTaskHandle *pxTask, signed portCHAR *pcTaskName )
{
    vMBPAssert(  );    
}
