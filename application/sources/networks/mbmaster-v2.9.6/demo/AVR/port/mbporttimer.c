/* 
 * MODBUS Library: AVR port
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: mbporttimer.c,v 1.13 2010-08-19 20:18:18 embedded-so.embedded-solutions.1 Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#if defined(  __CODEVISIONAVR__ )
#else
#include <avr/io.h>
#include <avr/interrupt.h>
#endif
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/

#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "common/mbtypes.h"
#include "common/mbportlayer.h"
#include "common/mbframe.h"
#include "common/mbutils.h"

/* ----------------------- Defines ------------------------------------------*/

#define MAX_TIMER_HDLS          ( 5 )
#define IDX_INVALID             ( 255 )
#define EV_NONE                 ( 0 )

#define TIMER_TIMEOUT_INVALID	( 65535U )
#define TIMER_PRESCALER			( 256 )
#define TIMER_TICKS_MS			( ( F_CPU ) / ( ( ULONG )TIMER_PRESCALER * 1000UL ) )


#define RESET_HDL( x ) do { \
    ( x )->ubIdx = IDX_INVALID; \
	( x )->usNTimeOutMS = 0; \
	( x )->usNTimeLeft = TIMER_TIMEOUT_INVALID; \
    ( x )->xMBMHdl = MB_HDL_INVALID; \
    ( x )->pbMBPTimerExpiredFN = NULL; \
} while( 0 )

/* ----------------------- Type definitions ---------------------------------*/
typedef struct
{
    UBYTE           ubIdx;
    USHORT          usNTimeOutMS;
    USHORT          usNTimeLeft;
    xMBHandle       xMBMHdl;
    pbMBPTimerExpiredCB pbMBPTimerExpiredFN;
} xTimerInternalHandle;

/* ----------------------- Static variables ---------------------------------*/
STATIC xTimerInternalHandle arxTimerHdls[MAX_TIMER_HDLS];
STATIC BOOL     bIsInitalized = FALSE;

