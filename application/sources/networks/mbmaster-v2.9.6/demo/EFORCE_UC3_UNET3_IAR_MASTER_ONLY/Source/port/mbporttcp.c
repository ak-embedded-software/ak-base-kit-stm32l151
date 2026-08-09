/*
 * MODBUS Libary: uC3/uNET port
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: template.c,v 1.1 2007-08-19 12:31:23 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "kernel.h"
#include "kernel_id.h"
#include "net_hdr.h"
#include "net_id.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define MAX_SLAVE_HDLS                      ( 1 )
#define MAX_MASTER_HDLS                     ( MB_UTILS_NARRSIZE( xMBPMasterSockets ) )
#define MAX_SLAVE_CLIENT_HDLS               ( 4 )
#define IDX_INVALID                         ( 255 )
#define MB_TCP_READBACKLOG					( 16 )
#define MB_CLIENT_MAXIDLETIME				( 60000 )

#ifndef MB_MASTER_CODE
#define MB_MASTER_CODE						( 1 )
#endif

#ifndef MB_SLAVE_CODE
#define MB_SLAVE_CODE						( 1 )
#endif


#ifndef MBP_TCP_DEBUG
#define MBP_TCP_DEBUG                       ( 0 )
#endif

#define MBP_TCP_HDL_COMMON \
    UBYTE           ubIdx; \
    xMBTCPIntHandleType eType; \
    xMBHandle       xMBHdl; \
    peMBPTCPClientNewDataCB eMBPTCPClientNewDataFN; \
    peMBPTCPClientDisconnectedCB eMBPTCPClientDisconnectedFN;

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    SOCKET_INIT,
    SOCKET_WAIT_CONNECT,
    SOCKET_IS_CONNECTED,
    SOCKET_CAN_READ,
    SOCKET_WAIT_CALLBACK,
    SOCKET_CLOSING
} xuNETSocketReadState;

typedef struct
{
    UH              xSOC;
    xuNETSocketReadState xSOCState;
    UBYTE           arubReadBackLog[MB_TCP_READBACKLOG];
    UBYTE           arubBytesRead;
} xMBPTCPIntClientHandle;

typedef enum
{
    TCP_MODBUS_UNKNOWN,
    TCP_MODBUS_MASTER,
    TCP_MODBUS_SLAVE
} xMBTCPIntHandleType;

typedef struct
{
MBP_TCP_HDL_COMMON
} xMBPTCPIntCommonHandle;

#if MB_MASTER_CODE == 1
typedef struct
{
    MBP_TCP_HDL_COMMON
	xMBPTCPIntClientHandle xClientCon;
} xMBPTCPIntMasterHandle;
#endif

#if MB_SLAVE_CODE == 1
typedef struct
{
    MBP_TCP_HDL_COMMON
	xMBPTCPIntClientHandle xServerCon;
	UW lastActiveTime;
    peMBPTCPClientConnectedCB eMBPTCPClientConnectedFN;
} xMBPTCPIntSlaveHandle;
#endif

/* ----------------------- Static variables ---------------------------------*/
#if MB_MASTER_CODE == 1
STATIC UH       xMBPMasterSockets[] = { MBMASTER_S1 };
STATIC xMBPTCPIntMasterHandle xMBTCPMasterHdls[MAX_MASTER_HDLS];

#endif
#if MB_SLAVE_CODE == 1
STATIC UH       xMBPSlaveSockets[] = { MBSLAVE_S1 };
STATIC xMBPTCPIntSlaveHandle xMBTCPSlaveHdls[MAX_SLAVE_HDLS];
#endif
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vMBPTCPInit( void );
STATIC void     vMBTCPCommonHandleReset( xMBPTCPIntCommonHandle * pxTCPHdl, BOOL bClose );
STATIC void     vMBTCPClientHandleReset( xMBPTCPIntClientHandle * pxClientHdl, BOOL bClose );
#if MB_SLAVE_CODE == 1
STATIC void     vMBTCPSlaveHandleReset( xMBPTCPIntSlaveHandle * pxTCPSlaveHdl, BOOL bClose );
#endif
#if MB_MASTER_CODE == 1
STATIC void     vMBTCPMasterHandleReset( xMBPTCPIntMasterHandle * pxTCPMasterHdl, BOOL bClose, BOOL bFullReset );
#endif
STATIC UW       vMBPTCPCallbacks( UH sid, UH event, ER ercd );
STATIC void     vMBTCPReadFromSocket( xMBPTCPIntClientHandle * pxTCPClientHdl, BOOL * bIsBroken );

