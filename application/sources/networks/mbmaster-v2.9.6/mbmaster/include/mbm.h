/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbm.h,v 1.36 2011-11-22 00:36:55 embedded-solutions.cwalter Exp $
 */

#ifndef _MBM_H
#define _MBM_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif
/*! \addtogroup mbm
 * @{
 */
    /* ----------------------- Modbus includes ---------------------------------- */
#if defined( MBM_APIHEADERS_ONLY ) && ( MBM_APIHEADERS_ONLY == 1 )
#if defined( MBM_WIN32_DLLIMPORT ) && ( MBM_WIN32_DLLIMPORT == 1 )
#define _DLLEXP __declspec(dllimport)
#else
#define _DLLEXP
#endif
#include "mbmconfig.h"
#include "mbmiconfig.h"
#include "mbtypes.h"
#else
#if defined( MBM_WIN32_DLLEXPORT ) && ( MBM_WIN32_DLLEXPORT == 1 )
#define _DLLEXP __declspec(dllexport)
#else
#define _DLLEXP
#endif
#include "mbmconfig.h"
#include "internal/mbmiconfig.h"
#include "common/mbtypes.h"
#endif
/* ----------------------- Defines ------------------------------------------*/
/* ----------------------- Type definitions ---------------------------------*/
/*! \brief A handle to a MODBUS master instance. */
typedef void   *xMBMHandle;

/*! \brief States used by the MODBUS stack internally. 
 *
 * \note These values are not important for the user and should not be accessed
 *   or used. They are used by the polled versions of the API calls.
 */
typedef enum
{
    MBM_STATE_NONE,             /*!< Not yet started. */
    MBM_STATE_SEND,             /*!< Frame will be sent. */
    MBM_STATE_WAITING,          /*!< Waiting for an event. */
    MBM_STATE_DISASSEMBLE,      /*!< Disassembling the frame. */
    MBM_STATE_ERROR,            /*!< An error occurred. */
    MBM_STATE_DONE              /*!< We are done processing the request. */
} eMBMQueryState;

#if MBM_FUNC_RD_FILES_ENABLED == 1
typedef struct
{
    USHORT          usFileNumber;       /*!< File number. 0x0001 - 0xFFFF */
    USHORT          usRecordNumber;     /*!< Record number. 0x0000 - 0x270F */
    USHORT          usRecordLength;     /*!< Record length. */
} xMBMFileSubReadReq_t;

typedef struct
{
    USHORT          usResponseLength;   /*!< Number of bytes in pubResponse array (always odd but not necessarily aligned */
    UBYTE          *pubResponse;
} xMBMFileSubReadResp_t;
#endif

#if MBM_FUNC_WR_FILES_ENABLED == 1
typedef struct
{
    USHORT          usFileNumber;       /*!< File number. 0x0001 - 0xFFFF */
    USHORT          usRecordNumber;     /*!< Record number. 0x0000 - 0x270F */
    USHORT          usRecordLength;     /*!< Number of registers in pubRecordData. */
    UBYTE          *pubRecordData;      /*!< Size of pubRecordData is two times usRecordLength. */
} xMBMFileSubWriteReq_t;
#endif

/* ----------------------- Function prototypes ------------------------------*/

/*! \brief Set the slave timeout for the next request. 
 *
 * \param xHdl A valid MODBUS master handle.
 * \param usNMilliSeconds The new slave device timeout. If no response is
 *   received within this time window after a MODBUS request has been sent to
 *   the slave the API functions will return eMBErrorCode::MB_ETIMEDOUT to
 *   indicate an timeout error.
 * \return eMBErrorCode::MB_ENOERR if the new slave timeout will be used
 *   on the next request.
 */
_DLLEXP eMBErrorCode eMBMSetSlaveTimeout( xMBMHandle xHdl, USHORT usNMilliSeconds );

/*! \brief Close the stack. 
 *
 * Shutdown the master stack. This function should not be called when there
 * are still pending requests.
 *
 * \param xHdl A handle for a MODBUS master instances. 
 * \return eMBErrorCode::MB_ENOERR if the stack has been shut down. 
 */
_DLLEXP eMBErrorCode eMBMClose( xMBMHandle xHdl );

#if defined( DOXYGEN ) || ( MBM_ENABLE_STATISTICS_INTERFACE == 1 )
/*! \brief Retrieve the current master statistics.
 *
 * This function populates the argument pxMBMCurrentStat with the
 * current internal counters.
 *
 * \param xHdl A valid MODBUS master handle. 
 * \param pxMBMCurrentStat A pointer to an (potentially unitialized)
 *  eMBMStat datastructure. When the return value is 
 *  eMBErrorCode::MB_ENOERR this data structure holds a copy
 *  of the internal counters.
 * \return eMBErrorCode::MB_ENOERR if successful. In case of an
 *  invalid argument the function returns eMBErrorCode::MB_EINVAL.
 */
_DLLEXP eMBErrorCode eMBMGetStatistics( xMBMHandle xHdl, xMBStat * pxMBMCurrentStat );

