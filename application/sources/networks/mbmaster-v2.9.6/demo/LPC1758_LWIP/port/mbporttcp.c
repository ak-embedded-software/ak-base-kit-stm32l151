/* 
 * MODBUS Library: lwIP/Linux/FreeRTOS port
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * Implemenation notes:
 *  
 * $Id: mbporttcp.c,v 1.2 2014-08-23 11:55:49 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <lwip/api.h>
#include <lwip/sockets.h>
#include <stdlib.h>
#include <string.h>
#if !defined(NO_FREERTOS)
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#endif

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBP_TCP_MASTER_TASK_PRIORITY        ( MBP_TASK_PRIORITY )
#define MBP_TCP_MASTER_TASK_STACKSIZE       ( 256 )
#define MBP_TCP_SLAVE_TASK_PRIORITY         ( MBP_TASK_PRIORITY )
#define MBP_TCP_SLAVE_TASK_STACKSIZE        ( 196 )
#define MBP_TCP_MASTER_QUEUE_SIZE           ( 1 )
#define MBP_TCP_MASTER_THREAD_TIMEOUT       ( 10000 / portTICK_RATE_MS )
#define MAX_MASTER_HDLS                     ( 4 )

#define MAX_SLAVE_HDLS                      ( 4 )       /* Should be equal to MBS_TCP_MAX_INSTANCES */
#define MAX_SLAVE_CLIENT_HDLS               ( 4 )       /* Should be equal to MBS_TCP_MAX_CLIENTS */
#define IDX_INVALID                         ( 255 )

#ifndef MBP_TCP_DEBUG
#define MBP_TCP_DEBUG                       ( 11 )
#endif

#define MBP_TCP_HDL_COMMON \
    UBYTE           ubIdx; \
    xMBTCPIntHandleType eType; \
    BOOL            bIsRunning; \
    xMBHandle       xMBHdl


/* ----------------------- Type definitions ---------------------------------*/

typedef struct
{
    int             iSocket;
    BOOL            bDelete;
} xMBPTCPIntClientHandle;

typedef enum
{
    TCP_MODBUS_UNKNOWN,
    TCP_MODBUS_MASTER,
    TCP_MODBUS_SLAVE
} xMBTCPIntHandleType;

typedef struct
{
    MBP_TCP_HDL_COMMON;
} xMBPTCPIntCommonHandle;

typedef struct
{
    MBP_TCP_HDL_COMMON;
    xMBPTCPIntClientHandle xClientCon;
    peMBPTCPClientNewDataCB eMBPTCPClientNewDataFN;
    peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN;
} xMBPTCPIntMasterHandle;

typedef struct
{
    MBP_TCP_HDL_COMMON;
    xMBPTCPIntClientHandle xServerCon;  /* Listening socket */
    xMBPTCPIntClientHandle xClientCons[MAX_SLAVE_CLIENT_HDLS];  /* Active clients */
    peMBPTCPClientNewDataCB eMBPTCPClientNewDataFN;
    peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN;
    peMBPTCPClientConnectedCB eMBPTCPClientConnectedFN;
} xMBPTCPIntSlaveHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xMBPTCPIntMasterHandle xMBTCPMasterHdls[MAX_MASTER_HDLS];
STATIC xMBPTCPIntSlaveHandle xMBTCPSlaveHdls[MAX_SLAVE_HDLS];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vMBPTCPInit( void );
STATIC void     vMBTCPCommonHandleReset( xMBPTCPIntCommonHandle * pxTCPHdl, BOOL bClose );
STATIC void     vMBTCPMasterHandleReset( xMBPTCPIntMasterHandle * pxTCPMasterHdl, BOOL bClose, BOOL bFullReset );
STATIC void     vMBTCPMasterHandlerThread( void *pvArg );
STATIC void     vMBTCPClientHandleReset( xMBPTCPIntClientHandle * pxClientHdl, BOOL bClose );
STATIC void     vMBTCPSlaveHandleReset( xMBPTCPIntSlaveHandle * pxTCPSlaveHdl, BOOL bClose );
STATIC void     vMBTCPSlaveHandlerThread( void *pvArg );

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
STATIC const char *pszMBTCPHdlType2Str( xMBTCPIntHandleType );
#endif

/* ----------------------- Start implementation -----------------------------*/

