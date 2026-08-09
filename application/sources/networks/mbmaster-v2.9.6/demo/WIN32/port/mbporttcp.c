/* 
 * MODBUS Library: TCP port
 * Copyright (c) 2008-2009 Christian Walter <cwalter@embedded-solutions.at>
 * Copyright (c) 2008 Kurt Schroeder <kurt.schroeder@iines.at>
 * All rights reserved.
 *
 * $Id: mbporttcp.c,v 1.7 2013-01-16 21:04:35 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <winsock2.h>
#include <ws2tcpip.h>

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
#define MBP_TCP_DEBUG                       ( 1 )

#define MBP_WAIT_TIMEOUTS                   ( 50 )

/* ----------------------- Type definitions ---------------------------------*/

typedef struct
{
    SOCKET          fd;
    WSAEVENT        xWSAEvent;
    BOOL            bInUse;
} fd_t;

typedef struct
{
    fd_t            xClientSocket;
    peMBPTCPClientNewDataCB eMBPTCPClientNewDataFN;
    peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN;
} xMBPTCPClientSock;

typedef struct
{
    fd_t            xSrvSocket[MAX_LISTENING_SOCKS];
    fd_t            xClientSocket[MAX_CLIENT_SOCKS];
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
    HANDLE          pxHandlingThread;
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
STATIC void     vMBTCPClientHandleReset( fd_t * pxPollFd, BOOL bClose );
STATIC void    *vMBTCPServerHandlerThread( void *xMBPTCPIntHandle );

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPTCPClientInit( xMBPTCPHandle * pxTCPHdl, xMBHandle xMBHdlArg,
                   peMBPTCPClientNewDataCB eMBPTCPClientNewDATAFNArg,
                   peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFNArg )
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
                /* Cleanup old unused thread data. */
                if( NULL != pxTCPIntHdl->pxHandlingThread )
                {
                    ( void )CloseHandle( pxTCPIntHdl->pxHandlingThread );
                    pxTCPIntHdl->pxHandlingThread = NULL;
                }
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
#if defined( MBP_LEAK_TEST ) && ( MBP_LEAK_TEST == 1 )
            if( ( double )rand(  ) / ( double )RAND_MAX < MBP_LEAK_RATE )
            {
                eStatus = MB_ENORES;
            }
#else
            if( 0 )
            {
            }
#endif
            else if( MB_ENOERR != eMBPTCPInitCommon( pxTCPIntHdl, vMBTCPClientHandlerThread ) )
            {
                eStatus = MB_ENORES;
            }
            else
            {
                *pxTCPHdl = pxTCPIntHdl;
                eStatus = MB_ENOERR;
            }

            if( MB_ENOERR != eStatus )
            {
                vMBTCPHandleReset( pxTCPIntHdl, FALSE );
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
eMBPTCPClientOpen( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle * pxTCPClientHdl, const CHAR * pcConnectAddress,
                   USHORT usTCPPort )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntHandle *pxTCPIntHdl = xTCPHdl;
    fd_t           *pxPollFd;
    struct addrinfo *ai0 = NULL, hints;
    char            arcServiceName[6];
    SOCKET          iSockAddr = INVALID_SOCKET;
    WSAEVENT        xWASEvent = NULL;
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
        ( void )_snprintf_s( arcServiceName, sizeof( arcServiceName ), _TRUNCATE, "%hu", usTCPPort );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[CL=%hu] connecting to (%S:%hu).\n",
                         ( USHORT ) pxTCPIntHdl->ubIdx, pcConnectAddress, usTCPPort );
        }
#endif
        if( 0 != getaddrinfo( pcConnectAddress, arcServiceName, &hints, &ai0 ) )
        {
            eStatus = MB_EIO;
        }
        else
        {
            MBP_ASSERT( NULL != ai0 );
#if defined( MBP_LEAK_TEST ) && ( MBP_LEAK_TEST == 1 )
            if( ( double )rand(  ) / ( double )RAND_MAX < MBP_LEAK_RATE )
            {
                eStatus = MB_EIO;
            }
#else
            if( 0 )
            {
            }
#endif
            else if( ( iSockAddr = socket( ai0->ai_family, ai0->ai_socktype, ai0->ai_protocol ) ) < 0 )
            {
                eStatus = MB_EIO;
            }
            else if( connect( iSockAddr, ai0->ai_addr, ( int )ai0->ai_addrlen ) < 0 )
            {
                eStatus = MB_EIO;
            }
            else if( WSA_INVALID_EVENT == ( xWASEvent = WSACreateEvent(  ) ) )
            {
                eStatus = MB_EIO;
            }
            else if( SOCKET_ERROR == WSAEventSelect( iSockAddr, xWASEvent, FD_READ | FD_CLOSE ) )
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
                    if( ( INVALID_SOCKET != pxPollFd->fd ) && !pxPollFd->bInUse )
                    {
                        MBP_EXIT_CRITICAL_SECTION(  );
                        bRetry = TRUE;
                        /* Wait for cleanup thread to release handle so that
                         * we do not have to return the caller an error.
                         */
                        Sleep( MBP_WAIT_TIMEOUTS );
                    }
                    else
                    {
                        MBP_EXIT_CRITICAL_SECTION(  );
                    }
                }
                while( bRetry );
