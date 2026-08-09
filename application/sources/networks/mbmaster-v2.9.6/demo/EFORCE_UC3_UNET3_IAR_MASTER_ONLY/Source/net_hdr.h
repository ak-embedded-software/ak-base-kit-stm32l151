/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    Protocol headers, internal structure & macro definitions
    Copyright (c)  2008-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.19: Created
                         2010.01.18: 'hhdrsz' field in T_NET_DEV, PPP_HDR_SZ
                         2010.03.24: SOC_FLG_PRT check
                         2010.08.10: Updated for broadcast IP address check
                         2010.08.10: Added net_rand() & net_rand_seed()
                         2010.08.17: Support SOC_RCV_PKT_INF socket option
                         2010.10.02: Updated for IPv6 support
                         2010.11.10: Updated for any network interface
                         2011.01.31: Added laddr6[4] as member of t_net_soc
                         2011.03.07: Updated for received packet queue
                         2011.04.04: Added definition of TCP/MSS for IPv6
                         2011.05.20: Set up networkbuffer offset
                         2011.11.14: Added ACD parameter
                         2011.11.14: Implemented Address Conflict Detection
                         2012.05.21: Implemented Keep-Alive
                         2012.06.06: Implemented port unreachable with ICMPv4
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.04.10  H/W OS supported version
                         2013.07.23  Implemented ready I/O event notification.
                         2013.08.06: Implememted multicast per socket
                         2013.09.25: Implememted local loopback
                         2013.12.06  Change to external function to allocate buffer
 **************************************************************************/

#ifndef NETHDR_H
#define NETHDR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "net_cfg.h"

#ifdef IPV6_SUP
#include "net_hdr6.h"
#endif
/***************************************************************************
 *  Protocol Header Definition
 */

/***************************************************************************
    PPP
***************************************************************************/
#define PPP_HDR_SZ      2   /* for network buffer header alignment */

/***************************************************************************
    Ethernet
***************************************************************************/

#define ETH_TYPE_IP4   0x0800  /* Ethernet Type of IP Protocol  */
#define ETH_TYPE_ARP   0x0806  /* Ethernet Type of ARP Protocol */
#define ETH_TYPE_IP6   0x86DD  /* Ethernet Type of IP6 Protocol */

typedef struct t_eth_hdr {
    UB da[6];
    UB sa[6];
    UH type;
}T_ETH_HDR;
#define ETH_HDR_SZ      14

/**************************************************************************
    ARP
**************************************************************************/

#define ARP_TYPE_ETH    0x0001  /* Hardware type: Ethernet  */
#define ARP_PROTO_IP    0x0800  /* Protocol type: IP        */
#define ARP_HA_LEN      0x06    /* Hardware Address length  */
#define ARP_PA_LEN      0x04    /* Protocol Address length  */
#define ARP_OPC_REQ     0x0001  /* ARP Request  : 1         */
#define ARP_OPC_REP     0x0002  /* ARP Reply    : 2         */
#define ARP_OPC_RREQ    0x0003  /* RARP Request : 3         */
#define ARP_OPC_RREP    0x0004  /* RARP Reply   : 4         */

typedef struct t_arp_hdr {
    UH hw;              /* Hardware Address Space  */
    UH proto;           /* Protocol Address Space  */
    UB ha_len;          /* Hardware Address Length */
    UB pa_len;          /* Protocol Address Length */
    UH op;              /* Opcode: Request | Reply */
    UB sha[6];          /* Sender Hardware Address */
    UB spa[4];          /* Sender Protocol Address */
    UB tha[6];          /* Target Hardware Address */
    UB tpa[4];          /* Target Protocol Address */
}T_ARP_HDR;
#define ARP_HDR_SZ      28

/**************************************************************************
    IP4
**************************************************************************/

#define IP_HDR_VER4     0x04
#define IP_VER          0x0F
#define IP_OFFSET       0x1FFF
#define IP_FLG_MF       0x01
#define IP_FLG_DF       0x02

#define IP_PROTO_ICMP        1       /* ICMP Protocol        */
#define IP_PROTO_IGMP        2       /* IGMP Protocol        */
#define IP_PROTO_TCP         6       /* TCP  Protocol        */
#define IP_PROTO_UDP        17       /* UDP  Protocol        */
#define IP_PROTO_UDPL      136       /* UDP-Lite Protocol    */

typedef struct t_ip4_hdr {
    UB   ver;                   /* Version, Internal Header Length */
    UB   tos;                   /* Type of Service */
    UH   tl;                    /* Total Length */
    UH   id;                    /* Identification */
    UH   fo;                    /* Flags, Fragment Offset */
    UB   ttl;                   /* Time to Live */
    UB   prot;                  /* Protocol Number */
    UH   hc;                    /* Header Checksum */
    UW   sa;                    /* Source IP Address */
    UW   da;                    /* Destination IP Address */
}T_IP4_HDR;
#define IP4_HDR_SZ      20

typedef struct t_ip4_pseudo {
    UW sa;
    UW da;
    UH proto;
    UH len;
}T_IP4_PSEUDO;

/**************************************************************************
    UDP
**************************************************************************/

typedef struct t_udp_hdr {
    UH sp;                  /* Source Port */
    UH dp;                  /* Destination Port */
    UH len;                 /* Data Length */
    UH cs;                  /* Checksum */
} T_UDP_HDR;
#define UDP_HDR_SZ      8

/**************************************************************************
    TCP
**************************************************************************/