ER              cfg_soc_trace( UD sid, UB code, VP val );
ER              con_soc_trace( UH sid, T_NODE * host, UB con_flg );
ER              snd_soc_trace( UH sid, VP data, UH len );
ER              rcv_soc_trace( UH sid, VP data, UH len );
ER              cls_soc_trace( UH sid, UB cls_flg );

/* ----------------------- Start implementation -----------------------------*/

/* --------------------------------------------------------------------------*/
/* ----------------------- SERVER CODE (MODBUS SLAVE) -----------------------*/
/* --------------------------------------------------------------------------*/
#if MB_SLAVE_CODE == 1
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

    ER              ercd;
    vMBPTCPInit(  );
    if( NULL != pxTCPHdl )
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPSlaveHdls ); ubIdx++ )
        {
            if( ( IDX_INVALID == xMBTCPSlaveHdls[ubIdx].ubIdx ) && ( SOCKET_INIT == xMBTCPSlaveHdls[ubIdx].xServerCon.xSOCState ) )
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
            pxTCPIntSlaveHdl->eMBPTCPClientNewDataFN = eMBPTCPClientNewDataFNArg;
            pxTCPIntSlaveHdl->eMBPTCPClientDisconnectedFN = eMBPTCPClientDisconnectedFNArg;
            pxTCPIntSlaveHdl->eMBPTCPClientConnectedFN = eMBPTCPClientConnectedFNArg;
            pxTCPIntSlaveHdl->xMBHdl = xMBHdlArg;
            pxTCPIntSlaveHdl->xServerCon.xSOC = xMBPSlaveSockets[pxTCPIntSlaveHdl->ubIdx];
            eStatus = MB_ENOERR;

            if( E_OK != ( ercd = cfg_soc_trace( pxTCPIntSlaveHdl->xServerCon.xSOC, SOC_CBK_HND, ( VP ) & vMBPTCPCallbacks ) ) )
            {
                /* can not set socket options. this should not fail so assert in debug */
                MBP_ASSERT( E_OK == ercd );
                eStatus = MB_EPORTERR;
            }
            else if( E_OK !=
                     ( ercd = cfg_soc_trace( pxTCPIntSlaveHdl->xServerCon.xSOC, SOC_CBK_FLG, ( VP ) ( EV_SOC_CON | EV_SOC_RCV | EV_SOC_SND | EV_SOC_CLS ) ) ) )
            {
                /* can not set socket options. this should not fail so assert in debug */
                MBP_ASSERT( E_OK == ercd );
                eStatus = MB_EPORTERR;
            }

            if( MB_ENOERR == eStatus )
            {
                *pxTCPHdl = pxTCPIntSlaveHdl;
            }
            else
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
    MBP_ASSERT( NULL != pxTCPSlaveHdl );
    vMBTCPClientHandleReset( &( pxTCPSlaveHdl->xServerCon ), bClose );
    vMBTCPCommonHandleReset( ( xMBPTCPIntCommonHandle * ) pxTCPSlaveHdl, bClose );
    pxTCPSlaveHdl->eMBPTCPClientConnectedFN = NULL;
    pxTCPSlaveHdl->eType = TCP_MODBUS_SLAVE;
	pxTCPSlaveHdl->lastActiveTime = 0;
}

