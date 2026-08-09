/***********************************************************************
    MICRO C CUBE / COMPACT, DEVICE DRIVER
    Ethernet driver for Fujitsu Semicon FM3 MB9BFxxx (MB9BF61x)
    Copyright (c)  2012, eForce Co., Ltd. All rights reserved.

    2012/10/12: Created
    2013/05/27: Modified incorrect operation received descriptor.
    2013/05/29: Masked unnecessary MMC interrupt.
    2014/04/22: The completion waiting of DMA reset was changed for
                timeout in DMA_Reset1().
 ***********************************************************************/

#include <string.h>
#include "kernel.h"
#include "DDR_M3_FM3_ETH1_cfg.h"
#include "DDR_M3_FM3_ETH.h"
#include "DDR_PHY.h"
#include "net_hdr.h"
#ifdef NET_C_OS
#include "kernel_id.h"
#endif

/*
    Configuration Macros for Ethernet ch1
*/

#ifndef ETH_PHY_ADDR1
#define ETH_PHY_ADDR1    0x01
#endif

#ifndef ETH_MDC_CLK1
#define ETH_MDC_CLK1     MACMIIAR_CR_62
#endif

#ifndef ETH_PHY_MODE1
#define ETH_PHY_MODE1    0
#endif

#ifndef ETH_FILTER_MODE1
#define ETH_FILTER_MODE1 0
#endif

#ifndef ETH_RXDESC_CNT1
#define ETH_RXDESC_CNT1  4
#endif

#ifndef ETH_TXDESC_CNT1
#define ETH_TXDESC_CNT1  4
#endif

#ifndef ETH_CSUM_MODE1
#define ETH_CSUM_MODE1   0
#endif

#ifndef ETH_TX_TMO1
#define ETH_TX_TMO1    100
#endif

#ifndef PHY_RST_WAIT_TMO1
#define PHY_RST_WAIT_TMO1    5000
#endif

#ifndef ETH_TIMESTAMP1
#define ETHE_TIMESTAMP1  0
#endif

/*
    Definitions
*/

UINT    LanStatus1;

/*
    DMA Descriptor Chain
*/
typedef struct t_ethdesc1 {
    UW  sts;
    UW  size;
    UW  buf;
    UW  next;
    UW  ext_sts;
    UW  reserved1;
    UW  reserved2;
    UW  reserved3;
    T_NET_BUF  *pkt;    /* Network buffer associated with this desc */
    UW dummy;
}T_ETHDESC1;

static T_ETHDESC1     EthRxDesc1[ETH_RXDESC_CNT1];
static T_ETHDESC1     EthTxDesc1[ETH_TXDESC_CNT1];

#ifndef ETH_MAXLEN1
#define ETH_MAXLEN1  1520U   /* Maximum ethernet frame size (TCP/IP MTU + 20) */
#endif

static UH eth_devnum1;       /* Device Number*/

void EthRxBufferInit1(void)
{
    T_NET_BUF *pkt;
    ER ercd;
    int i;

    for (i=0;i<ETH_RXDESC_CNT1;i++) {
        EthRxDesc1[i].pkt = NULL;
        ercd = net_buf_get(&pkt, ETH_MAXLEN1, TMO_POL);
        if (ercd != E_OK) {
            break;
        }        
        EthRxDesc1[i].pkt = pkt;
    }
}

void EthDMARxDescInit1(void)
{
    T_NET_BUF *pkt;
    int i;

    EthRxBufferInit1();

    /* Construct Rx Descriptor chain */
    for (i=0;i<ETH_RXDESC_CNT1;i++) {
        pkt = EthRxDesc1[i].pkt;
        EthRxDesc1[i].size = (RDES1_RCH | (0x1FFF & ETH_MAXLEN1));
        EthRxDesc1[i].buf = (UW)&pkt->buf[2];
        EthRxDesc1[i].sts  = RDES0_OWN;
        if ((i + 1) < ETH_RXDESC_CNT1) {
            EthRxDesc1[i].next = (UW)(&EthRxDesc1[i+1]);
        }
        else {
            EthRxDesc1[i].next = (UW)(&EthRxDesc1[0]);
            EthRxDesc1[i].sts |= RDES0_LS;
        }
    }

    /* Set top descriptor address */
    REG_ETH1.RDLAR    = (UW)(&EthRxDesc1[0]);
}

