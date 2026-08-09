/* 
 * MODBUS Library: LINUX/CYGWIN TCP port
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbporttcp.c,v 1.10 2014-08-23 09:36:06 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <netinet/tcp.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define USE_CONNECTWAITFORPENDING           ( 1 )

#define MAX_HDLS                            ( 4 )
#define MAX_LISTENING_SOCKS                 ( 4 )
#define MAX_CLIENT_SOCKS                    ( 2 )
#define IDX_INVALID                         ( 255 )
#define THREAD_STACKSIZE_MAX                ( 16384 )

#define MBP_WAIT_TIMEOUTS                   ( 50 )
#define MBP_POLL_TIMEOUT_MS                 ( 100 )

#define TCP_KEEPALIVE						( 1 )
#define TCP_KEEPALIVE_TIME					( 60 )
#define TCP_KEEPALIVE_INTVL					( 10 )
#define TCP_KEEPALIVE_PROBES				( 3 )

#define INVALID_SOCKET                      ( -1 )

#define max( x, y )   ((x)>(y) ? (x) : (y))

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    struct pollfd   xClientSocket;
    peMBPTCPClientNewDataCB eMBPTCPClientNewDataFN;
    peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN;
} xMBPTCPClientSock;

typedef struct
{
    struct pollfd   xSrvSocket[MAX_LISTENING_SOCKS];
    struct pollfd   xClientSocket[MAX_CLIENT_SOCKS];
    peMBPTCPClientNewDataCB eMBPTCPClientNewDataFN;
    peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN;
    peMBPTCPClientDisconnectedCB eMBPTCPClientConnectedFN;
} xMBPTCPServerSock;

typedef enum
{
    UNKNOWN,
    TCP_SERVER,
    TCP_CLIENT
} xMBTCPIntHandleType;

typedef struct
{
    UBYTE           ubIdx;
    xMBTCPIntHandleType eType;
    BOOL            bIsRunning;
    BOOL            bHasFailed;
    pthread_t       pxHandlingThread;
    xMBHandle       xMBHdl;
    union
    {
        xMBPTCPServerSock xServerSock;
        xMBPTCPClientSock xClientSock;
    } xShared;
} xMBPTCPIntHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xMBPTCPIntHandle xMBTCPHdl[MAX_HDLS];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vMBPTCPInit(  );
STATIC eMBErrorCode eMBPTCPInitCommon( xMBPTCPIntHandle * pxTCPIntHdl, void *( *pvHandlerThread ) ( void * ) );
STATIC void     vMBTCPHandleReset( xMBPTCPIntHandle * pxTCPHdl, BOOL bClose );
STATIC void    *vMBTCPClientHandlerThread( void *xMBPTCPIntHandle );
STATIC void     vMBTCPClientHandleReset( struct pollfd *pxPollFd, BOOL bClose );
STATIC void    *vMBTCPServerHandlerThread( void *xMBPTCPIntHandle );
STATIC void     vMBPTCPSocketEnableKeepalive( int socket, int iKeepAlive, int iKeepAliveTime, int iIntvl, int iProbes );

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBPTCPClientInit( xMBPTCPHandle * pxTCPHdl, xMBHandle xMBHdlArg,
                   peMBPTCPClientNewDataCB eMBPTCPClientNewDATAFNArg, peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFNArg )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntHandle *pxTCPIntHdl = NULL;
    UBYTE           ubIdx;

    vMBPTCPInit(  );
    if( NULL != pxTCPHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xMBTCPHdl[ubIdx].ubIdx )
            {
                pxTCPIntHdl = &xMBTCPHdl[ubIdx];
                /* Reset the data structure for a client. It could have been used
                 * by a server before.
                 */
                pxTCPIntHdl->eType = TCP_CLIENT;
                vMBTCPHandleReset( pxTCPIntHdl, FALSE );
                pxTCPIntHdl->ubIdx = ubIdx;
                break;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );

        if( NULL != pxTCPIntHdl )
        {
            pxTCPIntHdl->xShared.xClientSock.eMBPTCPClientNewDataFN = eMBPTCPClientNewDATAFNArg;
            pxTCPIntHdl->xShared.xClientSock.eMBPTCPClientDisconnectedFN = eMBPTCPClientDisconnectedFNArg;
            pxTCPIntHdl->xMBHdl = xMBHdlArg;
            if( MB_ENOERR != eMBPTCPInitCommon( pxTCPIntHdl, vMBTCPClientHandlerThread ) )
            {
                vMBTCPHandleReset( pxTCPIntHdl, FALSE );
                eStatus = MB_EPORTERR;
            }
            else
            {
                *pxTCPHdl = pxTCPIntHdl;
                eStatus = MB_ENOERR;
            }
        }
        else
        {
            eStatus = MB_ENORES;
        }
    }

    return eStatus;
}

