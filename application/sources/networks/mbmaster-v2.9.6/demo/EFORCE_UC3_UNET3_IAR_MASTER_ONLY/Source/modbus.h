/* 
 * MODBUS Libary: uC3/uNET port
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: template.h,v 1.1 2007-08-19 12:31:23 cwalter Exp $
 */

#ifndef _MODBUS_H
#define _MODBUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/
void task_mbslave_impl( void );
void task_mbmaster_impl( void );

/* poll function - this function shall be called after the uNet3 and all
 * hardware has been initialized.
 */
void mbstack_poll( void );
  
/* ----------------------- Function prototypes ------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
