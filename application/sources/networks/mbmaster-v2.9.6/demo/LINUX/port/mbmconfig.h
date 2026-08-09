/* 
 * MODBUS Master Library: Configuration
 * Copyright (c) 2008-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmconfig.h,v 1.7 2009-10-28 21:17:55 embedded-solutions.anovak Exp $
 */

#ifndef _MBMCONFIG_H
#define _MBMCONFIG_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif


/* ----------------------- Defines ------------------------------------------*/
#define MBP_ADVA_STARTUP_SHUTDOWN_ENABLED       ( 1 )
#define MBM_DEFAULT_RESPONSE_TIMEOUT            ( 1000 )
#ifndef MBM_ASCII_ENABLED
#define MBM_ASCII_ENABLED                       ( 1 )
#endif
#ifndef MBM_RTU_ENABLED
#define MBM_RTU_ENABLED                         ( 1 )
#endif
#ifndef MBM_TCP_ENABLED
#define MBM_TCP_ENABLED                         ( 1 )
#endif
#define MBM_SERIAL_API_VERSION                  ( 2 )
#define MBM_ASCII_BACKOF_TIME_MS                ( 0 )
#define MBM_SERIAL_APIV2_RTU_TIMEOUT_MS         ( 10 )
#endif

#ifdef __cplusplus
 PR_END_EXTERN_C
#endif
