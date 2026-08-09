/************************************************************************
   MICRO C CUBE / COMPACT, KERNEL
   Cortex-M3 core processor registers definitions

   Copyright (c)  2008-2010, eForce Co., Ltd. All rights reserved.

   Version Information
           2008.03.31: Created.
           2008.12.06: Added the interrupt management functions.
           2010.07.22: Corrected the definitions of register address.
                       Corrected the definitions of interrupt number.
************************************************************************/

#ifndef _CORTEX_M3_H_
#define _CORTEX_M3_H_

/* SysTick Controller */

struct t_systick {
        UW      CTRSTS;         /* SysTick Control and Status Register */
        UW      RELOAD;         /* SysTick Reload Value Register */
        UW      CURRENT;        /* SysTick Current Value Register */
        UW      CALIB;          /* SysTick Calibration Value Register */
};

#define REG_SYSTICK     (*(volatile struct t_systick *)0xE000E010) /* SysTick Address */

/* Nested Vectored Interrupt Controller */

struct t_anvic {
        UW      SETENA[8];      /* Irq Set Enable Register */
        VB      fill1[96];
        UW      CLRENA[8];      /* Irq Clear Enable Register */
        VB      fill2[96];
        UW      SETPEND[8];     /* Irq Set Pending Register */
        VB      fill3[96];
        UW      CLRPEND[8];     /* Irq Clear Pending Register */
        VB      fill4[96];
        UW      ACTBIT[8];      /* Irq Active Bit Register */
        VB      fill5[224];
        UW      IRQPRI[60];     /* Irq Priority Register */
};

struct t_snvic {
        UW      CPUIDB;         /* CPU ID Base Register */
        UW      ICTRSTS;        /* Interrupt Control Status Register */
        void   *VECTTBL;        /* Vector Table Offset Register */
        UW      APPIRCTR;       /* Application Interrupt/Rest Control Register */
        UW      SYSCTR;         /* System Control Register */
        UW      CFGCTR;         /* Configuration Control Register */
        UW      SYSPRI[3];      /* System Handler Priority Register */
        UW      SYSCTRSTS;      /* System Handler Control and Status Register */
        UW      CFAULTSTS;      /* Configurable Fault Status Register */
        UW      HFAULTSTS;      /* Hard Fault Status Register */
        UW      DFAULTSTS;      /* Debug Fault Status Register */
        UW      MFAULTSTS;      /* Mem Manage Fault Status Register */
        UW      BFAULTSTS;      /* Bus Fault Status Register */
        UW      AFAULTSTS;      /* Auxiliary Fault Status Register */
        UW      PFR0;           /* Processor Feature Register 0 */
        UW      PFR1;           /* Processor Feature Register 1 */
        UW      DFR0;           /* Debug Feature Register 0 */
        UW      AFR0;           /* Auxiliary Feature Register 0 */
        UW      MMFR0;          /* Memory Model Feature Register 0 */
        UW      MMFR1;          /* Memory Model Feature Register 1 */
        UW      MMFR2;          /* Memory Model Feature Register 2 */
        UW      MMFR3;          /* Memory Model Feature Register 3 */
        UW      ISAR0;          /* ISA Feature Register 0 */
        UW      ISAR1;          /* ISA Feature Register 1 */
        UW      ISAR2;          /* ISA Feature Register 2 */
        UW      ISAR3;          /* ISA Feature Register 3 */
        UW      ISAR4;          /* ISA Feature Register 4 */
};

#define REG_ANVIC       (*(volatile struct t_anvic *)0xE000E100)    /* NVIC Address */
#define REG_SNVIC       (*(volatile struct t_snvic *)0xE000ED00)    /* NVIC Address */