eMBErrorCode
eMBTCPServerClose( xMBPTCPHandle xTCPHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntSlaveHandle *pxTCPIntSlaveHdl = xTCPHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTCPIntSlaveHdl, xMBTCPSlaveHdls ) && ( pxTCPIntSlaveHdl->eType == TCP_MODBUS_SLAVE ) )
    {
        vMBTCPSlaveHandleReset( pxTCPIntSlaveHdl, TRUE );
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

void
vMBTCPServerPoll( void )
{
    UBYTE           ubIdx;
    BOOL            bIsBroken, bHasData;
    eMBErrorCode    eStatus;
    ER              ercd;
    T_NODE          host = { 0, 0, 0, 0 };

    UW              deltaTime;
    UW              currentTime = vget_tms(  );

	if( !bIsInitalized )
	{
	  	return;
	}
    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xMBTCPSlaveHdls ); ubIdx++ )
    {
        if( IDX_INVALID != xMBTCPSlaveHdls[ubIdx].ubIdx )
        {
            bIsBroken = FALSE;
            bHasData = FALSE;

			switch( xMBTCPSlaveHdls[ubIdx].xServerCon.xSOCState )
			{
			case SOCKET_INIT:
                ercd = con_soc( xMBTCPSlaveHdls[ubIdx].xServerCon.xSOC, &host, SOC_SER );
                xMBTCPSlaveHdls[ubIdx].xServerCon.xSOCState = SOCKET_WAIT_CONNECT;
                MBP_ASSERT( E_WBLK == ercd );
				break;
			case SOCKET_IS_CONNECTED:
                xMBTCPSlaveHdls[ubIdx].xServerCon.xSOCState = SOCKET_CAN_READ;
				xMBTCPSlaveHdls[ubIdx].lastActiveTime = vget_tms(  );
                if( MB_ENOERR !=
                    ( eStatus = xMBTCPSlaveHdls[ubIdx].eMBPTCPClientConnectedFN( xMBTCPSlaveHdls[ubIdx].xMBHdl, &xMBTCPSlaveHdls[ubIdx].xServerCon ) ) )
                {
                    ( void )eStatus;
                    vMBTCPClientHandleReset( &xMBTCPSlaveHdls[ubIdx].xServerCon, TRUE );
                }
				break;
			case SOCKET_WAIT_CALLBACK:
			  	deltaTime = currentTime - xMBTCPSlaveHdls[ubIdx].lastActiveTime;
				if( deltaTime > MB_CLIENT_MAXIDLETIME )
				{
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
					if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
					{
						vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[%d] disconnecting master due to inactivity\n", ( int )xMBTCPSlaveHdls[ubIdx].ubIdx );
					}
#endif
					bIsBroken = TRUE;
				}
				break;
			case SOCKET_CAN_READ:
			  	xMBTCPSlaveHdls[ubIdx].lastActiveTime =  vget_tms(  );
			  	vMBTCPReadFromSocket( &xMBTCPSlaveHdls[ubIdx].xServerCon, &bIsBroken );
			  	break;
			}

            if( xMBTCPSlaveHdls[ubIdx].xServerCon.arubBytesRead > 0 )
            {
                bHasData = TRUE;
            }


            if( bIsBroken )
            {
                ( void )xMBTCPSlaveHdls[ubIdx].eMBPTCPClientDisconnectedFN( xMBTCPSlaveHdls[ubIdx].xMBHdl, &( xMBTCPSlaveHdls[ubIdx].xServerCon ) );
            }
            else if( bHasData )
            {
                ( void )xMBTCPSlaveHdls[ubIdx].eMBPTCPClientNewDataFN( xMBTCPSlaveHdls[ubIdx].xMBHdl, &( xMBTCPSlaveHdls[ubIdx].xServerCon ) );
            }

        }
    }
}
#endif

