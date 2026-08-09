/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science. ]
 * Modifications (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* ----------------------- System includes --------------------------------*/
#include <stdlib.h>
#include <string.h>

/* ----------------------- Project includes -------------------------------*/
#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

/* ----------------------- Defines ----------------------------------------*/
#ifndef SYSARCH_DEBUG
#define SYSARCH_DEBUG           LWIP_DBG_OFF
#endif
/* ----------------------- Type Definitions -------------------------------*/

typedef struct _sys_arch_reent_t
{
    int             _errno;
    struct sys_timeouts timeouts;
} sys_arch_reent_t;

/* ----------------------- Global variables -------------------------------*/

/* ----------------------- Static variables -------------------------------*/
static sys_arch_reent_t *sys_arch_reent_cur;

/* ----------------------- Start implementation ---------------------------*/

/* ----------------------- Mailbox functions ------------------------------*/
/*
 * Creates an empty mailbox for maximum "size" elements. Elements stored
 * in mailboxes are pointers. You have to define macros "_MBOX_SIZE"
 * in your lwipopts.h, or ignore this parameter in your implementation
 * and use a default size.
 */
sys_mbox_t
sys_mbox_new( int iSize )
{
    xQueueHandle    mbox;

    ( void )iSize;

    mbox = xQueueCreate( archMESG_QUEUE_LENGTH, sizeof( void * ) );

#if SYS_STATS
    ++lwip_stats.sys.mbox.used;
    if( lwip_stats.sys.mbox.max < lwip_stats.sys.mbox.used )
    {
        lwip_stats.sys.mbox.max = lwip_stats.sys.mbox.used;
    }
#endif /* SYS_STATS */

    return mbox;
}

/*
 * Deallocates a mailbox. If there are messages still present in the
 * mailbox when the mailbox is deallocated, it is an indication of a
 * programming error in lwIP and the developer should be notified.
 */
void
sys_mbox_free( sys_mbox_t mbox )
{
    unsigned portBASE_TYPE uxMessagesWaiting;
    if( ( uxMessagesWaiting = uxQueueMessagesWaiting( mbox ) ) > 0 )
    {
        LWIP_ASSERT( "0 == uxMessagesWaiting", 0 == uxMessagesWaiting );
#if SYS_STATS
        lwip_stats.sys.mbox.err++;
#endif /* SYS_STATS */
    }
    vQueueDelete( mbox );

#if SYS_STATS
    --lwip_stats.sys.mbox.used;
#endif /* SYS_STATS */
}

/*
 * Posts the "msg" to the mailbox. This function have to block until
 * the "msg" is really posted.
 */
void
sys_mbox_post( sys_mbox_t mbox, void *data )
{
    while( xQueueSendToBack( mbox, &data, portMAX_DELAY ) != pdTRUE );
}

/*
 * Try to post the "msg" to the mailbox. Returns ERR_MEM if this one
 * is full, else, ERR_OK if the "msg" is posted.
 */
err_t
sys_mbox_trypost( sys_mbox_t mbox, void *msg )
{
    err_t           result;

    if( xQueueSend( mbox, &msg, 0 ) == pdPASS )
    {
        result = ERR_OK;
    }
    else
    {
        result = ERR_MEM;

#if SYS_STATS
        lwip_stats.sys.mbox.err++;
#endif /* SYS_STATS */

    }

    return result;
}

/*
 * Blocks the thread until a message arrives in the mailbox, but does
 * not block the thread longer than "timeout" milliseconds (similar to
 * the sys_arch_sem_wait() function). The "msg" argument is a result
 * parameter that is set by the function (i.e., by doing "*msg =
 * ptr"). The "msg" parameter maybe NULL to indicate that the message
 * should be dropped.
 *
 * The return values are the same as for the sys_arch_sem_wait() function:
 * Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
 * timeout.
 *
 * Note that a function with a similar name, sys_mbox_fetch(), is
 * implemented by lwIP.
 */
u32_t
sys_arch_mbox_fetch( sys_mbox_t mbox, void **msg, u32_t timeout )
{
    void           *dummyptr;
    portTickType    StartTime, EndTime, Elapsed;

    StartTime = xTaskGetTickCount(  );

    if( msg == NULL )
    {
        msg = &dummyptr;
    }

    if( timeout != 0 )
    {
        if( pdTRUE == xQueueReceive( mbox, &( *msg ), timeout / portTICK_RATE_MS ) )
        {
            EndTime = xTaskGetTickCount(  );
            Elapsed = ( EndTime - StartTime ) * portTICK_RATE_MS;
            if( 0 == Elapsed )
            {
                Elapsed = 1;
            }
            return ( Elapsed );
        }
        else
        {
            *msg = NULL;

            return SYS_ARCH_TIMEOUT;
        }
    }
    else
    {
        while( pdTRUE != xQueueReceive( mbox, &( *msg ), portMAX_DELAY ) );
        EndTime = xTaskGetTickCount(  );
        Elapsed = ( EndTime - StartTime ) * portTICK_RATE_MS;

        return ( Elapsed );
    }
}

/*
 * Similar to sys_arch_mbox_fetch, but if message is not ready immediately, we'll
 * return with SYS_MBOX_EMPTY.  On success, 0 is returned.
 */
u32_t
sys_arch_mbox_tryfetch( sys_mbox_t mbox, void **msg )
{
    void           *dummyptr;

    if( msg == NULL )
    {
        msg = &dummyptr;
    }

    if( pdTRUE == xQueueReceive( mbox, &( *msg ), 0 ) )
    {
        return ERR_OK;
    }
    else
    {
        return SYS_MBOX_EMPTY;
    }
}

/* ----------------------- Semaphore functions ----------------------------*/

/*
 * Creates and returns a new semaphore. The "count" argument specifies
 * the initial state of the semaphore.
 */
sys_sem_t
sys_sem_new( u8_t count )
{
    xSemaphoreHandle xSemaphore;
signed portBASE_TYPE  pdResult;
    vSemaphoreCreateBinary( xSemaphore )
    if( NULL == xSemaphore )
    {
#if SYS_STATS
        ++lwip_stats.sys.sem.err;
#endif /* SYS_STATS */
        LWIP_DEBUGF(SYSARCH_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("sys_sem_new(%d) can not create semaphore.\n", ( int )count));
        xSemaphore = SYS_SEM_NULL;
    }
    else
    {
        if( 0 == count )
        {
            pdResult = xSemaphoreTake( xSemaphore, 0 );
            LWIP_ASSERT( "pdResult == pdPASS", pdResult == pdPASS ); 
        }
    
#if SYS_STATS
        ++lwip_stats.sys.sem.used;
        if( lwip_stats.sys.sem.max < lwip_stats.sys.sem.used )
        {
            lwip_stats.sys.sem.max = lwip_stats.sys.sem.used;
        }
#endif /* SYS_STATS */
        LWIP_DEBUGF(SYSARCH_DEBUG | LWIP_DBG_TRACE, ("sys_sem_new(%d) created new semaphore = %p\n", ( int )count, xSemaphore));
    }

    return xSemaphore;
}

/*
 * Blocks the thread while waiting for the semaphore to be
 * signaled. If the "timeout" argument is non-zero, the thread should
 * only be blocked for the specified time (measured in
 * milliseconds).
 *
 * If the timeout argument is non-zero, the return value is the number of
 * milliseconds spent waiting for the semaphore to be signaled. If the
 * semaphore wasn't signaled within the specified time, the return value is
 * SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
 * (i.e., it was already signaled), the function may return zero.
 *
 * Notice that lwIP implements a function with a similar name,
 * sys_sem_wait(), that uses the sys_arch_sem_wait() function.
 */
u32_t
sys_arch_sem_wait( sys_sem_t xSemaphore, u32_t timeout )
{
    u32_t    StartTime, EndTime, ulElapsed;

    StartTime = ( u32_t )xTaskGetTickCount(  );

    if( timeout != 0 )
    {
        if( xSemaphoreTake( xSemaphore, timeout / portTICK_RATE_MS ) == pdTRUE )
        {
            EndTime = ( u32_t )xTaskGetTickCount(  );
            ulElapsed = ( EndTime - StartTime ) * portTICK_RATE_MS;
            if( 0 == ulElapsed )
            {
                ulElapsed = 1;
            }
        }
        else
        {
            ulElapsed = SYS_ARCH_TIMEOUT;
        }
    }
    else
    {
        while( xSemaphoreTake( xSemaphore, portMAX_DELAY ) != pdTRUE );
        EndTime = ( u32_t )xTaskGetTickCount(  );
        ulElapsed = ( EndTime - StartTime ) * portTICK_RATE_MS;
        if( 0 == ulElapsed )
        {
            ulElapsed = 1;
        }
    }
    LWIP_DEBUGF( SYSARCH_DEBUG | LWIP_DBG_TRACE, ("sys_arch_sem_wait( %p, %ul): time elapsed = %ul.\n" , xSemaphore, timeout, ulElapsed ) );
    return ulElapsed;
}

/*
 * Signals a semaphore
 */
void
sys_sem_signal( sys_sem_t xSemaphore )
{
    LWIP_DEBUGF(SYSARCH_DEBUG | LWIP_DBG_TRACE, ("sys_sem_signal(%p): signaling semaphore.\n" , xSemaphore ) );
    if( pdTRUE != xSemaphoreGive( xSemaphore ) )
    {
        LWIP_DEBUGF(SYSARCH_DEBUG | LWIP_DBG_TRACE, ("sys_sem_signal(%p): failed because semaphore was already signaled!\n" , xSemaphore ) );
    }
}

/*
 * Deallocates a semaphore
 */
void
sys_sem_free( sys_sem_t xSemaphore )
{
#if SYS_STATS
    --lwip_stats.sys.sem.used;
#endif /* SYS_STATS */

    vQueueDelete( xSemaphore );
    LWIP_DEBUGF(SYSARCH_DEBUG | LWIP_DBG_TRACE, ("sys_sem_free(%p): deleted semaphore.\n" , xSemaphore ) );
}

/* ----------------------- Timeout functions ------------------------------*/

/*
 * Returns a pointer to the per-thread sys_timeouts structure. In lwIP,
 * each thread has a list of timeouts which is represented as a linked
 * list of sys_timeout structures. The sys_timeouts structure holds a
 * pointer to a linked list of timeouts. This function is called by
 * the lwIP timeout scheduler and must not return a NULL value.
 *
 * In a single threaded sys_arch implementation, this function will
 * simply return a pointer to a global sys_timeouts variable stored in
 * the sys_arch module.
 */
struct sys_timeouts *
sys_arch_timeouts( void )
{
    LWIP_ASSERT( "NULL != sys_arch_reent_cur", NULL != sys_arch_reent_cur );
    return &( sys_arch_reent_cur->timeouts );
}

/* ----------------------- Core functions ---------------------------------*/

void
sys_init( void )
{
}

/*
 * Starts a new thread with priority "prio" that will begin its execution in the
 * function "thread()". The "arg" argument will be passed as an argument to the
 * thread() function. The id of the new thread is returned. Both the id and
 * the priority are system dependent.
 */
typedef struct _sys_thread_int_arg_t
{
    void            ( *pfnvThread ) ( void *pvArg );
    void           *pvArg;
} sys_thread_int_arg_t;

void
vTaskWrapper( void *pvArg )
{
    sys_thread_int_arg_t xThreadArg;
    sys_arch_reent_t my_reent;

    LWIP_ASSERT( pvArg, NULL != pvArg );
    /* We must free the malloced structure here and instead allocate 
     * this data on the stack because otherwise we could never free
     * it again.
     */
    memcpy( &xThreadArg, pvArg, sizeof( sys_thread_int_arg_t ) );
    vPortFree( pvArg );

    /* Prepare application context from errno, ... */
    memset( &my_reent, 0, sizeof( my_reent ) );
    vTaskSetApplicationTaskTag( NULL, ( pdTASK_HOOK_CODE ) & my_reent );

    /* Call taskYIELD to perform at least one context switch because
     * we need to get the trace macro called.
     */
    taskYIELD(  );

    /* Call task function. */
    xThreadArg.pfnvThread( xThreadArg.pvArg );

    /* In case somebody returns from the thread delete it. */
    vTaskDelete( NULL );
}

sys_thread_t
sys_thread_new( char *name, void ( *thread ) ( void *arg ), void *arg, int stacksize, int prio )
{
    xTaskHandle     xCreatedTask = NULL;
    sys_thread_int_arg_t *pxThreadArg;

    if( NULL != ( pxThreadArg = pvPortMalloc( sizeof( sys_thread_int_arg_t ) ) ) )
    {
        pxThreadArg->pfnvThread = thread;
        pxThreadArg->pvArg = arg;
        if( pdPASS !=
            xTaskCreate( vTaskWrapper, ( signed portCHAR * )name, stacksize, pxThreadArg, prio, &xCreatedTask ) )
        {
            vPortFree( pxThreadArg );
            /* TODO: Possibly log an error message here or assert in debug. */
        }
    }
    return xCreatedTask;
}

/*
 * This optional function does a "fast" critical region protection and returns
 * the previous protection level. This function is only called during very short
 * critical regions. An embedded system which supports ISR-based drivers might
 * want to implement this function by disabling interrupts. Task-based systems
 * might want to implement this by using a mutex or disabling tasking. This
 * function should support recursive calls from the same task or interrupt. In
 * other words, sys_arch_protect() could be called while already protected. In
 * that case the return value indicates that it is already protected.
 *
 * sys_arch_protect() is only required if your port is supporting an operating
 * system.
 */
sys_prot_t
sys_arch_protect( void )
{
    vPortEnterCritical(  );
    return 1;
}

/*
 * This optional function does a "fast" set of critical region protection to the
 * value specified by pval. See the documentation for sys_arch_protect() for
 * more information. This function is only required if your port is supporting
 * an operating system.
 */
void
sys_arch_unprotect( sys_prot_t pval )
{
    ( void )pval;
    vPortExitCritical(  );
}

/*
 * Prints an assertion messages and aborts execution.
 */
void
sys_assert( const char *msg )
{
    ( void )msg;
    vPortEnterCritical(  );
    for( ;; );
}

/*
 * In case of a context switch of a new process restore its current task
 * specific variables like errno, timeouts, ...
 */
void
sys_arch_switch_ctx( void *pvCtx )
{
    sys_arch_reent_cur = pvCtx;
}

/* 
 * Return errno for this task.
 */
int            *
sys_arch_errno( void )
{
    LWIP_ASSERT( "NULL != sys_arch_reent_cur", NULL != sys_arch_reent_cur );
    return &( sys_arch_reent_cur->_errno );
}
