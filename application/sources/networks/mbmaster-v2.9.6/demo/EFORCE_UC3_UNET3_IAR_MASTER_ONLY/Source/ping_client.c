/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    Ping Client
    Copyright (c)  2012-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2012.06.07: Created for new configurator
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.01.21  Update for IPv6
                         2013.07.10  Change IPv6 minimum packet length
 ***************************************************************************/


#include <stdlib.h>
#include "kernel.h"
#include "net_hdr.h"
#include "ping_client.h"

static ER clear_rcvq(ID sid, UB *buf, UH len)
{
    ER ercd = E_OK;

    cfg_soc(sid, SOC_TMO_RCV, (VP)0);
    while (ercd != E_TMOUT) {
        ercd = rcv_soc(sid, buf, len);
    }
    return E_OK;
}

#ifdef IPV6_SUP
ER ping6_client(T_PING_CLIENT_V6 *ping_client)
{
    T_ICMPV6_HDR *icmp6hdr;
    T_NET_BUF *buf;
    T_NODE host;
    UB *ping_data;
    ER ercd, i;
    char ptn;

    if (ping_client == NULL) {
        return E_PAR;
    }
    if (ip6_addr_type(ping_client->ip6addr) == IP6_ADDR_UNSPEC) {
        return E_PAR;
    }
    if (ping_client->devnum > NET_DEV_MAX) {
        return E_PAR;
    }

    if (ping_client->devnum == DEV_ANY) {
        ping_client->devnum++;
    }
    if (ping_client->tmo == 0) {
        ping_client->tmo = PING_TIMEOUT;
    }
    if (ping_client->len > PING_LEN_MAX) {
        ping_client->len = PING_LEN_MAX;
    }
    if (ping_client->len < PING_LEN_MIN + IP6_HDR_SZ) {
        ping_client->len = PING_LEN_MIN + IP6_HDR_SZ;
    }

    /* Create ICMP Socket */
    net_memset(host.ip6a, 0, 16);
    host.num  = ping_client->devnum;
    host.ver  = IP_VER6;
    host.port = 0;          /* Port should 0 for ICMP */
#ifdef NET_C
    if (ping_client->sid == 0) {
        return E_PAR;
    }
#else
    ercd = cre_soc(IP_PROTO_ICMPV6, &host);
    if (ercd <= 0) {
        return ercd;
    }
    ping_client->sid = ercd;
#endif

    buf = NULL;
    ercd = net_buf_get(&buf, ping_client->len, TMO_POL);
    if (ercd != E_OK || buf == NULL) {
        goto _ping_err;
    }
    ping_data = buf->hdr;
    clear_rcvq(ping_client->sid, ping_data, ping_client->len);

    /* Set Transmission Timeout             */
    ercd = cfg_soc(ping_client->sid, SOC_TMO_SND, (VP)ping_client->tmo);
    if (ercd != E_OK) {
        goto _ping_err;
    }

    /* Set Reception Timeout                */
    ercd = cfg_soc(ping_client->sid, SOC_TMO_RCV, (VP)ping_client->tmo);
    if (ercd != E_OK) {
        goto _ping_err;
    }

    /* Set Remote Host IP Address */
    ip6_addr_cpy(host.ip6a, ping_client->ip6addr);
    ercd = con_soc(ping_client->sid, &host, SOC_CLI);
    if (ercd != E_OK) {
        goto _ping_err;
    }

    /* Construct Ping Message */
    icmp6hdr = (T_ICMPV6_HDR *)ping_data;
    icmp6hdr->msg_type = ICMPV6_TYPE_ECHO_REQ;
    icmp6hdr->msg_code = 0;
    icmp6hdr->cs   = 0;
    icmp6hdr->msg_data.echo.id   = htons(1);
    icmp6hdr->msg_data.echo.seq  = 0;
    
    /* Set Data Pattern */
    ptn = 0;
    for (i=8;i<ping_client->len;i++) {
        ping_data[i] = ptn++;
    }
    ping_client->len += 8;

    /* Transmit ICMP Echo Request */
    ercd = snd_soc(ping_client->sid, (VP)ping_data, (UH)ping_client->len);
    if (ercd <= 0) {
        ercd = E_TMOUT;
        goto _ping_err;
    }

    /* Wait for ICMP Echo Reply */
    ercd = rcv_soc(ping_client->sid, (VP)ping_data, (UH)ping_client->len);
    if (ercd <= 0) {
        ercd = E_TMOUT;
        goto _ping_err;
    }

    /* Reply includes (IP + ICMP Header + Data) */
    icmp6hdr = (T_ICMPV6_HDR *)(ping_data + IP6_HDR_SZ);
    if (icmp6hdr->msg_type == ICMPV6_TYPE_ECHO_REPLY) {
        ercd = E_OK;
    }
    else {
        ercd = E_TMOUT;
    }

_ping_err:
    if (buf)net_buf_ret(buf);
#ifndef NET_C
    del_soc(ping_client->sid);
#endif
    return ercd;
}
#endif

