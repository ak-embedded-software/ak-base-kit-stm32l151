/* 
 * MODBUS Library: PC-LINT framework port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.2 2009-01-03 11:33:42 cwalter Exp $
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

#define PR_BEGIN_EXTERN_C                   extern "C" {
#define	PR_END_EXTERN_C                     }

#define MBP_ASSERT( x )                     assert( x )

#define MBP_ENTER_CRITICAL_SECTION( )       vMBPEnterCritical( )
#define MBP_EXIT_CRITICAL_SECTION( )        vMBPExitCritical( )

#ifndef TRUE
#define TRUE                                ( BOOL )1
#endif

#ifndef FALSE
#define FALSE                               ( BOOL )0
#endif

#define MBP_EVENTHDL_INVALID                ( xMBPEventHandle )NULL
#define MBP_TIMERHDL_INVALID                ( xMBPTimerHandle )NULL
#define MBP_SERIALHDL_INVALID               ( xMBPSerialHandle )NULL
#define MBP_TCPHDL_INVALID                  ( xMBPTCPHandle )NULL
#define MBP_TCPHDL_CLIENT_INVALID           ( xMBPTCPClientHandle )NULL

#define MBP_HAS_TIMESTAMP                   ( 1 )

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    MBP_LOG_DEBUG,
    MBP_LOG_INFO,
    MBP_LOG_WARN,
    MBP_LOG_ERROR
} eMBPPortLogLevel;

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
void              vMBPPortLog( eMBPPortLogLevel eLevel, const char * pcModule, const char * pcFmt, ... );

#ifdef _cplusplus
}
#endif

#endif
