/* 
 * MODBUS Library: Win32 port
 * Copyright (c) 2008-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.10 2014-04-28 06:41:04 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define DEBUG_CONSOLE               ( 1 )
#define DEBUG_TIMESTAMP             ( 1 )
#define DEBUG_PIDS                  ( 1 )
#define DEBUG_DEFAULT_ENVIRONMENT   ( 1 )
#define USE_WSACLEANUP              ( 1 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

STATIC CRITICAL_SECTION xCritSection;
STATIC CRITICAL_SECTION xCritSectionInit;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/
extern void     vMBPTimerDLLClose( void );
extern void     vMBPSerialDLLClose( void );
extern void     vMBPTCPDLLClose( void );
extern void     vMBPTCPDllInit( void );
extern void     vMBPUDPDllInit( void );
extern void     vMBPUDPDllClose( void );

/* ----------------------- Start implementation -----------------------------*/
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )

CHAR           *
Error2String( DWORD dwError )
{
    static CHAR     szUserBuf[512];
    static LPTSTR   szErrorMsg = _T( "internal error" );
    CHAR           *lpMsgBuf = NULL;
    DWORD           dwLength;

    dwLength = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL,
                               dwError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ( CHAR * ) & lpMsgBuf, 0, NULL );
    if( dwLength == 0 )
    {
        lpMsgBuf = "internal error";
    }

    strncpy_s( szUserBuf, sizeof( szUserBuf ) / sizeof( szUserBuf[0] ), lpMsgBuf, strlen( lpMsgBuf ) );
    LocalFree( lpMsgBuf );

    return szUserBuf;
}


BOOL
bMBPPortLogIsEnabled( eMBPortLogLevel eLevel, eMBPortLogFacility eModule )
{
    BOOL            bLogMessage = FALSE;
    CHAR           *pszLogFacilities;
    CHAR           *pszLogLevels;

    pszLogFacilities = getenv( "MODBUS_LOGMODULES" );
    pszLogLevels = getenv( "MODBUS_LOGLEVELS" );
    if( ( NULL != pszLogFacilities ) && ( NULL != pszLogLevels ) )
    {
        if( ( ( strtoul( pszLogFacilities, NULL, 0 ) & ( ULONG ) eModule ) > 0 )
            && ( ( ULONG ) eLevel < strtoul( pszLogLevels, NULL, 0 ) ) )
        {
            bLogMessage = TRUE;
        }
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
    case MB_LOG_UDP:
        pszRetValue = "UDP";
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
    case MB_LOG_PORT_UDP:
        pszRetValue = "UDP";
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
    CHAR            szBuf[512];
    CHAR           *pszLogFile;
    FILE           *logFile;
    int             i = 0, j = 0, max_len;
    va_list         args;

#if DEBUG_TIMESTAMP == 1
    SYSTEMTIME      xSystemTime;
#endif

    if( bMBPPortLogIsEnabled( eLevel, eModule ) )
    {
#if DEBUG_TIMESTAMP == 1
        GetSystemTime( &xSystemTime );
        max_len = sizeof( szBuf ) / sizeof( szBuf[0] );
        j = sprintf_s( &szBuf[i], sizeof( szBuf ) / sizeof( szBuf[0] ) - i,
                       "%04u-%02u-%02uT%02u:%02u:%02u:%03uZ;",
                       xSystemTime.wYear, xSystemTime.wMonth, xSystemTime.wDay,
                       xSystemTime.wHour, xSystemTime.wMinute, xSystemTime.wSecond, xSystemTime.wMilliseconds );
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
#endif
#if DEBUG_PIDS == 1
        j = sprintf_s( &szBuf[i], sizeof( szBuf ) / sizeof( szBuf[0] ) - i, "PID=%ld:TID=%ld:", GetCurrentProcessId(  ),
                       GetCurrentThreadId(  ) );
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
#endif
        j = sprintf_s( &szBuf[i], sizeof( szBuf ) / sizeof( szBuf[0] ) - i, "%s:%s:", pszMBPLevel2String( eLevel ),
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
        j = vsnprintf_s( &szBuf[i], sizeof( szBuf ) / sizeof( szBuf[0] ) - i, _TRUNCATE, szFmt, args );
        if( ( j < 0 ) || ( j >= max_len ) )
        {
            goto error;
        }
        else
        {
            max_len -= j;
            i += j;
        }
        va_end( args );

#if DEBUG_CONSOLE == 1
        fputs( szBuf, stderr );
#endif
        if( NULL != ( pszLogFile = getenv( "MODBUS_LOGFILE" ) ) )
        {
            if( NULL != ( logFile = fopen( pszLogFile, "a+" ) ) )
            {
                ( void )fprintf( logFile, "%s", szBuf );
                ( void )fclose( logFile );
            }

        }
    }
  error:
    return;
}
#endif

void
vMBPWSAInit( void )
{
    WSADATA         wsaData;
    WORD            wVersionRequested;
    int             iSocketErr;

    /* Request Winsock version 2.2 */
    wVersionRequested = MAKEWORD( 2, 2 );
    if( 0 != ( iSocketErr = WSAStartup( wVersionRequested, &wsaData ) ) )
    {
        MBP_ASSERT( 0 );
    }
}

void
vMBPWSAClose( void )
{
#if USE_WSACLEANUP == 1
    if( SOCKET_ERROR == WSACleanup(  ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_WARN, MB_LOG_PORT_TCP, "can't shutdown winsock V2.2: %s\n",
                         Error2String( WSAGetLastError(  ) ) );
        }
#endif
        MBP_ASSERT( 0 );
    }
    else
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "cleanup of winsock V2.2\n" );
        }
