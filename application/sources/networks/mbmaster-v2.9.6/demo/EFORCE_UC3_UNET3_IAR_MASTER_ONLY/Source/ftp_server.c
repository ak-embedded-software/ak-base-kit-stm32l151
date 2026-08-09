/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    FTP Server
    Copyright (c)  2008-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2008.07.22: Created
                         2010.06.29: Updated for IPv6 support
                         2010.07.30: Updated for IPv6 support (EPRT command)
                         2012.10.02  Modify to avoid use of string libraries.
 ***************************************************************************/

#include <stdlib.h> /* atoi   */

#include "kernel.h"
#include "net_hdr.h"
#include "net_strlib.h"

#include "ftp_server.h"

/* FTP Error Messages */
const char ftp_res_150[] = "150 About to open data connection\r\n";
const char ftp_res_200[] = "200 Okay\r\n";
const char ftp_res_202[] = "202 Command not supported\r\n";
const char ftp_res_220[] = "220 Connected to FTP Server\r\n";
const char ftp_res_221[] = "221 GoodBye\r\n";
const char ftp_res_226[] = "226 Transfer okay\r\n";
const char ftp_res_230[] = "230 User Logged in\r\n";
const char ftp_res_331[] = "331 User name okay, need password\r\n";
const char ftp_res_425[] = "425 Can't open data connection\r\n";
const char ftp_res_426[] = "426 Transfer aborted\r\n";
const char ftp_res_450[] = "450 Action not taken. File Busy\r\n";
const char ftp_res_452[] = "452 Action not taken. No resource\r\n";
const char ftp_res_500[] = "500 Syntax error, command unrecognized\r\n";
const char ftp_res_501[] = "501 Syntax error in arguments\r\n";
const char ftp_res_503[] = "503 Bad sequence of commands\r\n";
const char ftp_res_504[] = "504 Command not implemented for this parameter\r\n";
const char ftp_res_530[] = "530 Not logged in\r\n";
const char ftp_res_550[] = "550 Action not taken. File not available\r\n";
const char ftp_res_553[] = "553 Action not taken. File name not allowed\r\n";

const char ftp_mode[] = "-rw-r--r- 1 ftp ftp    ";

/* FTP Global Resources */
static T_FTP_FILE ftpfs;
static T_FTP_CTL ftp_ctl_buf;
static T_NODE ftp_host, ftp_dest;

static UW ftp_rcv_buf[FTP_BUFSZ/4];
static UW ftp_snd_buf[FTP_BUFSZ/4];
static char ftp_msg_buf[256];
static char tmp[16];

/* FTP Socket Callback */
#define FTP_FLG_CON     0x01
static UB ftp_flg;
static ID ftp_tskid;
static ER ftp_con_error;

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

UW ftp_cbk(UH sid, UH fncd, ER ercd)
{
    if (fncd & EV_SOC_CON) {
        if (ftp_flg & FTP_FLG_CON) {
            ftp_con_error = ercd;
            ftp_flg &= ~(UB)(FTP_FLG_CON);
            wup_tsk(ftp_tskid);
        }
    }
    return E_OK;
}

/*
    Assign memory to use for FTP File Transfer
*/
void ftp_file_ini(T_FTP_SERVER *ftp)
{
    net_strcpy((char *)ftpfs.fname, "");
    ftpfs.file  = ftp->fs_file;
    ftpfs.maxsz = ftp->fs_maxsz;
    ftpfs.size  = 0;
    ftpfs.offset = 0;
}

/*
    Transfer File to Remote
*/
static ER ftp_retr_cmd(T_FTP_SERVER *ftp)
{
    UW snd_len, tot_len, len;
    UB *data;
    ER ercd;

    data = ftpfs.file + ftpfs.offset;
    snd_len = 0;
    tot_len = ftpfs.size - ftpfs.offset;

    while (tot_len) {
        len = (tot_len > 1460) ? 1460 : tot_len;
        ercd = snd_soc(ftp->data_socid, (VP)(data + snd_len), (UH)len);
        if (ercd <= 0) {
            break;
        }
        snd_len += ercd;
        tot_len -= ercd;
    }

    return snd_len;
}

