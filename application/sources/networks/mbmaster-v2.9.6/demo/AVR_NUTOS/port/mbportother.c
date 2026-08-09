/* 
 * MODBUS Library: Nut/OS port
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportother.c,v 1.1 2010-02-21 19:23:07 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <cfg/crt.h>
#include <io.h>
#include <sys/timer.h>
#include <sys/mutex.h>
#include <dev/board.h>
#include "mbport.h"
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#endif
/* ----------------------- Platform includes --------------------------------*/

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define DEBUG_LED_ENABLED		( 1 )
#define DEBUG_MODBUS_LOGLEVELS	( 4UL )
#define DEBUG_MODBUS_LOGMODULES	( 0xFFFFUL )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
STATIC MUTEX    xLock;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
STATIC FILE    *xDebugOut;
STATIC MUTEX    xDebugLock;
#endif

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )

BOOL
bMBPPortLogIsEnabled( eMBPortLogLevel eLevel, eMBPortLogFacility eModule )
{
    BOOL            bLogMessage = FALSE;

    if( ( ( DEBUG_MODBUS_LOGMODULES & ( ULONG ) eModule ) > 0 ) && ( ( ULONG ) eLevel < DEBUG_MODBUS_LOGLEVELS ) )
    {
        bLogMessage = TRUE;
    }

    return bLogMessage;
}

void
vMBPPortLog( eMBPortLogLevel eLevel, eMBPortLogFacility eModule, const CHAR * szFmt, ... )
{
    va_list         args;

    NutMutexLock( &xDebugLock );
    if( NULL != xDebugOut )
    {
        if( bMBPPortLogIsEnabled( eLevel, eModule ) )
        {
            ( void )fprintf( xDebugOut, "%08lu:%1d:%04X:", NutGetMillis(  ), ( int )eLevel, ( int )eModule );
            va_start( args, szFmt );
            ( void )vfprintf( xDebugOut, szFmt, args );
            va_end( args );
        }
    }
    ( void )NutMutexUnlock( &xDebugLock );
    return;
}
#endif

void
vMBPInit( void )
{
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    u_long          ulBaudRate = 115200;
#endif
    NutMutexInit( &xLock );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    NutMutexInit( &xDebugLock );
    ( void )NutRegisterDevice( &DEV_UART1, 0, 0 );
    xDebugOut = fopen( DEV_UART1_NAME, "w" );
    if( NULL != xDebugOut )
    {
        ( void )_ioctl( _fileno( xDebugOut ), UART_SETSPEED, &ulBaudRate );
    }
#endif
#if DEBUG_LED_ENABLED == 1
    /* Initialize DEBUG LEDs */
    DDRF |= _BV( PF2 ) | _BV( PF3 ) | _BV( PF1 ) | _BV( PF0 );
    PORTF &= ~( _BV( PF2 ) | _BV( PF3 ) | _BV( PF1 ) | _BV( PF0 ) );
#endif
}

void
vMBPAssert( void )
{
    vMBPEnterCritical(  );
    while( TRUE )
    {
        vMBPSetLED( 1, 2 );
        NutSleep( 100 );
        vMBPSetLED( 1, 0 );
        NutSleep( 100 );
    }
}

void
vMBPSetLED( UBYTE ubLED, UBYTE bState )
{
#if DEBUG_LED_ENABLED == 1
    switch ( ubLED )
    {
    case 0:
        PORTF &= ~( _BV( PF2 ) | _BV( PF3 ) );
        switch ( bState )
        {
        case 2:
            PORTF |= _BV( PF3 );
            break;
        case 1:
            PORTF |= _BV( PF2 );
            break;
        default:
        case 0:
            break;
        }
        break;
    case 1:
        PORTF &= ~( _BV( PF1 ) | _BV( PF0 ) );
        switch ( bState )
        {
        case 2:
            PORTF |= _BV( PF1 );
            break;
        case 1:
            PORTF |= _BV( PF0 );
            break;
        default:
        case 0:
            break;
        }
        break;
    default:
        break;
    }
#endif
}

void
vMBPEnterCritical( void )
{
    NutMutexLock( &xLock );
}

void
vMBPExitCritical( void )
{
    NutMutexUnlock( &xLock );
}
