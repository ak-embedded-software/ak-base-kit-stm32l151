/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    SNTP Client
    Copyright (c)  2012-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2012.06.07: Created
                         2012.10.02  Modify to avoid use of string libraries.
                         2014.04.11  Modified to determine return value of con_soc
 ***************************************************************************/

#include <stdlib.h>
#include "kernel.h"
#include "net_hdr.h"
#include "sntp_client.h"

static ER clear_rcvq(ID sid, UB *buf, UH len)
{
    ER ercd = E_OK;

    cfg_soc(sid, SOC_TMO_RCV, (VP)0);
    while (ercd != E_TMOUT) {
        ercd = rcv_soc(sid, buf, len);
    }
    return E_OK;
}

ER sntp_client(T_SNTP_CLIENT *sntp_client, UW *sec, UW *msec)
{
    T_NET_BUF *buf;
    T_SNTP_PACKET *pkt;
    T_NODE host;
    ER ercd;

    if (sntp_client == NULL || sec == NULL || msec == NULL) {
        return E_PAR;
    }
    if (sntp_client->ipa == 0) {
        return E_PAR;
    }
    if (sntp_client->devnum > NET_DEV_MAX) {
        return E_PAR;
    }

    if (sntp_client->devnum == DEV_ANY) {
        sntp_client->devnum++;
    }
    if (sntp_client->port == PORT_ANY) {
        sntp_client->port = SNTP_PORT;
    }
    if (sntp_client->tmo == 0) {
        sntp_client->tmo = SNTP_TIMEOUT;
    }

    net_memset(&host, 0, sizeof(host));
    host.num = sntp_client->devnum;
    host.ver = sntp_client->ipv;
    host.ipa = INADDR_ANY;
    host.port = PORT_ANY;
#ifdef NET_C
    if (sntp_client->sid == 0) {
        return E_PAR;
    }
#else
    ercd = cre_soc(IP_PROTO_UDP, &host);
    if (ercd <= E_OK) {
        return ercd;
    }
    sntp_client->sid = ercd;
#endif

    buf = NULL;
    ercd = net_buf_get(&buf, sizeof(T_SNTP_PACKET), TMO_POL);
    if (ercd != E_OK || buf == NULL) {
        goto _sntp_err;
    }
    pkt = (T_SNTP_PACKET*)buf->hdr;
    clear_rcvq(sntp_client->sid, (UB*)pkt, sizeof(T_SNTP_PACKET));

    /* Set Reception Timeout                */
    cfg_soc(sntp_client->sid, SOC_TMO_RCV, (VP)(sntp_client->tmo));

    /* Set remote host as SNTP Server */
    host.port = sntp_client->port;
    host.ipa = sntp_client->ipa;
    ercd = con_soc(sntp_client->sid, &host, SOC_CLI);
    if (ercd != E_OK) {
        goto _sntp_err;
    }
    
    /* Set SNTP Client message */
    net_memset(pkt, 0, sizeof(T_SNTP_PACKET));
    pkt->cntwrd = (SNTP_VERSION << 27 | SNTP_MODE << 24);
    pkt->cntwrd = htonl(pkt->cntwrd);

    ercd = snd_soc(sntp_client->sid, pkt, sizeof(T_SNTP_PACKET));
    if (ercd != sizeof(T_SNTP_PACKET)) {
        goto _sntp_err;
    }

    ercd = rcv_soc(sntp_client->sid, pkt, sizeof(T_SNTP_PACKET));
    if (ercd <= E_OK) {
        goto _sntp_err;
    }
    if (ercd != sizeof(T_SNTP_PACKET)) {
        /* Protocol Error */
        ercd = E_OBJ;
        goto _sntp_err;
    }
    *sec = ntohl(pkt->tts);
    *msec = ntohl(pkt->ttf);
    ercd = E_OK;

_sntp_err:
    if (buf) {
        net_buf_ret(buf);
    }
#ifndef NET_C
    del_soc(sntp_client->sid);
#endif
    return ercd;
}

