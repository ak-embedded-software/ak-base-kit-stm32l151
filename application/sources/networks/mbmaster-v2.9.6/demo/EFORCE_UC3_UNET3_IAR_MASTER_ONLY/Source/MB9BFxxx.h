/****************************************************************************
       Micro C Cube Compact, DEVICE DRIVER
       Fujitsu Semiconductor MB9BFxxx (Cortex M3 core) registers definitions

       Copyright (c)  2010-2012, eForce Co., Ltd.  All rights reserved.

       Version Information 
             2010.11.18: Created.
             2011.04.08: Additional of the CRTRMM at REG_FLASH register.
                         Modification of the IO Selector for ch0-ch3 register.
                         Modification of the IO Selector for ch4-ch7 register.
                         Modification of the Software-based Simulation Starup register.
                         Additional of the REG_12ADC0,REG_12ADC1,REG_12ADC2 register.
                         Modification of the REG_GPIO register.
                         Modification of the REG_USB register.
                         Correction of the REG_DTIM register.
                         Correction of the REG_PPG register.
                         Correction of the REG_WCT register.
                         Correction of the REG_USB0,REG_USB1 register.
                         Correction of the REG_DMAC register.
                         Correction of the REG_CAN0,REG_CAN1 register.
             2011.07.21: Additional of the DSPS_CTL at REG_CLK.
                         Change of Base address at REG_BTS07.
                         Additional of the ODDPKS at INTRR.
                         Additional of the DSPS_CTL at REG_CLK.
                         Additional of the PFRE,PCRE,DDRE,PDIRE,PDORE,EPFR11 at REG_GPIO.
                         Additional of the UPCR5 at REG_USBC.
             2011.08.29: Additional of the REG_MFT2 for TYPE2.
                         Modification of the REG_PPG register for TYPE2.
                         Additional of the REG_BTIM8-15 for TYPE2.
                         Additional of the REG_BTS811,REG_BTS1215 for TYPE2.
                         Additional of the REG_QPRC2 for TYPE2.
                         Modification of the REG_EXTI register for TYPE2.
                         Modification of the INTRR register for TYPE2.
                         Modification of the REG_GPIO register for TYPE2.
                         Additional of the REG_DS for TYPE2.
                         Additional of the REG_NFC for TYPE2.
                         Additional of the REG_RTC for TYPE2.
                         Modification of the REG_EXTB register for TYPE2.
                         Modification of the REG_CAN register for TYPE2.
             2011.09.16: Additional of the QRCRR,QPCRR at REG_QPCR.
                         Correction of the REG_12ADC register.
                         Additional of the EXC02MON at INTRR.
                         Additional of the ATIM0-7 at REG_EXTB.
             2012.02.09: Additional of the FBFCR at REG_FLASH.
                         Change from BTSEL4567 to BTSEL89AB at REG_BTS811.
                         Change from BTSEL4567 to BTSELCDEF at REG_BTS1215.
                         Change Reserved spacesize from 767 to 63 REG_BTS07.
                         Additional of IRQCMODE and RCINTSEL0-7 at INTRR.
                         Additional of DSRAMR at REG_DS.
                         Add TREQR1/2,NEWDT1/2,INTPND1/2,MSGVAL1/2 at REG_CAN0/1.
                         Additional of REG_ETH_MODE,REG_ETH0,REG_ETH0 register.
             2012.03.06  Add Bit definitions for Ether Mode Register.
                         Additional of Bit definitions for Ether Clock Gating Register.
                         Additional of Bit definitions for USB/Ethernet clock Register.
             2012.07.18  Add IGBTC at REG_PPG.
                         Add PCR8 at REG_GPIO.
                         Add REG_CTL, RCK_CTL and change base address at REG_DS.
                         Add IRQ_Flash(IRQ47) in IRQ Interrupt vector table. 
             2014.02.03: Add REG_UID and REG_10DAC.
             2014.02.03: Definition size was changed according to the peripheral manual, 
                         for example, UW is set to UH or UB. 
                         A change part is the following.
                         "REG_CLK, REG_MFT, REG_PPG, REG_BTIM, REG_BTS07, REG_QPRC" 
                         "REG_12ADC, REG_EXTI, REG_INTRR, REG_LVD, REG_DS"
                         "REG_USBC, REG_CANP, REG_CRC, REG_WCT, REG_USB"
             2014.02.26: Add EPFR16,EPFR17,EPFR18 at REG_GPIO.
                         Correction of "struct t_canp" at REG_CANP.
                         Change from UW to UH at "I2CDNF" REG_NFC.
****************************************************************************/

#ifndef _MB9BFxxx_H_
#define _MB9BFxxx_H_

#include "Cortex-M3.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Flash Memory registers */

struct t_flash {
    UW      FASZR;      /* Flash access size register                   */
    UW      FRWTR;      /* Flash read wait register                     */
    UW      FSTR;       /* Flash status register                        */
    UW      fill1;      /* Reserved space                               */
    UW      FSYNDN;     /* Flash Sync Down Register                     */
    UW      FBFCR;      /* Flash Buffer Control Register                */
    UW      fill2[58];  /* Reserved space                               */
    UW      CRTRMM;     /* CR Trimming Data Mirror Register             */
};

#define REG_FLASH     (*(volatile struct t_flash *)0x40000000)

/* Unique ID registers */

struct t_uid {
    UW      UIDR0;      /* Unique ID 0 registers                        */
    UW      UIDR1;      /* Unique ID 1 registers                        */
};

#define REG_UID       (*(volatile struct t_uid *)0x40000200)

/* Clock and Reset control registers */

struct t_clk {
    UB      SCM_CTL;    /* System Clock Mode control register           */
    UB      fill1[3];   /* Reserved space                               */
    UB      SCM_STR;    /* System Clock Mode state register             */
    UB      fill2[3];   /* Reserved space                               */
    UW      STB_CTL;    /* Stanby Mode control register                 */
    UH      RST_STR;    /* ReSeT STatus Register                        */
    UH      fill3[1];   /* Reserved space                               */
    UB      BSC_PSR;    /* Base Clock Prescaler register                */
    UB      fill4[3];   /* Reserved space                               */
    UB      APBC0_PSR;  /* APB0 Prescaler register                      */
    UB      fill5[3];   /* Reserved space                               */
    UB      APBC1_PSR;  /* APB1 Prescaler register                      */
    UB      fill6[3];   /* Reserved space                               */
    UB      APBC2_PSR;  /* APB2 Prescaler register                      */
    UB      fill7[3];   /* Reserved space                               */
    UB      SWC_PSR;    /* SW-WDGT Clock Prescaler register             */
    UB      fill8[3];   /* Reserved space                               */
    UW      fill9;      /* Reserved space                               */
    UB      TTC_PSR;    /* Trace Clock Prescalor register               */
    UB      fill10[3];  /* Reserved space                               */
    UW      fill10_2;   /* Reserved space                               */
    UB      CSW_TMR;    /* Clock stability waiting register             */
    UB      fill11[3];  /* Reserved space                               */
    UB      PSW_TMR;    /* PLL Clock stability waiting register         */
    UB      fill12[3];  /* Reserved space                               */
    UB      PLL_CTL1;   /* PLL Clock control register K,M               */
    UB      fill13[3];  /* Reserved space                               */
    UB      PLL_CTL2;   /* PLL Clock control register N                 */
    UB      fill14[3];  /* Reserved space                               */
    UH      CSV_CTL;    /* CSV control register                         */
    UH      fill15[1];  /* Reserved space                               */
    UB      CSV_STR;    /* CSV status  register                         */
    UB      fill16[3];  /* Reserved space                               */
    UH      FCSWH_CTL;  /* Frequency detection window set register (high-order) */
    UH      fill17[1];  /* Reserved space                               */
    UH      FCSWL_CTL;  /* Frequency detection window set register (low-order)  */
    UH      fill18[1];  /* Reserved space                               */
    UH      FCSWD_CTL;  /* Frequency detection count re                 */
    UH      fill19[1];  /* Reserved space                               */
    UB      DBWDT_CTL;  /* Debug brake Watchdog timer control register  */
    UB      fill20[3];  /* Reserved space                               */
    UW      fill21;     /* Reserved space                               */
    UW      fill22;     /* Reserved space                               */
    UB      INT_ENR;    /* Interrupt enable register                    */
    UB      fill23[3];  /* Reserved space                               */
    UB      INT_STR;    /* Interrupt status register                    */
    UB      fill24[3];  /* Reserved space                               */
    UB      INT_CLR;    /* Interrupt clear register                     */
};

#define REG_CLK     (*(volatile struct t_clk *)0x40010000)

/* Hardware watchdog register */

struct t_hwdt {
    UW      WDG_LDR;    /* Hardware watchdog load register              */
    UW      WDG_VLR;    /* Hardware watchdog value register             */
    UW      WDG_CTL;    /* Hardware watchdog control register           */
    UW      WDG_ICL;    /* Hardware watchdog clear register             */
    UW      WDG_RIS;    /* Hardware watchdog interrupt status register  */
    UW      fill1[763]; /* Reserved space                               */
    UW      WDG_LCK;    /* Hardware watchdog lock register              */
};

#define REG_HWDT     (*(volatile struct t_hwdt *)0x40011000)

/* Software watchdog register */

struct t_swdt {
    UW      WDG_LDR;    /* Software watchdog load register              */
    UW      WDG_VLR;    /* Software watchdog value register             */
    UW      WDG_CTL;    /* Software watchdog control register           */
    UW      WDG_ICL;    /* Software watchdog clear register             */
    UW      WDG_RIS;    /* Software watchdog interrupt status register  */
    UW      fill1[763]; /* Reserved space                               */
    UW      WDG_LCK;    /* Software watchdog lock register              */
};

#define REG_SWDT     (*(volatile struct t_swdt *)0x40012000)

/* Dual timer register */

struct t_dtim {
    UW      TIM1_LDR;   /* Dual timer1 load register                    */
    UW      TIM1_VLR;   /* Dual timer1 value register                   */
    UW      TIM1_CTR;   /* Dual timer1 control register                 */
    UW      TIM1_ICL;   /* Dual timer1 interrupt clear register         */
    UW      TIM1_RIS;   /* Dual timer1 interrupt status register        */
    UW      TIM1_MIS;   /* Dual timer1 Mask interrupt status register   */
    UW      TIM1_BGL;   /* Dual timer1 Back background load register    */
    UW      fill1;      /* Reserved space                               */
    UW      TIM2_LDR;   /* Dual timer2 load register                    */
    UW      TIM2_VLR;   /* Dual timer2 value register                   */
    UW      TIM2_CTR;   /* Dual timer2 control register                 */
    UW      TIM2_ICL;   /* Dual timer2 interrupt clear register         */
    UW      TIM2_RIS;   /* Dual timer2 interrupt status register        */
    UW      TIM2_MIS;   /* Dual timer2 Mask interrupt status register   */
    UW      TIM2_BGL;   /* Dual timer2 Back background load register    */
};

#define REG_DTIM    (*(volatile struct t_dtim *)0x40015000)

/* Multi Function timer register */