/*! \brief Clears the current statistic counters
 *
 * \param xHdl A valid MODBUS master handle.
 * \return eMBErrorCode::MB_ENOERR if successful. In case of an
 *  invalid argument the function returns eMBErrorCode::MB_EINVAL.
 */
_DLLEXP eMBErrorCode eMBMResetStatistics( xMBMHandle xHdl );
#endif

#if defined( DOXYGEN ) || ( MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1 )
/*! \brief Register an protocol analyzer.
 *
 * If a protocol analyzer has been registered a callback is made
 * whenever a frame has been sent or received.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param pvMBAnalyzerCallbackFN A valid pointer to a callback
 *  handler or \c NULL if the analyzer should be removed.
 * \param pvCtxArg A user defined context. Can be \c NULL.
 * \return eMBErrorCode::MB_ENOERR if the analyzer has been added
 *  or removed. eMBErrorCode::MB_EINVAL in case of an invalid MODBUS
 *  handle.
 */
_DLLEXP eMBErrorCode eMBMRegisterProtAnalyzer( xMBMHandle xHdl, void *pvCtxArg,
                                               pvMBAnalyzerCallbackCB pvMBAnalyzerCallbackFN );
#endif

/* ----------------------- Function prototypes ( Blocking ) -----------------*/

#if MBM_FUNC_RD_HOLDING_ENABLED == 1
/*! \brief <em>Read Holding Registers</em> from a slave using the MODBUS 
 *   function code <b>0x03</b>
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address. Note that a broadcast address is not
 *   allowed for a function which expects a response.
 * \param usRegStartAddress The first holding register to be read. We use
 *   protocol addresses starting at zero.
 * \param ubNRegs Number of registers to read.
 * \param arusBufferOut An array of USHORT values of at least ubNRegs 
 *   elements.
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. In this 
 *   case the array arusBufferOut contains the values returned by the slave.
 *   In case of an exception from the slave any of the MODBUS exceptions 
 *   can be returned. If the slave did not respond within the timeout the
 *   function returns eMBErrorCode::MB_ETIMEDOUT. Any other errors are IO 
 *   errors.
 */
_DLLEXP eMBErrorCode eMBMReadHoldingRegisters( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                               USHORT usRegStartAddress, UBYTE ubNRegs, USHORT arusBufferOut[] );
#endif

#if MBM_FUNC_WR_SINGLE_REG_ENABLED == 1
/*! \brief <em>Write Single Register</em> in a slave using the MODBUS function
 *   code <b>0x06</b>.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address. Note that a broadcast address is not
 *   allowed for a function which expects a response.
 * \param usRegAddress The register address to write.
 * \param usValue The value which should be written to the register.
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. In this 
 *   case the array arusBufferOut contains the values returned by the slave.
 *   In case of an exception from the slave any of the MODBUS exceptions 
 *   can be returned. If the slave did not respond within the timeout the
 *   function returns eMBErrorCode::MB_ETIMEDOUT. Any other errors are IO 
 *   errors.
 */
_DLLEXP eMBErrorCode eMBMWriteSingleRegister( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usRegAddress,
                                              USHORT usValue );
#endif

#if MBM_FUNC_RD_INPUT_REGS_ENABLED == 1
/*! \brief <em>Read Input Registers</em> from a slave using the MODBUS function
 *   code <b>0x04</b>.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address. Not that a broadcast address is not
 *    allowed.
 * \param usRegStartAddress First register to read.
 * \param ubNRegs Numer of registers to read.
 * \param arusBufferOut An array of USHORT values of at least ubNRegs 
 *   elements.
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. In this 
 *   case the array arusBufferOut contains the values returned by the slave.
 *   In case of an exception from the slave any of the MODBUS exceptions 
 *   can be returned. If the slave did not respond within the timeout the
 *   function returns eMBErrorCode::MB_ETIMEDOUT. Any other errors are IO 
 *   errors.
 */
_DLLEXP eMBErrorCode eMBMReadInputRegisters( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usRegStartAddress,
                                             UBYTE ubNRegs, USHORT arusBufferOut[] );
#endif

#if MBM_FUNC_WR_MUL_REGS_ENABLED == 1
/*! \brief <em>Write Multiple Registers</em> from a slave using the MODBUS
 *   function code <b>0x10</b>.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usRegStartAddress First register to write to.
 * \param ubNRegs Number of registers to write.
 * \param arusBufferIn An Array of USHORT values of at least ubNRegs elements.
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. In this 
 *   case the array arusBufferOut contains the values returned by the slave.
 *   In case of an exception from the slave any of the MODBUS exceptions 
 *   can be returned. If the slave did not respond within the timeout the
 *   function returns eMBErrorCode::MB_ETIMEDOUT. Any other errors are IO 
 *   errors.
 */
_DLLEXP eMBErrorCode eMBMWriteMultipleRegisters( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usRegStartAddress,
                                                 UBYTE ubNRegs, const USHORT arusBufferIn[] );

#endif

