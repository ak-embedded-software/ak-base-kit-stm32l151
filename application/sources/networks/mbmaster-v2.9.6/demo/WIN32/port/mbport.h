/* 
 * MODBUS Library: WIN32 port
 * Copyright (c) 2009-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.18 2011-12-04 21:11:47 embedded-solutions.cwalter Exp $
 */

#ifndef _MB_PORT_H
#define _MB_PORT_H

#if defined( MBM_APIHEADERS_ONLY ) && ( MBM_APIHEADERS_ONLY == 1 )
#include <assert.h>
#else
#include "stdafx.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#endif

#ifdef __cplusplus
extern          "C" {
#endif

/* ----------------------- Defines ------------------------------------------*/

#define INLINE                              
#define STATIC                              static

#define PR_BEGIN_EXTERN_C                   extern "C" {
#define	PR_END_EXTERN_C                     }

#define MBP_ASSERT(_Expression) \
    (void)( (!!(_Expression)) || (vMBPAssert(#_Expression, __FILE__, __LINE__), 0) )

#define MBP_ENTER_CRITICAL_SECTION( )       vMBPEnterCritical( )
#define MBP_EXIT_CRITICAL_SECTION( )        vMBPExitCritical( )
#define MBP_ENTER_CRITICAL_INIT( )          vMBPEnterCriticalInit( )
#define MBP_EXIT_CRITICAL_INIT( )           vMBPExitCriticalInit( )

#ifndef TRUE
#define TRUE                                ( BOOL )1
#endif

#ifndef FALSE
#define FALSE                               ( BOOL )0
#endif

#define MBP_EVENTHDL_INVALID                NULL
#define MBP_TIMERHDL_INVALID                NULL
#define MBP_SERIALHDL_INVALID               NULL
#define MBP_TCPHDL_INVALID                  NULL
#define MBP_TCPHDL_CLIENT_INVALID           NULL

#define MBP_ENABLE_DEBUG_FACILITY			( 0 )
#define MBP_UDP_DEBUG                       ( 0 )

#define MBP_LEAK_TEST                       ( 0 )
#define MBP_LEAK_RATE                       ( 0.1 )
#define MBP_HAS_TIMESTAMP					( 1 )
#define MBP_SERIAL_PORT_DETECTS_TIMEOUT     ( 1 )

/* ----------------------- Type definitions ---------------------------------*/
typedef void      *xMBPEventHandle;
typedef void      *xMBPTimerHandle;
typedef void      *xMBPSerialHandle;
typedef void      *xMBPTCPHandle;
typedef void      *xMBPTCPClientHandle;
typedef time_t    xMBPTimeStamp;

typedef int       BOOL;
typedef unsigned char BYTE;
typedef unsigned char UBYTE;

typedef unsigned char UCHAR;
typedef char      CHAR;

typedef unsigned short USHORT;
typedef short     SHORT;

typedef unsigned long ULONG;
typedef long      LONG;

/* ----------------------- Function prototypes ------------------------------*/
void              vMBPOtherDLLInit( void );
void              vMBPOtherDLLClose( void );
void              vMBPEnterCritical( void );
void              vMBPExitCritical( void );
void              vMBPEnterCriticalInit( void );
void              vMBPExitCriticalInit( void );
void              vMBPAssert( CHAR *pszAssertion, CHAR *pszFilename, int line );
CHAR *            Error2String( DWORD dwError );

USHORT            vMBPSerialRTUV2Timeout( ULONG ulBaudRate );

#ifdef __cplusplus
}
#endif

#endif