struct t_mft {
    UH      OCCP0;      /* OCU ch0 Comparison value store register      */
    UH      fill1[1];   /* Reserved space                               */
    UH      OCCP1;      /* OCU ch1 Comparison value store register      */
    UH      fill2[1];   /* Reserved space                               */
    UH      OCCP2;      /* OCU ch2 Comparison value store register      */
    UH      fill3[1];   /* Reserved space                               */
    UH      OCCP3;      /* OCU ch3 Comparison value store register      */
    UH      fill4[1];   /* Reserved space                               */
    UH      OCCP4;      /* OCU ch4 Comparison value store register      */
    UH      fill5[1];   /* Reserved space                               */
    UH      OCCP5;      /* OCU ch5 Comparison value store register      */
    UH      fill6[1];   /* Reserved space                               */
    UB      OCSA10;     /* OCU ch.1, ch.0 control register A            */
    UB      OCSB10;     /* OCU ch.1, ch.0 control register B            */
    UB      fill7[2];   /* Reserved space                               */
    UB      OCSA32;     /* OCU ch.3, ch.2 control register A            */
    UB      OCSB32;     /* OCU ch.3, ch.2 control register B            */
    UB      fill8[2];   /* Reserved space                               */
    UB      OCSA54;     /* OCU ch.5, ch.4 control register A            */
    UB      OCSB54;     /* OCU ch.5, ch.4 control register B            */
    UB      fill9[2];   /* Reserved space                               */
    UB      fill10;     /* Reserved space                               */
    UB      OCSC;       /* OCU ch.5-ch.0 control register C             */
    UB      fill11[2];  /* Reserved space                               */
    UH      TCCP0;      /* FRT ch.0 Periodic set register               */
    UH      fill12[1];  /* Reserved space                               */
    UH      TCDT0;      /* FRT ch.0 cont value register                 */
    UH      fill13[1];  /* Reserved space                               */
    UH      TCSA0;      /* FRT ch.0 control register A                  */
    UH      fill14[1];  /* Reserved space                               */
    UH      TCSB0;      /* FRT ch.0 control register B                  */
    UH      fill15[1];  /* Reserved space                               */
    UH      TCCP1;      /* FRT ch.1 Periodic set register               */
    UH      fill16[1];  /* Reserved space                               */
    UH      TCDT1;      /* FRT ch.1 cont value register                 */
    UH      fill17[1];  /* Reserved space                               */
    UH      TCSA1;      /* FRT ch.1 control register A                  */
    UH      fill18[1];  /* Reserved space                               */
    UH      TCSB1;      /* FRT ch.1 control register B                  */
    UH      fill19[1];  /* Reserved space                               */
    UH      TCCP2;      /* FRT ch.2 Periodic set register               */
    UH      fill20[1];  /* Reserved space                               */
    UH      TCDT2;      /* FRT ch.2 cont value register                 */
    UH      fill21[1];  /* Reserved space                               */
    UH      TCSA2;      /* FRT ch.2 control register A                  */
    UH      fill22[1];  /* Reserved space                               */
    UH      TCSB2;      /* FRT ch.2 control register B                  */
    UH      fill23[1];  /* Reserved space                               */
    UB      OCFS10;     /* OCU ch.1, ch.0 Connection FRT select register*/
    UB      OCFS32;     /* OCU ch.3, ch.2 Connection FRT select register*/
    UB      fill24[2];   /* Reserved space                               */
    UB      OCFS54;     /* OCU ch.5, ch.4 Connection FRT select register*/
    UB      fill25[3];   /* Reserved space                               */
    UB      ICSF10;     /* ICU ch.1, ch.0 Connection FRT select register*/
    UB      ICSF32;     /* ICU ch.3, ch.2 Connection FRT select register*/
    UB      fill26[2];   /* Reserved space                               */
    UW      fill27;      /* Reserved space                               */
    UH      ICCP0;      /* ICU ch0 Capture value store register         */
    UH      fill28[1];  /* Reserved space                               */
    UH      ICCP1;      /* ICU ch1 Capture value store register         */
    UH      fill29[1];  /* Reserved space                               */
    UH      ICCP2;      /* ICU ch2 Capture value store register         */
    UH      fill30[1];  /* Reserved space                               */
    UH      ICCP3;      /* ICU ch3 Capture value store register         */
    UH      fill31[1];  /* Reserved space                               */
    UB      ICSA10;     /* ICU ch.1, ch.0 control register A            */
    UB      ICSB10;     /* ICU ch.1, ch.0 control register B            */
    UB      fill32[2];  /* Reserved space                               */
    UB      ICSA32;     /* ICU ch.3, ch.2 control register A            */
    UB      ICSB32;     /* ICU ch.3, ch.2 control register B            */
    UB      fill33[2];  /* Reserved space                               */
    UH      WFTM10;     /* WFG ch.10 timer value register               */
    UH      fill34[1];  /* Reserved space                               */
    UH      WFTM32;     /* WFG ch.32 timer value register               */
    UH      fill35[1];  /* Reserved space                               */
    UH      WFTM54;     /* WFG ch.54 timer value register               */
    UH      fill36[1];  /* Reserved space                               */
    UH      WFSA10;     /* WFG ch.10 control register A                 */
    UH      fill37[1];  /* Reserved space                               */
    UH      WFSA32;     /* WFG ch.32 control register A                 */
    UH      fill38[1];  /* Reserved space                               */
    UH      WFSA54;     /* WFG ch.54 control register A                 */
    UH      fill39[1];  /* Reserved space                               */
    UH      WFIR;       /* WFG interrupt control register               */
    UH      fill40[1];  /* Reserved space                               */
    UH      NZCL;       /* NZCL control register                        */
    UH      fill41[1];  /* Reserved space                               */
    UH      ACCP0;      /* ADCMP ch.0 Comparison value store register    */
    UH      fill42[1];  /* Reserved space                               */
    UH      ACCPDN0;    /* ADCMP ch.0 Comparison value store register(Exclusive of down count direction)*/
    UH      fill43[1];  /* Reserved space                               */
    UH      ACCP1;      /* ADCMP ch.1 Comparison value store register    */
    UH      fill44[1];  /* Reserved space                               */
    UH      ACCPDN1;    /* ADCMP ch.1 Comparison value store register(Exclusive of down count direction)*/
    UH      fill45[1];  /* Reserved space                               */
    UH      ACCP2;      /* ADCMP ch.2 Comparison value store register    */
    UH      fill46[1];  /* Reserved space                               */
    UH      ACCPD2;    /* ADCMP ch.2 Comparison value store register(Exclusive of down count direction)*/
    UH      fill47[1];  /* Reserved space                               */
    UB      ACSB;       /* ADCMP ch.2-ch.0 control register B            */
    UB      fill48[3];   /* Reserved space                               */
    UH      ACSA;       /* ADCMP ch.2-ch.0 control register A            */
    UH      fill49[1];  /* Reserved space                               */
    UH      ATSA;       /* ADC Starting factor select register           */
};

#define REG_MFT0    (*(volatile struct t_mft *)0x40020000)
#define REG_MFT1    (*(volatile struct t_mft *)0x40021000)
#define REG_MFT2    (*(volatile struct t_mft *)0x40022000)

/* PPG register */

