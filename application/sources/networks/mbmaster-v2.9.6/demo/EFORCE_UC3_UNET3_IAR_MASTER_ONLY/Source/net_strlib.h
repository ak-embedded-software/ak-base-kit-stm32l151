/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    String Library header
    Copyright (c)  2012-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2012.10.02: Created
 ***************************************************************************/

#ifndef UNET3_STRLIB_H
#define UNET3_STRLIB_H
#ifdef __cplusplus
extern "C" {
#endif

#include "kernel.h"

W net_strncasecmp(const char *str1, const char *str2, W len);
W net_strcmp(const char *str1, const char *str2);
char* net_strcpy(char *str1, const char *str2);
UW net_strlen(const char *str);
char* net_strcat(char *str1, const char *str2);
char* net_strchr(const char *str, int ch);

#ifdef __cplusplus
}
#endif
#endif

