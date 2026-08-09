/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    DNS Resolver
    Copyright (c)  2009-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2009.06.03: Created
                         2010.09.30: Corrected dns_query() return code
                         2010.11.02: Support IPv6
                         2012.03.30: Set to default value dns_transmit() var
                         2012.06.26: Check malformed reception
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.02.19: Delete unnecessary operator in parsing
                         2014.09.02: Discard received packet before issuing query
 ***************************************************************************/

#include <stdlib.h> /* atoi   */
#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"
#include "dns_client.h"

#define DNS_DEBUG(x)

static void SetQueryIp(const char *s, UW ipaddr)
{
  int i, j;
  UB a, b[3];
  char *str;

    str = (char *)s;

    for (i=0;i<4;i++) {
        a = (UB)(ipaddr >> (i*8));
        if (a == 0) {
            *str++ = '0';
        }
        else {
            for (j=0;j<3;j++) {
                b[j] = (a % 10) + '0';
                a /= 10;
                if (a == 0) break;
            }
            for (;j>=0;j--) {
                *str++ = (char)b[j];
            }
        }
        *str++ = '.';
    }
    *--str = '\0';
}

/*
    Convert labels to string 
*/
static ER parse_to_name(char *name, char *label)
{
    UH len, str_len;

    *name  = '\0';
    str_len = 0;

    for (;;) {
        len = *label++;
        if (len > LABEL_MAX_LEN) {
            return E_PAR;
        }
        else if (len == 0) {
            if (str_len) name--;    /* avoid display the root . */
            *name = '\0';
            break;
        }

        str_len += len;
        if (str_len >= DNAME_MAX_LEN) {
            return E_PAR;
        }

        while (len) {
            *name++ = *label++;
            len--;
        }
        *name = '.';
        name++;
    }

    return str_len;
}

/*
    Convert string to labels
*/
static ER parse_to_label(char *name, char *label)
{
    char *str;
    char tok_cnt;
    UH len;

    str = name;

    tok_cnt = 0;
    len     = 0;
    label++;    /* Reserve for label length */

    while (1) {
    
        /* End Of Lable or String? */
        if ((*str == '\0') || (*str == '.')) {

            /* Length of the parsed Label */
            if (tok_cnt) {
                if (tok_cnt > LABEL_MAX_LEN) {
                    return E_PAR;   /* Too long label */
                }
                *(char *)(label - 1 - tok_cnt) = tok_cnt;
                label++;    /* Reserve for label length */
                len++;      /* for label length */
            }

            /* String end with this label */
            if (*str == '\0') {
                break;
            }

            /* Label cannot be zero length */
            if (tok_cnt == 0) {
                return E_PAR;   /* two sequential dots in string? */
            }

            tok_cnt = 0;
        }
        else {
            /* Valid label: AlphaNumeric or - */
            if (!((*str >= '0' && *str <= '9') ||
                  (*str >= 'a' && *str <= 'z') ||
                  (*str >= 'A' && *str <= 'Z') ||
                  (*str == '-'))) {
                return E_PAR;
            }

            *label++     = *str;
            tok_cnt++;
            len++;
            if (len >= DNAME_MAX_LEN) {
                return E_PAR;           /* Too long name */
            }
        }

        str++;
    }

    if (len == 0) {
        return E_PAR;
    }

    *(label-1) = 0;     /* End of label */

    return (len + 1);
}

