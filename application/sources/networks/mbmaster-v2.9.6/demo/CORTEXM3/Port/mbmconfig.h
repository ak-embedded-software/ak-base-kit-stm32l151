/*
 * MODBUS Library: ARM STM32 Port (FWLIB 2.0x)
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * ARM STM32 Port by Niels Andersen, Elcanic A/S <niels.andersen.elcanic@gmail.com>
 *
 * $Id: mbmconfig.h,v 1.2 2009-01-02 14:07:29 cwalter Exp $
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
#define MBM_DEFAULT_RESPONSE_TIMEOUT            ( 500 )
#define MBM_SERIAL_RTU_MAX_INSTANCES            ( 1 )
#define MBM_SERIAL_ASCII_MAX_INSTANCES          ( 1 )
#define MBM_TCP_MAX_INSTANCES                   ( 0 )
#define MBM_RTU_WAITAFTERSEND_ENABLED			( 1 )
#define MBM_ASCII_WAITAFTERSEND_ENABLED			( 2 )

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
