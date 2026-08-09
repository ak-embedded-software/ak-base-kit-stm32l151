/* 
 * MODBUS Library: UDP port
 * Copyright (c) 2011 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved
 *
 * $Id: mbportudp.c,v 1.2 2011-12-05 23:49:08 embedded-solutions.cwalter Exp $
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
#define MAX_HDLS                            ( 4 )
#define IDX_INVALID                         ( 255 )
#define THREAD_STACKSIZE_MAX                ( 16384 )
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
    UBYTE           ubIdx;
    BOOL            bIsRunning;
    BOOL            bHasFailed;
    HANDLE          pxHandlingThread;
    xMBHandle       xMBHdl;
    fd_t            xUDPListeningSocket;
    SOCKADDR_IN     xUDPListeningAddr;
    peMBPUDPClientNewDataCB eMBPUDPClientNewDATAFN;
} xMBPUDPIntHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xMBPUDPIntHandle xMBUDPHdl[MAX_HDLS];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vMBPUDPInit( void );
STATIC void     vMBUDPClientHandleReset( fd_t * pxPollFd, BOOL bClose );
STATIC void     vMBUDPHandleReset( xMBPUDPIntHandle * pxUDPHdl, BOOL bClose );
STATIC eMBErrorCode eMBPUDPInitCommon( xMBPUDPIntHandle * pxUDPIntHdl, void *( *pvHandlerThread ) ( void * ) );
STATIC void    *vMBUDPClientHandlerThread( void *pxUDPIntHdl );

/* ----------------------- Start implementation -----------------------------*/
BOOL
bInitializeSockAddr( SOCKADDR_IN * pxSockAddr, const CHAR * pcAddr, LONG uUDPPort )
{
    BOOL            bOkay = FALSE;
    char            arcServName[6], *serviceHint = NULL;
    struct addrinfo *ai0 = NULL, *ac, hints;
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;

    if( 0 <= uUDPPort && uUDPPort <= 65535 )
    {
        ( void )_snprintf_s( arcServName, sizeof( arcServName ), _TRUNCATE, "%hu", uUDPPort );
        serviceHint = arcServName;
    }

    MBP_ASSERT( NULL != pxSockAddr );
    if( NULL != pxSockAddr )
    {
        if( 0 != getaddrinfo( pcAddr, serviceHint, &hints, &ai0 ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_UDP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "[CL=?] Getaddrinfo failed: %s!\n",
                             Error2String( WSAGetLastError(  ) ) );
            }
#endif
            bOkay = FALSE;
        }
        else
        {
            for( ac = ai0; ac != NULL; ac = ac->ai_next )
            {
                if( ac->ai_family = AF_INET )
                {
                    memcpy( pxSockAddr, ( struct sockaddr_in * )ai0->ai_addr, sizeof( SOCKADDR_IN ) );
                    bOkay = TRUE;
                    break;
                }
            }
            freeaddrinfo( ai0 );
        }
    }
    return bOkay;
}

