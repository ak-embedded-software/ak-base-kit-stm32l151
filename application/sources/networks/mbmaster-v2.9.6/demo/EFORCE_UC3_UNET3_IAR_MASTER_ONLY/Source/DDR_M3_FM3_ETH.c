/***********************************************************************
    MICRO C CUBE / COMPACT, DEVICE DRIVER
    Ethernet driver for Fujitsu Semicon FM3 MB9BFxxx (MB9BF61x, MB9BF21x)
    Copyright (c)  2012, eForce Co., Ltd. All rights reserved.

    2012/3/06: Created
    2012/6/29: The return value judging of phy_sts() is changed into
                "PHY_STS_LINK_UP" into a phy_link function.
    2012/10/05: Some of definitions moved to DDR_M3_FM3_ETH.h from
                DDR_M3_FM3_ETH.c.
    2013/05/27: Modified incorrect operation received descriptor.
    2013/05/29: Masked unnecessary MMC interrupt.
    2014/04/22: The completion waiting of DMA reset was changed for
                timeout in DMA_Reset().
 ***********************************************************************/

#include <string.h>
#include "kernel.h"
#include "DDR_M3_FM3_ETH_cfg.h"
#include "DDR_M3_FM3_ETH.h"
#include "DDR_PHY.h"
#include "net_hdr.h"
#ifdef NET_C_OS
#include "kernel_id.h"
#endif

/* I/O Port Set register (PFR,PCR,DDR,PDIR,PDOR) */
#define PRT_0    0x0001    /* PORT0 */
#define PRT_1    0x0002    /* PORT1 */
#define PRT_2    0x0004    /* PORT2 */
#define PRT_3    0x0008    /* PORT3 */
#define PRT_4    0x0010    /* PORT4 */
#define PRT_5    0x0020    /* PORT5 */
#define PRT_6    0x0040    /* PORT6 */
#define PRT_7    0x0080    /* PORT7 */
#define PRT_8    0x0100    /* PORT8 */
#define PRT_9    0x0200    /* PORT9 */
#define PRT_A    0x0400    /* PORTA */
#define PRT_B    0x0800    /* PORTB */
#define PRT_C    0x1000    /* PORTC */
#define PRT_D    0x2000    /* PORTD */
#define PRT_E    0x4000    /* PORTE */
#define PRT_F    0x8000    /* PORTF */

/* Extension Function control register (EPFR) */
#define E_TD0E  0x00040000 /*  */
#define E_TD1E  0x00080000 /*  */
#define E_TE0E  0x00100000 /*  */
#define E_TE1E  0x00200000 /*  */
#define E_MC0E  0x00400000 /*  */
#define E_MC1B  0x00800000 /*  */
#define E_MD0B  0x01000000 /*  */
#define E_MD1B  0x02000000 /*  */
#define E_CKE   0x04000000 /*  */
#define E_PSE   0x08000000 /*  */

/*
    Configuration Macros for Ethernet ch0
*/

#ifndef ETH_PHY_ADDR
#define ETH_PHY_ADDR    0x05
#endif

#ifndef ETH_EXT_CLK
#define ETH_EXT_CLK     1
#endif

#ifndef ETH_CLK_SRC
#define ETH_CLK_SRC     0
#endif

#ifndef ETH_CLK_UBSR
#define ETH_CLK_UBSR    6
#endif

#ifndef ETH_CLK_UPLLK
#define ETH_CLK_UPLLK   0
#endif

#ifndef ETH_CLK_UPLLN
#define ETH_CLK_UPLLN   49
#endif

#ifndef ETH_CLK_UPLLM
#define ETH_CLK_UPLLM   7
#endif

#ifndef ETH_MDC_CLK
#define ETH_MDC_CLK     MACMIIAR_CR_62
#endif

#ifndef ETH_RMII_MODE
#define ETH_RMII_MODE   0           /* 0 - MII, 1 - RMII */
#endif

#ifndef ETH_PHY_MODE
#define ETH_PHY_MODE    0
#endif

#ifndef ETH_FILTER_MODE
#define ETH_FILTER_MODE 0
#endif

#ifndef ETH_RXDESC_CNT
#define ETH_RXDESC_CNT  4
#endif