static ER dns_transmit(char *query, UH len, char *response, UW dns_server, ID socid)
{
    T_NODE host;
    ID sid;
    ER ercd;

#ifndef NET_C
    /* Create UDP Socket */
    host.num  = 0;  /* Default Interface */
    host.ipa  = INADDR_ANY;
    host.port = PORT_ANY;
    host.ver  = IP_VER4;
    sid = cre_soc(IP_PROTO_UDP, &host);
    if (sid <= 0) {
        return E_NOMEM;
    }
#else
    sid = (UH)socid;
#endif

#ifndef NET_C
    /* Set Reception Timeout (5 Sec)    */
    ercd = cfg_soc(sid, SOC_TMO_RCV, (VP)DNS_RES_TMO);
    if (ercd != E_OK) {
        ercd = E_TMOUT;
        goto _dns_tx_err;
    }
#endif

    /* Discard received packet */
    ercd = cls_soc(sid, 0);
    if (ercd != E_OK) {
        ercd = E_OBJ;
        goto _dns_tx_err;
    }

    /* Set DNS Server IP Address */
    host.ipa  = dns_server;
    host.port = DNS_SERVER_PORT;
    host.ver  = IP_VER4;
    host.num  = 0;  /* Default Interface */
    ercd = con_soc(sid, &host, SOC_CLI);
    if (ercd != E_OK) {
        ercd = E_TMOUT;
        goto _dns_tx_err;
    }

    /* Send DNS Query Message */
    ercd = snd_soc(sid, (VP)query, len);
    if (ercd <= 0) {
        ercd = E_TMOUT;
        goto _dns_tx_err;
    }

    /* Wait for DNS Response */
    ercd = rcv_soc(sid, (VP)response, DNS_MSG_MAX_LEN);
    if (ercd <= 0) {
        ercd = E_TMOUT;
        goto _dns_tx_err;
    }

_dns_tx_err:
#ifndef NET_C
    del_soc(sid);
#endif
    return ercd;
}

