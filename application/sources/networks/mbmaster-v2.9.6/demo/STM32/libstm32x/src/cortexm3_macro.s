/***********************************************************************************
*	Copyright 2005 Anglia Design
*	This demo code and associated components are provided as is and has no warranty,
*	implied or otherwise.  You are free to use/modify any of the provided
*	code at your own risk in your applications with the expressed limitation
*	of liability (see below)
* 
*	LIMITATION OF LIABILITY:   ANGLIA OR ANGLIA DESIGNS SHALL NOT BE LIABLE FOR ANY
*	LOSS OF PROFITS, LOSS OF USE, LOSS OF DATA, INTERRUPTION OF BUSINESS, NOR FOR
*	INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES OF ANY KIND WHETHER UNDER
*	THIS AGREEMENT OR OTHERWISE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*
*	Author			: Spencer Oliver
*	Web     		: www.anglia-designs.com
*
***********************************************************************************/

	.text
	.syntax unified
	.cpu cortex-m3
	.thumb
	.align 2

	/* exported functions */
	.global __WFI
	.global __WFE
	.global __SEV
	.global __ISB
	.global __DSB
	.global __DMB
	.global __SVC
	.global __MRS_CONTROL
	.global __MSR_CONTROL
	.global __MRS_PSP
	.global __MSR_PSP
	.global __MRS_MSP
	.global __MSR_MSP    
	.global __SETPRIMASK
	.global __RESETPRIMASK
	.global __SETFAULTMASK
	.global __RESETFAULTMASK
	.global __BASEPRICONFIG
	.global __GetBASEPRI
	.global __REV_HalfWord
	.global __REV_Word  

/*******************************************************************************
 Function Name  : __WFI
 Description    : Assembler function for the WFI instruction.
 Input          : None
 Return         : None
*******************************************************************************/
	.thumb_func
__WFI:
	wfi
	bx lr

/*******************************************************************************
 Function Name  : __WFE
 Description    : Assembler function for the WFE instruction.
 Input          : None
 Return         : None
*******************************************************************************/
	.thumb_func
__WFE:
	wfe
	bx lr

/*******************************************************************************
 Function Name  : __SEV
 Description    : Assembler function for the SEV instruction.
 Input          : None
 Return         : None
*******************************************************************************/
	.thumb_func
__SEV:
	sev
	bx lr

/*******************************************************************************
 Function Name  : __ISB
 Description    : Assembler function for the ISB instruction.
 Input          : None
 Return         : None
*******************************************************************************/
	.thumb_func
__ISB:
	isb
	bx lr

/*******************************************************************************
 Function Name  : __DSB
 Description    : Assembler function for the DSB instruction.
 Input          : None
 Return         : None
*******************************************************************************/
	.thumb_func
__DSB:
	dsb
	bx lr

/*******************************************************************************
 Function Name  : __DMB
 Description    : Assembler function for the DMB instruction.
 Input          : None
 Return         : None
*******************************************************************************/
	.thumb_func
__DMB:
	dmb
	bx lr

/*******************************************************************************
 Function Name  : __SVC
 Description    : Assembler function for the SVC instruction.
 Input          : None
 Return         : None
*******************************************************************************/
	.thumb_func
__SVC:
	svc 0x01
	bx lr

/*******************************************************************************
 Function Name  : __MRS_CONTROL
 Description    : Assembler function for the MRS instruction.
 Input          : None
 Return         : - r0 : Cortex-M3 CONTROL register value.
*******************************************************************************/
	.thumb_func
__MRS_CONTROL:
	mrs	r0,	control
	bx lr

/*******************************************************************************
 Function Name  : __MSR_CONTROL
 Description    : Assembler function for the MSR instruction.
 Input          : - r0 : Cortex-M3 CONTROL register new value.  
 Return         : None
*******************************************************************************/
	.thumb_func
__MSR_CONTROL:
	msr control, r0
	isb
	bx lr

/*******************************************************************************
 Function Name  : __MRS_PSP
 Description    : Assembler function for the MRS instruction.
 Input          : None
 Return         : - r0 : Process Stack value.
*******************************************************************************/
	.thumb_func
__MRS_PSP:
	mrs r0, psp
	bx lr

/*******************************************************************************
 Function Name  : __MSR_PSP
 Description    : Assembler function for the MSR instruction.
 Input          : - r0 : Process Stack new value.  
 Return         : None
*******************************************************************************/
	.thumb_func
__MSR_PSP:
	msr psp, r0		/* set Process Stack value */
	bx lr

/*******************************************************************************
 Function Name  : __MRS_MSP
 Description    : Assembler function for the MRS instruction.
 Input          : None
 Return         : - r0 : Main Stack value.
*******************************************************************************/
	.thumb_func
__MRS_MSP:
	mrs r0, msp
	bx lr

/*******************************************************************************
 Function Name  : __MSR_MSP
 Description    : Assembler function for the MSR instruction.
 Input          : - r0 : Main Stack new value.  
 Return         : None
*******************************************************************************/
	.thumb_func
__MSR_MSP:
	msr msp, r0		/* set Main Stack value */
	bx lr
            
/*******************************************************************************
 Function Name  : __SETPRIMASK
 Description    : Assembler function to set the PRIMASK.
 Input          : None 
 Return         : None
*******************************************************************************/
	.thumb_func
__SETPRIMASK:
	cpsid i
	bx lr

/*******************************************************************************
 Function Name  : __RESETPRIMASK
 Description    : Assembler function to reset the PRIMASK.
 Input          : None 
 Return         : None
*******************************************************************************/
	.thumb_func
__RESETPRIMASK:
	cpsie i
	bx lr

/*******************************************************************************
 Function Name  : __SETFAULTMASK
 Description    : Assembler function to set the FAULTMASK.
 Input          : None 
 Return         : None
*******************************************************************************/
	.thumb_func
__SETFAULTMASK:
	cpsid f
	bx lr

/*******************************************************************************
 Function Name  : __RESETFAULTMASK
 Description    : Assembler function to reset the FAULTMASK.
 Input          : None 
 Return         : None
*******************************************************************************/
	.thumb_func
__RESETFAULTMASK:
	cpsie f
	bx lr

/*******************************************************************************
 Function Name  : __BASEPRICONFIG
 Description    : Assembler function to set the Base Priority.
 Input          : - r0 : Base Priority new value  
 Return         : None
*******************************************************************************/
	.thumb_func
__BASEPRICONFIG:
	msr basepri, r0
	bx lr

/*******************************************************************************
 Function Name  : __GetBASEPRI
 Description    : Assembler function to get the Base Priority value.
 Input          : None 
 Return         : - r0 : Base Priority value 
*******************************************************************************/
	.thumb_func
__GetBASEPRI:
	mrs r0, basepri_max
	bx lr

/*******************************************************************************
 Function Name  : __REV_HalfWord
 Description    : Reverses the byte order in HalfWord(16-bit) input variable.
 Input          : - r0 : specifies the input variable
 Return         : - r0 : holds tve variable value after byte reversing.
*******************************************************************************/
	.thumb_func
__REV_HalfWord:
	rev16 r0, r0
	bx lr

/*******************************************************************************
 Function Name  : __REV_Word
 Description    : Reverses the byte order in Word(32-bit) input variable.
 Input          : - r0 : specifies the input variable
 Return         : - r0 : holds tve variable value after byte reversing.
*******************************************************************************/
	.thumb_func
__REV_Word:
	rev	r0, r0
	bx	lr
  
	.end