#define TCP_UNUSED          0x00
#define TCP_CLOSED          0x01
#define TCP_LISTEN          0x02
#define TCP_SYN_SENT        0x03
#define TCP_SYN_RECEIVED    0x04
#define TCP_ESTABLISHED     0x05
#define TCP_FIN_WAIT1       0x06
#define TCP_FIN_WAIT2       0x07
#define TCP_CLOSE_WAIT      0x08
#define TCP_CLOSING         0x09
#define TCP_LAST_ACK        0x0A
#define TCP_TIME_WAIT       0x0B

#define TCP_FLG_FIN         0x0001
#define TCP_FLG_SYN         0x0002
#define TCP_FLG_RST         0x0004
#define TCP_FLG_PSH         0x0008
#define TCP_FLG_ACK         0x0010
#define TCP_FLG_URG         0x0020

typedef struct t_tcp_hdr {
    UH  sp;     /* Source port */
    UH  dp;     /* Destination port */
    UW  seq;    /* Sequence value */
    UW  ack;    /* Acknowledgment value */
    UH  flag;   /* Data offset 3 and TCP Flags */
    UH  win;    /* TCP window value */
    UH  cs;     /* TCP Checksum */
    UH  up;     /* Urgent pointer */
}T_TCP_HDR;
#define TCP_HDR_SZ      20

/**************************************************************************
    ICMP
**************************************************************************/

/* ICMP Type :- RFC 792 */
#define ICMP_ECHO_REPLY     0   /* Echo Reply               */
#define ICMP_DST_UNREACH    3   /* Destination Unreachable  */
#define ICMP_SRC_QUENC      4   /* Source Quench            */
#define ICMP_REDIRECT       5   /* Redirect                 */
#define ICMP_ECHO_REQUEST   8   /* Echo Request             */
#define ICMP_TIME_EXCEED    11  /* Time Exceeded            */
#define ICMP_PRM_PROB       12  /* Parameter Problem        */

typedef struct t_icmp_hdr {
    UB type;                /* Type */
    UB code;                /* Code */
    UH cs;                  /* Checksum */
    UH id;                  /* Identifier */
    UH seq;                 /* Sequence Number */
}T_ICMP_HDR;
#define ICMP_HDR_SZ     8

/**************************************************************************
    IGMP
**************************************************************************/

#define IGMP_QUERY       0x11   /* Query            */
#define IGMP_REPORT_V1   0x12   /* Version1 Report  */
#define IGMP_REPORT      0x16   /* Version2 Report  */
#define IGMP_LEAVE       0x17   /* Leave            */

typedef struct t_igmp_hdr {
    UB type;    /* Type          */
    UB mrt;     /* Max Resp Time */
    UH cs;      /* Checksum      */
    UW ga;      /* Group Address */
} T_IGMP_HDR;
#define IGMP_HDR_SZ     8


/**************************************************************************
    Misc Controll Flag
**************************************************************************/

#define  PKT_CTL_REP_UNREACH          0x00000001
#define  PKT_CTL_SKIP_IPCS            0x00000002
#define  PKT_CTL_SKIP_TCPCS           0x00000004
#define  PKT_CTL_SKIP_UDPCS           0x00000008

/***************************************************************************
 *  Internal structure Definition
 */

/***************************************************************************
    Network Buffer
***************************************************************************/

#define IP_RCV_BCAST    0x0001
#define IP_RCV_MCAST    0x0002
#define TCP_DAT_BUF     0x0004
#define LOCAL_LOOPBK    0x0008

#define HW_CS_RX_IPH4   0x0010
#define HW_CS_RX_DATA   0x0020
#define HW_CS_TX_IPH4   0x0040
#define HW_CS_TX_DATA   0x0080

#define HW_CS_IPH4_ERR  0x0100
#define HW_CS_DATA_ERR  0x0200

#define PKT_FLG_BCAST   IP_RCV_BCAST
#define PKT_FLG_MCAST   IP_RCV_MCAST
#define PKT_FLG_TCP     TCP_DAT_BUF

typedef struct t_net_buf {
    UW  *next;
    ID  mpfid;
    struct t_net  *net;
    struct t_net_dev  *dev;
    struct t_net_soc  *soc;
    ER  ercd;       /* Socket Error                 */
    UH  flg;        /* Broadcast/Multicast          */
    UH  seq;        /* IP Fragment Sequence         */
    UH  hdr_len;    /* *hdr length                  */
    UH  dat_len;    /* *dat length                  */
    UB  *hdr;       /* 2byte Aligned                */
    UB  *dat;       /* 4byte Aligned                */
    UB  buf[2];     /* Packet Data                  */
}T_NET_BUF;

#define NET_BUF_MNG_SZ 42

#define NET_LOG(x)
#define NET_ERR(x)

/***************************************************************************
    Network Device
***************************************************************************/

/* Device Type */
#define NET_DEV_TYPE_LOOP   0   /* LoopBack Device  */
#define NET_DEV_TYPE_ETH    1   /* Ethernet Device  */
#define NET_DEV_TYPE_PPP    2   /* PPP              */
#define NET_DEV_TYPE_PPPOE  3   /* PPPoE            */

/* Device Status */
#define NET_DEV_STS_NON     0    /* Device not exists       */
#define NET_DEV_STS_INI     1    /* Device Initializized    */
#define NET_DEV_STS_DIS     2    /* Device Disabled         */

/* Device Flag */
#define NET_DEV_FLG_PROMISC 1    /* Device Promiscous        */
#define NET_DEV_FLG_BCAST   2    /* Device support Broadcast */
#define NET_DEV_FLG_MCAST   4    /* Device support Multicast */

