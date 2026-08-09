/*
 * MODBUS Libary: uC3/uNET port
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.1 2011-06-13 19:17:54 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <intrinsics.h>
#include "kernel.h"
#include "kernel_id.h"


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
static int      lockCounter;
static int      lockRecursion;
static ID       lockOwner;
/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
void
vMBPAssert( int line, const char *file )
{
    volatile BOOL   bBreakOut = FALSE;

    __disable_interrupt(  );
    while( !bBreakOut )
    {
        printf( "ASSERT: line:%d, file:%s", line, file );
    }
}

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
    va_list         args;

    /* The NULL check is necessary because this could be called during startup within
     * an assertion.
     */
    if( bMBPPortLogIsEnabled( eLevel, eModule ) )
    {
        printf( "%s;%s;", pszMBPLevel2String( eLevel ), pszMBPModule2String( eModule ) );
        va_start( args, szFmt );
        vprintf( szFmt, args );
        va_end( args );
    }
}
#endif

void
vMBPEnterCritical( void )
{
    ER              ercd;
    ID              my_id;
    __istate_t      s;

    ercd = get_tid( &my_id );
    MBP_ASSERT( E_OK == ercd );
    s = __get_interrupt_state(  );
    __disable_interrupt(  );
    lockCounter++;
    __set_interrupt_state( s );
    if( lockCounter > 1 )
    {
        if( lockOwner != my_id )
        {
            ercd = wai_sem( ID_MODBUS_SEM );
            MBP_ASSERT( E_OK == ercd );
        }
    }
    lockOwner = my_id;
    lockRecursion++;
}

void
vMBPExitCritical( void )
{
    ER              ercd;
    ID              my_id;
    __istate_t      s;

    ercd = get_tid( &my_id );
    MBP_ASSERT( E_OK == ercd );
    MBP_ASSERT( lockOwner == my_id );

    lockRecursion--;
    if( 0 == lockRecursion )
    {
        lockOwner = TSK_NONE;
    }
    s = __get_interrupt_state(  );
    __disable_interrupt(  );
    lockCounter--;
    __set_interrupt_state( s );

    if( lockCounter > 0 )
    {
        if( 0 == lockRecursion )
        {
            ercd = sig_sem( ID_MODBUS_SEM );
            MBP_ASSERT( E_OK == ercd );
        }
    }
}

void
vMBPInit( void )
{
}
