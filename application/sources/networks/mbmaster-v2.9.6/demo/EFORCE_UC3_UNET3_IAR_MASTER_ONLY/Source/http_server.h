/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    HTTP Server header file
    Copyright (c)  2009-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2009.01.09: Created
                         2010.03.08: Support for query in URI
                         2012.08.09: Add HttpSendFile(), HttpSendErrorResponse().
                         2012.08.14: Expanded TX/RX buffer length 16 -> 32bit
                         2013.02.13: Changed variable type to pointer path, ctype
 ***************************************************************************/

#ifndef HTTPD_H
#define HTTPD_H
#ifdef __cplusplus
extern "C" {
#endif

#define HTTP_PORT           80
#define HTTP_BUF_SIZ        ((NET_BUF_SZ) - sizeof(T_NET_BUF))

typedef struct t_http_header {
    char *method;   /* GET or HEAD or POST  */
    char *url;      /* URL / File Name      */
    char *url_q;    /* URL query            */
    char *ver;      /* 1.0 or 1.1           */
    char *host;     /* Client Host          */
    char *ctype;    /* Content-Type         */
    char *Content;  /* Content              */
    int  ContentLen;/* Content length       */
    char kpa;
}T_HTTP_HEADER;

typedef struct t_http_server {
    UW sbufsz;
    UW rbufsz;
    UW txlen;
    UW rxlen;
    UW rdlen;
    UW len;     /* temp */
    UB *rbuf;
    UB *sbuf;
    UB *req;
    UH Port;
    UH SocketID;
    T_HTTP_HEADER hdr;
    UB NetChannel;
    UB ver;     /* Added for IPV6 support */
}T_HTTP_SERVER;

typedef struct t_http_file {
    const char *path;
    const char *ctype;
    const char *file;
    int   len;
    void (*cbk)(T_HTTP_SERVER *http);
}T_HTTP_FILE;

/* API CGI */
void CgiGetParam(char *msg, int clen, char *cgi_var[], char *cgi_val[], int *cgi_cnt);

/* API HTTP */
ER http_server(T_HTTP_SERVER *http);
ER HttpSendText(T_HTTP_SERVER *http, char *str, int len);
ER HttpSendImage(T_HTTP_SERVER *http, char *str, int len);
ER HttpSendFile(T_HTTP_SERVER *http, char *str, int len, char *name, char *type);
ER HttpSendErrorResponse(T_HTTP_SERVER *http, char *str);

extern T_HTTP_FILE  *gHTTP_FILE;

#ifdef __cplusplus
}
#endif
#endif /* HTTPD_H */
