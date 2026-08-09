/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbsconfig.h,v 1.2 2009-02-18 11:27:14 embedded-solutions Exp $
 */

#ifndef _MBS_CONFIG_H
#define _MBS_CONFIG_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/
#define MBS_ASCII_ENABLED                       ( 0 )
#define MBS_RTU_ENABLED                         ( 1 )
#define MBS_TCP_ENABLED                         ( 0 )
#define MBS_SERIAL_RTU_MAX_INSTANCES            ( 1 )
#define MBS_SERIAL_ASCII_MAX_INSTANCES          ( 1 )
#define MBS_TCP_MAX_INSTANCES                   ( 0 )
#define MBS_SERIAL_API_VERSION                  ( 1 )

#define MBS_NCUSTOM_FUNCTION_HANDLERS           ( 0 )
#define MBS_FUNC_READ_DISCRETE_ENABLED 			( 0 )
#define MBS_FUNC_READ_COILS_ENABLED				( 0 )
#define MBS_FUNC_WRITE_SINGLE_COIL_ENABLED		( 0 )
#define MBS_FUNC_WRITE_MULTIPLE_COILS_ENABLED	( 0 )

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