struct t_ppg {
    UB      fill1;      /* Reserved space                               */
    UB      TTCR0;      /* PPG Starting trigger control register 0      */
    UB      fill2[2];   /* Reserved space                               */
    UW      fill3;      /* Reserved space                               */
    UB      fill4;      /* Reserved space                               */
    UB      COMP0;      /* PPG compare register 0                       */
    UB      fill5[2];   /* Reserved space                               */
    UB      COMP2;      /* PPG compare register 2                       */
    UB      fill6[3];   /* Reserved space                               */
    UB      fill7;      /* Reserved space                               */
    UB      COMP4;      /* PPG compare register 4                       */
    UB      fill8[2];   /* Reserved space                               */
    UB      COMP6;      /* PPG compare register 6                       */
    UB      fill9[3];   /* Reserved space                               */
    UW      fill10[2];  /* Reserved space                               */
    UB      fill11;     /* Reserved space                               */
    UB      TTCR1;      /* PPG Starting trigger control register 1      */
    UB      fill12[2];  /* Reserved space                               */
    UW      fill13;     /* Reserved space                               */
    UB      fill14;     /* Reserved space                               */
    UB      COMP1;      /* PPG compare register 1                       */
    UB      fill15[2];  /* Reserved space                               */
    UB      COMP3;      /* PPG compare register 3                       */
    UB      fill16[3];  /* Reserved space                               */
    UB      fill17;     /* Reserved space                               */
    UB      COMP5;      /* PPG compare register 5                       */
    UB      fill18[2];  /* Reserved space                               */
    UB      COMP7;      /* PPG compare register 7                       */
    UB      fill19[3];  /* Reserved space                               */
    UW      fill20[2];  /* Reserved space                               */
    UB      fill21;     /* Reserved space                               */
    UB      TTCR2;      /* PPG Starting trigger control register 2      */
    UB      fill22[2];  /* Reserved space                               */
    UW      fill23;     /* Reserved space                               */
    UB      fill24;     /* Reserved space                               */
    UB      COMP8;      /* PPG compare register 8                       */
    UB      fill25[2];  /* Reserved space                               */
    UB      COMP10;     /* PPG compare register 10                      */
    UB      fill26[3];  /* Reserved space                               */
    UB      fill27;     /* Reserved space                               */
    UB      COMP12;     /* PPG compare register 12                      */
    UB      fill28[2];  /* Reserved space                               */
    UB      COMP14;     /* PPG compare register 14                      */
    UB      fill29[3];  /* Reserved space                               */
    UW      fill30[42]; /* Reserved space                               */
    UH      TRG;        /* PPG starting register                        */
    UH      fill81[1];  /* Reserved space                               */
    UH      REVC;       /* Output reversal register                     */
    UH      fill82[1];  /* Reserved space                               */
    UW      fill31[14]; /* Reserved space                               */
    UH      TRG1;       /* PPG starting register 1                      */
    UH      fill83[1];  /* Reserved space                               */
    UH      REVC1;      /* Output reversal register 1                   */
    UH      fill84[1];  /* Reserved space                               */
    UW      fill32[46]; /* Reserved space                               */
    UB      PPGC1;      /* PPG Operation Mode control register 1        */
    UB      PPGC0;      /* PPG Operation Mode control register 0        */
    UB      fill33[2];  /* Reserved space                               */
    UB      PPGC3;      /* PPG Operation Mode control register 3        */
    UB      PPGC2;      /* PPG Operation Mode control register 2        */
    UB      fill34[2];  /* Reserved space                               */
    UB      PRLL0;      /* PPG reload register L0                       */
    UB      PRLH0;      /* PPG reload register H0                       */
    UB      fill35[2];  /* Reserved space                               */
    UB      PRLL1;      /* PPG reload register L1                       */
    UB      PRLH1;      /* PPG reload register H1                       */
    UB      fill36[2];  /* Reserved space                               */
    UB      PRLL2;      /* PPG reload register L2                       */
    UB      PRLH2;      /* PPG reload register H2                       */
    UB      fill37[2];  /* Reserved space                               */
    UB      PRLL3;      /* PPG reload register L3                       */
    UB      PRLH3;      /* PPG reload register H3                       */
    UB      fill38[2];  /* Reserved space                               */
    UB      GATEC0;     /* GATE Function control register 0             */
    UB      fill39[3];  /* Reserved space                               */
    UW      fill40[9];  /* Reserved space                               */
    UB      PPGC5;      /* PPG Operation Mode control register 5        */
    UB      PPGC4;      /* PPG Operation Mode control register 4        */
    UB      fill41[2];  /* Reserved space                               */
    UB      PPGC7;      /* PPG Operation Mode control register 7        */
    UB      PPGC6;      /* PPG Operation Mode control register 6        */
    UB      fill42[2];  /* Reserved space                               */
    UB      PRLL4;      /* PPG reload register L4                       */
    UB      PRLH4;      /* PPG reload register H4                       */
    UB      fill43[2];  /* Reserved space                               */
    UB      PRLL5;      /* PPG reload register L5                       */
    UB      PRLH5;      /* PPG reload register H5                       */
    UB      fill44[2];  /* Reserved space                               */
    UB      PRLL6;      /* PPG reload register L6                       */
    UB      PRLH6;      /* PPG reload register H6                       */
    UB      fill45[2];  /* Reserved space                               */
    UB      PRLL7;      /* PPG reload register L7                       */
    UB      PRLH7;      /* PPG reload register H7                       */
    UB      fill46[2];  /* Reserved space                               */
    UB      GATEC4;     /* GATE Function control register 4             */
    UB      fill47[3];  /* Reserved space                               */
    UW      fill48[9];  /* Reserved space                               */
    UB      PPGC9;      /* PPG Operation Mode control register 9        */
    UB      PPGC8;      /* PPG Operation Mode control register 8        */
    UB      fill49[2];  /* Reserved space                               */
    UB      PPGC11;     /* PPG Operation Mode control register 11       */
    UB      PPGC10;     /* PPG Operation Mode control register 10       */
    UB      fill50[2];  /* Reserved space                               */
    UB      PRLL8;      /* PPG reload register L8                       */
    UB      PRLH8;      /* PPG reload register H8                       */
    UB      fill51[2];  /* Reserved space                               */
    UB      PRLL9;      /* PPG reload register L9                       */
    UB      PRLH9;      /* PPG reload register H9                       */
    UB      fill52[2];  /* Reserved space                               */
    UB      PRLL10;     /* PPG reload register L10                      */
    UB      PRLH10;     /* PPG reload register H10                      */
    UB      fill53[2];  /* Reserved space                               */
    UB      PRLL11;     /* PPG reload register L11                      */
    UB      PRLH11;     /* PPG reload register H11                      */
    UB      fill54[2];  /* Reserved space                               */
    UB      GATEC8;     /* GATE Function control register 8             */
    UB      fill55[3];  /* Reserved space                               */
    UW      fill56[9];  /* Reserved space                               */
    UB      PPGC13;     /* PPG Operation Mode control register 13       */
    UB      PPGC12;     /* PPG Operation Mode control register 12       */
    UB      fill57[2];  /* Reserved space                               */
    UB      PPGC15;     /* PPG Operation Mode control register 15       */
    UB      PPGC14;     /* PPG Operation Mode control register 14       */
    UB      fill58[2];  /* Reserved space                               */
    UB      PRLL12;     /* PPG reload register L12                      */
    UB      PRLH12;     /* PPG reload register H12                      */
    UB      fill59[2];  /* Reserved space                               */
    UB      PRLL13;     /* PPG reload register L13                      */
    UB      PRLH13;     /* PPG reload register H13                      */
    UB      fill60[2];  /* Reserved space                               */
    UB      PRLL14;     /* PPG reload register L14                      */
    UB      PRLH14;     /* PPG reload register H14                      */
    UB      fill61[2];  /* Reserved space                               */
    UB      PRLL15;     /* PPG reload register L15                      */
    UB      PRLH15;     /* PPG reload register H15                      */
    UB      fill62[2];  /* Reserved space                               */
    UB      GATEC12;    /* GATE Function control register 12            */
    UB      fill63[3];  /* Reserved space                               */
    UW      fill64[9];  /* Reserved space                               */
    UB      PPGC17;     /* PPG Operation Mode control register 17       */
    UB      PPGC16;     /* PPG Operation Mode control register 16       */
    UB      fill65[2];  /* Reserved space                               */
    UB      PPGC19;     /* PPG Operation Mode control register 19       */
    UB      PPGC18;     /* PPG Operation Mode control register 18       */
    UB      fill66[2];  /* Reserved space                               */
    UB      PRLL16;     /* PPG reload register L16                      */
    UB      PRLH16;     /* PPG reload register H16                      */
    UB      fill67[2];  /* Reserved space                               */
    UB      PRLL17;     /* PPG reload register L17                      */
    UB      PRLH17;     /* PPG reload register H17                      */
    UB      fill68[2];  /* Reserved space                               */
    UB      PRLL18;     /* PPG reload register L18                      */
    UB      PRLH18;     /* PPG reload register H18                      */
    UB      fill69[2];  /* Reserved space                               */
    UB      PRLL19;     /* PPG reload register L19                      */
    UB      PRLH19;     /* PPG reload register H19                      */
    UB      fill70[2];  /* Reserved space                               */
    UB      GATEC16;    /* GATE Function control register 16            */
    UB      fill71[3];  /* Reserved space                               */
    UW      fill72[9];  /* Reserved space                               */
    UB      PPGC21;     /* PPG Operation Mode control register 21       */
    UB      PPGC20;     /* PPG Operation Mode control register 20       */
    UB      fill73[2];  /* Reserved space                               */
    UB      PPGC23;     /* PPG Operation Mode control register 23       */
    UB      PPGC22;     /* PPG Operation Mode control register 22       */
    UB      fill74[2];  /* Reserved space                               */
    UB      PRLL20;     /* PPG reload register L20                      */
    UB      PRLH20;     /* PPG reload register H20                      */
    UB      fill75[2];  /* Reserved space                               */
    UB      PRLL21;     /* PPG reload register L21                      */
    UB      PRLH21;     /* PPG reload register H21                      */
    UB      fill76[2];  /* Reserved space                               */
    UB      PRLL22;     /* PPG reload register L22                      */
    UB      PRLH22;     /* PPG reload register H22                      */
    UB      fill77[2];  /* Reserved space                               */
    UB      PRLL23;     /* PPG reload register L23                      */
    UB      PRLH23;     /* PPG reload register H23                      */
    UB      fill78[2];  /* Reserved space                               */
    UB      GATEC20;    /* GATE Function control register 20            */
    UB      fill79[3];  /* Reserved space                               */
    UW      fill80[9];  /* Reserved space                               */
    UB      IGBTC;      /* IGBT mode control register                   */
};

#define REG_PPG     (*(volatile struct t_ppg *)0x40024000)

/* Base Timer register */

struct t_btim {
    UH      PCSR;       /* PWM Periodic set register                    */
    UH      fill1[1];   /* Reserved space                               */
    UH      PDUT;       /* PWM Duty set register                        */
    UH      fill2[1];   /* Reserved space                               */
    UH      TMR;        /* Timer register                               */
    UH      fill3[1];   /* Reserved space                               */
    UH      TMCR;       /* PWM Timer control register at Timer select   */
    UH      fill4[1];   /* Reserved space                               */
    UB      STC;        /* Status control register                      */
    UB      TMCR2;      /* PWM Timer control register at Timer select   */
};

#define REG_BTIM0   (*(volatile struct t_btim *)0x40025000)
#define REG_BTIM1   (*(volatile struct t_btim *)0x40025040)
#define REG_BTIM2   (*(volatile struct t_btim *)0x40025080)
#define REG_BTIM3   (*(volatile struct t_btim *)0x400250C0)
#define REG_BTIM4   (*(volatile struct t_btim *)0x40025200)
#define REG_BTIM5   (*(volatile struct t_btim *)0x40025240)
#define REG_BTIM6   (*(volatile struct t_btim *)0x40025280)
#define REG_BTIM7   (*(volatile struct t_btim *)0x400252C0)
#define REG_BTIM8   (*(volatile struct t_btim *)0x40025400)
#define REG_BTIM9   (*(volatile struct t_btim *)0x40025440)
#define REG_BTIM10  (*(volatile struct t_btim *)0x40025480)
#define REG_BTIM11  (*(volatile struct t_btim *)0x400254C0)
#define REG_BTIM12  (*(volatile struct t_btim *)0x40025600)
#define REG_BTIM13  (*(volatile struct t_btim *)0x40025640)
#define REG_BTIM14  (*(volatile struct t_btim *)0x40025680)
#define REG_BTIM15  (*(volatile struct t_btim *)0x400256C0)

/* IO Selector for ch0-ch3 */

struct t_bts03 {
    UB      fill;       /* Reserved space                               */
    UB      BTSEL0123;  /* Input-and-output select register             */
};

#define REG_BTS03   (*(volatile struct t_bts03 *)0x40025100)

/* IO Selector for ch4-ch7 */

struct t_bts47 {
    UB      fill;       /* Reserved space                               */
    UB      BTSEL4567;  /* Input-and-output select register             */
};

#define REG_BTS47   (*(volatile struct t_bts47 *)0x40025300)

/* IO Selector for ch8-ch11 */

struct t_bts811 {
    UB      fill;       /* Reserved space                               */
    UB      BTSEL89AB;  /* Input-and-output select register             */
};

#define REG_BTS811   (*(volatile struct t_bts811 *)0x40025500)

/* IO Selector for ch12-ch15 */

struct t_bts1215 {
    UB      fill;       /* Reserved space                               */
    UB      BTSELCDEF;  /* Input-and-output select register             */
};

#define REG_BTS1215   (*(volatile struct t_bts1215 *)0x40025700)

/* Software-based Simulation Starup */

struct t_bts07 {
    UW      fill[63];   /* Reserved space                               */
    UH      BTSSSR;     /* Simultaneous soft start register             */
};

#define REG_BTS07   (*(volatile struct t_bts07 *)0x40025F00)

/* Quad Position & Revolution Counter register */

struct t_qprc {
    UH      QPCR;       /* Quad counter position count register         */
    UH      fill1[1];   /* Reserved space                               */
    UH      QRCR;       /* Quad counter circulation count register      */
    UH      fill2[1];   /* Reserved space                               */
    UH      QPCCR;      /* Quad counter position counter comparison register */
    UH      fill3[1];   /* Reserved space                               */
    UH      QPRCR;      /* Quad counter position & rotation counter comparison register */
    UH      fill4[1];   /* Reserved space                               */
    UH      QMPR;       /* Quad counter maximum position register       */
    UH      fill5[1];   /* Reserved space                               */
    UB      QICRL;      /* Quad counter interrupt control register(low-order)  */
    UB      QICRH;      /* Quad counter interrupt control register(high-order) */
    UB      fill6[2];   /* Reserved space                               */
    UB      QCRL;       /* Quad counter control register(low-order)     */
    UB      QCRH;       /* Quad counter control register(high-order)    */
    UB      fill7[2];   /* Reserved space                               */
    UH      QECR;       /* Quad counter extended control register       */
    UH      fill8[1];   /* Reserved space                               */
    UW      fill[7];    /* Reserved space                               */
    UH      QRCRR;      /* Quad counter rotate count indication bit     */
    UH      QPCRR;      /* Quad counter position count indication bit   */
};

