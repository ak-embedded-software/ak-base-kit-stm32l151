/*
 * MODBUS Library: AT91SAM7X port
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.1 2010-05-22 22:31:33 embedded-so.embedded-solutions.1 Exp $
 */

#ifndef _MBM_PORT_H
#define _MBM_PORT_H

#include <stdint.h>
#include "FreeRTOS.h"

#ifdef _cplusplus
extern          "C"
{
#endif

/* ----------------------- Defines ------------------------------------------*/

#define INLINE                              inline
#define STATIC                              static

#define PR_BEGIN_EXTERN_C                   extern "C" {
#define	PR_END_EXTERN_C                     }

#define MBP_ASSERT( x )                     ( ( x ) ? ( void ) 0 : vMBPAssert(  ) )

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

#define MBP_ENABLE_DEBUG_FACILITY           ( 0 )
#define MBP_FORMAT_USHORT                   "%hu"
#define MBP_FORMAT_SHORT                    "%hd"
#define MBP_FORMAT_UINT_AS_HEXBYTE          "%02X"
#define MBP_FORMAT_ULONG                    "%ul"

#define MBP_TASK_PRIORITY                   ( tskIDLE_PRIORITY + 1 )

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

typedef unsigned long ULONG;
typedef long    LONG;

/* ----------------------- Function prototypes ------------------------------*/
BOOL            bMBPIsWithinException( void );
void            vMBPInit( void );
void            vMBPAssert( void );
void            vMBPEnterCritical( void );
void            vMBPExitCritical( void );

#ifdef _cplusplus
}
#endif

#endif