void EthDMATxDescInit1(void)
{
    int i;

    /* Construct Tx Descriptor chain */
    for (i=0;i<ETH_TXDESC_CNT1;i++) {
        EthTxDesc1[i].sts  = TDES0_TCH;
        EthTxDesc1[i].size = 0;
        EthTxDesc1[i].buf  = 0;
        EthTxDesc1[i].pkt  = NULL;
        if ((i + 1) < ETH_TXDESC_CNT1) {
            EthTxDesc1[i].next = (UW)(&EthTxDesc1[i+1]);
        }
        else {
            EthTxDesc1[i].next = (UW)(&EthTxDesc1[0]);
        }
    }

    /* Set top descriptor address */
    REG_ETH1.TDLAR    = (UW)(&EthTxDesc1[0]);
}

static void phy_put1(UB reg, UH data)
{
    UW tmp;

    loc_cpu();
    REG_ETH1.GDR = data;
    REG_ETH1.GAR = (ETH_PHY_ADDR1 << 11) |
                       (reg      << 6)  |
                       (ETH_MDC_CLK1 <<2)|
                       MACMIIAR_MW      |
                       MACMIIAR_MB      ;
    do {
        tmp = REG_ETH1.GAR;
        unl_cpu();
        loc_cpu();
    } while (tmp & MACMIIAR_MB);
    unl_cpu();
}

static UH phy_get1(UB reg)
{
    UW tmp;
    UW dat;

    loc_cpu();
    REG_ETH1.GAR = (ETH_PHY_ADDR1 << 11) |
                       (reg      << 6)  |
                       (ETH_MDC_CLK1 <<2)|
                       MACMIIAR_MB      ;

    do {
        tmp = REG_ETH1.GAR;
        unl_cpu();
        loc_cpu();
    } while (tmp & MACMIIAR_MB);
    dat = REG_ETH1.GDR;
    unl_cpu();
    return dat;
}

UH phy_sts1(void)
{
    UH dat, sts;

    sts = PHY_STS_LINK_DOWN;

    /****** Check Link Status *****/
    phy_get1(PHY_BMSR);  /* released latched status (Dummy read) */
    dat = phy_get1(PHY_BMSR);
    if (!(dat & BMSR_LINK_STAT)) {
        /* Link Down */
        return sts;
    }
    sts |= PHY_STS_LINK_UP;

#if (ETH_PHY_MODE1 == 0 || ETH_PHY_MODE1 == 2 || ETH_PHY_MODE1 == 4)
    if (!(dat & BMSR_ANEG_COMP)) {
        /* Auto Negotition not yet completed */
        return sts;
    }
#endif
    sts |= PHY_STS_ANG_DONE;

    /****** Know Link details *****/
    /* Note: In Basic register set, there is no bit avaialble to read the 
             Link details. So, here we use the common capabilities of remote
             and local stations as Link details.
    */
    dat = phy_get1(PHY_ANLPAR);  /* Link partner abilities */
    dat &= phy_get1(PHY_ANAR);   /* Local station abilities */

    if (dat & ANLPAR_B_TX_FD) {
        /* 100Base-TX Full Duplex */
        sts |= (PHY_STS_SPEED_100 | PHY_STS_DUPLEX_FULL);
    }
    else if (dat & ANLPAR_B_TX) {
        /* 100Base-Tx Half Duplex */
        sts |= PHY_STS_SPEED_100;
    }
    else if (dat & ANLPAR_B_10_FD) {
        /* 10Base-T Full Duplex */
        sts |= PHY_STS_DUPLEX_FULL;
    }
    else {
        /* 10Base-T */
    }

    return sts;
}

ER phy_ini1(void)
{
    int i;

    /* Reset PHY */
    phy_put1(PHY_BMCR, BMCR_RESET);

    i = PHY_RST_WAIT_TMO1/10;
    for (;;) {
        dly_tsk(10);
        if (!(phy_get1(PHY_BMCR) & BMCR_RESET)) {
            break;
        }
        if (i-- <= 0) {
            return E_TMOUT;     /* Error: PHY reset command error */
        }
    }

    phy_get1(PHY_BMSR);          /* Dummy status read */

#if (ETH_PHY_MODE1 == 1)
    /* 10M Half Duplex manual mode */
    phy_put1(PHY_BMCR, 0);
#elif (ETH_PHY_MODE1 == 2)
    /* 10M Full/Half (Duplex auto select mode) */
    phy_put1(PHY_ANAR, ANAR_10_FD | ANAR_10 | ANAR_SF_802_3u);
    phy_put1(PHY_BMCR, BMCR_ANE | BMCR_RS_ANP);
#elif (ETH_PHY_MODE1 == 3)
    /* 100M Half Duplex manual mode */
    phy_put1(PHY_BMCR, BMCR_SPD);
#elif (ETH_PHY_MODE1 == 4)
    /* 100M Full/Half (Duplex auto select mode) */
    phy_put1(PHY_ANAR, ANAR_TX_FD | ANAR_TX | ANAR_SF_802_3u);
    phy_put1(PHY_BMCR, BMCR_ANE | BMCR_RS_ANP);
#else
    /* Auto select mode (10HD/10FD/100HD/100FD) */
    phy_put1(PHY_ANAR, ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10 | ANAR_SF_802_3u);
    phy_put1(PHY_BMCR, BMCR_ANE | BMCR_RS_ANP);
#endif
    return E_OK;
}

