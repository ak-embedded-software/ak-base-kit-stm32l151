/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbmcrc.h,v 1.7 2009-01-03 11:29:02 cwalter Exp $
 */


#ifndef _MBM_CRC_H
#define _MBM_CRC_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/*! 
 * \if INTERNAL_DOCS
 * \addtogroup mbm_rtu_int
 * @{
 * \endif
 */

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/

/*! \brief Calculates the CRC16 checksum of a frame of length usLen and
 *   returns it.
 * \internal
 *
 * \param pucFrame A pointer to a frame of length \c usLen.
 * \param usLen Size of the frame to check.
 * \return the CRC16 checksum for this frame.
 */
USHORT usMBMCRC16( const UBYTE * pucFrame, USHORT usLen );

/*! 
 * \if INTERNAL_DOCS
 * @} 
 * \endif
 */

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