#endif

    }
#endif
}

void
vMBPLibraryLoad( void )
{
    vMBPWSAInit(  );
    vMBPTCPDllInit(  );
#if defined( MBP_UDP_DEBUG ) && ( MBP_UDP_DEBUG == 1 )
    vMBPUDPDllInit(  );
#endif
}

void
vMBPLibraryUnload( void )
{
    vMBPTimerDLLClose(  );
    vMBPSerialDLLClose(  );
    vMBPTCPDLLClose(  );
    vMBPWSAClose(  );
}

void
vMBPOtherDLLClose( void )
{
    DeleteCriticalSection( &xCritSection );
    DeleteCriticalSection( &xCritSectionInit );
}

void
vMBPOtherDLLInit( void )
{
    InitializeCriticalSection( &xCritSection );
    InitializeCriticalSection( &xCritSectionInit );
#if DEBUG_DEFAULT_ENVIRONMENT == 1
    if( NULL == getenv( "MODBUS_LOGFILE" ) )
    {
        putenv( "MODBUS_LOGFILE=d:\\debug.log" );
    }
    if( NULL == getenv( "MODBUS_LOGMODULES" ) )
    {
        putenv( "MODBUS_LOGMODULES=0xFFFF" );
    }
    if( NULL == getenv( "MODBUS_LOGLEVELS" ) )
    {
        putenv( "MODBUS_LOGLEVELS=4" );
    }
#endif
}

void
vMBPEnterCritical( void )
{
    EnterCriticalSection( &xCritSection );
}

void
vMBPExitCritical( void )
{
    LeaveCriticalSection( &xCritSection );
}

void
vMBPEnterCriticalInit( void )
{
    EnterCriticalSection( &xCritSectionInit );
}

void
vMBPExitCriticalInit( void )
{
    LeaveCriticalSection( &xCritSectionInit );
}

void
vMBPAssert( CHAR * pszAssertion, CHAR * pszFilename, int line )
{
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    vMBPPortLog( MB_LOG_ERROR, MB_LOG_CORE, "assertion:%s:%d:%s", pszFilename, line, pszAssertion );
#endif
    assert( 0 );
}

void
vMBPGetTimeStamp( xMBPTimeStamp * pTimeStamp )
{
	time( pTimeStamp );
}