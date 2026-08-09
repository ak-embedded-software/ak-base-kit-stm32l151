/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP/UDP.
 * Copyright (c) 2007-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_udp_stubs.h,v 1.2 2011-11-22 00:42:49 embedded-solutions.cwalter Exp $
 */

#ifndef _UT_MBM_UDP_STUBS_H
#define _UT_MBM_UDP_STUBS_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/
xMBPUDPHandle   xMBMTestGetLastHandle( void );
void            vMBMTestRequestTransmit( xMBPUDPHandle xUDPHdl, UBYTE arubExpectedRequest[],
                                         USHORT usExpectedRequestLen );
void            vMBMTestPreparedResponse( xMBPUDPHandle xUDPHdl, UBYTE arubPreparedResponse[],
                                          USHORT usPreparedResponseLen );
void            vMBMTestFailOnWrite( xMBPUDPHandle xUDPHdl, BOOL bFail );
void            vMBMTestFailOnRead( xMBPUDPHandle xUDPHdl, BOOL bFail );
void			vMBMTestFailOnInit( BOOL bFail );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