#ifndef ETH_TXDESC_CNT
#define ETH_TXDESC_CNT  4
#endif

#ifndef ETH_CSUM_MODE
#define ETH_CSUM_MODE   0
#endif

#ifndef ETH_TX_TMO
#define ETH_TX_TMO    100
#endif

#ifndef PHY_RST_WAIT_TMO
#define PHY_RST_WAIT_TMO    5000
#endif

#ifndef ETH_TIMESTAMP
#define ETHE_TIMESTAMP  0
#endif

/*
    Configuration Macros for Ethernet ch1
*/

#ifndef ETH_RMII_MODE1
#define ETH_RMII_MODE1   0           /* 0 - MII, 1 - RMII */
#endif

/*
    Definitions
*/
UINT    LanStatus;

#define TX_HW_CS_ENABLE
#define RX_HW_CS_ENABLE

/*
    DMA Descriptor Chain
*/
typedef struct t_ethdesc {
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
}T_ETHDESC;

static T_ETHDESC     EthRxDesc[ETH_RXDESC_CNT];
static T_ETHDESC     EthTxDesc[ETH_TXDESC_CNT];

#ifndef ETH_MAXLEN
#define ETH_MAXLEN  1520U   /* Maximum ethernet frame size (TCP/IP MTU + 20) */
#endif

static UH eth_devnum;       /* Device Number*/

void InitClockSetting(void)
{
    /* Ethernet Clock Configure */
#if (ETH_EXT_CLK == 0)
    UW tmp;

    REG_USBC.UCCR &= ~(UCCR_ETH_UCEN0 | UCCR_ETH_UCEN1 | UCCR_ETH_ECEN);
    do {
        tmp = REG_USBC.UCCR;
    } while ((tmp & UCCR_ETH_UCEN0 == UCCR_ETH_UCEN0) ||
             (tmp & UCCR_ETH_UCEN1 == UCCR_ETH_UCEN1) ||
             (tmp & UCCR_ETH_UCEN1 == UCCR_ETH_ECEN));
    
    REG_USBC.UPCR1 &= ~(UPCR1_ETH_UPLLEN);
    
    REG_USBC.UCCR |= UCCR_ETH_SEL_PLL;        /* Ethernet clock select USB/Eth PLL */

    tmp = REG_USBC.UCCR & UCCR_ETH_SEL_PLL;
    if (tmp == UCCR_ETH_SEL_PLL){
        REG_USBC.UPCR1 &= ~(UPCR1_ETH_UPINC);
        REG_USBC.UPCR2 |= 0x00000003;         /* PLL waiting stability time(1.02ms)*/
        
        REG_USBC.UPCR3 |= 0x00000001;         /* PLL control register K */
        REG_USBC.UPCR4 |= 0x00000031;         /* PLL control register N */
        REG_USBC.UPCR5 |= 0x00000007;         /* PLL control register M */
        
        REG_USBC.UPCR1 |= UPCR1_ETH_UPLLEN;   /* PLL enable             */
        
        REG_USBC.UPINT_ENR &= ~(UPINT_ENR_UPCSE);
        REG_USBC.UPINT_CLR |= UPINT_CLR_UPCSC;
        REG_USBC.UPINT_ENR |= UPINT_ENR_UPCSE;
        
        REG_USBC.UPCR1 |= UPCR1_ETH_UPINC;
        while((REG_USBC.UPCR1 & UPCR1_ETH_UPINC) != UPCR1_ETH_UPINC);

        REG_USBC.UCCR |= UCCR_ETH_ECEN;
    } 
#elif (ETH_EXT_CLK == 1)
    REG_USBC.UCCR &= ~UCCR_ETH_ECEN;
#endif
}

