/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    HTTP Server
    Copyright (c)  2009-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2009.01.09: Created
                         2010.03.08: Correction of file not found error code.
                         2010.03.08: Support for query in URI
                         2010.06.29: Updated for IPv6 support
                         2010.10.21: Change timeout in accordance with Compact
                         2011.07.01: Remove lock/unlock TCP.
                         2012.03.30: Use strncasecmp for upper header case
                         2012.06.14: Corresponding reception of truncated header
                         2012.06.19: Corrected to break off posted data which
                                     is unacceptable and respond 413
                         2012.06.28: Replace strncasecmp() to net_strncasecmp()
                         2012.06.28: Change response phrase 200 and 404
                         2012.07.11: Corrected HttpTcpRecv() parameter the case
                                     of receiving partial contents
                         2012.08.09: Add HttpSendFile(), HttpSendErrorResponse()
                                     Allowed tx/rx buffer setting from parameter
                         2012.08.14: Expanded TX/RX buffer length 16 -> 32bit
                         2012.10.02  Modify to avoid use of string libraries.
                         2013.02.13: Changed variable type to pointer path, ctype
                         2013.05.07: Consider NULL terminate area in RX buffer
 ***************************************************************************/

#include <stdlib.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"

#include "http_server.h"

T_HTTP_FILE *gHTTP_FILE = NULL;

void CgiGetParam(char *msg, int clen, char *cgi_var[], char *cgi_val[], int *cgi_cnt)
{   
    char *c, *pr;
    int  i, cnt;

    *cgi_cnt = cnt = 0;

    if (clen == 0)
        return;

    /* Replace + into space */

    c = msg;
    for (i=0;i<clen;i++) {
        if (*(c+i) == '+')
            *(c+i) = ' ';
    }

    /* Split name and values name1=value1&name2=value2 */

    pr = c = msg;
    for (i=0;i<clen;i++) {
        /* variable */
        if (*c++ == '&') {
            *(c - 1) = '\0';
            cgi_var[cnt++] = pr;
            pr = c;
        }
    }

    if (cnt) {
        cgi_var[cnt++] = pr;
        *(c) = '\0';
    }
    else {
        cnt = 1;
        cgi_var[0] = pr;
        *(c) = '\0';
    }

    *cgi_cnt = cnt;
    cnt--;

    /* Seperate name1=value1 -> cgi_var[] = name1, cgi_val[] = value1 */
    for (;cnt>=0;cnt--) {
        cgi_val[cnt] = NULL;
        c = cgi_var[cnt];
        while (*c != '\0') {
            if (*c == '=') {
                *c = '\0';
                cgi_val[cnt] = c+1;
                break;
            }
            c++;
        }
    }
}

static void HttpCgiHandler(T_HTTP_SERVER *http, T_HTTP_FILE *fp)
{

    fp->cbk(http);

}

static T_HTTP_FILE *GetHttpFile(char *url)
{
    T_HTTP_FILE *fp = gHTTP_FILE;

    if (fp == NULL)
        return fp;

    while (1) {
        if (fp->path == NULL || net_strcmp(fp->path, "") == 0) {
            fp = NULL;
            break;
        }
        if (net_strcmp(fp->path, url) == 0) {
            break;
        }
        fp++;
    }

    return fp;
}

static UW HttpTcpSend(UH sid, UB *data, UW len)
{
    ER ercd;
    UW i, slen, tot_len;

    tot_len = len;

    i = 0;
    while (len > 0) {
        slen = (len > 1460) ? 1460 : len;
        ercd = snd_soc(sid, data + i, slen);
        if (ercd <= 0) {
            break;
        }
        i += ercd;
        len -= ercd;
    }

    return (i == tot_len);
}

static UW HttpTcpRecv(UH sid, UB *data, UW len)
{
    ER ercd;
    UW i;
    UH l;

    i = 0;
    while (len > 0)
    {
        l = len > 0xFFFF ? 0xFFFF : len;
        ercd = rcv_soc(sid, data + i, l);
        if (ercd <= 0) {
            break;
        }
        i += ercd;
        len -= ercd;
    }

    return i;
}

