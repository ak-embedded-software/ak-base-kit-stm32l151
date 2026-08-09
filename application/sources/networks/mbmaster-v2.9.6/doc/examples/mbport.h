/* 
 * MODBUS Libary: Porting layer
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbport.h,v 1.1 2007-08-19 12:26:04 cwalter Exp $
 */

#ifndef _MB_PORT_H
#define _MB_PORT_H

/*! \addtogroup mb_port
 * @{
 */

#ifdef _cplusplus
extern          "C"
{
#endif

/* ----------------------- Defines ------------------------------------------*/

/*! \brief INLINE macro to match different compiler types. */
#define INLINE                              inline
/*! \brief STATIC macro to match different compiler types. */
#define STATIC                              static

/*! \brief Mark functions as C code for C++ sources. Start tag. */
#define PR_BEGIN_EXTERN_C                   extern "C" {
/*! \brief Mark functions as C code for C++ sources. End tag. */
#define	PR_END_EXTERN_C                     }

/*! \brief An assertion macro.
 *
 * If this function is called the port should enter its debug mode and the
 * system should be restarted.
 */
#define MBM_PORT_ASSERT( x )                assert( x )

/*! \brief This macro must disable interrupts on the system.
 *
 * This function disable interrupts and keeps track on the number of
 * times it has been called. Typically it would save the processor status
 * register on entry, disable interrupts and if it was called the first time
 * it will save the processor status register. At every call it would 
 * increment the so called nesting counter.
 */
#define MBM_PORT_ENTER_CRITICAL_SECTION( )

/*! \brief This macro should restore the old system status.
 *
 * If this function has been called the same time as the function 
 * MBM_PORT_ENTER_CRITICAL_SECTION is should restore the processor context
 * stored when MBM_PORT_ENTER_CRITICAL_SECTION was called the first time. 
 * A typical implementation would decrement the counter and if the counter
 * reaches zero it would restore the old value.
 */
#define MBM_PORT_EXIT_CRITICAL_SECTION( )

#ifndef TRUE
/*! \brief A boolean evaluating to true in an boolean C-Expression. */
#define TRUE                                ( BOOL )1
#endif

#ifndef FALSE
/*! \brief A boolean evaluating to false in an boolean C-Expression. */
#define FALSE                               ( BOOL )0
#endif

/*! \brief An invalid handle for an event queue. */
#define MBM_PORT_EVENTHDL_INVALID           NULL
/*! \brief An invalid handle for a timer. */
#define MBM_PORT_TIMERHDL_INVALID           NULL
/*! \brief An invalid handle for a serial port. */
#define MBM_PORT_SERIALHDL_INVALID          NULL
/*! \brief An invalid handle for a TCP instance. */
#define MBM_PORT_TCPHDL_INVALID             NULL

/* ----------------------- Type definitions ---------------------------------*/

/*! \brief A port dependent type for an event handle. */
typedef void   *xMBMPortEventHandle;

/*! \brief A port dependent type for a timer handle. */
typedef void   *xMBMPortTimerHandle;

/*! \brief A port dependent type for a serial handle. */
typedef void   *xMBMPortSerialHandle;

/*! \brief A port dependent type for a TCP handle. */
typedef void   *xMBMPortTCPHandle;

/*! \brief Boolean data type. */
typedef char    BOOL;

/*! \brief An unsigned byte holding value from 0 to 255. */
typedef unsigned char UBYTE;
/*! \brief A byte holding value from -128 to 127. */
typedef char    BYTE;

/*! \brief An unsigned character holding value from 0 to 255. */
typedef unsigned char UCHAR;
/*! \brief A character holding value from -128 to 127. */
typedef char    CHAR;

/*! \brief An unsigned short value holding values from 0 to 65535. */
typedef unsigned short USHORT;
/*! \brief An signed short value holding values from -32768 to 32767. */
typedef short   SHORT;

/*! \brief An unsigned long value holding values from 0 to 4294967296. */
typedef unsigned long ULONG;
/*! \brief An signed long value holding values from -2147483648 to 2147483647. */
typedef long    LONG;

/* ----------------------- Function prototypes ------------------------------*/

#ifdef _cplusplus
}
#endif

/*! @) */

#endif