eMBErrorCode
eMBPTCPClientOpen( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle * pxTCPClientHdl, const CHAR * pcConnectAddress, USHORT usTCPPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntHandle *pxTCPIntHdl = xTCPHdl;
    struct pollfd  *pxPollFd;
    struct addrinfo *ai0 = NULL, hints;
    char            arcServiceName[6];
    int             iSockAddr;

#if USE_CONNECTWAITFORPENDING == 1
    BOOL            bRetry;
#endif

    if( MB_IS_VALID_HDL( pxTCPIntHdl, xMBTCPHdl ) && ( pxTCPIntHdl->eType == TCP_CLIENT ) )
    {
        memset( &hints, 0, sizeof( hints ) );
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family = 0;
        hints.ai_flags = 0;
        hints.ai_protocol = IPPROTO_TCP;
        ( void )snprintf( arcServiceName, sizeof( arcServiceName ), "%hu", usTCPPort );

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[CL %d] connecting to (%s:%hu).\n", pxTCPIntHdl->ubIdx, pcConnectAddress, usTCPPort );
        }
#endif
        if( 0 != getaddrinfo( pcConnectAddress, arcServiceName, &hints, &ai0 ) )
        {
            eStatus = MB_EIO;
        }
        else
        {
#if USE_CONNECTWAITFORPENDING == 1
            do
            {
                bRetry = FALSE;
                MBP_ENTER_CRITICAL_SECTION(  );
                pxPollFd = &( pxTCPIntHdl->xShared.xClientSock.xClientSocket );
                if( ( INVALID_SOCKET != pxPollFd->fd ) )
                {
                    MBP_EXIT_CRITICAL_SECTION(  );
                    bRetry = TRUE;
                    /* Wait for cleanup thread to release handle so that
                     * we do not have to return the caller an error.
                     */
                    sleep( MBP_WAIT_TIMEOUTS );
                }
                else
                {
                    MBP_EXIT_CRITICAL_SECTION(  );
                }
            }
            while( bRetry );
#endif
            MBP_ENTER_CRITICAL_SECTION(  );
            MBP_ASSERT( NULL != ai0 );
            if( ( iSockAddr = socket( ai0->ai_family, ai0->ai_socktype, ai0->ai_protocol ) ) < 0 )
            {
                eStatus = MB_EIO;
            }
            else if( connect( iSockAddr, ai0->ai_addr, ai0->ai_addrlen ) < 0 )
            {
                ( void )close( iSockAddr );
                eStatus = MB_EIO;
            }
            else if( -1 == fcntl( iSockAddr, F_SETFL, O_NONBLOCK ) )
            {
                ( void )close( iSockAddr );
                eStatus = MB_EIO;
            }
            else
            {
                pxPollFd = &( pxTCPIntHdl->xShared.xClientSock.xClientSocket );
                /* Make sure our client instance is not in use. We can not check
                 * the bUse flag since we must have checked if our client handler
                 * thread has already shutdown everything correctly.
                 */
                if( INVALID_SOCKET != pxPollFd->fd )
                {
                    eStatus = MB_ENORES;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[CL %d] porting layer has no free resources\n", pxTCPIntHdl->ubIdx );
                    }
#endif
                }
                else
                {
                    pxPollFd->fd = iSockAddr;
                    *pxTCPClientHdl = pxPollFd;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[CL %d] new client connection (%s:%hu): %d.\n",
                                     pxTCPIntHdl->ubIdx, pcConnectAddress, usTCPPort, pxPollFd->fd );
                    }
#endif
                    eStatus = MB_ENOERR;
                }
            }
            MBP_EXIT_CRITICAL_SECTION(  );
        }
        freeaddrinfo( ai0 );
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
    xMBPTCPIntHandle *pxTCPIntHdl = xTCPHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTCPIntHdl, xMBTCPHdl ) && ( pxTCPIntHdl->eType == TCP_CLIENT ) )
    {
        pxTCPIntHdl->bIsRunning = FALSE;

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[CL %d] closing instance.\n", pxTCPIntHdl->ubIdx );
        }
#endif

        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

