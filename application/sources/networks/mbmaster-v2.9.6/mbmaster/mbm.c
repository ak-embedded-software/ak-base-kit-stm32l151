/* 
 * MODBUS Library: A portable MODBUS master for MODBUS ASCII/RTU/TCP/UDP.
 * Copyright (c) 2008-2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbm.c,v 1.42 2013-05-21 21:02:47 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>
#if defined( __18CXX )
#include <stdio.h>
#endif

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"
#include "internal/mbmiframe.h"
#include "internal/mbmi.h"
#if MBM_ASCII_ENABLED == 1
#include "ascii/mbmascii.h"
#endif
#if MBM_RTU_ENABLED == 1
#include "rtu/mbmrtu.h"
#endif
#if MBM_TCP_ENABLED == 1
#include "tcp/mbmtcp.h"
#endif
#if MBM_UDP_ENABLED == 1
#include "udp/mbmudp.h"
#endif

/*lint --e{717} ~ suppress messages: do .. while(0); */
/*lint --e{767} ~ suppress messages: macro defined differently */
/*lint --e{788} ~ suppress messages: enum constant not used within defaulted switch */
/*lint --e{835} ~ suppress messages: A zero has been given as right argument to operator '+' */

/* ----------------------- Defines ------------------------------------------*/

#ifndef MBM_TEST_DISABLE_TIMEOUTS
#define MBM_TEST_DISABLE_TIMEOUTS   ( 0 )
#endif

#define IDX_INVALID                 ( 255 )

/* Calculate the required number of internal states required from
 * the number of enabled serial and TCP instances. 
 */
#define MBM_MAX_HANDLES     ( \
  ( ( ( MBM_ASCII_ENABLED ) ? /*lint -e(506) */1 : 0 ) * MBM_SERIAL_ASCII_MAX_INSTANCES ) + \
  ( ( ( MBM_RTU_ENABLED ) ? /*lint -e(506) */1 : 0 ) * MBM_SERIAL_RTU_MAX_INSTANCES ) + \
  ( ( ( MBM_TCP_ENABLED ) ? /*lint -e(506) */1 : 0 ) * MBM_TCP_MAX_INSTANCES ) + \
  ( MBM_TEST_INSTANCES ) )

#if MBM_ENABLE_STATISTICS_INTERFACE == 1
#define MBM_RESET_HDL_STAT( x ) do { \
    memset( &( x )->xFrameStat, 0, sizeof( ( x )->xFrameStat ) ); \
} while( 0 );
#else
#define MBM_RESET_HDL_STAT( x )
#endif

#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
#define MBM_RESET_HDL_ANALZER( x ) do { \
    ( x )->pvMBAnalyzerCallbackFN = NULL; \
} while( 0 );
#else
#define MBM_RESET_HDL_ANALZER( x )
#endif

#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
#define MBM_RESET_HDL_TIMEOUT( x ) do { \
  ( x )->usSlaveTimeoutLeftMS = 0; \
  ( x )->usSlaveTimeoutMS = MBM_DEFAULT_RESPONSE_TIMEOUT; \
} while( 0 )
#else
#define MBM_RESET_HDL_TIMEOUT( x )
#endif


#define MBM_RESET_HDL( x )  do { \
  ( x )->xRespTimeoutHdl = MBP_TIMERHDL_INVALID; \
  ( x )->xFrameEventHdl = MBP_EVENTHDL_INVALID; \
  ( x )->xFrameHdl = MBM_FRAME_HANDLE_INVALID; \
  ( x )->ubIdx = IDX_INVALID; \
  ( x )->pubFrameMBPDUBuffer = NULL; \
  ( x )->usFrameMBPDULength = 0; \
  ( x )->pFrameSendFN = NULL; \
  ( x )->pFrameRecvFN = NULL; \
  ( x )->pFrameCloseFN = NULL; \
  MBM_RESET_HDL_STAT( x ); \
  MBM_RESET_HDL_ANALZER( x ); \
  MBM_RESET_HDL_TIMEOUT( x ) ; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
STATIC BOOL     bIsInitalized = FALSE;
STATIC xMBMInternalHandle xMBMInternalHdl[MBM_MAX_HANDLES];