/* --------------------------------------------------------------------------*/
/* ----------------------- CLIENT CODE (MODBUS MASTER) ----------------------*/
/* --------------------------------------------------------------------------*/
#if MB_MASTER_CODE == 1
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
            if( ( IDX_INVALID == xMBTCPMasterHdls[ubIdx].ubIdx ) && ( SOCKET_INIT == xMBTCPMasterHdls[ubIdx].xClientCon.xSOCState ) )
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
            *pxTCPHdl = pxTCPMasterIntHdl;
            eStatus = MB_ENOERR;
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
    ER              ercd;
    xMBPTCPIntMasterHandle *pxTCPMasterIntHdl = xTCPHdl;
    T_NODE          remote;

    if( MB_IS_VALID_HDL( pxTCPMasterIntHdl, xMBTCPMasterHdls ) && ( pxTCPMasterIntHdl->eType == TCP_MODBUS_MASTER ) )
    {
        remote.port = usTCPPort;
        remote.ver = IP_VER4;
        remote.num = 0;
        remote.ipa = ip_aton( pcConnectAddress );

        /* it might be the case that a previsouly open instance is still about to close
         * as the disconnect takes some time. wait for this to finish.
         */
        while( SOCKET_CLOSING == pxTCPMasterIntHdl->xClientCon.xSOCState )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[%d] socket still closing previous connection...\n", ( int )pxTCPMasterIntHdl->ubIdx );
            }
#endif
            dly_tsk( 100 );
        }
        /* ressources are allocated statically - take one of the static sockets available
         * for each master instance. in case of an error connection is closed and socket
         * is freed again.
         */
        pxTCPMasterIntHdl->xClientCon.xSOC = xMBPMasterSockets[pxTCPMasterIntHdl->ubIdx];
        if( E_OK != ( ercd = cfg_soc_trace( pxTCPMasterIntHdl->xClientCon.xSOC, SOC_TMO_CON, ( VP ) 5000 ) ) )
        {
            eStatus = MB_EIO;
        }
        else if( E_OK != ( ercd = con_soc_trace( pxTCPMasterIntHdl->xClientCon.xSOC, &remote, SOC_CLI ) ) )
        {
            /* socket connection failed - maybe network is done. Reset
             * xSOC such that no cls_soc is called.
             */
            eStatus = MB_EIO;
        }
        else if( E_OK != ( ercd = cfg_soc_trace( pxTCPMasterIntHdl->xClientCon.xSOC, SOC_CBK_HND, ( VP ) & vMBPTCPCallbacks ) ) )
        {
            /* can not set socket options. this should not fail so assert in debug */
            MBP_ASSERT( E_OK == ercd );
            eStatus = MB_EPORTERR;
        }
        else if( E_OK != ( ercd = cfg_soc_trace( pxTCPMasterIntHdl->xClientCon.xSOC, SOC_CBK_FLG, ( VP ) ( EV_SOC_RCV | EV_SOC_SND | EV_SOC_CLS ) ) ) )
        {
            /* can not set socket options. this should not fail so assert in debug */
            MBP_ASSERT( E_OK == ercd );
            eStatus = MB_EPORTERR;
        }
        else
        {
            *pxTCPClientHdl = &( pxTCPMasterIntHdl->xClientCon );
            pxTCPMasterIntHdl->xClientCon.xSOCState = SOCKET_CAN_READ;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[%d] opened new socket %d\n", ( int )pxTCPMasterIntHdl->ubIdx,
                             ( int )pxTCPMasterIntHdl->xClientCon.xSOC );
            }
#endif
            eStatus = MB_ENOERR;
        }

        if( MB_ENOERR != eStatus )
        {
            /* Initialization failed - reset handle */
            vMBTCPClientHandleReset( &pxTCPMasterIntHdl->xClientCon, TRUE );
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
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[%d] closing master instance\n", ( int )pxTCPMasterIntHdl->ubIdx );
        }
