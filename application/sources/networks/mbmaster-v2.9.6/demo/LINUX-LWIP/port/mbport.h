/* 
 * MODBUS Library: Linux port
 * Copyright (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.2 2014-08-23 12:19:54 embedded-solutions.cwalter Exp $
 */

#ifndef _MB_PORT_H
#define _MB_PORT_H

#include <assert.h>
#include <time.h>

#ifdef __cplusplus
extern          "C"
{
#endif

/* ----------------------- Defines ------------------------------------------*/

#define MBP_CORRECT_FUNCPOINTER_CAST        ( 1 )

#define INLINE
#define STATIC                              static

#define PR_BEGIN_EXTERN_C                   extern "C" {
#define	PR_END_EXTERN_C                     }

#define MBP_ASSERT( x )                     \
    ( ( x ) ? (void)0 : vMBPAssert( __LINE__, #x ) )

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
#define MBP_HAS_TIMESTAMP                   ( 1 )
#define MBP_TASK_PRIORITY                   ( 1 )
#define MBP_ENABLE_DEBUG_FACILITY           ( 0 )

/* ----------------------- Function prototypes ------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/
typedef void   *xMBPEventHandle;
typedef void   *xMBPTimerHandle;
typedef void   *xMBPSerialHandle;
typedef void   *xMBPTCPHandle;
typedef void   *xMBPTCPClientHandle;

typedef int     BOOL;
typedef unsigned char BYTE;
typedef unsigned char UBYTE;

typedef unsigned char UCHAR;
typedef char    CHAR;

typedef unsigned short USHORT;
typedef short   SHORT;

typedef unsigned int UINT;
typedef int     INT;

typedef unsigned long ULONG;
typedef long    LONG;

typedef time_t  xMBPTimeStamp;

/* ----------------------- Function prototypes ------------------------------*/
void            vMBPOtherDLLInit( void );
void            vMBPOtherDLLClose( void );
void            vMBPEnterCritical( void );
void            vMBPExitCritical( void );
void            vMBPEnterCriticalInit( void );
void            vMBPExitCriticalInit( void );
void            vMBPAssert( int line, const char *msg );

#ifdef __cplusplus
}
#endif

#endif