/* ----------------------- Static functions ---------------------------------*/
STATIC BOOL     bMBMResponseTimeoutCB( xMBMHandle xHdl );

#if MBM_TEST_INSTANCES == 0
STATIC xMBMInternalHandle *pxMBMGetNewHdl( void );

STATIC eMBErrorCode eMBMReleaseHdl( xMBMInternalHandle * pxIntHdl );
#endif
#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
STATIC UBYTE    ubMBMCountInstances( void );
#endif

/* ----------------------- Start implementation -----------------------------*/
BOOL
bMBMIsHdlValid( const xMBMInternalHandle * pxIntHdl )
{
    return MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) ? TRUE : FALSE;
}

#if MBM_TEST_INSTANCES == 0
STATIC
#endif
    xMBMInternalHandle * pxMBMGetNewHdl( void )
{
    eMBErrorCode    eStatus = MB_ENORES, eStatus2;
    xMBMInternalHandle *pxIntHdl = NULL;
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xMBMInternalHdl ); ubIdx++ )
        {
            MBM_RESET_HDL( &xMBMInternalHdl[ubIdx] );
        }
        bIsInitalized = TRUE;
    }
    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xMBMInternalHdl ); ubIdx++ )
    {
        if( IDX_INVALID == xMBMInternalHdl[ubIdx].ubIdx )
        {
            pxIntHdl = &xMBMInternalHdl[ubIdx];
            pxIntHdl->ubIdx = ubIdx;
#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
            pxIntHdl->usSlaveTimeoutMS = MBM_DEFAULT_RESPONSE_TIMEOUT;
            if( MB_ENOERR != ( eStatus2 = eMBPTimerInit( &( pxIntHdl->xRespTimeoutHdl ), MBM_TIMEOUT_RESOLUTION_MS, bMBMResponseTimeoutCB, pxIntHdl ) ) )
#else
            if( MB_ENOERR != ( eStatus2 = eMBPTimerInit( &( pxIntHdl->xRespTimeoutHdl ), MBM_DEFAULT_RESPONSE_TIMEOUT, bMBMResponseTimeoutCB, pxIntHdl ) ) )
#endif
            {
                eStatus = eStatus2;
            }
            else if( MB_ENOERR != ( eStatus2 = eMBPEventCreate( &( pxIntHdl->xFrameEventHdl ) ) ) )
            {
                eStatus = eStatus2;
            }
            else
            {
                eStatus = MB_ENOERR;
            }
            break;
        }
    }
    if( MB_ENOERR != eStatus )
    {
        ( void )eMBMReleaseHdl( pxIntHdl );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return MB_ENOERR == eStatus ? pxIntHdl : NULL;      /*lint !e826 ~ suspicious ptr-to-ptr conversion */
}

#if MBM_TEST_INSTANCES == 0
STATIC
#endif
    eMBErrorCode