void phy_link1(void)
{
    UW  tmp;
    UH  sts;

    while (1) {
        sts = phy_sts1();
        if (sts & PHY_STS_LINK_UP) {
            break;
        }
        LanStatus1 = 0;
        dly_tsk(50);
    }

    if (LanStatus1) {
        return;
    }

    tmp = REG_ETH1.MCR;
    tmp &= ~(MACCR_DM|MACCR_FES);
    if (sts & PHY_STS_DUPLEX_FULL) {
        tmp |= MACCR_DM;
    }
    if (sts & PHY_STS_SPEED_100) {
        tmp |= MACCR_FES;
    }
    tmp |= MACCR_PS;
    REG_ETH1.MCR = tmp;

    LanStatus1 = 1;

    /* Restart Tx & Rx */
}

ER DMA_Reset1(void)
{
    UW tmp;
    int i;

    /* Software Reset */
    loc_cpu();
    REG_ETH1.BMR |= DMABMR_SWR;    
    unl_cpu();
    /* Wait until reset operation completes */
    i = 50;
    for (;;) {
        tmp = REG_ETH1.BMR;
        if (!(tmp & DMABMR_SWR)) {
            break;
        }
        if (i-- <= 0) {
            return E_TMOUT;     /* Error: DMA reset command error */
        }
        dly_tsk(0);
    }

    /* Wait until AHB master interface is not idle */
    i = 50;
    for (;;) {
        tmp = REG_ETH1.AHBSR;
        if (!(tmp & DMAAHB_AHBS)) {
            break;
        }
        if (i-- <= 0) {
            return E_TMOUT;     /* Error: DMA reset command error */
        }
        dly_tsk(0);
    }

    return E_OK;
}

