/* 
 * MODBUS Library: LINUX/CYGWIN port
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.5 2014-08-23 09:36:06 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"

/* ----------------------- Modbus includes ----------------------------------*/

/* ----------------------- Defines ------------------------------------------*/
#define MBM_LOGFACILITIES                       ( 0xFFFFU )
#define MBM_LOGLEVELS                           ( MB_LOG_DEBUG )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
static pthread_mutex_t xCritMutex;
static pthread_mutex_t xCritInitMutex;
static BOOL     bIsInitialized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
static void     vMBCritInit( pthread_mutex_t * );

/* ----------------------- Function prototypes ------------------------------*/
extern void     vMBPTimerDLLClose( void );
extern void     vMBPSerialDLLClose( void );
extern void     vMBPTCPDLLClose( void );
extern void     vMBPTCPDllInit( void );

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
    struct timeval  tp;

    va_list         args;

    ( void )gettimeofday( &tp, NULL );

    /* The NULL check is necessary because this could be called during startup within
     * an assertion.
     */

    if( bMBPPortLogIsEnabled( eLevel, eModule ) )
    {
        max_len = sizeof( arubDebugBuffer ) / sizeof( arubDebugBuffer[0] );
        j = snprintf( &arubDebugBuffer[i], max_len, "%s;%s;%lu.%lu;", pszMBPLevel2String( eLevel ),
                      pszMBPModule2String( eModule ), ( unsigned long )tp.tv_sec, ( unsigned long )tp.tv_usec );
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
vMBPLibraryLoad( void )
{
#if MBM_TCP_ENABLED == 1
    vMBPTCPDllInit(  );
#endif
}

void
vMBPLibraryUnload( void )
{
    vMBPTimerDLLClose(  );
#if MBM_ASCII_ENABLED == 1 || MBM_RTU_ENABLED == 1
    vMBPSerialDLLClose(  );
#endif
#if MBM_TCP_ENABLED == 1
    vMBPTCPDLLClose(  );
#endif
}

void
vMBPOtherDLLClose( void )
{
    int             iRes1, iRes2;

    iRes1 = pthread_mutex_destroy( &xCritMutex );
    iRes2 = pthread_mutex_destroy( &xCritInitMutex );

    assert( 0 == iRes1 );
    assert( 0 == iRes2 );
}

void
vMBPOtherDLLInit( void )
{
    vMBCritInit( &xCritMutex );
    vMBCritInit( &xCritInitMutex );
    signal( SIGPIPE, SIG_IGN );
}

void
vMBPEnterCritical( void )
{
    int             iRes;

    if( !bIsInitialized )
    {
        vMBCritInit( &xCritMutex );
    }
    iRes = pthread_mutex_lock( &xCritMutex );
    MBP_ASSERT( iRes == 0 );
}

void
vMBPExitCritical( void )
{
    int             iRes;

    if( !bIsInitialized )
    {
        vMBCritInit( &xCritMutex );
    }
    iRes = pthread_mutex_unlock( &xCritMutex );
    MBP_ASSERT( iRes == 0 );
}

void
vMBPEnterCriticalInit( void )
{
    int             iRes;

    if( !bIsInitialized )
    {
        vMBCritInit( &xCritInitMutex );
    }
    iRes = pthread_mutex_lock( &xCritInitMutex );
    MBP_ASSERT( iRes == 0 );
}

void
vMBPExitCriticalInit( void )
{
    int             iRes;

    if( !bIsInitialized )
    {
        vMBCritInit( &xCritInitMutex );
    }
    iRes = pthread_mutex_unlock( &xCritInitMutex );
    MBP_ASSERT( iRes == 0 );
}

void
vMBCritInit( pthread_mutex_t * pxMux )
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
        else if( 0 != pthread_mutex_init( pxMux, &xMutexAttr ) )
        {
            MBP_ASSERT( 0 );
        }
        else
        {
            bOkay = TRUE;
        }

        ( void )pthread_mutexattr_destroy( &xMutexAttr );
    }
    bIsInitialized = TRUE;
    assert( bOkay );
}

void
vMBPAssert( int line, const char *msg )
{
    fprintf( stderr, "%d:%s", line, msg );
    exit( 1 );
}
