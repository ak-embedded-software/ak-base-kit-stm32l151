/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    String Library
    Copyright (c)  2012-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2012.10.02: Created
                         2013.01.28: Fixed to be terminated NULL str1 in
                                     net_strcpy(), net_strcat()
 ***************************************************************************/

#include "kernel.h"
#include "net_strlib.h"

W net_strcmp(const char *str1, const char *str2)
{
    W c=0;
    while ((*str1) || (*str2)) {
        c = *str1 - *str2;
        if (c) {
            break;
        }
        str1++;
        str2++;
    }
    return c;
}

char* net_strcpy(char *str1, const char *str2)
{
    char *t = str1;
    do {
        *str1 = *str2;
        str1++;
    }while(*str2++);
    return t;
}

UW net_strlen(const char *str)
{
    UW l=0;
    while (*str) {
        str++;
        l++;
    }
    return l;
}

char* net_strcat(char *str1, const char *str2)
{
    char *t = str1;
    while (*str1) str1++;
    net_strcpy(str1, str2);
    return t;
}

char* net_strchr(const char *str, int ch)
{
    while (*str) {
        if (*str == (char)ch) {
            return (char*)str;
        }
        str++;
    }
    if (*str == ch) {
        return (char*)str;
    }
    return (char*)0;
}

#define IS_UPPER(c)   ('A' <= (c) && (c) <= 'Z')
#define TO_LOWER(c)   (IS_UPPER(c) ? ((c) + 0x20) : (c))
W net_strncasecmp(const char *str1, const char *str2, W len)
{
    W c1=0, c2=0;
    while ((len > 0) && ((*str1) || (*str2))) {
        c1 = TO_LOWER(*str1);
        c2 = TO_LOWER(*str2);
        if (c1 != c2) {
            break;
        }
        len--;
        str1++;
        str2++;
    }
    return (c1-c2);
}