#define REG_QPRC0   (*(volatile struct t_qprc *)0x40026000)
#define REG_QPRC1   (*(volatile struct t_qprc *)0x40026040)
#define REG_QPRC2   (*(volatile struct t_qprc *)0x40026080)

/* 12bit A/D converter register (TYPE0,TYPE2,TYPE4)*/

struct t_12adc {
    UB      ADSR;       /* A/D status register                          */
    UB      ADCR;       /* A/D control register                         */
    UB      fill1[2];   /* Reserved space                               */
    UW      fill2;      /* Reserved space                               */
    UB      SFNS;       /* Scan conversion control register             */
    UB      SCCR;       /* Scan conversion FIFO stage control register  */
    UB      fill3[2];   /* Reserved space                               */
    UW      SCFD;       /* Scan conversion FIFO data register           */
    UB      SCIS2;      /* Scan conversion input select register(AN23-AN16) */
    UB      SCIS3;      /* Scan conversion input select register(AN31-AN24) */
    UB      fill4[2];   /* Reserved space                               */
    UB      SCIS0;      /* Scan conversion input select register(AN7-AN0)   */
    UB      SCIS1;      /* Scan conversion input select register(AN15-AN8)  */
    UB      fill5[2];   /* Reserved space                               */
    UB      PFNS;       /* Priority FIFO stage control register         */
    UB      PCCR;       /* Priority conversion control register         */
    UB      fill6[2];   /* Reserved space                               */
    UW      PCFD;       /* Priority conversion FIFO data register       */
    UB      PCIS;       /* Priority conversion input select register    */
    UB      fill7[3];   /* Reserved space                               */
    UB      CMPCR;      /* A/D Comparison control register              */
    UB      fill;       /* Reserved space                               */
    UH      CMPD;       /* A/D Comparison value setting register        */
    UB      ADSS2;      /* Sampling Time select register(TS23-TS16)     */
    UB      ADSS3;      /* Sampling Time select register(TS31-TS24)     */
    UB      fill9[2];   /* Reserved space                               */
    UB      ADSS0;      /* Sampling Time select register(AN7-AN0)       */
    UB      ADSS1;      /* Sampling Time select register(AN15-AN8)      */
    UB      fill10[2];  /* Reserved space                               */
    UB      ADST1;      /* Sampling Time control register(low-order)    */
    UB      ADST0;      /* Sampling Time control register(high-order)   */
    UB      fill11[2];  /* Reserved space                               */
    UB      ADCT;       /* Compare time set register                    */
    UB      fill12[3];  /* Reserved space                               */
    UB      PRTSL;      /* Priority conversion timer trigger select register */
    UB      SCTSL;      /* Scan conversion timer trigger select register*/
    UB      fill13[2];  /* Reserved space                               */
    UB      ADCEN;      /* A/D Operation enable set register            */
};

#define REG_12ADC0    (*(volatile struct t_12adc *)0x40027000)
#define REG_12ADC1    (*(volatile struct t_12adc *)0x40027100)
#define REG_12ADC2    (*(volatile struct t_12adc *)0x40027200)

/* 10bit D/AC converter register */

struct t_10dac {
    UH      DADR0;      /* D/A Control register0                        */
    UB      DACR0;      /* D/A Data register0                           */
    UB      fill0;      /* Reserved space                               */
    UH      DADR1;      /* D/A Control register1                        */
    UB      DACR1;      /* D/A Data register1                           */
    UB      fill1;      /* Reserved space                               */
};

#define REG_10DAC       (*(volatile struct t_10dac *)0x40028000)

/* High-speed CR Trimming function registers */

struct t_crt {
    UW      MCR_PSR;    /* High-speed CR oscillator Prescaler set register */
    UW      MCR_FTRM;   /* High-speed CR oscillator Frequency trimming register */
    UW      fill1;      /* Reserved space                               */
    UW      MCR_RLR;    /* High-speed CR oscillator Write protect register */
};

#define REG_CRT     (*(volatile struct t_crt *)0x4002E000)

/* External interrupt, NMI controller registers */

struct t_exti {
    UW      ENIR;       /* Enable Interrupt request Register            */
    UW      EIRR;       /* External Interrupt Request Register          */
    UW      EICL;       /* External Interrupt Clear register            */
    UW      ELVR;       /* External Interrupt LeVel Register            */
    UW      ELVR1;      /* External Interrupt LeVel Register 1          */
    UH      NMIRR;      /* Non Maskable Interrupt Request Register      */
    UH      fill1;      /* Reserved space                               */
    UH      NMICL;      /* Non Maskable Interrupt CLear register        */
    UH      fill2;      /* Reserved space                               */
};

#define REG_EXTI    (*(volatile struct t_exti *)0x40030000)

/* DMA Transmission request select register, Interrupt request read-out at once register */

struct t_intrr {
    UW      DRQSEL;     /* DMA Transmission request select register     */
    UW      fill1;      /* Reserved space                               */
    UB      fill2[3];   /* Reserved space                               */
    UB      ODDPKS;     /* USB odd packet size DMA enable register      */
    UB      IRQCMODE;   /* Interrupt factor vector relocate set register*/
    UB      fill3[3];   /* Reserved space                               */
    UW      EXC02MON;   /* Interrupt request read-out at once register EXC02 */
    UW      IRQMON00;   /* Interrupt request read-out at once register00*/
    UW      IRQMON01;   /* Interrupt request read-out at once register01*/
    UW      IRQMON02;   /* Interrupt request read-out at once register02*/
    UW      IRQMON03;   /* Interrupt request read-out at once register03*/
    UW      IRQMON04;   /* Interrupt request read-out at once register04*/
    UW      IRQMON05;   /* Interrupt request read-out at once register05*/
    UW      IRQMON06;   /* Interrupt request read-out at once register06*/
    UW      IRQMON07;   /* Interrupt request read-out at once register07*/
    UW      IRQMON08;   /* Interrupt request read-out at once register08*/
    UW      IRQMON09;   /* Interrupt request read-out at once register09*/
    UW      IRQMON10;   /* Interrupt request read-out at once register10*/
    UW      IRQMON11;   /* Interrupt request read-out at once register11*/
    UW      IRQMON12;   /* Interrupt request read-out at once register12*/
    UW      IRQMON13;   /* Interrupt request read-out at once register13*/
    UW      IRQMON14;   /* Interrupt request read-out at once register14*/
    UW      IRQMON15;   /* Interrupt request read-out at once register15*/
    UW      IRQMON16;   /* Interrupt request read-out at once register16*/
    UW      IRQMON17;   /* Interrupt request read-out at once register17*/
    UW      IRQMON18;   /* Interrupt request read-out at once register18*/
    UW      IRQMON19;   /* Interrupt request read-out at once register19*/
    UW      IRQMON20;   /* Interrupt request read-out at once register20*/
    UW      IRQMON21;   /* Interrupt request read-out at once register21*/
    UW      IRQMON22;   /* Interrupt request read-out at once register22*/
    UW      IRQMON23;   /* Interrupt request read-out at once register23*/
    UW      IRQMON24;   /* Interrupt request read-out at once register24*/
    UW      IRQMON25;   /* Interrupt request read-out at once register25*/
    UW      IRQMON26;   /* Interrupt request read-out at once register26*/
    UW      IRQMON27;   /* Interrupt request read-out at once register27*/
    UW      IRQMON28;   /* Interrupt request read-out at once register28*/
    UW      IRQMON29;   /* Interrupt request read-out at once register29*/
    UW      IRQMON30;   /* Interrupt request read-out at once register30*/
    UW      IRQMON31;   /* Interrupt request read-out at once register31*/
    UW      IRQMON32;   /* Interrupt request read-out at once register32*/
    UW      IRQMON33;   /* Interrupt request read-out at once register33*/
    UW      IRQMON34;   /* Interrupt request read-out at once register34*/
    UW      IRQMON35;   /* Interrupt request read-out at once register35*/
    UW      IRQMON36;   /* Interrupt request read-out at once register36*/
    UW      IRQMON37;   /* Interrupt request read-out at once register37*/
    UW      IRQMON38;   /* Interrupt request read-out at once register38*/
    UW      IRQMON39;   /* Interrupt request read-out at once register39*/
    UW      IRQMON40;   /* Interrupt request read-out at once register40*/
    UW      IRQMON41;   /* Interrupt request read-out at once register41*/
    UW      IRQMON42;   /* Interrupt request read-out at once register42*/
    UW      IRQMON43;   /* Interrupt request read-out at once register43*/
    UW      IRQMON44;   /* Interrupt request read-out at once register44*/
    UW      IRQMON45;   /* Interrupt request read-out at once register45*/
    UW      IRQMON46;   /* Interrupt request read-out at once register46*/
    UW      IRQMON47;   /* Interrupt request read-out at once register47*/
    UW      fill4[75];  /* Reserved space                               */
    UW      DRQSEL1;    /* DMA Transmission request select register 1   */
    UW      DQESEL;     /*                                              */
    UW      fill5;      /* Reserved space                               */
    UB      fill6[3];   /* Reserved space                               */
    UB      ODDPKS1;     /* USB odd packet size DMA enable register      */
    UB      RCINTSEL0;  /* Interrupt factor select register0            */
    UB      RCINTSEL1;  /* Interrupt factor select register1            */
    UB      RCINTSEL2;  /* Interrupt factor select register2            */
    UB      RCINTSEL3;  /* Interrupt factor select register3            */
    UB      RCINTSEL4;  /* Interrupt factor select register4            */
    UB      RCINTSEL5;  /* Interrupt factor select register5            */
    UB      RCINTSEL6;  /* Interrupt factor select register6            */
    UB      RCINTSEL7;  /* Interrupt factor select register7            */
};

#define INTRR    (*(volatile struct t_intrr *)0x40031000)

/* LCD Controller register */

struct t_lcdc {
    UB      LCDCC1;     /* LCDC control register1                       */
    UB      LCDCC2;     /* LCDC control register2                       */
    UB      LCDCC3;     /* LCDC control register3                       */
    UB      fill1;      /* Reserved space                               */
    UW      LCDC_PSR;   /* LCDC clock prescaler register                */
    UW      LCDC_COMEN; /* LCDC COM output enable register              */
    UW      LCDC_SEGEN1;/* LCDC SEG output enable register1             */
    UW      LCDC_SEGEN2;/* LCDC SEG output enable register2             */
    UH      LCDC_BLINK; /* LCDC blink setup register                    */
    UH      fill2;      /* Reserved space                               */
    UW      fill3;      /* Reserved space                               */
    UB      LCDRAM[40]; /* LCDC data memory regiater                    */
};

#define LCDC     (*(volatile struct t_lcdc *)0x40032000)

/* General-Purpse I/O Controller registers */