#define REG_ICNTTYPE    (*(volatile unsigned long  *)0xE000E004)    /* Interrupt Control Type Register */
#define REG_SOFTTRG     (*(volatile unsigned long  *)0xE000EF00)    /* Soft Trigger Interrupt Register */
#define REG_PID4        (*(volatile unsigned char  *)0xE000EFD0)    /* Peripheral Identification Register */
#define REG_PID5        (*(volatile unsigned char  *)0xE000EFD4)    /* Peripheral Identification Register */
#define REG_PID6        (*(volatile unsigned char  *)0xE000EFD8)    /* Peripheral Identification Register */
#define REG_PID7        (*(volatile unsigned char  *)0xE000EFDC)    /* Peripheral Identification Register */
#define REG_PID0        (*(volatile unsigned char  *)0xE000EFE0)    /* Peripheral Identification Register Bits 7:0 */
#define REG_PID1        (*(volatile unsigned char  *)0xE000EFE4)    /* Peripheral Identification Register Bits 15:8 */
#define REG_PID2        (*(volatile unsigned char  *)0xE000EFE8)    /* Peripheral Identification Register Bits 23:15 */
#define REG_PID3        (*(volatile unsigned char  *)0xE000EFEC)    /* Peripheral Identification Register Bits 31:24 */
#define REG_CID0        (*(volatile unsigned char  *)0xE000EFF0)    /* Component Identification Register Bits 7:0 */
#define REG_CID1        (*(volatile unsigned char  *)0xE000EFF4)    /* Component Identification Register Bits 15:8 */
#define REG_CID2        (*(volatile unsigned char  *)0xE000EFF8)    /* Component Identification Register Bits 23:15 */
#define REG_CID3        (*(volatile unsigned char  *)0xE000EFFC)    /* Component Identification Register Bits 31:24 */

/* ARM Cortex-M3 dependent functions */

EXTERN  ER      ena_int(INTNO intno);
EXTERN  ER      dis_int(INTNO intno);
EXTERN  ER      vset_ipl(INTNO intno, IMASK imask);

/* IRQ Interrupt Vector */