#endif
                MBP_ENTER_CRITICAL_SECTION(  );
                pxPollFd = &( pxTCPIntHdl->xShared.xClientSock.xClientSocket );

                /* Make sure our client instance is not in use. We can not check
                 * the bUse flag since we must have checked if our client handler
                 * thread has already shutdown everything correctly.
                 */
#if defined( MBP_LEAK_TEST ) && ( MBP_LEAK_TEST == 1 )
                if( ( double )rand(  ) / ( double )RAND_MAX < MBP_LEAK_RATE )
                {
                    eStatus = MB_EIO;
                }
#else
                if( 0 )
                {
                }
#endif
                else if( INVALID_SOCKET != pxPollFd->fd )
                {
                    eStatus = MB_ENORES;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[CL=%hu] porting layer has no free resources\n",
                                     ( USHORT ) pxTCPIntHdl->ubIdx );
                    }
#endif
                }
                else
                {
                    pxPollFd->fd = iSockAddr;
                    pxPollFd->xWSAEvent = xWASEvent;
                    pxPollFd->bInUse = TRUE;
                    *pxTCPClientHdl = pxPollFd;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[CL=%hu] new client connection (%S:%hu): %d.\n",
                                     ( USHORT ) pxTCPIntHdl->ubIdx, pcConnectAddress, usTCPPort, pxPollFd->fd );
                    }
#endif
                    eStatus = MB_ENOERR;
                }
                MBP_EXIT_CRITICAL_SECTION(  );
            }

            if( MB_ENOERR != eStatus )
            {
                if( INVALID_SOCKET != iSockAddr )
                {
                    ( void )closesocket( iSockAddr );
                }
                if( NULL != xWASEvent )
                {
                    ( void )WSACloseEvent( xWASEvent );
                }
            }
        }
        freeaddrinfo( ai0 );
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
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}


eMBErrorCode
eMBPTCPConClose( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    fd_t           *pxPollFd = xTCPClientHdl;
    xMBPTCPIntHandle *pxTCPIntHdl = xTCPHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTCPIntHdl, xMBTCPHdl ) )
    {
        if( NULL != pxPollFd )
        {
            pxPollFd->bInUse = FALSE;
            eStatus = MB_ENOERR;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "marked client connection as closed (socket=%d).\n",
                             pxPollFd->fd );
            }