void EthPortInit(void)
{
#if (ETH_EXT_CLK == 0)
    REG_GPIO.PFRC |= (PRT_B);
    REG_GPIO.PCRC |= (PRT_B);
#endif  

/*
    USE ETHER CHANNEL 0 for MII
*/    
#if (ETH_RMII_MODE == 0)
    REG_GPIO.PFRC |= (PRT_0 | PRT_1 | PRT_2 | PRT_9 | PRT_A | PRT_D | PRT_E | PRT_F);
    REG_GPIO.PFRD |= (PRT_0);
    REG_GPIO.PCRC |= (PRT_0 | PRT_1 | PRT_2 | PRT_9 | PRT_A);
#endif  

/*
    USE ETHER CHANNEL 0 for RMII or MII
*/
    REG_GPIO.PFRC |= (PRT_3 | PRT_4 | PRT_5 | PRT_6 | PRT_7);
    REG_GPIO.PFRD |= (PRT_1 | PRT_2 | PRT_3);
    REG_GPIO.PCRC |= (PRT_3 | PRT_4 | PRT_5 | PRT_6 | PRT_7);
    
/*
    USE ETHER CHANNEL 1 for RMII
*/
#if (ETH_RMII_MODE1 == 1)
    REG_GPIO.PFRC |= (PRT_0 | PRT_1 | PRT_2 | PRT_C | PRT_D | PRT_E | PRT_F);
    REG_GPIO.PFRD |= (PRT_0);
    REG_GPIO.PCRC |= (PRT_0 | PRT_1 | PRT_2 | PRT_C);
#endif 

    
#if (ETH_EXT_CLK == 0)
    REG_GPIO.EPFR14 |= E_CKE;
#endif  
    
/*
    USE ETHER CHANNEL 0 for MII
*/   
#if (ETH_RMII_MODE == 0)
    REG_GPIO.EPFR14 |= E_MC1B | E_TE1E | E_TD1E;
#endif  

/*
    USE ETHER CHANNEL 0 for RMII or MII
*/
    REG_GPIO.EPFR14 |= E_MD0B | E_MC0E | E_TE0E | E_TD0E;

/*
    USE ETHER CHANNEL 1 for RMII
*/
#if (ETH_RMII_MODE1 == 1)
    REG_GPIO.EPFR14 |= E_MD1B | E_MC1B | E_TE1E | E_TD1E;
#endif 

}

void EthRxBufferInit(void)
{
    T_NET_BUF *pkt;
    ER ercd;
    int i;

    for (i=0;i<ETH_RXDESC_CNT;i++) {
        EthRxDesc[i].pkt = NULL;
        ercd = net_buf_get(&pkt, ETH_MAXLEN, TMO_POL);
        if (ercd != E_OK) {
            break;
        }        
        EthRxDesc[i].pkt = pkt;
    }
}

void EthDMARxDescInit(void)
{
    T_NET_BUF *pkt;
    int i;

    EthRxBufferInit();

    /* Construct Rx Descriptor chain */
    for (i=0;i<ETH_RXDESC_CNT;i++) {
        pkt = EthRxDesc[i].pkt;
        EthRxDesc[i].size = (RDES1_RCH | (0x1FFF & ETH_MAXLEN));
        EthRxDesc[i].buf = (UW)&pkt->buf[2];
        EthRxDesc[i].sts  = RDES0_OWN;
        if ((i + 1) < ETH_RXDESC_CNT) {
            EthRxDesc[i].next = (UW)(&EthRxDesc[i+1]);
        }
        else {
            EthRxDesc[i].next = (UW)(&EthRxDesc[0]);
            EthRxDesc[i].sts |= RDES0_LS;
        }
    }

    /* Set top descriptor address */
    REG_ETH0.RDLAR    = (UW)(&EthRxDesc[0]);
}

void EthDMATxDescInit(void)
{
    int i;

    /* Construct Tx Descriptor chain */
    for (i=0;i<ETH_TXDESC_CNT;i++) {
        EthTxDesc[i].sts  = TDES0_TCH;
        EthTxDesc[i].size = 0;
        EthTxDesc[i].buf  = 0;
        EthTxDesc[i].pkt  = NULL;
        if ((i + 1) < ETH_TXDESC_CNT) {
            EthTxDesc[i].next = (UW)(&EthTxDesc[i+1]);
        }
        else {
            EthTxDesc[i].next = (UW)(&EthTxDesc[0]);
        }
    }

    /* Set top descriptor address */
    REG_ETH0.TDLAR    = (UW)(&EthTxDesc[0]);
}