#if MBM_FUNC_RDWR_MUL_REGS_ENABLED == 1
/*! \brief <em>Read/Write Multiple Registers</em> from a slave using the MODBUS
 *   function code <b>0x17</b>
 *
 * \note The pointers arusBufferIn and arusBufferOut can point to the same 
 *   memory location. In this case the old contents is overwritten with
 *   the returned registers values.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usWriteRegStartAddress First register to write on the slave.
 * \param ubWriteNRegs Number of registers to write.
 * \param arusBufferIn An Array of USHORT values of ubWriteNRegs elements 
 *   which should be written on the slave.
 * \param usReadRegStartAddress First register to read from the slave.
 * \param ubReadNRegs Number of registers to read.
 * \param arusBufferOut An Array of ubReadNRegs elements where the registers
 *   values returned by the slave should be stored.
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. In this 
 *   case the array arusBufferOut contains the values returned by the slave.
 *   In case of an exception from the slave any of the MODBUS exceptions 
 *   can be returned. If the slave did not respond within the timeout the
 *   function returns eMBErrorCode::MB_ETIMEDOUT. Any other errors are IO 
 *   errors.
 */
_DLLEXP eMBErrorCode eMBMReadWriteMultipleRegisters( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                                     USHORT usWriteRegStartAddress, UBYTE ubWriteNRegs,
                                                     const USHORT arusBufferIn[],
                                                     USHORT usReadRegStartAddress, UBYTE ubReadNRegs,
                                                     USHORT arusBufferOut[] );
#endif

#if MBM_FUNC_READ_DISC_INPUTS_ENABLED == 1
/*! \brief <em>Read Discrete Inputs</em> from a slave using the MODBUS
 *   function code <b>0x02</b>
 *
 * The discrete inputs are packed as one input per bit. Statis is 1=ON and 0=OFF.
 * The LSB of the first data byte contains the input addressed in the query. The
 * other inputs follow toward the high order of the byte. If the input quantity
 * is not a multiple of eight the final data byte is padded with zeros.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usInputStartAddress Address of first discrete input.
 * \param usNInputs Number of discrete inputs to read.
 * \param arubBufferOut An array with a size of at least usNInputs/8 bytes.
 *   The size must be rounded up to next integer.
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. In this 
 *   case the array arubBufferIn contains the values returned by the slave.
 *   In case of an exception from the slave any of the MODBUS exceptions 
 *   can be returned. If the slave did not respond within the timeout the
 *   function returns eMBErrorCode::MB_ETIMEDOUT. 
 */
_DLLEXP eMBErrorCode eMBMReadDiscreteInputs( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                             USHORT usInputStartAddress, USHORT usNInputs, UBYTE arubBufferOut[] );
#endif

#if MBM_FUNC_READ_COILS_ENABLED == 1
/*! \brief <em>Read Coils</em> from a slave using the MODBUS
 *   function code <b>0x01</b>
 *
 * The coils are packed as one coil per bit. Statis is 1=ON and 0=OFF. The LSB 
 * of the first data byte contains the coil addressed in the query. The other 
 * coils follow toward the high order of the byte. If the input quantity
 * is not a multiple of eight the final data byte is padded with zeros.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usCoilStartAddress Address of first coil.
 * \param usNCoils Number of coils to read.
 * \param arubBufferOut An array with a size of at least usNCoils/8 bytes.
 *   The size must be rounded up to next integer.
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. In this 
 *   case the array arubBufferIn contains the values returned by the slave.
 *   In case of an exception from the slave any of the MODBUS exceptions 
 *   can be returned. If the slave did not respond within the timeout the
 *   function returns eMBErrorCode::MB_ETIMEDOUT. 
 */
_DLLEXP eMBErrorCode eMBMReadCoils( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usCoilStartAddress, USHORT usNCoils,
                                    UBYTE arubBufferOut[] );
#endif

#if MBM_FUNC_WRITE_SINGLE_COIL_ENABLED == 1
/*! \brief <em>Write Coil</em> on slave using the MODBUS
 *   function code <b>0x05</b>
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usOutputAddress Address of coil.
 * \param bOn Either \c TRUE or \c FALSE.
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. In case of 
 *   an exception from the slave any of the MODBUS exceptions can be returned. 
 *   If the slave did not respond within the timeout the function returns
 *   eMBErrorCode::MB_ETIMEDOUT. 
 */
_DLLEXP eMBErrorCode eMBMWriteSingleCoil( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usOutputAddress, BOOL bOn );
#endif

#if MBM_FUNC_WRITE_COILS_ENABLED == 1
/*! \brief <em>Write Coils</em> from a slave using the MODBUS
 *   function code <b>0x0F</b>
 *
 * The coils are packed as one coil per bit. Statis is 1=ON and 0=OFF. The LSB 
 * of the first data byte contains the coil addressed in the query. The other 
 * coils follow toward the high order of the byte. 
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usCoilStartAddress Address of first coil.
 * \param usNCoils Number of coils to read.
 * \param arubCoilsIn An array with a size of at least usNCoils/8 bytes.
 *   The size must be rounded up to next integer. A
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. In this 
 *   case the array arubBufferIn contains the values returned by the slave.
 *   In case of an exception from the slave any of the MODBUS exceptions 
 *   can be returned. If the slave did not respond within the timeout the
 *   function returns eMBErrorCode::MB_ETIMEDOUT. 
 */
