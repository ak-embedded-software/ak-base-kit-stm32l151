/***********************************************************************
   TCP/IP Configration file

   Generated at 2014-11-25 09:44:08

 ***********************************************************************/

#include "string.h"
#include "kernel.h"
#include "net_hdr.h"
#include "net_cfg.h"
#include "kernel_id.h"
#include "net_id.h"

/*******************************************
      uNet3 User Configuration 
********************************************/

#define CFG_NET_DEV_MAX         1
#define CFG_NET_SOC_MAX         2
#define CFG_NET_TCP_MAX         1
#define CFG_NET_ARP_MAX         8
#define CFG_NET_MGR_MAX         8
#define CFG_NET_IPR_MAX         2
#define CFG_NET_BUF_SZ          1576
#define CFG_NET_BUF_OFFSET      DEF_NET_BUF_OFFSET
#define CFG_PATH_MTU            1500
#define CFG_ARP_RET_CNT         3
#define CFG_ARP_RET_TMO         1*1000
#define CFG_ARP_CLR_TMO         20*60*1000
#define CFG_IP4_TTL             64
#define CFG_IP4_TOS             0
#define CFG_IP4_IPR_TMO         10*1000
#define CFG_IP4_MCAST_TTL       DEF_IP4_MCAST_TTL
#define CFG_IGMP_V1_TMO         DEF_IGMP_V1_TMO
#define CFG_IGMP_REP_TMO        DEF_IGMP_REP_TMO
#define CFG_TCP_MSS             (CFG_PATH_MTU-40)
#define CFG_TCP_MSS_IPV6        (CFG_PATH_MTU-60)
#define CFG_TCP_RTO_INI         DEF_TCP_RTO_INI
#define CFG_TCP_RTO_MIN         DEF_TCP_RTO_MIN
#define CFG_TCP_RTO_MAX         DEF_TCP_RTO_MAX
#define CFG_TCP_SND_WND         DEF_TCP_SND_WND
#define CFG_TCP_RCV_WND         DEF_TCP_RCV_WND
#define CFG_TCP_DUP_CNT         DEF_TCP_DUP_CNT
#define CFG_TCP_CON_TMO         75*1000
#define CFG_TCP_SND_TMO         64*1000
#define CFG_TCP_CLS_TMO         75*1000
#define CFG_TCP_CLW_TMO         DEF_TCP_CLW_TMO
#define CFG_TCP_ACK_TMO         DEF_TCP_ACK_TMO
#ifdef KEEPALIVE_SUP
#define CFG_TCP_KPA_CNT         0
#define CFG_TCP_KPA_INT         1*1000
#define CFG_TCP_KPA_TMO         7200*1000
#endif
#define CFG_PKT_RCV_QUE         1
#define CFG_PKT_CTL_FLG         0x0000
#ifdef ACD_SUP
#define CFG_ARP_PRB_WAI         DEF_ARP_PRB_WAI
#define CFG_ARP_PRB_NUM         DEF_ARP_PRB_NUM
#define CFG_ARP_PRB_MIN         DEF_ARP_PRB_MIN
#define CFG_ARP_PRB_MAX         DEF_ARP_PRB_MAX
#define CFG_ARP_ANC_WAI         DEF_ARP_ANC_WAI
#define CFG_ARP_ANC_NUM         DEF_ARP_ANC_NUM
#define CFG_ARP_ANC_INT         DEF_ARP_ANC_INT
#endif

/*******************************************
       uNet3 Resources
********************************************/

UW NET_SOC_MAX      = CFG_NET_SOC_MAX;  /* Maximum Number of Sockets          */
UW NET_TCP_MAX      = CFG_NET_TCP_MAX;  /* Maximum Number of TCP Sockets      */
UW NET_DEV_MAX      = CFG_NET_DEV_MAX;  /* Maximum No. of Network Interface   */
UW NET_ARP_MAX      = CFG_NET_ARP_MAX;  /* Maximum No. of ARP Cache           */
UW NET_MGR_MAX      = CFG_NET_MGR_MAX;  /* Maximum No. of Multicast Table     */
UW NET_IPR_MAX      = CFG_NET_IPR_MAX;  /* Maximum No. of IP Reassembly Queue */
UW NET_BUF_SZ       = CFG_NET_BUF_SZ;

/*******************************************
      Define TCP Resources
********************************************/

T_NET     gNET[CFG_NET_DEV_MAX];    /* Network Interface control block */
T_NET_DEV gNET_DEV[CFG_NET_DEV_MAX];/* Network Device control block */
T_NET_ARP gNET_ARP[CFG_NET_ARP_MAX];/* ARP Cache */
T_NET_MGR gNET_MGR[CFG_NET_MGR_MAX];/* Multicast Group Table */
T_NET_IPR gNET_IPR[CFG_NET_IPR_MAX];/* IP Reassembly Queue */
T_NET_SOC gNET_SOC[CFG_NET_SOC_MAX];/* Socket control block */
T_NET_TCP gNET_TCP[CFG_NET_TCP_MAX];/* TCP control block */
UB        gTCP_SND_BUF[260];       /* TCP Tx Buffer */