/* Device Configuration */
enum net_dev_ctl {
NET_DEV_CFG_MAC,                 /* MAC Address configuration */
NET_DEV_CFG_MTU,                 /* MTU                       */
NET_DEV_CFG_PROMISC,             /* Enable/Disable Promiscous */
NET_DEV_CFG_BCAST,               /* Enable/Disable MultiCast  */
NET_DEV_CFG_MCAST                /* Enable/Disable BroadCast  */
};

#define DEV_STR_LEN     8       /* Device Name Length */
#define ETH_ADR_LEN     6

typedef struct t_net_dev {
    UB name[DEV_STR_LEN];       /* Device Name                  */
    UH num;                     /* Device Number                */
    UH type;                    /* Device Type                  */
    UH sts;                     /* Device Status                */
    UH flg;                     /* Dummy                        */
    ER (*ini)(UH);              /* Initialize the device        */
    ER (*cls)(UH);              /* Uninitialize the device      */
    ER (*ctl)(UH,UH,VP);        /* Configure the device         */
    ER (*ref)(UH,UH,VP);        /* Read status of the device    */
    ER (*out)(UH,T_NET_BUF*);   /* Write frame to the device    */
    void (*cbk)(UH,UH,VP);      /* Device event notifier        */
    UW *tag;                    /* Device specific              */
    union  {                    /* Address                      */
    struct {
    UB mac[ETH_ADR_LEN];
    }eth;
    }cfg;
    UH  hhdrsz;                 /* Device header length         */
    UH  hhdrofs;                /* Device header offset         */
}T_NET_DEV;

/***************************************************************************
    Network Configuration
***************************************************************************/

typedef struct t_net_cfg {
    UW  use;
    /* Interface    */
    UW  PATH_MTU;

    /* ARP          */
    UW  ARP_RET_CNT;
    UW  ARP_RET_TMO;
    UW  ARP_CLR_TMO;
#ifdef ACD_SUP
    UW  ARP_PRB_WAI;
    UW  ARP_PRB_NUM;
    UW  ARP_PRB_MIN;
    UW  ARP_PRB_MAX;
    UW  ARP_ANC_WAI;
    UW  ARP_ANC_NUM;
    UW  ARP_ANC_INT;
#endif

    /* IP           */
    UW  IP4_TTL;
    UW  IP4_TOS;
    UW  IP4_IPR_TMO;    /*ip4_ipr_tmo, ip4_ipr_max*/

    /* IGMP         */
    UW  IP4_MCAST_TTL;
    UW  IGMP_V1_TMO;
    UW  IGMP_REP_TMO;

    /* TCP          */
    UW  TCP_MSS;
#ifdef IPV6_SUP
    UW  TCP_MSS_IPV6;
#endif

    UW  TCP_RTO_INI;
    UW  TCP_RTO_MIN;
    UW  TCP_RTO_MAX;

    UW  TCP_SND_WND;
    UW  TCP_RCV_WND;

    UW  TCP_DUP_CNT;

    UW  TCP_CON_TMO;
    UW  TCP_SND_TMO;
    UW  TCP_CLS_TMO;
    UW  TCP_CLW_TMO;
    UW  TCP_ACK_TMO;

#ifdef KEEPALIVE_SUP
    UW  TCP_KPA_CNT;
    UW  TCP_KPA_INT;
    UW  TCP_KPA_TMO;
#endif

    /* Datagram(UDP&ICMP) */
    UW  PKT_RCV_QUE;

    /* Misc Controll Flag */
    UW  PKT_CTL_FLG;

}T_NET_CFG;

/***************************************************************************
    Network Address
***************************************************************************/

#define NET_ADR_MOD_NONE       0
#define NET_ADR_MOD_STATIC     1
#define NET_ADR_MOD_DHCP       2
#define NET_ADR_MOD_AUTO       3

#define ARP_RESOLVE_IP         0
#define ARP_PROBE_IP           1
#define ARP_ANNOUNCE_IP        2
#define ARP_GRATUITOUS         3

#ifdef ACD_SUP
typedef struct t_net_acd {
    UW ip;
    UB mac[6];
}T_NET_ACD;

typedef ER (*ACD_HND)(T_NET_ACD*);
#endif /* ACD_SUP */

typedef struct t_net_adr {
    UB ver;         /* IP4/IPv6         */
    UB mode;        /* STATIC or DHCP   */
    UW ipaddr;      /* Default IP Addr  */
    UW gateway;     /* Default Gateway  */
    UW mask;        /* Subnet Mask      */
}T_NET_ADR;

/***************************************************************************
    Network
***************************************************************************/
extern UB IPR_TIMER_ON;
extern UB IGMP_TIMER_ON;
extern UB IGMPV1_TIMER_ON;
#ifdef IPV6_SUP
extern UB IPV6_TIMER_ON;
#endif

typedef struct t_net {
    struct t_net *next; /* Next interface in the list */
    ER (*out)(T_NET_BUF*);
    T_NET_DEV    *dev;  /* Net Device           */
    T_NET_ADR    *adr;  /* Host Address         */
#ifdef IPV6_SUP
    T_NET6_ADR   *ip6adr;/* Host Address         */
#endif
    T_NET_CFG    *cfg;  /* Net Configuration    */
    UH flag;            /* BCast/MCast          */
    UH ident;           /* IP Identification    */
    UW igmpv1_tmo;
    UB igmpv1_router;   /* IGMPV1 Router Present */
    UB igmp_exists;
#ifdef ACD_SUP
    ID prbtskid;        /* IP addr probing task    */
    T_NET_ACD *acd;     /* IP addr probing result  */
    ACD_HND   acdcbk;   /* Callback for detecting address conflict*/
#endif
}T_NET;

/***************************************************************************
    Network
***************************************************************************/