/* --------------------------------------------------------------------------*/
/* ----------------------- MODBUS MASTER CODE -------------------------------*/
/* --------------------------------------------------------------------------*/
eMBErrorCode
eMBPTCPClientInit( xMBPTCPHandle * pxTCPHdl, xMBHandle xMBHdlArg,
                   peMBPTCPClientNewDataCB eMBPTCPClientNewDATAFNArg, peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFNArg )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntMasterHandle *pxTCPMasterIntHdl = NULL;
    UBYTE           ubIdx;

    vMBPTCPInit(  );
    if( NULL != pxTCPHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPMasterHdls ); ubIdx++ )
        {
            if( IDX_INVALID == xMBTCPMasterHdls[ubIdx].ubIdx )
            {
                pxTCPMasterIntHdl = &xMBTCPMasterHdls[ubIdx];
                vMBTCPMasterHandleReset( pxTCPMasterIntHdl, FALSE, TRUE );
                pxTCPMasterIntHdl->ubIdx = ubIdx;
                break;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );

        if( NULL != pxTCPMasterIntHdl )
        {
            pxTCPMasterIntHdl->eMBPTCPClientNewDataFN = eMBPTCPClientNewDATAFNArg;
            pxTCPMasterIntHdl->eMBPTCPClientDisconnectedFN = eMBPTCPClientDisconnectedFNArg;
            pxTCPMasterIntHdl->xMBHdl = xMBHdlArg;
            pxTCPMasterIntHdl->bIsRunning = TRUE;
            if( NULL ==
                sys_thread_new( "MBP-TCP-MASTER", vMBTCPMasterHandlerThread, pxTCPMasterIntHdl, MBP_TCP_MASTER_TASK_STACKSIZE, MBP_TCP_MASTER_TASK_PRIORITY ) )
            {
                eStatus = MB_EPORTERR;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[MBM=%hu] Can't create MODBUS master handler thread!\n", ( USHORT ) pxTCPMasterIntHdl->ubIdx );
                }
#endif
            }
            else
            {
                *pxTCPHdl = pxTCPMasterIntHdl;
                eStatus = MB_ENOERR;
            }
            if( MB_ENOERR != eStatus )
            {
                vMBTCPMasterHandleReset( pxTCPMasterIntHdl, TRUE, TRUE );
            }
        }
        else
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "Can't create MODBUS master instance. To many instances open!\n" );
            }
#endif
            eStatus = MB_ENORES;
        }
    }

    return eStatus;
}

eMBErrorCode
eMBPTCPClientOpen( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle * pxTCPClientHdl, const CHAR * pcConnectAddress, USHORT usTCPPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntMasterHandle *pxTCPMasterIntHdl = xTCPHdl;
    struct sockaddr_in xClientAddr;
    int             iSockAddr;
    int             iDontBlock = 1;

    if( MB_IS_VALID_HDL( pxTCPMasterIntHdl, xMBTCPMasterHdls ) && ( pxTCPMasterIntHdl->eType == TCP_MODBUS_MASTER ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[MBM=%hu] Connecting to (%s:%hu).\n",
                         ( USHORT ) pxTCPMasterIntHdl->ubIdx, pcConnectAddress, usTCPPort );
        }
#endif
        memset( &xClientAddr, 0, sizeof( struct sockaddr_in ) );
        if( inet_aton( pcConnectAddress, ( struct in_addr * )&xClientAddr.sin_addr ) )
        {
            xClientAddr.sin_family = AF_INET;
            xClientAddr.sin_port = htons( usTCPPort );
            if( ( iSockAddr = lwip_socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[MBM=%hu] Can not create lwIP socket: %d!\n", ( USHORT ) pxTCPMasterIntHdl->ubIdx, errno );
                }
#endif
            }
            else if( lwip_connect( iSockAddr, ( const struct sockaddr * )&xClientAddr, sizeof( struct sockaddr_in ) ) < 0 )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[MBM=%hu] Can not connect to %s with socket %d: %d!\n",
                                 ( USHORT ) pxTCPMasterIntHdl->ubIdx, pcConnectAddress, iSockAddr, errno );
                }
#endif
                ( void )lwip_close( iSockAddr );
            }
            else if( lwip_ioctl( iSockAddr, FIONBIO, &iDontBlock ) < 0 )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                                 "[MBM=%hu] Can not set socket %d to non blocking: %d!\n", ( USHORT ) pxTCPMasterIntHdl->ubIdx, iSockAddr, errno );
                }
