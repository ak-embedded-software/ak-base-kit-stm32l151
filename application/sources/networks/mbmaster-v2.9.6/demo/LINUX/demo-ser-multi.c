/* 
 * MODBUS Library: Example applicaion for LINUX/CYGWIN
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: demo-ser-multi.c,v 1.2 2009-10-20 21:16:59 embedded-solutions.anovak Exp $
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

#define MASTER1_PORT	              ( 2 )
#define MASTER1_BAUDRATE              ( 19200 )
#define MASTER1_PARITY                ( MB_PAR_NONE )

#define MASTER2_PORT	              ( 5 )
#define MASTER2_BAUDRATE              ( 38400 )
#define MASTER2_PARITY                ( MB_PAR_NONE )

/* ----------------------- Type definitions ---------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
STATIC pthread_mutex_t xCritSection = PTHREAD_MUTEX_INITIALIZER;
STATIC struct
{

    BOOL            bMaster1Okay;

    BOOL            bMaster2Okay;

    BOOL            bFinished;
} xThreadData;


/* ----------------------- Static functions ---------------------------------*/
STATIC void    *pvHandlerThread1( void *lpParameter );

STATIC void    *pvHandlerThread2( void *lpParameter );


/* ----------------------- Start implementation -----------------------------*/
int
main( int argc, char *argv[] )
{

    pthread_t       xThread1Hdl;

    pthread_t       xThread2Hdl;

    char            cCmd;


    int             iThread1, iThread2;


    xThreadData.bMaster1Okay = FALSE;

    xThreadData.bMaster2Okay = FALSE;

    xThreadData.bFinished = FALSE;

    vMBPOtherDLLInit(  );

    iThread1 = pthread_create( &xThread1Hdl, NULL, pvHandlerThread1, NULL );

    iThread2 = pthread_create( &xThread2Hdl, NULL, pvHandlerThread2, NULL );

    if( ( iThread1 < 0 ) || ( iThread2 < 0 ) )


    {

    }


    else


    {


        do


        {

            printf( ">" );

            cCmd = getchar(  );

            if( 'q' == cCmd )


            {

                assert( 0 == pthread_mutex_lock( &xCritSection ) );

                xThreadData.bFinished = TRUE;

                assert( 0 == pthread_mutex_unlock( &xCritSection ) );

            }

            if( 's' == cCmd )


            {

                assert( 0 == pthread_mutex_lock( &xCritSection ) );

                printf( "MASTER 1 RUNNING: %d\tMASTER 2 RUNNING: %d\n", xThreadData.bMaster1Okay, xThreadData.bMaster2Okay );

                assert( 0 == pthread_mutex_unlock( &xCritSection ) );

            }

        }

        while( !xThreadData.bFinished );

    }

    assert( 0 == pthread_mutex_lock( &xCritSection ) );

    xThreadData.bFinished = TRUE;

    assert( 0 == pthread_mutex_unlock( &xCritSection ) );

    if( iThread1 == 0 )


    {

        assert( 0 == pthread_join( xThread1Hdl, NULL ) );

    }

    if( iThread2 == 0 )


    {

        assert( 0 == pthread_join( xThread2Hdl, NULL ) );

    }

    vMBPOtherDLLClose(  );

    return 0;
}

STATIC void    *
pvHandlerThread1( void *pvArg )
{

    xMBHandle       xMBMMaster;

    BOOL            bIsRunning;

    eMBErrorCode    eStatus, eStatus2;

    USHORT          usLoopCnt = 0;

    USHORT          usNRegs[1];


    do


    {

        if( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MASTER1_PORT, MASTER1_BAUDRATE, MASTER1_PARITY ) ) )


        {


            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usLoopCnt++ ) ) )


            {

                eStatus = eStatus2;

            }

            if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 1, sizeof( usNRegs ) / sizeof( usNRegs[0] ), usNRegs ) ) )


            {

                eStatus = eStatus2;

            }

            if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )


            {

                MBP_ASSERT( 0 );

            }

        }


        /* Wait 50ms before next try. */
        sleep( 50 );

        assert( 0 == pthread_mutex_lock( &xCritSection ) );

        bIsRunning = !xThreadData.bFinished;

        xThreadData.bMaster1Okay = eStatus == MB_ENOERR ? TRUE : FALSE;

        assert( 0 == pthread_mutex_unlock( &xCritSection ) );

    }

    while( bIsRunning );

    pthread_exit( NULL );

    return NULL;
}

STATIC void    *
pvHandlerThread2( void *pvArg )
{

    xMBHandle       xMBMMaster;

    eMBErrorCode    eStatus, eStatus2;

    USHORT          usLoopCnt = 0;

    USHORT          usNRegs[1];

    BOOL            bIsRunning;


    do


    {

        if( MB_ENOERR == ( eStatus = eMBMSerialInit( &xMBMMaster, MB_RTU, MASTER2_PORT, MASTER2_BAUDRATE, MASTER2_PARITY ) ) )


        {


            /* Write an incrementing counter to register address 0. */
            if( MB_ENOERR != ( eStatus2 = eMBMWriteSingleRegister( xMBMMaster, 1, 0, usLoopCnt++ ) ) )


            {

                eStatus = eStatus2;

            }

            if( MB_ENOERR != ( eStatus2 = eMBMReadHoldingRegisters( xMBMMaster, 1, 1, sizeof( usNRegs ) / sizeof( usNRegs[0] ), usNRegs ) ) )


            {

                eStatus = eStatus2;

            }

            if( MB_ENOERR != ( eStatus = eMBMClose( xMBMMaster ) ) )


            {

                MBP_ASSERT( 0 );

            }

        }


        /* Wait 50ms before next try. */
        sleep( 50 );

        assert( 0 == pthread_mutex_lock( &xCritSection ) );

        bIsRunning = !xThreadData.bFinished;

        xThreadData.bMaster2Okay = eStatus == MB_ENOERR ? TRUE : FALSE;

        assert( 0 == pthread_mutex_unlock( &xCritSection ) );

    }

    while( bIsRunning );

    pthread_exit( NULL );

    return NULL;
}