eMBMReleaseHdl( xMBMInternalHandle * pxIntHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) )
    {

        /* we are now sure that this was really a handle returned by create. */
        if( NULL != pxIntHdl->pFrameCloseFN )
        {
            if( MB_ENOERR != ( eStatus = pxIntHdl->pFrameCloseFN( pxIntHdl ) ) )
            {
            }
            else
            {
                if( MBP_EVENTHDL_INVALID != pxIntHdl->xFrameEventHdl )
                {
                    vMBPEventDelete( pxIntHdl->xFrameEventHdl );
                }
                if( MBP_TIMERHDL_INVALID != pxIntHdl->xRespTimeoutHdl )
                {
                    vMBPTimerClose( pxIntHdl->xRespTimeoutHdl );
                }
                MBM_RESET_HDL( pxIntHdl );
                eStatus = MB_ENOERR;
            }
        }
        else
        {
            if( MBP_EVENTHDL_INVALID != pxIntHdl->xFrameEventHdl )
            {
                vMBPEventDelete( pxIntHdl->xFrameEventHdl );
            }
            if( MBP_TIMERHDL_INVALID != pxIntHdl->xRespTimeoutHdl )
            {
                vMBPTimerClose( pxIntHdl->xRespTimeoutHdl );
            }
            MBM_RESET_HDL( pxIntHdl );
            eStatus = MB_ENOERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

STATIC          BOOL
bMBMResponseTimeoutCB( xMBMHandle xHdl )
{
    eMBErrorCode    eStatus;
    BOOL            bNeedCtxSwitch = FALSE;
    xMBMInternalHandle *pxIntHdl = xHdl;
    BOOL            bTimeout = FALSE;

#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
    MBP_ASSERT( NULL != pxIntHdl->pFrameIsTransmittingFN );
    if( !pxIntHdl->pFrameIsTransmittingFN( xHdl ) )
    {
        if( pxIntHdl->usSlaveTimeoutLeftMS )
        {
            if( pxIntHdl->usSlaveTimeoutLeftMS >= MBM_TIMEOUT_RESOLUTION_MS )
            {
                pxIntHdl->usSlaveTimeoutLeftMS -= MBM_TIMEOUT_RESOLUTION_MS;
            }
            else
            {
                pxIntHdl->usSlaveTimeoutLeftMS = 0;
            }
            eStatus = eMBPTimerStart( pxIntHdl->xRespTimeoutHdl );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
        else
        {
            bTimeout = TRUE;
        }
    }
    else
    {
        eStatus = eMBPTimerStart( pxIntHdl->xRespTimeoutHdl );
        MBP_ASSERT( MB_ENOERR == eStatus );
    }
#else
    bTimeout = TRUE;
#endif
    if( bTimeout )
    {
        if( MB_ENOERR == ( eStatus = eMBPEventPost( pxIntHdl->xFrameEventHdl, ( xMBPEventType ) MBM_EV_TIMEDOUT ) ) )
        {
            bNeedCtxSwitch = TRUE;
        }
        MBP_ASSERT( MB_ENOERR == eStatus );
    }
    return bNeedCtxSwitch;
}

eMBErrorCode
eMBMSetSlaveTimeout( xMBMHandle xHdl, USHORT usNMilliSeconds )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) )
    {
#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
        pxIntHdl->usSlaveTimeoutMS = usNMilliSeconds;
        eStatus = eMBPTimerSetTimeout( pxIntHdl->xRespTimeoutHdl, MBM_TIMEOUT_RESOLUTION_MS );
#else
        eStatus = eMBPTimerSetTimeout( pxIntHdl->xRespTimeoutHdl, usNMilliSeconds );
#endif
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBMClose( xMBMHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
    MBP_ENTER_CRITICAL_INIT(  );
#endif
    if( MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        eStatus = eMBMReleaseHdl( pxIntHdl );
        MBP_EXIT_CRITICAL_SECTION(  );
    }
#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
    if( 0 == ubMBMCountInstances(  ) )
    {
        vMBPLibraryUnload(  );
    }
    MBP_EXIT_CRITICAL_INIT(  );
#endif
    return eStatus;
}

void
vMBMMasterTransactionPolled( xMBMInternalHandle * pxIntHdl, UCHAR ucSlaveAddress, eMBMQueryState * peState, eMBErrorCode * peStatus )
{
    xMBPEventType   xFrameEvent;

    switch ( *peState )
    {
    case MBM_STATE_SEND:
        /* Start the timeout and then schedule the transmission of the frame.
         * If there was an error abort the transmission.
         */
#if MBM_TEST_DISABLE_TIMEOUTS == 0
#if MBM_TIMEOUT_MODE_AFTER_TRANSMIT == 1
        pxIntHdl->usSlaveTimeoutLeftMS = pxIntHdl->usSlaveTimeoutMS;
#endif
        if( ( MB_ENOERR != ( *peStatus = eMBPTimerStart( pxIntHdl->xRespTimeoutHdl ) ) ) )
        {
            *peState = MBM_STATE_ERROR;
        }

#else
        if( 0 )
        {
        }
#endif
        else if( MB_ENOERR != ( *peStatus = pxIntHdl->pFrameSendFN( pxIntHdl, ucSlaveAddress, pxIntHdl->usFrameMBPDULength ) ) )
        {
            *peState = MBM_STATE_ERROR;
        }
        else
        {
            /* Frame transmission has started. We now switch to the state where we
             * wait for an event.
             */
            *peState = MBM_STATE_WAITING;
        }
        break;

        /* Wait for an event. Possible events are either a timeout when no slave has
         * responded (MBM_EV_TIMEDOUT), the reception of a frame(MBM_EV_RECEIVED)
         * or in case of a broadcast message the end of the transmission(MBM_EV_SENT).
         */
    case MBM_STATE_WAITING:
        if( bMBPEventGet( pxIntHdl->xFrameEventHdl, &xFrameEvent ) )
        {
            /* In any case stop the timeout. */
#if MBM_TEST_DISABLE_TIMEOUTS == 0
            ( void )eMBPTimerStop( pxIntHdl->xRespTimeoutHdl );
#endif
            switch ( ( eMBMEvent ) xFrameEvent )
            {
            case MBM_EV_TIMEDOUT:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE, "[IDX=" MBP_FORMAT_USHORT "] Request timeout. Reseting state.\n", ( USHORT ) pxIntHdl->ubIdx );
                }
#endif
                /* Call the receiver to reset its state. */
                ( void )pxIntHdl->pFrameRecvFN( pxIntHdl, ucSlaveAddress, NULL );
#if MBM_ENABLE_STATISTICS_INTERFACE == 1
                pxIntHdl->xFrameStat.ulNTimeouts++;
#endif
                *peState = MBM_STATE_DONE;
                *peStatus = MB_ETIMEDOUT;
                break;

            case MBM_EV_RECEIVED:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE, "[IDX=" MBP_FORMAT_USHORT "] Response frame received.\n", ( USHORT ) pxIntHdl->ubIdx );
                }
#endif
                if( MB_ENOERR != ( *peStatus = pxIntHdl->pFrameRecvFN( pxIntHdl, ucSlaveAddress, &( pxIntHdl->usFrameMBPDULength ) ) ) )
                {
                    /* Error receiving frame. Abort this transmission. */
                    *peState = MBM_STATE_ERROR;
                }
                else
                {
                    *peState = MBM_STATE_DISASSEMBLE;
                }
                break;

            case MBM_EV_RECV_ERROR:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_CORE, "[IDX=" MBP_FORMAT_USHORT "] Receiver error!\n", ( USHORT ) pxIntHdl->ubIdx );
                }