ER HttpSetContent(T_HTTP_SERVER *http, char *str)
{
    int len;
    
    if (http == NULL)
        return E_PAR;

    if (str == NULL) {
        http->txlen = 0;
        return 0;
    }

    len = net_strlen(str);

    len = (len > (http->sbufsz - http->txlen)) ? (http->sbufsz - http->txlen) : len;
    if (len) {
        net_memcpy((http->sbuf + http->txlen), str, len);
        http->txlen += len;
    }

    return len;
}

static void itoa_std(UW num, char* str)
{
    char c, *p, *q;

    p = q = str;

    /* Convert to ascii */
    do {
        c = num%10;
        *p++ = '0'+ c;
        num = num/10;
    } while(num);
    *p-- = '\0';
    
    /* Reverse the string */
    do {
       c = *p;
      *p = *q;
      *q = c;
      p--; q++;
    } while (q < p);
}

ER HttpSetContentLen(T_HTTP_SERVER *http, char *str, int len)
{
    char s[8];

    if (http == NULL) {
        return E_PAR;
    }

    HttpSetContent(http, str);
    /*itoa(len, s, 10);*/
    itoa_std(len, s);
    HttpSetContent(http, s);
    HttpSetContent(http, "\r\n");

    return E_OK;
}

ER HttpSendContent(T_HTTP_SERVER *http, char *str, int len)
{
    int clen;

    if (http == NULL)
        return E_PAR;
    if (str && len != 0 && http->txlen != 0) {
        clen = http->sbufsz - http->txlen;
        clen = (len > clen) ? clen : len;
        net_memcpy(http->sbuf + http->txlen, str, clen);
        str += clen;
        len -= clen;
        http->txlen += clen;
    }

    clen = 0;

    if (http->txlen != 0)
        clen += HttpTcpSend(http->SocketID, http->sbuf, http->txlen);

    if (len != 0)
        clen += HttpTcpSend(http->SocketID, (UB *)str, len);
    
    return len;
}

ER HttpSendHeader(T_HTTP_SERVER *http, char *str)
{
    HttpSetContent(http, 0);
    HttpSetContent(http, "HTTP/1.1 ");
    HttpSetContent(http, str);
    HttpSetContent(http, "\r\n");
    HttpSendContent(http, str, 0);

    return E_OK;
}

ER HttpSendText(T_HTTP_SERVER *http, char *str, int len)
{
    HttpSetContent(http, 0);
    HttpSetContent(http, "HTTP/1.1 200 OK\r\n");
    HttpSetContent(http, "Content-Type: text/html\r\n");
    HttpSetContentLen(http, "Content-Length: ", len);
    HttpSetContent(http, "\r\n");
    HttpSendContent(http, str, len);

    return E_OK;
}

ER HttpSendImage(T_HTTP_SERVER *http, char *str, int len)
{
    HttpSetContent(http, 0);
    HttpSetContent(http, "HTTP/1.1 200 OK\r\n");
    HttpSetContent(http, "Content-Type: image/html\r\n");
    HttpSetContentLen(http, "Content-Length: ", len);
    HttpSetContent(http, "\r\n");
    HttpSendContent(http, str, len);

    return E_OK;
}

ER HttpSendErrorResponse(T_HTTP_SERVER *http, char *str)
{
    HttpSetContent(http, 0);
    HttpSetContent(http, "HTTP/1.1 ");
    HttpSetContent(http, str);
    HttpSetContentLen(http, "Content-Length: ", net_strlen(str)+9);
    HttpSetContent(http, "\r\n");
    HttpSetContent(http, "<h1>");
    HttpSetContent(http, str);
    HttpSetContent(http, "</h1>");
    HttpSendContent(http, str, 0);

    return E_OK;
}

ER HttpSendFile(T_HTTP_SERVER *http, char *str, int len, char *name, char *type)
{
    HttpSetContent(http, 0);
    HttpSetContent(http, "HTTP/1.1 200 OK\r\n");
    HttpSetContent(http, "Content-Disposition: attachment; filename=");
    HttpSetContent(http, name);
    HttpSetContent(http, "\r\n");
    HttpSetContent(http, "Content-Type: ");
    HttpSetContent(http, type);
    HttpSetContent(http, "\r\n");
    HttpSetContentLen(http, "Content-Length: ", len);
    HttpSetContent(http, "\r\n");
    HttpSendContent(http, str, len);

    return E_OK;
}