#endif
                ( void )lwip_close( iSockAddr );
            }
            else
            {
                MBP_ENTER_CRITICAL_SECTION(  );
                if( -1 != pxTCPMasterIntHdl->xClientCon.iSocket )
                {
                    ( void )lwip_close( iSockAddr );
                    eStatus = MB_ENORES;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                                     "[MBM=%hu] Old client connection still open (socket %d)!\n",
                                     ( USHORT ) pxTCPMasterIntHdl->ubIdx, pxTCPMasterIntHdl->xClientCon.iSocket );
                    }
#endif
                }
                else
                {
                    pxTCPMasterIntHdl->xClientCon.iSocket = iSockAddr;
                    *pxTCPClientHdl = &( pxTCPMasterIntHdl->xClientCon );
                    eStatus = MB_ENOERR;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_INFO, MB_LOG_PORT_TCP,
                                     "[MBM=%hu] New client connection (%s:%hu) established using socket %d.\n",
                                     ( USHORT ) pxTCPMasterIntHdl->ubIdx, pcConnectAddress, usTCPPort, pxTCPMasterIntHdl->xClientCon.iSocket );
                    }
#endif
                }
                MBP_EXIT_CRITICAL_SECTION(  );
            }
        }
    }
    else
    {
        eStatus = MB_EPORTERR;
    }
    return eStatus;
}

eMBErrorCode
eMBPTCPClientClose( xMBPTCPHandle xTCPHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntMasterHandle *pxTCPMasterIntHdl = xTCPHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTCPMasterIntHdl, xMBTCPMasterHdls ) && ( pxTCPMasterIntHdl->eType == TCP_MODBUS_MASTER ) )
    {
        pxTCPMasterIntHdl->bIsRunning = FALSE;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

STATIC void
vMBTCPMasterHandleReset( xMBPTCPIntMasterHandle * pxTCPMasterHdl, BOOL bClose, BOOL bFullReset )
{
    MBP_ASSERT( NULL != pxTCPMasterHdl );
    vMBTCPClientHandleReset( &( pxTCPMasterHdl->xClientCon ), bClose );
    if( bFullReset )
    {
        vMBTCPCommonHandleReset( ( xMBPTCPIntCommonHandle * ) pxTCPMasterHdl, bClose );
        pxTCPMasterHdl->eMBPTCPClientNewDataFN = NULL;
        pxTCPMasterHdl->eMBPTCPClientDisconnectedFN = NULL;
        pxTCPMasterHdl->eType = TCP_MODBUS_MASTER;
    }
}

void
vMBTCPMasterHandlerThread( void *pvArg )
{
    BOOL            bIsRunning = FALSE;
    xMBPTCPIntMasterHandle *pxTCPMasterIntHdl = pvArg;
    struct fd_set   xClientReadSet;
    struct fd_set   xClientErrorSet;
    struct timeval  xTimeout;
    int             iClientSocket;

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    UBYTE           ubOldIdx;
#endif

    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( TRUE == ( bIsRunning = pxTCPMasterIntHdl->bIsRunning ) )
        {
            /* Perform garbage collection on left over client connections. */
            if( pxTCPMasterIntHdl->xClientCon.bDelete )
            {
                vMBTCPClientHandleReset( &( pxTCPMasterIntHdl->xClientCon ), TRUE );

            }
            if( -1 != pxTCPMasterIntHdl->xClientCon.iSocket )
            {
                FD_ZERO( &xClientReadSet );
                FD_ZERO( &xClientErrorSet );
                iClientSocket = pxTCPMasterIntHdl->xClientCon.iSocket;
                FD_SET( iClientSocket, &xClientReadSet );
                FD_SET( iClientSocket, &xClientErrorSet );
                xTimeout.tv_sec = 0;
                xTimeout.tv_usec = 10000;
                MBP_EXIT_CRITICAL_SECTION(  );
                if( lwip_select( iClientSocket + 1, &xClientReadSet, NULL, &xClientErrorSet, &xTimeout ) < 0 )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                                     "[MBM=%hu] Select failed on socket %d with error: %d\n", ( USHORT ) pxTCPMasterIntHdl->ubIdx, iClientSocket, errno );
                    }
#endif
                }
                else
                {
                    MBP_ENTER_CRITICAL_SECTION(  );
                    /* Some time has passed between the start of the select and its end.
                     * Therefore the master instance could be marked as closed.
                     */
                    if( TRUE == ( bIsRunning = pxTCPMasterIntHdl->bIsRunning ) )
                    {
                        if( !pxTCPMasterIntHdl->xClientCon.bDelete && FD_ISSET( iClientSocket, &xClientReadSet ) )
                        {
                            MBP_ASSERT( NULL != pxTCPMasterIntHdl->eMBPTCPClientNewDataFN );
                            ( void )pxTCPMasterIntHdl->eMBPTCPClientNewDataFN( pxTCPMasterIntHdl->xMBHdl, &( pxTCPMasterIntHdl->xClientCon ) );
                        }
                        if( !pxTCPMasterIntHdl->xClientCon.bDelete && FD_ISSET( iClientSocket, &xClientErrorSet ) )
                        {
                            MBP_ASSERT( NULL != pxTCPMasterIntHdl->eMBPTCPClientDisconnectedFN );
                            ( void )pxTCPMasterIntHdl->eMBPTCPClientDisconnectedFN( pxTCPMasterIntHdl->xMBHdl, &( pxTCPMasterIntHdl->xClientCon ) );
                        }
                    }
                    MBP_EXIT_CRITICAL_SECTION(  );
                }
            }
            else
            {
                MBP_EXIT_CRITICAL_SECTION(  );
            }
        }
        else
        {
            MBP_EXIT_CRITICAL_SECTION(  );
			/* Nothing to do - Yield */
#if !defined(NO_FREERTOS)
			taskYIELD(  );
#else            
            usleep( 1000 );
#endif
        }

    }
    while( bIsRunning );

    MBP_ENTER_CRITICAL_SECTION(  );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    ubOldIdx = pxTCPMasterIntHdl->ubIdx;
