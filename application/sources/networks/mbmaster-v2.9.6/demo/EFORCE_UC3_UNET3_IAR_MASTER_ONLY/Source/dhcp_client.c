/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    DHCP Client
    Copyright (c)  2008-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2008.11.30: Created
                         2010.12.27: Modified BroadCast Reception at Compact
                         2011.05.23: Clear networkbuffer when error occured
                         2012.05.08: Set local variables initial value and
                                     corrected path for error cases.
                         2012.08.10: Delete unnecessary operator in parsing
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.04.10  H/W OS supported version
                         2014.03.11  Modified to not use unnecessary loc_tcp
 ***************************************************************************/

#include <stdlib.h>
#include "kernel.h"
#include "net_hdr.h"

#include "dhcp_client.h"

static void dhcp_cfg_netaddr(T_HOST_ADDR *dhcp)
{
    T_NET_ADR adr;

    adr.ipaddr  = dhcp->ipaddr;
    adr.mask    = dhcp->subnet;
    adr.gateway = dhcp->gateway;

    net_cfg(dhcp->dev_num, NET_IP4_CFG, (VP)&adr);
}

UB *dhcp_parse_opt_ptr(UB *ptr, UH len, UB opt)
{
    while (len != 0) {
        if (*ptr == 255)
            return NULL;

        if (*ptr++ == opt) {
            return (ptr + 1);
        }
        else {
            if (*ptr == 0) {    /*padding*/
                len--;
                ptr++;
            }
            len -= (*ptr) + 1;
            ptr = ptr + (*ptr) + 1;
        }
    }

    return NULL;
}

