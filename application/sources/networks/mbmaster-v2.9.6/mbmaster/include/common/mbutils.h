/*
 * MODBUS Library: Common utilities shared by master and slave stack.
 *
 * Copyright (c) 2007-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbutils.h,v 1.14 2011-12-13 23:45:24 embedded-solutions.cwalter Exp $
 */

#ifndef _MB_UTILS_H
#define _MB_UTILS_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif
/* ----------------------- Defines ------------------------------------------*/

/*!
 * \if INTERNAL_DOCS
 * \addtogroup mb_cmn_int
 * @{
 * \endif
 */

/*! \brief Rounds an integer number to the next multiple of 2.
 * \internal
 *
 * \param num The number to round.
 */
#define MB_CEIL_2( num )       			( ( ( ( num ) + 1 ) / 2 ) * 2 )

/* \brief Perform integer division where the result is rounded
 *  toward +infinity.
 * \internal
 *
 * \param dividend Divided
 * \param divisor  Divisor
 */
#define MB_INTDIV_CEIL( dividend, divisor )		\
	( ( ( dividend ) + ( divisor ) - 1 ) / ( divisor ) )

/* \brief Perform integer division where the result is rounded
 *   toward the nearest integer number.
 * \internal
 *
 * \param dividend Divided
 * \param divisor  Divisor
 */
#define MB_INTDIV_ROUND( dividend, divisor )	\
	( ( ( dividend ) + ( divisor ) / 2 ) / ( divisor ) )

/* \brief Perform integer division where the result is rounded
 *   toward -infinity.
 * \internal
 *
 * \param dividend Divided
 * \param divisor  Divisor
 */
#define MB_INTDIV_FLOOR( dividend, divisor )	\
	( ( dividend ) / ( divisor ) )

/*! \brief Wrap around index in ring buffer
 * \internal
 */
#define MB_UTILS_RINGBUFFER_INCREMENT( idx, buffer ) do { \
    idx++; \
    if( idx >= MB_UTILS_NARRSIZE( buffer ) ) { \
        idx = 0; \
    } \
} while( 0 )

/*! \brief Calculate the number of elements in an array which size is known
 *   at compile time.
 * \internal
 * \param x The array.
 */

#define MB_UTILS_NARRSIZE( x ) ( sizeof( x ) / sizeof( ( x )[ 0 ] ) )
/*! \brief Checks if a handle is valid.
 * \internal
 *
 * This method checks if a handle is valid. It uses the ubIdx index to
 * check if the pointer points to a valid handle.
 *
 * \param pxHdl A pointer to a handle.
 * \param arxHdl An array of handles.
 * \return A boolean value.
 */
#if ( ( defined( MBM_ENABLE_FULL_API_CHECKS ) && ( MBM_ENABLE_FULL_API_CHECKS == 1 ) ) || \
      ( defined( MBS_ENABLE_FULL_API_CHECKS ) && ( MBS_ENABLE_FULL_API_CHECKS == 1 ) ) )
#define MB_IS_VALID_HDL( pxHdl, arxHdl ) \
    ( ( ( pxHdl ) != NULL ) && \
      ( ( ( size_t )( ( pxHdl )->ubIdx ) ) < MB_UTILS_NARRSIZE( arxHdl ) ) && \
      ( ( pxHdl ) == &arxHdl[ ( pxHdl )->ubIdx ] ) )
#else
#define MB_IS_VALID_HDL( pxHdl, arxHdl ) \
    ( ( ( pxHdl ) != NULL ) && ( pxHdl ) == &arxHdl[ ( pxHdl )->ubIdx ] )
#endif
/*!
 * \if INTERNAL_DOCS
 * @}
 * \endif
 */

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/

/*! \brief Map a MODBUS exception code to an application error code.
 * \ingroup mb_cmn
 *
 * \param ubMBException a MODBUS exception code from a MODBUS frame.
 * \return The MODBUS exception converted to an application error.
 */
    eMBErrorCode eMBExceptionToErrorcode( UBYTE ubMBException );

/*! \brief Map an application exception to a MODBUS exception.
 * \ingroup mb_cmn
 *
 * \param eCode Any of the eMBErrorCode values. Non exceptional
 *   values are mapped to eMBException::MB_PDU_EX_SLAVE_DEVICE_FAILURE.
 * \return The exception converted to a MODBUS PDU exception.
 */
eMBException    eMBErrorcodeToException( eMBErrorCode eCode );

/*! \brief Check if the buffer contents is a standard MODBUS frame
 * \ingroup mb_cmn
 *
 * This buffer quickly inspects a MODBUS frame and checks if it
 * is a valid frame structure. It is used to implement quicker RTU
 * timeouts on platforms where the timers have not a sufficient 
 * resolution. E.g. WIN32 and/or LINUX.
 *
 * \return TRUE if the frame contents is a typical MODBUS frame, e.g.
 *   corresponds to a standard request/response. If FALSE it is either
 *   not a MODBUS frame or a custom function code one/
 */
BOOL
bMBGuessRTUFrameIsComplete( UBYTE arubBuffer[], USHORT usLength );

/*! \brief CRC16 table implementation
 */
USHORT
usUtlCRC16GetTab( UBYTE ubIdx );

/*! \brief On-the-fly CRC16 calculation
 *
 * This function shall be initially called with usCRC set to 0xFFFFU.
 * If the frame has a valid CRC16 then if all bytes have been processed
 * the final calculated value is 0x0000.
 *
 * \param usCRC Currently calculated checksum. Initialize with 0xFFFFU.
 * \param ubData New byte used to update CRC value.
 * \return Newly calculated checksum.
 */
USHORT 
prvCRC16Update( USHORT usCRC, UBYTE ubData );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
