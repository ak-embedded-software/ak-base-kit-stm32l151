/* 
 * MODBUS Library: CMX port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.4 2008-09-01 18:42:43 cwalter Exp $
 */

#ifndef _MBM_PORT_H
#define _MBM_PORT_H

#include <assert.h>

#ifdef _cplusplus
extern          "C"
{
#endif

/* ----------------------- Defines ------------------------------------------*/

#define INLINE
#define STATIC                         static

#define PR_BEGIN_EXTERN_C              extern "C" {
#define	PR_END_EXTERN_C                }

#define MBP_ASSERT( x )                ( ( x ) ? ( void ) 0 : vMBPAssert(  ) )

#define MBP_ENTER_CRITICAL_SECTION( )  vMBPEnterCritical( )
#define MBP_EXIT_CRITICAL_SECTION( )   vMBPExitCritical( )
#define MBP_ENTER_CRITICAL_SECTION_ISR( ) vMBPEnterCriticalISR( )
#define MBP_EXIT_CRITICAL_SECTION_ISR( )  vMBPExitCriticalISR( )


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

/* ----------------------- CMX specifics ------------------------------------*/
#define MB_USE_SINGLETASK              ( 0 )
#define MB_RESOURCE                    ( 1 )
#define MB_TIMERS_IDX_FIRST            ( 1 )
#define MB_TIMERS_EVENT_IDX_FIRST      ( 2 )
#define MB_TIMERS_COUNT                ( 3 )
#define MB_SERRX_EVENT                 ( 0x0001 )
#define MB_SERTX_EVENT                 ( 0x0002 )
#define MB_QUEUE_EVENT                 ( 0x0100 )
#define MB_SERIAL_TASK_PRIORITY        ( 5 )
#define MB_MODBUS_TASK_PRIORITY        ( 5 )
extern unsigned char g_bSerialTaskSlot;
extern unsigned char g_bMODBUSTaskSlot;

/* ----------------------- Type definitions ---------------------------------*/
typedef void   *xMBPEventHandle;
typedef void   *xMBPTimerHandle;
typedef void   *xMBPSerialHandle;
typedef void   *xMBPTCPHandle;
typedef void   *xMBPTCPClientHandle;

typedef char    BOOL;

typedef char    BYTE;
typedef unsigned char UBYTE;

typedef unsigned char UCHAR;
typedef char    CHAR;

typedef unsigned short USHORT;
typedef short   SHORT;

typedef unsigned long ULONG;
typedef long    LONG;

/* ----------------------- Function prototypes ------------------------------*/
extern void     vMBPDebugLED( UBYTE ubLED, BOOL bStatus );
extern void     vMBPAssert( void );
extern void     vMBPEnterCritical( void );
extern void     vMBPExitCritical( void );
extern void     prvvTimerHandler( unsigned char ucTmrIdx );
#if MB_USE_SINGLETASK
extern void     prvvSerHandlerTask( void );
#endif

#ifdef _cplusplus
}
#endif

#endif