#endif
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
    BOOL            bIsRunning = FALSE;
    xMBPTCPIntHandle *pxTCPIntHdl = pxMBPTCPServerHdlArg;
    xMBPTCPClientSock *pxTCPClientIntHdl;
    BOOL            bRx, bHup, bHasFailed;
    WSAEVENT        xCurWSAEvent;
    WSANETWORKEVENTS xWSANetEvents;
    SOCKET          xCurFD;
    DWORD           dwWaitResult;

    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        bIsRunning = pxTCPIntHdl->bIsRunning;
        pxTCPClientIntHdl = &( pxTCPIntHdl->xShared.xClientSock );
        if( INVALID_SOCKET != pxTCPClientIntHdl->xClientSocket.fd )
        {
            bHasFailed = FALSE;
            bRx = FALSE;
            bHup = FALSE;
            xCurWSAEvent = pxTCPClientIntHdl->xClientSocket.xWSAEvent;
            xCurFD = pxTCPClientIntHdl->xClientSocket.fd;
            MBP_EXIT_CRITICAL_SECTION(  );
            dwWaitResult = WSAWaitForMultipleEvents( 1, &xCurWSAEvent, TRUE, MBP_WAIT_TIMEOUTS, FALSE );
            /* First check if this client instance should be shut down.
             * This shutdown request was set by the MODBUS stack.
             */
            MBP_ENTER_CRITICAL_SECTION(  );
            if( !pxTCPClientIntHdl->xClientSocket.bInUse )
            {
                vMBTCPClientHandleReset( &( pxTCPClientIntHdl->xClientSocket ), TRUE );
            }
            else
            {
                if( WSA_WAIT_EVENT_0 == dwWaitResult )
                {
                    if( INVALID_SOCKET != pxTCPClientIntHdl->xClientSocket.fd )
                    {
                        if( SOCKET_ERROR != WSAEnumNetworkEvents( xCurFD, xCurWSAEvent, &xWSANetEvents ) )
                        {
                            bRx = ( xWSANetEvents.lNetworkEvents & FD_READ ) ? TRUE : FALSE;
                            if( bRx && xWSANetEvents.iErrorCode[FD_READ_BIT] )
                            {
                                bHasFailed = TRUE;
                            }
                            bHup = ( xWSANetEvents.lNetworkEvents & FD_CLOSE ) ? TRUE : FALSE;
                            if( bHup && xWSANetEvents.iErrorCode[FD_CLOSE_BIT] )
                            {
                                bHasFailed = TRUE;
                            }
                        }
                        else
                        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                            {
                                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                                             "[CL=%hu] failed to get network events (socket=%d): %s\n",
                                             ( USHORT ) pxTCPIntHdl->ubIdx, xCurFD,
                                             Error2String( WSAGetLastError(  ) ) );
                            }
#endif
                            bHasFailed = TRUE;
                        }
                        WSAResetEvent( xCurWSAEvent );

                        if( bRx )
                        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
                            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                            {
                                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                                             "[CL=%hu] new data for client (socket=%d).\n",
                                             ( USHORT ) pxTCPIntHdl->ubIdx, pxTCPClientIntHdl->xClientSocket.fd );
                            }
#endif
                            if( pxTCPIntHdl->bIsRunning && pxTCPClientIntHdl->xClientSocket.bInUse &&
                                ( NULL != pxTCPClientIntHdl->eMBPTCPClientNewDataFN ) )
                            {
                                pxTCPClientIntHdl->eMBPTCPClientNewDataFN( pxTCPIntHdl->xMBHdl, pxTCPClientIntHdl );
                            }
                        }
                        if( bHup )
                        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
                            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                            {
                                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                                             "[CL=%hu] client (socket=%d) disconnected.\n",
                                             ( USHORT ) pxTCPIntHdl->ubIdx, pxTCPClientIntHdl->xClientSocket.fd );
                            }
#endif

                            if( pxTCPIntHdl->bIsRunning && pxTCPClientIntHdl->xClientSocket.bInUse &&
                                ( NULL != pxTCPClientIntHdl->eMBPTCPClientDisconnectedFN ) )
                            {
                                pxTCPIntHdl->xShared.xClientSock.eMBPTCPClientDisconnectedFN( pxTCPIntHdl->xMBHdl,
                                                                                              &( pxTCPIntHdl->xShared.
                                                                                                 xClientSock.
                                                                                                 xClientSocket ) );
                            }
                        }
                        if( bHasFailed )
                        {
                            pxTCPIntHdl->bHasFailed = TRUE;
                        }
                    }
                }
                else if( WSA_WAIT_FAILED == dwWaitResult )
                {
                    /* Can happen if connection was closed because our
                     * lock does not spawn the select.
                     */
                    if( WSA_INVALID_HANDLE != WSAGetLastError(  ) )
                    {
                        pxTCPIntHdl->bHasFailed = TRUE;
                    }
                }
            }
            MBP_EXIT_CRITICAL_SECTION(  );
        }
        else
        {
            MBP_EXIT_CRITICAL_SECTION(  );
            Sleep( MBP_WAIT_TIMEOUTS );
        }
    }
    while( bIsRunning );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[CL=%hu] finishing handler thread.\n",
                     ( USHORT ) pxTCPIntHdl->ubIdx );
    }
#endif
    MBP_ENTER_CRITICAL_SECTION(  );
    vMBTCPHandleReset( pxTCPIntHdl, TRUE );
    MBP_EXIT_CRITICAL_SECTION(  );
    ExitThread( 0 );
    return NULL;
}

