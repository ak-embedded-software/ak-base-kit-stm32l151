/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    SNTP Client header file
    Copyright (c)  2012-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2012.06.07: Created
 ***************************************************************************/

#ifndef SNTP_C_H
#define SNTP_C_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"
#include "net_hdr.h"

#define SNTP_PORT           123
#define NTP_BASE_TIME       2208988800ul /* 1970-1900 sec */
#define UTC_FRACTION_SCAL   4294967296.0 /* 2^32 */

#define SNTP_LEAP_INDICATE  3
#define SNTP_VERSION        1
#define SNTP_MODE           3

#define SNTP_TIMEOUT        1000

typedef struct {
    UW cntwrd;                      /* Control Word */
    UW rtdly;                       /* Root Delay   */
    UW rtdsp;                       /* Root Dispersion */
    UW refid;                       /* Refarence Clock ID */
    UW reftim[2];                   /* Refarence Clock Update Time */
    UW orgtim[2];                   /* Originate Time Stamp */
    UW rcvtim[2];                   /* Receive Time Stamp */
    UW tts;                         /* Transmit Timestamp Seconds */
    UW ttf;                         /* Transmit Timestamp Fraction  */
}T_SNTP_PACKET;

typedef struct {
    ID sid;
    UW ipa;
    TMO tmo;
    UH devnum;
    UH port;
    UB ipv;
}T_SNTP_CLIENT;

ER sntp_client(T_SNTP_CLIENT *sntp_client, UW *sec, UW *msec);

#ifdef __cplusplus
}
#endif
#endif

