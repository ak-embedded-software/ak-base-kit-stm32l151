/***************************************************************************
    MICRO C CUBE / COMPACT, KERNEL
    ITRON macro definitions for eForce operating system (Compact)
    ARM Cortex-M3 Core version
    Copyright (c)  2008-2012, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.04.19: Created
            2008.12.09: Moved product version.
            2009.11.23: Supported 8-bytes stack-alignment.
            2010.06.10: Added byte order type.
            2011.01.20: Changed version.
            2012.05.10: Corresponded to the kernel version 2.
            2012.10.09: Modified _kernel_ALIGN_SIZE macro.
            2012.12.04: Added endian definition for GCC.
***************************************************************************/

#ifndef _ITRON_H_
#define _ITRON_H_

#if 0
#ifndef NULL
#ifdef __cplusplus
#define NULL    (0)
#else
#define NULL    ((void *)0)
#endif
#endif
#endif

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#define E_OK    0

#define _kernel_INT_SIZE    4
#define _kernel_ALIGN_SIZE  8
#define _kernel_INT_BIT     (_kernel_INT_SIZE*8)

#define TKERNEL_PRID    0x0144u
#define TKERNEL_PRVER   0x0202u


/************************************
    Byte Order Type
 ************************************/

#if defined (__CC_ARM)      /* for ARM Compiler */
#if defined (__BIG_ENDIAN)
#define _UC3_ENDIAN_BIG
#undef _UC3_ENDIAN_LITTLE
#else
#define _UC3_ENDIAN_LITTLE
#undef _UC3_ENDIAN_BIG
#endif
#elif defined (__ICCARM__)  /* for IAR Compiler */
#if (__LITTLE_ENDIAN__ == 1)
#define _UC3_ENDIAN_LITTLE
#undef _UC3_ENDIAN_BIG
#else
#define _UC3_ENDIAN_BIG
#undef _UC3_ENDIAN_LITTLE
#endif
#elif defined (__TMS470__)  /* for CCS Compiler */
#if defined (__little_endian__)
#define _UC3_ENDIAN_LITTLE
#undef _UC3_ENDIAN_BIG
#else
#define _UC3_ENDIAN_BIG
#undef _UC3_ENDIAN_LITTLE
#endif
#elif defined(__GNUC__)		/* for GNU C */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define _UC3_ENDIAN_LITTLE
#undef _UC3_ENDIAN_BIG
#else
#define _UC3_ENDIAN_BIG
#undef _UC3_ENDIAN_LITTLE
#endif
#endif


/************************************
    Data Types
 ************************************/

typedef signed char B;
typedef signed short H;
typedef signed long W;
typedef signed long long D;
typedef unsigned char UB;
typedef unsigned short UH;
typedef unsigned long UW;
typedef unsigned long long UD;
typedef char VB;
typedef short VH;
typedef long VW;
typedef long long VD;
typedef void *VP;
typedef void (*FP)(void);

typedef unsigned long SIZE;
typedef unsigned long ADDR;

typedef int INT;
typedef unsigned int UINT;

typedef VP VP_INT;

typedef INT BOOL;
typedef INT FN;
typedef INT ER;
typedef INT ID;
typedef INT PRI;
typedef INT BOOL_ID;
typedef INT ER_ID;
typedef INT ER_UINT;
typedef UINT FLGPTN;
typedef UINT INTNO;
typedef UINT IMASK;

typedef UINT ATR;
typedef UINT STAT;
typedef UINT MODE;

#ifdef _kernel_LARGE
typedef UH TID;
#else
typedef UB TID;
#endif

typedef struct t_systim {
    UW utime;
    UW ltime;
} SYSTIM;

typedef W TMO;

typedef UW RELTIM;

typedef UW T_REG;

typedef struct t_par {
    T_REG   fill[10];
    ER_UINT ercd;
    FLGPTN  flgptn;
    union {
        VP      ptr;
        VP_INT  dtq;
    } p2;
} T_PAR;

typedef struct t_cnsdep {
    FP          svchdr;
} T_CNSDEP;

#define CNSTBL_DEPENDEND T_CNSDEP cnsdep;
#define TCB_DEPENDEND

#define _KERNEL_MEMSET_
extern VP _kernel_memset(VP d, INT c, SIZE n);
#define _KERNEL_MEMCPY_
extern VP _kernel_memcpy(VP d, VP s, SIZE n);
#define _KERNEL_MEMCMP_
extern INT _kernel_memcmp(VP d, VP s, SIZE n);

extern UW _kernel_vector_atr[];
extern ER _kernel_change_level(UW level);
extern UW vget_ctx(void);

#endif /* _ITRON_H_ */
