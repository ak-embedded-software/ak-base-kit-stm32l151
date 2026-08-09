/***********************************************************************
    MICRO C CUBE / COMPACT, DEVICE DRIVER
    Ethernet PHY specific common defintions
    Copyright (c)  2009-2010, eForce Co., Ltd. All rights reserved.

    2008/12/13: First release
 ***********************************************************************/

#ifndef _DDR_PHY_H_
#define _DDR_PHY_H_

#include "COMMONDEF.h"

#ifdef __cplusplus
extern "C" {
#endif


#define PHY_BMCR        0x00    /* Basic mode Control */
#define PHY_BMSR        0x01    /* Basic mode Status */
#define PHY_IDR1        0x02    /* PHY Identifier */
#define PHY_IDR2        0x03    /* PHY Identifier */
#define PHY_ANAR        0x04    /* Auto-Negotiation Advertisement */
#define PHY_ANLPAR      0x05    /* Auto-Negotiation Link Partner Ability */
#define PHY_ANER        0x06    /* Auto-Negotiation Expansion */
#define PHY_ANNPTR      0x07    /* Auto-Negotiation Next Page Transmit */

#if (PHY_DEVICE == DP83848)
#define PHY_STS         0x10
#define PHY_MICR        0x11
#define PHY_MISR        0x12
#define PHY_FCSCR       0x14
#define PHY_RBR         0x17
#define PHY_LEDCR       0x18
#define PHY_CR          0x19
#define PHY_10BTSCR     0x1A
#endif

/* (Register 0) Basic mode Control Register */
#define BMCR_RESET          BIT15
#define BMCR_LOOPBACK       BIT14
#define BMCR_SPD            BIT13
#define BMCR_ANE            BIT12
#define BMCR_PDOWN          BIT11
#define BMCR_ISOL           BIT10
#define BMCR_RS_ANP         BIT9
#define BMCR_DUPLEX         BIT8
#define BMCR_COL_TEST       BIT7

/* (Register 1) Status Register */
#define BMSR_100_T4         BIT15
#define BMSR_100_X_FD       BIT14
#define BMSR_100_X_HD       BIT13
#define BMSR_10_FD          BIT12
#define BMSR_10_HD          BIT11
#define BMSR_PREAMBLE       BIT6
#define BMSR_ANEG_COMP      BIT5
#define BMSR_RM_FAULT       BIT4
#define BMSR_AN_ABLE        BIT3
#define BMSR_LINK_STAT      BIT2
#define BMSR_JABBER         BIT1
#define BMSR_EXT_CAP        BIT0

/* (Register 4) Auto-Negotiation Advertisement Register */
#define ANAR_NP             BIT15
#define ANAR_RF             BIT13
#define ANAR_ASM_DIR        BIT11
#define ANAR_PAUSE          BIT10
#define ANAR_T4             BIT9
#define ANAR_TX_FD          BIT8
#define ANAR_TX             BIT7
#define ANAR_10_FD          BIT6
#define ANAR_10             BIT5
#define ANAR_SF_802_3u      0x0001

/* (Register 5) Auto-Negotiation Link Partner Abilty Register (Base Page) */
#define ANLPAR_B_NP         BIT15
#define ANLPAR_B_ACK        BIT14
#define ANLPAR_B_R_FAULT    BIT13
#define ANLPAR_B_ASM_DIR    BIT11
#define ANLPAR_B_PAUSE      BIT10
#define ANLPAR_B_T4         BIT9
#define ANLPAR_B_TX_FD      BIT8
#define ANLPAR_B_TX         BIT7
#define ANLPAR_B_10_FD      BIT6
#define ANLPAR_B_10         BIT5
#define ANLPAR_B_SF_802_3u  0x0001

/* (Register 5) Auto-Negotiation Link Partner Abilty Register (Next Page) */
#define ANLPAR_N_NP         BIT15
#define ANLPAR_N_ACK        BIT14
#define ANLPAR_N_MP         BIT13
#define ANLPAR_N_ACK2       BIT12
#define ANLPAR_N_TOGGLE     BIT11

/* (Register 6) Auto-Negotiation Expansion */
#define ANER_PDF            BIT4
#define ANER_LP_NP_ABLE     BIT3
#define ANER_NP_ABLE        BIT2
#define ANER_PAGE_RX        BIT1
#define ANER_LP_AN_ABLE     BIT0

/* (Register 7) Auto-Negotiation Next Page Transmit Register */
#define ANNPTR_NP           BIT15
#define ANNPTR_MP           BIT13
#define ANNPTR_ACK2         BIT12
#define ANNPTR_TOG_TX       BIT11
#define ANNPTR_CODE_802_3u  0x0001


#if (PHY_DEVICE == DP83848)
#define PHYSTS_ANEG_DONE    BIT4
#define PHYSTS_LOOPBACK     BIT3
#define PHYSTS_FD           BIT2
#define PHYSTS_10M          BIT1
#define PHYSTS_LINK         BIT0
#endif


/* Disable Auto-Negotiaiton */
#define HD10   0x0000                       /*  10M, Half */
#define HD100  BMCR_SPD_LSB                 /* 100M, Half */
#define FD10   BMCR_DUPLEX                  /*  10M, Full */
#define FD100  (BMCR_SPD_LSB|BMCR_DUPLEX)   /* 100M, Full */

/* Enable Auto-Negotiation */
#define AHD10  ANA_TAF_10T_H                /*  10M, Half */
#define AHD100 ANA_TAF_100TX_H              /* 100M, Half */
#define AFD10  ANA_TAF_10T_F                /*  10M, Full */
#define AFD100 ANA_TAF_100TX_F              /* 100M, Full */


#ifdef __cplusplus
}
#endif
#endif /* _DDR_PHY_H_ */

