/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm_rtu_stubs.h,v 1.6 2007-08-17 23:34:01 cwalter Exp $
 */

#ifndef _UT_MBM_RTU_STUBS_H
#define _UT_MBM_RTU_STUBS_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/

void            vMBMTestSetExpectedSerialInit( UCHAR ucExpectedPort, ULONG ulExpectedBaudRate,
                                               UCHAR ucExpectedDataBits, UCHAR ucExpectedStopBits,
                                               eMBSerialParity eExpectedParity );

void            vMBMTestSetFailOnInit( BOOL bFail );

void            vMBMTestRequestTransmit( UCHAR ucExpectedPort, UBYTE arubExpectedRequest[],
                                         USHORT usExpectedRequestLen );
void            vMBMTestPreparedResponse( UCHAR ucExpectedPortArg, UBYTE arubPreparedResponse[],
                                          USHORT usPreparedResponseLen );
void            vMBMTestSetResponseTimeoutOnRx( UCHAR ucExpectedPortArg, BOOL bTimeout );

void            vMBMTestSetResponseViolateAPI( UCHAR ucExpectedPortArg, BOOL bViolate );
void            vMBMTestSetReequestViolateAPI( UCHAR ucExpectedPortArg, BOOL bViolate );

void            vMBMTestFailOnTxStart( UCHAR ucExpectedPort, BOOL bFail );

void            vMBMTestFailOnRxStart( UCHAR ucExpectedPort, BOOL bFail );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