eMBErrorCode
eMBPTCPServerInit( xMBPTCPHandle * pxTCPHdl, CHAR * pcBindAddress, USHORT usTCPPort,
                   xMBHandle xMBHdlArg,
                   peMBPTCPClientNewDataCB eMBPTCPClientNewDataFNArg,
                   peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFNArg,
                   peMBPTCPClientConnectedCB eMBPTCPClientConnectedFNArg )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    char            arcServName[6];
    struct addrinfo *ai, *ai0 = NULL, hints;
    xMBPTCPIntHandle *pxTCPIntHdl = NULL;
    UBYTE           ubIdx;
    int             nSockCnt;
    SOCKET          iSockAddr;
    int             iLastErr;
    WSAEVENT        xWSAEvent;

    vMBPTCPInit(  );
    if( NULL != pxTCPHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xMBTCPHdl[ubIdx].ubIdx )
            {
                pxTCPIntHdl = &xMBTCPHdl[ubIdx];
                /* Cleanup old unused thread data. */
                if( NULL != pxTCPIntHdl->pxHandlingThread )
                {
                    ( void )CloseHandle( pxTCPIntHdl->pxHandlingThread );
                    pxTCPIntHdl->pxHandlingThread = NULL;
                }
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
            ( void )_snprintf_s( arcServName, sizeof( arcServName ), _TRUNCATE, "%hu", usTCPPort );
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
                        ( void )closesocket( iSockAddr );
                    }
                    else if( listen( iSockAddr, SOMAXCONN ) < 0 )
                    {
                        ( void )closesocket( iSockAddr );
                    }
                    else if( WSA_INVALID_EVENT == ( xWSAEvent = WSACreateEvent(  ) ) )
                    {
                        ( void )closesocket( iSockAddr );
                    }
                    else if( SOCKET_ERROR == WSAEventSelect( iSockAddr, xWSAEvent, FD_ACCEPT ) )
                    {
                        ( void )closesocket( iSockAddr );
                        ( void )WSACloseEvent( xWSAEvent );
                        eStatus = MB_EIO;
                    }
                    else
                    {
                        pxTCPIntHdl->xShared.xServerSock.xSrvSocket[nSockCnt].fd = iSockAddr;
                        pxTCPIntHdl->xShared.xServerSock.xSrvSocket[nSockCnt].xWSAEvent = xWSAEvent;
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
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

void
vprvMBTCPServerAcceptClient( xMBPTCPIntHandle * pxTCPIntHdl, SOCKET xListeningSocket )
{
    BOOL            bDropClient;
    SOCKET          xNewClientSocket;
    WSAEVENT        xWSAEvent;
    UBYTE           ubClientSockIdx;
    fd_t           *pxTCPClientIntHdl;

    xMBPTCPServerSock *pxTCPServerIntHdl;

    if( INVALID_SOCKET == ( xNewClientSocket = accept( xListeningSocket, NULL, NULL ) ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "[SRV=%hu] could not accept client: %s!\n",
                         ( USHORT ) pxTCPIntHdl->ubIdx, Error2String( WSAGetLastError(  ) ) );
        }
#endif
    }
    else if( WSA_INVALID_EVENT == ( xWSAEvent = WSACreateEvent(  ) ) )
    {
        ( void )closesocket( xNewClientSocket );
    }
    else if( SOCKET_ERROR == WSAEventSelect( xNewClientSocket, xWSAEvent, FD_READ | FD_CLOSE ) )
    {
        ( void )closesocket( xNewClientSocket );
        ( void )WSACloseEvent( xWSAEvent );
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
                pxTCPClientIntHdl->xWSAEvent = xWSAEvent;
                pxTCPClientIntHdl->bInUse = TRUE;

                MBP_ASSERT( NULL != pxTCPServerIntHdl->eMBPTCPClientConnectedFN );
                bDropClient = TRUE;
                if( pxTCPIntHdl->bIsRunning
                    && ( MB_ENOERR ==
                         pxTCPServerIntHdl->eMBPTCPClientConnectedFN( pxTCPIntHdl->xMBHdl, pxTCPClientIntHdl ) ) )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[SRV=%hu] accepted new client %d (socket=%d).\n",
                                     ( USHORT ) pxTCPIntHdl->ubIdx, ubClientSockIdx, pxTCPClientIntHdl->fd );
                    }
#endif
                    bDropClient = FALSE;
                }
                if( bDropClient )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                                     "[SRV=%hu] instance closed or lacking resources. droping client!\n",
                                     ( USHORT ) pxTCPIntHdl->ubIdx );
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
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                             "[SRV=%hu] porting layer lacks resources. droping client!\n",
                             ( USHORT ) pxTCPIntHdl->ubIdx );
            }