/*
    Receive File from Remote
*/
static ER ftp_stor_cmd(T_FTP_SERVER *ftp)
{
    UW rcv_len;
    UB *data;
    ER ercd;

    data = ftpfs.file;
    ftpfs.size = 0;

    rcv_len = 0;
    for (;;) {
        ercd = rcv_soc(ftp->data_socid, (VP)(data + rcv_len), FTP_BUFSZ);
        if (ercd > 0) {
            rcv_len += ercd;
            if (rcv_len > ftpfs.maxsz) {
                break;
            }
        }
        else {
            break;
        }
    }

    ftpfs.size = rcv_len;

    return rcv_len;
}

/*
    Parse the FTP Arguments
*/
static ER ftp_parse_cmd(T_FTP_CTL *ftpctl, char *buf)
{
    char *str;
    UB tmp;

    ftpctl->str = NULL;
    if (net_strncasecmp(buf, "USER",4) == 0) {
        ftpctl->cmd = FTP_CMD_USER;
    }
    else if (net_strncasecmp(buf, "PASS",4) == 0) {
        ftpctl->cmd = FTP_CMD_PASS;
    }
    else if (net_strncasecmp(buf, "QUIT",4) == 0) {
        ftpctl->cmd = FTP_CMD_QUIT;
    }
    else if (net_strncasecmp(buf, "TYPE",4) == 0) {
        ftpctl->cmd = FTP_CMD_TYPE;
    }
    else if (net_strncasecmp(buf, "REST",4) == 0) {
        ftpctl->cmd = FTP_CMD_REST;
        buf += 5;
        ftpctl->pos = atol(buf);
    }
    else if (net_strncasecmp(buf, "LIST",4) == 0) {
        ftpctl->cmd = FTP_CMD_LIST;
    }
    else if (net_strncasecmp(buf, "PORT",4) == 0) {
        ftpctl->cmd = FTP_CMD_PORT;
        str = buf + 5;
        ftpctl->ipaddr = 0;
        ftpctl->port   = 0;

        tmp = atoi(str);
        ftpctl->ipaddr += tmp << 24;
        str = net_strchr(str, ',');
        if (str == NULL)    return E_PAR;
        str++;

        tmp = atoi(str);
        ftpctl->ipaddr += tmp << 16;
        str = net_strchr(str, ',');
        if (str == NULL)    return E_PAR;
        str++;

        tmp = atoi(str);
        ftpctl->ipaddr += tmp << 8;
        str = net_strchr(str, ',');
        if (str == NULL)    return E_PAR;
        str++;

        tmp = atoi(str);
        ftpctl->ipaddr += tmp;
        str = net_strchr(str, ',');
        if (str == NULL)    return E_PAR;
        str++;

        tmp = atoi(str);
        ftpctl->port += tmp << 8;
        str = net_strchr(str, ',');
        if (str == NULL)    return E_PAR;
        str++;

        tmp = atoi(str);
        ftpctl->port += tmp;

    }
#ifdef IPV6_SUP
    else if (net_strncasecmp(buf,"EPRT",4)==0) {
        ftpctl->cmd=FTP_CMD_EPRT;
        ftpctl->ip6addr[0]=0;
        ftpctl->port=0;

        str=buf+5;
        str=net_strchr(str,'|');
        if(str==NULL) return E_PAR;
        str++;

        if(net_strncasecmp(str,"2",1)==0) {
            str=net_strchr(str,'|');
            if(str==NULL) return E_PAR;
            str++;
            ip6_aton(str, ftpctl->ip6addr);
        
            str = net_strchr(str,'|');
            if (str == NULL)    return E_PAR;
            str++;
            ftpctl->port = atoi(str);

            str = net_strchr(str, '|');
            if (str == NULL)    return E_PAR; 
            str++;      
        }    
    }
#endif
    else if (net_strncasecmp(buf, "PASV",4) == 0) {
        ftpctl->cmd = FTP_CMD_PASV;
    }
    else if (net_strncasecmp(buf, "EPSV",4) == 0) {
        ftpctl->cmd = FTP_CMD_EPSV;
    }
    else if (net_strncasecmp(buf, "SIZE",4) == 0) {
        ftpctl->cmd = FTP_CMD_SIZE;
        *(buf + ftpctl->len-2) = '\0';
        ftpctl->str = buf + 5; /* file name */
    }
    else if (net_strncasecmp(buf, "STOR",4) == 0) {
        ftpctl->cmd = FTP_CMD_STOR;
        *(buf + ftpctl->len-2) = '\0';
        ftpctl->str = buf + 5; /* file name */
    }
    else if (net_strncasecmp(buf, "RETR",4) == 0) {
        ftpctl->cmd = FTP_CMD_RETR;
        *(buf + ftpctl->len-2) = '\0';
        ftpctl->str = buf + 5; /* file name */

    }
    else if (net_strncasecmp(buf, "ABOR",4) == 0) {
        ftpctl->cmd = FTP_CMD_ABOR;
    }
    else {
        ftpctl->cmd = FTP_CMD_NONE;
    }

    return E_OK;
}