static void phy_put(UB reg, UH data)
{
    UW tmp;

    loc_cpu();
    REG_ETH0.GDR = data;
    REG_ETH0.GAR = (ETH_PHY_ADDR << 11) |
                       (reg      << 6)  |
                       (ETH_MDC_CLK <<2)|
                       MACMIIAR_MW      |
                       MACMIIAR_MB      ;
    do {
        tmp = REG_ETH0.GAR;
        unl_cpu();
        loc_cpu();
    } while (tmp & MACMIIAR_MB);
    unl_cpu();
}

static UH phy_get(UB reg)
{
    UW tmp;
    UW dat;

    loc_cpu();
    REG_ETH0.GAR = (ETH_PHY_ADDR << 11) |
                       (reg      << 6)  |
                       (ETH_MDC_CLK <<2)|
                       MACMIIAR_MB      ;

    do {
        tmp = REG_ETH0.GAR;
        unl_cpu();
        loc_cpu();
    } while (tmp & MACMIIAR_MB);
    dat = REG_ETH0.GDR;
    unl_cpu();
    return dat;
}

UH phy_sts(void)
{
    UH dat, sts;

    sts = PHY_STS_LINK_DOWN;

    /****** Check Link Status *****/
    phy_get(PHY_BMSR);  /* released latched status (Dummy read) */
    dat = phy_get(PHY_BMSR);
    if (!(dat & BMSR_LINK_STAT)) {
        /* Link Down */
        return sts;
    }
    sts |= PHY_STS_LINK_UP;

#if (ETH_PHY_MODE == 0 || ETH_PHY_MODE == 2 || ETH_PHY_MODE == 4)
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
    dat = phy_get(PHY_ANLPAR);  /* Link partner abilities */
    dat &= phy_get(PHY_ANAR);   /* Local station abilities */

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

ER phy_ini(void)
{
    int i;

    /* Reset PHY */
    phy_put(PHY_BMCR, BMCR_RESET);

    i = PHY_RST_WAIT_TMO/10;
    for (;;) {
        dly_tsk(10);
        if (!(phy_get(PHY_BMCR) & BMCR_RESET)) {
            break;
        }
        if (i-- <= 0) {
            return E_TMOUT;     /* Error: PHY reset command error */
        }
    }

    phy_get(PHY_BMSR);          /* Dummy status read */

#if (ETH_PHY_MODE == 1)
    /* 10M Half Duplex manual mode */
    phy_put(PHY_BMCR, 0);
#elif (ETH_PHY_MODE == 2)
    /* 10M Full/Half (Duplex auto select mode) */
    phy_put(PHY_ANAR, ANAR_10_FD | ANAR_10 | ANAR_SF_802_3u);
    phy_put(PHY_BMCR, BMCR_ANE | BMCR_RS_ANP);
#elif (ETH_PHY_MODE == 3)
    /* 100M Half Duplex manual mode */
    phy_put(PHY_BMCR, BMCR_SPD);
#elif (ETH_PHY_MODE == 4)
    /* 100M Full/Half (Duplex auto select mode) */
    phy_put(PHY_ANAR, ANAR_TX_FD | ANAR_TX | ANAR_SF_802_3u);
    phy_put(PHY_BMCR, BMCR_ANE | BMCR_RS_ANP);
#else
    /* Auto select mode (10HD/10FD/100HD/100FD) */
    phy_put(PHY_ANAR, ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10 | ANAR_SF_802_3u);
    phy_put(PHY_BMCR, BMCR_ANE | BMCR_RS_ANP);
#endif
    return E_OK;
}

void phy_link(void)
{
    UW  tmp;
    UH  sts;

    while (1) {
        sts = phy_sts();
        if (sts & PHY_STS_LINK_UP) {
            break;
        }
        LanStatus = 0;
        dly_tsk(50);
    }

    if (LanStatus) {
        return;
    }

    tmp = REG_ETH0.MCR;
    tmp &= ~(MACCR_DM|MACCR_FES);
    if (sts & PHY_STS_DUPLEX_FULL) {
        tmp |= MACCR_DM;
    }
    if (sts & PHY_STS_SPEED_100) {
        tmp |= MACCR_FES;
    }
    tmp |= MACCR_PS;
    REG_ETH0.MCR = tmp;

    LanStatus = 1;

    /* Restart Tx & Rx */
}

void EthReset(void)
{
    loc_cpu();

/*
    Clock Gating Register for Ethernet ch0 and ch1, or ch0
*/
#if(ETH_RMII_MODE1 == 1)
    REG_ETH_MODE.CLKG = ETHCLKG_MACEN1_0; /* ch0 and ch1 */
#else
    REG_ETH_MODE.CLKG = ETHCLKG_MACEN0;   /* ch0 */
#endif


#if(ETH_RMII_MODE == 1)
    REG_ETH_MODE.MODE |= ETHMODE_IFMODE;  /* Select RMII */
#else
    REG_ETH_MODE.MODE &= ~ETHMODE_IFMODE; /* Select MII */
#endif   


    REG_ETH_MODE.MODE |= ETHMODE_RST0;       /* ETH0 MAC RST */
    REG_ETH_MODE.MODE &= ~ETHMODE_RST0;      /* ETH0 MAC RST Cancel */
#if(ETH_RMII_MODE1 == 1)
    REG_ETH_MODE.MODE |= ETHMODE_RST1;       /* ETH1 MAC RST */
    REG_ETH_MODE.MODE &= ~ETHMODE_RST1;      /* ETH1 MAC RST Cancel */
#endif

    unl_cpu();
}

ER DMA_Reset(void)
{
    UW tmp;
    int i;

    /* Software Reset */
    loc_cpu();
    REG_ETH0.BMR |= DMABMR_SWR; 
    unl_cpu();	
    /* Wait until reset operation completes */
    i = 50;
    for (;;) {
        tmp = REG_ETH0.BMR;
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
        tmp = REG_ETH0.AHBSR;
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

ER eth_ini(UH dev_num)
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
    eth_devnum = dev_num;

    net = &gNET[eth_devnum-1];
    if (net == NULL) {
        return E_PAR;
    }
    dev = net->dev;
    if (dev == NULL) {
        return E_PAR;
    }

#if (ETH_CSUM_MODE & 1)
    net->flag |= HW_CS_TX_IPH4;
    net->flag |= HW_CS_TX_DATA;
#endif

    LanStatus = 0;

    loc_cpu();
    /* GPIO Port Setup */
    EthPortInit();    
    
    /* Enable Ethernet Peripheral Clock */
    InitClockSetting();
    /* Reset MAC */
    EthReset();
    unl_cpu();  

    /***** DMA Initialization *****/
    ercd = DMA_Reset();
    if (ercd != E_OK) {
        return ercd;
    }

    loc_cpu();
    /* GMAC configration register, PS=1 for MII and RMII */
    REG_ETH0.MCR |= MACCR_PS; 
    
    /* DMA bus mode         */
    REG_ETH0.BMR |= DMABMR_PBL_1_1;  /* DA DMA Arbitration - TX/RX has same priority */

    REG_ETH0.BMR |= DMABMR_ATDS;  /* TX & RX Descriptor size extend */

    /* DMA Descriptors Initialize */
    EthDMARxDescInit();
    EthDMATxDescInit();

    /* DMA operation mode   */
    REG_ETH0.OMR |= DMAOMR_RSF | DMAOMR_TSF | DMAOMR_DT;

    /* Interrupt req clear */
    REG_ETH0.SR  |= DMASR_NIS;   
    
    /* Interrupt */
    REG_ETH0.IER |= (DMAIER_NIE | DMAIER_AIE | DMAIER_ETE | TX_ERR_BITS | DMAIER_RIE);

    /* Wait until AHB master interface is not idle */
    do {
        tmp = REG_ETH0.AHBSR;
        unl_cpu();
        loc_cpu();
    }
    while (tmp & DMAAHB_AHBS);
    
    /* Start Tx & Rx */
    REG_ETH0.OMR |= (DMAOMR_SR | DMAOMR_ST);
    unl_cpu();  

    /***** PHY Initialization *****/
    ercd = phy_ini();
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
    REG_ETH0.MCR |= reg;

    unl_cpu();
    dly_tsk(2);
    loc_cpu();
    
    /* Station address setting */
    mac = &dev->cfg.eth.mac[0];

    REG_ETH0.MAR0H = ((mac[5] << 8 )| mac[4] | MACMAR0H_MO);
    REG_ETH0.MAR0L = ((mac[3] << 24)| (mac[2] << 16) | (mac[1] << 8) | mac[0]);

    /* Rx Filter */
#if  (ETH_FILTER_MODE == 1)
    REG_ETH0.MFFR |= MACFFR_PR;     /* Receive All packets  */
#elif (ETH_FILTER_MODE == 2)
    REG_ETH0.MFFR |= MACFFR_PM;     /* Receive Multicast    */
#endif

    /* Checksum */
    reg = 0;
#if (ETH_CSUM_MODE & 2)
    reg |= MACCR_IPC;      /* IP & DATA checksum offload */
#endif

    /* Transmit and Receive enable */
    reg |= MACCR_TE | MACCR_RE;
    reg |= MACCR_PS;
    REG_ETH0.MCR |= reg;

    unl_cpu();

    sta_tsk(ID_ETH_RCV_TSK, 0);
    sta_tsk(ID_ETH_SND_TSK, 0);
    sta_tsk(ID_ETH_CTL_TSK, 0);

    /* masked MMC interrupt */
    REG_ETH0.MMC_INTR_MASK_TX = 0x01FFFFFF;
    REG_ETH0.MMC_INTR_MASK_RX = 0x00FFFFFF;
    REG_ETH0.MMC_IPC_INTR_MASK_RX = 0x3FFFFFFF;

    /* Enable Interrupt mode operation */
    vset_ipl(IRQ_CAN0, IPL_ETHERNET);
    ena_int(IRQ_CAN0);

    return E_OK;
}

ER eth_cls(UH dev_num)
{
    return E_OK;
}

ER eth_snd(UH dev_num, T_NET_BUF *pkt)
{
    /* Add to Device send queue */
    snd_mbx(ID_ETH_SND_MBX, (T_MSG *)pkt);
    return E_WBLK;
}

ER eth_cfg(UH dev_num, UH opt, VP val)
{
    return E_OK;
}

ER eth_ref(UH dev_num, UH opt, VP val)
{
    return E_OK;
}

/*******************************
        _ddr_m3_fm3_eth_intr0
 *******************************/
void _ddr_m3_fm3_eth_intr0(VP_INT exinf)
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
    isr = REG_ETH0.SR;
    imr = REG_ETH0.IER;

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
        REG_ETH0.SR = isr;
    }

    if (tx_ptn) {
        iset_flg(ID_ETH_SND_FLG, tx_ptn);
    }

    if (rx_ptn) {
        iset_flg(ID_ETH_RCV_FLG, rx_ptn);
    }

}