_DLLEXP eMBErrorCode eMBMWriteCoils( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usCoilStartAddress, USHORT usNCoils,
                                     const UBYTE arubCoilsIn[] );
#endif

#if MBM_FUNC_READWRITE_RAWPDU_ENABLED == 1
/*! \brief A function for sending raw MODBUS PDUs to a MODBUS slave. This
 *   function can be used if you want to transport custom data over a
 *   MODBUS connection or for briding other protocols. Please note that
 *   of course most standard slaves won't know how to deal with these
 *   custom data payload and therefore its use should be limited.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param ucFunctionCode MODBUS function code used for transport. Must
 *   be between 1 and 127. Recommend values are between 65-72 and 100-110.
 * \param arubPayloadIn MODBUS request which will be sent to the slave.
 * \param ubPayloadInLength Length of MODBUS request. Maximum is 252 bytes.
 * \param arubPayloadOut Holds the MODBUS response sent by the slave.
 *   arubPayloadOut can be \c NULL is ubPayloadOutLengthMax is set to
 *   zero.
 * \param ubPayloadOutLengthMax Size of the buffer arubPayloadOut in bytes.
 *   If the slave response exceeds the number of bytes available in the
 *   buffer the function returns eMBErrorCode::MB_EINVAL.
 * \param pubPayloadOutLength Actual number of bytes returned by the
 *   slave.
 * \return eMBErrorCode::MB_ENOERR if the slave responded within the timeout
 *   and the the reply conforms to the MODBUS protocol specification. If 
 *   the input arguments are invalid, for example arubPayloadIn is a \c NULL
 *   pointer, the function returns eMBErrorCode::MB_EINVAL. In case of an 
 *   exception from the slave any of the MODBUS exceptions can be returned. 
 *   If the slave did not respond within the timeout the function returns 
 *   eMBErrorCode::MB_ETIMEDOUT. 
 */
_DLLEXP eMBErrorCode eMBMReadWriteRAWPDU( xMBMHandle xHdl, UCHAR ucSlaveAddress, UCHAR ucFunctionCode,
                                          const UBYTE arubPayloadIn[], UBYTE ubPayloadInLength,
                                          UBYTE arubPayloadOut[], UBYTE ubPayloadOutLengthMax,
                                          UBYTE * pubPayloadOutLength );

#endif


#if MBM_FUNC_REPORT_SLAVEID_ENABLED == 1
/*! \brief <em>Report slave ID</em> from a MODBUS slave with function
 *    code <b>0x11</b>
 *
 * This function issues the report slave id command to a MODBUS slave.
 * The response is then stored in the buffer provided by arubBufferOut
 * where the caller has to allocate sufficient space. The maximum amount
 * of space available is determined by ubBufferMax. If there is not enough
 * space available the function returns eMBErrorCode::MB_ENORES.
 *
 * There is no chance for the MODBUS stack to tell something about the
 * content since the content is vendor specific. 
 *
 * \note There are some non vendor specific fields but most vendors do
 * not implement them correctly. Therefore we have completely avoided interpreting
 * the data.
 * 
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param arubBufferOut Buffer which can hold ubBufferMax bytes. 
 * \param ubBufferMax Size of buffer.
 * \param pubLength If the call succeeds contains the number of bytes
 *   written to the buffer.
 * \return eMBErrorCode::MB_ENOERR if the slave report command succeeded. If 
 *  the buffer does not have enough space the function returns eMBErrorCode::MB_ENORES.
 * If any of the input arguments are invalid or the buffer are \c NULL the function
 * return eMBErrorCode::MB_EINVAL. In case of an  exception from the slave any of 
 * the MODBUS exceptions can be returned. If the slave did not respond within
 * the timeout the function returns eMBErrorCode::MB_ETIMEDOUT. 
 * 
 */
_DLLEXP eMBErrorCode eMBMReportSlaveID( xMBHandle xHdl, UCHAR ucSlaveAddress,
                                        UBYTE arubBufferOut[], UBYTE ubBufferMax, UBYTE * pubLength );
#endif

#if MBM_FUNC_RD_FILES_ENABLED == 1
/*! \brief <em>)Read File Record</em> from a MODBUS slave with function
 *    code <b>0x14</b>
 *
 * This function issues a read file record request. The caller of this function
 * is required to provide an (possibly unitialized) array of arxSubResponses.
 * The size of this array must be equal or larger than the number of sub requests
 * within arxSubRequests.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param arxSubRequests Sub requests for file records. 
 * \param arxSubResponses A sub response for every requested arxSubRequests.
 * \param usNSubRequests Size of the arxSubRequests arrays.
 * \return eMBErrorCode::MB_ENOERR if the read file record command was successfull.
 *   If the arguments are not valid, e.g the number of sub requests would
 *   exceed the maximum length of a MODBUS frame, the function returns eMBErrorCode::MB_EINVAL.
 *   In case of an exception returned by the slave any of the MODBUS exceptions can be returned.
 *   If the slave did not respond within the timeout the function returns eMBErrorCode::MB_ETIMEDOUT.
 *
 */