#endif
        vMBTCPMasterHandleReset( pxTCPMasterIntHdl, TRUE, TRUE );
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
vMBTCPClientPoll( void )
{
    UBYTE           ubIdx;
    BOOL            bHasData;
    BOOL            bIsBroken;

	if( !bIsInitalized )
	{
	  	return;
	}
    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xMBTCPMasterHdls ); ubIdx++ )
    {
        MBP_ENTER_CRITICAL_SECTION(  );

        /* A client socket is polled only when
         *  a) the specific master instance is active
         *  b) the master is connect to a MODBUS slave
         */
        if( ( IDX_INVALID != xMBTCPMasterHdls[ubIdx].ubIdx ) && ( SOC_UNUSED != xMBTCPMasterHdls[ubIdx].xClientCon.xSOC ) )
        {
            bIsBroken = FALSE;
            bHasData = FALSE;

            /* Check if it is allowed to read from the socket. This is
             * a uNET3 limitation.
             */
            if( SOCKET_CAN_READ == xMBTCPMasterHdls[ubIdx].xClientCon.xSOCState )
            {
                vMBTCPReadFromSocket( &xMBTCPMasterHdls[ubIdx].xClientCon, &bIsBroken );
            }
            /* Check if we still have old data in the backlog not yet consumed by the
             * master stack.
             */
            if( xMBTCPMasterHdls[ubIdx].xClientCon.arubBytesRead > 0 )
            {
                bHasData = TRUE;
            }

            if( bIsBroken )
            {
                xMBTCPMasterHdls[ubIdx].eMBPTCPClientDisconnectedFN( xMBTCPMasterHdls[ubIdx].xMBHdl, &( xMBTCPMasterHdls[ubIdx].xClientCon ) );
            }
            else if( bHasData )
            {
                xMBTCPMasterHdls[ubIdx].eMBPTCPClientNewDataFN( xMBTCPMasterHdls[ubIdx].xMBHdl, &( xMBTCPMasterHdls[ubIdx].xClientCon ) );
            }
        }
        MBP_EXIT_CRITICAL_SECTION(  );
    }
}
#endif

/* --------------------------------------------------------------------------*/
/* ----------------------- COMMON CODE --------------------------------------*/
/* --------------------------------------------------------------------------*/
STATIC void
vMBTCPReadFromSocket( xMBPTCPIntClientHandle * pxTCPClientHdl, BOOL * bIsBroken )
{
    UBYTE           ubBytesToReadMax;
    ER              ercd;
    *bIsBroken = FALSE;
    ubBytesToReadMax = MB_UTILS_NARRSIZE( pxTCPClientHdl->arubReadBackLog ) - pxTCPClientHdl->arubBytesRead;
    if( ubBytesToReadMax > 0 )
    {
        ercd = rcv_soc_trace( pxTCPClientHdl->xSOC, &pxTCPClientHdl->arubReadBackLog[pxTCPClientHdl->arubBytesRead], ubBytesToReadMax );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[?] rcv socket %d: %d\n", ( int )pxTCPClientHdl->xSOC, ( int )ercd );
        }
#endif
        if( ercd > 0 )
        {
            pxTCPClientHdl->arubBytesRead += ercd;
        }
        else
        {
            switch ( ercd )
            {
            case E_OK:
                break;
            case E_WBLK:
                pxTCPClientHdl->xSOCState = SOCKET_WAIT_CALLBACK;
                break;
            default:
                /* client connection dead - signal disconnect */
                *bIsBroken = TRUE;
            }
        }
    }
}

STATIC          UW
vMBPTCPCallbacks( UH sid, UH event, ER ercd )
{
    xMBPTCPIntClientHandle *pxHdl = NULL;
    xMBPTCPIntCommonHandle *pxCommonHdl = NULL;
    UBYTE           ubIdx;

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "uNet3 tcp client callback sid=%d, event=%d, ercd=%d \n", sid, event, ercd );
    }
#endif

#if MB_MASTER_CODE == 1
    /* First we look up the socket ignoring the state of the instances */
    for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPMasterHdls ); ubIdx++ )
    {
        if( sid == xMBTCPMasterHdls[ubIdx].xClientCon.xSOC )
        {
            pxHdl = &xMBTCPMasterHdls[ubIdx].xClientCon;
            pxCommonHdl = ( xMBPTCPIntCommonHandle * ) & xMBTCPMasterHdls[ubIdx];
        }
    }
