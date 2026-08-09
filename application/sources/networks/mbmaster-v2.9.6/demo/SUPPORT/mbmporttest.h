/* 
 * MODBUS Library: Port testing utility
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbmporttest.h,v 1.3 2007-08-17 23:32:29 cwalter Exp $
 */

#ifndef _MBM_RTU_H
#define _MBM_RTU_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/*! \addtogroup mbm_port_test
 * @{
 */

/* ----------------------- Defines ------------------------------------------*/

#define MBM_PORT_DEBUG_LED_ERROR    ( 0 )
#define MBM_PORT_DEBUG_LED_WORKING  ( 1 )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/

void            vMBPTestDebugLED( UBYTE ubIdx, BOOL bTurnOn );
void            vMBPTestRun( void );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

/*! @} */

#endif
