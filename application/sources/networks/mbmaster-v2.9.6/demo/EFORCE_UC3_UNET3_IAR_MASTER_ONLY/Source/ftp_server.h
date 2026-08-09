/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    FTP Server header file
    Copyright (c)  2008-2014, eForce Co., Ltd. All rights reserved.

    Version Information  2008.07.22: Created
                         2010.07.30: Updated for IPv6 support
 ***************************************************************************/

#ifndef FTPD_H
#define FTPD_H
#ifdef __cplusplus
extern "C" {
#endif

/* FTP Configurables */
#define FTP_BUFSZ           1024    /* FTP send/recv buffer size       */
#define FTP_CON_TMO         5000    /* FTP socket connection timeout   */
#define FTP_SND_TMO         5000    /* FTP socket send timeout         */
#define FTP_RCV_TMO         5000    /* FTP socket recv timeout         */
#define FTP_IDLE_TMO        15000   /* FTP Idle timeout                */ 
#define FTP_FILE_NAME_LEN   256     /* FTP maximum length of file name */

/* FTP Macros */
#define FTP_DATA_PORT       20      /* FTP Data socket port    */
#define FTP_CTRL_PORT       21      /* FTP Control socket port */

/* FTP Commands */
enum {
FTP_CMD_NONE,
FTP_CMD_USER,
FTP_CMD_PASS,
FTP_CMD_QUIT,
FTP_CMD_PORT,
FTP_CMD_PASV,
FTP_CMD_EPSV,
FTP_CMD_LIST,
FTP_CMD_TYPE,
FTP_CMD_STOR,
FTP_CMD_RETR,
FTP_CMD_SIZE,
FTP_CMD_ABOR,
FTP_CMD_REST,
FTP_CMD_EPRT,
FTP_CMD_ERR
};

/* FTP Internals */

/* FTP File Structure */
typedef struct t_ftp_file {
    UW maxsz;       /* Maximum file size    */
    UW size;        /* File Size in bytes   */
    UW offset;      /* Read or Write offset */
    UB *file;       /* File storage offset  */
    UB fname[FTP_FILE_NAME_LEN];  /* File name            */
}T_FTP_FILE;

/* FTP Control structure */
typedef struct t_ftp_ctl {
    UH cmd;
    UH sid1;
    UH sid2;
    UH port;
    UW ipaddr;
#ifdef IPV6_SUP
    UW ip6addr[4];
#endif
    UH len;
    UB con;
    UB ver;         /* To support IPV6 */
    UW pos;
    char *str;
}T_FTP_CTL;

/* FTP User Interface */

typedef struct t_ftp_server {
    UB dev_num;     /* Network Interface to be used */
    UB ver;         /* IP version */
    UH ctl_socid;   /* FTP Control socket ID    */
    UH data_socid;  /* FTP Data socket ID       */
    UB *fs_file;    /* FTP file buffer          */
    UW fs_maxsz;    /* FTP file buffer size     */
}T_FTP_SERVER;

/* FTP API */
ER ftp_server(T_FTP_SERVER *ftp);

#ifdef __cplusplus
}
#endif
#endif /* FTPD_H */
