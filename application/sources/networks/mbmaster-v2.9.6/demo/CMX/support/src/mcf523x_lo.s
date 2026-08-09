/************************************************************************************
 * Copyright @ 1995-2005 Freescale Semiconductor, Inc. All rights reserved.         *
 *                                                                                  *
 *                                                                                  *
 * DESCRIPTION                                                                      *
 *   Lowest level routines for the MCF523x.                                         *
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

#ifdef _UNDERSCORE_
#define exception_handler	_exception_handler
#define mcf523x_init		_mcf523x_init
#define _start				__start
#endif
 
	.extern ___IPSBAR
	.extern ___SRAM
	.extern ___SP_INIT
	.extern ___SRAM_SIZE
	.extern exception_handler
	.extern mcf5xxx_wr_rambar0
	.extern mcf5xxx_wr_rambar1
	.extern mcf523x_init
	.extern _start

    .global asm_set_ipl
    .global _asm_set_ipl
	.global asm_startmeup
	.global _asm_startmeup
	.global	asm_exception_handler
	.global	_asm_exception_handler
	.global cpu_cache_flush
	.global _cpu_cache_flush
	.global	mcf5xxx_wr_cacr
	.global _mcf5xxx_wr_cacr
	.global	mcf5xxx_wr_vbr
	.global	_mcf5xxx_wr_vbr

	.text

/********************************************************************
 * This is the main entry point upon hard reset.
 */
asm_startmeup:
_asm_startmeup:
	
	move.w	#0x2700,sr

	/* Initialize IPSBAR */
	move.l	#(___IPSBAR + 1),d0
	move.l	d0,0x40000000
	
	/* Initialize RAMBAR1: locate SRAM and validate it */
	move.l	#(___SRAM + 0x21),d0
    .long   0x4e7b0C05      /* movec d0,RAMBAR1	*/

	/* Point Stack Pointer into SRAM temporarily */
	move.l	#(___SRAM + 0x10000),sp

	/* Initialize mcf523x periphs, etc */
	jsr		mcf523x_init

	/* Relocate Stack Pointer */ 
	move.l	#___SP_INIT,sp

	/* Jump to the main process */
	jmp		_start
	
	bra		.
	nop
	nop
	halt

/********************************************************************/
/*
 * This routines changes the IPL to the value passed into the routine.
 * It also returns the old IPL value back.
 * Calling convention from C:
 *   old_ipl = asm_set_ipl(new_ipl);
 * For the Diab Data C compiler, it passes return value thru D0.
 * Note that only the least significant three bits of the passed
 * value are used.
 */

asm_set_ipl:
_asm_set_ipl:
    link  a6,#-8
    movem.l d6-d7,(sp)

    move.w  sr,d7   /* current sr  */

    move.l  d7,d0   /* prepare return value  */
    andi.l  #0x0700,d0  /* mask out IPL  */
    lsr.l #8,d0   /* IPL   */

    move.l  8(a6),d6  /* get argument  */
    andi.l  #0x07,d6    /* least significant three bits  */
    lsl.l #8,d6   /* move over to make mask  */

    andi.l  #0x0000F8FF,d7  /* zero out current IPL  */
    or.l  d6,d7     /* place new IPL in sr   */
    move.w  d7,sr

    movem.l (sp),d6-d7
    lea   8(sp),sp
    unlk  a6
    rts	
	
/********************************************************************
/*
 * This routine is the lowest-level exception handler.
 */
asm_exception_handler:
_asm_exception_handler:

	lea     -20(sp), sp
	movem.l d0-d2/a0-a1, (sp)
	pea.l   20(sp)              /* push exception frame address */
	jsr		exception_handler
	movem.l 4(sp), d0-d2/a0-a1
	lea     24(sp), sp
	rte

	
/********************************************************************
 * The MCF523x cache can be configured as instruction, data or split.
 * Invalidate the entire cache.
 */
cpu_cache_flush:
_cpu_cache_flush:
	nop						/* sync */
	move.l	#0x01000000,d0	/* Invalidate the I-Cache */
	movec 	d0,cacr
	rts
	
	
/********************************************************************/
/*
 * These routines write to the special purpose registers in the ColdFire
 * core.  Since these registers are write-only in the supervisor model,
 * no corresponding read routines exist.
 */
 
mcf5xxx_wr_cacr:
_mcf5xxx_wr_cacr:
    move.l  4(sp),d0
    movec d0,cacr	 
    nop
    rts

mcf5xxx_wr_vbr:
_mcf5xxx_wr_vbr:
	move.l	4(sp),d0
	movec d0,VBR	
	nop
	rts


	.end
