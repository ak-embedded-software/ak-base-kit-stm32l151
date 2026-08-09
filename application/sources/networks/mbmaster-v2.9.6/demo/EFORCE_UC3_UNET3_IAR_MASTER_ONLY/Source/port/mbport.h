/*
 * MODBUS Libary: uC3/uNET port
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.1 2011-06-13 19:17:54 embedded-solutions.cwalter Exp $
 */

#ifndef _MBM_PORT_H
#define _MBM_PORT_H

#ifdef _cplusplus
extern          "C"
{
#endif

#include <stdint.h>
#include "itron.h"

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Defines ------------------------------------------*/

#define INLINE
#define STATIC                         static

#define PR_BEGIN_EXTERN_C              extern "C" {
#define	PR_END_EXTERN_C                }

#define MBP_ASSERT( x )                ( ( x ) ? ( void ) 0 : vMBPAssert( __LINE__, __FILE__ ) )

#define MBP_ENTER_CRITICAL_SECTION( )  vMBPEnterCritical( )
#define MBP_EXIT_CRITICAL_SECTION( )   vMBPExitCritical( )

#define MBP_ENABLE_DEBUG_FACILITY      ( 1 )
#ifndef TRUE
#define TRUE                           ( BOOL )1
#endif

#ifndef FALSE
#define FALSE                          !TRUE
#endif

#define MBP_EVENTHDL_INVALID           NULL
#define MBP_TIMERHDL_INVALID           NULL
#define MBP_SERIALHDL_INVALID          NULL
#define MBP_TCPHDL_INVALID             NULL
#define MBP_TCPHDL_CLIENT_INVALID      NULL
#define MB_SLAVE_CODE                  0

/* ----------------------- Type definitions ---------------------------------*/
typedef void       *xMBPEventHandle;
typedef void       *xMBPTimerHandle;
typedef void       *xMBPSerialHandle;
typedef void       *xMBPTCPHandle;
typedef void       *xMBPTCPClientHandle;

typedef int8_t      BYTE;
typedef uint8_t 	UBYTE;
typedef uint8_t 	UCHAR;
typedef int8_t      CHAR;
typedef uint16_t 	USHORT;
typedef int16_t     SHORT;
typedef uint32_t 	ULONG;
typedef int32_t     LONG;

/* ----------------------- Function prototypes ------------------------------*/
void                vMBPAssert( int line, const char *file );
void                vMBPEnterCritical( void );
void                vMBPExitCritical( void );

void				vMBPInit( void );

void                vMBPTimerPoll( void );
void                vMBTCPClientPoll(  );
void				vMBTCPServerPoll( void );

#ifdef _cplusplus
}
#endif

#endif