eMBErrorCode
eMBPUDPClientInit( xMBPUDPHandle * pxUDPHdl, xMBHandle xMBHdl,
                   const CHAR * pcUDPBindAddress, LONG uUDPListenPort, peMBPUDPClientNewDataCB eMBPUDPClientNewDATAFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPUDPIntHandle *pxUDPIntHdl = NULL;
    UBYTE           ubIdx;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    char            arcNameBuffer[16];
    DWORD           dwNameBufferLen;
#endif
    int             iSockLen;

    vMBPUDPInit(  );
    if( NULL != pxUDPHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBUDPHdl ); ubIdx++ )
        {
            if( IDX_INVALID == xMBUDPHdl[ubIdx].ubIdx )
            {
                pxUDPIntHdl = &xMBUDPHdl[ubIdx];
                /* Cleanup old unused thread data. */
                if( NULL != pxUDPIntHdl->pxHandlingThread )
                {
                    ( void )CloseHandle( pxUDPIntHdl->pxHandlingThread );
                    pxUDPIntHdl->pxHandlingThread = NULL;
                }
                /* Reset the data structure for a client. It could have been used
                 * by a server before.
                 */
                vMBUDPHandleReset( pxUDPIntHdl, FALSE );
                pxUDPIntHdl->ubIdx = ubIdx;
                break;
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );

        if( NULL != pxUDPIntHdl )
        {
            iSockLen = sizeof( SOCKADDR_IN );
            if( !bInitializeSockAddr( &pxUDPIntHdl->xUDPListeningAddr, pcUDPBindAddress, uUDPListenPort ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_UDP ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_UDP, "[CL=%hu] Can not get address for %s:%hu!\n",
                                 ( USHORT ) pxUDPIntHdl->ubIdx, pcUDPBindAddress, uUDPListenPort );
                }
#endif
                eStatus = MB_EIO;
            }
            else if( INVALID_SOCKET == ( pxUDPIntHdl->xUDPListeningSocket.fd = socket( AF_INET, SOCK_DGRAM, 0 ) ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_UDP ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_UDP, "[CL=%hu] Can not create listening socket: %s!\n",
                                 ( USHORT ) pxUDPIntHdl->ubIdx, Error2String( WSAGetLastError(  ) ) );
                }
#endif
                eStatus = MB_EIO;
            }
            else if( SOCKET_ERROR ==
                     bind( pxUDPIntHdl->xUDPListeningSocket.fd, ( SOCKADDR * ) & pxUDPIntHdl->xUDPListeningAddr,
                           sizeof( pxUDPIntHdl->xUDPListeningAddr ) ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_UDP ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_UDP, "[CL=%hu] Can not bind: %s!\n",
                                 ( USHORT ) pxUDPIntHdl->ubIdx, Error2String( WSAGetLastError(  ) ) );
                }
#endif
            }
            else if( SOCKET_ERROR ==
                     getsockname( pxUDPIntHdl->xUDPListeningSocket.fd, ( SOCKADDR * ) & pxUDPIntHdl->xUDPListeningAddr,
                                  &iSockLen ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_UDP ) )
                {
                    vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_UDP,
                                 "[CL=%hu] Can not retrieve socket name for listening socket: %s!\n",
                                 ( USHORT ) pxUDPIntHdl->ubIdx, Error2String( WSAGetLastError(  ) ) );
                }
#endif
            }
            else if( WSA_INVALID_EVENT == ( pxUDPIntHdl->xUDPListeningSocket.xWSAEvent = WSACreateEvent(  ) ) )
            {
            }
            else if( SOCKET_ERROR ==
                     WSAEventSelect( pxUDPIntHdl->xUDPListeningSocket.fd, pxUDPIntHdl->xUDPListeningSocket.xWSAEvent,
                                     FD_READ ) )
            {
            }
            else
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_INFO, MB_LOG_PORT_UDP ) )
                {
                    dwNameBufferLen = _countof( arcNameBuffer );

                    if( SOCKET_ERROR ==
                        WSAAddressToStringA( ( LPSOCKADDR ) & pxUDPIntHdl->xUDPListeningAddr,
                                             sizeof( pxUDPIntHdl->xUDPListeningAddr ), NULL, arcNameBuffer,
                                             &dwNameBufferLen ) )
                    {
                        strcpy( arcNameBuffer, "unknown" );
                    }
                    vMBPPortLog( MB_LOG_INFO, MB_LOG_PORT_UDP, "[CL=%hu] Using %s for sending/receiving!\n",
                                 ( USHORT ) pxUDPIntHdl->ubIdx, arcNameBuffer );
                }