/*
    " HTTP:\r\n\r\n"        return value    parameter
    Reached string length    0              NULL
    Reached end of line      1              NULL
    Reached token            1              start offset of token

Note:
    When token is \n then    1              start offset of token
*/

static char parse_tok(T_HTTP_SERVER *sess, char tok, char **par)
{
    char *c;

    *par = NULL;

    if (sess->len == 0) {
        return 0;
    }

    /* Skip leading space */
    if (*sess->req == 0x20) {
        for (;sess->len-- > 0;) {
            if (*++sess->req != 0x20)
                break;
        }
    }

    /* Search for token, break if \r\n */

    c = (char *)sess->req;

    for (;sess->len-- > 0;) {

        if (*sess->req == tok) {
            sess->req++;
            *par = c;
            return 1;
        }

        if (*sess->req == '\n') {
            sess->req++;
            return 1;
        }

        sess->req++;
    }

    return 0;
}

void HttpReadOptHeader(T_HTTP_SERVER *sess)
{
    char c;
    char *hdr = NULL;
    T_HTTP_HEADER *hr = &sess->hdr;

    c = 1;
    for (;;) {

        if ((c == 1)&& (hdr == NULL)) {
            if (net_strncasecmp((char const *)sess->req,"\r\n",2) == 0) {
                sess->req += 2;
                return; /* Blank line */
            }
        }

        c = parse_tok(sess, ':', &hdr);
        if (hdr == NULL) {
            if (!c) {   /* End of the string */
                return;
            }
            continue;   /* End of Line */
        }

        if (net_strncasecmp(hdr, "Host:", 5) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c) return;
            if (hdr) {
               *(sess->req - 2) = 0;    /* \r\n <- \r = 0 */
                hr->host = hdr;
                hdr = NULL;
            }
        }

        else if (net_strncasecmp(hdr, "Connection:", 11) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c)
                return;
            if (hdr) {
                if (net_strncasecmp(hdr, "Keep-Alive", 10) == 0) {
                    hr->kpa = 1;
                }
                hdr = NULL;
            }
        }

        else if (net_strncasecmp(hdr, "Content-Type:", 13) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c)
                return;
            if (hdr) {
               *(sess->req - 2) = 0;    /* \r\n <- \r = 0 */
                hr->ctype = hdr;
                hdr = NULL;
            }
        }

        else if (net_strncasecmp(hdr, "Content-Length:", 15) == 0) {
            c = parse_tok(sess, '\n', &hdr);
            if (!c)
                return;
            if (hdr) {
               *(sess->req - 2) = 0;    /* \r\n <- \r = 0 */
                hr->ContentLen = atoi(hdr);
                hdr = NULL;
            }
        }
    }
}


static ER HttpReadHeader(T_HTTP_SERVER *http)
{
    char c;
    T_HTTP_HEADER *hdr = &http->hdr;

    if (http->rdlen < 4) {
        goto _http_hdr_err;
    }

    http->req = http->rbuf;
    http->len = http->rdlen; 

    /* 1. Method (upper case) */
    hdr->method = (char *)http->req;
    if (net_strncasecmp((const char *)http->req, "GET ", 4) == 0) {
        *(http->req + 3) = '\0';
        http->len -= 4;
        http->req = http->req + 4;
    }
    else if (net_strncasecmp((const char *)http->req, "HEAD ", 5) == 0) {
        *(http->req + 4) = '\0';
        http->len -= 5;
        http->req += 5;
    }
    else if (net_strncasecmp((const char *)http->req, "POST ",5) == 0) {
        *(http->req + 4) = '\0';
        http->len -= 5;
        http->req += 5;
    }
    else {
        goto _http_hdr_err;
    }

    /* 2. Path - URI */

    c = parse_tok(http, 0x20, &hdr->url);
    if (!c) {
        goto _http_hdr_err;
    }

    if (hdr->url) {
        *(http->req - 1) = '\0';
        hdr->url_q = hdr->url;
        while (*hdr->url_q != 0) {
            if (*hdr->url_q == '?') {
                *hdr->url_q = '\0';
                hdr->url_q++;
                break;
            }
            hdr->url_q++;
        }
        if (*hdr->url_q == '\0') {
            hdr->url_q = NULL;
        }
    }

    /* check sess->len? */

    /* 3. HTTP Version (upper case) */
    hdr->ver = (char *)http->req;
    if (net_strncasecmp((const char *)http->req, "HTTP/1.1", 8) == 0) {
        *(http->req + 8) = '\0';
        http->len -= 8;
        http->req += 8;
    }
    else if (net_strncasecmp((const char *)http->req, "HTTP/1.0", 8) == 0) {
        *(http->req + 8) = '\0';
        http->len -= 8;
        http->req += 8;
    }
    else {
        /* Version not supported */
        goto _http_hdr_err;
    }

    /* 4. Optional Headers */

    HttpReadOptHeader(http);

    if (hdr->ContentLen != 0) {
        hdr->Content = (char *)http->req;
    }
    /* Validate Headers */

    /* HOST: required for HTTP 1.1 */

    if ((net_strncasecmp(hdr->ver, "HTTP/1.1",8) == 0) && (hdr->host == NULL))  {
        goto _http_hdr_err;
    }

    return E_OK;

_http_hdr_err:

    HttpSendErrorResponse(http, "405 Unsupported method\r\n");

    return E_OBJ;
}