#define IRQ0        16u
#define IRQ1        17u
#define IRQ2        18u
#define IRQ3        19u
#define IRQ4        20u
#define IRQ5        21u
#define IRQ6        22u
#define IRQ7        23u
#define IRQ8        24u
#define IRQ9        25u
#define IRQ10       26u
#define IRQ11       27u
#define IRQ12       28u
#define IRQ13       29u
#define IRQ14       30u
#define IRQ15       31u
#define IRQ16       32u
#define IRQ17       33u
#define IRQ18       34u
#define IRQ19       35u
#define IRQ20       36u
#define IRQ21       37u
#define IRQ22       38u
#define IRQ23       39u
#define IRQ24       40u
#define IRQ25       41u
#define IRQ26       42u
#define IRQ27       43u
#define IRQ28       44u
#define IRQ29       45u
#define IRQ30       46u
#define IRQ31       47u
#define IRQ32       48u
#define IRQ33       49u
#define IRQ34       50u
#define IRQ35       51u
#define IRQ36       52u
#define IRQ37       53u
#define IRQ38       54u
#define IRQ39       55u
#define IRQ40       56u
#define IRQ41       57u
#define IRQ42       58u
#define IRQ43       59u
#define IRQ44       60u
#define IRQ45       61u
#define IRQ46       62u
#define IRQ47       63u
#define IRQ48       64u
#define IRQ49       65u
#define IRQ50       66u
#define IRQ51       67u
#define IRQ52       68u
#define IRQ53       69u
#define IRQ54       70u
#define IRQ55       71u
#define IRQ56       72u
#define IRQ57       73u
#define IRQ58       74u
#define IRQ59       75u
#define IRQ60       76u
#define IRQ61       77u
#define IRQ62       78u
#define IRQ63       79u
#define IRQ64       80u
#define IRQ65       81u
#define IRQ66       82u
#define IRQ67       83u
#define IRQ68       84u
#define IRQ69       85u
#define IRQ70       86u
#define IRQ71       87u
#define IRQ72       88u
#define IRQ73       89u
#define IRQ74       90u
#define IRQ75       91u
#define IRQ76       92u
#define IRQ77       93u
#define IRQ78       94u
#define IRQ79       95u
#define IRQ80       96u
#define IRQ81       97u
#define IRQ82       98u
#define IRQ83       99u
#define IRQ84       100u
#define IRQ85       101u
#define IRQ86       102u
#define IRQ87       103u
#define IRQ88       104u
#define IRQ89       105u
#define IRQ90       106u
#define IRQ91       107u
#define IRQ92       108u
#define IRQ93       109u
#define IRQ94       110u
#define IRQ95       111u
#define IRQ96       112u
#define IRQ97       113u
#define IRQ98       114u
#define IRQ99       115u
#define IRQ100      116u
#define IRQ101      117u
#define IRQ102      118u
#define IRQ103      119u
#define IRQ104      120u
#define IRQ105      121u
#define IRQ106      122u
#define IRQ107      123u
#define IRQ108      124u
#define IRQ109      125u
#define IRQ110      126u
#define IRQ111      127u
#define IRQ112      128u
#define IRQ113      129u
#define IRQ114      130u
#define IRQ115      131u
#define IRQ116      132u
#define IRQ117      133u
#define IRQ118      134u
#define IRQ119      135u
#define IRQ120      136u
#define IRQ121      137u
#define IRQ122      138u
#define IRQ123      139u
#define IRQ124      140u
#define IRQ125      141u
#define IRQ126      142u
#define IRQ127      143u
#define IRQ128      144u
#define IRQ129      145u
#define IRQ130      146u
#define IRQ131      147u
#define IRQ132      148u
#define IRQ133      149u
#define IRQ134      150u
#define IRQ135      151u
#define IRQ136      152u
#define IRQ137      153u
#define IRQ138      154u
#define IRQ139      155u
#define IRQ140      156u
#define IRQ141      157u
#define IRQ142      158u
#define IRQ143      159u
#define IRQ144      160u
#define IRQ145      161u
#define IRQ146      162u
#define IRQ147      163u
#define IRQ148      164u
#define IRQ149      165u
#define IRQ150      166u
#define IRQ151      167u
#define IRQ152      168u
#define IRQ153      169u
#define IRQ154      170u
#define IRQ155      171u
#define IRQ156      172u
#define IRQ157      173u
#define IRQ158      174u
#define IRQ159      175u
#define IRQ160      176u
#define IRQ161      177u
#define IRQ162      178u
#define IRQ163      179u
#define IRQ164      180u
#define IRQ165      181u
#define IRQ166      182u
#define IRQ167      183u
#define IRQ168      184u
#define IRQ169      185u
#define IRQ170      186u
#define IRQ171      187u
#define IRQ172      188u
#define IRQ173      189u
#define IRQ174      190u
#define IRQ175      191u
#define IRQ176      192u
#define IRQ177      193u
#define IRQ178      194u
#define IRQ179      195u
#define IRQ180      196u
#define IRQ181      197u
#define IRQ182      198u
#define IRQ183      199u
#define IRQ184      200u
#define IRQ185      201u
#define IRQ186      202u
#define IRQ187      203u
#define IRQ188      204u
#define IRQ189      205u
#define IRQ190      206u
#define IRQ191      207u
#define IRQ192      208u
#define IRQ193      209u
#define IRQ194      210u
#define IRQ195      211u
#define IRQ196      212u
#define IRQ197      213u
#define IRQ198      214u
#define IRQ199      215u
#define IRQ200      216u
#define IRQ201      217u
#define IRQ202      218u
#define IRQ203      219u
#define IRQ204      220u
#define IRQ205      221u
#define IRQ206      222u
#define IRQ207      223u
#define IRQ208      224u
#define IRQ209      225u
#define IRQ210      226u
#define IRQ211      227u
#define IRQ212      228u
#define IRQ213      229u
#define IRQ214      230u
#define IRQ215      231u
#define IRQ216      232u
#define IRQ217      233u
#define IRQ218      234u
#define IRQ219      235u
#define IRQ220      236u
#define IRQ221      237u
#define IRQ222      238u
#define IRQ223      239u
#define IRQ224      240u
#define IRQ225      241u
#define IRQ226      242u
#define IRQ227      243u
#define IRQ228      244u
#define IRQ229      245u
#define IRQ230      246u
#define IRQ231      247u
#define IRQ232      248u
#define IRQ233      249u
#define IRQ234      250u
#define IRQ235      251u
#define IRQ236      252u
#define IRQ237      253u
#define IRQ238      254u
#define IRQ239      255u

#endif