eMBErrorCode
eMBPTCPConClose( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    struct pollfd  *pxPollFd = xTCPClientHdl;
    xMBPTCPIntHandle *pxTCPIntHdl = xTCPHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTCPIntHdl, xMBTCPHdl ) )
    {
        if( NULL != pxPollFd )
        {
            eStatus = MB_ENOERR;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[IDX %d]: marked client connection as closed (socket=%d).\n", pxTCPIntHdl->ubIdx, pxPollFd->fd );
            }
#endif
            vMBTCPClientHandleReset( pxPollFd, TRUE );
        }
        else
        {
            eStatus = MB_EPORTERR;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

STATIC void    *
vMBTCPClientHandlerThread( void *pxMBPTCPServerHdlArg )
{
    int             iRes;
    BOOL            bIsRunning = FALSE;
    xMBPTCPIntHandle *pxTCPIntHdl = pxMBPTCPServerHdlArg;
    xMBPTCPClientSock *pxTCPClientIntHdl;

    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        bIsRunning = pxTCPIntHdl->bIsRunning;
        pxTCPClientIntHdl = &( pxTCPIntHdl->xShared.xClientSock );
        if( INVALID_SOCKET != pxTCPClientIntHdl->xClientSocket.fd )
        {
            MBP_EXIT_CRITICAL_SECTION(  );
            iRes = poll( &( pxTCPIntHdl->xShared.xClientSock.xClientSocket ), 1, MBP_POLL_TIMEOUT_MS );
            if( iRes != 0 )
            {
                MBP_ENTER_CRITICAL_SECTION(  );
                if( iRes < 0 )
                {
                    if( ( EAGAIN != errno ) && ( EINTR != errno ) )
                    {
                        pxTCPIntHdl->bHasFailed = TRUE;
                    }
                }
                else if( iRes > 0 )
                {
                    if( 0 != ( pxTCPIntHdl->xShared.xClientSock.xClientSocket.revents & POLLHUP ) )
                    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                        if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_PORT_TCP ) )
                        {
                            vMBPPortLog( MB_LOG_INFO, MB_LOG_PORT_TCP, "[CL %d]: client (fd=%d) disconnected.\n", pxTCPIntHdl->ubIdx,
                                         pxTCPIntHdl->xShared.xClientSock.xClientSocket.fd );
                        }
#endif
                        if( NULL != pxTCPIntHdl->xShared.xClientSock.eMBPTCPClientDisconnectedFN )
                        {
                            pxTCPIntHdl->xShared.xClientSock.eMBPTCPClientDisconnectedFN( pxTCPIntHdl->xMBHdl,
                                                                                          &( pxTCPIntHdl->xShared.xClientSock.xClientSocket ) );
                        }
                    }
                    else if( 0 != ( pxTCPIntHdl->xShared.xClientSock.xClientSocket.revents & POLLIN ) )
                    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                        {
                            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[CL %d]: new data for client (fd=%d).\n", pxTCPIntHdl->ubIdx,
                                         pxTCPIntHdl->xShared.xClientSock.xClientSocket.fd );
                        }
#endif
                        if( NULL != pxTCPIntHdl->xShared.xClientSock.eMBPTCPClientNewDataFN )
                        {
                            pxTCPIntHdl->xShared.xClientSock.eMBPTCPClientNewDataFN( pxTCPIntHdl->xMBHdl, &( pxTCPIntHdl->xShared.xClientSock.xClientSocket ) );
                        }
                    }
                }
                MBP_EXIT_CRITICAL_SECTION(  );
            }
        }
        else
        {
            MBP_EXIT_CRITICAL_SECTION(  );
            usleep( 50000 );
        }
    }
    while( bIsRunning );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_INFO, MB_LOG_PORT_TCP, "[CL %d]: finishing handler thread.\n", pxTCPIntHdl->ubIdx );
    }
#endif

    MBP_ENTER_CRITICAL_SECTION(  );
    vMBTCPHandleReset( pxTCPIntHdl, TRUE );
    MBP_EXIT_CRITICAL_SECTION(  );

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_INFO, MB_LOG_PORT_TCP, "[CL ?]: finished handler thread.\n" );
    }
#endif

    pthread_exit( NULL );
    return NULL;
}