#endif
#if MB_SLAVE_CODE == 1
    for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPSlaveHdls ); ubIdx++ )
    {
        if( sid == xMBTCPSlaveHdls[ubIdx].xServerCon.xSOC )
        {
            pxHdl = &xMBTCPSlaveHdls[ubIdx].xServerCon;
            pxCommonHdl = ( xMBPTCPIntCommonHandle * ) & xMBTCPSlaveHdls[ubIdx];
        }
    }
#endif

    if( EV_SOC_RCV == event )
    {
        pxHdl->xSOCState = SOCKET_CAN_READ;
    }
    else if( EV_SOC_CLS == event )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "socket %d now closed: %d\n", sid, ( int )ercd );
        }
#endif
        pxHdl->xSOCState = SOCKET_INIT;
    }
    else if( EV_SOC_CON == event )
    {
        MBP_ASSERT( SOCKET_WAIT_CONNECT == pxHdl->xSOCState );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "socket %d connect event: %d\n", sid, ( int )ercd );
        }
#endif
        pxHdl->xSOCState = SOCKET_IS_CONNECTED;
    }
    else
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "unknown event for socket %d\n", sid );
        }
#endif
        pxHdl->xSOCState = SOCKET_INIT;
    }
    return E_OK;
}

STATIC void
vMBPTCPInit(  )
{
    UBYTE           ubIdx;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
#if MB_SLAVE_CODE == 1
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPSlaveHdls ); ubIdx++ )
        {
            vMBTCPSlaveHandleReset( &xMBTCPSlaveHdls[ubIdx], FALSE );
        }
#endif
#if MB_MASTER_CODE == 1
        for( ubIdx = 0; ubIdx < ( UBYTE ) MB_UTILS_NARRSIZE( xMBTCPMasterHdls ); ubIdx++ )
        {
            vMBTCPMasterHandleReset( &xMBTCPMasterHdls[ubIdx], FALSE, TRUE );
        }
#endif
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
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[%d] closing socket %d\n", ( int )pxTCPIntCommonHdl->ubIdx, ( int )pxTCPIntClientHdl->xSOC );
        }
#endif
        MBP_ENTER_CRITICAL_SECTION(  );
        vMBTCPClientHandleReset( pxTCPIntClientHdl, TRUE );
        MBP_EXIT_CRITICAL_SECTION(  );
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

void
vMBTCPClientHandleReset( xMBPTCPIntClientHandle * pxClientHdl, BOOL bClose )
{
    ER              ercd;
    if( bClose )
    {
        /* We only can close already open sockets. */
        if( SOC_UNUSED != pxClientHdl->xSOC )
        {
            /* Check if a close is already pending for this socket.
             * Do not try to close a socket twice.
             */
            if( SOCKET_CLOSING != pxClientHdl->xSOCState )
            {
                ercd = cls_soc_trace( pxClientHdl->xSOC, SOC_TCP_CLS );
                switch ( ercd )
                {
                case E_WBLK:
                    pxClientHdl->xSOCState = SOCKET_CLOSING;
                    break;
                default:
                    pxClientHdl->xSOCState = SOCKET_INIT;
                    pxClientHdl->xSOC = SOC_UNUSED;
                }
            }
        }
        else
        {
            pxClientHdl->xSOCState = SOCKET_INIT;
            pxClientHdl->xSOC = SOC_UNUSED;
        }
    }
    else
    {
        pxClientHdl->xSOCState = SOCKET_INIT;
        pxClientHdl->xSOC = SOC_UNUSED;
    }
    pxClientHdl->arubBytesRead = 0;
}

STATIC void
vMBTCPCommonHandleReset( xMBPTCPIntCommonHandle * pxTCPCommonHdl, BOOL bClose )
{
    pxTCPCommonHdl->ubIdx = IDX_INVALID;
    pxTCPCommonHdl->eType = TCP_MODBUS_UNKNOWN;
    pxTCPCommonHdl->xMBHdl = MB_HDL_INVALID;
    pxTCPCommonHdl->eMBPTCPClientDisconnectedFN = NULL;
    pxTCPCommonHdl->eMBPTCPClientNewDataFN = NULL;
}

