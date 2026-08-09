/* 
 * ModbusMaster Libary: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmrtu.h,v 1.12 2011-11-22 00:38:33 embedded-solutions.cwalter Exp $
 */

#ifndef _MBM_RTU_H
#define _MBM_RTU_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

#if MBM_RTU_ENABLED == 1

/*! 
 * \if INTERNAL_DOCS
 * \addtogroup mbm_rtu_int
 * @{
 * \endif
 */

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/

/*! \brief Configure a MODBUS master RTU instances.
 * \internal
 *
 * \param pxIntHdl An internal handle.
 * \param ucPort The port. This value is passed through to the porting layer.
 * \param ulBaudRate Baudrate.
 * \param eParity Parity.
 * \param ucStopBits Number of stopbits to use.
 *
 * \return eMBErrorCode::MB_ENOERR if a new instances has been created. In 
 *   this case the members pxFrameHdl, pFrameSendFN, pFrameRecvFN and 
 *   pFrameCloseFN in the handle are updated to point to this RTU instances.
 *   If pxIntHdl equals \c NULL or the baudrate is zero it returns 
 *   eMBErrorCode::MB_EINVAL. Otherwise the error code depends on the porting
 *   layer.
 */
eMBErrorCode
eMBMSerialRTUInit( xMBMInternalHandle * pxIntHdl, UCHAR ucPort, ULONG ulBaudRate, eMBSerialParity eParity, UCHAR ucStopBits );

/*! 
 * \if INTERNAL_DOCS
 * @}
 * \endif
 */

#endif

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
