/* 
 * MODBUS Library: Port testing utility
 * Copyright (c) 2007 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbmporttest.c,v 1.7 2007-09-11 22:23:28 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbm.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "mbmporttest.h"

/* ----------------------- Defines ------------------------------------------*/
#define MBM_PORT_TEST_SERIAL_PORT       ( 0 )
#define MBM_PORT_TEST_SERIAL_BAUDRATE   ( 38400 )
#define MBM_PORT_TEST_SERIAL_PARITY     ( MB_PAR_ODD )
#define MBM_PORT_TEST_SERIAL_DATABITS   ( 8 )
#define MBM_PORT_TEST_SERIAL_STOPBITS   ( 1 )

#define MBM_PORT_TEST_EVENT_RECV_OKAY   ( 1 )
#define MBM_PORT_TEST_EVENT_SENT        ( 2 )

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_INIT,
    STATE_WAIT_STRING,
    STATE_SENDING,
    STATE_DONE
} eTestState;

/* ----------------------- Static variables ---------------------------------*/
xMBPSerialHandle xSerHdl;
xMBPEventHandle xEvHdl1;
xMBPTimerHandle xTmr1Hdl;
xMBPTimerHandle xTmr2Hdl;

xMBHandle       xMBMHdl;

UBYTE           ubPatternRcv[] = { 't', 'e', 's', 't' };
UBYTE           ubPatternRcvCurPos;

UBYTE           ubPatternSnd[] = { 'o', 'k', 'a', 'y' };
UBYTE           ubPatternSndCurPos;
BYTE            bCycle;

/* ----------------------- Static functions ---------------------------------*/
STATIC void     vMBPTestAssert( void );

STATIC void     vMBPTestRunBasic( void );

BOOL
bMBMTmr1ExpiredCB( xMBHandle xHdl )
{
    ubPatternRcvCurPos = 0;
    return TRUE;
}

BOOL
bMBMTmr2ExpiredCB( xMBHandle xHdl )
{
    if( 0 == bCycle )
    {
        if( MB_ENOERR != eMBPTimerSetTimeout( xTmr2Hdl, 2000 ) )
        {
            vMBPTestAssert(  );
        }
        else if( MB_ENOERR != eMBPTimerStart( xTmr2Hdl ) )
        {
            vMBPTestAssert(  );
        }
        else
        {
            vMBPTestDebugLED( MBM_PORT_DEBUG_LED_WORKING, TRUE );
        }
        bCycle++;
    }
    else
    {
        if( MB_ENOERR != eMBPTimerSetTimeout( xTmr2Hdl, 1000 ) )
        {
            vMBPTestAssert(  );
        }
        else if( MB_ENOERR != eMBPTimerStart( xTmr2Hdl ) )
        {
            vMBPTestAssert(  );
        }
        else
        {
            vMBPTestDebugLED( MBM_PORT_DEBUG_LED_WORKING, FALSE );
            bCycle = 0;
        }
    }
    ubPatternRcvCurPos = 0;
    return TRUE;
}


BOOL
bMBMSerialTransmitterEmptyCB( xMBHandle xHdl, UBYTE * pubValue )
{
    BOOL            bHasData = FALSE;

    if( ubPatternSndCurPos < MB_UTILS_NARRSIZE( ubPatternSnd ) )
    {
        *pubValue = ubPatternSnd[ubPatternSndCurPos++];
        bHasData = TRUE;
    }
    else
    {
        /* Okay - Everything sent. */
        if( MB_ENOERR != eMBPEventPost( xEvHdl1, MBM_PORT_TEST_EVENT_SENT ) )
        {
            vMBPTestAssert(  );
        }
    }

    return bHasData;
}

void
vMBMSerialReceiverCB( xMBHandle xHdl, UBYTE ubValue )
{
    BOOL            bRestartTimer;

    if( ubPatternRcv[ubPatternRcvCurPos] == ubValue )
    {
        ubPatternRcvCurPos++;

        if( MB_UTILS_NARRSIZE( ubPatternRcv ) == ubPatternRcvCurPos )
        {
            bRestartTimer = FALSE;
            if( MB_ENOERR != eMBPSerialRxEnable( xSerHdl, NULL ) )
            {
                vMBPTestAssert(  );
            }
            if( MB_ENOERR != eMBPEventPost( xEvHdl1, MBM_PORT_TEST_EVENT_RECV_OKAY ) )
            {
                vMBPTestAssert(  );
            }
        }
        else
        {
            bRestartTimer = TRUE;
        }
    }
    else
    {
        /* Invalid character in sequence. Back to the beginning. */
        bRestartTimer = FALSE;
        ubPatternRcvCurPos = 0;
    }

    if( bRestartTimer )
    {
        if( MB_ENOERR != eMBPTimerStart( xTmr1Hdl ) )
        {
            vMBPTestAssert(  );
        }
    }
}