#endif
                /* Call the receiver to reset its state. */
                ( void )pxIntHdl->pFrameRecvFN( pxIntHdl, ucSlaveAddress, NULL );
                *peState = MBM_STATE_ERROR;
                *peStatus = MB_EIO;
                break;

            case MBM_EV_SENT:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_CORE, "[IDX=" MBP_FORMAT_USHORT "] Frame sent.\n", ( USHORT ) pxIntHdl->ubIdx );
                }
#endif
                *peState = MBM_STATE_DONE;
                *peStatus = MB_ENOERR;
                break;


            case MBM_EV_SEND_ERROR:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_CORE, "[IDX=" MBP_FORMAT_USHORT "] Transmitter error!\n", ( USHORT ) pxIntHdl->ubIdx );
                }
#endif
                *peState = MBM_STATE_ERROR;
                *peStatus = MB_EIO;
                break;

            default:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_CORE ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_CORE, "[IDX=" MBP_FORMAT_USHORT "] Illegal event received!\n", ( USHORT ) pxIntHdl->ubIdx );
                }
#endif
                *peState = MBM_STATE_ERROR;
                *peStatus = MB_EILLSTATE;
                break;
            }
        }
        break;

        /* We can only handle sending, waiting and receiving in this state machine. 
         * Abort in the DEBUG version and go into an error state in the release
         * version. */
    default:
#if defined( MBM_ENABLE_DEBUG_FACILITY ) && ( MBM_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_CORE ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_CORE, "[IDX=" MBP_FORMAT_USHORT "] Illegal state!\n", ( USHORT ) pxIntHdl->ubIdx );
        }
#endif
        *peState = MBM_STATE_ERROR;
        *peStatus = MB_EILLSTATE;
        break;
    }
}

#if MBM_ASCII_ENABLED == 1 || MBM_RTU_ENABLED == 1
eMBErrorCode
eMBMSerialInit( xMBMHandle * pxHdl, eMBSerialMode eMode, UCHAR ucPort, ULONG ulBaudRate, eMBSerialParity eParity )
{
    eMBErrorCode    eStatus;
    UCHAR           ucStopBits;
    ucStopBits = MB_PAR_NONE == eParity ? ( UCHAR ) 2 : ( UCHAR ) 1;
    eStatus = eMBMSerialInitExt( pxHdl, eMode, ucPort, ulBaudRate, eParity, ucStopBits );
    return eStatus;
}

