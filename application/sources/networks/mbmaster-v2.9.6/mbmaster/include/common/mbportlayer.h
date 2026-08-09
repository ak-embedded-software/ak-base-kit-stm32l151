/*
 * MODBUS Library: Porting layer shared by master and slave stack.
 * Copyright (c) 2007-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportlayer.h,v 1.18 2012-01-08 20:04:51 embedded-solutions.cwalter Exp $
 */

#ifndef _MB_PORT_LAYER_H
#define _MB_PORT_LAYER_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif
/* ----------------------- Defines ------------------------------------------*/
#ifndef MB_CDECL_SUFFIX
#define MB_CDECL_SUFFIX
#endif
#ifndef MBP_MEMSET
#define MBP_MEMSET( pDest, iCh, nCnt )  memset( pDest, iCh, nCnt )
#endif

/*! \brief If the porting layer detects the enf of frame. For RTU this implies the T3.5 character
 *   timeouts whereas for ASCII the porting layer checks itself on the LF character to
 *   avoid blocking on the read.
 *
 * If this macro is set the MODBUS stack does not internally detect the
 * timeout in RTU mode but depends on the porting layer to call the T35 timeout function.
 * It is passed as an additional argument to the eMBPSerialInit function.
 * For ASCII it does not change the implementation of the stack but the porting layer
 * has to ensure that it does not block after reception of the LF character.
 *
 * For a proper implementation within the serial porting layer it should be noted that.
 *   If the timeout is greater than 1.75ms for baudrates > 19200 baud or
 *   3.5 times the character time (3.5 * 1/baudrate * 11) then this signals the end of the
 *   frame and the function shall be called.
 *   Optionally and even better for full compliance to RTU is that if there is a small gap
 *   between characters exceeding 1.5 times the character time the complete frame shall
 *   be dropped. This needs to be fully implemented in the porting layer and although
 *   required by the standard it is most often ignored.
 *   For ASCII upon reception of the LF make sure that the porting layer does not block within
 *   the read call.
 *
 */
#ifndef MBP_SERIAL_PORT_DETECTS_TIMEOUT
#define MBP_SERIAL_PORT_DETECTS_TIMEOUT     ( 0 )
#endif

/* ----------------------- Type definitions ---------------------------------*/
/*! \brief An abstract data type used as an interface to the event queues.
 * \ingroup mb_port
 */
typedef UBYTE   xMBPEventType;

/*! \brief A callback function which should be called if the timer has
 *   expired.
 * \ingroup mb_port
 *
 * \param xHdl The MODBUS stack handle passed to eMBPortTimerInit.
 * \return The return value is only important if the RTOS can make use of
 *   any additional scheduling information. This function returns \c TRUE if
 *   an internal event has been generated and a context switch should be
 *   performed.
 */
typedef         BOOL( *pbMBPTimerExpiredCB ) ( xMBHandle xHdl );

/*! \brief This function is called by the porting layer if the transmitter
 *   is ready to accept new characters.
 * \ingroup mb_port_serial
 *
 * The function should either store a new byte in the pointer \c ubValue
 * and return \c TRUE indicating that there is a character to transmit or
 * it should return \c FALSE. If the function returns \c FALSE the transmitter
 * is automatically disabled by the porting layer.
 *
 * \param xHdl The handle passed to the eMBPSerialInit function.
 * \param pubValue A pointer where the character to transmit should be stored.
 * \return \c TRUE if there are more characters to transmit. Otherwise \c
 *   FALSE and the transmitter should be disabled.
 */
typedef         BOOL( *pbMBPSerialTransmitterEmptyAPIV1CB ) ( xMBHandle xHdl, UBYTE * pubValue ) MB_CDECL_SUFFIX;

/*! \brief This function should be called when a new character has been
 *   received by the porting layer.
 * \ingroup mb_port_serial
 *
 * \param xHdl The handle passed to the eMBPortSerialInit function.
 * \param ubValue The character received. Only valid characters should be
 *   passed to the stack.
 */
typedef void    ( *pvMBPSerialReceiverAPIV1CB ) ( xMBHandle xHdl, UBYTE ubValue );

/*! \brief Called by the porting layer if the transmitter can accept new
 *   data and APIV2 is enabled.
 * \ingroup mb_port_serial
 *
 * This function should be called by the porting layer when the transmitter
 * is enabled an new character can be accepted. The MODBUS stack will write
 * up to \c usBufferMax character into the buffer \c pubBufferOut. It will
 * store the number of bytes written in the variable \c pusBufferWritten.
 *
 * \param xHdl The handle passed to the eMBPSerialInit function.
 * \param pubBufferOut A buffer of \c usBufferMax bytes.
 * \param pusBufferWritten Set by the stack to the number of bytes written.
 *
 * \return \c FALSE if no more characters will be sent. In this case the
 *  transmitter should be disabled automatically by the porting layer.
 *  Otherwise \c TRUE to indicate that there are more characters waiting.
 */