#endif
    vMBTCPMasterHandleReset( pxTCPMasterIntHdl, TRUE, TRUE );
    MBP_EXIT_CRITICAL_SECTION(  );

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[MBM=%hu] Master thread exiting!\n", ubOldIdx );
    }
#endif
#if !defined(NO_FREERTOS)
    vTaskDelete( NULL );
#endif
}

/* --------------------------------------------------------------------------*/
/* ----------------------- MODBUS SLAVE CODE --------------------------------*/
/* --------------------------------------------------------------------------*/
eMBErrorCode
eMBPTCPServerInit( xMBPTCPHandle * pxTCPHdl, CHAR * pcBindAddress,
                   USHORT usTCPPort,
                   xMBHandle xMBHdlArg,
                   peMBPTCPClientNewDataCB eMBPTCPClientNewDataFNArg,
                   peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFNArg, peMBPTCPClientConnectedCB eMBPTCPClientConnectedFNArg )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntSlaveHandle *pxTCPIntSlaveHdl = NULL;
    UBYTE           ubIdx;
    struct sockaddr_in xListenAddr;
    int             iSockAddr;
    int             iDontBlock = 1;

    vMBPTCPInit(  );
    if( NULL != pxTCPHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPSlaveHdls ); ubIdx++ )
        {
            if( IDX_INVALID == xMBTCPSlaveHdls[ubIdx].ubIdx )
            {
                pxTCPIntSlaveHdl = &xMBTCPSlaveHdls[ubIdx];
                vMBTCPSlaveHandleReset( pxTCPIntSlaveHdl, FALSE );
                pxTCPIntSlaveHdl->ubIdx = ubIdx;
                break;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
        if( NULL != pxTCPIntSlaveHdl )
        {
            memset( &xListenAddr, 0, sizeof( struct sockaddr_in ) );
            if( inet_aton( pcBindAddress, ( struct in_addr * )&xListenAddr.sin_addr ) )
            {
                xListenAddr.sin_family = AF_INET;
                xListenAddr.sin_port = htons( usTCPPort );

                if( ( iSockAddr = lwip_socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[MBS=%hu] Can not create lwIP socket: %d!\n", ( USHORT ) pxTCPIntSlaveHdl->ubIdx, errno );
                    }
#endif
                }
                else if( lwip_bind( iSockAddr, ( struct sockaddr * )&xListenAddr, sizeof( xListenAddr ) ) < 0 )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[MBS=%hu] Can not bind to %s:%hu: %d!\n",
                                     ( USHORT ) pxTCPIntSlaveHdl->ubIdx, pcBindAddress, usTCPPort, errno );
                    }
#endif
                    ( void )close( iSockAddr );
                }
                else if( lwip_listen( iSockAddr, MAX_SLAVE_CLIENT_HDLS ) < 0 )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[MBS=%hu] Can not listen on socket %d: %d!\n",
                                     ( USHORT ) pxTCPIntSlaveHdl->ubIdx, iSockAddr, errno );
                    }
#endif
                }
                else if( lwip_ioctl( iSockAddr, FIONBIO, &iDontBlock ) < 0 )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                                     "[MBS=%hu] Can not set socket %d to non blocking: %d!\n", ( USHORT ) pxTCPIntSlaveHdl->ubIdx, iSockAddr, errno );
                    }