static ER HttpSocReadln(T_HTTP_SERVER *http, UB **str, UH *len)
{
    ER ercd;
    UW flen;
    UB *s;

    *len = 0;

    s = (http->rbuf + http->rdlen);
    *str = s;

    
    for (;;) {

        if (http->rdlen == http->rxlen) {
            flen = http->rbufsz - http->rxlen;
            if (flen == 0) return E_NOMEM;
            if (flen > 0xFFFF) flen = 0xFFFF;
            ercd = rcv_soc(http->SocketID, http->rbuf + http->rxlen, flen);
            if (ercd <= 0) {
                *str = NULL;
                return E_CLS;
            }
            http->rxlen += ercd;
        }

        while (http->rdlen < http->rxlen) {
            (*len)++;
            http->rdlen++;
            if (*s == '\n') {
                return ((ER)(*len));
            }
            s++;
        }
    }

    /*return ((ER)(*len));*/
}

static ER HttpRecvRequest(T_HTTP_SERVER *http)
{
    char *str;
    UH len;
    ER ercd;

    net_memset((char *)&http->hdr, 0, sizeof(T_HTTP_HEADER));

    for (;;) {
        ercd = HttpSocReadln(http, (UB **)&str, &len);
        if (len == 0) {
            /* broken http header */
            ercd = E_OBJ;
            break;
        }

        if (len == 2) {
            /* eof http header */
            ercd = E_OK;
            break;
        }

    }

    return ercd;
}

static ER HttpReadContent(T_HTTP_SERVER *http)
{
    ER  ercd;
    int len;
    T_HTTP_HEADER *hdr = &http->hdr;

    /* Content not present */
    if (hdr->ContentLen == 0) {
        return E_OK;
    }

    /* Content already in recv buffer */
    len = http->rxlen - http->rdlen;
    if (len >= hdr->ContentLen) {
        /* we assume the content is end at '\0' */
        *(UB *)(hdr->Content + hdr->ContentLen) = '\0';
        return E_OK;
    }

    /* Contents size is too big */
    if (http->rbufsz - http->rdlen < hdr->ContentLen) {
        return E_NOMEM;
    }

    /* recv partial or full content */
    len = hdr->ContentLen - len;
    ercd = HttpTcpRecv(http->SocketID, http->rbuf+http->rxlen, len);
    if (ercd <= 0) {
        return E_OBJ;
    }

    /* we assume the content is end at '\0' */
    *(UB *)(hdr->Content + hdr->ContentLen) = '\0';

    return E_OK;
}