eMBErrorCode
eMBPTCPServerInit( xMBPTCPHandle * pxTCPHdl, CHAR * pcBindAddress,
                   USHORT usTCPPort,
                   xMBHandle xMBHdlArg,
                   peMBPTCPClientNewDataCB eMBPTCPClientNewDataFNArg,
                   peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFNArg, peMBPTCPClientConnectedCB eMBPTCPClientConnectedFNArg )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    char            arcServName[6];
    struct addrinfo *ai, *ai0 = NULL, hints;
    xMBPTCPIntHandle *pxTCPIntHdl = NULL;
    UBYTE           ubIdx;
    int             nSockCnt;
    int             iSockAddr;
    int             iLastErr;

    vMBPTCPInit(  );
    if( NULL != pxTCPHdl )

    {
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xMBTCPHdl[ubIdx].ubIdx )
            {
                pxTCPIntHdl = &xMBTCPHdl[ubIdx];
                /* Reset the data structure for a client. It could have been used
                 * by a server before.
                 */
                pxTCPIntHdl->eType = TCP_SERVER;
                vMBTCPHandleReset( pxTCPIntHdl, FALSE );
                pxTCPIntHdl->ubIdx = ubIdx;
                break;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
        if( NULL != pxTCPIntHdl )

        {
            ( void )snprintf( arcServName, sizeof( arcServName ), "%hu", usTCPPort );
            memset( &hints, 0, sizeof( hints ) );
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_family = AF_INET;
            hints.ai_protocol = IPPROTO_TCP;
            if( 0 != ( iLastErr = getaddrinfo( pcBindAddress, arcServName, &hints, &ai0 ) ) )
            {
                eStatus = MB_EPORTERR;
            }
            else
            {
                nSockCnt = 0;
                for( ai = ai0; ( NULL != ai ) && ( nSockCnt < MAX_LISTENING_SOCKS ); )
                {
                    if( ( iSockAddr = socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol ) ) < 0 )
                    {
                    }
                    else if( bind( iSockAddr, ai->ai_addr, ( int )ai->ai_addrlen ) < 0 )
                    {
                        ( void )close( iSockAddr );
                    }
                    else if( listen( iSockAddr, SOMAXCONN ) < 0 )
                    {
                        ( void )close( iSockAddr );
                    }
                    else
                    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                        {
                            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[SRV %d] created listening socket.\n", iSockAddr );
                        }
#endif
                        pxTCPIntHdl->xShared.xServerSock.xSrvSocket[nSockCnt].fd = iSockAddr;
                        nSockCnt++;
                    }
                    ai = ai->ai_next;
                }
                if( nSockCnt > 0 )
                {
                    pxTCPIntHdl->xShared.xServerSock.eMBPTCPClientNewDataFN = eMBPTCPClientNewDataFNArg;
                    pxTCPIntHdl->xShared.xServerSock.eMBPTCPClientDisconnectedFN = eMBPTCPClientDisconnectedFNArg;
                    pxTCPIntHdl->xShared.xServerSock.eMBPTCPClientConnectedFN = eMBPTCPClientConnectedFNArg;
                    pxTCPIntHdl->xMBHdl = xMBHdlArg;
                    if( MB_ENOERR == eMBPTCPInitCommon( pxTCPIntHdl, vMBTCPServerHandlerThread ) )
                    {
                        *pxTCPHdl = pxTCPIntHdl;
                        eStatus = MB_ENOERR;
                    }
                    else
                    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                        {
                            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[SRV %d] resetting handle, closing socket: %s\n", iSockAddr, strerror( errno ) );
                        }
#endif
                        vMBTCPHandleReset( pxTCPIntHdl, TRUE );
                        eStatus = MB_EPORTERR;
                    }
                }
                else
                {
                    vMBTCPHandleReset( pxTCPIntHdl, TRUE );
                    eStatus = MB_EPORTERR;
                }
                freeaddrinfo( ai0 );
            }
        }
        else
        {
            eStatus = MB_ENORES;
        }
    }
    return eStatus;
}

eMBErrorCode
eMBTCPServerClose( xMBPTCPHandle xTCPHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntHandle *pxTCPIntHdl = xTCPHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTCPIntHdl, xMBTCPHdl ) && ( pxTCPIntHdl->eType == TCP_SERVER ) )
    {
        pxTCPIntHdl->bIsRunning = FALSE;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[IDX %d] closing instance.\n", pxTCPIntHdl->ubIdx );
        }
#endif
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