/* ----------------------- Start implementation -----------------------------*/
void
vMBPTestRun( void )
{
    vMBPTestRunBasic(  );
}

STATIC void
vMBPTestAssert( void )
{
    volatile BOOL   bExit = FALSE;

    MBP_ENTER_CRITICAL_SECTION(  );
    do
    {
        vMBPTestDebugLED( MBM_PORT_DEBUG_LED_ERROR, TRUE );
    }
    while( !bExit );
}

STATIC void
vMBPTestRunBasic( void )
{
    eTestState      eState = STATE_INIT;
    BOOL            bFinished = FALSE;
    eMBErrorCode    eStatus;
    xMBPEventType   xEvent;

    ubPatternRcvCurPos = 0;
    ubPatternSndCurPos = 0;

    xSerHdl = MBP_SERIALHDL_INVALID;
    xTmr1Hdl = MBP_TIMERHDL_INVALID;
    xTmr2Hdl = MBP_TIMERHDL_INVALID;
    xEvHdl1 = MBP_EVENTHDL_INVALID;

    /* Make sure that the handle is set to something because the API functions
     * must check if the handle equals MBM_HDL_INVALID.
     */
    xMBMHdl = &xMBMHdl;

    do
    {
        switch ( eState )
        {
        case STATE_INIT:
            if( MB_ENOERR != eMBPSerialInit( &xSerHdl, MBM_PORT_TEST_SERIAL_PORT,
                                             MBM_PORT_TEST_SERIAL_BAUDRATE,
                                             MBM_PORT_TEST_SERIAL_DATABITS,
                                             MBM_PORT_TEST_SERIAL_PARITY, MBM_PORT_TEST_SERIAL_STOPBITS, xMBMHdl ) )
            {
                vMBPTestAssert(  );
            }
            else if( MB_ENOERR != eMBPEventCreate( &xEvHdl1 ) )
            {
                vMBPTestAssert(  );
            }
            else if( MB_ENOERR != eMBPTimerInit( &xTmr1Hdl, 1000, &bMBMTmr1ExpiredCB, xMBMHdl ) )
            {
                vMBPTestAssert(  );
            }
            else if( MB_ENOERR != eMBPTimerInit( &xTmr2Hdl, 1000, &bMBMTmr2ExpiredCB, xMBMHdl ) )
            {
                vMBPTestAssert(  );
            }
            else if( MB_ENOERR != eMBPTimerStart( xTmr2Hdl ) )
            {
                vMBPTestAssert(  );
            }
            else if( MB_ENOERR != eMBPSerialRxEnable( xSerHdl, vMBMSerialReceiverCB ) )
            {
                vMBPTestAssert(  );
            }
            else
            {
                eState = STATE_WAIT_STRING;
            }
            break;

        case STATE_WAIT_STRING:
            if( bMBPEventGet( xEvHdl1, &xEvent ) )
            {
                if( MBM_PORT_TEST_EVENT_RECV_OKAY != xEvent )
                {
                    vMBPTestAssert(  );
                }
                else
                {
                    if( MB_ENOERR != eMBPSerialTxEnable( xSerHdl, bMBMSerialTransmitterEmptyCB ) )
                    {
                        vMBPTestAssert(  );
                    }
                    else
                    {
                        eState = STATE_SENDING;
                    }
                }
            }
            break;

        case STATE_SENDING:
            if( bMBPEventGet( xEvHdl1, &xEvent ) )
            {
                if( MBM_PORT_TEST_EVENT_SENT != xEvent )
                {
                    vMBPTestAssert(  );
                }
                else
                {
                    eState = STATE_DONE;
                }
            }
            break;

        case STATE_DONE:
            do
            {
                if( MB_ENOERR != ( eStatus = eMBPSerialClose( xSerHdl ) ) )
                {
                    /* We must wait for the transmitter to finish. */
                    if( MB_EAGAIN != eStatus )
                    {
                        vMBPTestAssert(  );
                    }
                }
            }
            while( MB_ENOERR != eStatus );
            MBP_ENTER_CRITICAL_SECTION(  );
            vMBPEventDelete( xEvHdl1 );
            vMBPTimerClose( xTmr1Hdl );
            vMBPTimerClose( xTmr2Hdl );
            MBP_EXIT_CRITICAL_SECTION(  );
            bFinished = TRUE;
            break;

        default:
            vMBPTestAssert(  );
        }
    }
    while( !bFinished );
}