#define ARP_UNUSED      0x00   /* Entry is Empty                */
#define ARP_RESOLVING   0x01   /* Resolving Remote IP Address   */
#define ARP_VALID       0x02   /* Valid IP & MAC values         */
#define ARP_STATIC      0x04   /* Static ARP entry              */

typedef struct t_net_arp {
    UW ipaddr;      /* Resolved or Resolving IP address         */
    UB mac[6];      /* Valid MAC when ARP status is ARP_VALID   */
    UB sts;         /* Unused->Resolving->Valid->Unused         */
    UB cnt;         /* Retry Count (Resolving)                  */
    UW tmo;         /* Retry (Resolving)/Cache (Valid)          */
    T_NET *net;     /* Network Interface                        */
    UW *ipq;        /* Queue IP packet during Resolving IP      */
}T_NET_ARP;

/***************************************************************************
    Network
***************************************************************************/

typedef struct t_net_ipr {
    UW *ipq;        /* Fragment packet  Queue   */
    UW *top;        /* First Fragment packet    */
    UW sa;          /* Hash value               */
    UW da;          /* Hash value               */
#ifdef IPV6_SUP
    UW ip6sa[4];
    UW ip6da[4];
#endif
    UH id;          /* Hash value               */
    UB prot;        /* Hash value               */
    UB flg;         /* First&Last Frg received  */
    UH tlen;        /* Required Length          */
    UH rlen;        /* Received Length          */
    UH cnt;         /* No. of Fragments Received*/
    UH iphdrlen;    /* First Fragment IPhdr len */
    UW tmo;         /* Reassembly Timeout       */
}T_NET_IPR;

/***************************************************************************
    Network
***************************************************************************/

/* IGMP Group State */
#define IGMP_NON_MEMBER     0
#define IGMP_DLY_MEMBER     1
#define IGMP_IDLE_MEMBER    2

/* IGMP Version 1 Router Present Time out */
#define IGMPV1_PRE_TMO  400000  /* 400 Seconds */
#define IGMP_REPORT_TMO 10000   /* 10 Seconds  */

#define IGMP_MSG_LEN        0x1C    /* 28 IP4HDR + IGMP + RouterAlertOption*/
#define IGMP_ALL_SYSTEMS    0xE0000001   /* 224.0.0.1 */
#define IGMP_ALL_ROUTERS    0xE0000002   /* 224.0.0.2 */

typedef struct t_net_mgr {
    UW ga;          /* Multicast Group Address  */
    UW tmo;         /* Report Time out          */
    UB cnt;         /* Report Retry count       */
    UB flg;         /* More than one host exist */
    UB state;       /* Group State              */
    UB ref;         /* Socket Count             */
    T_NET *net;     /* Network Interface        */
}T_NET_MGR;

/***************************************************************************
    Network
***************************************************************************/

#define SEQ_LT(a,b)     ((int)((UW)(a)-(UW)(b)) < 0)
#define SEQ_LEQ(a,b)    ((int)((UW)(a)-(UW)(b)) <= 0)
#define SEQ_GT(a,b)     ((int)((UW)(a)-(UW)(b)) > 0)
#define SEQ_GEQ(a,b)    ((int)((UW)(a)-(UW)(b)) >= 0)

#define TCP_SET_RET     0x0001  /* Retry Timer      */
#define TCP_SET_ACK     0x0002  /* Delay ACK Timer  */
#define TCP_SET_PRB     0x0004  /* Delay ACK Timer  */
#ifdef KEEPALIVE_SUP
#define TCP_SET_KPA     0x0008
#endif
#define TCP_SET_CON     0x0010
#define TCP_SET_CLS     0x0020
#define TCP_SET_DAT     0x0040

#define TCP_TIM_RET     0x0001
#define TCP_TIM_USR     0x0011
#define TCP_TIM_SND     0x0021
#define TCP_TIM_CLS     0x0041
#define TCP_TIM_IDE     0x0081
#define TCP_TIM_PRB     0x0101
#define TCP_TIM_ACK     0x1000   /* Delay ACK */

/* Con, ret, dack, persist, keepalive, finwait2, timewait */

typedef struct t_net_tcp {
    struct t_net_soc *soc;
    /* TCP Connection control */
    UB bsd;
    UB dup_cnt;
    UH rtt;
    UH sflg;
    UH tim_flg;
    UW tcp_tmo;
#ifdef KEEPALIVE_SUP
    UW kpa_cnt;
    UW kpa_tmo;
#endif

    /* TCP Receive Control  */
    UW *rdatque;
    UW rbufsz;      /* Rcv Buffer size */
    UW rdatsz;      /* Len of data available in Rcv buffer */
    UW irs;
    UW rcv_wnd;
    UW rcv_nxt;
    UW rcv_up;
    UW ack_tmo;     /* Delay ACK Timer */
    UW mss;

    /* TCP Transmit Control */
    UB *snd_buf;
    UW sputp;
    UW sbufsz;
    UW snd_len;
    UW iss;
    UW rcv_mss;
    UW snd_wnd;
    UW snd_una;
    UW snd_nxt;
    UW snd_max;
    UW snd_wl1;
    UW snd_wl2;
    UW snd_up;
    UW ret_tmo;     /* Retry Timer */

    /* TCP Flow Control */
    UW cwnd;
    UW ssth;
    UW recover;

    /* RTO */
    UW rto;
    UW srtt;
    UW rttvar;
    UW rtt_seq;
}T_NET_TCP;
/*35*/

#define TIM_LEQ     SEQ_LEQ

