/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP.
 * Copyright (c) 2008-2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbmiconfig.h,v 1.33 2013-05-21 21:04:10 embedded-solutions.cwalter Exp $
 */

#ifndef _MBM_CONFIG_DEFAULTS_H
#define _MBM_CONFIG_DEFAULTS_H

/* ----------------------- Defines ------------------------------------------*/
/*! \addtogroup mbm_cfg
 *  @{
 */
 
/*! \brief If MODBUS ASCII support is enabled. */
#ifndef MBM_ASCII_ENABLED
#define MBM_ASCII_ENABLED                       ( 0 )
#endif

/*! \brief If MODBUS RTU support is enabled. */
#ifndef MBM_RTU_ENABLED
#define MBM_RTU_ENABLED                         ( 1 )
#endif

/*! \brief If MODBUS TCP support is enabled. */
#ifndef MBM_TCP_ENABLED
#define MBM_TCP_ENABLED                         ( 0 )
#endif

/*! \brief Numer of serial RTU instances supported. */
#ifndef MBM_SERIAL_RTU_MAX_INSTANCES
#define MBM_SERIAL_RTU_MAX_INSTANCES            ( 2 )
#endif

/*! \brief Numer of serial ASCII instances supported. */
#ifndef MBM_SERIAL_ASCII_MAX_INSTANCES
#define MBM_SERIAL_ASCII_MAX_INSTANCES          ( 2 )
#endif

/*! \brief Choose the API of the serial read/write callbacks. There are
 *   two different versions.
 *
 * - API V1: This API uses the callback types 
 *    pbMBPSerialTransmitterEmptyAPIV1CB  and pvMBPSerialReceiverAPIV1CB.
 *    These type of callback are preferred because they can be used to 
 *    implement the strict timeouts for MODBUS RTU. The drawback is that
 *    they can not be implemented on every platform (Windows, Linux, ...).
 *
 * - API V2: This API uses the callback types
 *    pbMBPSerialTransmitterEmptyAPIV2CB  and pvMBPSerialReceiverAPIV2CB.
 *    Instead of single bytes this API can transfer multiple bytes at
 *    the same time. Using this API makes it impossible to implement the
 *    strict MODBUS timeouts but they have a better performance on 
 *    some platforms and the porting layer is easier to implement.
 */
#ifndef MBM_SERIAL_API_VERSION
#define MBM_SERIAL_API_VERSION                  ( 1 )
#endif

/*! \brief Serial timeout for RTU frame reception if the serial API
 *   version 2 is used.
 *
 * Most operating systems using the API version 2 do not support very tight
 * timeouts. Therefore we do not use the t3.5 timers in MODBUS RTU but use
 * a higher timeout. The best value depend on the operating system and
 * its realtime characteristics as well as the serial driver interface.
 *
 * \note You should set this value equal to the buffer size of your serial
 *   driver (For example 8, 16 or 32 bytes) and the minimum speed you use.
 *   As an example suppose you use 19200 baud as the slowest communication
 *   speed and a FIFO buffer of 8 bytes. The minimum value for the timeout
 *   is 11/19200 * 8 = 4,5ms. To add a margin we would use 10ms.
 */
#ifndef MBM_SERIAL_APIV2_RTU_TIMEOUT_MS
#define MBM_SERIAL_APIV2_RTU_TIMEOUT_MS         ( 20 )
#endif

/*! \brief Serial timeout for RTU frame reception computed dynamically
 *   on the baudrate. Only usef in V2 serial API.
 *
 * This function should return the timeout to use for end of frame
 * transmission. You should set this to match the internal buffer size
 * of your serial device. For example if you can buffer 32 bytes and
 * the speed is 9600 you would return approx. 11/9600 * 32 + 5ms
 * where the 5ms is an additional safety margin and 11/9600 is for
 * 8 databits, 1 stopbit and parity.
 */
#ifndef MBM_SERIAL_APIV2_RTU_DYNAMIC_TIMEOUT_MS
#define MBM_SERIAL_APIV2_RTU_DYNAMIC_TIMEOUT_MS( ulBaudRate ) ( MBM_SERIAL_APIV2_RTU_TIMEOUT_MS )
#endif

/*! \brief Numer of TCP instances supported. */
#ifndef MBM_TCP_MAX_INSTANCES
#define MBM_TCP_MAX_INSTANCES                   ( 1 )
#endif

/*! \brief Numer of UDP instances supported. */
#ifndef MBM_UDP_MAX_INSTANCES
#define MBM_UDP_MAX_INSTANCES                   ( 1 )
#endif

/*! \brief Default respose timeout. */
#ifndef MBM_DEFAULT_RESPONSE_TIMEOUT
#define MBM_DEFAULT_RESPONSE_TIMEOUT            ( 500 )
#endif
/*! \brief The character timeout value for Modbus ASCII.
 *
 * The character timeout value is not fixed for Modbus ASCII and is therefore
 * a configuration option. It should be set to the maximum expected delay
 * time of the network.
 */