_DLLEXP eMBErrorCode eMBMReadFileRecord( xMBHandle xHdl, UCHAR ucSlaveAddress,
                                         const xMBMFileSubReadReq_t arxSubRequests[],
                                         xMBMFileSubReadResp_t arxSubResponses[], USHORT usNSubRequests );
#endif

#if MBM_FUNC_WR_FILES_ENABLED == 1
/*! \brief <em>)Write File Record</em> to a MODBUS slave with function
 *    code <b>0x15</b>
 *
 * This function issues a write file record request. The caller of this function
 * is required to provide an array of arxSubRequests.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param arxSubRequests Sub requests for file records. 
 * \param usNSubRequests Size of the arxSubRequests arrays.
 * \return eMBErrorCode::MB_ENOERR if the write file record command was successfull.
 *   If the arguments are not valid, e.g the number of sub requests would
 *   exceed the maximum length of a MODBUS frame, the function returns eMBErrorCode::MB_EINVAL.
 *   In case of an exception returned by the slave any of the MODBUS exceptions can be returned.
 *   If the slave did not respond within the timeout the function returns eMBErrorCode::MB_ETIMEDOUT.
 */
_DLLEXP eMBErrorCode eMBMWriteFileRecord( xMBHandle xHdl, UCHAR ucSlaveAddress,
                                          const xMBMFileSubWriteReq_t arxSubRequests[], USHORT usNSubRequests );
#endif

/* ----------------------- Function prototypes ( Non-Blocking ) -------------*/
#if MBM_FUNC_RD_HOLDING_ENABLED == 1
/*! \brief <em>Read Holding Registers</em> from a slave using the MODBUS 
 *   function code <b>0x03</b> with the Non-Blocking API.
 *
 * This function provides the same functionality as the function 
 * eMBMReadHoldingRegisters but uses a different interface. 
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address. Note that a broadcast address is not
 *   allowed for a function which expects a response.
 * \param usRegStartAddress The first holding register to be read. We use
 *   protocol addresses starting at zero.
 * \param ubNRegs Number of registers to read.
 * \param arusBufferOut An array of USHORT values of at least ubNRegs 
 *   elements.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version. 
 */
_DLLEXP void    vMBMReadHoldingRegistersPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                                USHORT usRegStartAddress, UBYTE ubNRegs,
                                                USHORT arusBufferOut[],
                                                eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_WR_SINGLE_REG_ENABLED == 1
/*! \brief <em>Write Single Register</em> in a slave using the MODBUS function
 *   code <b>0x06</b> with the Non-Blocking API.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address. Note that a broadcast address is not
 *   allowed for a function which expects a response.
 * \param usRegAddress The register address to write.
 * \param usValue The value which should be written to the register.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version. 
 */
_DLLEXP void    vMBMWriteSingleRegisterPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                               USHORT usRegAddress, USHORT usValue,
                                               eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_RD_INPUT_REGS_ENABLED == 1
/*! \brief <em>Read Input Registers</em> from a slave using the MODBUS 
 *   function code <b>0x04</b> with the Non-Blocking API.
 *
 * This function provides the same functionality as the function 
 * eMBMReadInputRegisters but uses a different interface. 
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address. Note that a broadcast address is not
 *   allowed.
 * \param usRegStartAddress The first holding register to be read starting
 *   at zero.
 * \param ubNRegs Number of registers to read.
 * \param arusBufferOut An array of USHORT values of at least ubNRegs 
 *   elements.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version. 
 */
_DLLEXP void    vMBMReadInputRegistersPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                              USHORT usRegStartAddress, UBYTE ubNRegs,
                                              USHORT arusBufferOut[],
                                              eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_WR_MUL_REGS_ENABLED == 1
/*! \brief <em>Write Multiple Registers</em> from a slave using the MODBUS
 *   function code <b>0x10</b>.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usRegStartAddress First register to write to.
 * \param ubNRegs Number of registers to write.
 * \param arusBufferIn An Array of USHORT values of at least ubNRegs elements.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version. 
 */
_DLLEXP void    vMBMWriteMultipleRegistersPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                                  USHORT usRegStartAddress, UBYTE ubNRegs,
                                                  const USHORT arusBufferIn[],
                                                  eMBMQueryState * peState, eMBErrorCode * peStatus );

#endif

#if MBM_FUNC_RDWR_MUL_REGS_ENABLED == 1
/*! \brief <em>Read/Write Multiple Registers</em> from a slave using the MODBUS
 *   function code <b>0x17</b>
 *
 * \note The pointers arusBufferIn and arusBufferOut can point to the same 
 *   memory location. In this case the old contents is overwritten with
 *   the returned registers values.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usWriteRegStartAddress First register to write on the slave.
 * \param ubWriteNRegs Number of registers to write.
 * \param arusBufferIn An Array of USHORT values of ubWriteNRegs elements 
 *   which should be written on the slave.
 * \param usReadRegStartAddress First register to read from the slave.
 * \param ubReadNRegs Number of registers to read.
 * \param arusBufferOut An Array of ubReadNRegs elements where the registers
 *   values returned by the slave should be stored.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version. 
 */