#endif
                pxUDPIntHdl->eMBPUDPClientNewDATAFN = eMBPUDPClientNewDATAFN;
                pxUDPIntHdl->xMBHdl = xMBHdl;
                pxUDPIntHdl->xUDPListeningSocket.bInUse = TRUE;
                if( MB_ENOERR != eMBPUDPInitCommon( pxUDPIntHdl, vMBUDPClientHandlerThread ) )
                {
                    eStatus = MB_ENORES;
                }
                else
                {
                    *pxUDPHdl = pxUDPIntHdl;
                    eStatus = MB_ENOERR;
                }
            }

            if( MB_ENOERR != eStatus )
            {
                vMBUDPHandleReset( pxUDPIntHdl, FALSE );
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
eMBPUDPClientClose( xMBPUDPHandle xUDPHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPUDPIntHandle *pxUDPIntHdl = xUDPHdl;
    MBP_ENTER_CRITICAL_SECTION(  );
    if( NULL != pxUDPIntHdl )
    {
        pxUDPIntHdl->bIsRunning = FALSE;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

STATIC void
vMBPUDPInit(  )
{
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        memset( &xMBUDPHdl[0], 0, sizeof( xMBUDPHdl ) );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBUDPHdl ); ubIdx++ )
        {
            xMBUDPHdl[ubIdx].ubIdx = IDX_INVALID;
        }
        bIsInitalized = TRUE;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
}


STATIC          eMBErrorCode
eMBPUDPInitCommon( xMBPUDPIntHandle * pxUDPIntHdl, void *( *pvHandlerThread ) ( void * ) )
{
    eMBErrorCode    eStatus = MB_EPORTERR;

    MBP_ASSERT( NULL != pxUDPIntHdl );

    /* Must be set before thread is spawned to make sure that thread
     * does not immediately quit.
     */
    pxUDPIntHdl->bIsRunning = TRUE;
    pxUDPIntHdl->bHasFailed = FALSE;

    if( NULL ==
        ( pxUDPIntHdl->pxHandlingThread =
          CreateThread( NULL, THREAD_STACKSIZE_MAX, ( LPTHREAD_START_ROUTINE ) pvHandlerThread, pxUDPIntHdl, 0,
                        NULL ) ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_UDP ) )
        {
            vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_UDP, "Can't spawn new handler thread: %s\n",
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

STATIC void    *
vMBUDPClientHandlerThread( void *pxUDPIntHdlArg )
{
    BOOL            bIsRunning = FALSE;
    xMBPUDPIntHandle *pxUDPIntHdl = pxUDPIntHdlArg;
    BOOL            bRx, bHasFailed;

    struct timeval  timeout;
    struct fd_set   fds;
    int             iResult;


    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        bIsRunning = pxUDPIntHdl->bIsRunning;

        if( INVALID_SOCKET != pxUDPIntHdl->xUDPListeningSocket.fd )
        {
            bHasFailed = FALSE;
            bRx = FALSE;
            MBP_EXIT_CRITICAL_SECTION(  );

            timeout.tv_sec = 0;
            timeout.tv_usec = MBP_WAIT_TIMEOUTS * 1000;
            FD_ZERO( &fds );
            FD_SET( pxUDPIntHdl->xUDPListeningSocket.fd, &fds );
            iResult = select( 0, &fds, 0, 0, &timeout );

            MBP_ENTER_CRITICAL_SECTION(  );
            if( -1 == iResult )
            {
                bHasFailed = TRUE;
            }
            else if( iResult > 0 )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_UDP ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP,
                                 "[CL=%hu] New data for client (socket=%d).\n", ( USHORT ) pxUDPIntHdl->ubIdx,
                                 pxUDPIntHdl->xUDPListeningSocket.fd );
                }
#endif
                if( pxUDPIntHdl->bIsRunning && pxUDPIntHdl->xUDPListeningSocket.bInUse &&
                    ( NULL != pxUDPIntHdl->eMBPUDPClientNewDATAFN ) )
                {
                    pxUDPIntHdl->eMBPUDPClientNewDATAFN( pxUDPIntHdl->xMBHdl );
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
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_UDP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "[CL=%hu] Finishing handler thread.\n",
                     ( USHORT ) pxUDPIntHdl->ubIdx );
    }
#endif
    MBP_ENTER_CRITICAL_SECTION(  );
    vMBUDPHandleReset( pxUDPIntHdl, TRUE );
    MBP_EXIT_CRITICAL_SECTION(  );
    ExitThread( 0 );
    return NULL;
}

STATIC void
vMBUDPClientHandleReset( fd_t * pxPollFd, BOOL bClose )
{
    if( bClose && ( INVALID_SOCKET != pxPollFd->fd ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_UDP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "Closing socket (socket=%d)\n", pxPollFd->fd );
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
vMBUDPHandleReset( xMBPUDPIntHandle * pxUDPHdl, BOOL bClose )
{
    MBP_ASSERT( NULL != pxUDPHdl );
    pxUDPHdl->ubIdx = IDX_INVALID;
    pxUDPHdl->bHasFailed = TRUE;
    pxUDPHdl->bIsRunning = FALSE;
    pxUDPHdl->eMBPUDPClientNewDATAFN = NULL;
    /* pxUDPHdl->pxHandlingThread shall NOT be set to NULL because it
     * is always cleaned up when handle is allocated. 
     */
    pxUDPHdl->xMBHdl = MB_HDL_INVALID;
    vMBUDPClientHandleReset( &( pxUDPHdl->xUDPListeningSocket ), bClose );
    memset( &pxUDPHdl->xUDPListeningAddr, 0, sizeof( SOCKADDR_IN ) );
}

eMBErrorCode
eMBPUDPConWrite( xMBPUDPHandle xUDPHdl, const CHAR * pcUDPClientAddress, USHORT usUDPSlavePort, const UBYTE * pubBuffer,
                 USHORT usBufferLen )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    SOCKADDR_IN     xClientAddr;
    xMBPUDPIntHandle *pxUDPIntHdl = xUDPHdl;
    char            arcNameBuffer[32];
    DWORD           dwNameBufferLen;

    if( NULL != pxUDPIntHdl )
    {
        if( !bInitializeSockAddr( &xClientAddr, pcUDPClientAddress, usUDPSlavePort ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_UDP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "[CL=%hu] Can not get address for %s:%hu!\n",
                             ( USHORT ) pxUDPIntHdl->ubIdx, pcUDPClientAddress, usUDPSlavePort );
            }
#endif
            eStatus = MB_EIO;
        }
        else
        {
            dwNameBufferLen = _countof( arcNameBuffer );
            if( SOCKET_ERROR ==
                WSAAddressToStringA( ( LPSOCKADDR ) & xClientAddr, sizeof( xClientAddr ), NULL, arcNameBuffer,
                                     &dwNameBufferLen ) )
            {
                strcpy( arcNameBuffer, "unknown" );
            }
            if( !sendto
                ( pxUDPIntHdl->xUDPListeningSocket.fd, pubBuffer, usBufferLen, 0, ( SOCKADDR * ) & xClientAddr,
                  sizeof( xClientAddr ) ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_UDP ) )
                {

                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP,
                                 "[CL=%hu] Can not send %hu bytes (socket=%d) to %s: %s!\n",
                                 ( USHORT ) pxUDPIntHdl->ubIdx, usBufferLen, pxUDPIntHdl->xUDPListeningSocket.fd,
                                 arcNameBuffer, Error2String( WSAGetLastError(  ) ) );
                }
#endif
                eStatus = MB_EIO;
            }
            else
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_UDP ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "[CL=%hu] Sent %hu bytes (socket=%d) to %s.\n",
                                 ( USHORT ) pxUDPIntHdl->ubIdx, usBufferLen, pxUDPIntHdl->xUDPListeningSocket.fd,
                                 arcNameBuffer );
                }