#endif
            ( void )closesocket( xNewClientSocket );
            ( void )WSACloseEvent( xWSAEvent );
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
}

STATIC void    *
vMBTCPServerHandlerThread( void *pxMBPTCPServerHdlArg )
{
    BOOL            bIsRunning = FALSE;
    xMBPTCPIntHandle *pxTCPIntHdl = pxMBPTCPServerHdlArg;
    xMBPTCPServerSock *pxTCPServerIntHdl;
    WSAEVENT        arxWSAEvents[MAX_LISTENING_SOCKS + MAX_CLIENT_SOCKS];
	UBYTE           arubBuffer[2];
    struct
    {
        enum
        { UNUSED, LISTENING_SOCKET, CLIENT_SOCKET } eType;
        UBYTE           ubIdx;
    } arxWSAEventsParent[MAX_LISTENING_SOCKS + MAX_CLIENT_SOCKS];
    UBYTE           ubIdx,ubIdx2;
    UBYTE           ubNActiveEvObjects;
    DWORD           dwWaitResult;
    WSANETWORKEVENTS xWSANetworkEvents;
    int             iSocketRes;
    BOOL            bDropClient;
	int             iRoundRobinListeningSocks = 0, iRoundRobinClientSocks = 0;
    if( !SetThreadPriority( GetCurrentThread(  ), THREAD_PRIORITY_ABOVE_NORMAL ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TIMER, "Can't increase thread priority: %s\n",
                         Error2String( GetLastError(  ) ) );
        }
#endif
    }

    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        bIsRunning = pxTCPIntHdl->bIsRunning;
        pxTCPServerIntHdl = &( pxTCPIntHdl->xShared.xServerSock );

        /* Now build a list of all event objects on which we can currently 
         * wait.
         */
        for( ubIdx = 0; ubIdx < ( MAX_LISTENING_SOCKS + MAX_CLIENT_SOCKS ); ubIdx++ )
        {
			
            arxWSAEventsParent[ubIdx].eType = UNUSED;
            arxWSAEventsParent[ubIdx].ubIdx = IDX_INVALID;
            arxWSAEvents[ubIdx] = NULL;
        }
        ubNActiveEvObjects = 0;

        for( ubIdx = 0; ubIdx < MAX_LISTENING_SOCKS; ubIdx++ )
        {
			ubIdx2 = ( ubIdx + iRoundRobinListeningSocks ) % MAX_LISTENING_SOCKS;
            /* This listening socket is active. */
            if( NULL != pxTCPServerIntHdl->xSrvSocket[ubIdx2].xWSAEvent )
            {
                arxWSAEventsParent[ubNActiveEvObjects].eType = LISTENING_SOCKET;
                arxWSAEventsParent[ubNActiveEvObjects].ubIdx = ubIdx2;
                arxWSAEvents[ubNActiveEvObjects++] = pxTCPServerIntHdl->xSrvSocket[ubIdx2].xWSAEvent;
            }
        }
		iRoundRobinListeningSocks++;
		if( iRoundRobinListeningSocks >= MAX_LISTENING_SOCKS )
		{
			iRoundRobinListeningSocks = 0;
		}

        for( ubIdx = 0; ubIdx < MAX_CLIENT_SOCKS; ubIdx++ )
        {
			ubIdx2 = ( ubIdx + iRoundRobinClientSocks ) % MAX_CLIENT_SOCKS;
            /* This client socket is active. */
            if( NULL != pxTCPServerIntHdl->xClientSocket[ubIdx2].xWSAEvent )
            {
                arxWSAEventsParent[ubNActiveEvObjects].eType = CLIENT_SOCKET;
                arxWSAEventsParent[ubNActiveEvObjects].ubIdx = ubIdx2;
                arxWSAEvents[ubNActiveEvObjects++] = pxTCPServerIntHdl->xClientSocket[ubIdx2].xWSAEvent;
            }
        }
		iRoundRobinClientSocks++;
		if( iRoundRobinClientSocks >= MAX_CLIENT_SOCKS )
		{
			iRoundRobinClientSocks = 0;
		}

        MBP_EXIT_CRITICAL_SECTION(  );

        /* We can now poll all event objects for any network events. */
        dwWaitResult = WSAWaitForMultipleEvents( ubNActiveEvObjects, arxWSAEvents, FALSE, MBP_WAIT_TIMEOUTS, FALSE );
        if( ( WSA_WAIT_EVENT_0 <= dwWaitResult ) && dwWaitResult < ( WSA_WAIT_EVENT_0 + ubNActiveEvObjects ) )
        {
			
            /* Signaled object is dwWaitResult - WSA_WAIT_EVENT_0 */
            ubIdx = ( UBYTE ) ( dwWaitResult - WSA_WAIT_EVENT_0 );

            MBP_ASSERT( UNUSED != arxWSAEventsParent[ubIdx].eType );
            /* Handle any new clients. Clients are generally accepted
             * and if the MODBUS stacks has still resources left are
             * added as new valid clients. Otherwise the client 
             * connections are dropped immediately. Note that we have
             * to accept them because otherwise the listen object
             * would be immediately signaled again.
             */
            if( LISTENING_SOCKET == arxWSAEventsParent[ubIdx].eType )
            {
                /* We do not need our old index anymore because we now
                 * know the correct index of the listening socket. 
                 */
                ubIdx = arxWSAEventsParent[ubIdx].ubIdx;

                iSocketRes = WSAEnumNetworkEvents( pxTCPServerIntHdl->xSrvSocket[ubIdx].fd,
                                                   pxTCPServerIntHdl->xSrvSocket[ubIdx].xWSAEvent, &xWSANetworkEvents );
                if( SOCKET_ERROR != iSocketRes )
                {
                    /* Only FD_ACCEPT was enabled. */
                    MBP_ASSERT( FD_ACCEPT & xWSANetworkEvents.lNetworkEvents );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
                    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[SRV=%hu] new client connection waiting!\n",
                                     ( USHORT ) pxTCPIntHdl->ubIdx );
                    }
#endif
                    /* Try to accept this client connection. */
                    vprvMBTCPServerAcceptClient( pxTCPIntHdl, pxTCPServerIntHdl->xSrvSocket[ubIdx].fd );
                }
                else
                {
                    /* This server instance has failed. */
                    pxTCPIntHdl->bHasFailed = TRUE;
                }
            }

            /* Handle new data from TCP clients. */
            else if( CLIENT_SOCKET == arxWSAEventsParent[ubIdx].eType )
            {
                /* We do not need our old index anymore because we now
                 * know the correct index of the listening socket. 
                 */
                ubIdx = arxWSAEventsParent[ubIdx].ubIdx;
                iSocketRes = WSAEnumNetworkEvents( pxTCPServerIntHdl->xClientSocket[ubIdx].fd,
                                                   pxTCPServerIntHdl->xClientSocket[ubIdx].xWSAEvent,
                                                   &xWSANetworkEvents );

                bDropClient = TRUE;
                if( SOCKET_ERROR != iSocketRes )
                {
                    if( !xWSANetworkEvents.lNetworkEvents )
                    {
                        bDropClient = FALSE;
                    }
                    if( xWSANetworkEvents.lNetworkEvents & FD_READ )
                    {
                        if( !xWSANetworkEvents.iErrorCode[FD_READ_BIT] )
                        {

                            MBP_ENTER_CRITICAL_SECTION(  );
                            if( pxTCPIntHdl->bIsRunning && pxTCPServerIntHdl->xClientSocket[ubIdx].bInUse )
                            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
                                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                                {
                                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                                                 "[SRV=%hu] new data for client %d (socket=%d).\n",
                                                 ( USHORT ) pxTCPIntHdl->ubIdx, ubIdx,
                                                 pxTCPServerIntHdl->xClientSocket[ubIdx].fd );
                                }
#endif
                                MBP_ASSERT( NULL != pxTCPServerIntHdl->eMBPTCPClientNewDataFN );
                                ( void )pxTCPServerIntHdl->eMBPTCPClientNewDataFN( pxTCPIntHdl->xMBHdl,
                                                                                   &( pxTCPServerIntHdl->xClientSocket
                                                                                      [ubIdx] ) );
                            }
                            MBP_EXIT_CRITICAL_SECTION(  );
							( void )recv( pxTCPServerIntHdl->xClientSocket[ubIdx].fd, arubBuffer, 2, MSG_PEEK );
                            bDropClient = FALSE;
                        }
                        else
                        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
                            {
                                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP,
                                             "[SRV=%hu] read error on client %d (socket=%d).\n",
                                             ( USHORT ) pxTCPIntHdl->ubIdx, ubIdx,
                                             pxTCPServerIntHdl->xClientSocket[ubIdx].fd );
                            }