ER dhcp_client(T_HOST_ADDR *addr)
{
    T_NODE host;
    ID sid;
    ER ercd;
    T_NET *net;
    T_DHCP_MSG *dhcp_msg, *dhcp_rcv;
    static UW dhcp_xid = 0x13572468;
    UW yiaddr;
    UH opt_len;
    UB *opt_ptr, *ptr, ser_id[4];
    UH dev_num;
    INT n;
    T_NET_BUF *pkt[2];
    UB bcflg;

    if (addr == NULL) {
        return E_PAR;
    }

#ifdef NET_C
    if (addr->socid == 0) {
        return E_PAR;
    }
#endif

    pkt[0] = pkt[1] = NULL;
    bcflg = 1;
    sid = 0;

    ercd = net_buf_get(&pkt[0], DHCP_MSG_SZ, TMO_POL);
    if (ercd != E_OK) {
        goto _dhcp_env_err;
    }
    ercd = net_buf_get(&pkt[1], DHCP_MSG_SZ, TMO_POL);
    if (ercd != E_OK) {
        goto _dhcp_env_err;
    }

    dhcp_msg = (T_DHCP_MSG *)(pkt[0]->hdr);
    dhcp_rcv = (T_DHCP_MSG *)(pkt[1]->hdr);

    dev_num = addr->dev_num;

    host.num  = dev_num;
    host.ipa  = INADDR_ANY;
    host.port = 68;         /*BOOTP PORT*/
    host.ver  = IP_VER4;
#ifndef NET_C
    sid = cre_soc(IP_PROTO_UDP, &host);
    if (sid <= 0) {
        ercd = E_NOMEM;
        goto _dhcp_env_err;
    }
#else
    sid = addr->socid;
#endif

    /* Refer BroadCast Reception       */
    ercd = net_ref(host.num, NET_BCAST_RCV, (VP)&bcflg);
    if (ercd != E_OK) {
        goto _dhcp_env_err;
    }

    /* Enable BroadCast Reception       */
    if (bcflg == 0) {
        ercd = net_cfg(host.num, NET_BCAST_RCV, (VP)1);
        if (ercd != E_OK) {
            goto _dhcp_env_err;
        }
    }

#ifndef NET_C
    /* Set Transmission Timeout (5 Sec)    */
    ercd = cfg_soc(sid, SOC_TMO_SND, (VP)5000);
    if (ercd != E_OK) {
        goto _dhcp_err;
    }

    /* Set Reception Timeout (5 Sec)    */
    ercd = cfg_soc(sid, SOC_TMO_RCV, (VP)5000);
    if (ercd != E_OK) {
        goto _dhcp_err;
    }
#endif
    
    /* Set Remote Host IP Address */
    host.num  = dev_num;
    host.ipa  = 0xFFFFFFFF;
    host.port = 67;
    host.ver  = IP_VER4;
    ercd = con_soc(sid, &host, SOC_CLI);
    if (ercd != E_OK) {
        goto _dhcp_err;
    }

    if (dev_num != 0) {
        dev_num--;
    }

    net = &gNET[dev_num];
    net_memcpy(addr->mac, net->dev->cfg.eth.mac, 6);

    dhcp_xid += net_rand(); /*random*/

    /* DHCP Discover */
    net_memset((char*)dhcp_msg, 0, sizeof(T_DHCP_MSG));
    dhcp_msg->op = DHCP_OPC_BOOTREQ;
    dhcp_msg->htype = DHCP_ETH_TYPE;
    dhcp_msg->hlen  = DHCP_ETH_LEN;
    dhcp_msg->hops  = 0;
    dhcp_msg->xid   = htonl(dhcp_xid);
    dhcp_msg->secs  = 0;
    dhcp_msg->flags = htons(DHCP_FLG_BCAST);
    net_memcpy(dhcp_msg->chaddr, addr->mac, 6);
    dhcp_msg->ciaddr = 0;
    dhcp_msg->yiaddr = 0;
    dhcp_msg->siaddr = 0;
    dhcp_msg->giaddr = 0;

    /*MagicCookie 99,130,83,99 */
    dhcp_msg->opt[0] = 0x63;
    dhcp_msg->opt[1] = 0x82;
    dhcp_msg->opt[2] = 0x53;
    dhcp_msg->opt[3] = 0x63;
    dhcp_msg->opt[4] = DHCP_OPT_DHCPMSGTYPE; /*None*/
    dhcp_msg->opt[5] = 1;
    dhcp_msg->opt[6] = DHCP_MSG_DISCOVER;
    dhcp_msg->opt[7] = 255; /*EOP*/

    ercd = snd_soc(sid, (VP)dhcp_msg, (UH)sizeof(T_DHCP_MSG));
    if (ercd <= 0) {
        ercd = E_TMOUT;
        goto _dhcp_err;
    }

    /* DHCP Offer   */
    for (;;) {

        ercd = rcv_soc(sid, (VP)dhcp_rcv, (UH)sizeof(T_DHCP_MSG));
        if (ercd <= 0) {
            ercd = E_TMOUT;
            goto _dhcp_err;
        }

        if (dhcp_rcv->op != DHCP_OPC_BOOTREPLY) {
            continue;
        }

        if (ntohl(dhcp_rcv->xid) != dhcp_xid) {
            continue;
        }

        opt_len = ercd - DHCP_MSG_LEN;
        opt_ptr = dhcp_rcv->opt + 4;
        ptr = dhcp_parse_opt_ptr(opt_ptr, opt_len, DHCP_OPT_DHCPMSGTYPE);
        if (ptr == NULL) {
            ercd = E_OBJ;
            goto _dhcp_err;
        }

        if (*ptr != DHCP_MSG_OFFER) {
            ercd = E_OBJ;
            goto _dhcp_err;
        }

        break;
    }

    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_SERVERIDENT);
    if (ptr == NULL) {
        ercd = E_OBJ;
        goto _dhcp_err;
    }

    net_memcpy(ser_id, ptr, 4);
    yiaddr = dhcp_rcv->yiaddr;

    /* DHCP Request */
    dhcp_xid = ntohl(dhcp_rcv->xid);
    net_memset((char*)dhcp_msg, 0, sizeof(T_DHCP_MSG));
    dhcp_msg->op    = DHCP_OPC_BOOTREQ;
    dhcp_msg->htype = DHCP_ETH_TYPE;
    dhcp_msg->hlen  = DHCP_ETH_LEN;
    dhcp_msg->hops  = 0;
    dhcp_msg->xid   = htonl(dhcp_xid);
    dhcp_msg->secs  = 0;
    dhcp_msg->flags = htons(DHCP_FLG_BCAST);
    net_memcpy(dhcp_msg->chaddr, addr->mac, 6);
    dhcp_msg->ciaddr = 0;
    dhcp_msg->yiaddr = 0;
    dhcp_msg->siaddr = 0;
    dhcp_msg->giaddr = 0;

    /*MagicCookie 99,130,83,99 */
    n = 0;
    dhcp_msg->opt[n++] = 0x63;
    dhcp_msg->opt[n++] = 0x82;
    dhcp_msg->opt[n++] = 0x53;
    dhcp_msg->opt[n++] = 0x63;
    dhcp_msg->opt[n++] = DHCP_OPT_DHCPMSGTYPE;
    dhcp_msg->opt[n++] = 1;
    dhcp_msg->opt[n++] = DHCP_MSG_REQUEST;

    dhcp_msg->opt[n++] = DHCP_OPT_PRMLST;
    dhcp_msg->opt[n++] = 6;
    dhcp_msg->opt[n++] = DHCP_OPT_SUBNET;
    dhcp_msg->opt[n++] = DHCP_OPT_ROUTER;
    dhcp_msg->opt[n++] = DHCP_OPT_DNS;
    dhcp_msg->opt[n++] = DHCP_OPT_IPLEASE;
    dhcp_msg->opt[n++] = DHCP_OPT_RENETM;
    dhcp_msg->opt[n++] = DHCP_OPT_REBITM;

    dhcp_msg->opt[n++] = DHCP_OPT_SERVERIDENT;
    dhcp_msg->opt[n++] = 4;
    net_memcpy(&dhcp_msg->opt[n], ser_id, 4);
    n += 4;

    dhcp_msg->opt[n++] = DHCP_OPT_REQIPADDR;
    dhcp_msg->opt[n++] = 4;
    net_memcpy(&dhcp_msg->opt[n], (char*)&yiaddr, 4);
    n += 4;
    dhcp_msg->opt[n++] = 255;

    ercd = snd_soc(sid, (VP)dhcp_msg, (UH)sizeof(T_DHCP_MSG));
    if (ercd <= 0) {
        ercd = E_OBJ;
        goto _dhcp_err;
    }

    /* DHCP ACK     */
    for (;;) {

        ercd = rcv_soc(sid, (VP)dhcp_rcv, (UH)sizeof(T_DHCP_MSG));
        if (ercd <= 0) {
            ercd = E_TMOUT;
            goto _dhcp_err;
        }

        if (dhcp_rcv->op != DHCP_OPC_BOOTREPLY) {
            continue;
        }

        if (ntohl(dhcp_rcv->xid) != dhcp_xid) {
            continue;
        }

        opt_len = ercd - DHCP_MSG_LEN;
        opt_ptr = dhcp_rcv->opt + 4;
        ptr = dhcp_parse_opt_ptr(opt_ptr, opt_len, DHCP_OPT_DHCPMSGTYPE);
        if (ptr == NULL) {
            goto _dhcp_err;
        }

        if (*ptr != DHCP_MSG_ACK) {
            ercd = E_OBJ;
            goto _dhcp_err;
        }

        break;
    }

    /*DHCP Server*/
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_SERVERIDENT);
    if (ptr != NULL) {
        addr->dhcp = ip_byte2n((char*)ptr);
    }

    /*Router*/
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_ROUTER);
    if (ptr != NULL) {
        addr->gateway = ip_byte2n((char*)ptr);
    }

    /*SubNet Mask*/
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_SUBNET);
    if (ptr != NULL) {
        addr->subnet = ip_byte2n((char*)ptr);
    }

    /*DNS*/
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_DNS);
    if (ptr != NULL) {
        addr->dns[0] = ip_byte2n((char*)ptr);
    }

    /*IP Lease */
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_IPLEASE);
    if (ptr != NULL) {
        addr->lease = ip_byte2n((char*)ptr);
    }
    else {
        addr->lease = (UW)(-1);
    }

    /* Renew */
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_RENETM);
    if (ptr != NULL) {
        addr->t1 = ip_byte2n((char*)ptr);
    }
    else {
        /* (0.5 * duration of lease) */
        addr->t1 = addr->lease/2;
    }

    /* Rebind */
    ptr = dhcp_parse_opt_ptr(opt_ptr,opt_len, DHCP_OPT_REBITM);
    if (ptr != NULL) {
        addr->t2 = ip_byte2n((char*)ptr);
    }
    else {
        /* (0.875 * druation of lease) */
        addr->t2 = addr->lease - (addr->lease / 8);
    }

    addr->ipaddr = htonl(dhcp_rcv->yiaddr);

    dhcp_cfg_netaddr(addr);

    ercd = E_OK;

_dhcp_err:
    if (bcflg == 0) {
        net_cfg(host.num, NET_BCAST_RCV, (VP)0);
    }

_dhcp_env_err:    /* environments error */
#ifndef NET_C
    del_soc(sid);
#endif

    if (pkt[0]) net_buf_ret(pkt[0]);
    if (pkt[1]) net_buf_ret(pkt[1]);

    return ercd;
}