#endif
                eStatus = MB_ENOERR;
            }
        }
    }
    return eStatus;
}

eMBErrorCode
eMBPUDPConReadExt( xMBPUDPHandle xUDPHdl, CHAR ** pucClientAddress, USHORT * pusClientPort, UBYTE * pubBuffer,
                   USHORT * pusBufferLen, USHORT usBufferMax )
{
    char            arcNameBuffer[32];
    SOCKADDR_IN     xSockAddr;
    int             xSockAddrLen = sizeof( xSockAddr );
    DWORD           dwNameBufferLen;

    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPUDPIntHandle *pxUDPIntHdl = xUDPHdl;
    int             iBytesReceived;
    if( NULL != pxUDPIntHdl )
    {
        iBytesReceived =
            recvfrom( pxUDPIntHdl->xUDPListeningSocket.fd, pubBuffer, usBufferMax, 0, ( LPSOCKADDR ) & xSockAddr,
                      &xSockAddrLen );
        if( ( SOCKET_ERROR == iBytesReceived ) || ( 0 == iBytesReceived ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_UDP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "[CL=%hu] Can not recv data (socket=%d): %s!\n",
                             ( USHORT ) pxUDPIntHdl->ubIdx, pxUDPIntHdl->xUDPListeningSocket.fd,
                             Error2String( WSAGetLastError(  ) ) );
            }
#endif
            eStatus = MB_EIO;
        }
        else
        {
            dwNameBufferLen = _countof( arcNameBuffer );
            if( NULL != pusClientPort )
            {
                *pusClientPort = ntohs( xSockAddr.sin_port );
            }
            xSockAddr.sin_port = htons( 0 );
            if( 0 !=
                WSAAddressToStringA( ( LPSOCKADDR ) & xSockAddr, sizeof( xSockAddr ), NULL, arcNameBuffer,
                                     &dwNameBufferLen ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_UDP ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "[CL=%hu] Failed resolving client source address: %s\n",
                                 ( USHORT ) pxUDPIntHdl->ubIdx, Error2String( WSAGetLastError(  ) ) );
                }
#endif
                eStatus = MB_EIO;
            }
            else
            {
                *pusBufferLen = iBytesReceived;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_UDP ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "[CL=%hu] Received %hu bytes (socket=%d).\n",
                                 ( USHORT ) pxUDPIntHdl->ubIdx, *pusBufferLen, pxUDPIntHdl->xUDPListeningSocket.fd );
                }