#endif
                }
                else
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[MBS=%hu] created listening socket %d.\n", ( USHORT ) pxTCPIntSlaveHdl->ubIdx, iSockAddr );
                    }
#endif

                    pxTCPIntSlaveHdl->xServerCon.iSocket = iSockAddr;
                    pxTCPIntSlaveHdl->eMBPTCPClientNewDataFN = eMBPTCPClientNewDataFNArg;
                    pxTCPIntSlaveHdl->eMBPTCPClientDisconnectedFN = eMBPTCPClientDisconnectedFNArg;
                    pxTCPIntSlaveHdl->eMBPTCPClientConnectedFN = eMBPTCPClientConnectedFNArg;
                    pxTCPIntSlaveHdl->xMBHdl = xMBHdlArg;
                    pxTCPIntSlaveHdl->bIsRunning = TRUE;
                    if( NULL == sys_thread_new( "MBP-TCP-SLAVE", vMBTCPSlaveHandlerThread, pxTCPIntSlaveHdl,
                                                MBP_TCP_SLAVE_TASK_STACKSIZE, MBP_TCP_SLAVE_TASK_PRIORITY ) )
                    {
                        eStatus = MB_EPORTERR;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                        {
                            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                                         "[MBM=%hu] Can't create SLAVE master handler thread!\n", ( USHORT ) pxTCPIntSlaveHdl->ubIdx );
                        }
#endif
                    }
                    else
                    {
                        *pxTCPHdl = pxTCPIntSlaveHdl;
                        eStatus = MB_ENOERR;
                    }
                }
            }
            if( MB_ENOERR != eStatus )
            {
                vMBTCPSlaveHandleReset( pxTCPIntSlaveHdl, TRUE );
            }
        }
        else
        {
            eStatus = MB_ENORES;
        }
    }
    return eStatus;
}

STATIC void
vMBTCPSlaveHandleReset( xMBPTCPIntSlaveHandle * pxTCPSlaveHdl, BOOL bClose )
{
    UBYTE           ubIdx;

    MBP_ASSERT( NULL != pxTCPSlaveHdl );
    for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( pxTCPSlaveHdl->xClientCons ); ubIdx++ )
    {
        vMBTCPClientHandleReset( &( pxTCPSlaveHdl->xClientCons[ubIdx] ), bClose );
    }
    vMBTCPClientHandleReset( &( pxTCPSlaveHdl->xServerCon ), bClose );
    vMBTCPCommonHandleReset( ( xMBPTCPIntCommonHandle * ) pxTCPSlaveHdl, bClose );
    pxTCPSlaveHdl->eMBPTCPClientNewDataFN = NULL;
    pxTCPSlaveHdl->eMBPTCPClientDisconnectedFN = NULL;
    pxTCPSlaveHdl->eMBPTCPClientConnectedFN = NULL;
    pxTCPSlaveHdl->eType = TCP_MODBUS_SLAVE;
}

void
vprvMBTCPServerAcceptClient( xMBPTCPIntSlaveHandle * pxTCPIntSlaveHdl )
{
    int             iClientSocket;
    struct sockaddr_in xClientAddr;
    socklen_t       xClientAddrLen;
    int             iDontBlock = 1;
    UBYTE           ubIdx;
    eMBErrorCode    eStatus;
    BOOL            bDropClient = TRUE;
    int             optval;

    if( ( iClientSocket = lwip_accept( pxTCPIntSlaveHdl->xServerCon.iSocket, ( struct sockaddr * )&xClientAddr, &xClientAddrLen ) ) < 0 )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[MBS=%hu] Accept failed: %d\n", ( USHORT ) pxTCPIntSlaveHdl->ubIdx, errno );
        }
#endif
    }
    else if( lwip_ioctl( iClientSocket, FIONBIO, &iDontBlock ) < 0 )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                         "[MBS=%hu] Can not set socket %d to non blocking: %d!\n", ( USHORT ) pxTCPIntSlaveHdl->ubIdx, iClientSocket, errno );
        }