#define IS_LBACK_IP(i)      ((((UW)(i)) & 0xFF000000) == 0x7F000000)
#define IS_MCAST_IP(i)      ((((UW)(i)) & 0xF0000000) == 0xE0000000)
#define IS_RES_BCAST_IP(i)  ((UW)(i) == 0xFFFFFFFF)
#define IS_BCAST_IP(i,a,m)  (((UW)(i) == 0xFFFFFFFF) || \
                             ((UW)(i)== ((UW)(a) | ~(UW)(m))))
#define IS_ROUTE_IP(l,i,m)  ((((UW)(l))&((UW)(m))) == (((UW)(i))&((UW)(m))))

/***************************************************************************
 *  Socket
 */

typedef struct t_node {
    UH  port;   /* Port number 1 - 65535, 0 -> PORT any */
    UB  ver;    /* IP Address type */
    UB  num;    /* Device number   */
    UW  ipa;    /* IPv4 Address    */
#ifdef IPV6_SUP
    UW  ip6a[4];/* IPv6 Address    */
#endif
}T_NODE;

typedef struct t_rcv_pkt_inf {
   UW   src_ipa;    /* Source IP Address */
   UW   dst_ipa;    /* Destination IP Address */
#ifdef IPV6_SUP
    UW  ip6sa[4];   /* IPv6 Source Address     */
    UW  ip6da[4];   /* IPv6 Destination Address    */
#endif
   UH   src_port;   /* Source Port */
   UH   dst_port;   /* Destination Port */
   UB   ttl;        /* IP TTL */
   UB   tos;        /* IP TOS */
   UB   ver;        /* IP Version */
   UB   num;        /* Received Device Number */
}T_RCV_PKT_INF;

/* Socket callback  prototype */

typedef void(*SOC_HND)(UH,UH,ER);

#define EV_SOC_CON      0x0001
#define EV_SOC_CLS      0x0002
#define EV_SOC_SND      0x0004
#define EV_SOC_RCV      0x0008
#define EV_SOC_SRY      0x0010
#define EV_SOC_RRY      0x0020
#ifdef IO_READY_SUP
#define EV_RDY_SND      0x0100
#define EV_RDY_RCV      0x0200
#endif
#define EV_SOC_ALL      0xFFFF

#define SOC_FLG_SER     0x8000  /* Passive Connection */
#define SOC_FLG_PRT     0x4000  /* Port Any           */

typedef struct t_net_soc {
    T_NET_TCP *tcp; /* TCP control Block */
    T_NET *net;     /* I/F to Send/Recv  */
    T_NET *usr_net;
    UH sid;         /* 0 to max */
    UH state;       /* CREATED/CONNECTED */
    UH lport;
    UH rport;
    UW raddr;
#ifdef IPV6_SUP
    UW  laddr6[4];
    UW  raddr6[4];
#endif
    UB rqsz;
    UB ver;         /* IP Version */
    UB proto;       /* TCP/UDP  */
    UB fncd;
    UB tos;
    UB ttl;
    UH flg;
    ER slen;
    ER rlen;
    ER ercd;         /* socket error */
    UW  *sdatque;
    UW  *rdatque;
    void (*cbk)(UH,UH,ER);
    UH cptn;
    UH wptn;
    ID ctskid;      /* Con/Cls Task */
    ID rtskid;      /* Snd Task     */
    ID stskid;      /* Rcv Task     */
    TMO snd_tmo;     /* User timeout for send process            */
    TMO rcv_tmo;     /* User timeout for receive process         */
    TMO con_tmo;     /* User timeout for connect process         */
    TMO cls_tmo;     /* User timeout for close process           */
    T_RCV_PKT_INF  rpi;    /* Received packet information       */
#ifdef MCAST_SOC_SUP
    UW mgr_idx;
#endif
}T_NET_SOC;

#define SOC_UNUSED          TCP_UNUSED
#define SOC_CREATED         TCP_CLOSED
#define SOC_CONNECTED       TCP_ESTABLISHED

#define INADDR_ANY          ((UW)0U)     /* Auto IP ADDRESS */
#define PORT_ANY            ((UH)0U)     /* Auto PORT       */
#define DEV_ANY             ((UH)0U)
/* Supported IP VERSION */

#define IP_VER4             0       /* IPv4 */
#define IP_VER6             1       /* IPv6 */

/* SOC_CON */

#define SOC_SER         0           /* Wait for connection      */
#define SOC_CLI         1           /* Establish connection     */
#define SOC_TCP_ACP     SOC_SER     /* TCP ACTIVE connection    */
#define SOC_TCP_CON     SOC_CLI     /* TCP PASSIVE connection   */
#define SOC_UDP_SND     SOC_SER     /* Use Receive IP           */
#define SOC_UDP_RCV     SOC_CLI     /* Set Remote IP            */

/* SOC_CLS */

#define SOC_TCP_CLS         0       /* CLOSE Connection */
#define SOC_TCP_SHT         1       /* CLOSE Transmission */

/* SOC_ABT */

#define SOC_ABT_ALL         0       /* ABORT All Activities             */
#define SOC_ABT_SND         1       /* ABORT Send Process               */
#define SOC_ABT_RCV         2       /* ABORT Recv Process               */
#define SOC_ABT_CON         3       /* ABORT Connection Process         */
#define SOC_ABT_CLS         4       /* ABORT Connection closing process */

/* SOC_CFG */
/* SOC_REF */
                                    /* Socket configuration             */
