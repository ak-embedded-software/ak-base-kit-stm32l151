/* 
 * MODBUS Library: ARM7/AT91SAM7X port
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbmconfig.h,v 1.1 2010-05-22 22:31:33 embedded-so.embedded-solutions.1 Exp $
 */

#ifndef _MBM_CONFIG_H
#define _MBM_CONFIG_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif
/* ----------------------- Defines ------------------------------------------*/
#define MBM_ASCII_ENABLED                       ( 0 )
#define MBM_RTU_ENABLED                         ( 1 )
#define MBM_TCP_ENABLED                         ( 0 )
#define MBM_DEFAULT_RESPONSE_TIMEOUT            ( 500 )
#define MBM_SERIAL_RTU_MAX_INSTANCES            ( 1 )
#define MBM_SERIAL_ASCII_MAX_INSTANCES          ( 1 )
#define MBM_TCP_MAX_INSTANCES                   ( 0 )
#ifdef __cplusplus
    PR_END_EXTERN_C
#endif
#endif
