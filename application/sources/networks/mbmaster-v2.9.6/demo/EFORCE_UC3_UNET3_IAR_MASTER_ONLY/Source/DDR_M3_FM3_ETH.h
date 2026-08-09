/***********************************************************************
    MICRO C CUBE / COMPACT, DEVICE DRIVER
    FM3 MB9BF21x/61x Ethernet Controller
    Copyright (c)  2012, eForce Co., Ltd. All rights reserved.

    Version Information
        2012/02/17: Created
        2012/10/04: Some of definitions moved to DDR_M3_FM3_ETH.h from
                    DDR_M3_FM3_ETH.c. (Status Flag for drivers)
 ***********************************************************************/

#ifndef _DDR_ETH_H_
#define _DDR_ETH_H_

#include "COMMONDEF.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************
 * Ethernet MAC configuration register (ETH_MACMCR)
 ************************************************************************/
#define MACCR_RE            BIT2    /* Receiver enable */
#define MACCR_TE            BIT3    /* Transmitter enable */
#define MACCR_DC            BIT4    /* Deferral check */
 #define MACCR_BL_10        0       /* Back-off limit */
 #define MACCR_BL_8         BIT5
 #define MACCR_BL_4         BIT6
 #define MACCR_BL_1         (BIT5 | BIT6)
#define MACCR_ACS           BIT7    /* Automatic pad/CRC stripping */
#define MACCR_DR            BIT9    /* Retry disable */
#define MACCR_IPC           BIT10   /* IPv4 checksum offload */
#define MACCR_DM            BIT11   /* Duplex mode */
#define MACCR_LM            BIT12   /* Loopback mode */
#define MACCR_DO            BIT13   /* Receive own disable */
#define MACCR_FES           BIT14   /* Fast Ethernet Speed (MII) */
#define MACCR_PS            BIT15   /* Port select of MII or RMII */
#define MACCR_DCRS          BIT15   /* Disable Carrier During transaction */
 #define MACCR_IFG_96       0       /* Interframe gap */
 #define MACCR_IFG_88       BIT17
 #define MACCR_IFG_80       BIT18
 #define MACCR_IFG_72       (BIT18 | BIT17)
 #define MACCR_IFG_64       BIT19
 #define MACCR_IFG_56       (BIT19 | BIT17)
 #define MACCR_IFG_48       (BIT19 | BIT18)
 #define MACCR_IFG_40       (BIT19| BIT18| BIT17)
#define MACCR_JE            BIT20   /* Jumbo frame enable */
#define MACCR_BE            BIT21   /* Frame burst enable */
#define MACCR_JD            BIT22   /* Jabber disable */
#define MACCR_WD            BIT23   /* Watchdog disable */

/*************************************************************************
 * Ethernet MAC frame filter register (ETH_MACMFFR)
 ************************************************************************/
#define MACFFR_PR           BIT0    /* Promiscuous mode */
#define MACFFR_HUC          BIT1    /* Hash unicast     */
#define MACFFR_HMC          BIT2    /* Hash multicast   */
#define MACFFR_DAIF         BIT3    /* Destination address inverse filtering */
#define MACFFR_PM           BIT4    /* Pass all multicast */
#define MACFFR_DB           BIT5    /* Disable broadcast frames */
 #define MACFFR_PCF_OFF     BIT6        /* Pass control frames : disable*/
 #define MACFFR_PCF_ALL     BIT7        /* Pass control frames : all frames */
 #define MACFFR_PCF_FILTER  (BIT7|BIT6) /* Pass control frames : frames that passes address filter */
#define MACFFR_SAIF         BIT8    /* Source address inverse filtering */
#define MACFFR_SAF          BIT9    /* Source address filter */
#define MACFFR_HPF          BIT10   /* Hash or perfect filter */
#define MACFFR_RA           BIT31   /* Receive all */

/*************************************************************************
 * Ethernet MAC GMII/MII address register (ETH_MACGAR)
 ************************************************************************/