#ifndef MBM_ASCII_TIMEOUT_SEC
#define MBM_ASCII_TIMEOUT_SEC                   ( 1 )
#endif

/*! \brief Backof time milliseconds for MODBUS ASCII to introduce an 
 *   additional delay after the frame has been received and before the reply
 *   is sent.
 *
 * \warning Don't change the default unless you now exactly what you are doing.
 * \note If a MODBUS slave has sent a respond the MASTER stack waits this
 *  configured time. This can be necessary because otherwise the MASTER could
 *  send a new query (depends on the application) but the slave is still not
 *  read (For example it needs some time to switch its RS485 transceiver from
 *  write to read).
 */
#ifndef MBM_ASCII_BACKOF_TIME_MS
#define MBM_ASCII_BACKOF_TIME_MS                ( 2 )
#endif

/*! \brief If the <em>Read Holding Registers</em> function should be enabled. */
#ifndef MBM_FUNC_RD_HOLDING_ENABLED
#define MBM_FUNC_RD_HOLDING_ENABLED             ( 1 )
#endif

/*! \brief If the <em>Write Single Register</em> function should be enabled. */
#ifndef MBM_FUNC_WR_SINGLE_REG_ENABLED
#define MBM_FUNC_WR_SINGLE_REG_ENABLED          ( 1 )
#endif

/*! \brief If the <em>Read Input Registers</em> function should be enabled. */
#ifndef MBM_FUNC_RD_INPUT_REGS_ENABLED
#define MBM_FUNC_RD_INPUT_REGS_ENABLED          ( 1 )
#endif

/*! \brief If the <em>Read Input Registers</em> function should be enabled. */
#ifndef MBM_FUNC_WR_MUL_REGS_ENABLED
#define MBM_FUNC_WR_MUL_REGS_ENABLED            ( 1 )
#endif

/*! \brief If the <em>Read/Write Multiple Registers</em> function should be
 *   enabled.
 */
#ifndef MBM_FUNC_RDWR_MUL_REGS_ENABLED
#define MBM_FUNC_RDWR_MUL_REGS_ENABLED          ( 1 )
#endif

/*! \brief If the <em>Read Discrete Inputs</em> function should be
 *   enabled.
 */
#ifndef MBM_FUNC_READ_DISC_INPUTS_ENABLED
#define MBM_FUNC_READ_DISC_INPUTS_ENABLED       ( 1 )
#endif

/*! \brief If the <em>Read Coils</em> function should be
 *   enabled.
 */
#ifndef MBM_FUNC_READ_COILS_ENABLED
#define MBM_FUNC_READ_COILS_ENABLED             ( 1 )
#endif

/*! \brief If the <em>Write Single Coil</em> function should be
 *   enabled.
 */
#ifndef MBM_FUNC_WRITE_SINGLE_COIL_ENABLED
#define MBM_FUNC_WRITE_SINGLE_COIL_ENABLED      ( 1 )
#endif

/*! \brief If the <em>Write Coils</em> function should be
 *   enabled.
 */
#ifndef MBM_FUNC_WRITE_COILS_ENABLED
#define MBM_FUNC_WRITE_COILS_ENABLED            ( 1 )
#endif

/*! \brief If a function for rending raw frames should be
 *   enabled.
 */
#ifndef MBM_FUNC_READWRITE_RAWPDU_ENABLED
#define MBM_FUNC_READWRITE_RAWPDU_ENABLED       ( 1 )
#endif

/*! \brief If the <em>Report Slave ID</em> function should be
 *   enabled.
 */
#ifndef MBM_FUNC_REPORT_SLAVEID_ENABLED
#define MBM_FUNC_REPORT_SLAVEID_ENABLED         ( 1 )
#endif

/*! \brief If the <em>Read File Record</em> function should be enabled.
 */
#ifndef MBM_FUNC_RD_FILES_ENABLED
#define MBM_FUNC_RD_FILES_ENABLED				( 1 )
#endif

/*! \brief If the <em>Write File Record</em> function should be enabled.
 */
#ifndef MBM_FUNC_WR_FILES_ENABLED
#define MBM_FUNC_WR_FILES_ENABLED               ( 1 )
#endif

/*! \brief If the API should check arguments for errors. 
 *
 * This has a small performance impact and one might not use this on very
 * small systems.
 */
#ifndef MBM_ENABLE_FULL_API_CHECKS
#define MBM_ENABLE_FULL_API_CHECKS              ( 1 )
#endif

/*! \brief If the RTU layer should at least wait 3,5 times
 *    the character timeout before enabling the receiver.
 */
#ifndef MBM_RTU_WAITAFTERSEND_ENABLED
#define MBM_RTU_WAITAFTERSEND_ENABLED           ( 0 )
#endif

/*! \brief If the ASCII layer should wait some time before
 *     enabling the receiver. 
 */
#ifndef MBM_ASCII_WAITAFTERSEND_ENABLED
#define MBM_ASCII_WAITAFTERSEND_ENABLED			( 0 )
#endif

