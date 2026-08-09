/* 
 * MODBUS Library: Luminary Cortex M3, FreeRTOS and lwIP Example
 * Copyright (c) 2008-2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.5 2010-06-13 17:04:48 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include <driverlib/interrupt.h>
#include <driverlib/gpio.h>

/* ----------------------- Platform includes --------------------------------*/

#include "utils/uartstdio.h"
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBM_LOGFACILITIES                       ( 0xFFFFU )
#define MBM_LOGLEVELS                           ( MB_LOG_ERROR )
#define MBP_LOCK_TIMEOUT                        ( 10000 / portTICK_RATE_MS )
#define MBP_FLUSH_AFTER_WRITE                   ( 1 )
/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
STATIC xSemaphoreHandle xCritSection;

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
STATIC CHAR     arubDebugBuffer[256];

xSemaphoreHandle xDebugBufferLock;
#endif
/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )

BOOL
bMBPPortLogIsEnabled( eMBPortLogLevel eLevel, eMBPortLogFacility eModule )
{
    BOOL            bLogMessage = FALSE;

    if( ( ( MBM_LOGFACILITIES & ( ULONG ) eModule ) > 0 ) && ( ( ULONG ) eLevel <= MBM_LOGLEVELS ) )
    {
        bLogMessage = TRUE;
    }
    return bLogMessage;
}

const CHAR     *
pszMBPModule2String( eMBPortLogFacility eModule )
{
    const CHAR     *pszRetValue;

    switch ( eModule )
    {
    case MB_LOG_CORE:
        pszRetValue = "CORE";
        break;
    case MB_LOG_RTU:
        pszRetValue = "RTU";
        break;
    case MB_LOG_ASCII:
        pszRetValue = "ASCII";
        break;
    case MB_LOG_TCP:
        pszRetValue = "TCP";
        break;
    case MB_LOG_PORT_EVENT:
        pszRetValue = "EVENT";
        break;
    case MB_LOG_PORT_TIMER:
        pszRetValue = "TIMER";
        break;
    case MB_LOG_PORT_SERIAL:
        pszRetValue = "SERIAL";
        break;
    case MB_LOG_PORT_TCP:
        pszRetValue = "TCP";
        break;
    case MB_LOG_PORT_OTHER:
        pszRetValue = "OTHER";
        break;
    default:
        pszRetValue = "UNKNOWN";
        break;
    }
    return pszRetValue;
}

const CHAR     *
pszMBPLevel2String( eMBPortLogLevel eLevel )
{
    const CHAR     *pszRetValue;

    switch ( eLevel )
    {
    case MB_LOG_ERROR:
        pszRetValue = "ERROR";
        break;
    case MB_LOG_WARN:
        pszRetValue = "WARN";
        break;
    case MB_LOG_INFO:
        pszRetValue = "INFO";
        break;
    case MB_LOG_DEBUG:
        pszRetValue = "DEBUG";
        break;
    default:
        pszRetValue = "UNKNOWN";
        break;
    }
    return pszRetValue;
}

void
vMBPPortLogLWIP( const CHAR * szFmt, ... )
{
    int             i = 0, j = 0, max_len;

    va_list         args;


    if( xSemaphoreTake( xDebugBufferLock, portMAX_DELAY ) )
    {
        max_len = sizeof( arubDebugBuffer ) / sizeof( arubDebugBuffer[0] );
        j = snprintf( &arubDebugBuffer[i], max_len, "UNKNOWN;lwIP;" );
        /* Truncation or snprintf error */
        if( ( j < 0 ) || ( j >= max_len ) )
        {
            goto error;
        }
        else
        {
            max_len -= j;
            i += j;
        }

        va_start( args, szFmt );
        j = vsnprintf( &arubDebugBuffer[i], max_len, szFmt, args );
        va_end( args );
        /* Truncation or snprintf error */
        if( ( j < 0 ) || ( j >= max_len ) )
        {
            goto error;
        }
        else
        {
            max_len -= j;
            i += j;
        }

        j = 0;
        do
        {
            j += UARTwrite( &arubDebugBuffer[j], i - j );
            if( i != j )
            {
                UARTFlushTx( 0 );
            }
        }
        while( i != j );
#if defined( MBP_FLUSH_AFTER_WRITE ) && ( MBP_FLUSH_AFTER_WRITE == 1 )
        UARTFlushTx( 0 );
#endif
      error:
        /* Can not fail according to API dock if obtained correctly. */
        ( void )xSemaphoreGive( xDebugBufferLock );
    }
}

