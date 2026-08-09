/***********************************************************************
    MICRO C CUBE / COMPACT, DEVICE DRIVER
    Ethernet driver common defintions
    Copyright (c)  2009-2010, eForce Co., Ltd. All rights reserved.

    2008/12/13: First release
 ***********************************************************************/

#ifndef _COMMON_DEF_H_
#define _COMMON_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************
 * Bit definitions
 ************************************************************************/
#define BIT0    0x00000001UL
#define BIT1    0x00000002UL
#define BIT2    0x00000004UL
#define BIT3    0x00000008UL
#define BIT4    0x00000010UL
#define BIT5    0x00000020UL
#define BIT6    0x00000040UL
#define BIT7    0x00000080UL
#define BIT8    0x00000100UL
#define BIT9    0x00000200UL
#define BIT10   0x00000400UL
#define BIT11   0x00000800UL
#define BIT12   0x00001000UL
#define BIT13   0x00002000UL
#define BIT14   0x00004000UL
#define BIT15   0x00008000UL
#define BIT16   0x00010000UL
#define BIT17   0x00020000UL
#define BIT18   0x00040000UL
#define BIT19   0x00080000UL
#define BIT20   0x00100000UL
#define BIT21   0x00200000UL
#define BIT22   0x00400000UL
#define BIT23   0x00800000UL
#define BIT24   0x01000000UL
#define BIT25   0x02000000UL
#define BIT26   0x04000000UL
#define BIT27   0x08000000UL
#define BIT28   0x10000000UL
#define BIT29   0x20000000UL
#define BIT30   0x40000000UL
#define BIT31   0x80000000UL


#define KHZ     (1024)
#define MHZ     (1024*1024)

#define ALIGN_CONST(d, p)       ((((d)+((p)-1))/(p))*(p))

#ifdef __cplusplus
}
#endif
#endif /* _COMMON_DEF_H_ */
