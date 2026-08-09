/* 
 * MODBUS Library: LINUX/CYGWIN serial port
 * Copyright (c) 2009-2015 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbportserial.c,v 1.8 2014-08-23 09:36:06 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <termios.h>
#include <time.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <semaphore.h>

/* ----------------------- Platform includes --------------------------------*/
#include "mbport.h"
#if defined( MBP_USE_LOW_LATENCY_IO ) && ( MBP_USE_LOW_LATENCY_IO == 1 )
#include <linux/serial.h>
#endif

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/
#define DEMO_BUILD                      ( 0 )
#define IDX_INVALID                     ( 255 )
#define UART_BAUDRATE_MIN               ( 9600 )
#define SER_MAX_INSTANCES               ( 8 )
#define SER_BUFFER_SIZE                 ( 256 )
#define SER_TIMEOUT                     ( 5UL )
#define THREAD_STACKSIZE_MAX            ( 16384 )

#define HDL_RESET( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
    ( x )->pbMBPSerialTransmitterEmptyFN = NULL; \
    ( x )->pvMBPSerialReceiverFN = NULL; \
    ( x )->pbMBPFrameTimeoutFN = NULL; \
    ( x )->eMode = MB_RTU; \
    ( x )->bIsRunning = FALSE; \
    memset( &( ( x )->xThreadHdl ), 0, sizeof( pthread_t ) ); \
    memset( &( ( x )->xIdleSem ), 0, sizeof( sem_t ) ); \
    ( x )->xSerHdl = -1; \
    ( x )->xMBHdl = MB_HDL_INVALID; \
  } while( 0 )

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    pbMBPSerialTransmitterEmptyAPIV2CB pbMBPSerialTransmitterEmptyFN;
    pvMBPSerialReceiverAPIV2CB pvMBPSerialReceiverFN;
    pbMBPTimerExpiredCB pbMBPFrameTimeoutFN;
    eMBSerialMode   eMode;
    BOOL            bIsRunning;
    pthread_t       xThreadHdl;
    sem_t           xIdleSem;
    int             xSerHdl;
    xMBHandle       xMBHdl;
} xSerialHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xSerialHandle xSerialHdls[SER_MAX_INSTANCES];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/
STATIC void    *pvMBPSerialHandlerThread( void *pvArg );

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPSerialInit( xMBPSerialHandle * pxSerialHdl, UCHAR ucPort, ULONG ulBaudRate,
                UCHAR ucDataBits, eMBSerialParity eParity, UCHAR ucStopBits, xMBHandle xMBHdl, pbMBPTimerExpiredCB pbFrameTimeoutFN, eMBSerialMode eMode )
{
    UBYTE           ubIdx;
    CHAR            arcDevice[16];
    eMBErrorCode    eStatus = MB_EINVAL;
    struct termios  xNewTIO;
    speed_t         xNewSpeed;
    pthread_attr_t  xThreadAttr;
    xSerialHandle  *pxSerHdl = NULL;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( !bIsInitalized )
    {
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( xSerialHdls ); ubIdx++ )
        {
            HDL_RESET( &xSerialHdls[ubIdx] );
        }
        bIsInitalized = TRUE;
    }

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
        memset( &xNewTIO, 0, sizeof( struct termios ) );
        xNewTIO.c_iflag |= IGNBRK | INPCK;
        xNewTIO.c_cflag |= CREAD | CLOCAL;
        xNewTIO.c_oflag = 0;
        xNewTIO.c_lflag = 0;
        switch ( eParity )
        {
        case MB_PAR_NONE:
            break;
        case MB_PAR_EVEN:
            xNewTIO.c_cflag |= PARENB;
            break;
        case MB_PAR_ODD:
            xNewTIO.c_cflag |= PARENB | PARODD;
            break;
        default:
            eStatus = MB_EINVAL;
        }

        switch ( ucDataBits )
        {
        case 8:
            xNewTIO.c_cflag |= CS8;
            break;
        case 7:
            xNewTIO.c_cflag |= CS7 | CSTOPB;
            break;
        default:
            eStatus = MB_EINVAL;
        }

        switch ( ulBaudRate )
        {
        case 9600:
            xNewSpeed = B9600;
            break;
        case 19200:
            xNewSpeed = B19200;
            break;
        case 38400:
            xNewSpeed = B38400;
            break;
        case 57600:
            xNewSpeed = B57600;
            break;
        case 115200:
            xNewSpeed = B115200;
            break;
        default:
            eStatus = MB_EINVAL;
        }

        pxSerHdl->bIsRunning = TRUE;
        pxSerHdl->xMBHdl = xMBHdl;
        pxSerHdl->pbMBPFrameTimeoutFN = pbFrameTimeoutFN;
        pxSerHdl->eMode = eMode;
        /* snprintf( arcDevice, MB_UTILS_NARRSIZE( arcDevice ), "/dev/ttyS%d", ( int )ucPort ); */
        snprintf( arcDevice, MB_UTILS_NARRSIZE( arcDevice ), "/dev/ttyUSB%d", ( int )ucPort );

        if( -1 == sem_init( &pxSerHdl->xIdleSem, 1, 0 ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't create semaphore: %s\n", arcDevice, strerror( errno ) );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        if( -1 == ( pxSerHdl->xSerHdl = open( arcDevice, O_RDWR | O_NOCTTY | O_NONBLOCK ) ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't open serial port %s: %s\n", arcDevice, strerror( errno ) );
            }
#endif

            eStatus = MB_EPORTERR;
        }
        else if( cfsetispeed( &xNewTIO, xNewSpeed ) != 0 )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't set baud rate %ld for port %s: %s\n", ulBaudRate, arcDevice, strerror( errno ) );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        else if( cfsetospeed( &xNewTIO, xNewSpeed ) != 0 )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't set baud rate %ld for port %s: %s\n", ulBaudRate, arcDevice, strerror( errno ) );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        else if( tcsetattr( pxSerHdl->xSerHdl, TCSANOW, &xNewTIO ) != 0 )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't set settings for port %s: %s\n", arcDevice, strerror( errno ) );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        else if( 0 != pthread_attr_init( &xThreadAttr ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't initialize thread attributes: %s", strerror( errno ) );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        else if( 0 != pthread_attr_setdetachstate( &xThreadAttr, PTHREAD_CREATE_DETACHED ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't set thread attributes: %s\n", strerror( errno ) );
            }
#endif
        }
        else if( 0 != pthread_create( &( pxSerHdl->xThreadHdl ), &xThreadAttr, pvMBPSerialHandlerThread, pxSerHdl ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_ERROR, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't create polling thread: %s\n", strerror( errno ) );
            }
#endif
            eStatus = MB_EPORTERR;
        }
        else
        {
            *pxSerialHdl = pxSerHdl;
            eStatus = MB_ENOERR;
        }
        ( void )pthread_attr_destroy( &xThreadAttr );

        if( MB_ENOERR != eStatus )
        {
            ( void )sem_destroy( &( pxSerHdl->xIdleSem ) );
            ( void )close( pxSerHdl->xSerHdl );
            HDL_RESET( pxSerHdl );
        }
    }
    else
    {
        eStatus = MB_ENORES;
    }

    MBP_EXIT_CRITICAL_SECTION(  );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "serial init done\n" );
    }