void
vMBPPortLog( eMBPortLogLevel eLevel, eMBPortLogFacility eModule, const CHAR * szFmt, ... )
{

    int             i = 0, j = 0, max_len;

    va_list         args;

    /* The NULL check is necessary because this could be called during startup within 
     * an assertion.
     */
    if( ( NULL != xDebugBufferLock ) && xSemaphoreTake( xDebugBufferLock, portMAX_DELAY ) )
    {
        if( bMBPPortLogIsEnabled( eLevel, eModule ) )
        {
            max_len = sizeof( arubDebugBuffer ) / sizeof( arubDebugBuffer[0] );
            j = snprintf( &arubDebugBuffer[i], max_len, "%s;%s;", pszMBPLevel2String( eLevel ),
                          pszMBPModule2String( eModule ) );
            if( ( j < 0 ) || ( j >= max_len ) )
            {
                goto error;
            }
            else
            {
                max_len -= j;
                i += j;
            }

            va_start( args, szFmt );
            j = vsnprintf( &arubDebugBuffer[i], max_len, szFmt, args );
            va_end( args );
            /* Truncation or snprintf error */
            if( ( j < 0 ) || ( j >= max_len ) )
            {
                goto error;
            }
            else
            {
                max_len -= j;
                i += j;
            }

            j = 0;
            do
            {
                j += UARTwrite( &arubDebugBuffer[j], i - j );
                if( i != j )
                {
                    UARTFlushTx( 0 );
                }
            }
            while( i != j );
#if defined( MBP_FLUSH_AFTER_WRITE ) && ( MBP_FLUSH_AFTER_WRITE == 1 )
            UARTFlushTx( 0 );
#endif
        }
      error:
        /* Can not fail according to API dock if obtained correctly. */
        ( void )xSemaphoreGive( xDebugBufferLock );
    }
}
#endif

void
vMBPInit( void )
{
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    extern void     UARTStdioIntHandler( void );
#endif

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
#if defined( PART_LM3S6965 )
    GPIOPinTypeUART( GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
    IntRegister( INT_UART0, UARTStdioIntHandler );
    UARTStdioInitExpClk( 0, 57600 );   
#elif defined ( PART_LM3S8962 )
    GPIOPinTypeUART( GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
    IntRegister( INT_UART0, UARTStdioIntHandler );
    UARTStdioInitExpClk( 0, 57600 );  
#elif defined ( PART_LM3S818 )
    GPIOPinTypeUART( GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3 );
    IntRegister( INT_UART1, UARTStdioIntHandler );
    UARTStdioInitExpClk( 1, 57600 );   
#else
#error "part not supported"
#endif
#endif

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    xDebugBufferLock = xSemaphoreCreateMutex(  );
    MBP_ASSERT( NULL != xDebugBufferLock );
#endif

    xCritSection = xSemaphoreCreateRecursiveMutex(  );
    MBP_ASSERT( NULL != xCritSection );
}

void
vMBPEnterCritical( void )
{
    signed portBASE_TYPE xResult;

    do
    {
        xResult = xSemaphoreTakeRecursive( xCritSection, MBP_LOCK_TIMEOUT );
        if( pdTRUE != xResult )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_OTHER ) )
            {
                vMBPPortLog( MB_LOG_WARN, MB_LOG_PORT_OTHER, "Timeout waiting on core lock! Retrying...\n" );
            }
#endif
        }
    }
    while( pdTRUE != xResult );
}

void
vMBPExitCritical( void )
{
    signed portBASE_TYPE xResult;

    xResult = xSemaphoreGiveRecursive( xCritSection );
    MBP_ASSERT( pdTRUE == xResult );
}

void
vMBPAssert( const char *pszFile, int iLineNo )
{
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )    
    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_OTHER, "ASSERT: %s:%d\n", pszFile, iLineNo );
#endif    
    ( void )IntMasterDisable(  );
    /* Let the watchdog trigger a reset here. */
    for( ;; );
}
