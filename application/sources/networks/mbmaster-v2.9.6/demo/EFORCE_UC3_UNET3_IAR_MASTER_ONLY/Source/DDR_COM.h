/***********************************************************************
    MICRO C CUBE / COMPACT,DEVICE DRIVER
    Standard Communication Interface
    Copyright (c)  2008-2009, eForce Co., Ltd. All rights reserved.

    Version Information  2008.03.31: Created
                         2009.01.13: Corrected definitions
                         2012.01.23: Add include guard.
 ***********************************************************************/

#ifndef _DDR_COM_H_
#define _DDR_COM_H_

/* Definitions of control character                                     */

#define     XON         0x11
#define     XOFF        0x13

/* Definitions of chatacter length                                      */

#define     BLEN8       0       /* 8-bits length                        */
#define     BLEN7       1       /* 7-bits length                        */
#define     BLEN6       2       /* 6-bits length                        */
#define     BLEN5       3       /* 5-bits length                        */

/* Definitions of parity bit                                            */

#define     PAR_NONE    0       /* None parity                          */
#define     PAR_EVEN    1       /* Even parity                          */
#define     PAR_ODD     2       /* Odd                                  */

/* Definitions of stop bit length                                       */

#define     SBIT1       0       /* 1 stop bit                           */
#define     SBIT15      1       /* 1.5 stop bits                        */
#define     SBIT2       2       /* 2 stop bits                          */

/* Definitions of flow control                                          */

#define     FLW_NONE    0       /* None flow control                    */
#define     FLW_XON     1       /* Software flow control                */
#define     FLW_HARD    2       /* Hardware flow control                */

/* Definitions of command code                                          */

#define     RST_COM     0xF800  /* Reset COM Port                       */
#define     CLN_TXBUF   0x8000  /* Clean Tx Buffer                      */
#define     RST_BUF     0x6000  /* Reset both Rx and Tx data buffers    */
#define     RST_TXBUF   0x4000  /* Reset Tx data buffer                 */
#define     RST_RXBUF   0x2000  /* Reset Rx data and status Buffer      */
#define     STP_COM     0x1800  /* Stop both receiver and transmitter   */
#define     STP_TX      0x1000  /* Stop transmitter                     */
#define     STP_RX      0x0800  /* Stop receiver                        */
#define     SND_BRK     0x0400  /* Send line BREAK                      */
#define     STA_COM     0x0300  /* Start both receiver and transmitter  */
#define     STA_TX      0x0200  /* Start transmitter                    */
#define     STA_RX      0x0100  /* Start receiver                       */
#define     LOC_TX      0x0080  /* Lock transmitter                     */
#define     LOC_RX      0x0040  /* Lock receiver                        */
#define     UNL_TX      0x0020  /* Unlock transmitter                   */
#define     UNL_RX      0x0010  /* Unlock receiver                      */
#define     ASR_RTS     0x0008  /* Assert RTS                           */
#define     NGT_RTS     0x0004  /* Negate RTS                           */
#define     ASR_DTR     0x0002  /* Assert DTR                           */
#define     NGT_DTR     0x0001  /* Negate DTR                           */

/* Definitions of control code                                          */

#define     TA_COM_INI  1       /* Initialize communication port        */
#define     TA_COM_REF  2       /* Rrefarence communication port        */
#define     TA_COM_CTR  3       /* Control communication port           */
#define     TA_COM_PUTS 4       /* Transmit character stringth          */
#define     TA_COM_GETS 5       /* Receive character stringth           */

/* Definitions of status code                                           */

#define     T_COM_EROVB 0x0001  /* COM FIFO Overrun error               */
#define     T_COM_EROR  0x0002  /* Rx Buffer Overflow error             */
#define     T_COM_ERP   0x0004  /* Parity error                         */
#define     T_COM_ERF   0x0008  /* Framing error                        */
#define     T_COM_BRK   0x0010  /* line break status                    */
#define     T_COM_TXOFF 0x0020  /* Tx XON/XOFF flow control status      */
#define     T_COM_RXOFF 0x0040  /* Rx XON/XOFF flow control status      */
#define     T_COM_RTS   0x0080  /* RTS control status                   */
#define     T_COM_CTS   0x0100  /* CTS control status                   */
#define     T_COM_DTR   0x0200  /* DTR control status                   */
#define     T_COM_DSR   0x0400  /* DSR control status                   */
#define     T_COM_CD    0x0800  /* CD  control status                   */
#define     T_COM_RI    0x1000  /* RI  control status                   */
#define     T_COM_ENARX 0x2000  /* 1= Rx Enable, 0= Rx Disable          */
#define     T_COM_ENATX 0x4000  /* 1= Tx Enable, 0= Tx Disable          */
#define     T_COM_INIT  0x8000  /* 1= Port Initialized                  */

typedef struct t_com_smod {
    UW          baud;
    UB          blen;
    UB          par;
    UB          sbit;
    UB          flow;
} T_COM_SMOD;

typedef struct t_com_ref {
    UH          rxcnt;
    UH          txcnt;
    UH          status;
} T_COM_REF;

typedef struct t_com_ctr {
    UH          command;
    TMO         time;
} T_COM_CTR;

typedef struct t_com_snd {
    VB const    *tbuf;
    TMO         time;
    UH          tcnt;
} T_COM_SND;

typedef struct t_com_eos {
    UB          flg[4];
    VB          chr[4];
} T_COM_EOS;

typedef struct t_com_rcv {
    VB          *rbuf;
    UB          *sbuf;
    T_COM_EOS   *eos;
    TMO         time;
    UH          rcnt;
} T_COM_RCV;

ER ini_com(ID, const T_COM_SMOD *);
ER ref_com(ID, T_COM_REF *);
ER ctr_com(ID, UH, TMO);
ER putc_com(ID, VB, TMO);
ER puts_com(ID, VB const *, UINT *, TMO);
ER getc_com(ID, VB *, UB *, TMO);
ER gets_com(ID, VB *, UB *, INT, UINT *, TMO);

#endif /* _DDR_COM_H_ */
