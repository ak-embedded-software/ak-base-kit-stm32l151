/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    DNS Resolver header file
    Copyright (c)  2009-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2009.06.03: Created
                         2010.11.02: Support IPv6
 ***************************************************************************/

#ifndef NETDNS_H
#define NETDNS_H
#ifdef __cplusplus
extern "C" {
#endif

#define DNS_RES_TMO         5000 /* DNS Response Timeout */

#define DNS_SERVER_PORT     53  /* Domain Name System */

typedef struct t_dns_hdr {
    UH  id;
    UH  flag;
    UH  qdcount;
    UH  ancount;
    UH  nscount;
    UH  arcount;
}T_DNS_HDR;
#define DNS_HDR_SZ          12

/* 0: */
#define DNS_QR_QUERY        0
#define DNS_QR_RES          1

/* 1-4:4 */
#define DNS_OP_QUERY        0
#define DNS_OP_IQUERY       1
#define DNS_OP_STATUS       2

/* 5: AA*/

/* 27-31:4 */
#define DNS_RCODE_NONE      0
#define DNS_RCODE_FORMAT    1
#define DNS_RCODE_SERVER    2
#define DNS_RCODE_NAME      3
#define DNS_RCODE_NOT_IMP   4
#define DNS_RCODE_REFUSED   5

typedef struct t_dns_rr {
    UH  type;
    UH  class;
    UW  ttl; 
    UH  rdlength;
    UB  rdata[2];   /*variable length*/
}T_DNS_RR;
#define DNS_RR_SZ           10          

/* TYPE */
#define RR_TYPE_A           1   /* host address */
#define RR_TYPE_NS          2   /* an authoritative name server */
#define RR_TYPE_MD          3   /* a mail destination (Obsolete - use MX) */
#define RR_TYPE_MF          4   /* a mail forwarder (Obsolete - use MX) */
#define RR_TYPE_CNAME       5   /* the canonical name for an alias */
#define RR_TYPE_SOA         6   /* marks the start of a zone of authority */
#define RR_TYPE_MB          7   /* a mailbox domain name (EXPERIMENTAL) */
#define RR_TYPE_MG          8   /* a mail group member (EXPERIMENTAL) */
#define RR_TYPE_MR          9   /* a mail rename domain name (EXPERIMENTAL) */
#define RR_TYPE_NULL        10  /* a null RR (EXPERIMENTAL) */
#define RR_TYPE_WKS         11  /* a well known service description */
#define RR_TYPE_PTR         12  /* a domain name pointer */
#define RR_TYPE_HINFO       13  /* host information */
#define RR_TYPE_MINFO       14  /* mailbox or mail list information */
#define RR_TYPE_MX          15  /* mail exchange */
#define RR_TYPE_TXT         16  /* text strings */
#define RR_TYPE_AAAA        28  /* ip6 host address */
/* QTYPE */
#define RR_TYPE_AXFR        252 /* A request for a transfer of an entire zone */
#define RR_TYPE_MAILB       253 /* A request for mailbox-related records (MB, MG or MR) */
#define RR_TYPE_MAILA       254 /* A request for mail agent RRs (Obsolete - see MX) */
#define RR_TYPE_ANY         255 /* A request for all records */

/* CLASS */
#define RR_CLASS_IN         1   /* Internet      */
#define RR_CLASS_CS         2   /* CSNET <obsolete> */
#define RR_CLASS_CH         3   /* CHAOS     */
#define RR_CLASS_HS         4   /* Hesiod    */
/* QCLASS */
#define RR_CLASS_ANY        255 /* Any Class (*) */

/* Misc */
#define LABEL_MAX_LEN       63
#define DNAME_MAX_LEN       255
#define DNS_MSG_MAX_LEN     512

/* API */
ER dns_get_ipaddr(ID socid, UW dns_server, char *name, UW *ipaddr);
ER dns_get_name(ID socid, UW dns_server, char *name, UW *ipaddr);
ER dns_query(UH code, char *name, UW *ipaddr, UW dns_server, ID socid);
#ifdef IPV6_SUP
ER dns_get_ip6addr(ID socid, UW dns_server, char *name, UW *ipaddr);
#endif

#ifdef __cplusplus
}
#endif
#endif /* NETDNS_H */