typedef         BOOL( *pbMBPSerialTransmitterEmptyAPIV2CB ) ( xMBHandle xHdl, UBYTE * pubBufferOut,
                                                              USHORT usBufferMax,
                                                              USHORT * pusBufferWritten ) MB_CDECL_SUFFIX;

/*! \brief Called by the porting layer if new data is available and API V2
 *   is enabled.
 * \ingroup mb_port_serial
 *
 * If the receiver is enabled the porting layer should call this function
 * whenever there is new data available. The number of bytes available
 * is passed in \c usBufferLen.
 *
 * \param pubBufferIn Buffer holding \c usBufferLen bytes.
 * \param usBufferLen Length of the buffer.
 */
typedef void    ( *pvMBPSerialReceiverAPIV2CB ) ( xMBHandle xHdl, const UBYTE * pubBufferIn,
                                                  USHORT usBufferLen ) MB_CDECL_SUFFIX;

/*! \brief Abstract type which points either to the API V1 or API V2
 *   functions depending on the setting of MBS_SERIAL_API_VERSION.
 * \ingroup mb_port_serial
 *
 * If API V1 is enabled a variable of this type holds a function pointer
 * of type pbMBPSerialTransmitterEmptyAPIV1CB. If API V2 is enabled
 * it holds a pointer of type pbMBPSerialTransmitterEmptyAPIV2CB.
 */
#if defined( MBP_FORCE_SERV2PROTOTYPES) && ( MBP_FORCE_SERV2PROTOTYPES == 1 )
typedef pbMBPSerialTransmitterEmptyAPIV2CB pbMBPSerialTransmitterEmptyCB;
#elif defined( MBP_FORCE_SERV1PROTOTYPES) && ( MBP_FORCE_SERV1PROTOTYPES == 1 )
typedef pbMBPSerialTransmitterEmptyAPIV1CB pbMBPSerialTransmitterEmptyCB;
#elif defined( MBP_CORRECT_FUNCPOINTER_CAST ) && ( MBP_CORRECT_FUNCPOINTER_CAST == 1 )
typedef void    ( *pbMBPSerialTransmitterEmptyCB ) ( void );
#else
typedef void   *pbMBPSerialTransmitterEmptyCB;
#endif

/*! \brief Abstract type which points either to the API V1 or API V2
 *   functions depending on the setting of MBS_SERIAL_API_VERSION.
 * \ingroup mb_port_serial
 *
 * If API V1 is enabled a variable of this type holds a function pointer
 * of type pvMBPSerialReceiverAPIV1CB. If API V2 is enabled it holds a
 * pointer of type pvMBPSerialReceiverAPIV2CB.
 */
#if defined( MBP_FORCE_SERV2PROTOTYPES) && ( MBP_FORCE_SERV2PROTOTYPES == 1 )
typedef pvMBPSerialReceiverAPIV2CB pvMBPSerialReceiverCB;
#elif defined( MBP_FORCE_SERV1PROTOTYPES) && ( MBP_FORCE_SERV1PROTOTYPES == 1 )
typedef pvMBPSerialReceiverAPIV1CB pvMBPSerialReceiverCB;
#elif defined( MBP_CORRECT_FUNCPOINTER_CAST ) && ( MBP_CORRECT_FUNCPOINTER_CAST == 1 )
typedef void    ( *pvMBPSerialReceiverCB ) ( void );
#else
typedef void   *pvMBPSerialReceiverCB;
#endif

/*! \brief This function is called by the TCP porting layer when a new
 *   client connection is made.
 * \ingroup mb_port_tcp
 *
 * The stack will check if it can still handle more clients. This value is
 * configured by the compile time configuration directive MBS_TCP_MAX_CLIENTS.
 *
 * \param xMBHdl A handle to a MODBUS stack.
 * \param xTCPClientHdl New client handle.
 *
 * \return eMBErrorCode::MB_ENOERR if the stack has accepted this client.
 *   If no more clients can be handled the function returns
 *   eMBErrorCode::MB_ENORES. In this case the porting layer should drop
 *   the client connection. In case of an invalid handle this function
 *   returns eMBErrorCode::MB_EINVAL.
 */
typedef         eMBErrorCode( *peMBPTCPClientConnectedCB ) ( xMBHandle xMBHdl,
                                                             xMBPTCPClientHandle xTCPClientHdl ) MB_CDECL_SUFFIX;

/*! \brief Called by the TCP porting layer when new data is available for
 *   a TCP client.
 *
 * This function shall be called by the porting layer when new data is available. It is
 * important that the actual state, i.e. the availability of data is used for calling
 * this function and not the event of receiving some data on a connection. On BSD socket
 * like implementations using select with FD_READ is a proper way to do it. In case
 * the TCP/IP stack uses event based semantics the data should be buffered by the porting
 * layer and the function should be called.
 *
 * \note Please note that it is not guranteed that the stack is actually going to read
 *   the data when this function is called. For example this could happen if it is internally
 *   in a state for sending a response to another client. The porting layer should simply
 *   continuing calling this function with a small delay.
 *
 * \ingroup mb_port_tcp
 *
 * \param xMBHdl A handle to a MODBUS stack.
 * \param xTCPClientHdl A handle for a TCP client.
 *
 * \return eMBErrorCode::MB_ENOERR if the data was handled.
 *   eMBErrorCode::MB_EINVAL if the MODBUS handle or the client index was
 *   invalid.
 */