ER eth_ini1(UH dev_num)
{
    T_NET       *net;
    T_NET_DEV   *dev;
    UB          *mac;
    UW          reg;
    ER          ercd;
    UW          tmp;

    if (dev_num == 0) {
        return E_ID;
    }
    eth_devnum1 = dev_num;

    net = &gNET[eth_devnum1-1];
    if (net == NULL) {
        return E_PAR;
    }
    dev = net->dev;
    if (dev == NULL) {
        return E_PAR;
    }

#if (ETH_CSUM_MODE1 & 1)
    net->flag |= HW_CS_TX_IPH4;
    net->flag |= HW_CS_TX_DATA;
#endif

    LanStatus1 = 0;

    /***** DMA Initialization *****/
    ercd = DMA_Reset1();
    if (ercd != E_OK) {
        return ercd;
    }

    loc_cpu();
    /* GMAC configration register, PS=1 for MII and RMII */
    REG_ETH1.MCR |= MACCR_PS; 
    
    /* DMA bus mode         */
    REG_ETH1.BMR |= DMABMR_PBL_1_1;  /* DA DMA Arbitration - TX/RX has same priority */

    REG_ETH1.BMR |= DMABMR_ATDS;  /* TX & RX Descriptor size extend */

    /* DMA Descriptors Initialize */
    EthDMARxDescInit1();
    EthDMATxDescInit1();

    /* DMA operation mode   */
    REG_ETH1.OMR |= DMAOMR_RSF | DMAOMR_TSF | DMAOMR_DT;

    /* Interrupt req clear */
    REG_ETH1.SR  |= DMASR_NIS;   
    
    /* Interrupt */
    REG_ETH1.IER |= (DMAIER_NIE | DMAIER_AIE | DMAIER_ETE | TX_ERR_BITS | DMAIER_RIE);

    /* Wait until AHB master interface is not idle */
    do {
        tmp = REG_ETH1.AHBSR;
        unl_cpu();
        loc_cpu();
    }
    while (tmp & DMAAHB_AHBS);
    
    /* Start Tx & Rx */
    REG_ETH1.OMR |= (DMAOMR_SR | DMAOMR_ST);
    unl_cpu();  

    /***** PHY Initialization *****/
    ercd = phy_ini1();
    if (ercd != E_OK) {
        return ercd;
    }
    
    /***** MAC Initialization *****/
    loc_cpu();
    /* Rx own disable, Automatic pad/CRC stripping */
    reg = 0;
    reg |= MACCR_DO | MACCR_ACS;
    reg |= MACCR_FES | MACCR_DM;    /* Duplex, Full Speed */
    reg |= MACCR_PS;
    REG_ETH1.MCR |= reg;

    unl_cpu();
    dly_tsk(2);
    loc_cpu();
    
    /* Station address setting */
    mac = &dev->cfg.eth.mac[0];

    REG_ETH1.MAR0H = ((mac[5] << 8 )| mac[4] | MACMAR0H_MO);
    REG_ETH1.MAR0L = ((mac[3] << 24)| (mac[2] << 16) | (mac[1] << 8) | mac[0]);

    /* Rx Filter */
#if  (ETH_FILTER_MODE1 == 1)
    REG_ETH1.MFFR |= MACFFR_PR;     /* Receive All packets  */
#elif (ETH_FILTER_MODE1 == 2)
    REG_ETH1.MFFR |= MACFFR_PM;     /* Receive Multicast    */
#endif

    /* Checksum */
    reg = 0;
#if (ETH_CSUM_MODE1 & 2)
    reg |= MACCR_IPC;      /* IP & DATA checksum offload */
#endif

    /* Transmit and Receive enable */
    reg |= MACCR_TE | MACCR_RE;
    reg |= MACCR_PS;
    REG_ETH1.MCR |= reg;

    unl_cpu();

    sta_tsk(ID_ETH_RCV_TSK1, 0);
    sta_tsk(ID_ETH_SND_TSK1, 0);
    sta_tsk(ID_ETH_CTL_TSK1, 0);

    /* masked MMC interrupt */
    REG_ETH1.MMC_INTR_MASK_TX = 0x01FFFFFF;
    REG_ETH1.MMC_INTR_MASK_RX = 0x00FFFFFF;
    REG_ETH1.MMC_IPC_INTR_MASK_RX = 0x3FFFFFFF;

    /* Enable Interrupt mode operation */
    vset_ipl(IRQ_CAN1, IPL_ETHERNET1);
    ena_int(IRQ_CAN1);

    return E_OK;
}

ER eth_cls1(UH dev_num)
{
    return E_OK;
}

ER eth_snd1(UH dev_num, T_NET_BUF *pkt)
{
    /* Add to Device send queue */
    snd_mbx(ID_ETH_SND_MBX1, (T_MSG *)pkt);
    return E_WBLK;
}

ER eth_cfg1(UH dev_num, UH opt, VP val)
{
    return E_OK;
}

ER eth_ref1(UH dev_num, UH opt, VP val)
{
    return E_OK;
}

/*******************************
        _ddr_m3_fm3_eth_intr1
 *******************************/
void _ddr_m3_fm3_eth_intr1(VP_INT exinf)
{
    UW isr;
    UW imr;
    FLGPTN tx_ptn, rx_ptn;

    tx_ptn = 0;
    rx_ptn = 0;

    /* DMASR 
      16 : Normal interrupt summary Bit 0,2,6,14 either is set
      15 : Abnormal interrupt summary  1,3,4,5,7,8,9,10,13 is set
           (clear by writing 1 to this bit or individual bit??)
    */
    isr = REG_ETH1.SR;
    imr = REG_ETH1.IER;

    isr &= imr; /* Interrupt events */

    if (isr & DMAIER_ETE) {
        tx_ptn |= EV_ETH_TX;
    }

    if (isr & DMASR_TJT) {
        tx_ptn |= EV_ETH_TX_ERR;
    }

    if (isr & DMASR_RI) {
        rx_ptn |= EV_ETH_RX;
    }

    /*
        TDES0
            30: Interrupt on completion (sets R5[0])
    */

    /* Clear interrupt */
    if (isr) {
        REG_ETH1.SR = isr;
    }

    if (tx_ptn) {
        iset_flg(ID_ETH_SND_FLG1, tx_ptn);
    }

    if (rx_ptn) {
        iset_flg(ID_ETH_RCV_FLG1, rx_ptn);
    }

}