#endif

    }
    else
    {
        /* enable TCP keep alive timers */
        optval = 1;
        ( void )lwip_setsockopt( iClientSocket, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof( optval ) );

        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( pxTCPIntSlaveHdl->xClientCons ); ubIdx++ )
        {
            if( -1 == pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket )
            {
                MBP_ASSERT( NULL != pxTCPIntSlaveHdl->eMBPTCPClientConnectedFN );
                pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket = iClientSocket;
                if( MB_ENOERR !=
                    ( eStatus = pxTCPIntSlaveHdl->eMBPTCPClientConnectedFN( pxTCPIntSlaveHdl->xMBHdl, &( pxTCPIntSlaveHdl->xClientCons[ubIdx] ) ) ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                                     "[MBS=%hu] Instance closed or MODBUS stack lacking resources. Dropping client socket %d. Status = %d!\n",
                                     ( USHORT ) pxTCPIntSlaveHdl->ubIdx, ubIdx, iClientSocket, ( int )eStatus );
                    }
#endif
                    /* Do not close socket in this function because we close it
                     * using the bDropClient flag.
                     */
                    vMBTCPClientHandleReset( &( pxTCPIntSlaveHdl->xClientCons[ubIdx] ), FALSE );
                }
                else
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[MBS=%hu] Accepted new client %d on socket %d\n",
                                     ( USHORT ) pxTCPIntSlaveHdl->ubIdx, ubIdx, pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket );
                    }
#endif
                    bDropClient = FALSE;
                }
                break;
            }

        }
    }
    if( bDropClient && ( -1 != iClientSocket ) )
    {
        ( void )lwip_close( iClientSocket );
    }

    /* Compiler warning - Variable not used in debug */
    ( void )eStatus;
}

void
vMBTCPSlaveHandlerThread( void *pvArg )
{
    BOOL            bIsRunning = FALSE;
    xMBPTCPIntSlaveHandle *pxTCPIntSlaveHdl = pvArg;
    struct fd_set   xClientReadSet;
    struct fd_set   xClientErrorSet;
    struct timeval  xTimeout;
    UBYTE           ubIdx;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    UBYTE           ubOldIdx;
#endif
    int             iSocketLast;

    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( TRUE == ( bIsRunning = pxTCPIntSlaveHdl->bIsRunning ) )
        {
            /* Perform garbage collection on old client handles. */
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( pxTCPIntSlaveHdl->xClientCons ); ubIdx++ )
            {
                if( pxTCPIntSlaveHdl->xClientCons[ubIdx].bDelete )
                {
                    vMBTCPClientHandleReset( &( pxTCPIntSlaveHdl->xClientCons[ubIdx] ), TRUE );
                }
            }
            FD_ZERO( &xClientReadSet );
            FD_ZERO( &xClientErrorSet );
            /* First add listening socket to the list of file descriptors. */
            FD_SET( pxTCPIntSlaveHdl->xServerCon.iSocket, &xClientReadSet );
            iSocketLast = pxTCPIntSlaveHdl->xServerCon.iSocket;
            /* Now add all active client sockets to the list of file descriptors. */
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( pxTCPIntSlaveHdl->xClientCons ); ubIdx++ )
            {
                if( -1 != pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket )
                {
                    FD_SET( pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket, &xClientReadSet );
                    FD_SET( pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket, &xClientErrorSet );
                    if( iSocketLast < pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket )
                    {
                        iSocketLast = pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket;
                    }
                }
            }
            xTimeout.tv_sec = 0;
            xTimeout.tv_usec = 10000;
            MBP_EXIT_CRITICAL_SECTION(  );
            if( lwip_select( iSocketLast + 1, &xClientReadSet, NULL, &xClientErrorSet, &xTimeout ) < 0 )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[MBS=%hu] Select failed with error: %d\n", ( USHORT ) pxTCPIntSlaveHdl->ubIdx, errno );
                }