static ER HttpServerProcess(T_HTTP_SERVER *http)
{
    T_HTTP_FILE *fp;
    ER   ercd;
    T_HTTP_HEADER *hdr = &http->hdr;


    /* 1. Receive Request */

    /* Receive HTTP Header */
    ercd = HttpRecvRequest(http);
    if (ercd != E_OK) {
        return ercd;
    }

    /* Parse Header             */
    ercd = HttpReadHeader(http);
    if (ercd != E_OK) {
        return ercd;
    }

    /* Read Content             */
    if (hdr->ContentLen >= 0) {
        ercd = HttpReadContent(http);
        if (ercd == E_NOMEM) {
            HttpSendErrorResponse(http, "413 Request Entity Too Large\r\n");
            cls_soc(http->SocketID, SOC_TCP_SHT);
            while (1) {
                ercd = rcv_soc(http->SocketID, http->rbuf, http->rbufsz);
                if (ercd <= 0) {
                    break;
                }
            }
            return ercd;
        }

        if (ercd != E_OK) {
            return ercd;
        }
    }

    /* 2. Process Request */

    fp = GetHttpFile(hdr->url);
    if (fp==NULL) {
        HttpSendErrorResponse(http, "404 Not found\r\n");
        return E_OK;
    }

    if (fp->cbk == NULL) {
        HttpSetContent(http, 0);
        HttpSetContent(http, "HTTP/1.1 200 OK\r\n");
        HttpSetContent(http, "Content-Type: ");
        HttpSetContent(http, (char *)fp->ctype);
        HttpSetContent(http, "\r\n");
        HttpSetContentLen(http, "Content-Length: ", fp->len);
        HttpSetContent(http, "\r\n");
        HttpSendContent(http, (char *)fp->file, fp->len);
        return E_OK;
    }

    HttpCgiHandler(http, fp);

    return E_OK;

}

static ER HttpServerListen(T_HTTP_SERVER *http)
{
    T_NODE host;
    ER ercd;
    
    for (;;) {
        net_memset((char *)&host, 0, sizeof(host));
        host.num = http->NetChannel;
        host.ver = http->ver;
        ercd = con_soc(http->SocketID, &host, SOC_SER);
        if (ercd == E_OK) {
            break;
        }
        tslp_tsk(1);
    }

    return ercd;
}

static ER HttpServerClose(T_HTTP_SERVER *http)
{
    cls_soc(http->SocketID, 0);

    return E_OK;
}

ER http_server(T_HTTP_SERVER *http)
{
    T_NET_BUF *pkt[2];
    ER ercd;
#ifndef NET_C
    T_NODE host;
#endif

    /* Validate parameter */
    if (http == NULL) {
        return E_PAR;
    }

    if (http->Port == 0) {
        http->Port = HTTP_PORT;
    }

#ifdef NET_C
    if (http->SocketID == 0) {
        return E_PAR;
    }
#else
    /* Create HTTP Socket  */
    host.num  = http->NetChannel;
    host.ver  = http->ver;      /* Modified to support IPV6 */
#ifdef IPV6_SUP
    if(host.ver == IP_VER6)
        net_memset(host.ip6a, 0, 16);
    else
#endif
    host.ipa  = INADDR_ANY;
    host.port = http->Port;
    ercd = cre_soc(IP_PROTO_TCP, &host);
    if (ercd <= 0) {
        return ercd;
    }
    http->SocketID = ercd;

    /* Set Timeout's */
    ercd = cfg_soc(http->SocketID, SOC_TMO_SND, (VP)25000);
    ercd = cfg_soc(http->SocketID, SOC_TMO_RCV, (VP)25000);
    ercd = cfg_soc(http->SocketID, SOC_TMO_CLS, (VP)25000);
#endif

    pkt[0]=pkt[1]=NULL;
    if (http->rbuf == NULL) {
        ercd = net_buf_get(&pkt[0], HTTP_BUF_SIZ, TMO_POL);
        if (ercd != E_OK) {
            return ercd;
        }
        http->rbuf       = pkt[0]->hdr;
        http->rbufsz     = HTTP_BUF_SIZ;
    }
    http->rbufsz--; /* for end of contents */

    if (http->sbuf == NULL) {
        ercd = net_buf_get(&pkt[1], HTTP_BUF_SIZ, TMO_POL);
        if (ercd != E_OK) {
            if (pkt[0] != NULL) {
                net_buf_ret(pkt[0]);
            }
            return ercd;
        }
        http->sbuf       = pkt[1]->hdr;
        http->sbufsz     = HTTP_BUF_SIZ;
    }

    for (;;) {

        ercd = HttpServerListen(http);
        if (ercd != E_OK) {
            continue;
        }

        http->rxlen      = 0;
        http->rdlen      = 0;
        http->req        = NULL;
        http->len        = 0;

        HttpServerProcess(http);

        HttpServerClose(http);
    }
}
