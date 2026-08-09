/*
 * MODBUS Library: MODBUS frame format. Shared by MODBUS master and slave.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbframe.h,v 1.7 2012-01-08 20:04:28 embedded-solutions.cwalter Exp $
 */

#ifndef _MB_FRAME_H
#define _MB_FRAME_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/*!
 * \if INTERNAL_DOCS
 * \addtogroup mb_cmn_int
 * @{
 * \endif
 */

/* ----------------------- Defines ------------------------------------------*/
#define MB_PDU_SIZE_MAX             ( 253 )     /*!< Maximum size of a PDU. */
#define MB_PDU_SIZE_MIN             ( 1 )       /*!< Function Code */
#define MB_PDU_FUNC_OFF             ( 0 )       /*!< Offset of function code in PDU. */
#define MB_PDU_EX_CODE_OFF          ( 1 )       /*!< Offset of exception code in PDU. */
#define MB_PDU_DATA_OFF             ( 1 )       /*!< Offset for response data in PDU. */
#define MB_PDU_EX_RESP_SIZE         ( 2 )       /*!< Size of an exception response. */
#define MB_SER_BROADCAST_ADDR       ( 0 )       /*!< MODBUS serial broadcast address. */
#define MB_SER_SLAVE_ADDR_MIN       ( 1 )       /*!< Smallest valid serial address. */
#define MB_SER_SLAVE_ADDR_MAX       ( 247 )     /*!< Biggest valid serial address. */
#define MB_ANY_ADDR                 ( 255 )     /*!< Special address sometimes used in Modbus/TCP. */

/*! \brief Test if a MODBUS function byte in a MODBUS PDU is an exception.
 * \internal
 *
 * \param ubFuncByte The byte in the MODBUS PDU.
 * \return \c TRUE if this function code is an exception. Otherwise \c FALSE.
 */
#define MB_PDU_FUNC_ISEXCEPTION( ubFuncByte ) ( ( ( ubFuncByte ) & 0x80 ) != 0 )

/*! \brief Test if a MODBUS function byte in a MODBUS PDU is an exception
 *   for a specific exception.
 * \internal
 *
 * \param ubFuncByte The byte in the MODBUS PDU.
 * \param ubFuncExpected Any MODBUS function code less than 128.
 * \return \c TRUE if this function code is an exception. Otherwise \c FALSE.
 */
#define MB_PDU_FUNC_ISEXCEPTION_FOR( ubFuncByte, ubFuncExpected ) \
    ( ( ubFuncByte ) == ( ( ubFuncExpected ) + 0x80 ) )

/*! \brief Check if the slave address ucAddr can be used for a transaction
 *  requiring an reply.
 * \internal
 *
 * \param ucAddr the slave address.
 * \return \c TRUE if ucAddr is a valid slave address but not the broadcast
 *   address.
 */
#define MB_IS_VALID_READ_ADDR( ucAddr ) \
    ( ( ( ucAddr ) >= MB_SER_SLAVE_ADDR_MIN ) && ( ( ucAddr ) <= MB_SER_SLAVE_ADDR_MAX ) )

/*! \brief Check if the slave address ucAddr can be used for a transaction
    *   not necessarily requiring an reply.
    * \internal
    *
    * \param ucAddr the slave address.
    * \return \c TRUE if ucAddr is a valid slave address including the broadcast
    *   address.
    */
#define MB_IS_VALID_WRITE_ADDR( ucAddr ) \
    ( ( ( ucAddr ) == MB_SER_BROADCAST_ADDR ) || MB_IS_VALID_READ_ADDR( ucAddr ) )

/*! \brief Check if the slave address ucAddr can be used for the unit identifier field
 *    for a write transaction.
 * \internal
 *
 * \param ucAddr the slave address.
 * \return \c TRUE if ucAddr is a valid slave address
 */
#define MB_IS_VALID_TCP_ADDR( ucAddr ) \
	( ( ( ( ucAddr ) >= MB_SER_SLAVE_ADDR_MIN ) && ( ( ucAddr ) <= MB_SER_SLAVE_ADDR_MAX ) )|| ( MB_ANY_ADDR == ( ucAddr ) ) || ( MB_SER_BROADCAST_ADDR == ( ucAddr ) ) )

/* ----------------------- Type definitions ---------------------------------*/

/*! \brief MODBUS exception defined for the MODBUS PDU.
 * \ingroup mb_cmn
 */
typedef enum
{
    MB_PDU_EX_NONE = 0x00,
    MB_PDU_EX_ILLEGAL_FUNCTION = 0x01,
    MB_PDU_EX_ILLEGAL_DATA_ADDRESS = 0x02,
    MB_PDU_EX_ILLEGAL_DATA_VALUE = 0x03,
    MB_PDU_EX_SLAVE_DEVICE_FAILURE = 0x04,
    MB_PDU_EX_ACKNOWLEDGE = 0x05,
    MB_PDU_EX_SLAVE_BUSY = 0x06,
    MB_PDU_EX_NOT_ACKNOWLEDGE = 0x07,
    MB_PDU_EX_MEMORY_PARITY_ERROR = 0x08,
    MB_PDU_EX_GATEWAY_PATH_UNAVAILABLE = 0x0A,
    MB_PDU_EX_GATEWAY_TARGET_FAILED = 0x0B
} eMBException;

/* ----------------------- Function prototypes ------------------------------*/

/*!
 * \if INTERNAL_DOCS
 * @}
 * \endif
 */

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
