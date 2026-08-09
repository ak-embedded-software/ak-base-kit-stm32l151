/* 
 * MODBUS Library: CUNIT framework port
 * Copyright (c) 2007-2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.9 2014-08-23 09:45:01 embedded-solutions.cwalter Exp $
 */

#ifndef _MBM_PORT_H
#define _MBM_PORT_H

#include <assert.h>

#ifdef _cplusplus
extern          "C"
{
#endif

/* ----------------------- Defines ------------------------------------------*/
#define INLINE                              inline
#define STATIC                              static

#define MBP_ENABLE_DEBUG_FACILITY           1
#define MBM_ENABLE_DEBUG_FACILITY           1
#define TCP_STUB_DEBUG                      1
#define UDP_STUB_DEBUG                      0

#define PR_BEGIN_EXTERN_C                   extern "C" {
#define    PR_END_EXTERN_C                     }

#define MBP_ASSERT( x )                     \
    ( ( x ) ? ( void )0 : vMBPAssert(__FILE__, __LINE__, #x) )

#define MBP_ENTER_CRITICAL_SECTION( )       vMBPEnterCritical( )
#define MBP_EXIT_CRITICAL_SECTION( )        vMBPExitCritical( )

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

#define MBP_HAS_TIMESTAMP                   ( 1 )

/* ----------------------- Type definitions ---------------------------------*/

typedef void      *xMBPEventHandle;
typedef void      *xMBPTimerHandle;
typedef void      *xMBPSerialHandle;
typedef void      *xMBPTCPHandle;
typedef void      *xMBPTCPClientHandle;
typedef char      BOOL;

typedef char      BYTE;
typedef unsigned char UBYTE;

typedef unsigned char UCHAR;
typedef char      CHAR;

typedef unsigned short USHORT;
typedef short     SHORT;

typedef unsigned long ULONG;
typedef long      LONG;

typedef ULONG     xMBPTimeStamp;

/* ----------------------- Function prototypes ------------------------------*/
void              vMBPEnterCritical( void );
void              vMBPExitCritical( void );
void              vMBPAssert( const char *pucFile, int line, const char *pucExpr );

#ifdef _cplusplus
}
#endif

#endif
