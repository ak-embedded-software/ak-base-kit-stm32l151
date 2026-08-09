/* 
 * ModbusMaster Libary: A portable MODBUS master for MODBUS ASCII/RTU/TCP/UDP.
 * Copyright (c) 2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmudp.h,v 1.2 2011-12-04 20:51:59 embedded-solutions.cwalter Exp $
 */

#ifndef _MBM_UDP_H
#define _MBM_UDP_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

#if defined( DOXYGEN ) || ( MBM_UDP_ENABLED == 1 )
/*! 
 * \if INTERNAL_DOCS
 * \addtogroup mbm_udp_int
 * @{
 * \endif
 */

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/
eMBErrorCode eMBMUDPFrameInit( xMBMInternalHandle * pxIntHdl, const CHAR * pcUDPBindAddress, LONG uUDPListenPort );

eMBErrorCode eMBMUDPFrameSetClient( xMBMInternalHandle * pxIntHdl, const CHAR * pcUDPClientAddress, USHORT usUDPPort );

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