_DLLEXP void    vMBMReadWriteMultipleRegistersPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                                      USHORT usWriteRegStartAddress, UBYTE ubWriteNRegs,
                                                      const USHORT arusBufferIn[],
                                                      USHORT usReadRegStartAddress, UBYTE ubReadNRegs,
                                                      USHORT arusBufferOut[],
                                                      eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_READ_DISC_INPUTS_ENABLED == 1
/*! \brief <em>Read Discrete Inputs</em> from a slave using the MODBUS
 *   function code <b>0x02</b>
 *
 * The discrete inputs are packed as one input per bit. Statis is 1=ON and 0=OFF.
 * The LSB of the first data byte contains the input addressed in the query. The
 * other inputs follow toward the high order of the byte. If the input quantity
 * is not a multiple of eight the final data byte is padded with zeros.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usInputStartAddress Address of first discrete input.
 * \param usNInputs Number of discrete inputs to read.
 * \param arubBufferOut An array with a size of at least udNInputs/8 bytes.
 *   The size must be rounded up to next integer.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version.
 */
_DLLEXP void    vMBMReadDiscreteInputsPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                              USHORT usInputStartAddress, USHORT usNInputs,
                                              UBYTE arubBufferOut[],
                                              eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_READ_COILS_ENABLED == 1
/*! \brief <em>Read Coils</em> from a slave using the MODBUS
 *   function code <b>0x01</b>
 *
 * The coils are packed as one coil per bit. Statis is 1=ON and 0=OFF. The LSB 
 * of the first data byte contains the coil addressed in the query. The other 
 * coils follow toward the high order of the byte. If the input quantity
 * is not a multiple of eight the final data byte is padded with zeros.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usCoilStartAddress Address of first coil.
 * \param usNCoils Number of coils to read.
 * \param arubBufferOut An array with a size of at least udNInputs/8 bytes.
 *   The size must be rounded up to next integer.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version.
 */
_DLLEXP void    vMBMReadCoilsPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usCoilStartAddress, USHORT usNCoils,
                                     UBYTE arubBufferOut[], eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_WRITE_SINGLE_COIL_ENABLED == 1
/*! \brief <em>Write Coil</em> on slave using the MODBUS
 *   function code <b>0x06</b>
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usOutputAddress Address of coil.
 * \param bOn Either \c TRUE or \c FALSE.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version.
 */
_DLLEXP void    vMBMWriteSingleCoilPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usOutputAddress, BOOL bOn,
                                           eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_WRITE_COILS_ENABLED == 1
/*! \brief <em>Write Coils</em> from a slave using the MODBUS
 *   function code <b>0x0F</b>
 *
 * The coils are packed as one coil per bit. Statis is 1=ON and 0=OFF. The LSB 
 * of the first data byte contains the coil addressed in the query. The other 
 * coils follow toward the high order of the byte. 
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param usCoilStartAddress Address of first coil.
 * \param usNCoils Number of coils to read.
 * \param arubCoilsIn An array with a size of at least usNCoils/8 bytes.
 *   The size must be rounded up to next integer. A
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version. 
 */
_DLLEXP void    vMBMWriteCoilsPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress, USHORT usCoilStartAddress, USHORT usNCoils,
                                      const UBYTE arubCoilsIn[], eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_READWRITE_RAWPDU_ENABLED == 1
/*! \brief A function for sending raw MODBUS PDUs to a MODBUS slave. This
 *   function can be used if you want to transport custom data over a
 *   MODBUS connection or for briding other protocols. Please note that
 *   of course most standard slaves won't know how to deal with these
 *   custom data payload and therefore its use should be limited.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param ucFunctionCode MODBUS function code used for transport. Must
 *   be between 1 and 127. Recommend values are between 65-72 and 100-110.
 * \param arubPayloadIn MODBUS request which will be sent to the slave.
 * \param ubPayloadInLength Length of MODBUS request. Maximum is 252 bytes.
 * \param arubPayloadOut Holds the MODBUS response sent by the slave.
 *   arubPayloadOut can be \c NULL is ubPayloadOutLengthMax is set to
 *   zero.
 * \param ubPayloadOutLengthMax Size of the buffer arubPayloadOut in bytes.
 *   If the slave response exceeds the number of bytes available in the
 *   buffer the function set peStatus to eMBErrorCode::MB_EINVAL.
 * \param pubPayloadOutLength Actual number of bytes returned by the
 *   slave.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version. 
 */
_DLLEXP void    vMBMReadWriteRAWPDUPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress, UCHAR ucFunctionCode,
                                           const UBYTE arubPayloadIn[], UBYTE ubPayloadInLength,
                                           UBYTE arubPayloadOut[], UBYTE ubPayloadOutLengthMax,
                                           UBYTE * pubPayloadOutLength,
                                           eMBMQueryState * peState, eMBErrorCode * peStatus );