typedef         eMBErrorCode( *peMBPTCPClientNewDataCB ) ( xMBHandle xMBHdl,
                                                           xMBPTCPClientHandle xTCPClientHdl ) MB_CDECL_SUFFIX;

/*! \brief Called by the porting layer when a client has disconnected.
 * \ingroup mb_port_tcp
 *
 * \return eMBErrorCode::MB_ENOERR if the stack was notified correctly that
 *   the client has been disconnected. eMBErrorCode::MB_EINVAL if the MODBUS
 *   handle or the client index was invalid.
 */
typedef         eMBErrorCode( *peMBPTCPClientDisconnectedCB ) ( xMBHandle xMBHdl,
                                                                xMBPTCPClientHandle xTCPClientHdl ) MB_CDECL_SUFFIX;

/*! \brief Called by the UDP porting layer when new data has been received.
 *
 * \return eMBErrorCode::MB_ENOERR if the data was handled. If the data could not be
 *   processed right now then the stack returns eMBErrorCode::MB_EAGAIN. The caller
 *   than can either discard the data in which case the request is lost or call
 *   again later when the stack is ready to handle the data.
 * \ingroup mb_port_udp
 */
typedef         eMBErrorCode( *peMBPUDPClientNewDataCB ) ( xMBHandle xMBHdl ) MB_CDECL_SUFFIX;

#if defined( DOXYGEN) || ( defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) )

#ifndef MBP_FORMAT_USHORT
/*! \brief Default format string for C-style printf for an unsigned 16 bit integer. */
#define MBP_FORMAT_USHORT                   "%hu"
#endif
#ifndef MBP_FORMAT_SHORT
/*! \brief Default format string for C-style printf for a signed 16 bit integer. */
#define MBP_FORMAT_SHORT                    "%hd"
#endif
#ifndef MBP_FORMAT_UINT_AS_HEXBYTE
/*! \brief Default format string for C-style printf for printing an integer as 1 byte hex. */
#define MBP_FORMAT_UINT_AS_HEXBYTE          "%02X"
#endif
#ifndef MBP_FORMAT_ULONG
/*! \brief Default format string for C-style printf for an unsigned 32 bit integer. */
#define MBP_FORMAT_ULONG                    "%ul"
#endif

/*! \brief Debug facilities */
typedef enum
{
    MB_LOG_CORE = 0x0001,
    MB_LOG_RTU = 0x0002,
    MB_LOG_ASCII = 0x0004,
    MB_LOG_TCP = 0x0008,
    MB_LOG_PORT_EVENT = 0x0010,
    MB_LOG_PORT_TIMER = 0x0020,
    MB_LOG_PORT_SERIAL = 0x0040,
    MB_LOG_PORT_TCP = 0x0080,
    MB_LOG_PORT_OTHER = 0x0100,
    MB_LOG_UDP = 0x0200,
    MB_LOG_PORT_UDP = 0x0400,
    MB_LOG_ALL = 0xFFFF
} eMBPortLogFacility;

/*! \brief Debug levels used by debugging facility */
typedef enum
{
    MB_LOG_ERROR = 0,           /*! Error message */
    MB_LOG_WARN = 1,            /*! Warning message */
    MB_LOG_INFO = 2,            /*! Informational message */
    MB_LOG_DEBUG = 3            /*! Debug message */
} eMBPortLogLevel;
#endif

 /* ----------------------- Function prototypes ( Timer functions ) --------- */

/*! \addtogroup mb_port
 * @{
 */

/*! \brief Creates a new Timer and returns a handle to this timer.
 *
 * Timers are a generic instruction. If the timer has been started by a call
 * to eMBPTimerStart a callback should be made when the timer has expired
 * using the function pointer pbMBPTimerExpiredFN with the handle xHdl as an
 * argument.<br>
 *
 * The following semantics are important for the implementation of the porting
 * layer:
 * - If a timer has been started and it has expired a callback should be made.
 * - All timers are singleshot. Therefore it does not reenable itself.
 * - It must be possible to stop an already running timer. In this case
 *   no callback should be made.
 * - If a timer is closed all its resources should be released and the handle
 *   should no longer be valid.
 * - Starting a timer when it is already running resets its timeout.
 * - Stopping a timer multiple times has no side effects other than the
 *   disabling of the timer.
 * - All timers must start disarmed.i
 *
 * \param xTimerHdl A pointer to a timer handle. If the function returns
 *   eMBErrorCode::MB_ENOERR the handle should be valid.
 * \param usTimeOut1ms Timeout in milliseconds.
 * \param pbMBPTimerExpiredFN A pointer to a function.
 * \param xHdl A MODBUS handle.
 *
 * \return eMBErrorCode::MB_ENOERR if a new timer has been created.
 *   eMBErrorCode::MB_EINVAL if either xTimerHdl equals \c NULL or
 *   pbMBPTimerExpiredFN equals \c NULL or xHdl equals MBP_HDL_INVALID. In
 *   case no more timers can be created the function should return
 *   eMBErrorCode::MB_ENORES. All other errors should be mapped
 *   to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPTimerInit( xMBPTimerHandle * xTimerHdl, USHORT usTimeOut1ms,
                               pbMBPTimerExpiredCB pbMBPTimerExpiredFN, xMBHandle xHdl );

/*! \brief Releases this timer.
 *
 * \param xTimerHdl A timer handle. If not valid the function should do
 *   nothing.
 */