void dev_ctl_tsk(VP_INT exinf)
{
    FLGPTN ptn;
    UW     tmp;
    UH     sts;

    for (;;) {
        wai_flg(ID_ETH_RCV_FLG, EV_ETH_LNK_ERR, TWF_ORW, &ptn);
        clr_flg(ID_ETH_RCV_FLG, ~(EV_ETH_LNK_ERR));
        LanStatus = 0;
        while (1) {
            sts = phy_sts();
            if (sts & PHY_STS_ANG_DONE) {
                break;
            }
            dly_tsk(100);
        }

        loc_cpu();
        tmp = REG_ETH0.MCR;
        tmp &= ~(MACCR_DM|MACCR_FES);
        if (sts & PHY_STS_DUPLEX_FULL) {
            tmp |= MACCR_DM;
        }
        if (sts & PHY_STS_SPEED_100) {
            tmp |= MACCR_FES;
        }

        tmp |= MACCR_PS;
        REG_ETH0.MCR = tmp;
        unl_cpu();

        LanStatus = 1;
        set_flg(ID_ETH_RCV_FLG, EV_ETH_LNK);
    }
}

void dev_rcv_tsk(VP_INT exinf)
{
    T_NET_BUF *pkt;
    T_NET_DEV *dev;
    ER ercd;
    UW       rxdesc;
    UW       rxdesc_ext;
    UW       rxlen;
    T_ETHDESC *rxdes;
    UB       RxDescIndex;
    UH       err;
    FLGPTN   ptn;

    dev = &gNET_DEV[eth_devnum-1];

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

        rxdes = &EthRxDesc[RxDescIndex];
        rxdesc = rxdes->sts;
        rxdesc_ext = rxdes->ext_sts;

        if (rxdesc & RDES0_OWN) {
            ercd = twai_flg(ID_ETH_RCV_FLG, (EV_ETH_RX|EV_ETH_RX_ERR), TWF_ORW, &ptn, 1000);
            if (ercd == E_OK) {
                clr_flg(ID_ETH_RCV_FLG, ~(ptn));
            }
            else {
                phy_link();
            }
            continue;
        }

        /* Need to clear Receive status? */

        RxDescIndex++;
        if (RxDescIndex >= ETH_RXDESC_CNT) {
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

#if (ETH_CSUM_MODE & 2)
        pkt->flg    |= (HW_CS_RX_IPH4|HW_CS_RX_DATA);
        pkt->flg    |= err;
#endif
        /* Protocol stack process */
        net_pkt_rcv(pkt);

        /* allocate buffer */
        ercd = net_buf_get(&pkt, ETH_MAXLEN, TMO_FEVR);
        if (ercd != E_OK) {
            /* Fatal Error! */
            break;
        }
        rxdes->pkt = pkt;
        rxdes->buf = (UW)&pkt->buf[2];

rx_drop:
        rxdes->size = (RDES1_RCH | (0x1FFF & ETH_MAXLEN));
        rxdes->sts  = RDES0_OWN;
    }
}