#endif
    return eStatus;
}

eMBErrorCode
eMBPSerialClose( xMBPSerialHandle xSerialHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
        {
            vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] Closing handle.\n", pxSerialIntHdl->ubIdx );
        }
#endif

        pxSerialIntHdl->bIsRunning = FALSE;
        eStatus = MB_ENOERR;
        ( void )sem_post( &( pxSerialIntHdl->xIdleSem ) );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPSerialTxEnable( xMBPSerialHandle xSerialHdl, pbMBPSerialTransmitterEmptyCB pbMBPTransmitterEmptyFN )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xSerialHandle  *pxSerialIntHdl = xSerialHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) && pxSerialIntHdl->bIsRunning )
    {
        eStatus = MB_ENOERR;
        if( NULL != pbMBPTransmitterEmptyFN )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] enabled transmitter callback %p\n", pxSerialIntHdl->ubIdx, pbMBPTransmitterEmptyFN );
            }
#endif

            pxSerialIntHdl->pbMBPSerialTransmitterEmptyFN = ( pbMBPSerialTransmitterEmptyAPIV2CB ) pbMBPTransmitterEmptyFN;
        }
        else
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] disabled transmitter callback\n", pxSerialIntHdl->ubIdx );
            }
#endif

            pxSerialIntHdl->pbMBPSerialTransmitterEmptyFN = NULL;
        }
        ( void )sem_post( &( pxSerialIntHdl->xIdleSem ) );
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
    if( MB_IS_VALID_HDL( pxSerialIntHdl, xSerialHdls ) && pxSerialIntHdl->bIsRunning )
    {
        eStatus = MB_ENOERR;
        if( NULL != pvMBPReceiveFN )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] enabled receiver callback %p\n", pxSerialIntHdl->ubIdx, pvMBPReceiveFN );
            }