void
vprvMBTCPServerAcceptClient( xMBPTCPIntHandle * pxTCPIntHdl, int xListeningSocket )
{
    BOOL            bDropClient;
    int             xNewClientSocket;
    UBYTE           ubClientSockIdx;
    struct pollfd  *pxTCPClientIntHdl;
    xMBPTCPServerSock *pxTCPServerIntHdl;
    struct sockaddr xRemoteAddr;
    unsigned int    iRemoteAddrLen;

    if( INVALID_SOCKET == ( xNewClientSocket = accept( xListeningSocket, &xRemoteAddr, &iRemoteAddrLen ) ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[SRV %d] could not accept client: %s!\n", pxTCPIntHdl->ubIdx, strerror( errno ) );
        }
#endif
    }
    else if( -1 == fcntl( xNewClientSocket, F_SETFL, O_NONBLOCK ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[SRV %d] dropped client connection because of lack of resources!", pxTCPIntHdl->ubIdx );
        }
#endif
        ( void )close( xNewClientSocket );
        bDropClient = TRUE;
    }
    else
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        pxTCPServerIntHdl = &( pxTCPIntHdl->xShared.xServerSock );
        for( ubClientSockIdx = 0; ubClientSockIdx < MAX_CLIENT_SOCKS; ubClientSockIdx++ )
        {
            pxTCPClientIntHdl = &( pxTCPServerIntHdl->xClientSocket[ubClientSockIdx] );

            /* Check if this client handle is available. */
            if( INVALID_SOCKET == pxTCPClientIntHdl->fd )
            {
                pxTCPClientIntHdl->fd = xNewClientSocket;
                vMBPTCPSocketEnableKeepalive( pxTCPClientIntHdl->fd, TCP_KEEPALIVE, TCP_KEEPALIVE_TIME, TCP_KEEPALIVE_INTVL, TCP_KEEPALIVE_PROBES );
                MBP_ASSERT( NULL != pxTCPServerIntHdl->eMBPTCPClientConnectedFN );
                bDropClient = TRUE;
                if( pxTCPIntHdl->bIsRunning && ( MB_ENOERR == pxTCPServerIntHdl->eMBPTCPClientConnectedFN( pxTCPIntHdl->xMBHdl, pxTCPClientIntHdl ) ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[SRV %d] accepted new client %d (socket=%d).\n",
                                     pxTCPIntHdl->ubIdx, ubClientSockIdx, pxTCPClientIntHdl->fd );
                    }
#endif

                    bDropClient = FALSE;
                }
                if( bDropClient )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[SRV %d] instance closed or lacking resources. droping client!\n", pxTCPIntHdl->ubIdx );
                    }
#endif
                    vMBTCPClientHandleReset( pxTCPClientIntHdl, TRUE );
                }
                break;
            }
        }
        if( MAX_CLIENT_SOCKS == ubClientSockIdx )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[SRV %d] porting layer lacks resources. droping client!\n", pxTCPIntHdl->ubIdx );
            }
#endif
            ( void )close( xNewClientSocket );
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
}

STATIC void    *
vMBTCPServerHandlerThread( void *pxMBPTCPServerHdlArg )
{
    int             x;
    BOOL            bIsRunning;
    int             iWaitResult;
    BOOL            bDropClient;
    xMBPTCPIntHandle *pxTCPIntHdl = pxMBPTCPServerHdlArg;
    xMBPTCPServerSock *pxTCPServerIntHdl;

    signal( SIGPIPE, SIG_IGN );
    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        pxTCPServerIntHdl = &( pxTCPIntHdl->xShared.xServerSock );
        bIsRunning = pxTCPIntHdl->bIsRunning;
        MBP_EXIT_CRITICAL_SECTION(  );

        /* We can now poll all event objects for any network events. */
        iWaitResult = poll( pxTCPServerIntHdl->xClientSocket, MAX_CLIENT_SOCKS, MBP_POLL_TIMEOUT_MS );
        if( iWaitResult < 0 )
        {
            if( ( EINTR == errno ) || ( EAGAIN == errno ) )
            {
            }
            else
            {
                pxTCPIntHdl->bIsRunning = FALSE;
            }
        }
        else if( iWaitResult > 0 )
        {
            /* Handle new data from TCP clients. */
            MBP_ENTER_CRITICAL_SECTION(  );
            for( x = 0; x < MAX_CLIENT_SOCKS; x++ )
            {
                bDropClient = FALSE;
                if( 0 != ( pxTCPServerIntHdl->xClientSocket[x].revents & POLLIN ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[SRV %d] new data for client %d (socket=%d).\n",
                                     pxTCPIntHdl->ubIdx, x, pxTCPServerIntHdl->xClientSocket[x].fd );
                    }
#endif
                    MBP_ASSERT( NULL != pxTCPServerIntHdl->eMBPTCPClientNewDataFN );
                    ( void )pxTCPServerIntHdl->eMBPTCPClientNewDataFN( pxTCPIntHdl->xMBHdl, &( pxTCPServerIntHdl->xClientSocket[x] ) );
                }
                else if( 0 != ( pxTCPServerIntHdl->xClientSocket[x].revents ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[SRV %d] unexpected poll events for client %d (events=%d).\n",
                                     pxTCPIntHdl->ubIdx, x, pxTCPServerIntHdl->xClientSocket[x].revents );
                    }
#endif
                    bDropClient = TRUE;
                }

                /* In case of an error or an FD_CLOSE event drop this
                 * client connection.
                 */
                if( bDropClient )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_PORT_TCP ) )
                    {
                        /* Drop the client. */
                        vMBPPortLog( MB_LOG_INFO, MB_LOG_PORT_TCP, "[SRV %d] dropping client %d (socket=%d).\n",
                                     pxTCPIntHdl->ubIdx, x, pxTCPServerIntHdl->xClientSocket[x].fd );
                    }

