/* 
 * MODBUS Master Library: Configuration
 * Copyright (c) 2008-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmconfig.h,v 1.16 2011-11-22 00:44:53 embedded-solutions.cwalter Exp $
 */

#ifndef _MBMCONFIG_H
#define _MBMCONFIG_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

/* RTU porting layer always passes full frames. No need for further timeouts */
#define MBM_SERIAL_APIV2_RTU_DYNAMIC_TIMEOUT_MS( ulBaudRate ) \
    vMBPSerialRTUV2Timeout( ulBaudRate )
    
#define MBP_ADVA_STARTUP_SHUTDOWN_ENABLED       ( 1 )
#define MBM_DEFAULT_RESPONSE_TIMEOUT            ( 1000 )
#define MBM_ASCII_ENABLED                       ( 1 )
#define MBM_RTU_ENABLED                         ( 1 )
#define MBM_TCP_ENABLED                         ( 1 )
#define MBM_UDP_ENABLED                         ( 1 )
#define MBM_SERIAL_API_VERSION                  ( 2 )
#define MBM_ASCII_BACKOF_TIME_MS                ( 0 ) 
#define MBS_SERIAL_APIV2_RTU_TIMEOUT_MS         ( 10 )
#define MBM_ENABLE_DEBUG_FACILITY				( 0 )
#define MBM_ENABLE_PROT_ANALYZER_INTERFACE		( 0 )
#define MBM_ENABLE_DEBUG_FACILITY_HEAVY			( 0 )

/* ----------------------- Functions  ---------------------------------------*/

#endif