#endif
                        }
                    }
                    if( xWSANetworkEvents.lNetworkEvents & FD_CLOSE )
                    {
                        /* Drop the client. */
                        bDropClient = TRUE;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
                        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
                        {
                            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                                         "[SRV=%hu] client %d (socket=%d) disconnected.\n",
                                         ( USHORT ) pxTCPIntHdl->ubIdx, ubIdx,
                                         pxTCPServerIntHdl->xClientSocket[ubIdx].fd );
                        }
#endif
                    }

                }
                /* In case of an error or an FD_CLOSE event drop this
                 * client connection.
                 */
                if( bDropClient )
                {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
                    /* Drop the client. */
                    if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_PORT_TCP ) )
                    {
                        vMBPPortLog( MB_LOG_INFO, MB_LOG_PORT_TCP, "[SRV=%hu] dropping client %d (socket=%d).\n",
                                     ( USHORT ) pxTCPIntHdl->ubIdx, ubIdx, pxTCPServerIntHdl->xClientSocket[ubIdx].fd );
                    }
#endif
                    MBP_ENTER_CRITICAL_SECTION(  );
                    MBP_ASSERT( NULL != pxTCPServerIntHdl->eMBPTCPClientDisconnectedFN );
                    if( pxTCPIntHdl->bIsRunning && pxTCPServerIntHdl->xClientSocket[ubIdx].bInUse )
                    {
                        ( void )pxTCPServerIntHdl->eMBPTCPClientDisconnectedFN( pxTCPIntHdl->xMBHdl,
                                                                                &( pxTCPServerIntHdl->xClientSocket
                                                                                   [ubIdx] ) );
                    }
                    MBP_EXIT_CRITICAL_SECTION(  );
                }
            }
        }
        else if( WSA_WAIT_FAILED == dwWaitResult )
        {
            switch ( WSAGetLastError(  ) )
            {
            case WSAEINPROGRESS:
                /* This is okay. */
                break;
            default:
                pxTCPIntHdl->bHasFailed = TRUE;
            }
        }

        /* Cleanup any unused client handles which are still open. */
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < MAX_CLIENT_SOCKS; ubIdx++ )
        {
            /* This client handle has been marked as closed but the 
             * resources are still open.
             */
            if( !pxTCPServerIntHdl->xClientSocket[ubIdx].bInUse
                && ( INVALID_SOCKET != pxTCPServerIntHdl->xClientSocket[ubIdx].fd ) )
            {
                vMBTCPClientHandleReset( &( pxTCPServerIntHdl->xClientSocket[ubIdx] ), TRUE );
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
    while( bIsRunning );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[SRV=%hu] finishing handler thread.\n",
                     ( USHORT ) pxTCPIntHdl->ubIdx );
    }
