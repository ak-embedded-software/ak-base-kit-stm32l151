/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbmiframe.h,v 1.13 2013-05-21 21:04:10 embedded-solutions.cwalter Exp $
 */


#ifndef _MBM_FRAME_H
#define _MBM_FRAME_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/*! 
 * \if INTERNAL_DOCS
 * \addtogroup mbm_int
 * @{
 * \endif
 */

/* ----------------------- Defines ------------------------------------------*/

#define MBM_FRAME_HANDLE_INVALID    ( NULL )    /*!< An invalid frame handle. */

/* ----------------------- Type definitions ---------------------------------*/

/*! \brief A handle for a frame handler instance.
 * \internal
 */
typedef void   *xMBMFrameHandle;

/*! \brief This function check if a transmission is still in progress.
 *
 * \param xHdl A handle to the stack
 *
 * \return FALSE if no transmission is in progress. Otherwise TRUE.
 */
typedef		BOOL( *peMBMFrameIsTransmitting )( xMBMHandle xHdl );

/*! \brief This function is called when the stack has prepared the MODBUS request.
 * \internal
 *
 * Before this function is called the MODBUS stack has prepared the buffer 
 * pxIntHdl->pubFrameMBPDUBuffer. The size of the MODBUS PDU is usMBPDULength. This
 * function is then responsible for adding the header and the trailer depending
 * on the transmission mode used. For example in MODBUS serial in RTU mode it
 * would add the slave address to the beginning and the CRC16 checksum to
 * the end.
 *
 * \param xHdl A handle to the stack.
 * \param ucSlaveAddress The slave to which this request should be sent.
 * \param usMBPDULength The length of the MODBUS PDU.
 *
 * \return eMBErrorCode::MB_ENOERR is the sending has been started.
 */
typedef         eMBErrorCode( *peMBMFrameSend ) ( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usMBPDULength );

/*! \brief This function is called by the stack when it wants to process the
 *  received frame.
 *
 * If the stack is notified by the frame handler that a new frame has been
 * received it calls this functions to retrieve the frame. The frame is then
 * processed as the MODBUS slave response.
 *
 * \param xHdl A handle to the stack.
 * \param ucSlaveAddress The slave which should have created the response.
 * \param pusMBPDULength If \c NULL the stack does not want to retrieve the data.
 *   In this case the layer should reset itself such that it can send another
 *   frames. If not \c NULL the frame handler should update the pointer to
 *   hold the number of bytes received.
 *
 * \return eMBErrorCode::MB_ENOERR if a frame has been received or the frame
 *   handler has reset itself. In case of an I/O error is returns eMBErrorCode::MB_EIO.
 */
typedef         eMBErrorCode( *peMBMFrameReceive ) ( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT * pusMBPDULength );

/*! \brief This functions closes the frame handler.
 *
 * \param xHdl A handle to the stack.
 * \return eMBErrorCode::MB_ENOERR if the frame handler has been closed.
 */
typedef         eMBErrorCode( *peMBMFrameClose ) ( xMBMHandle xHdl );

/*! 
 * \if INTERNAL_DOCS
 * @} 
 * \endif
 */

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