#endif
                if( NULL != arcNameBuffer )
                {
                    MBP_UDP_CLIENTADDR_COPY( arcNameBuffer, *pucClientAddress );
                }

                eStatus = MB_ENOERR;
            }
        }
    }
    return eStatus;
}

eMBErrorCode
eMBPUDPConRead( xMBPUDPHandle xUDPHdl, UBYTE * pubBuffer, USHORT * pusBufferLen, USHORT usBufferMax )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPUDPIntHandle *pxUDPIntHdl = xUDPHdl;
    int             iBytesReceived;
    if( NULL != pxUDPIntHdl )
    {
        iBytesReceived = recvfrom( pxUDPIntHdl->xUDPListeningSocket.fd, pubBuffer, usBufferMax, 0, NULL, 0 );
        if( ( SOCKET_ERROR == iBytesReceived ) || ( 0 == iBytesReceived ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_UDP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "[CL=%hu] Can not recv data (socket=%d): %s!\n",
                             ( USHORT ) pxUDPIntHdl->ubIdx, pxUDPIntHdl->xUDPListeningSocket.fd,
                             Error2String( WSAGetLastError(  ) ) );
            }
#endif
            eStatus = MB_EIO;
        }
        else
        {
            *pusBufferLen = iBytesReceived;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 ) && ( MBP_UDP_DEBUG == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_UDP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_UDP, "[CL=%hu] Received %hu bytes (socket=%d).\n",
                             ( USHORT ) pxUDPIntHdl->ubIdx, *pusBufferLen, pxUDPIntHdl->xUDPListeningSocket.fd );
            }
#endif
            eStatus = MB_ENOERR;
        }
    }
    return eStatus;
}

void
vMBPUDPDllInit( void )
{
}

void
vMBPUDPDLLClose( void )
{
    BOOL            bThreadsRunning = FALSE;
    UBYTE           ubIdx;
    HANDLE          hThreadHandle;

    /* If this code is called we know that no MODBUS instances are
     * newly created (Because of the INIT lock) and that all
     * still serial threads are going to shut down since
     * their bIsRunning flag has been set to FALSE.
     */
    do
    {
        hThreadHandle = NULL;
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBUDPHdl ); ubIdx++ )
        {
            if( NULL != xMBUDPHdl[ubIdx].pxHandlingThread )
            {
                hThreadHandle = xMBUDPHdl[ubIdx].pxHandlingThread;
                xMBUDPHdl[ubIdx].pxHandlingThread = NULL;
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