#endif
            pxSerialIntHdl->pvMBPSerialReceiverFN = ( pvMBPSerialReceiverAPIV2CB ) pvMBPReceiveFN;
        }
        else
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] disabled receiver callback at\n", pxSerialIntHdl->ubIdx );
            }
#endif

            pxSerialIntHdl->pvMBPSerialReceiverFN = NULL;
        }
        ( void )sem_post( &( pxSerialIntHdl->xIdleSem ) );
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

void
vMBPSerialDLLClose( void )
{
    /* If this code is called we know that no MODBUS instances are
     * newly created (Because of the INIT lock) and that all
     * still running serial threads are going to shut down since
     * their bIsRunning flag has been set to FALSE.
     */
    MBP_ENTER_CRITICAL_SECTION(  );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "Unloading library\n" );
    }
#endif
    /* Nothing to do here for now. */
    MBP_EXIT_CRITICAL_SECTION(  );
}

STATIC          BOOL
prvbMBPSerialWaitReceive( int xSerHdl, eMBErrorCode * peStatus )
{
    BOOL            bHasData = FALSE;
    fd_set          rd_fds, err_fds;
    int             iRetVal;
    struct timeval  tv;

    *peStatus = MB_ENOERR;
    FD_ZERO( &rd_fds );
    FD_ZERO( &err_fds );
    FD_SET( xSerHdl, &rd_fds );
    FD_SET( xSerHdl, &err_fds );

    tv.tv_sec = 0;
    tv.tv_usec = 10000UL;
    iRetVal = select( xSerHdl + 1, &rd_fds, NULL, &err_fds, &tv );
    switch ( iRetVal )
    {
    case 0:
        break;
    case -1:
        /* select failed - this error can not be covered */
        *peStatus = MB_EIO;
        break;
    default:
        if( FD_ISSET( xSerHdl, &rd_fds ) )
        {
            bHasData = TRUE;
        }
        if( FD_ISSET( xSerHdl, &err_fds ) )
        {
            *peStatus = MB_EIO;
        }
    }
    return bHasData;
}

STATIC          eMBErrorCode
prveMBPSerialSendBytes( int xSerHdl, UBYTE arubBuffer[], USHORT usBytesToSend )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    fd_set          wr_fds, err_fds;
    int             iRetVal;
    struct timeval  tv;
    ssize_t         iBytesWritten;
    USHORT          usBytesSent = 0;

    FD_ZERO( &wr_fds );
    FD_ZERO( &err_fds );
    FD_SET( xSerHdl, &wr_fds );
    FD_SET( xSerHdl, &err_fds );

    do
    {
        /* The longest time we want to block wait for a write to become ready
         * is the size of the serial buffer size plus some guard time.
         */
        tv.tv_sec = 0;
        tv.tv_usec = SER_BUFFER_SIZE * 11UL / UART_BAUDRATE_MIN + 20000UL;
        iRetVal = select( xSerHdl + 1, NULL, &wr_fds, &err_fds, &tv );
        switch ( iRetVal )
        {
        case 0:
            /* timeout - break the loop but do not signal an error as this error
             * case is recoverable.
             */
            break;
        case -1:
            /* select failed - this error can not be covered */
            eStatus = MB_EIO;
            break;
        default:
            if( FD_ISSET( xSerHdl, &wr_fds ) )
            {
                iBytesWritten = write( xSerHdl, &arubBuffer[usBytesSent], usBytesToSend - usBytesSent );
                if( iBytesWritten > 0 )
                {
                    usBytesSent += ( USHORT ) iBytesWritten;
                }
                else
                {
                    if( ( errno != EINTR ) && ( errno != EAGAIN ) )
                    {
                        eStatus = MB_EIO;
                    }
                }
            }
            if( FD_ISSET( xSerHdl, &err_fds ) )
            {
                eStatus = MB_EIO;
            }
        }
    }
    while( ( usBytesSent < usBytesToSend ) && ( MB_ENOERR == eStatus ) );
    return eStatus;
}