void            vMBPTimerClose( xMBPTimerHandle xTimerHdl );

/*! \brief Changes the timeout for a timer.
 *
 * \param xTimerHdl A valid timer handle.
 * \param usTimeOut1ms The new timeout in milliseconds.
 * \return eMBErrorCode::MB_ENOERR if the timeout has been changed.
 *   eMBErrorCode::MB_EINVAL if the timer handle was not valid. All other
 *   errors should be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPTimerSetTimeout( xMBPTimerHandle xTimerHdl, USHORT usTimeOut1ms );

/*! \brief Starts the timer.
 *
 * When a timer has been started the timer module should perform a callback
 * after the timeout for this timer has been elapsed or the timer has been
 * disabled before it has expired.
 *
 * \param xTimerHdl A valid timer handle.
 * \return eMBErrorCode::MB_ENOERR if the timer has been started.
 *   eMBErrorCode::MB_EINVAL if the timer handle was not valid. All other
 *   errors should be mapped to  eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPTimerStart( xMBPTimerHandle xTimerHdl );

/*! \brief Stops the timer.
 *
 * If the timer has been stopped no callbacks should be performed.
 *
 * \param xTimerHdl A valid timer handle.
 * \return eMBErrorCode::MB_ENOERR if the timer has been stopped.
 *   eMBErrorCode::MB_EINVAL if the timer handle was not valid. All other
 *   errors should be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPTimerStop( xMBPTimerHandle xTimerHdl );

/*! @} */

/* ----------------------- Function prototypes ( Event Handlers )------------*/

/*! \addtogroup mb_port
 * @{
 */

/*! \brief Creates a new event handler.
 *
 * An event handler allows the posting of an event. At most one event is
 * posted at once and this event can be retrieved later by calling
 * bMBPEventGet. The function bMBPEventGet should return \c FALSE if
 * no event is available.
 *
 * \note If an RTOS is available this function should be implemented by
 *   message queues with a depth of one. If an event is posted a message
 *   should be placed in the queue. The function bMBPEventGet should
 *   try to get a message from the queue and it is allowed to block.
 *
 * \note If no RTOS is used the function eMBPEventPost could simply
 *   place the event into a global variable. The function bMBPEventGet
 *   checks if an event has been posted and has not been processed (
 *   Processed means it has been returned by a previous call). If no
 *   event is ready it should return FALSE. Otherwise the event is removed
 *  and the function returns TRUE.
 *
 * \param pxEventHdl A pointer to an event handle. If the function returns
 *   eMBErrorCode::MB_ENOERR the value should contain a valid event handle.
 * \return eMBErrorCode::MB_ENOERR if the event handle has been created.
 *   eMBErrorCode::MB_EINVAL if pxEventHdl was \c NULL. All other errors
 *   should be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPEventCreate( xMBPEventHandle * pxEventHdl );

/*! \brief Post a event to an event queue.
 *
 * \param xEventHdl A valid event handle.
 * \param xEvent The event to post. Any previous event will be overwritten.
 * \return eMBErrorCode::MB_ENOERR if the event has been posted.
 *   eMBErrorCode::MB_EINVAL if the handle was not valid. All other errors
 *   should be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPEventPost( const xMBPEventHandle xEventHdl, xMBPEventType xEvent );

/*! \brief Get an event from the queue.
 *
 * \param xEventHdl A valid handle created by eMBPortEventCreate.
 * \param pxEvent If the function returns TRUE the pointer is updated
 *   to hold the stored event. Otherwise it is left unchanged.
 * \return TRUE if an event was available in the queue. Otherwise FALSE.
 */
BOOL            bMBPEventGet( const xMBPEventHandle xEventHdl, xMBPEventType * pxEvent );

/*! \brief Releases the event queue.
 *
 * \param xEventHdl A event handle to remove.
 */
void            vMBPEventDelete( xMBPEventHandle xEventHdl );

/*! @} */

/* ----------------------- Function prototypes ( Serial functions ) ---------*/

/*! \addtogroup mb_port_serial
 *
 * @{
 */