void dev_ctl_tsk1(VP_INT exinf)
{
    FLGPTN ptn;
    UW     tmp;
    UH     sts;

    for (;;) {
        wai_flg(ID_ETH_RCV_FLG1, EV_ETH_LNK_ERR, TWF_ORW, &ptn);
        clr_flg(ID_ETH_RCV_FLG1, ~(EV_ETH_LNK_ERR));
        LanStatus1 = 0;
        while (1) {
            sts = phy_sts1();
            if (sts & PHY_STS_ANG_DONE) {
                break;
            }
            dly_tsk(100);
        }

        loc_cpu();
        tmp = REG_ETH1.MCR;
        tmp &= ~(MACCR_DM|MACCR_FES);
        if (sts & PHY_STS_DUPLEX_FULL) {
            tmp |= MACCR_DM;
        }
        if (sts & PHY_STS_SPEED_100) {
            tmp |= MACCR_FES;
        }

        tmp |= MACCR_PS;
        REG_ETH1.MCR = tmp;
        unl_cpu();

        LanStatus1 = 1;
        set_flg(ID_ETH_RCV_FLG1, EV_ETH_LNK);
    }
}

void dev_rcv_tsk1(VP_INT exinf)
{
    T_NET_BUF *pkt;
    T_NET_DEV *dev;
    ER ercd;
    UW       rxdesc;
    UW       rxdesc_ext;
    UW       rxlen;
    T_ETHDESC1 *rxdes;
    UB       RxDescIndex;
    UH       err;
    FLGPTN   ptn;

    dev = &gNET_DEV[eth_devnum1-1];

    RxDescIndex = 0;
    
    for (;;) {

        /* DMA status:
           ----------
        6: Receive status

        4: Receive overflow status  // RDES0[11]
        7: Receive buffer unavailable status
        8: Receive process stopped status
        9: Receive watchdog timeout
       14: Early receive status
 19,18,17: Receive process state
        */

        rxdes = &EthRxDesc1[RxDescIndex];
        rxdesc = rxdes->sts;
        rxdesc_ext = rxdes->ext_sts;

        if (rxdesc & RDES0_OWN) {
            ercd = twai_flg(ID_ETH_RCV_FLG1, (EV_ETH_RX|EV_ETH_RX_ERR), TWF_ORW, &ptn, 1000);
            if (ercd == E_OK) {
                clr_flg(ID_ETH_RCV_FLG1, ~(ptn));
            }
            else {
                phy_link1();
            }
            continue;
        }

        /* Need to clear Receive status? */

        RxDescIndex++;
        if (RxDescIndex >= ETH_RXDESC_CNT1) {
            RxDescIndex = 0;
        }

        err = 0;

        if (rxdesc & RDES0_ES) {
            /* 1,3,4,6,7,11,14  */
            /* 1 CRC Error      */
            /* 3 Receive Error  */
            /* 4 Receive watchdog timeout */
            /* 6 Late collision */
            /* 7 IP Header checksum error */
            /* 11 Overflow error */
            /* 14 Descriptor error */

            if (rxdesc & RDES0_TS) {
                err   |= HW_CS_IPH4_ERR;
            }
            else if (rxdesc & (RX_ERR_BITS)){/* Drop the frame   */
                goto rx_drop;
            }
        }

        /* 2: Dribble bit error */
        if (rxdesc & RDES0_DBE) {
            goto rx_drop;
        }

        /* 5: Frame type */
        if (rxdesc & RDES0_FT) {

            /* 0: Payload checksum error */
            if (rxdesc_ext & RDES4_IPE) {
                err   |= HW_CS_DATA_ERR;
            }

            /* 7: IP Header checksum error */
            if (rxdesc_ext & RDES4_IPHE) {
                err   |= HW_CS_IPH4_ERR;
            }
        }
        else {

            if (rxdesc_ext & RDES4_IPE) {
                /* Checksum bypassed */
                err = 0;
            }
            else if (rxdesc_ext & RDES4_IPHE) {
                /* Reserved: assuming as header error */
                err |= HW_CS_IPH4_ERR;
            }
            else {
                /* IEEE 802.3 type frame */
                goto rx_drop;
            }
        }

        /* 8: Last descriptor, 9: First descriptor */
        if ((rxdesc & (RDES0_LS|RDES0_FS)) != ((RDES0_LS|RDES0_FS))) {
            goto rx_drop;
        }

        /* 12: Length Error (valid when Frame Type is set) */
        if (rxdesc & RDES0_LE) {
            /* Actual length not matching with Length/Type field */
            goto rx_drop;
        }

        /* 13: Source address filter fail */
        if (rxdesc & RDES0_SAF) {
            goto rx_drop;
        }

        /* 30: Destination address filter fail */
        if (rxdesc & RDES0_AFM) {
            /* Ignore? */
        }

        rxlen  = ((rxdesc & 0x3FFF0000) >> 16);
        rxlen -= 4; /* CRC */
        if (rxlen < ETH_HDR_SZ) {
            goto rx_drop;
        }

        pkt          = rxdes->pkt;
        pkt->hdr     = pkt->buf + 2;
        pkt->hdr_len = ETH_HDR_SZ;
        pkt->dat     = pkt->hdr + pkt->hdr_len;
        pkt->dat_len = rxlen - pkt->hdr_len;
        pkt->dev = dev;

#if (ETH_CSUM_MODE1 & 2)
        pkt->flg    |= (HW_CS_RX_IPH4|HW_CS_RX_DATA);
        pkt->flg    |= err;
#endif
        /* Protocol stack process */
        net_pkt_rcv(pkt);

        /* allocate buffer */
        ercd = net_buf_get(&pkt, ETH_MAXLEN1, TMO_FEVR);
        if (ercd != E_OK) {
            /* Fatal Error! */
            break;
        }
        rxdes->pkt = pkt;
        rxdes->buf = (UW)&pkt->buf[2];

rx_drop:
        rxdes->size = (RDES1_RCH | (0x1FFF & ETH_MAXLEN1));
        rxdes->sts  = RDES0_OWN;
    }
}