#define MACMIIAR_MB         BIT0    /* PHY busy */
#define MACMIIAR_MW         BIT1    /* PHY write */
 #define MACMIIAR_CR_42     0               /* SYSCLK range: 60-100MHz */
 #define MACMIIAR_CR_62     BIT2            /* SYSCLK range: 100-150MHz */
 #define MACMIIAR_CR_16     BIT3            /* SYSCLK range: 20-35MHz */
 #define MACMIIAR_CR_26     (BIT3|BIT2)     /* SYSCLK range: 35-60MHz */
 #define MACMIIAR_CR_102    BIT4            /* SYSCLK range: 150-250MHz */
 #define MACMIIAR_CR_122    (BIT4|BIT2)     /* SYSCLK range: 250-300MHz */
/* MACMIIAR_MR  MII register (10:6)  */
/* MACMIIAR_PA  PHY address  (15:11) */

/*************************************************************************
 * Ethernet MAC flow control register (ETH_MACFCR)
 ************************************************************************/

#define MACFCR_FCB_BPA      BIT0    /* Flow control busy/back pressure activate */
#define MACFCR_TFE          BIT1    /* Transmit flow control enable */
#define MACFCR_RFE          BIT2    /* Receive flow control enable  */
#define MACFCR_UP           BIT3    /* Unicast pause frame detect   */
/* MACFCR_PT  (31:16) Pause time */

/*************************************************************************
 * Ethernet MAC VLAN tag register (ETH_MACVTR)
 ************************************************************************/

/* MACVLANTR_VLANT1 (15:0) VLAN tag identifier */
#define MACVLANTR_ETV       BIT16   /* 12 bit VLAN tag comparison */

/*************************************************************************
 * Ethernet MAC VLAN tag register (ETH_MACRWFFR)
 ************************************************************************/

/*************************************************************************
 * Ethernet MAC PMT control and status register (ETH_MACPMTR)
 ************************************************************************/
#define MACPMTR_PD          BIT0    /* Power down */
#define MACPMTR_MPE         BIT1    /* Magic Packet enable */
#define MACPMTR_WFE         BIT2    /* Wakeup frame enable */
#define MACPMTR_MPR         BIT5    /* Magic Packet received */
#define MACPMTR_WFR         BIT6    /* Wakeup frame received */
#define MACPMTR_GU          BIT9    /* Global unicast */
#define MACPMTR_RWFFRPR     BIT31   /* Wakeup frame filter register pointer reset */

/*************************************************************************
 * Ethernet MAC interrupt status register (ETH_MACISR)
 ************************************************************************/
#define MACSR_PIS           BIT3    /* PMT interrupt status */
#define MACSR_MIS           BIT4    /* MMC interrupt status */
#define MACSR_RIS           BIT5    /* MMC receive interrupt status */
#define MACSR_TIS           BIT6    /* MMC transmit interrupt status */
#define MACSR_TSIS          BIT9    /* Time stamp interrupt status */

/*************************************************************************
 * Ethernet MAC interrupt mask register (ETH_MACIMR)
 ************************************************************************/
#define MACIMR_PIM          BIT3    /* PMT interrupt mask */
#define MACIMR_TSIM         BIT9    /* Time stamp interrupt mask */
#define MACIMR_LPIIM        BIT10   /* LPI interrupt mask */

/*************************************************************************
 * Ethernet MAC address 0 high register (ETH_MACMAR0H) (15:0)
 ************************************************************************/
#define MACMAR0H_MO         BIT31   /* MO */



/*************************************************************************
 * Ethernet MMC control register (ETH_MMCCR)
 ************************************************************************/

#define MMCCR_CR            BIT0    /* Counter reset */
#define MMCCR_CSR           BIT1    /* Counter stop rollover */
#define MMCCR_ROR           BIT2    /* Reset on read */
#define MMCCR_MCF           BIT3    /* MMC counter freeze */

/*************************************************************************
 * Ethernet MMC receive interrupt register (ETH_MMCRIR)
 ************************************************************************/

#define MMCRIR_RFCES        BIT5    /* Received frames CRC error status */
#define MMCRIR_RFAES        BIT6    /* Received frames alignment error status */
#define MMCRIR_RGUFS        BIT17   /* Received Good Unicast Frames Status */

/*************************************************************************
 * Ethernet MMC transmit interrupt register (ETH_MMCTIR)
 ************************************************************************/