#define SOC_IP_TTL          0
#define SOC_IP_TOS          1
#define SOC_IP_MTU          2
#define SOC_TCP_MTU         3
#define SOC_MCAST_TTL       4
#define SOC_MCAST_LOOP      5
#define SOC_MCAST_JOIN      6
#define SOC_MCAST_DROP      7
#define SOC_BCAST_RECV      8
#define SOC_TMO_SND         9
#define SOC_TMO_RCV         10
#define SOC_TMO_CON         11
#define SOC_TMO_CLS         12
#define SOC_IP_LOCAL        13
#define SOC_IP_REMOTE       14
#define SOC_CBK_HND         15
#define SOC_CBK_FLG         16
#define SOC_TCP_STATE       17
#define SOC_RCV_PKT_INF     18
#define SOC_PRT_LOCAL       19

#define NET_IP4_CFG         0
#define NET_IP4_TTL         1
#define NET_BCAST_RCV       2
#define NET_MCAST_JOIN      3
#define NET_MCAST_DROP      4
#define NET_MCAST_TTL       5
#ifdef ACD_SUP
#define NET_ACD_CBK         6
#endif


/* Well know ports          0 -1023     */
/* Registered ports         1024-49151  */
/* Dynamic/Private ports    49152-65535 */
#define EPHEMERAL_PORT      49152

/* Inc ISS for each second */
#define TCP_ISS_INCR        (125*1024L)

/***************************************************************************
 *  Internal
 */
extern UW NET_TICK;

#define EV_LINK     -97
#define EV_ADDR     -98

ER eth_pkt_out(T_NET_BUF *pkt);
ER ppp_pkt_out(T_NET_BUF *pkt);

void arp_timer(UW ctick);
ER arp_resolve(T_NET_BUF *pkt);
void arp_recv(T_NET_BUF *pkt);
ER arp_send(T_NET *net, UW tpa, UB type);

void ip4_rcv(T_NET_BUF *pkt);
void icmp_pkt_rcv(T_NET_BUF *pkt);
#ifdef PING_SUP
ER icmp_rcv(T_NET_SOC *soc, VP data, UH len);
ER icmp_snd(T_NET_SOC *soc, VP data, UH len);
ER icmp_error(T_NET_BUF *pkt);
#endif

#ifdef IPV6_SUP
void ip6_rcv(T_NET_BUF *nbuf);
ER ip6_snd(T_NET_BUF *pkt);
ER icmp6_pkt_snd(T_NET_BUF *pkt, UH dlen);
void icmp6_err_snd(T_NET_BUF *pkt, UB type, UB code, UW offset);
#endif

#ifdef IPR_SUP
void ipr_init(void);
void ipr_timer(UW ctimval);
ER ip4_reassembly(T_NET_BUF **pkt, T_IP4_HDR *ip4hdr);
ER ipf_snd(T_NET_BUF *pkt);
#endif

#ifdef MCAST_SUP
void igmp_init(void);
void igmp_timer(UW ctimval);
void igmpv1_timer(UW ctimval);
UB is_mgroup_in(T_NET *net, UW mcast);
B mgroup_index(T_NET *net, UW mcast);
ER igmp_join(T_NET *net, UW ga);
ER igmp_leave(T_NET *net, UW ga);

void igmp_rcv(T_NET_BUF *pkt);
void igmp_snd(T_NET *net, UW ga, UB type);
#endif

UW net_csum(UH *dat, UH len, UW c);
UH ip4_csum(T_NET_BUF *pkt);
UH icmp_csum(T_NET_BUF *pkt);
UH tcp_csum(T_NET_BUF *pkt);
#define udp_csum(x) tcp_csum(x)
#define igmp_csum(x) icmp_csum(x)

#ifdef UDP_SUP
void udp_pkt_rcv(T_NET_BUF *pkt);
ER udp_rcv(T_NET_SOC *soc, VP data, UH len);
ER udp_snd(T_NET_SOC *soc, VP data, UH len);
#endif

#ifdef TCP_SUP
void tcp_usr_timer(void);
void tcp_ack_timer(void);
void tcp_dat_timer(void);

void tcp_out(T_NET_TCP *tcp, UH flag);
ER tcp_ack(T_NET_TCP *tcp, UB flag);
ER tcp_out_rst(T_NET_BUF *rpkt, UH len);

void tcp_pkt_rcv(T_NET_BUF *pkt);
UH tcp_wnd_adv(T_NET_TCP *tcp);
void tcp_wnd_upd(T_NET_TCP *tcp, UH len);

ER tcp_cre(T_NET_SOC *soc);
ER tcp_del(T_NET_SOC *soc);
ER tcp_con(T_NET_SOC *soc, T_NODE *host, UB con_flg);
ER tcp_cls(T_NET_SOC *soc, UB cls_flg);
ER tcp_abt(T_NET_TCP *tcp, UB code);
ER tcp_snd(T_NET_SOC *soc, VP data, UH len);
ER tcp_rcv(T_NET_SOC *soc, VP data, UH len);
#endif

void soc_ini(void);
void soc_wakeup(T_NET_BUF *pkt);
UH get_eport(UB proto);
ER soc_event(T_NET_SOC *soc, UH ptn, ER ercd);

/***************************************************************************
 *  Configuration
 */

#ifdef DEF_CFG

