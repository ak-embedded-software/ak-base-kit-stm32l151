/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: ut_mbm.h,v 1.5 2007-08-17 23:34:01 cwalter Exp $
 */


#ifndef _UT_MBM_H
#define _UT_MBM_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/

eMBErrorCode    eMBMTestInit( xMBMHandle * pxHdl );

void            eMBMTestSetExpectedRequest( const xMBMHandle xHdl, /*@unique@ */ const UBYTE arubExpectedRequestArg[],
                                            USHORT usExpectedRequestLen );

void            eMBMTestSetPreparedResponse( const xMBMHandle xHdl, /*@unique@ */ const UBYTE arubPreparedResponseArg[],
                                             USHORT usPreparedResponseLen );

eMBErrorCode    eMBPTimerSetFailOnSetTimeout( xMBPTimerHandle xTimerHdl, BOOL bFail );
eMBErrorCode    eMBPTimerSetFailOnStart( xMBPTimerHandle xTimerHdl, BOOL bFail );
eMBErrorCode    eMBPTimerSetFailOnStop( xMBPTimerHandle xTimerHdl, BOOL bFail );
void            vMBPTimerFailOnInit( BOOL bFail );

eMBErrorCode    eMBPEventFailOnPost( xMBPEventHandle xEventHdl, BOOL bFail );
void            vMBPEventFailOnInit( BOOL bFail );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
