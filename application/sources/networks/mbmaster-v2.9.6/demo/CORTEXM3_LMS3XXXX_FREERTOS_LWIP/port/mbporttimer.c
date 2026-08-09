/* 
 * MODBUS Library: Luminary Cortex M3, FreeRTOS and lwIP Example
 * Copyright (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbporttimer.c,v 1.5 2010-06-13 17:04:48 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <FreeRTOS.h>
#include <task.h>
#if defined( WITH_LWIP )  
#include "lwip/api.h"
#endif

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define TIMER_TASK_PRIORITY             ( MBP_TASK_PRIORITY )
#define TIMER_TASK_STACKSIZE            ( 128 )
#define TIMER_TICKRATE_MS               ( 10 / portTICK_RATE_MS )
#define MAX_TIMER_HDLS                  ( 3 )
#define IDX_INVALID                     ( 255 )

#define RESET_HDL( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->bIsRunning = FALSE; \
    ( x )->xNTimeoutTicks = 0; \
    ( x )->xNExpiryTimeTicks = 0; \
    ( x )->xMBMHdl = MB_HDL_INVALID; \
    ( x )->pbMBPTimerExpiredFN = NULL; \
} while( 0 );

#ifndef MBP_TIMER_DEBUG
#define MBP_TIMER_DEBUG                    ( 0 )
#endif

#if defined( MBP_TIMER_DEBUG ) && ( MBP_TIMER_DEBUG == 1 )
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/gpio.h>

#if defined( PART_LM3S6965 ) 
#define DEBUG_INIT                          do { \
    GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_0, 0x00 ); \
    GPIOPinTypeGPIOOutput( GPIO_PORTF_BASE, GPIO_PIN_0 ); \
    GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_0, 0x00 ); \
    GPIOPinTypeGPIOOutput( GPIO_PORTB_BASE, GPIO_PIN_0 ); \
} while( 0 )

#define DEBUG_TIMEREXPIRED_ON               do { \
    GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_0, 0xFF ); \
} while( 0 )

#define DEBUG_TIMEREXPIRED_OFF              do { \
    GPIOPinWrite( GPIO_PORTF_BASE, GPIO_PIN_0, 0x00 ); \
} while( 0 )

#define DEBUG_TIMERSTART_ON                 do { \
    GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_0, 0xFF ); \
} while( 0 )

#define DEBUG_TIMERSTART_OFF                do { \
    GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_0, 0x00 ); \
} while( 0 )
#else
#warning "No debug definitions for this part. Consider adding them to mbportimer.c"
#define DEBUG_INIT
#define DEBUG_TIMEREXPIRED_ON 
#define DEBUG_TIMEREXPIRED_OFF
#define DEBUG_TIMERSTART_ON
#define DEBUG_TIMERSTART_OFF
#endif
#else
#define DEBUG_INIT
#define DEBUG_TIMEREXPIRED_ON 
#define DEBUG_TIMEREXPIRED_OFF
#define DEBUG_TIMERSTART_ON
#define DEBUG_TIMERSTART_OFF
#endif

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    BOOL            bIsRunning;
    portTickType    xNTimeoutTicks;
    portTickType    xNExpiryTimeTicks;
    xMBHandle       xMBMHdl;
    pbMBPTimerExpiredCB pbMBPTimerExpiredFN;
} xTimerInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xTimerInternalHandle arxTimerHdls[MAX_TIMER_HDLS];

STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
void            vMBPTimerTask( void *pvParameters );

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPTimerInit( xMBPTimerHandle * xTimerHdl, USHORT usTimeOut1ms,
               pbMBPTimerExpiredCB pbMBPTimerExpiredFN, xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EPORTERR;
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( ( NULL != xTimerHdl ) && ( NULL != pbMBPTimerExpiredFN ) && ( MB_HDL_INVALID != xHdl ) )
    {
        if( !bIsInitalized )
        {
            DEBUG_INIT;
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
            {
                RESET_HDL( &arxTimerHdls[ubIdx] );
            }
#if defined( WITH_LWIP )            
            if( NULL == sys_thread_new( "MBP-TIMER", vMBPTimerTask, NULL, TIMER_TASK_STACKSIZE, TIMER_TASK_PRIORITY ) )
            {
                eStatus = MB_EPORTERR;
            }
#else          
            if( pdPASS != xTaskCreate( vMBPTimerTask, "MBP-TIMER", TIMER_TASK_STACKSIZE, NULL, TIMER_TASK_PRIORITY, NULL ) )
            {
                eStatus = MB_EPORTERR;
            }
#endif            
            else
            {
                bIsInitalized = TRUE;
            }
        }
        if( bIsInitalized )
        {
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
            {
                if( IDX_INVALID == arxTimerHdls[ubIdx].ubIdx )
                {
                    break;
                }
            }
            if( MAX_TIMER_HDLS != ubIdx )
            {
                arxTimerHdls[ubIdx].ubIdx = ubIdx;
                arxTimerHdls[ubIdx].bIsRunning = FALSE;
                arxTimerHdls[ubIdx].xNTimeoutTicks = MB_INTDIV_CEIL( ( portTickType ) usTimeOut1ms, portTICK_RATE_MS );
                arxTimerHdls[ubIdx].xNExpiryTimeTicks = 0;
                arxTimerHdls[ubIdx].xMBMHdl = xHdl;
                arxTimerHdls[ubIdx].pbMBPTimerExpiredFN = pbMBPTimerExpiredFN;

                *xTimerHdl = &arxTimerHdls[ubIdx];
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

void
vMBPTimerClose( xMBPTimerHandle xTimerHdl )
{
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        RESET_HDL( pxTimerIntHdl );
    }
}

eMBErrorCode
eMBPTimerSetTimeout( xMBPTimerHandle xTimerHdl, USHORT usTimeOut1ms )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) && ( usTimeOut1ms > 0 ) )
    {
        pxTimerIntHdl->xNTimeoutTicks = MB_INTDIV_CEIL( ( portTickType ) usTimeOut1ms, portTICK_RATE_MS );
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPTimerStart( xMBPTimerHandle xTimerHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;

    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    DEBUG_TIMERSTART_ON;
    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        pxTimerIntHdl->bIsRunning = TRUE;
        pxTimerIntHdl->xNExpiryTimeTicks = xTaskGetTickCount(  );
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    DEBUG_TIMERSTART_OFF;
    return eStatus;
}

eMBErrorCode
eMBPTimerStop( xMBPTimerHandle xTimerHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        pxTimerIntHdl->bIsRunning = FALSE;
        pxTimerIntHdl->xNExpiryTimeTicks = 0;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

void
vMBPTimerTask( void *pvParameters )
{
    UBYTE           ubIdx;
    xTimerInternalHandle *pxTmrHdl;
    portTickType    xLastWakeTime;
    portTickType    xCurrentTime;

    xLastWakeTime = xTaskGetTickCount(  );
    for( ;; )
    {
        vTaskDelayUntil( &xLastWakeTime, ( portTickType ) TIMER_TICKRATE_MS );
        xCurrentTime = xTaskGetTickCount(  );
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
        {
            pxTmrHdl = &arxTimerHdls[ubIdx];
            if( ( IDX_INVALID != pxTmrHdl->ubIdx ) && ( pxTmrHdl->bIsRunning ) )
            {
                if( ( xCurrentTime - pxTmrHdl->xNExpiryTimeTicks ) >= pxTmrHdl->xNTimeoutTicks )
                {
                    DEBUG_TIMEREXPIRED_ON;
                    pxTmrHdl->bIsRunning = FALSE;
                    if( NULL != pxTmrHdl->pbMBPTimerExpiredFN )
                    {
                        ( void )pxTmrHdl->pbMBPTimerExpiredFN( pxTmrHdl->xMBMHdl );
                    }
                    DEBUG_TIMEREXPIRED_OFF;                    
                }
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
}