#define NET_DEV_MAX     DEF_NET_DEV_MAX
#define NET_SOC_MAX     DEF_NET_SOC_MAX
#define NET_TCP_MAX     DEF_NET_TCP_MAX
#define NET_ARP_MAX     DEF_NET_ARP_MAX
#define NET_MGR_MAX     DEF_NET_MGR_MAX
#define NET_IPR_MAX     DEF_NET_IPR_MAX
#define NET_BUF_CNT     DEF_NET_BUF_CNT
#define NET_BUF_SZ      DEF_NET_BUF_SZ
#define PATH_MTU        DEF_PATH_MTU
#define ARP_RET_CNT     DEF_ARP_RET_CNT
#define ARP_RET_TMO     DEF_ARP_RET_TMO
#define ARP_CLR_TMO     DEF_ARP_CLR_TMO
#ifdef ACD_SUP
#define ARP_PRB_WAI     DEF_ARP_PRB_WAI
#define ARP_PRB_NUM     DEF_ARP_PRB_NUM
#define ARP_PRB_MIN     DEF_ARP_PRB_MIN
#define ARP_PRB_MAX     DEF_ARP_PRB_MAX
#define ARP_ANC_WAI     DEF_ARP_ANC_WAI
#define ARP_ANC_NUM     DEF_ARP_ANC_NUM
#define ARP_ANC_INT     DEF_ARP_ANC_INT
#endif
#define IP4_TTL         DEF_IP4_TTL
#define IP4_TOS         DEF_IP4_TOS
#define IP4_IPR_TMO     DEF_IP4_IPR_TMO
#define IP4_MCAST_TTL   DEF_IP4_MCAST_TTL
#define IGMP_V1_TMO     DEF_IGMP_V1_TMO
#define IGMP_REP_TMO    DEF_IGMP_REP_TMO
#define TCP_MSS         DEF_TCP_MSS
#ifdef IPV6_SUP
#define TCP_MSS_IPV6    DEF_TCP_MSS_IPV6
#endif
#define TCP_RTO_INI     DEF_TCP_RTO_INI
#define TCP_RTO_MIN     DEF_TCP_RTO_MIN
#define TCP_RTO_MAX     DEF_TCP_RTO_MAX
#define TCP_SND_WND     DEF_TCP_SND_WND
#define TCP_RCV_WND     DEF_TCP_RCV_WND
#define TCP_SND_WND_MAX DEF_TCP_SND_WND
#define TCP_DUP_CNT     DEF_TCP_DUP_CNT
#define TCP_CON_TMO     DEF_TCP_CON_TMO
#define TCP_SND_TMO     DEF_TCP_SND_TMO
#define TCP_CLS_TMO     DEF_TCP_CLS_TMO
#define TCP_CLW_TMO     DEF_TCP_CLW_TMO
#define TCP_ACK_TMO     DEF_TCP_ACK_TMO
#ifdef KEEPALIVE_SUP
#define TCP_KPA_CNT     DEF_TCP_KPA_CNT
#define TCP_KPA_INT     DEF_TCP_KPA_INT
#define TCP_KPA_TMO     DEF_TCP_KPA_TMO
#endif
#define PKT_RCV_QUE     DEF_PKT_RCV_QUE

#else

extern UW NET_DEV_MAX;
extern UW NET_SOC_MAX;
extern UW NET_TCP_MAX;
extern UW NET_ARP_MAX;
extern UW NET_MGR_MAX;
extern UW NET_IPR_MAX;
extern UW NET_BUF_CNT;
extern UW NET_BUF_SZ;
extern UW TCP_SND_WND_MAX;

#define PATH_MTU        net->cfg->PATH_MTU
#define ARP_RET_CNT     net->cfg->ARP_RET_CNT
#define ARP_RET_TMO     net->cfg->ARP_RET_TMO
#define ARP_CLR_TMO     net->cfg->ARP_CLR_TMO
#ifdef ACD_SUP
#define ARP_PRB_WAI     net->cfg->ARP_PRB_WAI
#define ARP_PRB_NUM     net->cfg->ARP_PRB_NUM
#define ARP_PRB_MIN     net->cfg->ARP_PRB_MIN
#define ARP_PRB_MAX     net->cfg->ARP_PRB_MAX
#define ARP_ANC_WAI     net->cfg->ARP_ANC_WAI
#define ARP_ANC_NUM     net->cfg->ARP_ANC_NUM
#define ARP_ANC_INT     net->cfg->ARP_ANC_INT
#endif
#define IP4_TTL         net->cfg->IP4_TTL
#define IP4_TOS         net->cfg->IP4_TOS
#define IP4_IPR_TMO     net->cfg->IP4_IPR_TMO
#define IP4_MCAST_TTL   net->cfg->IP4_MCAST_TTL
#define IGMP_V1_TMO     net->cfg->IGMP_V1_TMO
#define IGMP_REP_TMO    net->cfg->IGMP_REP_TMO
#define TCP_MSS         net->cfg->TCP_MSS
#ifdef IPV6_SUP
#define TCP_MSS_IPV6    net->cfg->TCP_MSS_IPV6
#endif
#define TCP_RTO_INI     net->cfg->TCP_RTO_INI
#define TCP_RTO_MIN     net->cfg->TCP_RTO_MIN
#define TCP_RTO_MAX     net->cfg->TCP_RTO_MAX
#define TCP_SND_WND     net->cfg->TCP_SND_WND
#define TCP_RCV_WND     net->cfg->TCP_RCV_WND
#define TCP_DUP_CNT     net->cfg->TCP_DUP_CNT
#define TCP_CON_TMO     net->cfg->TCP_CON_TMO
#define TCP_SND_TMO     net->cfg->TCP_SND_TMO
#define TCP_CLS_TMO     net->cfg->TCP_CLS_TMO
#define TCP_CLW_TMO     net->cfg->TCP_CLW_TMO
#define TCP_ACK_TMO     net->cfg->TCP_ACK_TMO
#ifdef KEEPALIVE_SUP
#define TCP_KPA_CNT     net->cfg->TCP_KPA_CNT
#define TCP_KPA_INT     net->cfg->TCP_KPA_INT
#define TCP_KPA_TMO     net->cfg->TCP_KPA_TMO
#endif
#define PKT_RCV_QUE     net->cfg->PKT_RCV_QUE

