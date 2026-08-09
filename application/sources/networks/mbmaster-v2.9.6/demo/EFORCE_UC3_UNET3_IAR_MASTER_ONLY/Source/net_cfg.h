/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    Default configuration parameters
    Copyright (c)  2008-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.19: Created
                         2010.06.02: LITTLE_ENDIAN -> _UC3_ENDIAN_LITTLE
                         2010.10.02: Updated for IPv6 support
                         2011.03.07: Updated for received packet queue
                         2011.04.04: Added definition of TCP/MSS for IPv6
                         2011.05.20: Added configurable parameter
                                    (NET_TCP_MAX, PKT_RCV_QUE, NET_BUF_OFFSET)
                         2011.11.14: Added ACD parameter
                         2012.05.21: Implemented Keep-Alive
                         2012.06.06: Implemented port unreachable with ICMPv4
                         2012.06.11: Update default network buffer size
                         2013.04.10  H/W OS supported version
                         2013.06.24  Update for GNU C compiler
                         2013.07.21  Implement I/O ready event for select API
                         2013.08.06: Implememted multicast per socket
                         2014.01.06: Update for ANDES GCC compiler
 ***************************************************************************/

#ifndef NETCFG_H
#define NETCFG_H
#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/* TCP/IP Default Configuration Values                                    */
/**************************************************************************/

#define DEF_NET_DEV_MAX     1       /* Maximum Interface            */
#define DEF_NET_SOC_MAX     10      /* Maximum Sockets (TCP&UDP)    */
#define DEF_NET_TCP_MAX     5       /* Maximum TCP Control blocks   */
#define DEF_NET_ARP_MAX     8       /* ARP Cache                    */
#define DEF_NET_MGR_MAX     8       /* Multicast Group              */
#define DEF_NET_IPR_MAX     2       /* IP Reassembly                */
#define DEF_NET_BUF_CNT     8
#define DEF_NET_BUF_SZ      1576
#define DEF_NET_BUF_OFFSET  2
#define DEF_PATH_MTU        1500    /* IP Path MTU                  */
#define DEF_ARP_RET_CNT     3           /* 3 Times  */
#define DEF_ARP_RET_TMO     1000        /* 1 sec    */
#define DEF_ARP_CLR_TMO     20*60*1000  /* 20 min   */
#define DEF_ARP_PRB_WAI     1*1000
#define DEF_ARP_PRB_NUM     3
#define DEF_ARP_PRB_MIN     1*1000
#define DEF_ARP_PRB_MAX     2*1000
#define DEF_ARP_ANC_WAI     2*1000
#define DEF_ARP_ANC_NUM     2
#define DEF_ARP_ANC_INT     2*1000
#define DEF_IP4_TTL         64
#define DEF_IP4_TOS         0
#define DEF_IP4_IPR_TMO     10*1000     /* 10 sec   */
#define DEF_IP4_MCAST_TTL   1
#define DEF_IGMP_V1_TMO     400*1000
#define DEF_IGMP_REP_TMO    10*1000
#define DEF_TCP_MSS         (DEF_PATH_MTU-40)   /* TCP MSS */
#define DEF_TCP_RTO_INI     3*1000      /* 3 sec    */
#define DEF_TCP_RTO_MIN     500         /* 500 msec */
#define DEF_TCP_RTO_MAX     60*1000     /* 60 sec   */
#define DEF_TCP_SND_WND     1024
#define DEF_TCP_RCV_WND     1024
#define DEF_TCP_DUP_CNT     4
#define DEF_TCP_CON_TMO     75*1000     /* 75 secs */
#define DEF_TCP_SND_TMO     64*1000     /* 64 secs */
#define DEF_TCP_CLS_TMO     75*1000     /* 75 secs */
#define DEF_TCP_CLW_TMO     20*1000     /* 20 secs */
#define DEF_TCP_ACK_TMO     200
#define DEF_TCP_KPA_CNT     0           /* 0 times(Disable) */
#define DEF_TCP_KPA_INT     1*1000      /* 1 secs */
#define DEF_TCP_KPA_TMO     7200*1000   /* 7200 secs(2 hours) */
#define DEF_PKT_RCV_QUE     1

#define DEF_NET_TSK_PRI     4
#define DEF_NET_TSK_SIZ     1024