/*! \brief This function should initialize a new serial port and return a
 *   handle to it.
 *
 * \note The serial port should start in the disabled mode. I.e. it should
 *   behave the same as if the transmitter and the receiver has been disabled
 *   by the appropriate calls.
 *
 * \param pxSerialHdl A pointer to a serial handle. If the function returns
 *   MBP_ENOERR this value must hold a valid handle.
 * \param ucPort A porting layer dependent number to distinguish between
 *   different serial ports.
 * \param ulBaudRate The baudrate. For example 38400.
 * \param ucDataBits Number of databits. Values used are 8 and 7.
 * \param eParity The parity.
 * \param ucStopBits Either one or two stopbits.
 * \param xMBHdl A MODBUS stack handle. This should be passed in every
 *   callbacks made by the serial porting layer.
 * \param bFrameTimeoutFN If MBP_SERIAL_PORT_DETECTS_TIMEOUT is active and
 *   this argument is not equal to NULL the porting layer needs to detect the
 *   timeouts. See the documentation of MBP_SERIAL_PORT_DETECTS_TIMEOUT.
 * \param eMode If the serial port detects the timeout the argument includes 
 *   wheter ASCII or RTU is used.
 * \return eMBErrorCode::MB_ENOERR if a new serial port instances has been
 *   created. If pxSerialHdl equals \c NULL or one of the arguments is not
 *   valid it should return eMBErrorCode::MB_EINVAL. All other errors should
 *   be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBHdl
#if MBP_SERIAL_PORT_DETECTS_TIMEOUT == 1
								, pbMBPTimerExpiredCB pbFrameTimeoutFN, eMBSerialMode eMode
#endif
                                );

/*! \brief Close a serial port.
 *
 * This function should release all resources used by this instance such that
 * it can be used again.
 *
 * \param xSerialHdl A valid handle for a serial port.
 * \return eMBErrorCode::MB_ENOERR If the port has been released.
 *   eMBErrorCode::MB_EAGAIN if the function should be called again because
 *   a shutdown is not possible right now. eMBErrorCode::MB_EINVAL if the
 *   handle is not valid. All other errors should be mapped to
 *   eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPSerialClose( xMBPSerialHandle xSerialHdl );

/*! \brief Enables the transmitter and registers a callback or disables it.
 *
 * After the transmitter has been enabled the callback function should be
 * called until it returns \c FALSE indicating that no more characters should
 * be transmitted.
 *
 * \param xSerialHdl A valid handle for a serial port.
 * \param pbMBPTransmitterEmptyFN A pointer to the callback function or \c
 *   NULL if the transmitter should be disabled.
 * \return eMBErrorCode::MB_ENOERR is the transmitter has been enabled.
 *   eMBErrorCode::MB_EINVAL if the handle is not valid. All other errors
 *   should be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPSerialTxEnable( xMBPSerialHandle xSerialHdl,
                                    pbMBPSerialTransmitterEmptyCB pbMBPTransmitterEmptyFN );

/*! \brief Enables the receiver and registers a callback or disables it.
 *
 * After the receiver has been enabled the callback function should be
 * called for every new character. Only valid characters should be passed
 * to the stack.
 *
 * \param xSerialHdl A valid handle for a serial port.
 * \param pvMBPReceiveFN  A pointer to the callback function or \c NULL if
 *   the receiver should be disabled.
 * \return eMBErrorCode::MB_ENOERR is the receiver has been disabled.
 *   eMBErrorCode::MB_EINVAL if the handle is not valid. All other errors
 *   should be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBPReceiveFN );

/*! @} */

/* ----------------------- Function prototypes ( TCP functions ) ------------*/

/*! \addtogroup mb_port_tcp
 *
 * @{
 */

/*! \brief Create a new TCP instance for handling client connections.
 *
 * \param pxTCPHdl A TCP handle for the client instance.
 * \param xMBHdl A handle for the MODBUS stack.
 * \param eMBPTCPClientNewDATAFN Callback function if new client data
 *    is available.
 * \param eMBPTCPClientDisconnectedFN Callback function if a client
 *    connection is broken.
 * \return eMBErrorCode::MB_ENOERR if a new instance has been crated.
 *   In case of invalid arguments it should return eMBErrorCode::MB_EINVAL.
 *   Otherweise eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPTCPClientInit( xMBPTCPHandle * pxTCPHdl, xMBHandle xMBHdl,
                                   peMBPTCPClientNewDataCB eMBPTCPClientNewDATAFN,
                                   peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN );

/*! \brief Shutdown a TCP instance for handling client connections.
 * \param xTCPHdl A TCP handle for a client instance.
 *
 * \return MBErrorCode::MB_ENOERR if the client instances has been
 *   closed.
 */
eMBErrorCode    eMBPTCPClientClose( xMBPTCPHandle xTCPHdl );