struct t_gpio {
    UW      PFR0;       /* Port0 Function control register              */
    UW      PFR1;       /* Port1 Function control register              */
    UW      PFR2;       /* Port2 Function control register              */
    UW      PFR3;       /* Port3 Function control register              */
    UW      PFR4;       /* Port4 Function control register              */
    UW      PFR5;       /* Port5 Function control register              */
    UW      PFR6;       /* Port6 Function control register              */
    UW      PFR7;       /* Port7 Function control register              */
    UW      PFR8;       /* Port8 Function control register              */
    UW      PFR9;       /* Port9 Function control register              */
    UW      PFRA;       /* PortA Function control register              */
    UW      PFRB;       /* PortB Function control register              */
    UW      PFRC;       /* PortC Function control register              */
    UW      PFRD;       /* PortD Function control register              */
    UW      PFRE;       /* PortE Function control register              */
    UW      PFRF;       /* PortF Function control register              */
    UW      fill1[48];  /* Reserved space                               */
    UW      PCR0;       /* Port0 pull-up control register               */
    UW      PCR1;       /* Port1 pull-up control register               */
    UW      PCR2;       /* Port2 pull-up control register               */
    UW      PCR3;       /* Port3 pull-up control register               */
    UW      PCR4;       /* Port4 pull-up control register               */
    UW      PCR5;       /* Port5 pull-up control register               */
    UW      PCR6;       /* Port6 pull-up control register               */
    UW      PCR7;       /* Port7 pull-up control register               */
    UW      PCR8;       /* Port8 pull-up control register               */
    UW      PCR9;       /* Port9 pull-up control register               */
    UW      PCRA;       /* PortA pull-up control register               */
    UW      PCRB;       /* PortB pull-up control register               */
    UW      PCRC;       /* PortC pull-up control register               */
    UW      PCRD;       /* PortD pull-up control register               */
    UW      PCRE;       /* PortE pull-up control register               */
    UW      PCRF;       /* PortF pull-up control register               */
    UW      fill2[48];  /* Reserved space                               */
    UW      DDR0;       /* Port0 direction control register             */
    UW      DDR1;       /* Port1 direction control register             */
    UW      DDR2;       /* Port2 direction control register             */
    UW      DDR3;       /* Port3 direction control register             */
    UW      DDR4;       /* Port4 direction control register             */
    UW      DDR5;       /* Port5 direction control register             */
    UW      DDR6;       /* Port6 direction control register             */
    UW      DDR7;       /* Port7 direction control register             */
    UW      DDR8;       /* Port8 direction control register             */
    UW      DDR9;       /* Port9 direction control register             */
    UW      DDRA;       /* PortA direction control register             */
    UW      DDRB;       /* PortB direction control register             */
    UW      DDRC;       /* PortC direction control register             */
    UW      DDRD;       /* PortD direction control register             */
    UW      DDRE;       /* PortE direction control register             */
    UW      DDRF;       /* PortF direction control register             */
    UW      fill3[48];  /* Reserved space                               */
    UW      PDIR0;      /* Port0 input data register                    */
    UW      PDIR1;      /* Port1 input data register                    */
    UW      PDIR2;      /* Port2 input data register                    */
    UW      PDIR3;      /* Port3 input data register                    */
    UW      PDIR4;      /* Port4 input data register                    */
    UW      PDIR5;      /* Port5 input data register                    */
    UW      PDIR6;      /* Port6 input data register                    */
    UW      PDIR7;      /* Port7 input data register                    */
    UW      PDIR8;      /* Port8 input data register                    */
    UW      PDIR9;      /* Port9 input data register                    */
    UW      PDIRA;      /* PortA input data register                    */
    UW      PDIRB;      /* PortB input data register                    */
    UW      PDIRC;      /* PortC input data register                    */
    UW      PDIRD;      /* PortD input data register                    */
    UW      PDIRE;      /* PortE input data register                    */
    UW      PDIRF;      /* PortF input data register                    */
    UW      fill4[48];  /* Reserved space                               */
    UW      PDOR0;      /* Port0 output data register                   */
    UW      PDOR1;      /* Port1 output data register                   */
    UW      PDOR2;      /* Port2 output data register                   */
    UW      PDOR3;      /* Port3 output data register                   */
    UW      PDOR4;      /* Port4 output data register                   */
    UW      PDOR5;      /* Port5 output data register                   */
    UW      PDOR6;      /* Port6 output data register                   */
    UW      PDOR7;      /* Port7 output data register                   */
    UW      PDOR8;      /* Port8 output data register                   */
    UW      PDOR9;      /* Port9 output data register                   */
    UW      PDORA;      /* PortA output data register                   */
    UW      PDORB;      /* PortB output data register                   */
    UW      PDORC;      /* PortC output data register                   */
    UW      PDORD;      /* PortD output data register                   */
    UW      PDORE;      /* PortE output data register                   */
    UW      PDORF;      /* PortF output data register                   */
    UW      fill5[48]; /* Reserved space                               */
    UW      ADE;        /* Analog input control register                */
    UW      fill6[31]; /* Reserved space                               */
    UW      SPSR;       /* special port setregister                     */
    UW      fill7[31]; /* Reserved space                               */
    UW      EPFR00;     /* Extension Function control register          */
    UW      EPFR01;     /* Extension Function control register          */
    UW      EPFR02;     /* Extension Function control register          */
    UW      EPFR03;     /* Extension Function control register          */
    UW      EPFR04;     /* Extension Function control register          */
    UW      EPFR05;     /* Extension Function control register          */
    UW      EPFR06;     /* Extension Function control register          */
    UW      EPFR07;     /* Extension Function control register          */
    UW      EPFR08;     /* Extension Function control register          */
    UW      EPFR09;     /* Extension Function control register          */
    UW      EPFR10;     /* Extension Function control register          */
    UW      EPFR11;     /* Extension Function control register          */
    UW      EPFR12;     /* Extension Function control register          */
    UW      EPFR13;     /* Extension Function control register          */
    UW      EPFR14;     /* Extension Function control register          */
    UW      EPFR15;     /* Extension Function control register          */
    UW      EPFR16;     /* Extension Function control register          */
    UW      EPFR17;     /* Extension Function control register          */
    UW      EPFR18;     /* Extension Function control register          */
    UW      fill8[45]; /* Reserved space                               */
    UW      PZR0;       /* Port0 pseudo-open drain control register     */
    UW      PZR1;       /* Port1 pseudo-open drain control register     */
    UW      PZR2;       /* Port2 pseudo-open drain control register     */
    UW      PZR3;       /* Port3 pseudo-open drain control register     */
    UW      PZR4;       /* Port4 pseudo-open drain control register     */
    UW      PZR5;       /* Port5 pseudo-open drain control register     */
    UW      PZR6;       /* Port6 pseudo-open drain control register     */
    UW      PZR7;       /* Port7 pseudo-open drain control register     */
    UW      PZR8;       /* Port8 pseudo-open drain control register     */
    UW      PZR9;       /* Port9 pseudo-open drain control register     */
    UW      PZRA;       /* PortA pseudo-open drain control register     */
    UW      PZRB;       /* PortB pseudo-open drain control register     */
    UW      PZRC;       /* PortC pseudo-open drain control register     */
    UW      PZRD;       /* PortD pseudo-open drain control register     */
    UW      PZRE;       /* PortE pseudo-open drain control register     */
    UW      PZRF;       /* PortF pseudo-open drain control register     */
    UW      fill9[48]; /* Reserved space                               */
    UW      reseved2;   /* Reserved space                               */
    UW      reseved3;   /* Reserved space                               */
};

#define REG_GPIO    (*(volatile struct t_gpio *)0x40033000)

/* Low-voltage detection registers (TYPE0,TYPE2,TYPE4)*/

struct t_lvd {
    UB      LVD_CTL;    /* Low-voltage detection voltage set register   */
    UB      fill1[3];   /* Reserved space                               */
    UB      LVD_STR;    /* Low-voltage detection interrupt factor register */
    UB      fill2[3];   /* Reserved space                               */
    UB      LVD_CLR;    /* Low-voltage detection interrupt factor clear register */
    UB      fill3[3];   /* Reserved space                               */
    UW      LVD_RLR;    /* Low-voltage detection Voltage protection register */
    UB      LVD_STR2;   /* Low-voltage detection circuit status register */
};

#define REG_LVD     (*(volatile struct t_lvd *)0x40035000)

/* Deep standby control registers */

struct t_ds {
    UB      REG_CTL;    /* Sub oscillator power supply control register */
    UB      fill1[3];   /* Reserved space                               */
    UB      RCK_CTL;    /* Sub clock apply control register             */
    UB      fill2[3];   /* Reserved space                               */
    UW      fill3[446];   /* Reserved space                               */
    UB      PMD_CTL;    /* RTC mode control register                    */
    UB      fill4[3];   /* Reserved space                               */
    UB      WRFSR;      /* Deep standby return factor register 1        */
    UB      fill5[3];   /* Reserved space                               */
    UH      WIFSR;      /* Deep standby return factor register 2        */
    UH      fill6[1];   /* Reserved space                               */
    UH      WIER;       /* Deep standby return permission register      */
    UH      fill7[1];   /* Reserved space                               */
    UB      WILVR;      /* Terminal WKUP input register                 */
    UB      fill8[3];   /* Reserved space                               */
    UB      DSRAMR;     /* Deep standby RAM holding register            */
    UB      fill9[3];   /* Reserved space                               */
    UW      fill10[58];  /* Reserved space                               */
    UB      BUR01;      /* Backup register 01                           */
    UB      BUR02;      /* Backup register 02                           */
    UB      BUR03;      /* Backup register 03                           */
    UB      BUR04;      /* Backup register 04                           */
    UB      BUR05;      /* Backup register 05                           */
    UB      BUR06;      /* Backup register 06                           */
    UB      BUR07;      /* Backup register 07                           */
    UB      BUR08;      /* Backup register 08                           */
    UB      BUR09;      /* Backup register 09                           */
    UB      BUR10;      /* Backup register 10                           */
    UB      BUR11;      /* Backup register 11                           */
    UB      BUR12;      /* Backup register 12                           */
    UB      BUR13;      /* Backup register 13                           */
    UB      BUR14;      /* Backup register 14                           */
    UB      BUR15;      /* Backup register 15                           */
    UB      BUR16;      /* Backup register 16                           */
};

#define REG_DS     (*(volatile struct t_ds *)0x40035100)

/* USB clock generation registers */

struct t_usbc {
    UB      UCCR;       /* USB clock control register                   */
    UB      fill1[3];   /* Reserved space                               */
    UB      UPCR1;      /* USB PLL control 1 register                   */
    UB      fill2[3];   /* Reserved space                               */
    UB      UPCR2;      /* USB PLL control 2 register                   */
    UB      fill3[3];   /* Reserved space                               */
    UB      UPCR3;      /* USB PLL control 3 register                   */
    UB      fill4[3];   /* Reserved space                               */
    UB      UPCR4;      /* USB PLL control 4 register                   */
    UB      fill5[3];   /* Reserved space                               */
    UB      UP_STR;     /* USB PLL macro status register                */
    UB      fill6[3];   /* Reserved space                               */
    UB      UPINT_ENR;  /* USB PLL interrupt factor enable register     */
    UB      fill7[3];   /* Reserved space                               */
    UB      UPINT_CLR;  /* USB PLL interrupt factor clear register      */
    UB      fill8[3];   /* Reserved space                               */
    UB      UPINT_STR;  /* USB PLL interrupt factor status register     */
    UB      fill9[3];   /* Reserved space                               */
    UB      UPCR5;      /* USB PLL control 5 register                   */
    UB      fill10[3];  /* Reserved space                               */
    UB      UPCR6;      /* USB PLL control 6 register                   */
    UB      fill11[3];  /* Reserved space                               */
    UB      UPCR7;      /* USB PLL control 7 register                   */
    UB      fill12[3];  /* Reserved space                               */
    UB      USBEN;      /* USB Permission register                      */
    UB      fill13[3];  /* Reserved space                               */
    UB      USBEN1;     /* USB Permission register 1                    */
};

