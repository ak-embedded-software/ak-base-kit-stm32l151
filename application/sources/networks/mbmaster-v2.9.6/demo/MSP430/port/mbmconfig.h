/*
 * MODBUS Library: MSP430 port
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmconfig.h,v 1.1 2010-11-28 22:24:38 embedded-solutions.cwalter Exp $
 */

#ifndef _MBM_CONFIG_H
#define _MBM_CONFIG_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/
#define MBM_ASCII_ENABLED                       ( 1 )
#define MBM_RTU_ENABLED                         ( 1 )
#define MBM_TCP_ENABLED                         ( 0 )
#define MBM_SERIAL_RTU_MAX_INSTANCES            ( 1 )
#define MBM_SERIAL_ASCII_MAX_INSTANCES          ( 1 )
#define MBM_TCP_MAX_INSTANCES                   ( 0 )
#define MBS_SERIAL_API_VERSION                  ( 1 )

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