eMBErrorCode
eMBPTCPConRead( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, UBYTE * pubBuffer, USHORT * pusBufferLen, USHORT usBufferMax )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntCommonHandle *pxTCPIntCommonHdl = xTCPHdl;
    xMBPTCPIntClientHandle *pxTCPIntClientHdl = xTCPClientHdl;
    USHORT          usIdx, usIdx2;
    ( void )pxTCPIntCommonHdl;
    if( NULL != pxTCPIntClientHdl )
    {
        /* First copy bytes from backlog into the buffer */
        for( usIdx = 0; ( usIdx < pxTCPIntClientHdl->arubBytesRead ) && ( usIdx < usBufferMax ); usIdx++ )
        {
            pubBuffer[usIdx] = pxTCPIntClientHdl->arubReadBackLog[usIdx];
        }
        *pusBufferLen = usIdx;
        /* Now move any pending bytes to the front */
        for( usIdx2 = 0; usIdx < pxTCPIntClientHdl->arubBytesRead; usIdx2++, usIdx++ )
        {
            pxTCPIntClientHdl->arubReadBackLog[usIdx2] = pxTCPIntClientHdl->arubReadBackLog[usIdx];
        }
        pxTCPIntClientHdl->arubBytesRead = usIdx2;
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[%d] read %d bytes on socket %d. %d bytes left\n", ( int )pxTCPIntCommonHdl->ubIdx,
                         ( int )*pusBufferLen, ( int )pxTCPIntClientHdl->xSOC, ( int )pxTCPIntClientHdl->arubBytesRead );
        }
#endif
        eStatus = MB_ENOERR;
    }
    return eStatus;
}

eMBErrorCode
eMBPTCPConWrite( xMBPTCPHandle xTCPHdl, xMBPTCPClientHandle xTCPClientHdl, const UBYTE * pubBuffer, USHORT usBufferLen )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xMBPTCPIntCommonHandle *pxTCPIntCommonHdl = xTCPHdl;
    xMBPTCPIntClientHandle *pxTCPIntClientHdl = xTCPClientHdl;
    ER              ercd;

    ( void )pxTCPIntCommonHdl;
    if( NULL != xTCPClientHdl )
    {

        ercd = snd_soc_trace( pxTCPIntClientHdl->xSOC, ( VP ) pubBuffer, usBufferLen );
        if( usBufferLen == ercd )
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
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "[%d] sent %d bytes on socket %d: %d\n", ( int )pxTCPIntCommonHdl->ubIdx, ( int )usBufferLen,
                         ( int )pxTCPIntClientHdl->xSOC, ( int )ercd );
        }
#endif
    }
    return eStatus;
}

ER
cfg_soc_trace( UD sid, UB code, VP val )
{
    ER              res = cfg_soc( sid, code, val );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "cfg_soc( %d, %d, %x): %d\n", ( int )sid, ( int )code, ( unsigned int )val, ( int )res );
    }
#endif
    return res;
}

ER
con_soc_trace( UH sid, T_NODE * host, UB con_flg )
{
    ER              res = con_soc( sid, host, con_flg );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "con_soc( %d, %x, %x): %d\n", ( int )sid, ( unsigned int )host, ( unsigned int )con_flg, ( int )res );
    }
#endif
    return res;
}

ER
snd_soc_trace( UH sid, VP data, UH len )
{
    ER              res = snd_soc( sid, data, len );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "snd_soc( %d, %x, %d): %d\n", ( int )sid, ( unsigned int )data, ( int )len, ( int )res );
    }
#endif
    return res;
}

ER
rcv_soc_trace( UH sid, VP data, UH len )
{
    ER              res = rcv_soc( sid, data, len );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "rcv_soc( %d, %x, %d): %d\n", ( int )sid, ( unsigned int )data, ( int )len, ( int )res );
    }
#endif
    return res;
}

ER
cls_soc_trace( UH sid, UB cls_flg )
{
    ER              res = cls_soc( sid, cls_flg );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_TCP ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_TCP, "cls_soc( %d, %x): %d\n", ( int )sid, ( unsigned int )cls_flg, ( int )res );
    }
#endif
    return res;
}
