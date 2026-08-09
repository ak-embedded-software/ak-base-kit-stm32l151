/* 
 * ModbusMaster Libary: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbmi.h,v 1.17 2013-05-21 21:04:10 embedded-solutions.cwalter Exp $
 */

#ifndef _MBM_INTERNAL_H
#define _MBM_INTERNAL_H

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

#ifndef MBM_TEST_INSTANCES
/*! \brief Number of test instances. Use by unit testing. 
 * \internal
 */
#define MBM_TEST_INSTANCES          ( 0 )
#endif

/* ----------------------- Type definitions ---------------------------------*/

/*! \brief Every MODBUS master instance has a handle which contains pointers to
 *  functions and a buffer for assembling MODBUS frames.
 * \internal
 */
typedef struct
{
    xMBPTimerHandle xRespTimeoutHdl;    /*!< Timer for implementing the response timeout. */
    xMBPEventHandle xFrameEventHdl;     /*!< Receives MBM_EV_SENT, MBM_EV_TIMEOUT and MBM_EV_RECEIVED events. */
    xMBMFrameHandle xFrameHdl;  /*!< Private data for the ASCII/RTU or TCP implementations. */

    UBYTE           ubIdx;      /*!< The internal index used to lookup handles. */

    USHORT          usFrameMBPDULength; /*!< The size of the request or response FRAME. */
    UBYTE          *pubFrameMBPDUBuffer;        /*!< Buffer to assemble MODBUS PDUs. */
    peMBMFrameSend  pFrameSendFN;       /*!< Pointer to a function used to transmit MODBUS frames. */
    peMBMFrameReceive pFrameRecvFN;     /*!< Pointer to a function used to receive MODBUS frames. */
    peMBMFrameClose pFrameCloseFN;      /*!< Pointer to a function used for shutdown. */
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
    xMBStat         xFrameStat;         /*!< Statistic information. */
#endif
#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
    pvMBAnalyzerCallbackCB pvMBAnalyzerCallbackFN;  /*!< Protocol analyzer. */
    void *pvCtx;
#endif
#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
	USHORT			usSlaveTimeoutMS;	/*!< Slave timeout in milliseconds */
	USHORT			usSlaveTimeoutLeftMS;	/*!< Slave timeout left in milliseconds. */
	peMBMFrameIsTransmitting pFrameIsTransmittingFN;	/*!< Pointer to a function used to check transmitter state. */
#endif
} xMBMInternalHandle;

/*! \brief The events which are used by the main state machine.
 * \internal
 */
typedef enum
{
    MBM_EV_NONE,                /*!< Dummy event. */
    MBM_EV_DONE,                /*!< If the request has been processed. */
    MBM_EV_TIMEDOUT,            /*!< No frame was received within the timeout. */
    MBM_EV_RECEIVED,            /*!< A frame has been received. */
    MBM_EV_RECV_ERROR,          /*!< There was an errror receiving a frame. */
    MBM_EV_SENT,                /*!< The frame has been sent. */
    MBM_EV_SEND_ERROR           /*!< There was an error sending the frame. */
} eMBMEvent;

/* ----------------------- Global variables ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/

#if MBM_TEST_INSTANCES != 0
xMBMInternalHandle *pxMBMGetNewHdl( void );
eMBErrorCode    eMBMReleaseHdl( xMBMInternalHandle * pxIntHdl );
#endif

/*! \brief Checks if a handle is valid.
 * \internal
 *
 * \param pxIntHdl A pointer to a handle.
 * \return \c TRUE if this is a valid handle which was allocated by the stack.
 */
BOOL            bMBMIsHdlValid( const xMBMInternalHandle * pxIntHdl );

/*! \brief The common part of the state machine for a MODBUS transaction.
 * \internal
 *
 * It should be called the first time after the frame has been assembled and
 * the state was changed to MBM_STATE_SEND. It then starts transmitting the frame
 * and enables a timeout to detect broken slaves. If the frame has been sent
 * and a response is expected it waits either for the timeout of the reception
 * of the frame and changes to the state MBM_STATE_DISASSEMBLE. If no response is
 * expected it waits for the end of the transmission and switches to the state
 * MBM_STATE_DONE. In case of an error it switch to the stat eMBM_STATE_ERROR.
 * <br>
 * This function should be called whenever the current state is either 
 * MBM_STATE_SEND or MBM_STATE_WAITING.
 *
 * \param pxIntHdl A pointer to an MODBUS internal handle.
 * \param ucSlaveAddress The slave which will receive this request.
 * \param peState A pointer to the state. The value is changed as described
 *   above.
 * \param peStatus A pointer to the status of this MODBUS transaction. This is
 *   the value returned to the caller of the API function.
 */
void            vMBMMasterTransactionPolled( xMBMInternalHandle * pxIntHdl, UCHAR ucSlaveAddress,
                                             /*@out@ */ eMBMQueryState * peState, /*@out@ */ eMBErrorCode * peStatus );

/*! 
 * \if INTERNAL_DOCS
 * @} 
 * \endif
 */

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif
