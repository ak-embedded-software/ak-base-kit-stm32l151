/* 
 * MODBUS Library: CMX Port
 * Copyright (c) 2008 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * $Id: mbportserial.c,v 1.4 2008-09-01 18:43:42 cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "cxfuncs.h"
#include "m523xevb.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"
#include "serial.h"

/* ----------------------- Defines ------------------------------------------*/
#define BAUDRATE_VALUE( ulFSYS, ulBaud ) \
                                        ( ( ulFSYS )/( 32UL * ulBaud ) )
#define IDX_INVALID                     ( 255 )
#define UART_BAUDRATE_MIN               ( 300 )
#define UART_BAUDRATE_MAX               ( 115200 )
#define SER_MAX_INSTANCES               ( 1 )
#define SER_BUFFER_SIZE                 ( 32 )
#define SER_TASK_STACKSIZE              ( 384 )

#if MB_USE_SINGLETASK
#define SER_EVENT_TIMEOUT               ( 10 )
#else
#define SER_EVENT_TIMEOUT               ( 100 )
#endif
#define HDL_RESET( x ) do { \
	( x )->ubIdx = IDX_INVALID; \
	( x )->pbMBPTransmitterEmptyFN = NULL; \
	( x )->pvMBPReceiveFN = NULL; \
	( x )->xMBMHdl = MB_HDL_INVALID; \
    ( x )->bIsRunning = FALSE; \
} while( 0 );

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    pbMBPSerialTransmitterEmptyAPIV2CB pbMBPTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV2CB pvMBPReceiveFN;
    BOOL            bIsRunning;
    xMBHandle       xMBMHdl;
} xSerialHandle;

/* ----------------------- Global variables ---------------------------------*/
byte            g_bSerialTaskSlot;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialHandle xSerialHdls[SER_MAX_INSTANCES];

/* ----------------------- Static functions ---------------------------------*/
#if MB_USE_SINGLETASK
void            prvvSerHandlerTask( void );
#else
STATIC void     prvvSerHandlerTask( void );
#endif

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
vMBSerialInit( void )
{
    UBYTE           ubIdx;
    eMBErrorCode    eStatus = MB_EPORTERR;

    MBP_ENTER_CRITICAL_SECTION(  );
    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
    {
        HDL_RESET( &xSerialHdls[ubIdx] );
    }
    if( K_OK != K_Task_Create( MB_SERIAL_TASK_PRIORITY, &g_bSerialTaskSlot, prvvSerHandlerTask, SER_TASK_STACKSIZE ) )
    {

    }
#if !MB_USE_SINGLETASK
    else if( K_OK != K_Task_Start( g_bSerialTaskSlot ) )
    {
        K_Task_Delete( g_bSerialTaskSlot );
    }
#endif
    else
    {
#if MB_USE_SINGLETASK
        g_bSerialTaskSlot = g_bMODBUSTaskSlot;
#endif
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBMHdl )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UBYTE           ubIdx;
    xSerialHandle  *pxSerHdl = NULL;


    ( void )ucPort;

    MBP_ENTER_CRITICAL_SECTION(  );
    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
    {
        if( IDX_INVALID == xSerialHdls[ubIdx].ubIdx )
        {
            pxSerHdl = &( xSerialHdls[ubIdx] );
            HDL_RESET( pxSerHdl );
            pxSerHdl->ubIdx = ubIdx;
            break;
        }
    }

    if( NULL != pxSerHdl )
    {
        vMBPSerialUART0Init( ulBaudRate, ucDataBits, eParity, ucStopBits );
        pxSerHdl->xMBMHdl = xMBMHdl;
        pxSerHdl->bIsRunning = TRUE;
        *pxSerialHdl = pxSerHdl;
        eStatus = MB_ENOERR;
    }
    else
    {
        eStatus = MB_ENORES;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialClose( xMBPSerialHandle xSerialHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( ( NULL != pxSerialIntHdl ) && MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        pxSerialIntHdl->bIsRunning = FALSE;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialTxEnable( xMBPSerialHandle xSerialHdl, pbMBPSerialTransmitterEmptyCB pbMBPTransmitterEmptyFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;
    byte            bStatus;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pbMBPTransmitterEmptyFN )
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = pbMBPTransmitterEmptyFN;
            bStatus = K_Event_Signal( 0, g_bSerialTaskSlot, MB_SERTX_EVENT );
            MBP_ASSERT( K_OK == bStatus );
        }
        else
        {
            pxSerialIntHdl->pbMBPTransmitterEmptyFN = NULL;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialRxEnable( xMBPSerialHandle xSerialHdl, pvMBPSerialReceiverCB pvMBPReceiveFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
        eStatus = MB_ENOERR;
        if( NULL != pvMBPReceiveFN )
        {
            pxSerialIntHdl->pvMBPReceiveFN = pvMBPReceiveFN;
        }
        else
        {
            pxSerialIntHdl->pvMBPReceiveFN = NULL;
        }
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

#if MB_USE_SINGLETASK
void
#else
STATIC void
#endif
prvvSerHandlerTask( void )
{
    word16          wEvent;
    pbMBPSerialTransmitterEmptyAPIV2CB pbMBPTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV2CB pvMBPReceiveFN;
    UBYTE           arubBuffer[SER_BUFFER_SIZE];
    USHORT          usReceivedBytes;
    USHORT          usBytesToSend;
    byte            bStatus;
    UBYTE           ubIdx;
    unsigned short  usTmrEventMask;

    do
    {
        /* Build the timer event mask. Timer events are handled at the bottom
         * of this function.
         */
        usTmrEventMask = 0;
        for( ubIdx = 0; ubIdx < MB_TIMERS_COUNT; ubIdx++ )
        {
            usTmrEventMask |= 1 << ( MB_TIMERS_EVENT_IDX_FIRST + ( unsigned short )ubIdx );
        }

        /* We do not want to wait indefinetly because we need to poll
         * data from the serial porting layer.
         */
        wEvent =
            K_Event_Wait( ( unsigned short )( usTmrEventMask | MB_SERRX_EVENT | MB_SERTX_EVENT ), SER_EVENT_TIMEOUT,
                          2 );

        /* First we check all serial devices to check if some data
         * has arrived in the meantime. If yes we process this first
         * since this might reset a timer.
         */
        MBP_ENTER_CRITICAL_SECTION(  );
        if( xSerialHdls[0].bIsRunning )
        {
            pvMBPReceiveFN = xSerialHdls[0].pvMBPReceiveFN;
            pbMBPTransmitterEmptyFN = xSerialHdls[0].pbMBPTransmitterEmptyFN;
            MBP_EXIT_CRITICAL_SECTION(  );
            /* We do not check if the event is acutally set because we have
             * or own status flag if we want to handle the receiver.
             */
            if( NULL != pvMBPReceiveFN )
            {
                usReceivedBytes = vMBPSerialUART0Receive( arubBuffer, SER_BUFFER_SIZE );
                if( usReceivedBytes > 0 )
                {
                    MBP_ENTER_CRITICAL_SECTION(  );
                    if( NULL != xSerialHdls[0].pvMBPReceiveFN )
                    {
                        xSerialHdls[0].pvMBPReceiveFN( xSerialHdls[0].xMBMHdl, arubBuffer, usReceivedBytes );
                    }
                    MBP_EXIT_CRITICAL_SECTION(  );
                }
            }
            /* We do not check if the event is acutally set because we have
             * or own status flag if we want to handle the transmitter.
             */
            if( NULL != pbMBPTransmitterEmptyFN )
            {
                K_Event_Reset( g_bSerialTaskSlot, MB_SERTX_EVENT );
                MBP_ENTER_CRITICAL_SECTION(  );
                if( NULL != xSerialHdls[0].pbMBPTransmitterEmptyFN )
                {
                    if( !xSerialHdls[0].
                        pbMBPTransmitterEmptyFN( xSerialHdls[0].xMBMHdl, arubBuffer, SER_BUFFER_SIZE, &usBytesToSend ) )
                    {
                        xSerialHdls[0].pbMBPTransmitterEmptyFN = NULL;
                        usBytesToSend = 0;
                    }
                    else
                    {
                        bStatus = K_Event_Signal( 0, g_bSerialTaskSlot, MB_SERTX_EVENT );
                        MBP_ASSERT( K_OK == bStatus );
                    }
                    MBP_EXIT_CRITICAL_SECTION(  );
                    if( usBytesToSend > 0 )
                    {
                        if( !bMBPSerialUART0Send( arubBuffer, usBytesToSend ) )
                        {
                            MBP_ASSERT( 0 );
                        }
                    }

                }
                else
                {
                    MBP_EXIT_CRITICAL_SECTION(  );
                }

            }
        }
        else
        {
            MBP_EXIT_CRITICAL_SECTION(  );
        }

        /* Now handle the timers. */
        usTmrEventMask = 0;
        for( ubIdx = 0; ubIdx < MB_TIMERS_COUNT; ubIdx++ )
        {
            usTmrEventMask = ( unsigned short )( 1 << ( MB_TIMERS_EVENT_IDX_FIRST + ( unsigned short )ubIdx ) );
            if( usTmrEventMask & wEvent )
            {
                prvvTimerHandler( ubIdx );
            }
        }
    }
#if MB_USE_SINGLETASK
    while( 0 );
#else
    while( TRUE );
    K_Task_End(  );
#endif
}
