/*
 * MODBUS Libary: uC3/uNET port
 * Copyright (c) 2014 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: template.c,v 1.1 2007-08-19 12:31:23 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>
#include "kernel.h"
#include "kernel_id.h"
#include "hw_dep.h"
#include "net_hdr.h"
#include "net_id.h"

/* ----------------------- Platform includes --------------------------------*/

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbport.h"
#include "mbm.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBS_LISTEN_ADDRESS              "0.0.0.0"       /* Bind on all addresses */
#define MBS_LISTEN_PORT                 502

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
static int      modbus_initialized;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
/* This is a simple task where a MODBUS master instance is created. First
 * the stack is initialized by eMBMTCPInit. Once this is done this instance
 * can be used to connect/disconnect to multiple slaves and polls there
 * data.
 *
 * This is a DEMO code only and actual implementation code should not
 * call connect/disconnect/init/close that often but ony reconnect in
 * case of a network error.
 */
void
task_mbmaster_impl( void )
{
    xMBMHandle      xMBMHdl;
    eMBErrorCode    eStatus;
    USHORT          usNRegs[25];
    int             errorCnt = 0;
	int				connectLoops;
    int             pollLoops;
    /* Wait till modbus_poll has been called at least once such that initialization
     * has finished.
     */
	while( !modbus_initialized )
    {
        dly_tsk( 10 );
    }

    for( ;; )
    {

        if( MB_ENOERR != ( eStatus = eMBMTCPInit( &xMBMHdl ) ) )
        {
            /* Can not initialize stack. Wait some time to retry in case
             * it was a ressource issue.
             */
            dly_tsk( 10 );
        }
        else
        {
		  	for( connectLoops = 0; connectLoops < 10; connectLoops++ )
			{
				/* Connect to MODBUS TCP slave */
				if( MB_ENOERR == ( eStatus = eMBMTCPConnect( xMBMHdl, "172.16.0.1", 502 ) ) )
				{
					for( pollLoops = 0; pollLoops < 100; pollLoops++ )
					{
						/* Try to read 20 input registers from MODBUS TCP slave using address 255 */
						eStatus = eMBMReadHoldingRegisters( xMBMHdl, 255, 0, 20, usNRegs );
						if( MB_ENOERR != eStatus )
						{
							errorCnt++;
							/* The request has failed */
						}
					}

					/* Disconnect from the slave as TCP connection no longer needed. */
					eStatus = eMBMTCPDisconnect( xMBMHdl );
				}
			}

            /* The close shall never fail if the instance was opened correctly. Assert on this
             * in debug build.
             */
            eStatus = eMBMClose( xMBMHdl );
            MBP_ASSERT( MB_ENOERR == eStatus );
        }
    }
}

void
mbstack_poll( void )
{
    ER              ercd;
    static int      bFirstStart = 1;

    /* The poll function has been called the first time. We assume that the
     * hardware is ready and startup the master and the slave tasks.
     */
    if( bFirstStart )
    {
        vMBPInit(  );
        ercd = act_tsk( ID_TASK_MBMASTER );
        MBP_ASSERT( E_OK == ercd );
        bFirstStart = 0;
        modbus_initialized = 1;
    }

    vMBPTimerPoll(  );
    vMBTCPClientPoll(  );
}