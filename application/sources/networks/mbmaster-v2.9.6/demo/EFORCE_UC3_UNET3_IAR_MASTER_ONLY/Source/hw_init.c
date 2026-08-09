/***********************************************************************
    Initialization of the hardware-dependent
 ***********************************************************************/

#include "kernel.h"
#include "MB9BFxxx.h"

extern void _ddr_m_systick_init(void);
extern void _ddr_mb9_uart_init(void);
extern int _wfi_entry(void);

/***********************************************
  Initialize Cortex-M peripherals
 ***********************************************/
extern UW __vector_table[];
void cortex_m_init_peripheral(void)
{
    INT inttype;
    INT i;

    inttype = REG_ICNTTYPE + 1;
    for (i = 0; i < inttype; i++) {
        REG_ANVIC.CLRENA[i] = 0xFFFFFFFF;
        REG_ANVIC.CLRPEND[i] = 0xFFFFFFFF;
    }

    for (i = 0; i < (inttype * 8); i++) {
        REG_ANVIC.IRQPRI[i] = 0xFFFFFFFF;
    }
    REG_SNVIC.APPIRCTR = 0x05FA0000 + 0x0300;   /* 4bit pre-emption priority */
    REG_SNVIC.VECTTBL = __vector_table;
    REG_SNVIC.SYSPRI[0] = 0x00000000;
    REG_SNVIC.SYSPRI[1] = 0x00000000;
    REG_SNVIC.SYSPRI[2] = 0x00000000;
    REG_SNVIC.CFGCTR |= 0x00000200;
}

/***********************************************
        Initialize MB9BFxxx's peripherals
 ***********************************************/
void init_peripheral(void)
{
    /* Initialize  */
    REG_CLK.SCM_CTL &= ~0x000000FA;
    REG_CLK.CSW_TMR &= ~0x0000007F;
    REG_CLK.PSW_TMR &= ~0x00000007;

    /* Setup each bus prescaler */
    REG_CLK.BSC_PSR = 0x00000000;       /* Base Clock Prescaler */
    REG_CLK.APBC0_PSR = 0x00000001;     /* APB0 Clock Prescaler */
    REG_CLK.APBC1_PSR = 0x00000081;     /* APB1 Clock Prescaler */
    REG_CLK.APBC2_PSR = 0x00000081;     /* APB2 Clock Prescaler */
    REG_CLK.TTC_PSR = 0x00000000;       /* Trace Clock Prescaler*/

    REG_CLK.INT_CLR |= 0x00000007;      /* Interrupt clear for Clock setup */

    REG_CLK.CSW_TMR |= 0x0000000A;      /* Main waiting stability time(4.0ms)*/
    REG_CLK.INT_ENR |= 0x00000001;      /* Interrupt enable for Main Clock */

    REG_CLK.SCM_CTL |= 0x00000002;      /* Main Clock Enable */
    while((REG_CLK.SCM_STR & 0x00000002) != 0x00000002);

    REG_CLK.PSW_TMR |= 0x00000003;      /* PLL waiting stability time(1.02ms)*/
    REG_CLK.INT_ENR |= 0x00000004;      /* Interrupt enable for PLL  Clock */

    REG_CLK.PLL_CTL1 = 0x00000001;      /* PLL control register K,M */
    REG_CLK.PLL_CTL2 = 0x00000023;      /* PLL control register N */

    REG_CLK.SCM_CTL |= 0x00000010;      /* PLL Clock Enable */
    while((REG_CLK.SCM_STR & 0x00000010) != 0x00000010);

    REG_CLK.SCM_CTL |= 0x00000040;      /* PLL Clock Output for Master clock */
    while((REG_CLK.SCM_STR & 0x00000040) != 0x00000040);

}

/***********************************************
  Device driver Initialize entry
 ***********************************************/
void _ddr_init(void)
{
    _ddr_m_systick_init();
    _ddr_mb9_uart_init();
}

/***********************************************
  Sleep Mode for Inner idle function
 ***********************************************/
void _wfi_mode_slp(void)
{
    _wfi_entry();
}

/* end */
