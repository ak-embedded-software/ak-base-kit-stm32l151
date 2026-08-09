/* 
 * ModbusMaster Libary: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmtcp.h,v 1.2 2008-03-06 10:46:48 cwalter Exp $
 */

#ifndef _MBM_TCP_H
#define _MBM_TCP_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

#if defined( DOXYGEN ) || ( MBM_TCP_ENABLED == 1 )
/*! 
 * \if INTERNAL_DOCS
 * \addtogroup mbm_tcp_int
 * @{
 * \endif
 */
 
/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/
/*! \brief Configure a MODBUS master TCP instance.
 * \internal
 *
 * \param pxIntHdl An internal handle for the MODBUS master stack.
 *
 * \return eMBErrorCode::MB_ENOERR if a new instances has been created. In 
 *   this case the members pxFrameHdl, pFrameSendFN, pFrameRecvFN and 
 *   pFrameCloseFN in the handle are updated to point to this RTU instances.
 *   If pxIntHdl equals \c NULL or the baudrate is zero it returns 
 *   eMBErrorCode::MB_EINVAL. Otherwise the error code depends on the porting
 *   layer.
 */
eMBErrorCode eMBMTCPFrameInit(  /*@shared@ */ xMBMInternalHandle * pxIntHdl );

/*! \brief Connect the MODBUS master TCP instance to a client.
 * \internal
 *
 * \param pxIntHdl An internal handle for the MODBUS master stack.
 * \param pcTCPClientAddress The client address.
 * \param usTCPPort The TCP port to connect to.
 * \return eMBErrorCode::MB_ENOERR if the client connection is open and
 *   ready for use. eMBErrorCode::MB_EIO if the client connection could
 *   not be established. In case of an fatal error eMBErrorCode::MB_EILLSTATE.
 *   In the last case this instance is not usable anymore.
 */
eMBErrorCode
eMBMTCPFrameConnect(  /*@shared@ */ xMBMInternalHandle * pxIntHdl, const CHAR * pcTCPClientAddress, USHORT usTCPPort );

/*! \brief Disconnect a MODBUS master client connection.
 * \internal
 * 
 * \param pxIntHdl An internal handle for the MODBUS master stack.
 * \return eMBErrorCode::MB_ENOERR if the connection has been freed. In
 *   case of an fatal error eMBErrorCode::MB_EILLSTATE. In the last case
 *   this instance is not usable anymore.
 */
eMBErrorCode    eMBMTCPFrameDisconnect(  /*@shared@ */ xMBMInternalHandle * pxIntHdl );

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