/*! \brief If we want advanced startup/shutdown locking.
 *
 * Advanced startup/shutdown locking might be necessary if the MODBUS
 * stack should be embedded as a DLL and the porting layers needs
 * specials hooks for startup, i.e. when the first instance is created
 * and when the last instance is shut down.
 * If for the first time an MODBUS serial or TCP init function is called
 * the stack calls vMBPLibraryLoad(  ). If the stack is shutdown
 * the function calls vMBPLibraryUnload( ). Because the init and the
 * close functions could be mixed due to threading a second locking
 * primitive MBP_ENTER_CRTICIAL_INIT and MBP_EXIT_CRITICAL_INIT have
 * to be provided. They default to empty macros but should be defined.
 * for safe startup/shutdown behaviour is this can not be enforced by
 * the application.
 */
#ifndef MBP_ADVA_STARTUP_SHUTDOWN_ENABLED
#define MBP_ADVA_STARTUP_SHUTDOWN_ENABLED       ( 0 )
#endif

/*! \brief If the statistics interface should be enabled.
 *
 * The statistics interface allows to gather additional statistics
 * from the MODBUS stacks. This includes
 *
 *  - Number of received frames (valid)
 *  - Number of sent frames
 *  - Timeout errors
 *  - Number of checksum errors
 *  - Total number of bytes sent
 *  - Total number of bytes received.
 */
#ifndef MBM_ENABLE_STATISTICS_INTERFACE
#define MBM_ENABLE_STATISTICS_INTERFACE         ( 0 )
#endif

/*! \brief If the protocol analyzer should be enabled.
 *
 * The protocol analyser allows the user to supply to custom callbacks 
 * which are called by the stack whenever a new frame is sent or received.
 * These protocol callbacks are maded from the different implementation
 * layers (RTU, ASCII or TCP) and include the MODBUS PDU as well as 
 * the encapsulating elements.
 */
#ifndef MBM_ENABLE_PROT_ANALYZER_INTERFACE
#define MBM_ENABLE_PROT_ANALYZER_INTERFACE      ( 0 )
#endif

/*! \brief If debugging code should be built into the MODBUS stack.
 */
#ifndef MBM_ENABLE_DEBUG_FACILITY
#define MBM_ENABLE_DEBUG_FACILITY               ( 0 )
#endif

/*! \brief If heavy (memory intensive) debugging code should be built into the MODBUS stack.
 */
#ifndef MBM_ENABLE_DEBUG_FACILITY_HEAVY
#define MBM_ENABLE_DEBUG_FACILITY_HEAVY			( 0 )
#endif

/*! \brief The timeout to wait when wait after send timeouts are used in RTU
 *   transmission mode.
 *
 * This macro should be defined if the default wait after send timeouts
 * are not useful. It is only used when either MBS_RTU_WAITAFTERSEND_ENABLED is
 * defined.
 */
#ifndef MBM_SERIAL_RTU_DYNAMIC_WAITAFTERSEND_TIMEOUT_MS
#define MBM_SERIAL_RTU_DYNAMIC_WAITAFTERSEND_TIMEOUT_MS( ulBaudRate ) \
	MB_INTDIV_CEIL( 1000UL * 11UL, ulBaudRate )
#endif

/*! \brief The timeout to wait when wait after send timeouts are used in ASCII
 *   transmission mode.
 *
 * This macro should be defined if the default wait after send timeouts
 * are not useful. It is only used when either MBS_ASCII_WAITAFTERSEND_ENABLED is
 * defined.
 */
#ifndef MBM_SERIAL_ASCII_DYNAMIC_WAITAFTERSEND_TIMEOUT_MS
#define MBM_SERIAL_ASCII_DYNAMIC_WAITAFTERSEND_TIMEOUT_MS( ulBaudRate ) \
	MB_INTDIV_CEIL( 1000UL * 10UL, ulBaudRate )
#endif

/*! \brief Purge TCP sockets before next write
 *
 * If enabled the client socket is purged before the next master transaction
 * is started. This avoids problems in case of a previous timeout with data
 * that is delivered late.
 */
#ifndef MBM_TCP_PURGE_BEFORE_SEND
#define MBM_TCP_PURGE_BEFORE_SEND               ( 1 )
#endif

/*! \brief Activate transmit timeout only after the transmit has been sent.
 *
 * This function polls the transmit state machine and only decrements the
 * timeout when the stack is not transmitting data. This is useful when the
 * baudrate is low as in this case transmitting could already take more time
 * than the timeout.
 */
#ifndef MBM_TIMEOUT_MODE_AFTER_TRANSMIT
#define MBM_TIMEOUT_MODE_AFTER_TRANSMIT			( 0 )
#endif
#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
#ifndef MBM_TIMEOUT_RESOLUTION_MS
#define MBM_TIMEOUT_RESOLUTION_MS				( 100 )
#endif
#endif

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Function prototypes ------------------------------*/

/*! @} */
#endif