#define MMCTIR_TGFSCS       BIT14   /* Transmitted good frames single collision status */
#define MMCTIR_TGFMSCS      BIT15   /* Transmitted good frames more single collision status */
#define MMCTIR_TGFS         BIT21   /* Transmitted good frames status */

/*************************************************************************
 * Ethernet MMC receive interrupt mask register (ETH_MMCRIMR)
 ************************************************************************/

#define MMCRIMR_RFCEM       BIT5    /* Received frame CRC error mask */
#define MMCRIMR_RFAEM       BIT6    /* Received frames alignment error mask */
#define MMCRIMR_RGUFM       BIT17   /* Received good unicast frames mask */

/*************************************************************************
 * Ethernet MMC transmit interrupt mask register (ETH_MMCTIMR)
 ************************************************************************/

#define MMCTIMR_TGFSCM      BIT14   /* Transmitted good frames single collision mask */
#define MMCTIMR_TGFMSCM     BIT15   /* Transmitted good frames more single collision mask */
#define MMCTIMR_TGFM        BIT21   /* Transmitted good frames mask */

/*************************************************************************
 * Ethernet PTP time stamp control register (ETH_PTPSCR)
 ************************************************************************/

#define PTPSCR_TSE          BIT0    /* Time stamp enable */
#define PTPSCR_TFCU         BIT1    /* Time stamp fine or coarse update */
#define PTPSCR_TSI          BIT2    /* Time stamp system time initialize */
#define PTPSCR_TSUP         BIT3    /* Time stamp system time update */
#define PTPSCR_TITE         BIT4    /* Time stamp interrupt trigger enable */
#define PTPSCR_TARU         BIT5    /* Time stamp addend register update */

/*************************************************************************
 * Ethernet PTP subsecond increment register (ETH_PTPSSIR)
 ************************************************************************/
/* PTPSSCIR_STSSI       (7:0)   System time subsecond increment */


/*************************************************************************
 * Ethernet DMA bus mode register (ETH_DMABMR)
 ************************************************************************/
#define DMABMR_SWR          BIT0            /* Software reset */
#define DMABMR_DA           BIT1            /* DMA Arbitration */
/* DMABMR_DSL               (6:2)   Descriptor skip length */
#define DMABMR_ATDS         BIT7            /* Alternate Description size */
/* DMABMR_PBL               (13:8)  Programmable burst length */
 #define DMABMR_PBL_1_1         0           /* Rx Tx priority ration 1:1 */
 #define DMABMR_PBL_2_1     BIT14           /* Rx Tx priority ration 2:1 */
 #define DMABMR_PBL_3_1     BIT15           /* Rx Tx priority ration 3:1 */
 #define DMABMR_PBL_4_1     (BIT15|BIT14)   /* Rx Tx priority ration 3:1 */
#define DMABMR_FB           BIT16           /* Fixed burst */
/* DMABMR_RPBL               (22:17) Rx DMA PBL */
#define DMABMR_USP          BIT23           /* Use seperate PBL */
#define DMABMR_PBL          BIT24           /* 8xPBL mode */
#define DMABMR_AAL          BIT25           /* Address-aligned beats */
#define DMABMR_MB           BIT26           /* Mixed burst */
#define DMABMR_TXPR         BIT27           /* Transmit priority */

/*************************************************************************
 * Ethernet DMA status register (ETH_DMASR)
 ************************************************************************/
