/************************************************************************************
 * Copyright @ 1995-2005 Freescale Semiconductor, Inc. All rights reserved          *
 *                                                                                  *
 *                                                                                  *
 *                                                                                  *
 * DESCRIPTION                                                                      *
 *   This file has got the exception handlers.                                      *
 *                                                                                  *
 *                                                                                  *
 *                                                                                  *
 * NOTE                                                                             *
 *                                                                                  *
 *                                                                                  *
 *                                                                                  *
 * HISTORY                                                                          *
 *                                                                                  *	
 ************************************************************************************/

#include "m523xevb.h"

/********************************************************************/

/* Called by asm_exception_handler */
void 
exception_handler (void *framep) 
{
	/*
	 * This is the exception handler for all defined exceptions.  Most
	 * exceptions do nothing, but some of the more important ones are
	 * handled to some extent.
	 */
}

/********************************************************************/
__interrupt__
void irq_handler (void) 
{
	/* 
	 * This is the catch all interrupt handler for all user defined
	 * interrupts.  To create specific handlers, create a new interrupt
	 * handler and change vectors.s to point to the new handler.
	 */
}

/********************************************************************/
