/***********************************************************************
    Micro C Cube Standard, DEVICE DRIVER
    Serial Interface MB9BFxxxx UART

    Copyright (c)  2010, eForce Co., Ltd. All rights reserved.

    Version Information  
            2010.11.18: Created
************************************************************************/

#ifndef DDR_MB9BF_UART_H
#define DDR_MB9BF_UART_H

typedef union t_mb9_uart_msts {
    UH      word;
    struct {
        UH      init_flg:1;
        UH      ena_tx:1;
        UH      ena_rx:1;
        UH      sft_flw:1;
        UH      hrd_flw:1;
        UH      sns_brk:1;
        UH      tx_xoff:1;
        UH      rx_xoff:1;
        UH      req_xon_xoff:1;
        UH      er_buf_ovr:1;
        UH      cln_wait:1;
        UH      dummy:5;
    } bit;
} T_MB9_UART_MSTS;


typedef struct t_mb9_uart_cmng {
    VB          *tbuf;
    VB          *rbuf;
    UB          *sbuf;
    UW          pclock;
    UH          tsize;
    UH          rsize;
    UH          xoff_size;
    UH          xon_size;
    UH          tsemid;
    UH          rsemid;
    UH          tintno;
    UH          rintno;
} T_MB9_UART_CMNG;


typedef struct t_mb9_uart_dmng {
    T_COM_SND           *SndData;
    T_COM_RCV           *RcvData;
    T_MB9_UART_MSTS     status;
    UH                  sndp;
    UH                  rcvp;
    UH                  tcnt;
    UH                  rcnt;
    UH                  tlockid;
    UH                  rlockid;
    UB                  ssr;
    UB                  msr;
    UB                  fcr0;
    UB                  fcr1;
    UB                  fbyte1;
    UB                  fbyte2;
} T_MB9_UART_DMNG;


typedef struct t_mb9_uart_mng {
    volatile struct t_mfs      *port;
    T_MB9_UART_CMNG const   *cdata;
    T_MB9_UART_DMNG         *data;
} T_MB9_UART_MNG;

#endif