/*! \brief Open a new client connection.
 *
 * The client connection can be used to read and write data. If the stack
 * wants to transmit data it calls the function eMBPTCPWrite. If new data
 * is available the porting layer executes the callback function
 * eMBPTCPClientNewDataCB. The stack can then read the data by a call to
 * the function eMBPTCPRead.
 *
 * \param xTCPHdl A handle to the TCP client instance.
 * \param pxTCPClientHdl If the function returns eMBErrorCode::MB_ENOERR
 *   this pointer holds a valid client handle.
 * \param pcConnectAddress The IP address to connect to.
 * \param usTCPPort The TCP port to use.
 *
 * \return eMBErrorCode::MB_ENOERR if the connection has been opened and
 *   is ready for use. In case of a connection error eMBErrorCode::MB_EIO.
 *   All other errors are mapped to eMBErrorCode::MB_EPORTERR indicating
 *   that the WHOLE TCP client instance is faulty.
 */
eMBErrorCode    eMBPTCPClientOpen( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle * pxTCPClientHdl,
                                   const CHAR * pcConnectAddress, USHORT usTCPPort );

/*! \brief Create a new listening server on address \c usTCPPort which
 *   accepts connection for the addresses specificed in \c pcBindAddress.
 *
 * The exact meaning of \c pcBindAddress is application and platform
 * dependent and therefore not further specified.
 *
 * \param pxTCPHdl If a new listening server has been created this handle
 *   should point to the server.
 * \param pcBindAddress The address to bind to. The exact meaning is port
 *   dependent.
 * \param usTCPPort The TCP port.
 * \param xMBHdl A handle which is used for the callback functions to
 *   identify the stack.
 * \param eMBPTCPClientNewDataFN Callback function if new client data
 *    is available.
 * \param eMBPTCPClientDisconnectedFN Callback function if a client
 *    connection is broken.
 * \param eMBPTCPClientConnectedFN Callback function if a client
 *    connects to the server.
 * \return eMBErrorCode::MB_ENOERR if a new server was created. In case
 *   of invalid arguments it should return eMBErrorCode::MB_EINVAL. Otherwise
 *   eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPTCPServerInit( xMBPTCPHandle * pxTCPHdl, CHAR * pcBindAddress, USHORT usTCPPort,
                                   xMBHandle xMBHdl,
                                   peMBPTCPClientNewDataCB eMBPTCPClientNewDataFN,
                                   peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN,
                                   peMBPTCPClientConnectedCB eMBPTCPClientConnectedFN );

/*! \brief Closes a server instance.
 *
 * \param xTCPHdl A handle for a TCP server.
 * \return eMBErrorCode::MB_ENOERR if the server has shut down (and closed
 *   all client connections). eMBErrorCode::MB_EINVAL if the handle was
 *   not valid. All other errors should be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBTCPServerClose( xMBPTCPHandle xTCPHdl );

/*! \brief This function is called by the MODBUS stack when new data should
 *    be read from a client.
 *
 * This function must not block and should read up to \c usBufferMax bytes and
 * store them into the buffer \c pubBuffer. The value of \c pusBufferLen should
 * be set to the number of bytes read where 0 is used when there is no data
 * available.
 *
 * \param xTCPHdl A handle to a TCP server instance.
 * \param xTCPClientHdl A handle for a TCP client.
 * \param pubBuffer A buffer of at least \c usBufferMax bytes where the read
 *   bytes from the client should be stored.
 * \param pusBufferLen On return the value should hold the number of bytes
 *   written into the buffer.
 * \param usBufferMax Maximum number of bytes which can be stored in the
 *   buffer.
 * \return The function should return eMBErrorCode::MB_ENOERR if zero or more
 *   bytes have been stored in the buffer and no error occurred. In case of a
 *   client error it should return eMBErrorCode::MB_EIO. In case of an invalid
 *   handle is should return eMBErrorCode::MB_EINVAL. Other errors should be
 *   mapped to eMBErrorCode::MB_EPORTERR signaling the stack that the porting
 *   layer is no longer functional.
 */
eMBErrorCode    eMBPTCPConRead( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, UBYTE * pubBuffer,
                                USHORT * pusBufferLen, USHORT usBufferMax );

/*! \brief This function is called by the MODBUS stack when new data should
 *    be sent over a client connection.
 *
 * This function should not block and should transmit \c usBufferLen bytes over
 * the client connection.
 *
 * \param xTCPHdl A handle to a TCP server instance.
 * \param xTCPClientHdl A handle for a TCP client.
 * \param pubBuffer A buffer of \c usBufferLen bytes which should be transmitted.
 * \param usBufferLen Number of bytes to transmit.
 *
 * \return The function should return eMBErrorCode::MB_ENOERR if all bytes have
 *   been written. In case of an I/O error it should return eMBErrorCode::MB_EIO.
 *   In case of an invalid handle or invalid arguments it should return
 *   eMBErrorCode::MB_EINVAL. All other errors should be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPTCPConWrite( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, const UBYTE * pubBuffer,
                                 USHORT usBufferLen );

/*! \brief Close a TCP client connection.
 *
 * Called by the stack when a TCP client connection should be closed.
 *
 * \param xTCPHdl A handle to a TCP server instance.
 * \param xTCPClientHdl A handle for a TCP client which should be closed.
 * \return The function should return eMBErrorCode::MB_ENOERR if the client
 *   connection has been closed. If any of the handles are invalid it should
 *   return eMBErrorCode::MB_EINVAL. All other errors should be mapped to
 *   eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPTCPConClose( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl );

/*! @} */