STATIC int
iprvTimeValDiff( struct timeval *result, struct timeval *x, struct timeval *y )
{
    /* Perform the carry for the later subtraction by updating y. */
    if( x->tv_usec < y->tv_usec )
    {
        int             nsec = ( y->tv_usec - x->tv_usec ) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if( x->tv_usec - y->tv_usec > 1000000 )
    {
        int             nsec = ( x->tv_usec - y->tv_usec ) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

void           *
pvMBPSerialHandlerThread( void *pvArg )
{
    UBYTE           arubBuffer[SER_BUFFER_SIZE];
    xSerialHandle  *pxSerHdl = pvArg;
    eMBErrorCode    eStatus;
    USHORT          usBytesToSend;
    USHORT          usBytesReadTotal;
    BOOL            bFrameIsComplete;
    BOOL            bTimeout;
    USHORT          usCRC16;
    USHORT          usInhibitEOFDetectionCode = 0;
    ssize_t         iBytesRead;
    struct timeval  xLastReceiveTime, xCurrentTime, xTimeDiff;
    struct sched_param xSchedParam;

#if defined( MBP_USE_LOW_LATENCY_IO ) && ( MBP_USE_LOW_LATENCY_IO == 1 )
    struct serial_struct xSerInfo;
#endif


#if defined( MBP_USE_LOW_LATENCY_IO ) && ( MBP_USE_LOW_LATENCY_IO == 1 )
    if( 0 == ioctl( pxSerHdl->xSerHdl, TIOCGSERIAL, &xSerInfo ) )
    {
        xSerInfo.flags |= ASYNC_LOW_LATENCY;
        if( 0 != ioctl( pxSerHdl->xSerHdl, TIOCSSERIAL, &xSerInfo ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_WARN, MB_LOG_PORT_SERIAL, "Can't set low latency serial I/O\n", strerror( errno ) );
            }
#endif
        }
    }
#endif

    ( void )gettimeofday( &xLastReceiveTime, NULL );

    if( 0 != sched_getparam( getpid(  ), &xSchedParam ) )
    {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
        if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_SERIAL ) )
        {
            vMBPPortLog( MB_LOG_WARN, MB_LOG_PORT_SERIAL, "Can't get scheduler parameters: %s\n", strerror( errno ) );
        }
#endif
    }
    else
    {
        xSchedParam.sched_priority = sched_get_priority_max( SCHED_RR );
        if( 0 != sched_setscheduler( getpid(  ), SCHED_RR, &xSchedParam ) )
        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
            if( bMBPPortLogIsEnabled( MB_LOG_WARN, MB_LOG_PORT_SERIAL ) )
            {
                vMBPPortLog( MB_LOG_ERROR, MB_LOG_PORT_SERIAL, "Can't raise priority: %s\n", strerror( errno ) );
            }
#endif
        }
    }

    do
    {
        MBP_ENTER_CRITICAL_SECTION(  );
        if( NULL != pxSerHdl->pbMBPSerialTransmitterEmptyFN )
        {
            usBytesToSend = 0;
            if( !pxSerHdl->pbMBPSerialTransmitterEmptyFN( pxSerHdl->xMBHdl, arubBuffer, SER_BUFFER_SIZE, &usBytesToSend ) )
            {
                pxSerHdl->pbMBPSerialTransmitterEmptyFN = NULL;
            }
            MBP_EXIT_CRITICAL_SECTION(  );
            if( usBytesToSend > 0 )
            {
                ( void )gettimeofday( &xCurrentTime, NULL );
                ( void )iprvTimeValDiff( &xTimeDiff, &xCurrentTime, &xLastReceiveTime );
                if( ( xTimeDiff.tv_sec == 0 ) && ( xTimeDiff.tv_usec < 2000 ) )
                {
                    usleep( 2000 );
                }
                if( MB_ENOERR != prveMBPSerialSendBytes( pxSerHdl->xSerHdl, arubBuffer, usBytesToSend ) )
                {
                    pxSerHdl->bIsRunning = FALSE;
                }
            }
        }
        else if( NULL != pxSerHdl->pvMBPSerialReceiverFN )
        {
            MBP_EXIT_CRITICAL_SECTION(  );
            if( prvbMBPSerialWaitReceive( pxSerHdl->xSerHdl, &eStatus ) )
            {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] initial new RX data available\n", pxSerHdl->ubIdx );
                }
#endif
                usBytesReadTotal = 0;
                bFrameIsComplete = FALSE;
                bTimeout = FALSE;
                usCRC16 = 0xFFFFU;
                do
                {
                    iBytesRead = read( pxSerHdl->xSerHdl, &arubBuffer[usBytesReadTotal], 1 );
                    switch ( iBytesRead )
                    {
                        /* IO error or no data available */
                    case 0:
                    case -1:
                        /* The socket is non blocking and if we read from it and there is
                         * no data it returns EAGAIN.
                         */
                        if( ( 0 == iBytesRead ) || ( errno == EINTR ) || ( errno == EAGAIN ) )
                        {
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                            if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
                            {
                                vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] waiting for new RX data (read total=%u)\n", pxSerHdl->ubIdx,
                                             ( unsigned int )usBytesReadTotal );
                            }
#endif
                            /* Wait for some data. In case of an error break the loop */
                            if( !prvbMBPSerialWaitReceive( pxSerHdl->xSerHdl, &eStatus ) )
                            {
                                bTimeout = TRUE;
                                if( MB_ENOERR != eStatus )
                                {
                                    pxSerHdl->bIsRunning = FALSE;
                                }
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
                                {
                                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] wait timed out: error = %d\n", pxSerHdl->ubIdx, ( int )eStatus );
                                }
#endif
                            }
                        }
                        else
                        {
                            /* The read call failed due to an low level I/O error. break out of the loop */
                            bTimeout = TRUE;
                            pxSerHdl->bIsRunning = FALSE;
                        }
                        break;
                    case 1:
                        switch ( pxSerHdl->eMode )
                        {
                        case MB_ASCII:
                            if( arubBuffer[usBytesReadTotal] == '\n' )
                            {
                                bFrameIsComplete = TRUE;
                            }
                            break;
                        case MB_RTU:
                            if( 0x0000U == ( usCRC16 = prvCRC16Update( usCRC16, arubBuffer[usBytesReadTotal] ) ) )
                            {
                                if( 0 == usInhibitEOFDetectionCode )
                                {
                                    if( bMBGuessRTUFrameIsComplete( arubBuffer, usBytesReadTotal + 1 ) )
                                    {
                                        bFrameIsComplete = TRUE;
                                    }
                                }
                            }
                            break;
                        }
                        /* We have read one more character */
                        usBytesReadTotal++;
                        break;
                    }
                }
                while( ( usBytesReadTotal < SER_BUFFER_SIZE ) && !bFrameIsComplete && !bTimeout );
                ( void )gettimeofday( &xLastReceiveTime, NULL );