/*******************************
    Socket Table
********************************/

T_SOC_TABLE const gSOC_TABLE[] = {
    { ID_SOC1, 17, 60004, -1, -1, -1, -1, 1024, 1024, ID_NETIF_DEV1, IP_VER4},
    { MBMASTER_S1, 6, 0, -1, -1, 1000, 1000, 260, 260, 0, IP_VER4},
};

/*******************************
    Define Local IP Address
********************************/

T_NET_ADR gNET_ADR[] = {
    {0x0, 0x0, 0xAC100002, 0x00000000, 0xFFFFFF00}, 
};

/*******************************************
      Initialize TCP/IP Globals
********************************************/

const VP net_inftbl[] = {
(VP)gSOC_TABLE,
(VP)gNET_SOC,
(VP)gNET_TCP,
(VP)gNET_IPR,
(VP)gNET_MGR,
(VP)gTCP_SND_BUF
};

/*******************************************
      Define TCP/IP Default Parameters
********************************************/

T_NET_CFG gNET_CFG[]  = {
{
FALSE,
1500,
CFG_ARP_RET_CNT,
CFG_ARP_RET_TMO,
CFG_ARP_CLR_TMO,
#ifdef ACD_SUP
CFG_ARP_PRB_WAI,
CFG_ARP_PRB_NUM,
CFG_ARP_PRB_MIN,
CFG_ARP_PRB_MAX,
CFG_ARP_ANC_WAI,
CFG_ARP_ANC_NUM,
CFG_ARP_ANC_INT,
#endif
CFG_IP4_TTL,
CFG_IP4_TOS,
CFG_IP4_IPR_TMO,
CFG_IP4_MCAST_TTL,  /* Default Multicast TTL */
CFG_IGMP_V1_TMO,
CFG_IGMP_REP_TMO,
1460,
CFG_TCP_RTO_INI,
CFG_TCP_RTO_MIN,
CFG_TCP_RTO_MAX,
CFG_TCP_SND_WND,
CFG_TCP_RCV_WND,
CFG_TCP_DUP_CNT,
CFG_TCP_CON_TMO,
CFG_TCP_SND_TMO,
CFG_TCP_CLS_TMO,
CFG_TCP_CLW_TMO,
CFG_TCP_ACK_TMO,
#ifdef KEEPALIVE_SUP
CFG_TCP_KPA_CNT,
CFG_TCP_KPA_INT,
CFG_TCP_KPA_TMO,
#endif
CFG_PKT_RCV_QUE,
CFG_PKT_CTL_FLG
},
};

/*******************************************
     Define Driver Information
********************************************/

extern ER eth_ini(UH dev_num);
extern ER eth_cls(UH dev_num);
extern ER eth_cfg(UH dev_num, UH opt, VP val);
extern ER eth_ref(UH dev_num, UH opt, VP val);
extern ER eth_snd(UH dev_num, T_NET_BUF *pkt);

T_NET_DEV gNET_DEV[] = {
{
  "",                 /* Device Name      */
  ID_NETIF_DEV1,      /* Device Number    */
  NET_DEV_TYPE_ETH,   /* Device Type      */
  0,                  /* Status           */
  0,                  /* Flags            */
  eth_ini,            /* Device Init      */
  eth_cls,            /* Device Close     */
  eth_cfg,            /* Device Configure */
  eth_ref,            /* Device Status    */
  eth_snd,            /* Device Transmit  */
  0,                  /* Device Callback  */
  0,
  {{{ 0x12, 0x34, 0x56, 0x78, 0x58, 0x5C }}},   /* MAC Address */
  0,                  /* Link Header Size */
  CFG_NET_BUF_OFFSET  /* Network buffer data Offset */
},
};

/*******************************************
     Define TCP/IP Kernel resource
********************************************/

ID ID_NET_TSK = ID_TCP_TIM_TSK;
ID ID_NET_SEM = ID_TCP_SEM;
ID ID_NET_MPF = ID_TCP_MPF;

/*******************************
      Initialize Network
 *******************************/
ER net_setup(void)
{
    ER ercd;

    /* Initialize TCP/IP Stack */
    ercd = net_ini();
    if (ercd != E_OK) {
        return ercd;
    }

    /* Initialize Network Driver */
    ercd = net_dev_ini(ID_NETIF_DEV1);
    if (ercd != E_OK) {
        return ercd;
    }


    return ercd;
}

/*******************************
     memory function
 *******************************/
VP net_memset(VP d, int c, UINT n)
{
#ifdef _KERNEL_MEMSET_
    return _kernel_memset(d, c, n);
#else
    return memset(d, c, n);
#endif
}

VP net_memcpy(VP d, VP s, UINT n)
{
#ifdef _KERNEL_MEMCPY_
    return _kernel_memcpy(d, s, n);
#else
    return memcpy(d, s, n);
#endif
}

int net_memcmp(VP d, VP s, UINT n)
{
#ifdef _KERNEL_MEMCMP_
    return _kernel_memcmp(d, s, n);
#else
    return memcmp(d, s, n);
#endif
}

/* end */