static UB TxDescIndex1 = 0;

ER EthTxFrame1(T_NET_BUF *pkt)
{
    T_ETHDESC1 *txdes;
    FLGPTN       ptn;
    UW           sts;
    ER           ercd;

    /* Check for Link Status */
    if (LanStatus1 == 0) {
        /* Wait for few milli seconds and check for link before
           giveup transmission.
           Note: EV_ETH_LNK should be set when link interrupt occurs.
                 Since the link interrupt is not used, EV_ETH_LNK will
                 never set.
        */
        twai_flg(ID_ETH_RCV_FLG1, EV_ETH_LNK, TWF_ORW, &ptn, ETH_TX_TMO1);
        if (LanStatus1 == 0) {
            return E_TMOUT;
        }
    }

    /* Get free Desc */
    txdes = &EthTxDesc1[TxDescIndex1];
    if (txdes->sts & TDES0_OWN) {
        return E_TMOUT;
    }

    TxDescIndex1++;
    if (TxDescIndex1 >= ETH_TXDESC_CNT1) {
        TxDescIndex1 = 0;
    }
    clr_flg(ID_ETH_SND_FLG1, ~(EV_ETH_TX|EV_ETH_TX_ERR));

    txdes->pkt  = pkt;
    txdes->buf  = (UW)(pkt->hdr);   /* 2byte aligned */
    txdes->size = pkt->hdr_len;     /* data length to be transmitted */

    sts = TDES0_OWN | TDES0_TCH | TDES0_LS | TDES0_FS | TDES0_IC;
#if (ETH_CSUM_MODE1 & 1)
    if (pkt->flg & HW_CS_TX_IPH4) {
        sts  |= TDES0_CIC_IPHDR;
    }
    if (pkt->flg & HW_CS_TX_DATA) {
        sts  |= TDES0_CIC_IPHDR_PSEUDO_PAYLOAD;
    }
#endif
    txdes->sts  = sts;

    loc_cpu();
    REG_ETH1.TPDR = 1;
    unl_cpu();

    ercd = twai_flg(ID_ETH_SND_FLG1, (EV_ETH_TX|EV_ETH_TX_ERR),TWF_ORW, &ptn, ETH_TX_TMO1);
    if (ptn & EV_ETH_TX_ERR) {
        ercd = E_TMOUT;
    }
    return ercd;
}

void dev_snd_tsk1(VP_INT exinf)
{
    T_NET_BUF *pkt;
    ER ercd;

    for (;;) {

        /* Wait for packet from Protocol stack */
        ercd = rcv_mbx(ID_ETH_SND_MBX1, (T_MSG **)&pkt);
        if (ercd != E_OK) {
            break;
        }

        ercd = EthTxFrame1(pkt);
        if (ercd != E_OK) {
            pkt->ercd = ercd;
        }
        loc_tcp();
        net_buf_ret(pkt);
        ulc_tcp();
    }
}