/*
    DNS Resolver
*/
ER dns_query(UH code, char *name, UW *ipaddr, UW dns_server, ID socid)
{
    T_DNS_HDR *dns_hdr;
    T_DNS_RR  rrd, *rr;
    UB *dns_tx_msg, *dns_rx_msg;
    UB *msg;
    UH msg_len, rcode, tx_id;
    ER ercd;
    UB *ptr, *end;
    T_NET_BUF *pkt[2];

    pkt[0] = pkt[1] = NULL;

    ercd = net_buf_get(&pkt[0], DNS_MSG_MAX_LEN, TMO_POL);
    if (ercd != E_OK) {
        ercd = E_NOMEM;
        goto _dns_err;
    }

    ercd = net_buf_get(&pkt[1], DNS_MSG_MAX_LEN, TMO_POL);
    if (ercd != E_OK) {
        ercd = E_NOMEM;
        goto _dns_err;
    }

    dns_tx_msg = pkt[0]->hdr;
    dns_rx_msg = pkt[1]->hdr;

    tx_id= net_rand();

    /* Construct DNS Query Message */

    dns_hdr             =   (T_DNS_HDR *)dns_tx_msg;
    dns_hdr->id         =   htons(tx_id);
    dns_hdr->flag       =   htons(0x0100);  /* RD */
    dns_hdr->qdcount    =   htons(1);
    dns_hdr->ancount    =   0;
    dns_hdr->nscount    =   0;
    dns_hdr->arcount    =   0;
    msg_len             =   DNS_HDR_SZ;

    /* Query RR */

    msg = dns_tx_msg + msg_len;
    rr  = &rrd; 

    if (code == RR_TYPE_PTR) {
        /*sprintf(name,"%d.%d.%d.%d.IN-ADDR.ARPA.", ptr[0], ptr[1], ptr[2], ptr[3]);*/
        SetQueryIp(name, *ipaddr);  /* set IP in Reverse order */
        net_strcat((char *)name,".IN-ADDR.ARPA.");
    }

    ercd = parse_to_label(name, (char *)msg);
    if (ercd <= 0) {
        ercd = E_PAR;
        goto _dns_err;
    }
    msg_len += ercd;

    rr->type    =   htons(code);
    rr->class   =   htons(RR_CLASS_IN);
    net_memcpy(msg + ercd, rr, 4);
    msg_len   +=    4;

    /* Do socket process for send & recv DNS messages */
    ercd = dns_transmit((char *)dns_tx_msg, msg_len, (char *)dns_rx_msg, dns_server, socid);
    if (ercd <= 0) {
        ercd = E_TMOUT;     /* Send or Recv Timeout */
        goto _dns_err;
    }
    end = dns_rx_msg + ercd;

    /* Process received DNS Response */
    ercd = E_OBJ;

    dns_hdr = (T_DNS_HDR *)dns_rx_msg;
    dns_hdr->id = ntohs(dns_hdr->id);
    dns_hdr->flag = ntohs(dns_hdr->flag);

    /* Discard if id does not match */ 
    if (dns_hdr->id != tx_id) {
        DNS_DEBUG(printf("\r\n dns_hdr id %d %d", dns_hdr->id, tx_id));
        goto _dns_err;
    }

    /* Discard if not a response */
    if (!(dns_hdr->flag & 0x8000)) {
        DNS_DEBUG(printf("\r\n dns not a response flg %x", dns_hdr->flag));
        goto _dns_err;
    }

    /* Discard if truncated response */
    if ((dns_hdr->flag & 0x0200)) {
        DNS_DEBUG(printf("\r\n dns not a response flg %x", dns_hdr->flag));
        goto _dns_err;
    }

    /* Discard if error code in response */
    rcode = dns_hdr->flag & 0x000F;
    if (rcode != 0) {
        DNS_DEBUG(printf("\r\n rcode %d", rcode));
        goto _dns_err;
    }

    dns_hdr->qdcount = ntohs(dns_hdr->qdcount);
    dns_hdr->ancount = ntohs(dns_hdr->ancount);

    ptr     = dns_rx_msg + DNS_HDR_SZ;

    /* Skip Query RR */
    while (dns_hdr->qdcount) {
        while (1) {
            if (ptr >= end) {
                ercd = E_OBJ;
                goto _dns_err;
            }
            if (*ptr == 0) {
                ptr++;
                break;
            }
            if ((*ptr & 0xC0) == 0xC0) {
                ptr += 2;
                break;
            }
            ptr += (*ptr) + 1;
        }
        ptr += 4;
        dns_hdr->qdcount--;
    }

    /* Search for Answer RR */
    while (dns_hdr->ancount) {
        while (1) {
            if (ptr >= end) {
                ercd = E_OBJ;
                goto _dns_err;
            }
            if (*ptr == 0) {
                ptr++;
                break;
            }
            if ((*ptr & 0xC0) == 0xC0) {
                ptr += 2;
                break;
            }
            ptr += (*ptr) + 1;
        }

        net_memcpy((char*)rr, ptr, DNS_RR_SZ);
        ptr += DNS_RR_SZ;
        if (ntohs(rr->class) == RR_CLASS_IN &&
            ntohs(rr->type)  == code) {
            ercd = E_OK;
            break;  
        }
        ptr += ntohs(rr->rdlength);
        dns_hdr->ancount--;
    }   

    if (ercd != E_OK) {
        goto _dns_err;
    }

    switch (ntohs(rr->type)) {
        case RR_TYPE_A:
            net_memcpy((UB*)ipaddr, ptr, 4);
            *ipaddr = htonl(*ipaddr);
            break;
        case RR_TYPE_PTR:
            parse_to_name(name, (char *)ptr);
            break;
#ifdef IPV6_SUP
        case RR_TYPE_AAAA:
            net_memcpy((UB*)ipaddr, ptr, 16);
            ip6_addr_hton(ipaddr, ipaddr);
            break;
#endif
        default:
            break;
    }

_dns_err:
    if (pkt[0]) net_buf_ret(pkt[0]);
    if (pkt[1]) net_buf_ret(pkt[1]);

    return ercd;
}


/*
    Resolve IPAddress for domain name
*/
ER dns_get_ipaddr(ID socid, UW dns_server, char *name, UW *ipaddr)
{
    ER ercd;

    if ((name == NULL) || (ipaddr == NULL)) {
        return E_PAR;
    }

    if (dns_server == 0) {
        return E_PAR;
    }

#ifdef NET_C
    if (socid == 0) {
        return E_PAR;
    }
#endif

    if (net_strcmp(name, "") == 0) {
        return E_PAR;
    }

    ercd = dns_query(RR_TYPE_A, name, ipaddr, dns_server, socid);

    return ercd;
}

/*
    Resolve Domain name for IPAddress
*/

ER dns_get_name(ID socid, UW dns_server, char *name, UW *ipaddr)
{
    ER ercd;
    
    if ((name == NULL) || (ipaddr == NULL)) {
        return E_PAR;
    }

    if (dns_server == 0) {
        return E_PAR;
    }

    if (*ipaddr == 0) {
        return E_PAR;
    }

#ifdef NET_C
    if (socid == 0) {
        return E_PAR;
    }
#endif

    /* if local host return LocalHost */

    ercd = dns_query(RR_TYPE_PTR, name, ipaddr, dns_server, socid);

    return ercd;
}