eMBErrorCode
eMBMSerialInitExt( xMBMHandle * pxHdl, eMBSerialMode eMode, UCHAR ucPort, ULONG ulBaudRate, eMBSerialParity eParity, UCHAR ucStopBits )
{
    xMBMInternalHandle *pxMBMNewIntHdl;
    eMBErrorCode    eStatus = MB_EINVAL, eStatus2;
#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
    MBP_ENTER_CRITICAL_INIT(  );
    if( 0 == ubMBMCountInstances(  ) )
    {
        vMBPLibraryLoad(  );
    }
#endif

    if( NULL != pxHdl )
    {
        if( NULL == ( pxMBMNewIntHdl = pxMBMGetNewHdl(  ) ) )
        {
            eStatus = MB_ENORES;
        }
        else
        {
            switch ( eMode )
            {
#if MBM_ASCII_ENABLED == 1
            case MB_ASCII:
                eStatus = eMBMSerialASCIIInit( pxMBMNewIntHdl, ucPort, ulBaudRate, eParity, ucStopBits );
                break;
#endif

#if MBM_RTU_ENABLED == 1
            case MB_RTU:
                eStatus = eMBMSerialRTUInit( pxMBMNewIntHdl, ucPort, ulBaudRate, eParity, ucStopBits );
                break;
#endif

            default:
                eStatus = MB_EINVAL;
                break;
            }
        }

        if( eStatus != MB_ENOERR )
        {
            if( NULL != pxMBMNewIntHdl )
            {
                if( MB_ENOERR != ( eStatus2 = eMBMReleaseHdl( pxMBMNewIntHdl ) ) )
                {
                    eStatus = eStatus2;
                }
            }
            *pxHdl = NULL;
        }
        else
        {
            *pxHdl = pxMBMNewIntHdl;
        }
    }

#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
    /* If the startup failed we have to cleanup. */
    if( 0 == ubMBMCountInstances(  ) )
    {
        vMBPLibraryUnload(  );
    }
    MBP_EXIT_CRITICAL_INIT(  );
#endif

    return eStatus;
}
#endif

#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
STATIC          UBYTE
ubMBMCountInstances( void )
{
    UBYTE           ubIdx;
    UBYTE           ubNInstances = 0;

    if( bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xMBMInternalHdl ); ubIdx++ )
        {
            if( IDX_INVALID != xMBMInternalHdl[ubIdx].ubIdx )
            {
                ubNInstances++;
            }
        }
    }
    return ubNInstances;
}
#endif

#if ( MBM_TCP_ENABLED == 1 )
eMBErrorCode
eMBMTCPInit( xMBMHandle * pxHdl )
{
    xMBMInternalHandle *pxMBMNewIntHdl = NULL;
    eMBErrorCode    eStatus = MB_EINVAL, eStatus2;

#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
    MBP_ENTER_CRITICAL_INIT(  );
    if( 0 == ubMBMCountInstances(  ) )
    {
        vMBPLibraryLoad(  );
    }
#endif

    if( NULL != pxHdl )
    {
        if( NULL == ( pxMBMNewIntHdl = pxMBMGetNewHdl(  ) ) )
        {
            eStatus = MB_ENORES;
        }
        else if( MB_ENOERR != ( eStatus2 = eMBMTCPFrameInit( pxMBMNewIntHdl ) ) )
        {
            eStatus = eStatus2;
        }
        else
        {
            *pxHdl = pxMBMNewIntHdl;
            eStatus = MB_ENOERR;
        }

        if( MB_ENOERR != eStatus )
        {
            if( NULL != pxMBMNewIntHdl )
            {
                if( MB_ENOERR != ( eStatus2 = eMBMReleaseHdl( pxMBMNewIntHdl ) ) )
                {
                    eStatus = eStatus2;
                }
            }
        }
    }

#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
    /* If the startup failed we have to cleanup. */
    if( 0 == ubMBMCountInstances(  ) )
    {
        vMBPLibraryUnload(  );
    }
    MBP_EXIT_CRITICAL_INIT(  );
#endif
    return eStatus;
}