ER ftp_server(T_FTP_SERVER *ftp)
{
    T_FTP_CTL *ftpctl;
    const char *ftp_res;
    ID ftp_sid1, ftp_sid2;
    UB cls, ftp_action;
    UH ftp_res_len;
    ER ercd;

    if (ftp == NULL) {
        return E_PAR;
    }

#ifdef NET_C
    if (ftp->ctl_socid == 0 || ftp->data_socid == 0) {
        return E_PAR;
    }
#endif

    if (ftp->fs_file == NULL || ftp->fs_maxsz == 0) {
        return E_PAR;
    }

    ftp_file_ini(ftp);

    ftp_sid1 = 0;
    ftp_sid2 = 0;
    get_tid(&ftp_tskid);

#ifndef NET_C
    /* Create FTP Socket (21)   */
    ftp_host.num  = ftp->dev_num;
    ftp_host.ver  = ftp->ver;
#ifdef IPV6_SUP
    if(ftp_host.ver == IP_VER6)
        net_memset(ftp_host.ip6a, 0, 16);
    else
#endif
    ftp_host.ipa  = INADDR_ANY;
    ftp_host.port = FTP_CTRL_PORT;
    ercd = cre_soc(IP_PROTO_TCP, &ftp_host);
    if (ercd <= 0) {
        return ercd;
    }
    ftp_sid1 = ercd;

    /* Set Timeout's */
    ercd = cfg_soc(ftp_sid1, SOC_TMO_SND, (VP)FTP_SND_TMO);
    ercd = cfg_soc(ftp_sid1, SOC_TMO_RCV, (VP)FTP_IDLE_TMO);
    ercd = cfg_soc(ftp_sid1, SOC_TMO_CLS, (VP)FTP_SND_TMO);
#else
    ftp_sid1 = ftp->ctl_socid;
#endif
    ftpctl = &ftp_ctl_buf;
    ftpctl->con = 0;

    for (;;) {

        /* Wait for FTP Client      */

        ercd = con_soc(ftp_sid1, &ftp_host, SOC_SER);
        if (ercd != E_OK) {
            cls_soc(ftp_sid1, 0);
            continue;
        }

        /* Send Welcome Message to Client */
        ftp_res = ftp_res_220;
        ftp_res_len = net_strlen(ftp_res_220);
        ercd = snd_soc(ftp_sid1, (VP)ftp_res, ftp_res_len);
        if (ercd <= 0) {
            cls_soc(ftp_sid1, 0);
            continue;
        }

        for (;;) {

            /* Wait for Client Request */

            ercd = rcv_soc(ftp_sid1, (VP)&ftp_rcv_buf[0], FTP_BUFSZ);
            if (ercd <= 0) {
                break;
            }
            ftpctl->len = ercd;

            /* Parse the FTP commands */

            ercd = ftp_parse_cmd(ftpctl, (char*)&ftp_rcv_buf[0]);
            if (ercd != E_OK) {
                /* Command Syntax Error */
                ftpctl->cmd = FTP_CMD_ERR;
            }

            /* Execute the commands */
            ftp_action = 0;
            cls = 0;
            switch (ftpctl->cmd) {

                case FTP_CMD_USER:
                    ftp_res     = ftp_res_331;
                    ftp_res_len = net_strlen(ftp_res_331);
                    break;

                case FTP_CMD_PASS:
                    ftp_res     = ftp_res_230;
                    ftp_res_len = net_strlen(ftp_res_230);
                    break;

                case FTP_CMD_QUIT:
                    ftp_res     = ftp_res_221;
                    ftp_res_len = net_strlen(ftp_res_221);
                    cls = 1;
                    break;

                case FTP_CMD_TYPE:
                    ftp_res     = ftp_res_200;
                    ftp_res_len = net_strlen(ftp_res_200);
                    break;

                case FTP_CMD_SIZE:
                    ftp_res     = ftp_res_550;
                    ftp_res_len = net_strlen(ftp_res_550);
                    if (ftpfs.size == 0) {
                        break;
                    }
                    if (net_strcmp(ftpctl->str,(char*)ftpfs.fname) != 0) {
                        break;
                    }
                    net_strcpy(ftp_msg_buf, "213 ");
                    itoa_std(ftpfs.size, tmp);
                    net_strcat(ftp_msg_buf, tmp);
                    net_strcat(ftp_msg_buf, "\r\n");
                    /*sprintf(ftp_msg_buf,"213 %d\r\n",ftpfs.size);*/
                    ftp_res = ftp_msg_buf;
                    ftp_res_len = net_strlen(ftp_msg_buf);
                    break;

                case FTP_CMD_REST:
                    if (ftpctl->pos < ftpfs.size) {
                        ftpfs.offset = ftpctl->pos;
                        net_strcpy(ftp_msg_buf, "350 Restarting at ");
                        itoa_std(ftpctl->pos, tmp);
                        net_strcat(ftp_msg_buf, tmp);
                        net_strcat(ftp_msg_buf, ".Send STORE or RETRIEVE\r\n");
                        /*sprintf(ftp_msg_buf,"350 Restarting at %d.Send STORE or RETRIEVE\r\n",ftpctl->pos);*/
                        ftp_res = ftp_msg_buf;
                        ftp_res_len = net_strlen(ftp_msg_buf);
                    }
                    else {
                        ftp_res     = ftp_res_452;
                        ftp_res_len = net_strlen(ftp_res_452);
                    }
                    break;

                case FTP_CMD_PORT:
                    if (ftp_sid2 != 0) {
                        cls_soc(ftp_sid2, 0);
                        ftp_flg &= ~(UB)(FTP_FLG_CON);
#ifndef NET_C
                        del_soc(ftp_sid2);
#endif
                        ftp_sid2 = 0;
                        ftpctl->con = 0;
                    }
                    ftpfs.offset = 0;
                    ftpctl->con = 1;
                    ftp_dest.port = ftpctl->port;
                    ftp_dest.ipa  = ftpctl->ipaddr;
                    ftp_res = ftp_res_200;
                    ftp_res_len = net_strlen(ftp_res_200);
                    break;
#ifdef IPV6_SUP                    
                    case FTP_CMD_EPRT:
                    if (ftp_sid2 != 0) {
                        cls_soc(ftp_sid2, 0);
                        ftp_flg &= ~(UB)(FTP_FLG_CON);
#ifndef NET_C
                        del_soc(ftp_sid2);
#endif
                        ftp_sid2 = 0;
                        ftpctl->con = 0;
                    }
                    ftpfs.offset = 0;
                    ftpctl->con = 1;
                    ftp_dest.port = ftpctl->port;
                    ip6_addr_cpy(&ftp_dest.ip6a[4],&ftpctl->ip6addr[4]);
                    ftp_res = ftp_res_200;
                    ftp_res_len = net_strlen(ftp_res_200);
                    break;
#endif         
                case FTP_CMD_EPSV:
                    if (ftp_sid2 != 0) {
                        cls_soc(ftp_sid2, 0);
                        ftp_flg &= ~(UB)(FTP_FLG_CON);
#ifndef NET_C
                        del_soc(ftp_sid2);
#endif
                        ftp_sid2 = 0;
                        ftpctl->con = 0;
                    }
                    ftpfs.offset = 0;
                    ftp_dest.num = ftp->dev_num;
                    ftp_dest.port= PORT_ANY; /* Port any: 5133 */
#ifdef IPV6_SUP
                    if((ftp_dest.ver = ftp->ver) == IP_VER6)
                        net_memset(ftp_dest.ip6a, 0, 16);
                    else
#endif
                    ftp_dest.ipa = INADDR_ANY;
#ifndef NET_C
                    ercd = cre_soc(IP_PROTO_TCP, &ftp_dest);
                    ftp->data_socid = ercd;
#else
                    ercd = ftp->data_socid;
#endif
                    if (ercd > 0) {
                        ftp_sid2 = ercd;
#ifndef NET_C
                        /* Set Socket Timeout's */
                        ercd = cfg_soc(ftp_sid2, SOC_TMO_CON, (VP)FTP_CON_TMO);
                        ercd = cfg_soc(ftp_sid2, SOC_TMO_SND, (VP)FTP_SND_TMO);
                        ercd = cfg_soc(ftp_sid2, SOC_TMO_RCV, (VP)FTP_RCV_TMO);
                        ercd = cfg_soc(ftp_sid2, SOC_TMO_CLS, (VP)FTP_SND_TMO);
#endif
                        ercd = cfg_soc(ftp_sid2, SOC_CBK_HND, (VP)&ftp_cbk);
                        ercd = cfg_soc(ftp_sid2, SOC_CBK_FLG, (VP)EV_SOC_CON);
                        ftp_flg |= FTP_FLG_CON;
                        ercd = con_soc(ftp_sid2, &ftp_dest, SOC_SER);
                        if (ercd != E_WBLK) {
                            ftp_flg &= ~(UB)(FTP_FLG_CON);
                            cls_soc(ftp_sid2,0);
                            ftp_flg &= ~(UB)(FTP_FLG_CON);
#ifndef NET_C
                            del_soc(ftp_sid2);
#endif
                            ftp_sid2 = 0;
                        }
                    }

                    if (ftp_sid2 == 0) {
                        ftp_res = ftp_res_452;  /* Resources Error */
                        ftp_res_len = net_strlen(ftp_res_452);
                    }
                    else {
                        net_strcpy(ftp_msg_buf, "229 Entering Extended Passive Mode (|||");
                        itoa_std(ftp_dest.port, tmp);
                        net_strcat(ftp_msg_buf, tmp);
                        net_strcat(ftp_msg_buf, "|)\r\n");
                        /*sprintf(ftp_msg_buf, "229 Entering Extended Passive Mode (|||%d|)\r\n",ftp_dest.port);*/
                        ftp_res = ftp_msg_buf;
                        ftp_res_len = net_strlen(ftp_msg_buf);
                        ftpctl->con = 2;
                    }
                    break;

                case FTP_CMD_RETR:
                    ftp_res     = ftp_res_550;
                    ftp_res_len = net_strlen(ftp_res_550);
                    if (ftpfs.size == 0) {
                        break;
                    }
                    if (net_strcmp(ftpctl->str,(char*)ftpfs.fname) != 0) {
                        break;
                    }
                case FTP_CMD_STOR:
                case FTP_CMD_LIST:
                    if (ftpctl->con == 2) {
                        ftp_action = 1;
                        ftp_res     = ftp_res_150;          /* about to open data connection */
                        ftp_res_len = net_strlen(ftp_res_150);
                        break;
                    }
                    ftp_host.port = FTP_DATA_PORT;
                    ftp_res     = ftp_res_425;
                    ftp_res_len = net_strlen(ftp_res_425);
#ifndef NET_C
                    ercd = cre_soc(IP_PROTO_TCP, &ftp_host);
                    if (ercd <= 0) {
                        break;
                    }
                    ftp->data_socid = ercd;
#else
                    ercd = ftp->data_socid;
#endif
                    ftp_sid2 = ercd;
                    ftp_dest.num  = ftp->dev_num;
                    ftp_dest.ver  = ftp->ver;
                    ftp_dest.port = ftpctl->port;
#ifdef IPV6_SUP
                    if(ftp_dest.ver == IP_VER6)
                        ip6_addr_cpy(ftp_dest.ip6a, ftpctl->ip6addr); 
#endif                    
                    ftp_dest.ipa  = ftpctl->ipaddr;
                    ercd = con_soc(ftp_sid2, &ftp_dest, SOC_CLI);
                    if (ercd != E_OK) {
                        break;
                    }
                    ftp_action = 1;
                    ftp_res     = ftp_res_150;          /* about to open data connection */
                    ftp_res_len = net_strlen(ftp_res_150);
                    break;

                case FTP_CMD_ABOR:
                    if (ftp_sid2) {
                        cls_soc(ftp_sid2, 0);
                        ftp_flg &= ~(UB)(FTP_FLG_CON);
#ifndef NET_C
                        del_soc(ftp_sid2);
#endif
                        ftp_sid2 = 0;
                        ftpctl->con = 0;
                    }
                    ftp_res     = ftp_res_226;
                    ftp_res_len = net_strlen(ftp_res_226);
                    break;

                case FTP_CMD_ERR:
                    ftp_res     = ftp_res_501;
                    ftp_res_len = net_strlen(ftp_res_501);
                    break;
                default:
                    ftp_res     = ftp_res_500;
                    ftp_res_len = net_strlen(ftp_res_500);
                    break;
            }

            ercd = snd_soc(ftp_sid1, (VP)ftp_res, ftp_res_len);
            if (ercd <=0) {
                break;
            }

            if (ftp_action) {

                /* Make or Wait for Connection  */
                if (ftpctl->con == 2) {
                    ercd = tslp_tsk(FTP_CON_TMO);
                    if (ercd == E_OK) {
                        ercd = ftp_con_error;
                    }
                    if (ercd != E_OK) {
                        ftp_flg &= ~(UB)(FTP_FLG_CON);
                        ercd = snd_soc(ftp_sid1, (VP)ftp_res_425, net_strlen(ftp_res_425));
                        cls_soc(ftp_sid2,0);
#ifndef NET_C
                        del_soc(ftp_sid2);
#endif
                        continue;
                    }
                }

                /* Transfer/Receive Data        */
                switch (ftpctl->cmd) {
                    case FTP_CMD_LIST:
                        if (ftpfs.size) {
                            net_strcpy((char *)ftp_snd_buf, ftp_mode);
                            itoa_std(ftpfs.size,tmp);
                            net_strcat((char *)ftp_snd_buf, tmp);
                            net_strcat((char *)ftp_snd_buf, " ");
                            net_strcat((char *)ftp_snd_buf, (char const *)ftpfs.fname);
                            net_strcat((char *)ftp_snd_buf, "\r\n");
                            /*sprintf((char*)ftp_snd_buf, "%s %d %s\r\n",ftp_mode,ftpfs.size,ftpfs.fname);*/
                            ercd = net_strlen((char*)ftp_snd_buf);
                            snd_soc(ftp_sid2, (VP)&ftp_snd_buf[0], ercd);
                        }
                        break;
                    case FTP_CMD_RETR:
                        ftp_retr_cmd(ftp);
                        break;
                    case FTP_CMD_STOR:
                        net_strcpy((char*)ftpfs.fname,ftpctl->str);
                        ftp_stor_cmd(ftp);
                        break;
                    default:
                        break;
                }
                /* Close the connection         */
                ercd = snd_soc(ftp_sid1, (VP)ftp_res_226, net_strlen(ftp_res_226));
                cls_soc(ftp_sid2, 0);
                ftp_flg &= ~(UB)(FTP_FLG_CON);
#ifndef NET_C
                del_soc(ftp_sid2);
#endif
                ftp_sid2 = 0;
            }

            if (cls) {
                break;
            }

        }

        cls_soc(ftp_sid1, 0);
        if (ftp_sid2) {
            cls_soc(ftp_sid2, 0);
            ftp_flg &= ~(UB)(FTP_FLG_CON);
#ifndef NET_C
            del_soc(ftp_sid2);
#endif
            ftp_sid2 = 0;
        }
    }
}