#define DMASR_TI            BIT0        /* Transmit interrupt status */
#define DMASR_TPS           BIT1        /* Transmit process stopped status */
#define DMASR_TU            BIT2        /* Transmitter buffer unavailable status */
#define DMASR_TJT           BIT3        /* Transmitter jabber timeout status */
#define DMASR_OVF           BIT4        /* Receive overflow status */
#define DMASR_UNF           BIT5        /* Transmit underflow status */
#define DMASR_RI            BIT6        /* Receive interrupt status */
#define DMASR_RU            BIT7        /* Receive buffer unavailable status */
#define DMASR_RPS           BIT8        /* Receive process stopped status */
#define DMASR_RWT           BIT9        /* Receive watchdog timeout status */
#define DMASR_ETI           BIT10       /* Earty transmit interrupt status */
#define DMASR_FBI           BIT13       /* Fatal bus error interrupt status */
#define DMASR_ERI           BIT14       /* Early receive interrupt status */
#define DMASR_AIS           BIT15       /* Abnormal interrupt summary */
#define DMASR_NIS           BIT16       /* Normal interrupt summary */
#define DMASR_RS           (BIT17|BIT18|BIT19) /* Receive process state */
#define DMASR_TS           (BIT22|BIT21|BIT20) /* Transmit process state */
#define DMASR_EB           (BIT25|BIT24|BIT23) /* Error bits status */
#define DMASR_GLI           BIT26       /* GMAC line interface interrupt status */
#define DMASR_GMI           BIT27       /* GMAC MMC interrupt status */
#define DMASR_GPI           BIT28       /* GMAC PMT interrupt status */
#define DMASR_TTI           BIT29       /* Time statmp trigger interrupt status */
#define DMASR_GLPII         BIT30       /* GMAC LPI interrupt status */

/*************************************************************************
 * Ethernet DMA operation mode register (ETH_DMAOMR)
 ************************************************************************/
#define DMAOMR_SR           BIT1        /* Start/Stop receive */
#define DMAOMR_OSF          BIT2        /* Operate on second frame */
 #define DMAOMR_RTC_64      0           /* Receive threshold control 64 */
 #define DMAOMR_RTC_32      (BIT3)      /* Receive threshold control 32 */
 #define DMAOMR_RTC_96      (BIT4)      /* Receive threshold control 96 */
 #define DMAOMR_RTC_128     (BIT4|BIT3) /* Receive threshold control 128 */
#define DMAOMR_FUF          BIT6        /* Forward undersized good frames */
#define DMAOMR_FEF          BIT7        /* Forward error frames */
#define DMAOMR_ST           BIT13       /* Start/Stop transmission */
 #define DMAOMR_TTC_64      0                   /* Transmit threshold control 64  */
 #define DMAOMR_TTC_128     BIT14               /* Transmit threshold control 128 */
 #define DMAOMR_TTC_192     BIT15               /* Transmit threshold control 192 */
 #define DMAOMR_TTC_256     (BIT15|BIT14)       /* Transmit threshold control 128 */
 #define DMAOMR_TTC_40      BIT16               /* Transmit threshold control 40 */
 #define DMAOMR_TTC_32      (BIT16|BIT14)       /* Transmit threshold control 32 */
 #define DMAOMR_TTC_24      (BIT16|BIT15)       /* Transmit threshold control 24 */
 #define DMAOMR_TTC_16      (BIT16|BIT15|BIT14) /* Transmit threshold control 16 */
#define DMAOMR_FTF          BIT20       /* Flush transmit FIFO */
#define DMAOMR_TSF          BIT21       /* Transmit store and forward */
#define DMAOMR_DFF          BIT24       /* Disable flushing of received frames */
#define DMAOMR_RSF          BIT25       /* Receive store and forward */
#define DMAOMR_DT           BIT26       /* Dropping of TCP/IP checksum error frames disable */

/*************************************************************************
 * Ethernet DMA interrupt enable register (ETH_DMAIER)
 ************************************************************************/
#define DMAIER_TIE          BIT0        /* Transmit interrupt enable */
#define DMAIER_TSE          BIT1        /* Transmit process stopped interrupt enable */
#define DMAIER_TUE          BIT2        /* Transmit buffer unavailable interrupt enable */
#define DMAIER_TJE          BIT3        /* Transmit jabber timeout interrupt enable */
#define DMAIER_OVE          BIT4        /* Receive overflow interrupt enable */
#define DMAIER_UNE          BIT5        /* Transmit underflow interrupt enable */
#define DMAIER_RIE          BIT6        /* Receive interrupt enable */
#define DMAIER_RUE          BIT7        /* Receive buffer unavailable interrupt enable */
#define DMAIER_RSE          BIT8        /* Receive process stopped interrupt enable */
#define DMAIER_RWE          BIT9        /* Receive watchdog timeout interrupt enable */
#define DMAIER_ETE          BIT10       /* Early transmit interrupt enable */
#define DMAIER_FBE          BIT13       /* Fatal bus error interrupt enable */
#define DMAIER_ERE          BIT14       /* Early receive interrupt enable */
#define DMAIER_AIE          BIT15       /* Abnormal interrupt summary enable */
#define DMAIER_NIE          BIT16       /* Normal interrupt summary enable */