ER ping_client(T_PING_CLIENT *ping_client)
{
    T_ICMP_HDR *icmphdr;
    T_NET_BUF *buf;
    T_NODE host;
    UB *ping_data;
    ER ercd, i;
    char ptn;

    if (ping_client == NULL) {
        return E_PAR;
    }
    if (ping_client->ipa == 0) {
        return E_PAR;
    }
    if (ping_client->devnum > NET_DEV_MAX) {
        return E_PAR;
    }

    if (ping_client->devnum == DEV_ANY) {
        ping_client->devnum++;
    }
    if (ping_client->tmo == 0) {
        ping_client->tmo = PING_TIMEOUT;
    }
    if (ping_client->len > PING_LEN_MAX) {
        ping_client->len = PING_LEN_MAX;
    }
    if (ping_client->len < PING_LEN_MIN) {
        ping_client->len = PING_LEN_MIN;
    }

    /* Create ICMP Socket */
    net_memset(&host, 0, sizeof(host));
    host.num = ping_client->devnum;
    host.ver = IP_VER4;  /* this API is IPv4 only*/
    host.ipa = INADDR_ANY;
    host.port = PORT_ANY;/* Port should 0 for ICMP */
#ifdef NET_C
    if (ping_client->sid == 0) {
        return E_PAR;
    }
#else
    ercd = cre_soc(IP_PROTO_ICMP, &host);
    if (ercd <= 0) {
        return ercd;
    }
    ping_client->sid = ercd;
#endif

    buf = NULL;
    ercd = net_buf_get(&buf, ping_client->len, TMO_POL);
    if (ercd != E_OK || buf == NULL) {
        goto _ping_err;
    }
    ping_data = buf->hdr;
    clear_rcvq(ping_client->sid, ping_data, ping_client->len);

    /* Set Reception Timeout                */
    ercd = cfg_soc(ping_client->sid, SOC_TMO_RCV, (VP)ping_client->tmo);
    if (ercd != E_OK) {
        goto _ping_err;
    }

    /* Set Remote Host IP Address */
    host.ipa  = ping_client->ipa;
    ercd = con_soc(ping_client->sid, &host, SOC_CLI);
    if (ercd != E_OK) {
        goto _ping_err;
    }

    /* Construct Ping Message */
    icmphdr = (T_ICMP_HDR *)ping_data;

    icmphdr->type = ICMP_ECHO_REQUEST;
    icmphdr->code = 0;
    icmphdr->cs   = 0;
    icmphdr->id   = htons(1);
    icmphdr->seq  = htons(0);

    /* Set Data Pattern */
    ptn = 0;
    for (i=8;i<ping_client->len;i++) {
        ping_data[i] = ptn++;
    }
    ping_client->len += 8;

    /* Transmit ICMP Echo Request */
    ercd = snd_soc(ping_client->sid, (VP)ping_data, ping_client->len);
    if (ercd <= 0) {
        ercd = E_TMOUT;
        goto _ping_err;
    }

    /* Wait for ICMP Echo Reply */
    ercd = rcv_soc(ping_client->sid, (VP)ping_data, ping_client->len);
    if (ercd <= 0) {
        ercd = E_TMOUT;
        goto _ping_err;
    }

    /* Reply includes (IP + ICMP Header + Data) */
    icmphdr = (T_ICMP_HDR *)(ping_data + IP4_HDR_SZ);
    if (icmphdr->type == ICMP_ECHO_REPLY) {
        ercd = E_OK;
    }
    else {
        ercd = E_TMOUT;
    }

_ping_err:
    if (buf)net_buf_ret(buf);
#ifndef NET_C
    del_soc(ping_client->sid);
#endif

    return ercd;
}