#endif

#if MBM_FUNC_REPORT_SLAVEID_ENABLED == 1
/*! \brief <em>Report slave ID</em> from a MODBUS slave with function
 *    code <b>0x11</b>
 *
 * This function issues the report slave id command to a MODBUS slave.
 * The response is then stored in the buffer provided by arubBufferOut
 * where the caller has to allocate sufficient space. The maximum amount
 * of space available is determined by ubBufferMax. If there is not enough
 * space available the function returns eMBErrorCode::MB_ENORES.
 *
 * There is no chance for the MODBUS stack to tell something about the
 * content since the content is vendor specific. 
 *
 * \note There are some non vendor specific fields but most vendors do
 * not implement them correctly. Therefore we have completely avoided interpreting
 * the data.
 * 
* \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param arubBufferOut Buffer which can hold ubBufferMax bytes. 
 * \param ubBufferMax Size of buffer.
 * \param pubLength If the call succeeds contains the number of bytes
 *   written to the buffer.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version. 
 */
_DLLEXP void    vMBMReportSlaveID( xMBHandle xHdl, UCHAR ucSlaveAddress, UBYTE arubBufferOut[],
                                   UBYTE ubBufferMax, UBYTE * pubLength,
                                   eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_RD_FILES_ENABLED == 1
/*! \brief <em>)Read File Record</em> from a MODBUS slave with function
 *    code <b>0x14</b>
 *
 * This function issues a read file record request. The caller of this function
 * is required to provide an (possibly unitialized) array of arxSubResponses.
 * The size of this array must be equal or larger than the number of sub requests
 * within arxSubRequests.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param arxSubRequests Sub requests for file records. 
 * \param arxSubResponses A sub response for every requested arxSubRequests.
 * \param usNSubRequests Size of the arxSubRequests arrays.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version eMBMReadFileRecord. 
 */
void
 
 
 
 
 
 vMBMReadFileRecordPolled( xMBHandle xHdl, UCHAR ucSlaveAddress,
                           const xMBMFileSubReadReq_t arxSubRequests[], xMBMFileSubReadResp_t arxSubResponses[],
                           USHORT usNSubRequests, eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

#if MBM_FUNC_WR_FILES_ENABLED == 1
/*! \brief <em>)Write File Record</em> to a MODBUS slave with function
 *    code <b>0x15</b>
 *
 * This function issues a write file record request. The caller of this function
 * is required to provide an array of arxSubRequests.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param ucSlaveAddress Slave address.
 * \param arxSubRequests Sub requests for file records. 
 * \param usNSubRequests Size of the arxSubRequests arrays.
 * \param peState A pointer where the internal state can be stored. This value
 *   should be initialized to MBM_STATE_NONE. Violating this rule results in
 *   undefined behavior.
 * \param peStatus A pointer to a status variable. After the command has
 *   finished the status variable contains the final result of the transaction.
 *   The error codes are the same as for the blocking version eMBMWriteFileRecord. 
 */
_DLLEXP void    vMBMWriteFileRecordPolled( xMBHandle xHdl, UCHAR ucSlaveAddress,
                                           const xMBMFileSubWriteReq_t arxSubRequests[],
                                           USHORT usNSubRequests, eMBMQueryState * peState, eMBErrorCode * peStatus );
#endif

/* ----------------------- Function prototypes ( Serial ) -------------------*/

#if defined( DOXYGEN ) || ( MBM_ASCII_ENABLED == 1 ) || ( MBM_RTU_ENABLED == 1 )
/*! 
 * \brief Create a new instances for a serial MODBUS master instance using
 *   either ASCII or RTU transmission mode.
 *
 * \note
 * In RTU mode 11 bits are used for each data byte. The coding system is
 * 8bit binary.
 *  - 1 start bit.
 *  - 8 data bits with LSB sent first.
 *  - 1 bit for parity (Even, Odd)
 *  - 1 or 2 stop bits (Two stopbits if no parity is used).
 *
 * In ASCII mode 10 bits are used. The coding system uses the hexadecimal
 * ASCII characters 0-8 and A-F. One hexadecimal characters contains 4-bits
 * of data.
 *  - 1 start bit
 *  - 7 data bits with LSB sent first.
 *  - 1 bit for parity (Even, Odd)
 *  - 1 or 2 stop bits (Two stopbits if no parity is used).
 *
 * \param pxHdl A pointer to a MODBUS handle. If the function returns 
 *   MB_ENOERR the handle is updated to hold a new and valid master handle.
 *   This handle should never be modified by the user.
 * \param eMode The serial transmission mode to use. Either MB_RTU or MB_ASCII.
 * \param ucPort The serial port to use. The meaning of this value depends on
 *   the porting layer.
 * \param ulBaudRate The baudrate. For example 38400.
 * \param eParity The parity to use. 
 *
 * \return eMBErrorCode::MB_ENOERR if a new MASTER instances is ready.
 */
_DLLEXP eMBErrorCode eMBMSerialInit( xMBMHandle * pxHdl, eMBSerialMode eMode, UCHAR ucPort,
                                     ULONG ulBaudRate, eMBSerialParity eParity );

/*! 
 * \brief Create a new instances for a serial MODBUS master instance using
 *   either ASCII or RTU transmission mode.
 *
 * Identical to eMBMSerialInit with the exception that the number of stopbits can
 * be set. Note that this can result in non standard conforming behavior.
 *
 * \param pxHdl A pointer to a MODBUS handle. If the function returns 
 *   MB_ENOERR the handle is updated to hold a new and valid master handle.
 *   This handle should never be modified by the user.
 * \param eMode The serial transmission mode to use. Either MB_RTU or MB_ASCII.
 * \param ucPort The serial port to use. The meaning of this value depends on
 *   the porting layer.
 * \param ulBaudRate The baudrate. For example 38400.
 * \param eParity The parity to use. 
 * \param ucStopBits Number of stopbits to use.
 *
 * \return eMBErrorCode::MB_ENOERR if a new MASTER instances is ready.
 */
_DLLEXP eMBErrorCode eMBMSerialInitExt( xMBMHandle * pxHdl, eMBSerialMode eMode, UCHAR ucPort,
                                         ULONG ulBaudRate, eMBSerialParity eParity, UCHAR ucStopBits );
#endif

/* ----------------------- Function prototypes ( TCP ) ----------------------*/

#if defined( DOXYGEN ) || ( MBM_TCP_ENABLED == 1 )

/*!
 * \brief Create a new instaces for a MODBUS TCP master.
 *
 * \param pxHdl A pointer to a MODBUS handle. If the function returns 
 *   MB_ENOERR the handle is updated to hold a new and valid master handle.
 *   This handle should never be modified by the user.
 * \return eMBErrorCode::MB_ENOERR if a new MASTER instances is ready.
 */
_DLLEXP eMBErrorCode eMBMTCPInit( xMBMHandle * pxHdl );

/*!
 * \brief Connect to a new MODBUS TCP slave.
 *
 * If a connection is already open the old connection is closed.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param pcTCPClientAddress A TCP client address. This address is
 *   directly passed to the porting layer and therefore its meaning is
 *   platform dependent.
 * \param usTCPPort The TCP port to use. 502 is standard.
 * \return eMBErrorCode::MB_ENOERR if a connection has been opened. In
 *   case of a connection error eMBErrorCode::MB_EIO. 
 */
_DLLEXP eMBErrorCode eMBMTCPConnect( xMBMHandle xHdl, const CHAR * pcTCPClientAddress, USHORT usTCPPort );

/*!
 * \brief Disconnects a MODBUS TCP slave.
 *
 * \param xHdl A valid MODBUS master handle.
 */
_DLLEXP eMBErrorCode eMBMTCPDisconnect( xMBMHandle xHdl );
#endif

/* ----------------------- Function prototypes ( UDP ) ----------------------*/

#if defined( DOXYGEN ) || ( MBM_UDP_ENABLED == 1 )

/*!
 * \brief Create a new instaces for a MODBUS UDP master.
 *
 * \param pxHdl A pointer to a MODBUS handle. If the function returns 
 *   MB_ENOERR the handle is updated to hold a new and valid master handle.
 *   This handle should never be modified by the user.
 * \param pcUDPBindAddress Not used within the stack but passed to the porting
 *   layer. This argument can be used to bind only to a specific IP address.
 * \param uUDPListenPort Use -1 if the listing port should be allocated dynamically
 *    by the porting layer. Otherwise pass a valid port number between 0 and 65535. 
 * \return eMBErrorCode::MB_ENOERR if a new MASTER instances is ready.
 */
_DLLEXP eMBErrorCode eMBMUDPInit( xMBMHandle * pxHdl, const CHAR * pcUDPBindAddress, LONG uUDPListenPort );

/*!
 * \brief Set the IP address of the MODBUS UDP slave.
 *
 * Not data is actually sent by specifying the client address. All future
 * data sent by any of the read/write registers calls will be directed
 * to this client address. The previous client address is overwritten.
 *
 * \param xHdl A valid MODBUS master handle.
 * \param pcUDPClientAddress A UDP client address. This address is
 *   directly passed to the porting layer and therefore its meaning is
 *   platform dependent.
 * \param usUDPSlavePort The UDP port to use. 502 is standard.
 * \return eMBErrorCode::MB_ENOERR if a connection has been opened. In
 *   case of a connection error eMBErrorCode::MB_EIO. 
 *
 * \warning A reference to pcUDPClientAddress is stored internally. As long
 *   as this instance is used the data where pcUDPClientAddress points to 
 *   must not change. This specially implies that extra care has to be
 *   taken when the address is stored on the stack.
 */
_DLLEXP eMBErrorCode eMBMUDPSetSlave( xMBMHandle xHdl, const CHAR * pcUDPClientAddress, USHORT usUDPSlavePort );

#endif

/*! @} */

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