#define REP_UNREACH    (net->cfg->PKT_CTL_FLG & PKT_CTL_REP_UNREACH)
#define SKIP_IPCS      (net->cfg->PKT_CTL_FLG & PKT_CTL_SKIP_IPCS)
#define SKIP_TCPCS     (net->cfg->PKT_CTL_FLG & PKT_CTL_SKIP_TCPCS)
#define SKIP_UDPCS     (net->cfg->PKT_CTL_FLG & PKT_CTL_SKIP_UDPCS)

#endif  /* DEF_CFG */

extern T_NET       gNET[];
extern T_NET_ADR   gNET_ADR[];
extern T_NET_DEV   gNET_DEV[];
extern T_NET_ARP   gNET_ARP[];
extern T_NET_CFG   gNET_CFG[];
#ifndef EXT_ALOC_SUP
extern const T_CMPF c_net_mpf;
#endif
extern const T_CTSK c_net_tsk;
extern const T_CSEM c_net_sem;

/***************************************************************************
 *  API
 */

/* TCP/IP Stack */

ER net_ini(void);
ER net_cfg(UH dev_num, UH opt, VP val);
ER net_ref(UH dev_num, UH opt, VP val);
ER net_ext(void);

/* Network Buffer */

ER net_buf_ini(void);
ER net_buf_get(T_NET_BUF **buf, UH len, TMO tmo);
void net_buf_ret(T_NET_BUF *buf);

/* Network Device */

ER net_dev_ini(UH dev_num);
ER net_dev_cls(UH dev_num);
ER net_dev_sts(UH dev_num, UH opt, VP val);
ER net_dev_ctl(UH dev_num, UH opt, VP val);
void net_pkt_rcv(T_NET_BUF *pkt);

T_NET *get_net_bynum(UH num);
T_NET *get_net_default(void);

/* Socket */
#ifndef NET_C
ER cre_soc(UB proto, T_NODE *host);
ER del_soc(UH sid);
#endif
ER con_soc(UH sid, T_NODE *host, UB con_flg);
ER snd_soc(UH sid, VP data, UH len);
ER rcv_soc(UH sid, VP data, UH len);
ER cls_soc(UH sid, UB cls_flg);
ER cfg_soc(ID sid, UB code, VP val);
ER ref_soc(ID sid, UB code, VP val);
ER abt_soc(UH sid, UB code);

#ifdef IO_READY_SUP
ER snd_rdy(ID sid);
ER rcv_rdy(ID sid);
#endif

#ifdef LO_IF_SUP
typedef ER(*LO_PKT_HDL)(UH,  T_NET_BUF*);
#endif

#ifndef NET_C
#define soc_cre cre_soc
#define soc_del del_soc
#endif
#define soc_con con_soc
#define soc_snd snd_soc
#define soc_rcv rcv_soc
#define soc_cls cls_soc
#define soc_cfg cfg_soc
#define soc_ref ref_soc
#define soc_abt abt_soc

/* Misc */

UW ip_aton(const char *str);
void ip_ntoa(const char *s, UW ipaddr);
UW ip_byte2n(char *s);
void ip_n2byte(char *s, UW ip);

void net_rand_seed(UW seed);
UW net_rand(void);

/* external API */
extern VP net_memset(VP d, int c, UINT n);
extern VP net_memcpy(VP d, VP s, UINT n);
extern int net_memcmp(VP d, VP s, UINT n);
extern ER net_memini(void);
extern ER net_memext(void);
extern ER net_memget(VP *adr, UINT n, TMO tmo, ID *id);
extern ER net_memret(VP adr, ID id);

/* other */
void loc_tcp(void);
void ulc_tcp(void);

void arp_ini(void);
void arp_rmv_que(T_NET_BUF *pkt, ER err);
#ifdef ARP_API_SUP
ER arp_static(T_NET *net, UW ip, UB *mac);
ER arp_ref(T_NET *net, UW ip, UB *mac);
#endif

void net_tim_tsk(VP_INT exinf);
void add2que_end(UW **que, T_NET_BUF *pkt);
void rmv4que_top(UW **que);

#ifdef ACD_SUP
ER net_acd(UH dev_num, T_NET_ACD *acd);
#endif

#ifdef NET_C
typedef struct t_soc_table {
    UH  sid;        /* Socket ID,   0 - Unused, 1 to NET_SOC_MAX + 1  */
    UB  proto;      /* IP Protocol, 0 - Unused, 17 for TCP, 6 for UDP */
    UH  port;       /* Local Port,  0 - Default, 1 to 65535 */
    TMO snd_tmo;
    TMO rcv_tmo;
    TMO con_tmo;
    TMO cls_tmo;
    UW  sbufsz;
    UW  rbufsz;
    UH  dev_num;    /* Device Number */
    UB  ver;        /* IP version */
}T_SOC_TABLE;
#endif

#ifdef NET_C
extern T_SOC_TABLE *pSOC_TABLE;
#endif
extern T_NET_SOC   *pNET_SOC;
extern T_NET_TCP   *pNET_TCP;
extern T_NET_MGR   *pNET_MGR;
extern T_NET_IPR   *pNET_IPR;
#ifdef LO_IF_SUP
extern const VP net_inftbl[7];
#else
extern const VP net_inftbl[6];
#endif

extern ID ID_NET_TSK;
extern ID ID_NET_SEM;
extern ID ID_NET_MPF;

#ifdef __cplusplus
}
#endif
#endif /* NETHDR_H */
