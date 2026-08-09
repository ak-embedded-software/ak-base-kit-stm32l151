/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.7 2010-08-19 20:18:18 embedded-so.embedded-solutions.1 Exp $
 */

#ifndef _MBM_PORT_H
#define _MBM_PORT_H

#include <assert.h>

#ifdef _cplusplus
extern          "C"
{
#endif


/* ----------------------- Defines ------------------------------------------*/
#if defined(  __CODEVISIONAVR__ )
#define MBP_FORCE_SERV1PROTOTYPES       ( 1 )
#if defined( __AVR_ATmega128__ )
#include <mega128.h>
#else
#error "Unsupported AVR hardware platform."
#endif
#define _BV(bit)                        ( 1 << ( bit ) )
#define PB7                             7
#define PC1                             1
#define sei( )                          #asm("sei")
#define cli( )                          #asm("cli")
typedef unsigned int                    size_t;
#endif

/* ----------------------- Defines ------------------------------------------*/

#define INLINE                         
#define STATIC                         static

#define PR_BEGIN_EXTERN_C              extern "C" {
#define	PR_END_EXTERN_C                }

#if defined(  __CODEVISIONAVR__ )
#define MBP_ASSERT( x )                ( ( x ) ? 0 : vMBPAssert(  ) )
#else
#define MBP_ASSERT( x )                ( ( x ) ? ( void ) 0 : vMBPAssert(  ) )
#endif

#define MBP_ENTER_CRITICAL_SECTION( )  vMBPEnterCritical( )
#define MBP_EXIT_CRITICAL_SECTION( )   vMBPExitCritical( )

#ifndef TRUE
#define TRUE                           ( BOOL )1
#endif

#ifndef FALSE
#define FALSE                          ( BOOL )0
#endif

#define MBP_EVENTHDL_INVALID           NULL
#define MBP_TIMERHDL_INVALID           NULL
#define MBP_SERIALHDL_INVALID          NULL
#define MBP_TCPHDL_INVALID             NULL

/* ----------------------- Type definitions ---------------------------------*/
typedef void       *xMBPEventHandle;
typedef void       *xMBPTimerHandle;
typedef void       *xMBPSerialHandle;
typedef void       *xMBPTCPHandle;
typedef void       *xMBPTCPClientHandle;

typedef char        BOOL;

typedef char        BYTE;
typedef unsigned char UBYTE;

typedef unsigned char UCHAR;
typedef char        CHAR;

typedef unsigned short USHORT;
typedef short       SHORT;

typedef unsigned long ULONG;
typedef long        LONG;

/* ----------------------- Function prototypes ------------------------------*/
void                vMBPAssert( void );
void                vMBPEnterCritical( void );
void                vMBPExitCritical( void );

#ifdef _cplusplus
}
#endif

#endif
