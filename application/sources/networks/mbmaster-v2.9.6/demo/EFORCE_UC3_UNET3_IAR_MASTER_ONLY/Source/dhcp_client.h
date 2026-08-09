/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    DHCP Client header file
    Copyright (c)  2008-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.30: Created
 ***************************************************************************/

#ifndef NET_DHCP_H
#define NET_DHCP_H
#ifdef __cplusplus
extern "C" {
#endif

/* DHCP Client State            */

#define DHCP_STS_INITREBOOT     0
#define DHCP_STS_INIT           1
#define DHCP_STS_REBOOTING      2
#define DHCP_STS_REQUESTING     3
#define DHCP_STS_BOUND          4
#define DHCP_STS_SELECTING      5
#define DHCP_STS_REBINDING      6
#define DHCP_STS_RENEWING       7

/*DHCP Messsage*/

typedef struct t_dhcp_msg {
  UB    op;
  UB    htype;
  UB    hlen;
  UB    hops;
  UW    xid;
  UH    secs;
  UH    flags;
  UW    ciaddr;
  UW    yiaddr;
  UW    siaddr;
  UW    giaddr;
  char  chaddr[16];
  char  sname[64];
  char  file[128];
  UB    opt[64];
}T_DHCP_MSG;

#define DHCP_MSG_SZ         300
#define DHCP_MSG_LEN        240 /*Upto magiccookie opt[4]*/

/* DHCP Message Fields      */
#define DHCP_OPC_BOOTREQ    1
#define DHCP_OPC_BOOTREPLY  2
#define DHCP_ETH_TYPE       1   /*Ethernet(10MB) IANA:arp-parameters*/
#define DHCP_ETH_LEN        6
#define DHCP_FLG_BCAST      0x8000

/* DHCP Messages Type (RFC 2132)*/
#define DHCP_MSG_DISCOVER   1
#define DHCP_MSG_OFFER      2
#define DHCP_MSG_REQUEST    3
#define DHCP_MSG_DECLINE    4
#define DHCP_MSG_ACK        5
#define DHCP_MSG_NAK        6
#define DHCP_MSG_RELEASE    7

/* DHCP Options */
#define DHCP_OPT_SUBNET         1
#define DHCP_OPT_ROUTER         3
#define DHCP_OPT_DNS            6
#define DHCP_OPT_REQIPADDR      50  /*:4*/
#define DHCP_OPT_IPLEASE        51  /*:1*/
#define DHCP_OPT_DHCPMSGTYPE    53  /*:1*/
#define DHCP_OPT_SERVERIDENT    54  /*:4*/
#define DHCP_OPT_PRMLST          55  /* Parameter Request List */
#define DHCP_OPT_RENETM          58  /* Renewal Time */
#define DHCP_OPT_REBITM          59  /* Rebinding Time */
#define DHCP_OPT_CLIENT          61  /* Client ID */

/* For Multichannel */
typedef struct t_host_addr {
    UW ipaddr;
    UW subnet;
    UW gateway;
    UW dhcp;
    UW dns[2];
    UW lease;   /* IP Lease time */
    UW t1;      /* Renew  Time   */
    UW t2;      /* Rebind Time   */
    UB mac[6];
    UB dev_num;
    UB state;
    UH socid;
}T_HOST_ADDR;

ER dhcp_client(T_HOST_ADDR *addr);

#ifdef __cplusplus
}
#endif
#endif /* NET_DHCP_H */

