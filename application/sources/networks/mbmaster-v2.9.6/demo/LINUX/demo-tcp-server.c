/*
 * MODBUS Library: Example applicaion for LINUX/CYGWIN
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-tcp-server.c,v 1.2 2009-10-20 21:16:59 embedded-solutions.anovak Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/

#define MASTER1_PORT	            ( 1 )
#define MASTER1_BAUDRATE            ( 19200 )
#define MASTER1_PARITY              ( MB_PAR_NONE )

#define SERVER_LISTEN_IFACE        "127.0.0.1"
#define SERVER_PORT                502

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/
eMBErrorCode    eMBPTCPClientConnected( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl );
eMBErrorCode    eMBTCPClientNewDataCB( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl );
eMBErrorCode    eMBTCPClientDisconnected( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl );

/* ----------------------- Start implementation -----------------------------*/
int
main( int argc, char *argv[] )
{

    xMBHandle       xMBMMaster;

    xMBPTCPHandle   xHandle;

    if( MB_ENOERR != eMBMTCPInit( &xMBMMaster ) )


    {

        fprintf( stderr, "%s: Could not initialize modbus stack.\n", argv[0] );

        return -1;

    }


    else if( MB_ENOERR !=
             eMBPTCPServerInit( &xHandle, SERVER_LISTEN_IFACE, SERVER_PORT,
                                xMBMMaster, eMBPTCPClientConnected, eMBTCPClientNewDataCB, eMBTCPClientDisconnected ) )


    {

        fprintf( stderr, "%s: Could not initialize modbus server on interface %s:%d.\n", argv[0], SERVER_LISTEN_IFACE, SERVER_PORT );

        return -2;

    }


    else


    {

        printf( "%s: Successfully initialized modbus server on interface %s:%d\n", argv[0], SERVER_LISTEN_IFACE, SERVER_PORT );

        while( 'q' != getchar(  ) );

    }

    return 0;
}

eMBErrorCode
eMBPTCPClientConnected( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl )
{

    printf( "Client connected.\n" );

    return MB_ENOERR;
}

eMBErrorCode
eMBTCPClientNewDataCB( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl )
{

    printf( "Client new Data.\n" );

    return MB_ENOERR;
}

eMBErrorCode
eMBTCPClientDisconnected( xMBHandle xMBHdl, xMBPTCPClientHandle xTCPClientHdl )
{

    printf( "Client disconnected.\n" );

    return MB_ENOERR;
}