#endif
                    MBP_ASSERT( NULL != pxTCPServerIntHdl->eMBPTCPClientDisconnectedFN );
                    if( pxTCPIntHdl->bIsRunning && ( pxTCPServerIntHdl->xClientSocket[x].fd != INVALID_SOCKET ) )
                    {
                        MBP_EXIT_CRITICAL_SECTION(  );
                        ( void )pxTCPServerIntHdl->eMBPTCPClientDisconnectedFN( pxTCPIntHdl->xMBHdl, &( pxTCPServerIntHdl->xClientSocket[x] ) );
                        MBP_ENTER_CRITICAL_SECTION(  );
                        close( pxTCPServerIntHdl->xClientSocket[x].fd );
                    }
                }
            }
            MBP_EXIT_CRITICAL_SECTION(  );
        }

        /* We can now poll all event objects for any network events. */
        iWaitResult = poll( pxTCPServerIntHdl->xSrvSocket, MAX_LISTENING_SOCKS, MBP_POLL_TIMEOUT_MS );
        if( iWaitResult < 0 )
        {
            if( ( EINTR != errno ) && ( EAGAIN != errno ) )
            {
                MBP_ENTER_CRITICAL_SECTION(  );
                pxTCPIntHdl->bIsRunning = FALSE;
                MBP_EXIT_CRITICAL_SECTION(  );
            }
        }
        else if( iWaitResult > 0 )
        {
            /* Handle any new clients. Clients are generally accepted
             * and if the MODBUS stacks has still resources left are
             * added as new valid clients. Otherwise the client
             * connections are dropped immediately. Note that we have
             * to accept them because otherwise the listen object
             * would be immediately signaled again.
             */
            MBP_ENTER_CRITICAL_SECTION(  );
            for( x = 0; x < MAX_LISTENING_SOCKS; x++ )
            {
                if( 0 != ( pxTCPServerIntHdl->xSrvSocket[x].revents & POLLIN ) )
                {

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[SRV %d] new client connection waiting!\n", pxTCPIntHdl->ubIdx );
                    }

#endif
                    /* Try to accept this client connection. */
                    MBP_EXIT_CRITICAL_SECTION(  );
                    vprvMBTCPServerAcceptClient( pxTCPIntHdl, pxTCPServerIntHdl->xSrvSocket[x].fd );
                    MBP_ENTER_CRITICAL_SECTION(  );
                }

                /* Since we have only requested error events we should always
                 * fail on any other error.
                 */
                else if( 0 != pxTCPServerIntHdl->xSrvSocket[x].revents )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[SRV %d] unexpected poll events %d.\n", pxTCPIntHdl->ubIdx,
                                     pxTCPServerIntHdl->xSrvSocket[x].revents );
                    }
#endif
                    bDropClient = TRUE;
                    pxTCPIntHdl->bIsRunning = FALSE;
                }
            }
            MBP_EXIT_CRITICAL_SECTION(  );
        }
    }
    while( bIsRunning );

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_INFO, MB_LOG_PORT_TCP, "[SRV %d] finishing handler thread.\n", pxTCPIntHdl->ubIdx );
    }
#endif
    MBP_ENTER_CRITICAL_SECTION(  );
    vMBTCPHandleReset( pxTCPIntHdl, TRUE );
    MBP_EXIT_CRITICAL_SECTION(  );

    return NULL;
}

