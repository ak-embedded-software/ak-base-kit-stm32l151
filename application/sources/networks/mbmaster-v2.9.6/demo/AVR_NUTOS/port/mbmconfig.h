/* 
 * MODBUS Master Library: Nut/OS port
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmconfig.h,v 1.1 2010-02-21 19:23:07 embedded-so.embedded-solutions.1 Exp $
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

#define MBM_ASCII_ENABLED                       ( 1 )
#define MBM_RTU_ENABLED                         ( 1 )
#define MBM_TCP_ENABLED                         ( 0 )
#define MBM_SERIAL_API_VERSION              	( 2 )
#define MBM_SERIAL_APIV2_RTU_DYNAMIC_TIMEOUT_MS( ulBaudRate ) \
    usMBPSerialRTUV2Timeout( ulBaudRate )
#define MBM_ENABLE_DEBUG_FACILITY				( 0 )

#endif
