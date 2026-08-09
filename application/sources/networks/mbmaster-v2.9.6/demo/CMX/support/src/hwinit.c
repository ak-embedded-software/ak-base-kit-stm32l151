/************************************************************************************
 * Copyright @ 1995-2005 Freescale Semiconductor, Inc. All rights reserved          *
 *                                                                                  *
 *                                                                                  *
 *                                                                                  *
 * DESCRIPTION                                                                      *
 *   This file initialize the hardware.                                             *
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

#pragma cplusplus off
#include "m523xevb.h"

/********************************************************************/

void mcf523x_init(void);
void mcf523x_wtm_init(void);
void mcf523x_pll_init(void);
void mcf523x_uart_init(void);
void mcf523x_scm_init(void);
void mcf523x_gpio_init(void);
void mcf523x_cs_init(void);
void mcf523x_sdram_init(void);

/********************************************************************/
void
mcf523x_init(void)
{
	extern char __DATA_ROM[];
	extern char __DATA_RAM[];
	extern char __DATA_END[];
	extern char __BSS_START[];
	extern char __BSS_END[];
	extern uint32 VECTOR_TABLE[];
	extern uint32 __VECTOR_RAM[];
	register uint32 n;

	mcf523x_wtm_init();
	mcf523x_pll_init();
	mcf523x_gpio_init();
	mcf523x_scm_init();
	mcf523x_uart_init();
	mcf523x_cs_init();
	mcf523x_sdram_init();

	/* Turn Instruction Cache ON */
	mcf5xxx_wr_cacr(0
		| MCF5XXX_CACR_CENB
		| MCF5XXX_CACR_CINV
		| MCF5XXX_CACR_DISD
		| MCF5XXX_CACR_CEIB
		| MCF5XXX_CACR_CLNF_00);


	/* Copy the vector table to RAM */
	if (__VECTOR_RAM != VECTOR_TABLE)
	{
		for (n = 0; n < 256; n++)
			__VECTOR_RAM[n] = VECTOR_TABLE[n];
	}
	mcf5xxx_wr_vbr((uint32)__VECTOR_RAM);

}
/********************************************************************/
void
mcf523x_wtm_init(void)
{
	/*
	 * Disable Software Watchdog Timer
	 */
	MCF_WTM_WCR = 0;
}
/********************************************************************/
void
mcf523x_pll_init(void)
{
	/*
     * Multiply 25Mhz reference crystal to acheive system clock of 150Mhz
	 */

    MCF_FMPLL_SYNCR = MCF_FMPLL_SYNCR_MFD(1) | MCF_FMPLL_SYNCR_RFD(0);

	while (!(MCF_FMPLL_SYNSR & MCF_FMPLL_SYNSR_LOCK));
}
/********************************************************************/
void
mcf523x_scm_init(void)
{
	/*
	 * Enable on-chip modules to access internal SRAM
	 */
	MCF_SCM_RAMBAR = (0
		| MCF_SCM_RAMBAR_BA(SRAM_ADDRESS>>16)
		| MCF_SCM_RAMBAR_BDE);
}
/********************************************************************/
void
mcf523x_gpio_init(void)
{

	/*
	 * When booting from external Flash, the port-size is less than
	 * the port-size of SDRAM.  In this case it is necessary to enable
	 * Data[15:0] on Port Address/Data.
	 */
	MCF_GPIO_PAR_AD = (0
		| MCF_GPIO_PAR_AD_PAR_ADDR23
		| MCF_GPIO_PAR_AD_PAR_ADDR22
		| MCF_GPIO_PAR_AD_PAR_ADDR21
		| MCF_GPIO_PAR_AD_PAR_DATAL);

	/*
	 * Initialize PAR to enable SDRAM signals
	 */
	MCF_GPIO_PAR_SDRAM = 0x3F;

	/*
	 * Initialize PAR to enable Ethernet signals
	 */
	MCF_GPIO_PAR_FECI2C = 0xF0;
}
/********************************************************************/
void
mcf523x_uart_init(void)
{
	/*
	 * Initialize all three UARTs for serial communications
	 */

	register uint16 ubgs;

	/*
	 * Set Port UA to initialize URXD0/URXD1 UTXD0/UTXD1
	 */
	MCF_GPIO_PAR_UART = 0x0F;

	/*
	 * Reset Transmitter
	 */
	MCF_UART_UCR0 = MCF_UART_UCR_RESET_TX;
	MCF_UART_UCR1 = MCF_UART_UCR_RESET_TX;
	MCF_UART_UCR2 = MCF_UART_UCR_RESET_TX;

	/*
	 * Reset Receiver
	 */
	MCF_UART_UCR0 = MCF_UART_UCR_RESET_RX;
	MCF_UART_UCR1 = MCF_UART_UCR_RESET_RX;
	MCF_UART_UCR2 = MCF_UART_UCR_RESET_RX;

	/*
	 * Reset Mode Register
	 */
	MCF_UART_UCR0 = MCF_UART_UCR_RESET_MR;
	MCF_UART_UCR1 = MCF_UART_UCR_RESET_MR;
	MCF_UART_UCR2 = MCF_UART_UCR_RESET_MR;

	/*
	 * No parity, 8-bits per character
	 */
	MCF_UART_UMR0 = (0
		| MCF_UART_UMR_PM_NONE
		| MCF_UART_UMR_BC_8 );
	MCF_UART_UMR1 = (0
		| MCF_UART_UMR_PM_NONE
		| MCF_UART_UMR_BC_8 );
	MCF_UART_UMR2 = (0
		| MCF_UART_UMR_PM_NONE
		| MCF_UART_UMR_BC_8 );

	/*
	 * No echo or loopback, 1 stop bit
	 */
	MCF_UART_UMR0 = (0
		| MCF_UART_UMR_CM_NORMAL
		| MCF_UART_UMR_SB_STOP_BITS_1);
	MCF_UART_UMR1 = (0
		| MCF_UART_UMR_CM_NORMAL
		| MCF_UART_UMR_SB_STOP_BITS_1);
	MCF_UART_UMR2 = (0
		| MCF_UART_UMR_CM_NORMAL
		| MCF_UART_UMR_SB_STOP_BITS_1);

	/*
	 * Set Rx and Tx baud by timer
	 */
	MCF_UART_UCSR0 = (0
		| MCF_UART_UCSR_RCS_SYS_CLK
		| MCF_UART_UCSR_TCS_SYS_CLK);
	MCF_UART_UCSR1 = (0
		| MCF_UART_UCSR_RCS_SYS_CLK
		| MCF_UART_UCSR_TCS_SYS_CLK);
	MCF_UART_UCSR2 = (0
		| MCF_UART_UCSR_RCS_SYS_CLK
		| MCF_UART_UCSR_TCS_SYS_CLK);

	/* 
	 * Mask all UART interrupts 
	 */
	MCF_UART_UIMR0 = 0;
	MCF_UART_UIMR1 = 0;
	MCF_UART_UIMR2 = 0;
                 
	/* 
	 * Calculate baud settings 
	 */
	ubgs = (uint16)((SYSTEM_CLOCK*1000000)/(UART_BAUD * 32));

	MCF_UART_UBG1(0) = (uint8)((ubgs & 0xFF00) >> 8);
	MCF_UART_UBG2(0) = (uint8)(ubgs & 0x00FF);

	MCF_UART_UBG1(1) = (uint8)((ubgs & 0xFF00) >> 8);
	MCF_UART_UBG2(1) = (uint8)(ubgs & 0x00FF);

	MCF_UART_UBG1(2) = (uint8)((ubgs & 0xFF00) >> 8);
	MCF_UART_UBG2(2) = (uint8)(ubgs & 0x00FF);

	/* 
	 * Enable receiver and transmitter 
	 */
	MCF_UART_UCR0 = (0
		| MCF_UART_UCR_TX_ENABLED
		| MCF_UART_UCR_RX_ENABLED);
	MCF_UART_UCR1 = (0
		| MCF_UART_UCR_TX_ENABLED
		| MCF_UART_UCR_RX_ENABLED);
	MCF_UART_UCR2 = (0
		| MCF_UART_UCR_TX_ENABLED
		| MCF_UART_UCR_RX_ENABLED);

}
/********************************************************************/
void
mcf523x_sdram_init(void)
{
	int i;

	/*
	 * Check to see if the SDRAM has already been initialized
	 * by a run control tool
	 */
	if (!(MCF_SDRAMC_DACR0 & MCF_SDRAMC_DACR0_RE))
	{
		/* 
		 * Initialize DRAM Control Register: DCR 
		 */
		MCF_SDRAMC_DCR = (0
			| MCF_SDRAMC_DCR_RTIM(6)
			| MCF_SDRAMC_DCR_RC((15 * SYSTEM_CLOCK)>>4));

		/* 
		 * Initialize DACR0
		 */
		MCF_SDRAMC_DACR0 = (0
			| MCF_SDRAMC_DACR0_BA(SDRAM_ADDRESS)
			| MCF_SDRAMC_DACR0_CASL(1)
			| MCF_SDRAMC_DACR0_CBM(3)
			| MCF_SDRAMC_DACR0_PS(32));
			
		/*
		 * Initialize DMR0
		 */
		MCF_SDRAMC_DMR0 = (0
			| MCF_SDRAMC_DMR_BAM_16M
			| MCF_SDRAMC_DMR0_V);

		/*	
		 * Set IP (bit 3) in DACR 
		 */
		MCF_SDRAMC_DACR0 |= MCF_SDRAMC_DACR0_IP;

		/* 
		 * Wait 30ns to allow banks to precharge 
		 */
		for (i = 0; i < 5; i++)
		{
			#ifndef __MWERKS__
				asm(" nop");
			#else
				asm( nop);
			#endif
		}
		
		/*	
		 * Write to this block to initiate precharge 
		 */
		*(uint32 *)(SDRAM_ADDRESS) = 0xA5A59696;

		/*	
		 * Set RE (bit 15) in DACR 
		 */
		MCF_SDRAMC_DACR0 |= MCF_SDRAMC_DACR0_RE;
			
		/* 
		 * Wait for at least 8 auto refresh cycles to occur 
		 */
		for (i = 0; i < 2000; i++)
		{
			#ifndef __MWERKS__
				asm(" nop");
			#else
				asm( nop);
			#endif
		}

		/*	
		 * Finish the configuration by issuing the IMRS. 
		 */
		MCF_SDRAMC_DACR0 |= MCF_SDRAMC_DACR0_MRS;
		
		/*
		 * Write to the SDRAM Mode Register 
		 */
		*(uint32 *)(SDRAM_ADDRESS + 0x400) = 0xA5A59696;
	}

}
/********************************************************************/
void
mcf523x_cs_init(void)
{
	/* 
	 * ChipSelect 1 - External SRAM 
	 */
	MCF_CS_CSAR1 = MCF_CS_CSAR_BA(EXT_SRAM_ADDRESS);
//	MCF_CS_CSCR1 = MCF_CS_CSCR_AA | MCF_CS_CSCR_BEM | MCF_CS_CSCR_PS_32;
	MCF_CS_CSCR1 = 0x3d20;
	MCF_CS_CSMR1 = MCF_CS_CSMR_BAM_1M | MCF_CS_CSMR_V;
    
    /* 
	 * ChipSelect 0 - External Flash 
	 */ 
	MCF_CS_CSAR0 = MCF_CS_CSAR_BA(EXT_FLASH_ADDRESS);
	MCF_CS_CSCR0 = (0
		| MCF_CS_CSCR_IWS(6)
		| MCF_CS_CSCR_AA
		| MCF_CS_CSCR_PS_16);
	MCF_CS_CSMR0 = MCF_CS_CSMR_BAM_2M | MCF_CS_CSMR_V;
}
/********************************************************************/
