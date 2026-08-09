/*
 * MODBUS Libary: uC3/uNET port
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmconfig.h,v 1.1 2011-01-02 16:16:22 embedded-solutions.cwalter Exp $
 */

#ifndef _MBMCONFIG_H
#define _MBMCONFIG_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/
#define MBM_DEFAULT_RESPONSE_TIMEOUT            ( 5000 )

#define MBM_SERIAL_API_VERSION                  ( 2 )
#define MBM_ASCII_ENABLED                       ( 0 )
#define MBM_RTU_ENABLED                         ( 0 )
#define MBM_TCP_ENABLED                         ( 1 )

/* ----------------------- Functions  ---------------------------------------*/

#endif