/* ----------------------- Static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode
eMBPTimerInit( xMBPTimerHandle * xTimerHdl, USHORT usTimeOut1ms,
               pbMBPTimerExpiredCB pbMBPTimerExpiredFN, xMBHandle xHdl )
{
    eMBErrorCode    eStatus = MB_EPORTERR;
    UBYTE           ubIdx;


    MBP_ENTER_CRITICAL_SECTION(  );
    if( ( NULL != xTimerHdl ) && ( NULL != pbMBPTimerExpiredFN ) && ( MB_HDL_INVALID != xHdl ) )
    {
        if( !bIsInitalized )
        {
#if defined( __AVR_ATmega32__ )
            TCCR2 = _BV( WGM21 ) | _BV( CS22 ) | _BV( CS21 );
            OCR2 = TIMER_TICKS_MS - 1;
            TIMSK |= _BV( OCIE2 );
#elif defined( __AVR_ATmega128__ ) || defined( __AVR_ATmega64__ )
            TCCR2 = _BV( WGM21 ) | _BV( CS22 );
            OCR2 = TIMER_TICKS_MS - 1;
            TIMSK |= _BV( OCIE2 );
#elif defined( __AVR_ATmega168__ ) 
            TCCR2A = _BV( WGM21 );
            TCCR2B = _BV( CS22 ) | _BV( CS21 );
            OCR2A = TIMER_TICKS_MS - 1;
            TIMSK2 |= _BV( OCIE2A );
#elif defined( __AVR_ATmega328P__ ) 
            TCCR2A = _BV( WGM21 );
            TCCR2B = _BV( CS22 ) | _BV( CS21 );
            OCR2A = TIMER_TICKS_MS - 1;
            TIMSK2 |= _BV( OCIE2A );
#elif defined( __AVR_ATmega644P__ ) 
            TCCR2A = _BV( WGM21 );
            TCCR2B = _BV( CS22 ) | _BV( CS21 );
            OCR2A = TIMER_TICKS_MS - 1;
            TIMSK2 |= _BV( OCIE2A );
#elif defined( __AVR_ATmega2561__ )
            TCCR4A = 0;
            TCCR4B = _BV( WGM42 );            
            TCCR4C = 0;
            OCR4A = TIMER_TICKS_MS - 1;
            TIMSK4 |= _BV( OCIE4A );
            TCCR4B |= _BV( CS42 );
#else
#error "Unsupported AVR hardware platform."
#endif
            for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
            {
                RESET_HDL( &arxTimerHdls[ubIdx] );
            }

            bIsInitalized = TRUE;

        }
        for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
        {
            if( IDX_INVALID == arxTimerHdls[ubIdx].ubIdx )
            {
                break;
            }
        }
        if( MAX_TIMER_HDLS != ubIdx )
        {
            arxTimerHdls[ubIdx].ubIdx = ubIdx;
            arxTimerHdls[ubIdx].usNTimeOutMS = usTimeOut1ms;
            arxTimerHdls[ubIdx].usNTimeLeft = TIMER_TIMEOUT_INVALID;
            arxTimerHdls[ubIdx].xMBMHdl = xHdl;
            arxTimerHdls[ubIdx].pbMBPTimerExpiredFN = pbMBPTimerExpiredFN;

            *xTimerHdl = &arxTimerHdls[ubIdx];
            eStatus = MB_ENOERR;
        }
        else
        {
            eStatus = MB_ENORES;
        }
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

void
vMBPTimerClose( xMBPTimerHandle xTimerHdl )
{
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        RESET_HDL( pxTimerIntHdl );
    }
}

eMBErrorCode
eMBPTimerSetTimeout( xMBPTimerHandle xTimerHdl, USHORT usTimeOut1ms )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) &&
        ( usTimeOut1ms > 0 ) && ( usTimeOut1ms != TIMER_TIMEOUT_INVALID ) )
    {

        pxTimerIntHdl->usNTimeOutMS = usTimeOut1ms;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPTimerStart( xMBPTimerHandle xTimerHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        pxTimerIntHdl->usNTimeLeft = pxTimerIntHdl->usNTimeOutMS;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBPTimerStop( xMBPTimerHandle xTimerHdl )
{
    eMBErrorCode    eStatus = MB_EINVAL;
    xTimerInternalHandle *pxTimerIntHdl = xTimerHdl;

    MBP_ENTER_CRITICAL_SECTION(  );
    if( MB_IS_VALID_HDL( pxTimerIntHdl, arxTimerHdls ) )
    {
        pxTimerIntHdl->usNTimeLeft = TIMER_TIMEOUT_INVALID;
        eStatus = MB_ENOERR;
    }
    MBP_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

#if defined( __CODEVISIONAVR__ )
#if defined( __AVR_ATmega128__ )
interrupt[TIM2_COMP] void vMBTimerISR( void ) 
#else
#error "Unsupported AVR hardware platform."
#endif
#else
#if defined( __AVR_ATmega32__ ) 
SIGNAL( SIG_OUTPUT_COMPARE2 )
#elif defined( __AVR_ATmega128__ ) || defined( __AVR_ATmega64__ )
SIGNAL( SIG_OUTPUT_COMPARE2 )
#elif defined( __AVR_ATmega168__ ) 
SIGNAL( SIG_OUTPUT_COMPARE2A )
#elif defined( __AVR_ATmega328P__ )
ISR( TIMER2_COMPA_vect, ISR_BLOCK )
#elif defined( __AVR_ATmega644P__ )
ISR( TIMER2_COMPA_vect, ISR_BLOCK )
#elif defined( __AVR_ATmega2561__ )
ISR( TIMER4_COMPA_vect, ISR_BLOCK )
#endif
#endif
{
    UBYTE           ubIdx;

    for( ubIdx = 0; ubIdx < MB_UTILS_NARRSIZE( arxTimerHdls ); ubIdx++ )
    {
        if( ( IDX_INVALID != arxTimerHdls[ubIdx].ubIdx ) &&
            ( TIMER_TIMEOUT_INVALID != arxTimerHdls[ubIdx].usNTimeLeft ) )
        {
            arxTimerHdls[ubIdx].usNTimeLeft--;
            if( 0 == arxTimerHdls[ubIdx].usNTimeLeft )
            {
                arxTimerHdls[ubIdx].usNTimeLeft = TIMER_TIMEOUT_INVALID;
                ( void )arxTimerHdls[ubIdx].pbMBPTimerExpiredFN( arxTimerHdls[ubIdx].xMBMHdl );
            }
        }
    }
}
