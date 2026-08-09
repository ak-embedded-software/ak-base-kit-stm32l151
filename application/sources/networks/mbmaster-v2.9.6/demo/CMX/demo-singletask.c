/* 
 * MODBUS Library: CMX demo application.
 * Copyright (c) 2008 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-singletask.c,v 1.1 2008-08-27 17:56:08 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "cxfuncs.h"

/* ----------------------- Platform includes --------------------------------*/
#include "m523xevb.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbport.h"
#include "mbm.h"

/* ----------------------- Defines ------------------------------------------*/
#define MCF_PIT_PRESCALER               512UL
#define MCF_PIT_TIMER_TICKS             ( SYSTEM_CLOCK * 1000000UL / MCF_PIT_PRESCALER )
#define MCF_PIT_MODULUS_REGISTER(freq)  ( MCF_PIT_TIMER_TICKS / ( freq ) - 1UL)
#define MODBUS_TASK_STACKSIZE           ( 1024 )

/* ----------------------- Functions ----------------------------------------*/
void            prvvTimerExpiredCB( void );

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
byte            g_bMODBUSTaskSlot;
static byte     bApplTaskSlot;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

void            vMBMWriteSingleRegisterPolled( xMBMHandle xHdl, UCHAR ucSlaveAddress,
                                               USHORT usRegAddress, USHORT usValue,
                                               /*@out@ */ eMBMQueryState * peState,
                                               /*@out@ */ eMBErrorCode * peStatus );

void
vMODBUSTask( void )
{
    eMBErrorCode    eStatus;
    eMBMQueryState  eState;
    xMBHandle       xMBMMaster;
    USHORT          usRegCnt = 0;

    /* Preinitialize serial task to allocate CMX task ids. */
    if( MB_ENOERR != vMBSerialInit(  ) )
    {
        MBP_ASSERT( 0 );
    }
    do
    {
        if( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, 0, 38400, MB_PAR_NONE ) ) )
        {
            do
            {
                usRegCnt++;
                eState = MBM_STATE_NONE;
                do
                {
                    vMBMWriteSingleRegisterPolled( xMBMMaster, 1, 0, usRegCnt, &eState, &eStatus );
                    prvvSerHandlerTask(  );
                }
                while( eState != MBM_STATE_DONE );
            }
            while( TRUE );
        }
        else
        {
            ( void )eMBMClose( xMBMMaster );
        }
        /* Wait some time before trying to reopen the serial port. */
        ( void )K_Task_Wait( 100 );
    }
    while( TRUE );

    K_Task_End(  );
}

static void
vInitHW( void )
{
    /* Configure prescaler */
    MCF_PIT_PCSR0 = MCF_PIT_PCSR_PRE( 0x9 ) | MCF_PIT_PCSR_RLD | MCF_PIT_PCSR_OVW;
    /* Initialize the periodic timer interrupt. */
    MCF_PIT_PMR0 = MCF_PIT_MODULUS_REGISTER( 1000 );    /* 146 */
    /* Configure interrupt priority and level and unmask interrupt. */
    MCF_INTC0_ICR36 = MCF_INTC0_ICRn_IL( 0x2 ) | MCF_INTC0_ICRn_IP( 0x0 );
    MCF_INTC0_IMRH &= ~( MCF_INTC0_IMRH_INT_MASK36 );
    MCF_INTC0_IMRL &= ~( MCF_INTC0_IMRL_MASKALL );
    /* Enable interrupts */
    MCF_PIT_PCSR0 |= MCF_PIT_PCSR_PIE | MCF_PIT_PCSR_EN | MCF_PIT_PCSR_PIF;
}

int
main( void )
{
    vInitHW(  );
    K_OS_Init(  );

    if( K_OK != K_Task_Create( MB_MODBUS_TASK_PRIORITY, &g_bMODBUSTaskSlot, vMODBUSTask, MODBUS_TASK_STACKSIZE ) )
    {

    }
    else if( K_OK != K_Task_Start( g_bMODBUSTaskSlot ) )
    {

    }
    else
    {
        K_OS_Slice_On(  );
        K_OS_Start(  );
    }
    MBP_ASSERT( 0 );
    return 0;
}

void
vCMXTimerTick( void )
{
    /* ack PIT interrupt */
    K_OS_Tick_Update(  );
    MCF_PIT_PCSR0 |= MCF_PIT_PCSR_PIF;
}