STATIC void
vMBPTCPInit(  )
{
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        memset( &xMBTCPHdl[0], 0, sizeof( xMBTCPHdl ) );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPHdl ); ubIdx++ )
        {
            xMBTCPHdl[ubIdx].ubIdx = IDX_INVALID;
        }
        bIsInitalized = TRUE;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}

STATIC          eMBErrorCode
eMBPTCPInitCommon( xMBPTCPIntHandle * pxTCPIntHdl, void *( *pvHandlerThread ) ( void * ) )
{
    eMBErrorCode    eStatus = MB_EPORTERR;
    pthread_attr_t  xAttr;

    MBP_ASSERT( NULL != pxTCPIntHdl );

    pxTCPIntHdl->bIsRunning = TRUE;
    pxTCPIntHdl->bHasFailed = FALSE;

    if( 0 != pthread_attr_init( &xAttr ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "Can't set thread attributes: %s\n", strerror( errno ) );
        }
#endif
    }
    else if( 0 != pthread_attr_setdetachstate( &xAttr, PTHREAD_CREATE_DETACHED ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "Can't set thread attributes: %s\n", strerror( errno ) );
        }
#endif
    }
    else if( 0 != pthread_create( &( pxTCPIntHdl->pxHandlingThread ), &xAttr, pvHandlerThread, pxTCPIntHdl ) )

    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "Can't create thread: %s\n", strerror( errno ) );
        }
#endif
    }
    else
    {
        eStatus = MB_ENOERR;
    }
    ( void )pthread_attr_destroy( &xAttr );

    return eStatus;
}

void
vMBTCPClientHandleReset( struct pollfd *pxPollFd, BOOL bClose )
{
    if( bClose && ( -1 != pxPollFd->fd ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "closing socket (socket=%d)\n", pxPollFd->fd );
        }
#endif
        ( void )close( pxPollFd->fd );
    }
    pxPollFd->fd = -1;
    pxPollFd->events = POLLIN | POLLHUP;
    pxPollFd->revents = 0;
}

STATIC void
vMBTCPHandleReset( xMBPTCPIntHandle * pxTCPIntHdl, BOOL bClose )
{
    int             i;

    MBP_ASSERT( NULL != pxTCPIntHdl );
    pxTCPIntHdl->ubIdx = IDX_INVALID;
    pxTCPIntHdl->bHasFailed = TRUE;
    pxTCPIntHdl->bIsRunning = FALSE;
    pxTCPIntHdl->xMBHdl = MB_HDL_INVALID;
    memset( &( pxTCPIntHdl->pxHandlingThread ), 0, sizeof( pthread_t ) );

    switch ( pxTCPIntHdl->eType )
    {
    case TCP_SERVER:
        for( i = 0; i < MAX_LISTENING_SOCKS; i++ )
        {
            if( bClose && ( -1 != pxTCPIntHdl->xShared.xServerSock.xSrvSocket[i].fd ) )
            {
                ( void )close( pxTCPIntHdl->xShared.xServerSock.xSrvSocket[i].fd );
            }
            pxTCPIntHdl->xShared.xServerSock.xSrvSocket[i].fd = -1;
            pxTCPIntHdl->xShared.xServerSock.xSrvSocket[i].events = POLLIN;
        }
        for( i = 0; i < MAX_CLIENT_SOCKS; i++ )
        {
            vMBTCPClientHandleReset( &( pxTCPIntHdl->xShared.xServerSock.xClientSocket[i] ), bClose );
        }
        break;
    case TCP_CLIENT:
        if( bClose && ( -1 != pxTCPIntHdl->xShared.xClientSock.xClientSocket.fd ) )
        {
            ( void )close( pxTCPIntHdl->xShared.xClientSock.xClientSocket.fd );
        }
        vMBTCPClientHandleReset( &( pxTCPIntHdl->xShared.xClientSock.xClientSocket ), bClose );
        pxTCPIntHdl->xShared.xClientSock.eMBPTCPClientNewDataFN = NULL;
        pxTCPIntHdl->xShared.xClientSock.eMBPTCPClientDisconnectedFN = NULL;

        break;
    default:
        memset( &( pxTCPIntHdl->xShared ), 0, sizeof( pxTCPIntHdl->xShared ) );
        break;
    }
}

void
vMBPTCPDllInit( void )
{
}

void
vMBPTCPDLLClose( void )
{
}

