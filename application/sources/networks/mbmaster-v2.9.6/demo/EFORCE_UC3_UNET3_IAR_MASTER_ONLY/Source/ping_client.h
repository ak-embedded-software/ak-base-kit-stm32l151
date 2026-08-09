/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    Ping Client header file
    Copyright (c)  2012-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2012.06.07: Created
 ***************************************************************************/

#ifndef PING_C_H
#define PING_C_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"
#include "net_hdr.h"

#define PING_LEN_MAX    1472
#define PING_LEN_MIN      32

#define PING_TIMEOUT    3000

typedef struct {
    ID sid;
    UW ipa;
    TMO tmo;
    UH devnum;
    UH len;

}T_PING_CLIENT;

#ifdef IPV6_SUP
typedef struct {
    ID sid;
    UW *ip6addr;
    TMO tmo;
    UH devnum;
    UH len;

}T_PING_CLIENT_V6;
#endif

ER ping_client(T_PING_CLIENT *ping_client);
#ifdef IPV6_SUP
ER ping6_client(T_PING_CLIENT_V6 *ping_client);
#endif

#ifdef __cplusplus
}
#endif
#endif