eMBErrorCode
eMBMTCPConnect( xMBMHandle xHdl, const CHAR * pcTCPClientAddress, USHORT usTCPPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

    if( MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) )
    {
        eStatus = eMBMTCPFrameConnect( pxIntHdl, pcTCPClientAddress, usTCPPort );
    }
    return eStatus;
}

eMBErrorCode
eMBMTCPDisconnect( xMBMHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

    if( MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) )
    {
        eStatus = eMBMTCPFrameDisconnect( pxIntHdl );
    }
    return eStatus;
}
#endif

#if ( MBM_UDP_ENABLED == 1 )
eMBErrorCode
eMBMUDPInit( xMBMHandle * pxHdl, const CHAR * pcUDPBindAddress, LONG uUDPListenPort )
{
    xMBMInternalHandle *pxMBMNewIntHdl = NULL;
    eMBErrorCode    eStatus = MB_EINVAL, eStatus2;

#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
    MBP_ENTER_CRITICAL_INIT(  );
    if( 0 == ubMBMCountInstances(  ) )
    {
        vMBPLibraryLoad(  );
    }
#endif

    if( NULL != pxHdl )
    {
        if( NULL == ( pxMBMNewIntHdl = pxMBMGetNewHdl(  ) ) )
        {
            eStatus = MB_ENORES;
        }
        else if( MB_ENOERR != ( eStatus2 = eMBMUDPFrameInit( pxMBMNewIntHdl, pcUDPBindAddress, uUDPListenPort ) ) )
        {
            eStatus = eStatus2;
        }
        else
        {
            *pxHdl = pxMBMNewIntHdl;
            eStatus = MB_ENOERR;
        }

        if( MB_ENOERR != eStatus )
        {
            if( NULL != pxMBMNewIntHdl )
            {
                if( MB_ENOERR != ( eStatus2 = eMBMReleaseHdl( pxMBMNewIntHdl ) ) )
                {
                    eStatus = eStatus2;
                }
            }
        }
    }

#if MBP_ADVA_STARTUP_SHUTDOWN_ENABLED == 1
    /* If the startup failed we have to cleanup. */
    if( 0 == ubMBMCountInstances(  ) )
    {
        vMBPLibraryUnload(  );
    }
    MBP_EXIT_CRITICAL_INIT(  );
#endif
    return eStatus;
}

eMBErrorCode
eMBMUDPSetSlave( xMBMHandle xHdl, const CHAR * pcUDPClientAddress, USHORT usUDPPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

    if( MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) )
    {
        eStatus = eMBMUDPFrameSetClient( pxIntHdl, pcUDPClientAddress, usUDPPort );
    }
    return eStatus;
}
#endif

#if MBM_ENABLE_STATISTICS_INTERFACE == 1
eMBErrorCode
eMBMGetStatistics( xMBMHandle xHdl, xMBStat * pxMBMCurrentStat )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

    if( ( NULL != pxMBMCurrentStat ) && MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        memcpy( pxMBMCurrentStat, &( pxIntHdl->xFrameStat ), sizeof( pxIntHdl->xFrameStat ) );
        MBP_EXIT_CRITICAL_SECTION(  );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eMBMResetStatistics( xMBMHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

    if( MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        MBM_RESET_HDL_STAT( pxIntHdl );
        MBP_EXIT_CRITICAL_SECTION(  );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}
#endif

#if MBM_ENABLE_PROT_ANALYZER_INTERFACE == 1
eMBErrorCode
eMBMRegisterProtAnalyzer( xMBMHandle xHdl, void *pvCtxArg, pvMBAnalyzerCallbackCB pvMBAnalyzerCallbackFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBMInternalHandle *pxIntHdl = xHdl;

    if( MB_IS_VALID_HDL( pxIntHdl, xMBMInternalHdl ) )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        pxIntHdl->pvMBAnalyzerCallbackFN = pvMBAnalyzerCallbackFN;
        pxIntHdl->pvCtx = pvCtxArg;
        MBP_EXIT_CRITICAL_SECTION(  );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}
#endif