#define REG_USBC    (*(volatile struct t_usbc *)0x40036000)

/* Bit definitions for USB/Ethernet clock Register */

#define UCCR_ETH_UCEN0      (0x00000001)   /*   */
#define UCCR_ETH_UCEN1      (0x00000008)   /*   */

#define UCCR_ETH_ECEN       (0x00000010)   /* Ethernet clock output enable bit  */
#define UCCR_ETH_SEL_PLL    (0x00000020)   /* Ethernet clock select USB/Eth PLL */
#define UCCR_ETH_SEL_CLKPLL (0x00000040)   /* Ethernet clock select CLKPLL      */

#define UPCR1_ETH_UPLLEN    (0x00000001)
#define UPCR1_ETH_UPINC     (0x00000002)

#define UPINT_ENR_UPCSE     (0x00000001)
#define UPINT_CLR_UPCSC     (0x00000001)

/* CAN Prescaler registers */

struct t_canp {
    UB      CANPRE;       /* CAN Prescaler registers                    */
};

#define REG_CANP    (*(volatile struct t_canp *)0x40037000)

/* Multi-Function serial interface registers */

struct t_mfs {
    UB      SMR;        /* Serial mode register                 */
    UB      SCR_IBCR;   /* Serial control register/I2C Bus control register */
    UB      fill1[2];   /* Reserved space                       */
    UB      ESCR_IBSR;  /* Extended Serial control register/I2C Bus status register */
    UB      SSR;        /* Serial status register               */
    UB      fill2[2];   /* Reserved space                       */
    UH      RDR_TDR;    /* Serial Send receive Data register    */
    UH      fill3;      /* Reserved space                       */
    UH      BGR;        /* Bord rate generator register         */
    UH      fill4;      /* Reserved space                       */
    UB      ISBA;       /* 7 Bit slave address register         */
    UB      ISMK;       /* 7 Bit slave address mask register    */
    UB      fill5[2];   /* Reserved space                       */
    UB      FCR0;       /* FIFO control register                */
    UB      FCR1;       /* FIF1 control register                */
    UB      fill6[2];   /* Reserved space                       */
    UB      FBYTE1;     /* FIF1 byte register                   */
    UB      FBYTE2;     /* FIF2 byte register                   */
    UB      fill7[2];   /* Reserved space                       */
};

#define REG_MFS0    (*(volatile struct t_mfs *)0x40038000)
#define REG_MFS1    (*(volatile struct t_mfs *)0x40038100)
#define REG_MFS2    (*(volatile struct t_mfs *)0x40038200)
#define REG_MFS3    (*(volatile struct t_mfs *)0x40038300)
#define REG_MFS4    (*(volatile struct t_mfs *)0x40038400)
#define REG_MFS5    (*(volatile struct t_mfs *)0x40038500)
#define REG_MFS6    (*(volatile struct t_mfs *)0x40038600)
#define REG_MFS7    (*(volatile struct t_mfs *)0x40038700)

/* I2C assist noise filter control register */

struct t_nfc {
    UH      I2CDNF;     /* I2C assist noise filter control register                 */
};

#define REG_NFC     (*(volatile struct t_nfc *)0x40038800)

/* Cyclic Redundancy Check registers */

struct t_crc {
    UB      CRCCR;      /* CRC Control register                 */
    UB      fill1[3];   /* Reserved space                       */
    UW      CRCINIT;    /* Initial value register               */
    UW      CRCIN;      /* Input Data register                  */
    UW      CRCR;       /* CRC register                         */
};

#define REG_CRC     (*(volatile struct t_crc *)0x40039000)

/* Watch counter registers */

struct t_wct {
    UB      WCRD;       /* Watch counter read register          */
    UB      WCRL;       /* Watch counter reload register        */
    UB      WCCR;       /* Watch counter control register       */
    UB      fill1;      /* Reserved space                       */
    UW      fill2[3];   /* Reserved space                       */
    UH      CLK_SEL;    /* Clock Select register                */
    UH      fill3[1];   /* Reserved space                       */
    UB      CLK_EN;     /* Division clock enable register       */
};

#define REG_WCT     (*(volatile struct t_wct *)0x4003A000)

/* RTC count registers */

struct t_rtc {
    UW      WTCR1;      /* Control register1                    */
    UW      WTCR2;      /* Control register2                    */
    UW      WTBR;       /* Counter cycle set register           */
    UB      WTSR;       /* Second register                      */
    UB      WTMIR;      /* Minutes register                     */
    UB      WTHR;       /* Hour register                        */
    UB      WTDR;       /* Date register                        */
    UB      WTDW;       /* A day of the week register           */
    UB      WTMOR;      /* Month register                       */
    UB      WTYR;       /* Year register                        */
    UB      fill1;      /* Reserved space                       */
    UB      fill2;      /* Reserved space                       */
    UB      ALMIR;      /* Alarm Minutes register               */
    UB      ALHR;       /* Alarm Hour register                  */
    UB      ALDR;       /* Alarm Date register                  */
    UB      fill3;      /* Reserved space                       */
    UB      ALMOR;      /* Alarm Month register                 */
    UB      ALYR;       /* Alarm Year register                  */
    UB      fill4;      /* Reserved space                       */
    UW      WTTR;       /* Alarm set register                   */
    UB      WTCLKS;     /* Clock select register                */
    UB      WTCLKM;     /* Select clock state register          */
    UB      fill5[2];   /* Reserved space                       */
    UB      WTCAL;      /* Frequency compensation set register  */
    UB      WTCALEN;    /* Frequency compensation permission register */
    UB      fill6[2];   /* Reserved space                       */
    UB      WTDIV;      /* Division ratio set register          */
    UB      WTDIVEN;    /* Division circuit permission register */
};

#define REG_RTC    (*(volatile struct t_rtc *)0x4003B000)

/* External bus interface registers */

struct t_extb {
    UW      MODE0;      /* Mode register 0                      */
    UW      MODE1;      /* Mode register 1                      */
    UW      MODE2;      /* Mode register 2                      */
    UW      MODE3;      /* Mode register 3                      */
    UW      MODE4;      /* Mode register 4                      */
    UW      MODE5;      /* Mode register 5                      */
    UW      MODE6;      /* Mode register 6                      */
    UW      MODE7;      /* Mode register 7                      */
    UW      TIM0;       /* Timing register 0                    */
    UW      TIM1;       /* Timing register 1                    */
    UW      TIM2;       /* Timing register 2                    */
    UW      TIM3;       /* Timing register 3                    */
    UW      TIM4;       /* Timing register 4                    */
    UW      TIM5;       /* Timing register 5                    */
    UW      TIM6;       /* Timing register 6                    */
    UW      TIM7;       /* Timing register 7                    */
    UW      AREA0;      /* Area register 0                      */
    UW      AREA1;      /* Area register 1                      */
    UW      AREA2;      /* Area register 2                      */
    UW      AREA3;      /* Area register 3                      */
    UW      AREA4;      /* Area register 4                      */
    UW      AREA5;      /* Area register 5                      */
    UW      AREA6;      /* Area register 6                      */
    UW      AREA7;      /* Area register 7                      */
    UW      ATIM0;      /* ALE timing register 0                */
    UW      ATIM1;      /* ALE timing register 1                */
    UW      ATIM2;      /* ALE timing register 2                */
    UW      ATIM3;      /* ALE timing register 3                */
    UW      ATIM4;      /* ALE timing register 4                */
    UW      ATIM5;      /* ALE timing register 5                */
    UW      ATIM6;      /* ALE timing register 6                */
    UW      ATIM7;      /* ALE timing register 7                */
    UW      fill1[160]; /* Reserved space                       */
    UW      DCLKR;       /* Clock divider register               */
};

#define REG_EXTB    (*(volatile struct t_extb *)0x4003F000)

/* USB Function, USB Host registers */

struct t_usb {
    UW      fill1[2112];/* Reserved space                       */
    UB      HCNT0;      /* Host control register 0              */
    UB      HCNT1;      /* Host control register 1              */
    UB      fill2[2];   /* Reserved space                       */
    UB      HIRQ;       /* Host Interrupt register              */
    UB      HERR;       /* Host error status register           */
    UB      fill3[2];   /* Reserved space                       */
    UB      HSTATE;     /* Host state status register           */
    UB      HFCOMP;     /* Soft interrupt Frame comparison register */
    UB      fill4[2];   /* Reserved space                       */
    UH      HRTIMER10;  /* Retry timer set register (1/0)       */
    UH      fill20;     /* Reserved space                       */
    UB      HRTIMER2;   /* Retry timer set register (2)         */
    UB      HADR;       /* Host Address register                */
    UB      fill5[2];   /* Reserved space                       */
    UH      HEOF;       /* EOF set register                     */
    UH      fill21;     /* Reserved space                       */
    UH      HFRAME;     /* FRAME set register                   */
    UH      fill22;     /* Reserved space                       */
    UB      HTOKEN;     /* Host Token End point register        */
    UB      fill6[3];   /* Reserved space                       */
    UH      UDCC;       /* UDC control register                 */
    UH      fill7;      /* Reserved space                       */
    UH      EP0C;       /* EP0 control register                 */
    UH      fill23;     /* Reserved space                       */
    UH      EP1C;       /* EP1 control register                 */
    UH      fill24;     /* Reserved space                       */
    UH      EP2C;       /* EP2 control register                 */
    UH      fill25;     /* Reserved space                       */
    UH      EP3C;       /* EP3 control register                 */
    UH      fill26;     /* Reserved space                       */
    UH      EP4C;       /* EP4 control register                 */
    UH      fill27;     /* Reserved space                       */
    UH      EP5C;       /* EP5 control register                 */
    UH      fill28;     /* Reserved space                       */
    UH      TMSP;       /* Time stamp register                  */
    UH      fill29;     /* Reserved space                       */
    UB      UDCS;       /* UDC status register                  */
    UB      UDCIE;      /* UDC Interrupt enable register        */
    UB      fill8[2];   /* Reserved space                       */
    UH      EP0IS;      /* EP0I status register                 */
    UH      fill30;     /* Reserved space                       */
    UH      EP0OS;      /* EP0O status register                 */
    UH      fill31;     /* Reserved space                       */
    UH      EP1S;       /* EP1 status register                  */
    UH      fill32;     /* Reserved space                       */
    UH      EP2S;       /* EP2 status register                  */
    UH      fill33;     /* Reserved space                       */
    UH      EP3S;       /* EP3 status register                  */
    UH      fill34;     /* Reserved space                       */
    UH      EP4S;       /* EP4 status register                  */
    UH      fill35;     /* Reserved space                       */
    UH      EP5S;       /* EP5 status register                  */
    UH      fill36;     /* Reserved space                       */
    UB      EP0DTL;     /* EP0 data register high-order         */
    UB      EP0DTH;     /* EP0 data register low-order          */
    UB      fill9[2];   /* Reserved space                       */
    UB      EP1DTL;     /* EP1 data register high-order         */
    UB      EP1DTH;     /* EP1 data register low-order          */
    UB      fill10[2];   /* Reserved space                       */
    UB      EP2DTL;     /* EP2 data register high-order         */
    UB      EP2DTH;     /* EP2 data register low-order          */
    UB      fill11[2];  /* Reserved space                       */
    UB      EP3DTL;     /* EP3 data register high-order         */
    UB      EP3DTH;     /* EP3 data register low-order          */
    UB      fill12[2];  /* Reserved space                       */
    UB      EP4DTL;     /* EP4 data register high-order         */
    UB      EP4DTH;     /* EP4 data register low-order          */
    UB      fill13[2];  /* Reserved space                       */
    UB      EP5DTL;     /* EP5 data register high-order         */
    UB      EP5DTH;     /* EP5 data register low-order          */
};