#ifdef IPV6_SUP     
#define IP6_MIN_MTU     1280

#define DEF_NET6_BUF_SZ     DEF_NET_BUF_SZ

/* Neighbor Discovery */
#define DEF_NEIGH_CACHE     05
#define DEF_DST_CACHE       05
#define DEF_RTR_LST         02
#define DEF_PRFX_LST        05
#define DEF_PMTU_CACHE      05

#define DEF_DAD_CNT         01
#define DEF_IPV6_TIM_RES    10

#define DEF_UCAST_CNT       3   /* Linklocal, site local and Global */
#define DEF_MCAST_CNT       3   /* All node and Solicit node */

#define DEF_TCP_MSS_IPV6    (DEF_PATH_MTU-60)   /* TCP MSS */

#endif

/**************************************************************************/
/* CPU Architecture dependent Macros                                      */
/**************************************************************************/

#ifdef TKERNEL_PRID     /* Kernel éØï î‘çÜ */
#if ((TKERNEL_PRID & 0x0F00) == 0x0100)
#define NET_C_OS        /* Using Compact Kernel */
#elif ((TKERNEL_PRID & 0x0F00) == 0x0200) 
#define NET_S_OS        /* Using Standard Kernel */
#endif
#else
#define NET_HW_OS       /* Using H/W RTOS */
#endif

/* Define endian, if not already defined else where */
#if !defined(_UC3_ENDIAN_BIG) && !defined(_UC3_ENDIAN_LITTLE)
#if defined (__CC_ARM)     /* for ARM Compiler */
#if !defined (__LITTLE_ENDIAN)
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#if defined (__ICCARM__)  /* for IAR Compiler */
#if (__LITTLE_ENDIAN__ == 1)
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#if defined (__GNUC__)    /* for GCC Compiler */
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#if defined (__TMS470__)  /* for CCS Compiler */
#if defined (__little_endian__)
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#ifdef _SH                /* for SuperH */
#ifdef _LIT
#define _UC3_ENDIAN_LITTLE
#endif
#endif
#if defined(__GCC_NDS32)  /* for ANDES GCC */
#define _UC3_ENDIAN_LITTLE
#endif
#endif

/**************************************************************************/
/* Network Architecture dependent Macros                                  */
/**************************************************************************/

#ifdef NET_C_OS
#define NET_C           /* Using Network Configurator */
#endif

#ifdef NET_C
#define MAC_RESOLVE     /* MAC Resolver */
#ifdef MAC_RESOLVE
#define UDP_MAC_PORT    5000
#endif
#endif

#ifndef UNDEF_TCP
#define TCP_SUP         /* TCP Enabled */
#endif
#ifndef UNDEF_UDP
#define UDP_SUP         /* UDP Enabled */
#endif
#ifndef UNDEF_IPR
#define IPR_SUP         /* IP Reassembly Enabled */
#endif
#ifndef UNDEF_PING
#define PING_SUP        /* ICMP API Enabled */
#endif

/*#define RARP_SUP*/    /* RARP Reply */

#ifdef UDP_SUP
#ifndef UNDEF_MCAST
#define MCAST_SUP       /* IGMP Enabled */
#endif
#endif

#ifdef TCP_SUP
#ifndef UNDEF_KEEPALIVE
#define KEEPALIVE_SUP
#endif
#endif

#ifndef UNDEF_ACD
#define ACD_SUP
#endif


#ifdef POSIX_API_SUP
#define IO_READY_SUP
#define LO_IF_SUP
#ifdef  MCAST_SUP
    #define MCAST_SOC_SUP   /* spec for multicast per socket */
#endif
#define EXT_ALOC_SUP
#endif

#ifdef NET_HW_OS
#ifndef EXT_ALOC_SUP
#define EXT_ALOC_SUP
#endif
#endif

#ifdef _UC3_ENDIAN_LITTLE
UH ntohs(UH val);
UW ntohl(UW val);
#else
#define ntohs(x) (x)
#define ntohl(x) (x)
#endif /* _UC3_ENDIAN_LITTLE */

#define htons(x) ntohs(x)
#define htonl(x) ntohl(x)

#ifdef __cplusplus
}
#endif
#endif /* NETCFG_H */
