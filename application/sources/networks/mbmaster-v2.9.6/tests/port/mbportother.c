/* 
 * MODBUS Library: CUNIT framework port
 * Copyright (c) 2007-2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.8 2014-08-23 09:45:01 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBM_LOGFACILITIES                       ( 0xFFFFU )
#define MBM_LOGLEVELS                           ( MB_LOG_DEBUG )


/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

static pthread_mutex_t xMutex;
static BOOL     bIsInitialized = FALSE;
static BOOL     bFailOnAssert = TRUE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vMBPInit( void );

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
    else
    {
    	bLogMessage = FALSE;
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
	STATIC CHAR     arubDebugBuffer[256];
    int             i = 0, j = 0, max_len;

    va_list         args;

    /* The NULL check is necessary because this could be called during startup within
     * an assertion.
     */

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
            fprintf( stderr, "%s", arubDebugBuffer );
            error:
            ( void )args;
        }

}
#endif

void
vMBPEnterCritical( void )
{
    int             iRes;

    if( !bIsInitialized )
    {
        vMBPInit(  );
    }
    iRes = pthread_mutex_lock( &xMutex );
    MBP_ASSERT( iRes == 0 );
}

void
vMBPExitCritical( void )
{
    int             iRes;

    if( !bIsInitialized )
    {
        vMBPInit(  );
    }
    iRes = pthread_mutex_unlock( &xMutex );
    MBP_ASSERT( iRes == 0 );
}

void
vMBPInit( void )
{
    BOOL            bOkay = FALSE;
    pthread_mutexattr_t xMutexAttr;

    if( 0 != pthread_mutexattr_init( &xMutexAttr ) )
    {
    }
    else
    {
        if( 0 != pthread_mutexattr_settype( &xMutexAttr, PTHREAD_MUTEX_RECURSIVE ) )
        {
            /* no recursive mutexes available. */
        }
        if( 0 != pthread_mutex_init( &xMutex, &xMutexAttr ) )
        {
            MBP_ASSERT( 0 );
        }
        else
        {
            bOkay = TRUE;
        }
    }
    bIsInitialized = TRUE;
    assert( bOkay );
}

void
vMBPFailOnAssert( BOOL bFail )
{
    bFailOnAssert = bFail;
}

void
vMBPAssert( const char *pucFile, int line, const char *pucExpr )
{
    volatile BOOL   bBreakOut = FALSE;

    fprintf( stderr, "ASSERT: %s:%d:%s !\n", pucFile, line, pucExpr );
    do
    {
    }
    while( !bBreakOut );
}

void
vMBPGetTimeStamp( xMBPTimeStamp * pTimeStamp )
{
    STATIC xMBPTimeStamp xCurrentTime;
    *pTimeStamp = xCurrentTime;
    xCurrentTime++;
}
