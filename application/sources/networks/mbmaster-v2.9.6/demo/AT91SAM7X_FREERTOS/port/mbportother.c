/*
 * MODBUS Library: AT91SAM7X port
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.1 2010-05-22 22:31:33 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#if defined( __ICCARM__ ) && ( __ICCARM__ == 1 )
#include <intrinsics.h>
#endif

/* ----------------------- Platform includes --------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "dbgu.h"
#include "AT91SAM7X256.h"
#include "lib_AT91SAM7X256.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbport.h"
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBM_LOGFACILITIES                       ( 0xFFFFU )
#define MBM_LOGLEVELS                           ( MB_LOG_ERROR )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
STATIC CHAR     arubDebugBuffer[256];

xSemaphoreHandle xDebugBufferLock;
#endif
/* ----------------------- Static functions ---------------------------------*/

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
                DBGU_PutChar( arubDebugBuffer[j] );
                j++;
            }
            while( i != j );
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
    AT91F_DBGU_CfgPMC(  );
    AT91F_DBGU_CfgPIO(  );
    DBGU_Configure( DBGU_STANDARD, 115200, configCPU_CLOCK_HZ );
    xDebugBufferLock = xSemaphoreCreateMutex(  );
    MBP_ASSERT( NULL != xDebugBufferLock );
#endif
}

void
vMBPAssert( void )
{
    volatile BOOL   bBreakOut = FALSE;

    MBP_ENTER_CRITICAL_SECTION(  );
    while( !bBreakOut );
}

BOOL
bMBPIsWithinException( void )
{
    BOOL bIsWithinException = FALSE;
    unsigned long   ulCPSR;
#if defined( __ICCARM__ ) && ( __ICCARM__ == 1 )
    ulCPSR = __get_CPSR(  );
#endif
    if( ( ulCPSR & ( 0x80 | 0x40 ) ) > 0 )
    {
        bIsWithinException = TRUE;
    }
    return bIsWithinException;
}

void
vMBPEnterCritical( void )
{
    /* WARNING: Assume special version of portENTER_CRITICAL for FreeRTOS
     * on SAM7X
     */
    portENTER_CRITICAL(  );
}

void
vMBPExitCritical( void )
{
    /* WARNING: Assume special version of portENTER_CRITICAL for FreeRTOS
     * on SAM7X
     */
    portEXIT_CRITICAL(  );
}
