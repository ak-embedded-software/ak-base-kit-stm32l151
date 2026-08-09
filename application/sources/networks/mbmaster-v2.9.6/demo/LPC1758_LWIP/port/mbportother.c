/* 
 * MODBUS Library: NXP, FreeRTOS and lwIP Example
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.1 2011-01-02 16:16:22 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* ----------------------- Platform includes --------------------------------*/

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
        while( j < i )
        {
            while(!LPC_UART1->LSR & ( 1UL << 5 ));
            LPC_UART1->THR = arubDebugBuffer[j];
            j++;
        };

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
            while( j < i )
            {
                while(!(LPC_UART1->LSR & ( 1UL << 5 )));
                LPC_UART1->THR = arubDebugBuffer[j];
                j++;
            };
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
    ULONG ulPCLKDIV;
    ULONG ulPCLK;
    ULONG ulDIV;

    vPortEnterCritical(  );
    LPC_SC->PCLKSEL0 &= ~( 0x03UL << 8UL );
    LPC_SC->PCLKSEL0 |= ( 0x01UL << 8UL );
    ulPCLKDIV = (LPC_SC->PCLKSEL0 >> 8) & 0x03;
    switch ( ulPCLKDIV )
    {
    case 0x00:
    default:
        ulPCLK = SystemCoreClock/4;
        break;
    case 0x01:
        ulPCLK = SystemCoreClock;
        break;
      case 0x02:
          ulPCLK = SystemCoreClock/2;
        break;
      case 0x03:
          ulPCLK = SystemCoreClock/8;
        break;
    }

    LPC_SC->PCONP |= 1UL << 4UL;
    LPC_PINCON->PINSEL4 &= ~0x0000000F;
    LPC_PINCON->PINSEL4 |= 0x0000000A;
    ulDIV = ulPCLK / ( 16UL * 115200UL ) ;
    LPC_UART1->LCR = 0x83;      /* 8 bits, no Parity, 1 Stop bit */
    LPC_UART1->DLM = ulDIV / 256;
    LPC_UART1->DLL = ulDIV % 256;
    LPC_UART1->LCR = 0x03;      /* DLAB = 0 */
    LPC_UART1->FCR = 0x07;      /* Enable and reset TX and RX FIFO. */
    vPortExitCritical(  );
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
    __disable_irq(  );
    /* Let the watchdog trigger a reset here. */
    for( ;; );
}

void
vMBPSetLed( UBYTE ubIdx, BOOL bOn )
{
    switch( ubIdx )
    {
    case 0:
        LPC_GPIO2->FIODIR |= ( 1 << 4U );
        if( bOn )
        {
            LPC_GPIO2->FIOSET |= ( 1 << 4U );
        }
        else
        {
            LPC_GPIO2->FIOCLR |= ( 1 << 4U );
        }
        break;
    default:
        break;
    }
}