#endif
    MBP_ENTER_CRITICAL_SECTION(  );
    vMBTCPHandleReset( pxTCPIntHdl, TRUE );
    MBP_EXIT_CRITICAL_SECTION(  );
    ExitThread( 0 );
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

    MBP_ASSERT( NULL != pxTCPIntHdl );

    /* Must be set before thread is spawned to make sure that thread
     * does not immediately quit.
     */
    pxTCPIntHdl->bIsRunning = TRUE;
    pxTCPIntHdl->bHasFailed = FALSE;

    if( NULL ==
        ( pxTCPIntHdl->pxHandlingThread =
          CreateThread( NULL, THREAD_STACKSIZE_MAX, ( LPTHREAD_START_ROUTINE ) pvHandlerThread, pxTCPIntHdl, 0,
                        NULL ) ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_TCP, "can't spawn new handler thread: %s\n",
                         Error2String( GetLastError(  ) ) );
        }
#endif
    }
    else
    {
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

void
vMBTCPClientHandleReset( fd_t * pxPollFd, BOOL bClose )
{
    if( bClose && ( INVALID_SOCKET != pxPollFd->fd ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "closing socket (socket=%d)\n", pxPollFd->fd );
        }
#endif
        ( void )closesocket( pxPollFd->fd );
        ( void )WSACloseEvent( pxPollFd->xWSAEvent );
    }
    pxPollFd->xWSAEvent = WSA_INVALID_EVENT;
    pxPollFd->fd = INVALID_SOCKET;
    pxPollFd->bInUse = FALSE;
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

    switch ( pxTCPIntHdl->eType )
    {
    case TCP_SERVER:
        for( i = 0; i < MAX_LISTENING_SOCKS; i++ )
        {
            if( bClose && ( INVALID_SOCKET != pxTCPIntHdl->xShared.xServerSock.xSrvSocket[i].fd ) )
            {
                ( void )closesocket( pxTCPIntHdl->xShared.xServerSock.xSrvSocket[i].fd );
            }
            pxTCPIntHdl->xShared.xServerSock.xSrvSocket[i].fd = INVALID_SOCKET;
        }
        for( i = 0; i < MAX_CLIENT_SOCKS; i++ )
        {
            vMBTCPClientHandleReset( &( pxTCPIntHdl->xShared.xServerSock.xClientSocket[i] ), bClose );
        }
        pxTCPIntHdl->xShared.xServerSock.eMBPTCPClientNewDataFN = NULL;
        pxTCPIntHdl->xShared.xServerSock.eMBPTCPClientDisconnectedFN = NULL;
        pxTCPIntHdl->xShared.xServerSock.eMBPTCPClientConnectedFN = NULL;
        break;
    case TCP_CLIENT:
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
    BOOL            bThreadsRunning = FALSE;
    UBYTE           ubIdx;
    HANDLE          hThreadHandle;

    /* If this code is called we know that no MODBUS instances are
     * newly created (Because of the INIT lock) and that all
     * still running serial threads are going to shut down since
     * their bIsRunning flag has been set to FALSE.
     */
    do
    {
        hThreadHandle = NULL;
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPHdl ); ubIdx++ )
        {
            if( NULL != xMBTCPHdl[ubIdx].pxHandlingThread )
            {
                hThreadHandle = xMBTCPHdl[ubIdx].pxHandlingThread;
                xMBTCPHdl[ubIdx].pxHandlingThread = NULL;
                break;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
        if( NULL != hThreadHandle )
        {
            if( WAIT_OBJECT_0 != WaitForSingleObject( hThreadHandle, 5000 ) )
            {
                MBP_ASSERT( 0 );
            }
            ( void )CloseHandle( hThreadHandle );
        }
    }
    while( NULL != hThreadHandle );
}

eMBErrorCode
eMBPTCPConRead( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, UBYTE * pubBuffer,
                USHORT * pusBufferLen, USHORT usBufferMax )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    fd_t           *pxPollFd = xTCPClientHdl;
    int             iBytesRead;
    int             iSocketError;

    if( NULL != xTCPClientHdl )
    {
        MBP_ASSERT( INVALID_SOCKET != pxPollFd->fd );
        iBytesRead = recv( pxPollFd->fd, pubBuffer, usBufferMax, 0 );
        if( iBytesRead > 0 )
        {
            *pusBufferLen = ( USHORT ) iBytesRead;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "client %d: read %d bytes.\n", pxPollFd->fd, iBytesRead );
            }
#endif
            eStatus = MB_ENOERR;
        }
        else if( 0 == iBytesRead )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "client %d: read returned 0 bytes.\n", pxPollFd->fd,
                             iBytesRead );
            }