static UB TxDescIndex = 0;

ER EthTxFrame(T_NET_BUF *pkt)
{
    T_ETHDESC *txdes;
    FLGPTN       ptn;
    UW           sts;
    ER           ercd;

    /* Check for Link Status */
    if (LanStatus == 0) {
        /* Wait for few milli seconds and check for link before
           giveup transmission.
           Note: EV_ETH_LNK should be set when link interrupt occurs.
                 Since the link interrupt is not used, EV_ETH_LNK will
                 never set.
        */
        twai_flg(ID_ETH_RCV_FLG, EV_ETH_LNK, TWF_ORW, &ptn, ETH_TX_TMO);
        if (LanStatus == 0) {
            return E_TMOUT;
        }
    }

    /* Get free Desc */
    txdes = &EthTxDesc[TxDescIndex];
    if (txdes->sts & TDES0_OWN) {
        return E_TMOUT;
    }

    TxDescIndex++;
    if (TxDescIndex >= ETH_TXDESC_CNT) {
        TxDescIndex = 0;
    }
    clr_flg(ID_ETH_SND_FLG, ~(EV_ETH_TX|EV_ETH_TX_ERR));

    txdes->pkt  = pkt;
    txdes->buf  = (UW)(pkt->hdr);   /* 2byte aligned */
    txdes->size = pkt->hdr_len;     /* data length to be transmitted */

    sts = TDES0_OWN | TDES0_TCH | TDES0_LS | TDES0_FS | TDES0_IC;
#if (ETH_CSUM_MODE & 1)
    if (pkt->flg & HW_CS_TX_IPH4) {
        sts  |= TDES0_CIC_IPHDR;
    }
    if (pkt->flg & HW_CS_TX_DATA) {
        sts  |= TDES0_CIC_IPHDR_PSEUDO_PAYLOAD;
    }
#endif
    txdes->sts  = sts;

    loc_cpu();
    REG_ETH0.TPDR = 1;
    unl_cpu();

    ercd = twai_flg(ID_ETH_SND_FLG, (EV_ETH_TX|EV_ETH_TX_ERR),TWF_ORW, &ptn, ETH_TX_TMO);
    if (ptn & EV_ETH_TX_ERR) {
        ercd = E_TMOUT;
    }
    return ercd;
}

void dev_snd_tsk(VP_INT exinf)
{
    T_NET_BUF *pkt;
    ER ercd;

    for (;;) {

        /* Wait for packet from Protocol stack */
        ercd = rcv_mbx(ID_ETH_SND_MBX, (T_MSG **)&pkt);
        if (ercd != E_OK) {
            break;
        }

        ercd = EthTxFrame(pkt);
        if (ercd != E_OK) {
            pkt->ercd = ercd;
        }
        loc_tcp();
        net_buf_ret(pkt);
        ulc_tcp();
    }
}
