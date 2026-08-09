/* 
 * MODBUS Master Library: Configuration
 * Copyright (c) 2008-2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmconfig.h,v 1.1 2014-08-23 11:48:50 embedded-solutions.cwalter Exp $
 */

#ifndef _MBMCONFIG_H
#define _MBMCONFIG_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/
#define MBM_DEFAULT_RESPONSE_TIMEOUT            ( 1000 )
#define MBM_TCP_ENABLED                         ( 1 )
#define MBM_RTU_ENABLED                         ( 0 )
#define MBM_ASCII_ENABLED                       ( 0 )

#endif
#ifdef __cplusplus
    PR_END_EXTERN_C
#endif