#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
                if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
                {
                    vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] read data finished (read total=%u, timeout=%u, complete=%u)\n", ( int )pxSerHdl->ubIdx,
                                 ( unsigned int )usBytesReadTotal, ( unsigned int )bTimeout, ( unsigned int )bFrameIsComplete );
                }
#endif

                /* In case of a CRC16 error we always return with a timeout. This can happen as
                 * an result of an incorrect guessed EOF (Very unlikely but still can happen) or
                 * due to an communication error. To recover from this case we disable the performance
                 * improvement until we have a proper frame with standard EOF timeout and no CRC error.
                 */
                if( bTimeout )
                {
                    if( 0x0000U != usCRC16 )
                    {
                        usInhibitEOFDetectionCode = 1;
                    }
                    else
                    {
                        usInhibitEOFDetectionCode = 0;
                    }
                }

                if( usBytesReadTotal > 0 )
                {
                    MBP_ENTER_CRITICAL_SECTION(  );
                    if( pxSerHdl->bIsRunning && ( NULL != pxSerHdl->pvMBPSerialReceiverFN ) )
                    {
                        pxSerHdl->pvMBPSerialReceiverFN( pxSerHdl->xMBHdl, arubBuffer, usBytesReadTotal );
                        if( NULL != pxSerHdl->pbMBPFrameTimeoutFN )
                        {
                            pxSerHdl->pbMBPFrameTimeoutFN( pxSerHdl->xMBHdl );
                        }


                    }
                    MBP_EXIT_CRITICAL_SECTION(  );
                }
            }
            else
            {
                if( MB_ENOERR != eStatus )
                {
                    pxSerHdl->bIsRunning = FALSE;
                }
            }
        }
        else
        {
            MBP_EXIT_CRITICAL_SECTION(  );

            sem_wait( &( pxSerHdl->xIdleSem ) );
        }
    }
    while( pxSerHdl->bIsRunning );
#if defined( MBP_ENABLE_DEBUG_FACILITY ) && ( MBP_ENABLE_DEBUG_FACILITY == 1 )
    if( bMBPPortLogIsEnabled( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL ) )
    {
        vMBPPortLog( MB_LOG_DEBUG, MB_LOG_PORT_SERIAL, "[%d] closing serial handler thread\n", pxSerHdl->ubIdx );
    }
#endif

    ( void )close( pxSerHdl->xSerHdl );
    ( void )sem_destroy( &( pxSerHdl->xIdleSem ) );
    MBP_ENTER_CRITICAL_SECTION(  );
    HDL_RESET( pxSerHdl );
    MBP_EXIT_CRITICAL_SECTION(  );

    pthread_exit( NULL );

    return NULL;
}