eMBErrorCode
eMBPTCPConRead( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, UBYTE * pubBuffer, USHORT * pusBufferLen, USHORT usBufferMax )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    struct pollfd  *pxPollFd = xTCPClientHdl;
    xMBPTCPIntHandle *pxTCPIntHdl = xTCPHdl;
    ssize_t         iBytesRead;

    ( void )pxTCPIntHdl;
    if( NULL != xTCPClientHdl )
    {
        MBP_ASSERT( INVALID_SOCKET != pxPollFd->fd );
        iBytesRead = read( pxPollFd->fd, pubBuffer, ( size_t ) usBufferMax );
        if( iBytesRead > 0 )
        {
            *pusBufferLen = ( USHORT ) iBytesRead;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[IDX %d]: read %d bytes (fd=%d).\n", pxTCPIntHdl->ubIdx, iBytesRead, pxPollFd->fd );
            }
#endif
            eStatus = MB_ENOERR;
        }
        else
        {
            *pusBufferLen = 0;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[IDX %d]: read returned %d bytes (fd=%d): %s\n", pxTCPIntHdl->ubIdx,
                             iBytesRead, pxPollFd->fd, strerror( errno ) );
            }
#endif
            if( ( errno != EWOULDBLOCK ) && ( errno != EAGAIN ) && ( errno != EINTR ) )
            {
                eStatus = MB_EIO;
            }
            else
            {
                eStatus = MB_ENOERR;
            }
        }
    }
    else
    {
        MBP_ASSERT( 0 );
    }
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[IDX %d]: read status = %d\n", pxTCPIntHdl->ubIdx, eStatus );
    }
#endif

    return eStatus;
}

eMBErrorCode
eMBPTCPConWrite( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, const UBYTE * pubBuffer, USHORT usBufferLen )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    struct pollfd  *pxPollFd = xTCPClientHdl;
    xMBPTCPIntHandle *pxTCPIntHdl = xTCPHdl;
    ssize_t         iBytesWritten;
    size_t          iBytesLeft;

    ( void )pxTCPIntHdl;
    if( NULL != pxPollFd )
    {
        iBytesLeft = ( size_t ) usBufferLen;
        eStatus = MB_ENOERR;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[IDX %d]: trying to write %d bytes (fd=%d)\n", pxTCPIntHdl->ubIdx, ( int )usBufferLen, pxPollFd->fd );
        }
#endif
        while( iBytesLeft > 0 )
        {
            iBytesWritten = write( pxPollFd->fd, pubBuffer, ( size_t ) usBufferLen );
            if( iBytesWritten < 0 )
            {
                eStatus = MB_EIO;
                break;
            }
            else if( 0 == iBytesWritten )
            {
                if( ( EAGAIN != errno ) && ( EINTR != errno ) )
                {
                    eStatus = MB_EIO;
                    break;
                }
            }
            else
            {
                iBytesLeft -= iBytesWritten;
            }
        }
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[IDX %d]: wrote %d bytes of %d bytes (fd=%d). Status=%d\n",
                         pxTCPIntHdl->ubIdx, ( int )( ( ssize_t ) usBufferLen - iBytesLeft ), ( int )usBufferLen, pxPollFd->fd, eStatus );
        }
#endif
    }
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[IDX %d]: write status = %d\n", pxTCPIntHdl->ubIdx, eStatus );
    }
#endif
    return eStatus;
}

STATIC void
vMBPTCPSocketEnableKeepalive( int socket, int iKeepAlive, int iKeepAliveTime, int iIntvl, int iProbes )
{
    int             optval;
    socklen_t       optlen = sizeof( optval );

    /* enable keep-alive */
    optval = iKeepAlive;
    optlen = sizeof( optval );
    if( setsockopt( socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen ) < 0 )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "Failed enabling tcp keep-alive\n" );
        }
#endif
    }
    /* set keep-alive time */
    optval = iKeepAliveTime;
    optlen = sizeof( optval );
    if( setsockopt( socket, IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen ) < 0 )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "Failed setting tcp keep-alive time\n" );
        }
#endif
    }
    /* set keep-alive interval */
    optval = iIntvl;
    optlen = sizeof( optval );
    if( setsockopt( socket, IPPROTO_TCP, TCP_KEEPINTVL, &optval, optlen ) < 0 )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "Failed setting tcp keep-alive interval\n" );
        }
#endif
    }
    /* set keep-alive probes */
    optval = iProbes;
    optlen = sizeof( optval );
    if( setsockopt( socket, IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen ) < 0 )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "Failed setting tcp keep-alive probes\n" );
        }
#endif
    }

}