#define REG_USB0    (*(volatile struct t_usb *)0x40040000)
#define REG_USB1    (*(volatile struct t_usb *)0x40050000)

/* Direct Memory Access Controller registers */

struct t_dmac {
    UW      DMACR;      /* DMAC entirety configration register  */
    UW      fill1[3];   /* Reserved space                       */
    UW      DMACA0;     /* ch0 configration register A          */
    UW      DMACB0;     /* ch0 configration register B          */
    UW      DMACSA0;    /* ch0 Sorce address register           */
    UW      DMACDA0;    /* ch0 Destination address register     */
    UW      DMACA1;     /* ch1 configration register A          */
    UW      DMACB1;     /* ch1 configration register B          */
    UW      DMACSA1;    /* ch1 Sorce address register           */
    UW      DMACDA1;    /* ch1 Destination address register     */
    UW      DMACA2;     /* ch2 configration register A          */
    UW      DMACB2;     /* ch2 configration register B          */
    UW      DMACSA2;    /* ch2 Sorce address register           */
    UW      DMACDA2;    /* ch2 Destination address register     */
    UW      DMACA3;     /* ch3 configration register A          */
    UW      DMACB3;     /* ch3 configration register B          */
    UW      DMACSA3;    /* ch3 Sorce address register           */
    UW      DMACDA3;    /* ch3 Destination address register     */
    UW      DMACA4;     /* ch4 configration register A          */
    UW      DMACB4;     /* ch4 configration register B          */
    UW      DMACSA4;    /* ch4 Sorce address register           */
    UW      DMACDA4;    /* ch4 Destination address register     */
    UW      DMACA5;     /* ch5 configration register A          */
    UW      DMACB5;     /* ch5 configration register B          */
    UW      DMACSA5;    /* ch5 Sorce address register           */
    UW      DMACDA5;    /* ch5 Destination address register     */
    UW      DMACA6;     /* ch6 configration register A          */
    UW      DMACB6;     /* ch6 configration register B          */
    UW      DMACSA6;    /* ch6 Sorce address register           */
    UW      DMACDA6;    /* ch6 Destination address register     */
    UW      DMACA7;     /* ch7 configration register A          */
    UW      DMACB7;     /* ch7 configration register B          */
    UW      DMACSA7;    /* ch7 Sorce address register           */
    UW      DMACDA7;    /* ch7 Destination address register     */
};

#define REG_DMAC    (*(volatile struct t_dmac *)0x40060000)

/* CAN controller registers */

struct t_can {
    UH      CTRLR;      /* CAN control register                 */
    UH      STATR;      /* CAN status register                  */
    UH      ERRCNT;     /* CAN error counter                    */
    UH      BTR;        /* CAN bit timming register             */
    UH      INTR;       /* CAN interrupt register               */
    UH      TESTR;      /* CAN test register                    */
    UH      BRPER;      /* CAN prescaler expansion register     */
    UH      fill1;      /* Reserved space                       */
    UH      IF1CREQ;    /* IF1 command request register         */
    UH      IF1CMSK;    /* IF1 command mask register            */
    UH      IF1MSK1;    /* IF1 mask register 1                  */
    UH      IF1MSK2;    /* IF1 mask register 2                  */
    UH      IF1ARB1;    /* IF1 arbitration register 1           */
    UH      IF1ARB2;    /* IF1 arbitration register 2           */
    UH      IF1MCTR;    /* IF1 message control register         */
    UH      fill2;      /* Reserved space                       */
    UH      IF1DTA1L;   /* IF1 Data A register 1(little endian) */
    UH      IF1DTA2L;   /* IF1 Data A register 2(little endian) */
    UH      IF1DTB1L;   /* IF1 Data B register 1(little endian) */
    UH      IF1DTB2L;   /* IF1 Data B register 2(little endian) */
    UW      fill3[2];   /* Reserved space                       */
    UH      IF1DTA2B;   /* IF1 Data A register 1(big endian)    */
    UH      IF1DTA1B;   /* IF1 Data A register 2(big endian)    */
    UH      IF1DTB2B;   /* IF1 Data B register 1(big endian)    */
    UH      IF1DTB1B;   /* IF1 Data B register 2(big endian)    */
    UW      fill4[2];   /* Reserved space                       */
    UH      IF2CREQ;    /* IF2 command request register         */
    UH      IF2CMSK;    /* IF2 command mask register            */
    UH      IF2MSK1;    /* IF2 mask register 1                  */
    UH      IF2MSK2;    /* IF2 mask register 2                  */
    UH      IF2ARB1;    /* IF2 arbitration register 1           */
    UH      IF2ARB2;    /* IF2 arbitration register 2           */
    UH      IF2MCTR;    /* IF2 message control register         */
    UH      fill5;      /* Reserved space                       */
    UH      IF2DTA1L;   /* IF2 Data A register 1(little endian) */
    UH      IF2DTA2L;   /* IF2 Data A register 2(little endian) */
    UH      IF2DTB1L;   /* IF2 Data B register 1(little endian) */
    UH      IF2DTB2L;   /* IF2 Data B register 2(little endian) */
    UW      fill6[2];   /* Reserved space                       */
    UH      IF2DTA2B;   /* IF2 Data A register 1(big endian)    */
    UH      IF2DTA1B;   /* IF2 Data A register 2(big endian)    */
    UH      IF2DTB2B;   /* IF2 Data B register 1(big endian)    */
    UH      IF2DTB1B;   /* IF2 Data B register 2(big endian)    */
    UW      fill7[6];   /* Reserved space                       */
    UH      TREQR1;     /* CAN transmit request register1       */
    UH      TREQR2;     /* CAN transmit request register2       */
    UW      fill8[3];   /* Reserved space                       */
    UH      NEWDT1;     /* CAN data update register1            */
    UH      NEWDT2;     /* CAN data update register2            */
    UW      fill9[3];   /* Reserved space                       */
    UH      INTPND1;    /* CAN interrupt pending register1      */
    UH      INTPND2;    /* CAN interrupt pending register2      */
    UW      fill10[3];  /* Reserved space                       */
    UH      MSGVAL1;    /* CAN message validate register1       */
    UH      MSGVAL2;    /* CAN message validate register2       */
};

#define REG_CAN0    (*(volatile struct t_can *)0x40062000)
#define REG_CAN1    (*(volatile struct t_can *)0x40063000)


/* Ethernet Mode Controller Registers */

struct t_lan_mode {
    UW      MODE;       /* Mode Select */
    UW      fill1;      /* Reserved space                       */
    UW      CLKG;       /* Clock Gating Register */
};
#define REG_ETH_MODE (*(volatile struct t_lan_mode *)0x40066000)

/* Bit definitions for Ether Mode Register */

#define ETHMODE_PPSSEL   (0x10000000)   /* PTP Output pin configure 1:MAC1, 0:MAC0 */
#define ETHMODE_RST1     (0x00000200)   /* 1:MAC1 H/W Reset, 0:Reset Clear */
#define ETHMODE_RST0     (0x00000100)   /* 1:MAC0 H/W Reset, 0:Reset Clear */
#define ETHMODE_IFMODE   (0x00000001)   /* 1: RMII, 0:MII */

/* Bit definitions for Ether Clock Gating Register */
#define ETHCLKG_MACEN0   (0x00000001)
#define ETHCLKG_MACEN1_0 (0x00000003)
    /* MB9BF21x 00:MAC Clock Stop, 01:MAC Clcok Start, 10&11 Inhibit */
    /* MB9BF61x 00:MAC0,MAC1 Clock Stop, 01:MAC0 Clcok Start & MAC1 Clock Stop,
                11:MAC0,MAC1 Start Clock, 10 Inhibit */


/* Ethernet Controller Registers */

struct t_lan {
    UW      MCR;        /* MAC Configuration Register */
    UW      MFFR;       /* MAC Frame Filter Register */
    UW      MHTRH;      /* MAC Hash Table Register (High) */
    UW      MHTRL;      /* MAC Hash Table Register (Low) */
    UW      GAR;        /* GMII Address Rergister */
    UW      GDR;        /* GMII Data Register */
    UW      FCR;        /* Flow Control Register */
    UW      VTR;        /* VLAN tag Register */
    UW      fill1[2];
    UW      RWFFR;      /* Remote Wake-up Frame FIlter Register */
    UW      PMTR;       /* PMT Register */
    UW      LPICSR;     /* LPI Control and Status Register */
    UW      LPITCR;     /* LPI Timers Control Register */
    UW      ISR;        /* Interrupt Status Register */
    UW      IMR;        /* Interrupt Mask Register */
    UW      MAR0H;      /* MAC Address0 Register (High) */
    UW      MAR0L;      /* MAC Address0 Register (Low) */
    UW      MAR1H;      /* MAC Address1 Register (High) */
    UW      MAR1L;      /* MAC Address1 Register (Low) */
    UW      MAR2H;      /* MAC Address2 Register (High) */
    UW      MAR2L;      /* MAC Address2 Register (Low) */
    UW      MAR3H;      /* MAC Address3 Register (High) */
    UW      MAR3L;      /* MAC Address3 Register (Low) */
    UW      MAR4H;      /* MAC Address4 Register (High) */
    UW      MAR4L;      /* MAC Address4 Register (Low) */
    UW      MAR5H;      /* MAC Address5 Register (High) */
    UW      MAR5L;      /* MAC Address5 Register (Low) */
    UW      MAR6H;      /* MAC Address6 Register (High) */
    UW      MAR6L;      /* MAC Address6 Register (Low) */
    UW      MAR7H;      /* MAC Address7 Register (High) */
    UW      MAR7L;      /* MAC Address7 Register (Low) */
    UW      MAR8H;      /* MAC Address8 Register (High) */
    UW      MAR8L;      /* MAC Address8 Register (Low) */
    UW      MAR9H;      /* MAC Address9 Register (High) */
    UW      MAR9L;      /* MAC Address9 Register (Low) */
    UW      MAR10H;     /* MAC Address10 Register (High) */
    UW      MAR10L;     /* MAC Address10 Register (Low) */
    UW      MAR11H;     /* MAC Address11 Register (High) */
    UW      MAR11L;     /* MAC Address11 Register (Low) */
    UW      MAR12H;     /* MAC Address12 Register (High) */
    UW      MAR12L;     /* MAC Address12 Register (Low) */
    UW      MAR13H;     /* MAC Address13 Register (High) */
    UW      MAR13L;     /* MAC Address13 Register (Low) */
    UW      MAR14H;     /* MAC Address14 Register (High) */
    UW      MAR14L;     /* MAC Address14 Register (Low) */
    UW      MAR15H;     /* MAC Address15 Register (High) */
    UW      MAR15L;     /* MAC Address15 Register (Low) */
    UW      fill2[6];   /* 0x00c0 - 0x00d4 */
    UW      RGSR;       /* RGMII Status Register */
    UW      fill3[9];   /* 0x00dc - 0x00fc */
    /* MMC */
    UW      MMC_CNTL;    /* MMC Control Register */
    UW      MMC_INTR_RX; /* MMC Receiver Interrupt Register */
    UW      MMC_INTR_TX; /* MMC Transmit Interrupt Register */
    UW      MMC_INTR_MASK_RX; /* MMC Receiver Interrupt Mask Register */
    UW      MMC_INTR_MASK_TX; /* MMC Transmit Interrupt Mask Register */
    UW      MMC_TX_COUNT0[25];/* MMC TX Count */
    UW      fill4[2];
    UW      MMC_RX_COUNT0[24];/* MMC RX Count */
    UW      fill5[8];
    UW      MMC_IPC_INTR_MASK_RX;/* MMC Receiver Checksum Offload Interrupt Mask Register */
    UW      fill6;
    UW      MMC_IPC_INTR_RX;  /* MMC Receiver Checksum Offload Interrupt Register */
    UW      fill7;
    UW      MMC_RX_COUNT1[14];/* MMC RX Count */
    UW      fill8[2];
    UW      MMC_RX_COUNT2[14];/* MMC RX Count */
    UW      fill9[286];
    