/* ----------------------- Function prototypes ( UDP functions ) ------------*/

/*! \addtogroup mb_port_udp
 *
 * @{
 */

#ifndef MBP_DROP_BACKWARD_COMPATIBILITY
/*! \brief UDP porting layer invalid handle */
#define MBP_UDPHDL_INVALID                  NULL
/*! \brief UDP porting layer instance handle */
typedef void   *xMBPUDPHandle;
#endif

#if defined( DOXYGEN) || !defined( MBP_UDP_CLIENTADDR_COPY )
/*! \brief Make a copy a client address.
 *
 * You can either use strdup (default), an assigment or a custom function.
 *
 * \param source Address to copy.
 * \param dest Pointer of type const CHAR * where the copied source address
 *   should be stored.
 */
#define MBP_UDP_CLIENTADDR_COPY( source, dest ) do { \
	if( source != NULL ) \
	{ \
		dest = strdup( source ); \
	} \
	else \
	{ \
		dest = NULL; \
	} \
} while( 0 )
#endif

#if defined( DOXYGEN) || !defined( MBP_UDP_CLIENTADDR_FREE )
/*! \brief Free a previously copied address.
 *
 * \param addr Address to release. It must be safe to pass a \c NULL pointer.
 */
#define MBP_UDP_CLIENTADDR_FREE( addr ) do { \
	free( addr ); \
	addr = NULL; \
} while( 0 )
#endif

 /*! \brief Create a new UDP instance used by a MODBUS master.
  *
  * This function should create two new sockets for sending and receiving data.
  * If pcUDPBindAddress is not equal to \c NULL the sockets should be bound
  * to the address specified in pcUDPBindAddress. Otherwise it should bind to
  * all available interfaces. If uUDPListenPort equals -1 this function
  * should allocate a free port used as a source port for sending data and
  * as a listening port for receiving data.
  *
  * \param pxUDPHdl A pointer to a handle. If the function succeeds the pointer
  *    contains a valid handle.
  * \param xMBHdl A valid handle for the MODBUS stack which handels this client
  *    connection.
  * \param pcUDPBindAddress Not used within the stack but passed to the porting
  *   layer. This argument can be used to bind only to a specific IP address.
  * \param uUDPListenPort Use -1 if the listing port should be allocated dynamically
  *    by the porting layer. Otherwise pass a valid port number between 0 and 65535.
  * \param eMBPUDPClientNewDATAFN Callback function if new client data
  *    is available.
  * \return eMBErrorCode::MB_ENOERR if a new instance has been created.
  *   In case of invalid arguments it should return eMBErrorCode::MB_EINVAL.
  *   Otherweise eMBErrorCode::MB_EPORTERR.
  */
eMBErrorCode    eMBPUDPClientInit( xMBPUDPHandle * pxUDPHdl, xMBHandle xMBHdl,
                                   const CHAR * pcUDPBindAddress, LONG uUDPListenPort,
                                   peMBPUDPClientNewDataCB eMBPUDPClientNewDATAFN );

