/* 
 * MODBUS Master Library: Luminary Cortex M3, FreeRTOS and lwIP Example
 * Copyright (c) 2008-2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmconfig.h,v 1.3 2010-04-25 13:47:16 embedded-so.embedded-solutions.1 Exp $
 */

#ifndef _MBMCONFIG_H
#define _MBMCONFIG_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/

#define MBM_DEFAULT_RESPONSE_TIMEOUT            ( 1000 )

#define MBM_SERIAL_API_VERSION                  ( 2 )
#define MBM_ASCII_ENABLED                       ( 1 )
#define MBM_ASCII_WAITAFTERSEND_ENABLED	        ( 0 )
#define MBM_ASCII_BACKOF_TIME_MS                ( 5 ) 

#define MBM_RTU_ENABLED                         ( 1 )
#define MBM_RTU_WAITAFTERSEND_ENABLED	        ( 0 )
#define MBM_SERIAL_APIV2_RTU_DYNAMIC_TIMEOUT_MS( ulBaudRate ) \
	usMPSerialTimeout( ulBaudRate )

#define MBM_TCP_ENABLED                         ( 1 )

#define MBM_ENABLE_DEBUG_FACILITY		        ( 0 )

/* ----------------------- Functions  ---------------------------------------*/

#endif
