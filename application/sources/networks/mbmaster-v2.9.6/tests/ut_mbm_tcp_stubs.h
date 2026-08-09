/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_tcp_stubs.h,v 1.2 2008-03-06 10:48:11 cwalter Exp $
 */

#ifndef _UT_MBM_TCP_STUBS_H
#define _UT_MBM_TCP_STUBS_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/
xMBPTCPHandle   xMBMTestGetLastHandle( void );
void            vMBMTestRequestTransmit( xMBPTCPHandle xTCPHdl, UBYTE arubExpectedRequest[],
                                         USHORT usExpectedRequestLen );
void            vMBMTestPreparedResponse( xMBPTCPHandle xTCPHdl, UBYTE arubPreparedResponse[],
                                          USHORT usPreparedResponseLen );
void            VMBMTestFailOnWrite( xMBPTCPHandle xTCPHdl, BOOL bFail );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