#endif
            eStatus = MB_EIO;
        }
        else
        {
            iSocketError = GetLastError(  );
            switch ( iSocketError )
            {
            case WSAEWOULDBLOCK:
                /* If Serial Devices and Sockets are used at the same
                 * time sometimes the read functions return stupid
                 * error codes. The reason for the error is still
                 * not clear but during testing a retry showed that
                 * it solves the problem.
                 */
            case WSASYSCALLFAILURE:
                eStatus = MB_ENOERR;
                break;
            default:
                eStatus = MB_EIO;
                break;
            }
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "client %d: read failed: %s\n",
                             pxPollFd->fd, Error2String( iSocketError ) );
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
    fd_t           *pxPollFd = xTCPClientHdl;
    int             iBytesWritten;
    size_t          iBytesLeft;

    if( NULL != pxPollFd )
    {
        iBytesLeft = ( size_t ) usBufferLen;
        eStatus = MB_ENOERR;
        while( iBytesLeft > 0 )
        {
            iBytesWritten = send( pxPollFd->fd, pubBuffer, usBufferLen, 0 );
            if( SOCKET_ERROR == iBytesWritten )
            {
                eStatus = MB_EIO;
                break;
            }
            else
            {
                iBytesLeft -= iBytesWritten;
            }
        }
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_TCP_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "client (socket=%d) wrote %d bytes of %d bytes.\n",
                         pxPollFd->fd, ( int )( ( int )usBufferLen - iBytesLeft ), ( int )usBufferLen );
        }
#endif
    }

    return eStatus;
}