/*************************************************************************
 * Ethernet DMA AHB status register (ETH_DMAAHBSR)
 ************************************************************************/
#define DMAAHB_AHBS         BIT0        /* AHB status */  
  
/*************************************************************************
 * Ethernet DMA missed frame and buffer overflow counter register (ETH_DMAMFBOCR)
 ************************************************************************/
/* DMAMFBOCR_NMFH           (15:0)      Missed frames by the controller */
/* DMAMFBOCR_ONMFC          (16)        Overflow bit for NMFH bit */
/* DMAMFBOCR_NMFF           (27:17)     Missed frames by the application */
/* DMAMFBOCR_ONMFF          (28)        Overflow bit for NMFF bit */

/*************************************************************************
 * Receive enhanced descriptor 0 (RDES0)
 ************************************************************************/

#define RDES0_OWN           BIT31       /* Own bit */
#define RDES0_AFM           BIT30       /* Destination address filter fail */
#define RDES0_FL            0x3FFF0000  /* Frame length */
#define RDES0_ES            BIT15       /* Error summary */
#define RDES0_DE            BIT14       /* Descriptor error */
#define RDES0_SAF           BIT13       /* Source address filter fail */
#define RDES0_LE            BIT12       /* Length error */
#define RDES0_OE            BIT11       /* Overflow error */
#define RDES0_VLAN          BIT10       /* VLAN tag */
#define RDES0_FS            BIT9        /* First descriptor */
#define RDES0_LS            BIT8        /* Last descriptor */
#define RDES0_TS            BIT7        /* Time Stamp */
#define RDES0_LC            BIT6        /* Late collision */
#define RDES0_FT            BIT5        /* Frame type */
#define RDES0_RWT           BIT4        /* Receive watchdog timeout */
#define RDES0_RE            BIT3        /* Receive error */
#define RDES0_DBE           BIT2        /* Dribble bit error (MII only)*/
#define RDES0_CE            BIT1        /* CRC error */
#define RDES0_ESA           BIT0        /* Extended Status Available */

/*************************************************************************
 * Receive enhanced descriptor 1 (RDES1)
 ************************************************************************/
#define RDES1_DIC           BIT31       /* Disable interrupt on completion */
#define RDES1_RER           BIT15       /* Receive end of ring */
#define RDES1_RCH           BIT14       /* Second address chained */

/*************************************************************************
 * Receive enhanced descriptor 4 (RDES4)
 ************************************************************************/
#define RDES4_TD            BIT14       /* Timestamp Dropped */
#define RDES4_PV            BIT13       /* PTP Version */
#define RDES4_PFT           BIT12       /* PTP Frame Type */
#define RDES4_MT            0x00000f00  /* Message Type */
 #define MT_NO_PTP   0000 /* No PTP message received or PTP packet with Reserved message type (*) */
 #define MT_SYNC     0001 /* SYNC (all clock types) */
 #define MT_FOLLOW   0010 /* Follow_Up (all clock types) */
 #define MT_DLY_REQ  0011 /* Delay_Req (all clock types) */
 #define MT_DLY_RES  0100 /* Delay_Resp (all clock types) */
 #define MT_PDLY_REQ 0101 /* Pdelay_Req (in peer-to-peer transparent clock) */
 #define MT_PDLY_RES 0110 /* Pdelay_Resp (in peer-to-peer transparent clock) */
 #define MT_PDLY_FLW 0111 /* Pdelay_Resp_Follow_Up (in peer-to-peer transparent clock) */
 #define MT_ANNOUNCE 1000 /* Announce */
 #define MT_MANAGE   1001 /* Management */
 #define MT_SUGNAL   1010 /* Signaling */
 #define MT_PTP      1111 /* PTP packet with reserved message type (Control Field 0x05- 0x0f) */