/*! \brief This function is called by the MODBUS stack when new data should
 *    be sent over a client connection.
 *
 * This function should not block and should transmit \c usBufferLen bytes over
 * the client connection.
 *
 * \param xUDPHdl A handle to a UDP instance.
 * \param pcUDPClientAddress UDP address where packet should be sent to.
 * \param usUDPSlavePort UDP port where packet should be sent to.
 * \param pubBuffer A buffer of \c usBufferLen bytes which should be transmitted.
 * \param usBufferLen Number of bytes to transmit.
 *
 * \return The function should return eMBErrorCode::MB_ENOERR if all bytes have
 *   been written. In case of an I/O error it should return eMBErrorCode::MB_EIO.
 *   In case of an invalid handle or invalid arguments it should return
 *   eMBErrorCode::MB_EINVAL. All other errors should be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPUDPConWrite( xMBPUDPHandle xUDPHdl, const CHAR * pcUDPClientAddress, USHORT usUDPSlavePort,
                                 const UBYTE * pubBuffer, USHORT usBufferLen );

/*! \brief This function is called by the MODBUS after the porting layer has
 *   called the eMBPUDPClientNewDATAFN.
 *
 * This function shall return an entire UDP message in one call. This is a typical
 * behaviour also for example for the Unix/OpenGroup recvfrom call for datagram
 * sockets (http://pubs.opengroup.org/onlinepubs/009695399/functions/recvfrom.html)
 *
 * \param xUDPHdl A handle to a UDP instance.
 * \param pusBufferLen Number of bytes received.
 * \param pubBuffer A buffer of \c usBufferMax bytes.
 * \param usBufferMax Size of buffer.
 *
 * \return The function should store up to usBufferMax in the buffer. If no error
 *   has occured it should return eMBErrorCode::MB_ENOERR. If the message has been
 *   truncated it shall return MB_EIO. Incase of an invalid handle or invalid
 *   arguments it should return eMBErrorCode::MB_EINVAL. All other errors should
 *   be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPUDPConRead( xMBPUDPHandle xUDPHdl, UBYTE * pubBuffer, USHORT * pusBufferLen, USHORT usBufferMax );

/*! \brief This function is identical to eMBPUDPConRead with the exception
 *    that is stores the source address and the source port of the client
 *    from whom we have received the data.
 *
 * \param xUDPHdl A handle to a UDP instance.
 * \param pucClientAddress Pointer where the source address of the received UDP datagram
 *   shall be stored. It must be allocated such that the MBP_UDP_CLIENTADDR_FREE macros
 *   can be used. This buffer is freed by the stack so it is NOT allowed to pass a temporary
 *   static buffer.
 * \param pusClientPort Source port for the UDP datagram received.
 * \param pusBufferLen Number of bytes received.
 * \param pubBuffer A buffer of \c usBufferMax bytes.
 * \param usBufferMax Size of buffer.
 *
 * \return The function should store up to usBufferMax in the buffer. If no error
 *   has occured it should return eMBErrorCode::MB_ENOERR. If the message has been
 *   truncated it shall return MB_EIO. Incase of an invalid handle or invalid
 *   arguments it should return eMBErrorCode::MB_EINVAL. All other errors should
 *   be mapped to eMBErrorCode::MB_EPORTERR.
 */
eMBErrorCode    eMBPUDPConReadExt( xMBPUDPHandle xUDPHdl, CHAR **pucClientAddress, USHORT *pusClientPort, UBYTE * pubBuffer, USHORT * pusBufferLen, USHORT usBufferMax );

/*! \brief Shutdown a UDP instance for handling client connections.
 *
 * All resources for this instance should be freed. This includes closing all
 * sockets and dropping any possibly remaining data.
 *
 * \param xUDPHdl A UDP handle for a client instance.
 *
 * \return MBErrorCode::MB_ENOERR if the client instance has been
 *   closed.
 */
eMBErrorCode    eMBPUDPClientClose( xMBPUDPHandle xUDPHdl );

 /*! @} */

/* ----------------------- Function prototypes ( Library ) ------------------*/
/*! \addtogroup mb_port
 * @{
 */

/*! \brief If the advanced startup/shutdown functionallity has been
 *   enabled this function is called when the first TCP or RTU/ASCII
 *   instance is created.
 *
 * This functionallity can be enabled by setting the macro
 * MBP_ADVA_STARTUP_SHUTDOWN_ENABLED to 1 in the file mbmconfig.h or mbsconfig.h.
 */
void            vMBPLibraryLoad( void );

/*! \brief If the advanced startup/shutdown functionallity has been
 *   enabled this function is called when the last TCP or RTU/ASCII
 *   has been removed.
 *
 * This functionallity can be enabled by setting the macro
 * MBP_ADVA_STARTUP_SHUTDOWN_ENABLED to 1 in the file mbmconfig.h or mbsconfig.h.
 */
void            vMBPLibraryUnload( void );

#if defined( DOXYGEN) || ( defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) )

/*! \brief Log a message using using the porting layer facility.
 *
 * \param eLevel Severity level of log message.
 * \param eModule Module which wants to a log a message.
 * \param szFmt printf style formatting options.
 */
void            vMBPPortLog( eMBPortLogLevel eLevel, eMBPortLogFacility eModule, const CHAR * szFmt, ... );

/*! \brief Checks if a given level module is enabled. This is used to improve
 *    execution speed because if logging is not required the printf formatter is
 *    not called and a function call is avoided.
 *
 * \param eLevel Severity level of log message.
 * \param eModule Module which wants to a log a message.
 * \return TRUE if the message will be logged.
 */
BOOL            bMBPPortLogIsEnabled( eMBPortLogLevel eLevel, eMBPortLogFacility eModule );
#endif

/*! @} */

/* ----------------------- Function prototypes ( Analyser functions ) ------ */
/*! \addtogroup mb_port
 * @{
 */

#if defined( DOXYGEN) || \
( defined( MBP_HAS_TIMESTAMP ) && ( MBP_HAS_TIMESTAMP == 1 ) )

/*! \brief Get the current timestamp.
 *
 * This function must store the current implementation dependent timestamp
 * into the variable pointed to by pTimeStamp. It must be safe for an
 * implementation to copy this data structure into a different one, for
 * example by using memcpy.
 */
void            vMBPGetTimeStamp( xMBPTimeStamp * pTimeStamp );
#endif

/*! @} */

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