#endif
            }
            else
            {
                MBP_ENTER_CRITICAL_SECTION(  );
                /* Since lock has not spawn the select we could have been closed
                 * in the meantime.
                 */
                if( TRUE == ( bIsRunning = pxTCPIntSlaveHdl->bIsRunning ) )
                {
                    /* First check for listening events. */
                    if( FD_ISSET( pxTCPIntSlaveHdl->xServerCon.iSocket, &xClientReadSet ) )
                    {
                        vprvMBTCPServerAcceptClient( pxTCPIntSlaveHdl );
                    }
                    /* Now check all possible active client connections. */
                    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( pxTCPIntSlaveHdl->xClientCons ); ubIdx++ )
                    {
                    	if( -1 != pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket )
                    	{
							if( !pxTCPIntSlaveHdl->xClientCons[ubIdx].bDelete && FD_ISSET( pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket, &xClientErrorSet ) )
							{
								MBP_ASSERT( NULL != pxTCPIntSlaveHdl->eMBPTCPClientDisconnectedFN );
								( void )pxTCPIntSlaveHdl->eMBPTCPClientDisconnectedFN( pxTCPIntSlaveHdl->xMBHdl, &( pxTCPIntSlaveHdl->xClientCons[ubIdx] ) );
							}
							/* Recheck delete flag since data callback could mark connection as dead. */
							if( !pxTCPIntSlaveHdl->xClientCons[ubIdx].bDelete && FD_ISSET( pxTCPIntSlaveHdl->xClientCons[ubIdx].iSocket, &xClientReadSet ) )
							{
								MBP_ASSERT( NULL != pxTCPIntSlaveHdl->eMBPTCPClientNewDataFN );
								( void )pxTCPIntSlaveHdl->eMBPTCPClientNewDataFN( pxTCPIntSlaveHdl->xMBHdl, &( pxTCPIntSlaveHdl->xClientCons[ubIdx] ) );
							}
                    	}
                    }
                }
                MBP_EXIT_CRITICAL_SECTION(  );
            }
        }
        else
        {
            MBP_EXIT_CRITICAL_SECTION(  );
            /* Nothing to do - Yield */
#if !defined(NO_FREERTOS)
			taskYIELD(  );
#else
            usleep( 1000 );
#endif
        }
    }
    while( bIsRunning );

    MBP_ENTER_CRITICAL_SECTION(  );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    ubOldIdx = pxTCPIntSlaveHdl->ubIdx;
#endif
    vMBTCPSlaveHandleReset( pxTCPIntSlaveHdl, TRUE );
    MBP_EXIT_CRITICAL_SECTION(  );

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[MBS=%hu] Slave thread exiting!\n", ubOldIdx );
    }
#endif
#if !defined(NO_FREERTOS)
	vTaskDelete( NULL );
#endif
}

eMBErrorCode
eMBTCPServerClose( xMBPTCPHandle xTCPHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntSlaveHandle *pxTCPIntSlaveHdl = xTCPHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTCPIntSlaveHdl, xMBTCPSlaveHdls ) && ( pxTCPIntSlaveHdl->eType == TCP_MODBUS_SLAVE ) )
    {
        pxTCPIntSlaveHdl->bIsRunning = FALSE;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[MBS=%hu] Closing slave instance!\n", pxTCPIntSlaveHdl->ubIdx );
        }
#endif
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

/* --------------------------------------------------------------------------*/
/* ----------------------- COMMON CODE --------------------------------------*/
/* --------------------------------------------------------------------------*/

STATIC void
vMBPTCPInit(  )
{
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPMasterHdls ); ubIdx++ )
        {
            vMBTCPMasterHandleReset( &xMBTCPMasterHdls[ubIdx], FALSE, TRUE );
        }
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPSlaveHdls ); ubIdx++ )
        {
            vMBTCPSlaveHandleReset( &xMBTCPSlaveHdls[ubIdx], FALSE );
        }
        bIsInitalized = TRUE;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}

eMBErrorCode
eMBPTCPConClose( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntCommonHandle *pxTCPIntCommonHdl = xTCPHdl;
    xMBPTCPIntClientHandle *pxTCPIntClientHdl = xTCPClientHdl;

    ( void )pxTCPIntCommonHdl;

    if( NULL != xTCPClientHdl )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                         "[%s=%hu] Marking client connection with socket %d for deletion.\n",
                         pszMBTCPHdlType2Str( pxTCPIntCommonHdl->eType ), ( USHORT ) pxTCPIntCommonHdl->ubIdx, pxTCPIntClientHdl->iSocket );
        }
#endif
        MBP_ENTER_CRITICAL_SECTION(  );
        pxTCPIntClientHdl->bDelete = TRUE;
        MBP_EXIT_CRITICAL_SECTION(  );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

void
vMBTCPClientHandleReset( xMBPTCPIntClientHandle * pxClientHdl, BOOL bClose )
{
    if( bClose && ( -1 != pxClientHdl->iSocket ) )
    {
        if( close( pxClientHdl->iSocket ) < 0 )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[MBS/MBM=?] Close failed on socket %d with error: %d\n", pxClientHdl->iSocket, errno );
            }
#endif
        }
        else
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[MBS/MBM=?] Closed socket %d.\n", pxClientHdl->iSocket );
            }
#endif
        }
    }
    pxClientHdl->iSocket = -1;
    pxClientHdl->bDelete = FALSE;
}