    /* Time Stamp */
    UW      TSCR;        /* Time Stamp Control Register */
    UW      SSIR;        /* Sub-Second Increment Register */
    UW      STSR;        /* System Time-Seconds Register */
    UW      STNR;        /* System Time Nanosecond Register */
    UW      STSUR;       /* System Time Seconds Update Register */
    UW      STSNUR;      /* System Time Nanoseconds Update Register */
    UW      TSAR;        /* Time Stamp Addend Register */
    UW      TTSR;        /* Target Time Seconds Register */
    UW      TTNR;        /* Target Time Nanoseconds Register */
    UW      STHWSR;      /* System TIme High Word Seconds Register */
    UW      TSR;         /* TIme Stamp Status Register */
    UW      PPSCR;       /*PPC Control Register */
    UW      ATNR;        /* Auxiliary Time Stamp Nanosecond Register */
    UW      ATSR;        /* Auziliary Time Stamp Seconds Register */
    UW      fill10[50];
    
    UW      MAR16H;      /* MAC Address16 Register (High) */
    UW      MAR16L;      /* MAC Address16 Register (Low) */
    UW      MAR17H;      /* MAC Address17 Register (High) */
    UW      MAR17L;      /* MAC Address17 Register (Low) */
    UW      MAR18H;      /* MAC Address18 Register (High) */
    UW      MAR18L;      /* MAC Address18 Register (Low) */
    UW      MAR19H;      /* MAC Address19 Register (High) */
    UW      MAR19L;      /* MAC Address19 Register (Low) */
    UW      MAR20H;      /* MAC Address20 Register (High) */
    UW      MAR20L;      /* MAC Address20 Register (Low) */
    UW      MAR21H;      /* MAC Address21 Register (High) */
    UW      MAR21L;      /* MAC Address21 Register (Low) */
    UW      MAR22H;      /* MAC Address22 Register (High) */
    UW      MAR22L;      /* MAC Address22 Register (Low) */
    UW      MAR23H;      /* MAC Address23 Register (High) */
    UW      MAR23L;      /* MAC Address23 Register (Low) */
    UW      MAR24H;      /* MAC Address24 Register (High) */
    UW      MAR24L;      /* MAC Address24 Register (Low) */
    UW      MAR25H;      /* MAC Address25 Register (High) */
    UW      MAR25L;      /* MAC Address25 Register (Low) */
    UW      MAR26H;      /* MAC Address26 Register (High) */
    UW      MAR26L;      /* MAC Address26 Register (Low) */
    UW      MAR27H;      /* MAC Address27 Register (High) */
    UW      MAR27L;      /* MAC Address27 Register (Low) */
    UW      MAR28H;      /* MAC Address28 Register (High) */
    UW      MAR28L;      /* MAC Address28 Register (Low) */
    UW      MAR29H;      /* MAC Address29 Register (High) */
    UW      MAR29L;      /* MAC Address29 Register (Low) */
    UW      MAR30H;      /* MAC Address30 Register (High) */
    UW      MAR30L;      /* MAC Address30 Register (Low) */
    UW      MAR31H;      /* MAC Address31 Register (High) */
    UW      MAR31L;      /* MAC Address31 Register (Low) */
    UW      fill11[480];
    UW      BMR;         /* Bus Mode Register */
    UW      TPDR;        /* Transmit Polling Demand Register */
    UW      RPDR;        /* Recevie Polling Demand Register */
    UW      RDLAR;       /* Receive Descriptor List Address Register */
    UW      TDLAR;       /* Transmit Descriptor List Address Register */
    UW      SR;          /* Status Register */
    UW      OMR;         /* Operation Mode Register */
    UW      IER;         /* Interrupt Enable Register */
    UW      MFBOCR;      /* Missed Frame and Buffer Overflow Counter Register */
    UW      RIWTR;       /* Receive Interrupt Watchdog Timer Register */
    UW      fill12;
    UW      AHBSR;       /* AHB Status Register */
    UW      fill13[6];
    UW      CHTDR;       /* Current Host Transmit Descriptor Register */
    UW      CHRDR;       /* Current Host Receive Descriptor Register */
    UW      CHTBAR;      /* Current Host Transmit Buffer Address Register */
    UW      CHRBAR;      /* Current Host Receive Buffer Address Register */
};

#define REG_ETH0     (*(volatile struct t_lan *)0x40064000)
#define REG_ETH1     (*(volatile struct t_lan *)0x40067000)


/* IRQ Interrupt vector */
#define IRQ_FCS             IRQ0    /* Unusual frequency detection by the Clock Supervisor (FCS) */
#define IRQ_SWDT            IRQ1    /* Software watchdog timer          */
#define IRQ_LVD             IRQ2    /* Low Voltage detection (LVD)*/
#define IRQ_MFT_TDIF        IRQ3    /* MFT unit0,unit1,unit2 A waveform generator / DTIF (motor scram) */
#define IRQ_EXTI0_7         IRQ4    /* External terminal interrupt request ch.0-ch.7 */
#define IRQ_EXTI8_15        IRQ5    /* External terminal interrupt request ch.8-ch.31*/
#define IRQ_DTIM_QPRC       IRQ6    /* Dual timer / Quad counter (QPRC) ch.0,ch.1,ch.2 */
#define IRQ_MF_SIO_RECV0    IRQ7    /* Multi function serial interface ch0 receive interrupt  */
#define IRQ_MF_SIO_SEND0    IRQ8    /* Multi function serial interface ch0 transmit interrupt */
#define IRQ_MF_SIO_RECV1    IRQ9    /* Multi function serial interface ch1 receive interrupt  */
#define IRQ_MF_SIO_SEND1    IRQ10   /* Multi function serial interface ch1 transmit interrupt */
#define IRQ_MF_SIO_RECV2    IRQ11   /* Multi function serial interface ch2 receive interrupt  */
#define IRQ_MF_SIO_SEND2    IRQ12   /* Multi function serial interface ch2 transmit interrupt */
#define IRQ_MF_SIO_RECV3    IRQ13   /* Multi function serial interface ch3 receive interrupt  */
#define IRQ_MF_SIO_SEND3    IRQ14   /* Multi function serial interface ch3 transmit interrupt */
#define IRQ_MF_SIO_RECV4    IRQ15   /* Multi function serial interface ch4 receive interrupt  */
#define IRQ_MF_SIO_SEND4    IRQ16   /* Multi function serial interface ch4 transmit interrupt */
#define IRQ_MF_SIO_RECV5    IRQ17   /* Multi function serial interface ch5 receive interrupt  */
#define IRQ_MF_SIO_SEND5    IRQ18   /* Multi function serial interface ch5 transmit interrupt */
#define IRQ_MF_SIO_RECV6    IRQ19   /* Multi function serial interface ch6 receive interrupt  */
#define IRQ_MF_SIO_SEND6    IRQ20   /* Multi function serial interface ch6 transmit interrupt */
#define IRQ_MF_SIO_RECV7    IRQ21   /* Multi function serial interface ch7 receive interrupt  */
#define IRQ_MF_SIO_SEND7    IRQ22   /* Multi function serial interface ch7 transmit interrupt */
#define IRQ_PPG             IRQ23   /* PPG ch.0/2/4/8/10/12/16/18/20 */
#define IRQ_OSC             IRQ24   /* External main OSC/External sub OSC/Main PLL/USB PLL/Clock counter/RTC counter */
#define IRQ_ADC0            IRQ25   /* A/D Converter unit0 */
#define IRQ_ADC1            IRQ26   /* A/D Converter unit1 */
#define IRQ_ADC2            IRQ27   /* A/D Converter unit2 */
#define IRQ_MFT_FRT         IRQ28   /* MFT unit0, unit1, unit2 Freerun timer */
#define IRQ_MFT_ICU         IRQ29   /* MFT unit0, unit1, unit2 Input capture */
#define IRQ_MFT_OCU         IRQ30   /* MFT unit0, unit1, unit2 Output compare */
#define IRQ_BTIM0_7         IRQ31   /* Base timer ch.0-ch.7 */
#define IRQ_CAN0            IRQ32   /* CAN ch.0, Ethernet ch.0 */
#define IRQ_CAN1            IRQ33   /* CAN ch.1, Ethernet ch.1 */
#define IRQ_USB1_5          IRQ34   /* USB ch0 function (DRQ of End Point 1-5) */
#define IRQ_USB0_HOST       IRQ35   /* USB ch0 Function (DRQI, DRQO, and each status of End Point 0)/USB Host (each status) */
#define IRQ_USB1_1_5        IRQ36   /* USB ch1 function (DRQ of End Point 1-5) */
#define IRQ_USB1_0_HOST     IRQ37   /* USB ch1 Function (DRQI, DRQO, and each status of End Point 0)/USB Host (each status) */
#define IRQ_DMAC0           IRQ38   /* DMA controller(DMAC) ch.0 */
#define IRQ_DMAC1           IRQ39   /* DMA controller(DMAC) ch.1 */
#define IRQ_DMAC2           IRQ40   /* DMA controller(DMAC) ch.2 */
#define IRQ_DMAC3           IRQ41   /* DMA controller(DMAC) ch.3 */
#define IRQ_DMAC4           IRQ42   /* DMA controller(DMAC) ch.4 */
#define IRQ_DMAC5           IRQ43   /* DMA controller(DMAC) ch.5 */
#define IRQ_DMAC6           IRQ44   /* DMA controller(DMAC) ch.6 */
#define IRQ_DMAC7           IRQ45   /* DMA controller(DMAC) ch.7 */
#define IRQ_BTIM8_15        IRQ46   /* Base timer ch.8-ch.15 */
#define IRQ_Flash           IRQ47   /* Flash RDY, HANG Interrupt */

#ifdef __cplusplus
}
#endif
#endif /* _MB9BFxxx_H_ */
