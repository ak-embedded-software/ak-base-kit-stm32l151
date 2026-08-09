/* 
 * MODBUS Library: NXP Cortex M3, FreeRTOS and lwIP Example
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.1 2011-01-02 16:16:22 embedded-solutions.cwalter Exp $
 */

#ifndef _MB_PORT_H
#define _MB_PORT_H

#ifdef __cplusplus
extern          "C" {
#endif

/* ----------------------- Defines ------------------------------------------*/

#define INLINE                              
#define STATIC                              static

#define PR_BEGIN_EXTERN_C                   extern "C" {
#define	PR_END_EXTERN_C                     }

#define MBP_ASSERT( x )                     do { if( !( x ) ) { vMBPAssert( __FILE__, __LINE__ ); } } while( 0 )

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

#define MBP_ENABLE_DEBUG_FACILITY           ( 1 )
#define MBP_FORMAT_USHORT                   "%hu"
#define MBP_FORMAT_SHORT                    "%hd"
#define MBP_FORMAT_UINT_AS_HEXBYTE          "%02X"
#define MBP_FORMAT_ULONG                    "%ul"
  
#define MBP_TASK_PRIORITY                   ( tskIDLE_PRIORITY + 1 )  

#define DRV_SERIAL_MAX_INSTANCES			( 2 )
#define MBP_FORCE_SERV2PROTOTYPES           ( 1 )

/* ----------------------- Type definitions ---------------------------------*/
typedef void      *xMBPEventHandle;
typedef void      *xMBPTimerHandle;
typedef void      *xMBPSerialHandle;
typedef void      *xMBPTCPHandle;
typedef void      *xMBPTCPClientHandle;

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
void              vMBPInit( void );
void              vMBPEnterCritical( void );
void              vMBPExitCritical( void );
void              vMBPAssert( const char *pszFile, int iLineNo );
USHORT	          usMPSerialTimeout( ULONG ulBaudRate );
void              vMBPSetLed( UBYTE ubIdx, BOOL bOn );

#ifdef __cplusplus
}
#endif

#endif