STATIC void
vMBTCPCommonHandleReset( xMBPTCPIntCommonHandle * pxTCPCommonHdl, BOOL bClose )
{
    pxTCPCommonHdl->ubIdx = IDX_INVALID;
    pxTCPCommonHdl->eType = TCP_MODBUS_UNKNOWN;
    pxTCPCommonHdl->bIsRunning = FALSE;
    pxTCPCommonHdl->xMBHdl = MB_HDL_INVALID;
}

eMBErrorCode
eMBPTCPConRead( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, UBYTE * pubBuffer, USHORT * pusBufferLen, USHORT usBufferMax )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntCommonHandle *pxTCPIntCommonHdl = xTCPHdl;
    xMBPTCPIntClientHandle *pxTCPIntClientHdl = xTCPClientHdl;

    int             iErr;

    ( void )pxTCPIntCommonHdl;
    if( NULL != xTCPClientHdl )
    {
        *pusBufferLen = 0;
        iErr = lwip_read( pxTCPIntClientHdl->iSocket, pubBuffer, usBufferMax );
        if( iErr < 0 )
        {
            if( ( errno == ETIMEDOUT ) || ( errno == EWOULDBLOCK ) )
            {
                eStatus = MB_ENOERR;
            }
            else
            {
                eStatus = MB_EIO;
            }
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                             "[%s=%hu] Read failed on socket %d with %d. Return status = %d.\n",
                             pszMBTCPHdlType2Str( pxTCPIntCommonHdl->eType ), ( USHORT ) pxTCPIntCommonHdl->ubIdx,
                             pxTCPIntClientHdl->iSocket, errno, ( int )eStatus );
            }
#endif
        }
        /* Client has closed connection. */
        else if( 0 == iErr )
        {
            eStatus = MB_EIO;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                             "[%s=%hu] Client has closed connection on socket %d!. Return status = %d.\n",
                             pszMBTCPHdlType2Str( pxTCPIntCommonHdl->eType ), ( USHORT ) pxTCPIntCommonHdl->ubIdx, pxTCPIntClientHdl->iSocket, ( int )eStatus );
            }
#endif
        }
        /* Data has been available */
        else
        {
            eStatus = MB_ENOERR;
            *pusBufferLen = iErr;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                             "[%s=%hu] Read %hu bytes on socket %d. Return status = %d.\n",
                             pszMBTCPHdlType2Str( pxTCPIntCommonHdl->eType ), ( USHORT ) pxTCPIntCommonHdl->ubIdx,
                             *pusBufferLen, pxTCPIntClientHdl->iSocket, ( int )eStatus );
            }
#endif
        }
    }
    return eStatus;
}

eMBErrorCode
eMBPTCPConWrite( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, const UBYTE * pubBuffer, USHORT usBufferLen )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntCommonHandle *pxTCPIntCommonHdl = xTCPHdl;
    xMBPTCPIntClientHandle *pxTCPIntClientHdl = xTCPClientHdl;

    ( void )pxTCPIntCommonHdl;
    if( NULL != xTCPClientHdl )
    {
        if( lwip_write( pxTCPIntClientHdl->iSocket, pubBuffer, ( size_t ) usBufferLen ) < 0 )
        {
            eStatus = MB_EIO;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                             "[%s=%hu] Writing %hu bytes on socket %d failed with %d! Return status = %d.\n",
                             pszMBTCPHdlType2Str( pxTCPIntCommonHdl->eType ), ( USHORT ) pxTCPIntCommonHdl->ubIdx,
                             usBufferLen, pxTCPIntClientHdl->iSocket, errno, ( int )eStatus );
            }
#endif
        }
        else
        {
            eStatus = MB_ENOERR;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                             "[%s=%hu] Wrote %hu bytes on socket %d. Return status = %d.\n",
                             pszMBTCPHdlType2Str( pxTCPIntCommonHdl->eType ), ( USHORT ) pxTCPIntCommonHdl->ubIdx,
                             usBufferLen, pxTCPIntClientHdl->iSocket, ( int )eStatus );
            }
#endif
        }

    }
    return eStatus;
}

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
const char     *
pszMBTCPHdlType2Str( xMBTCPIntHandleType eType )
{
    const char     *pszRetVal;

    switch ( eType )
    {
    case TCP_MODBUS_UNKNOWN:
        pszRetVal = "?";
        break;
    case TCP_MODBUS_MASTER:
        pszRetVal = "MBM";
        break;
    case TCP_MODBUS_SLAVE:
        pszRetVal = "MBS";
        break;
    }
    return pszRetVal;
}
#endif