#define RDES4_IP6R          BIT7        /* Indicate for IPv6 */
#define RDES4_IP4R          BIT6        /* Indicate for IPv4 */
#define RDES4_IPCB          BIT5        /* Bypassed Checksum */
#define RDES4_IPE           BIT4        /* IP Payload Error */
#define RDES4_IPHE          BIT3        /* IP Header Error */
#define RDES4_TPT           0x00000007  /* Payload Type */
#define TPT_UNKNOWN  000 /* Unknown Type */
#define TPT_UDP      001 /* UDP payload */
#define TPT_TCP      010 /* TCP Payload */
#define TPT_ICMP     000 /* ICMP Payload */


/*************************************************************************
 * Transmit enhanced descriptor0 (TDES0)
 ************************************************************************/
#define TDES0_OWN           BIT31       /* Own bit */
#define TDES0_IC            BIT30       /* Interrupt on completion */
#define TDES0_LS            BIT29       /* Last segment */
#define TDES0_FS            BIT28       /* First segment */
#define TDES0_DC            BIT27       /* Disable CRC */
#define TDES0_DP            BIT26       /* Disable pad */
#define TDES0_TTSE          BIT25       /* Transmit time stamp enable */
#define TDES0_CIC           BIT23|BIT22 /* Checksum insertion control */
 #define TDES0_CIC_OFF      0
 #define TDES0_CIC_IPHDR                    (BIT22)
 #define TDES0_CIC_IPHDR_PAYLOAD            (BIT23)
 #define TDES0_CIC_IPHDR_PSEUDO_PAYLOAD     (BIT23|BIT22)
#define TDES0_TER           BIT21       /* Transmit end of ring */
#define TDES0_TCH           BIT20       /* Second address chained */
#define TDES0_TTSS          BIT17       /* Transmit time stamp status */
#define TDES0_IHE           BIT16       /* IP header error */
#define TDES0_ES            BIT15       /* Error summary */
#define TDES0_JT            BIT14       /* Jabber timeout */
#define TDES0_FF            BIT13       /* Frame flushed */
#define TDES0_IPE           BIT12       /* IP payload error */
#define TDES0_LC            BIT11       /* Loss of carrier  */
#define TDES0_NC            BIT10       /* No carrier */
#define TDES0_LCO           BIT9        /* Late collison */
#define TDES0_EC            BIT8        /* Excessive collision */
#define TDES0_VF            BIT7        /* VLAN frame */
#define TDES0_CC            0x00000078  /* Collision count */
#define TDES0_ED            BIT2        /* Excessive deferral */
#define TDES0_UF            BIT1        /* Underflow error */
#define TDES0_DB            BIT0        /* Deferred bit */

/*************************************************************************
 * Notification Event Command / Status definitions:
 ************************************************************************/
/* Link Status */
#define PHY_STS_LINK_DOWN   0x00U    /* PHY media link down status      */
#define PHY_STS_10HD        0x10U    /* PHY media is 10M/Half-Duplex    */
#define PHY_STS_10FD        0x20U    /* PHY media is 10M/Full-Duplex    */
#define PHY_STS_100HD       0x30U    /* PHY media is 100M/Half-Duplex   */
#define PHY_STS_100FD       0x40U    /* PHY media is 100M/Full-Duplex   */

   
/* Status Flag for drivers */ 
#define EV_ETH_TX       0x0001
#define EV_ETH_TX_ERR   0x0002
#define EV_ETH_TX_RDY   0x0004

#define EV_ETH_RX       0x0001
#define EV_ETH_RX_ERR   0x0002
#define EV_ETH_LNK      0x0004
#define EV_ETH_LNK_ERR  0x0008
   
#define PHY_STS_LINK_UP         0x0001
#define PHY_STS_ANG_DONE        0x0002
#define PHY_STS_SPEED_100       0x0004
#define PHY_STS_DUPLEX_FULL     0x0010

#define TX_ERR_BITS (DMAIER_TSE | DMAIER_TJE | DMAIER_UNE)
#define RX_ERR_BITS (RDES0_CE | RDES0_RE | RDES0_RWT | RDES0_LC | \
                     RDES0_OE|RDES0_DE)   
   
#ifdef __cplusplus
}
#endif
#endif /* _DDR_ETH_H_ */
