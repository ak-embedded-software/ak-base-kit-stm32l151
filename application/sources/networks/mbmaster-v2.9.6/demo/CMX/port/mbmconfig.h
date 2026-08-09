/* 
 * MODBUS Master Library: Configuration
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbmconfig.h,v 1.2 2008-09-01 18:42:09 cwalter Exp $
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

#define MBM_DEFAULT_RESPONSE_TIMEOUT            ( 1000 )
#define MBM_ASCII_ENABLED                       ( 0 )
#define MBM_RTU_ENABLED                         ( 1 )
#define MBM_SERIAL_API_VERSION                  ( 2 )
#define MBM_ASCII_BACKOF_TIME_MS                ( 0 )
#define MBM_SERIAL_APIV2_RTU_DYNAMIC_TIMEOUT_MS( ulBaudRate )  \
    ( ( 11UL * 8UL * 1000UL + ulBaudRate ) / ( ulBaudRate ) )
#endif
