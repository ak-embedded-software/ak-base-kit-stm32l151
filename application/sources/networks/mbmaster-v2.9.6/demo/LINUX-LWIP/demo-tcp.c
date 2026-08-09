/* 
 * MODBUS Slave Library: Linux/lwIP
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-tcp.c,v 1.1 2014-08-23 11:48:22 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <lwip/api.h>
#include <lwip/tcpip.h>
#include "netif/tapif.h"
#include "netif/tunif.h"
#include "netif/unixif.h"
#include "netif/dropif.h"
#include "netif/pcapif.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbtypes.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "common/mbportlayer.h"

/* ----------------------- Defines ------------------------------------------*/
#define CLIENT_RETRIES              10
#define CLIENT_HOSTNAME             "192.168.0.1"
#define CLIENT_PORT                 1502

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Global variables ---------------------------------*/
unsigned char   debug_flags;

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/

static void modbus_thread( void *arg );

/* (manual) host IP configuration */
static ip_addr_t ipaddr, netmask, gw;
static struct netif netif;


/* ----------------------- Start implementation -----------------------------*/
static void
init_netifs( void )
{
    netif_set_default( netif_add( &netif, &ipaddr, &netmask, &gw, NULL, tapif_init, tcpip_input ) );
    netif_set_up( &netif );
}


static void
tcpip_init_done( void *arg )
{
    sys_sem_t      *sem;
    sem = ( sys_sem_t * ) arg;

    init_netifs(  );

    sys_sem_signal( sem );
}

static void
main_thread( void *arg )
{
    sys_sem_t       sem;
    netif_init(  );
    if( sys_sem_new( &sem, 0 ) != ERR_OK )
    {
        LWIP_ASSERT( "Failed to create semaphore", 0 );
    }
    tcpip_init( tcpip_init_done, &sem );
    sys_sem_wait( &sem );
    printf( "TCP/IP initialized.\n" );
    sys_thread_new( "modbus_thread", modbus_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO );
    sys_sem_wait( &sem );
}

static void
modbus_thread( void *arg )
{
    eMBErrorCode    eStatus, eStatus2;
    xMBHandle       xMBMMaster;
    USHORT          usNRegs[5];
    USHORT          usRegCnt = 0;
    UBYTE           ubIdx;
    int             iPolls = 100;

	 if( MB_ENOERR == ( eStatus = eMBMTCPInit( &xMBMMaster ) ) )
	    {
			eStatus = MB_EIO;
			while( iPolls-- > 0 )
			{
				if( MB_ENOERR != eStatus )
				{
					/* First disconnect - Function is safe even when not connected. */
					( void )eMBMTCPDisconnect( xMBMMaster );
					eStatus = eMBMTCPConnect( xMBMMaster, CLIENT_HOSTNAME, CLIENT_PORT );
				}
				if( MB_ENOERR == eStatus )
				{
	                /* Write an incrementing counter to register address 0. */
	                if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usRegCnt++ ) ) )
	                {
	                    eStatus = eStatus2;
	                }

	                /* Read holding register from adress 5 - 10, increment them by one and store
	                 * them at address 10.
	                 */
	                if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 5, 5, usNRegs ) ) )
	                {
	                    eStatus = eStatus2;
	                }
	                for( ubIdx = 0; ubIdx < 5; ubIdx++ )
	                {
	                    usNRegs[ubIdx]++;
	                }
	                if( MB_ENOERR != ( eStatus2 = eMBMWriteMultipleRegisters( xMBMMaster, 1, 10, 5, usNRegs ) ) )
	                {
	                    eStatus = eStatus2;
	                }
				}
				fprintf( stderr, "poll cycle: %s\n", eStatus == MB_ENOERR ? "okay" : "failed" );
				sleep( 1 );
			}
	    }
		else
		{
			fprintf( stderr, "TCP init failed with error = %d\n", eStatus );
		}
	    if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )
	    {
	        MBP_ASSERT( 0 );
	    }
}


int
main( int argc, char *argv[] )
{
    /* startup defaults (may be overridden by one or more opts) */
    IP4_ADDR( &gw, 192, 168, 0, 1 );
    IP4_ADDR( &netmask, 255, 255, 255, 0 );
    IP4_ADDR( &ipaddr, 192, 168, 0, 2 );
    debug_flags = LWIP_DBG_OFF;

    vMBPOtherDLLInit(  );
    sys_thread_new( "main_thread", main_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO );

    pause(  );

    vMBPOtherDLLClose(  );
    return 0;
}
