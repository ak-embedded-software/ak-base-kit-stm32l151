/***************************************************************************
    Micro C Cube Compact, DEVICE DRIVER
    Serial Interface MB9BFxxx UART

    Copyright (c)  2010-2014, eForce Co., Ltd. All rights reserved.

    Version Information  
            2010.11.18: Created
            2011.04.12: Addition of the GPIO configuration
            2011.08.29: _ddr_mb9_uart_init() modified for MB9B310T,MB9B610T
            2011.12.27: SCK** pin is not configured in _ddr_mb9_uart_init()
            2012.03.06: _ddr_mb9_uart_init() modified for MB9B210T
            2012.05.08: _ddr_mb9_uart_init() correct in UART6,7 Relocate2
            2012.06.27: Corrected the error code of the time out.
            2012.07.11: _ddr_mb9_uart_init() modified for MB9AB40N, MB9AA40N
                        , MB9A340N, MB9A140N.
            2012.07.18: _ddr_mb9_uart_init() correct in REG_GPIO.ADE setting.
            2012.08.29: Corrected the channel closing.
            2012.09.14: _ddr_mb9_uart_init() modified for MB9B510R, MB9B410R
                        , MB9B310R, MB9B110R.
            2013.02.05: _ddr_mb9_uart_init() modified for MB9Aseries define.
            2014.01.23: _ddr_mb9_uart_init() and Initialize modified for
                        MB9A130K,L,M,N series and MB9AA30L,M,N series.
                        (FM3_SRS == 10200,10210,10220,10230,10240,10250,10260)
            2014.02.17: _ddr_mb9_uart_init() and Initialize modified for
                        MB9B560M,N,R : MB9B460M,N,R : MB9B360M,N,R : MB9B160M,N,R series.
                        (FM3_SRS == 10500,10510,10520,10600,10610,10620,
                         10700,10710,10720,10800,10810,10820)
            2014.06.19: add wisreed series.
            2014.07.04: _ddr_mb9_uart_init() support FIFO wisreed series.
            2014.07.15: support LPMODE driver.
            2014.08.21  Corrected processing of reception interruption in 
                        "_ddr_mb9_uart_rxi", since there was a case where 
                        receiving status was unacquirable uses gets_com function. 
            2014.09.10:  _ddr_mb9_uart_init() and Initialize modified for
                        MB9B120M,MB9B320ML,MB9B520M series.
                        (FM3_SRS == 120,320,520)
***************************************************************************/

#include "kernel.h"
#include "kernel_id.h"

#include "DDR_COM.h"
#include "DDR_MB9BF_UART_cfg.h"
#include "DDR_MB9BF_UART.h"

#include <string.h>

/* ポート機能設定(PFR)・レジスタ */
#define PFR_0    0x0001    /* PFR0 */
#define PFR_1    0x0002    /* PFR1 */
#define PFR_2    0x0004    /* PFR2 */
#define PFR_3    0x0008    /* PFR3 */
#define PFR_4    0x0010    /* PFR4 */
#define PFR_5    0x0020    /* PFR5 */
#define PFR_6    0x0040    /* PFR6 */
#define PFR_7    0x0080    /* PFR7 */
#define PFR_8    0x0100    /* PFR8 */
#define PFR_9    0x0200    /* PFR9 */
#define PFR_A    0x0400    /* PFR10 */
#define PFR_B    0x0800    /* PFR11 */
#define PFR_C    0x1000    /* PFR12 */
#define PFR_D    0x2000    /* PFR13 */
#define PFR_E    0x4000    /* PFR14 */
#define PFR_F    0x8000    /* PFR15 */

/* アナログ入力設定レジスタ(ADE) */
#define AN00     0x00000001 /* AN00 */
#define AN01     0x00000002 /* AN01 */
#define AN02     0x00000004 /* AN02 */
#define AN03     0x00000008 /* AN03 */
#define AN04     0x00000010 /* AN04 */
#define AN05     0x00000020 /* AN05 */
#define AN06     0x00000040 /* AN06 */
#define AN07     0x00000080 /* AN07 */
#define AN08     0x00000100 /* AN08 */
#define AN09     0x00000200 /* AN09 */
#define AN10     0x00000400 /* AN10 */
#define AN11     0x00000800 /* AN11 */
#define AN12     0x00001000 /* AN12 */
#define AN13     0x00002000 /* AN13 */
#define AN14     0x00004000 /* AN14 */
#define AN15     0x00008000 /* AN15 */
#define AN16     0x00010000 /* AN16 */
#define AN17     0x00020000 /* AN17 */
#define AN18     0x00040000 /* AN18 */
#define AN19     0x00080000 /* AN19 */
#define AN20     0x00100000 /* AN20 */
#define AN21     0x00200000 /* AN21 */
#define AN22     0x00400000 /* AN22 */
#define AN23     0x00800000 /* AN23 */
#define AN24     0x01000000 /* AN24 */
#define AN25     0x02000000 /* AN25 */
#define AN26     0x04000000 /* AN26 */
#define AN27     0x08000000 /* AN27 */
#define AN28     0x10000000 /* AN28 */
#define AN29     0x20000000 /* AN29 */
#define AN30     0x40000000 /* AN30 */
#define AN31     0x80000000 /* AN31 */

/* 拡張機能設定(EPFR)・レジスタ */
#define EPF_0R    4         /* SIN0S */
#define EPF_0T    6         /* SOT0B */
#define EPF_0C    8         /* SCK0B */
#define EPF_1R    10        /* SIN1S */
#define EPF_1T    12        /* SOT1B */
#define EPF_1C    14        /* SCK1B */
#define EPF_2R    16        /* SIN2S */
#define EPF_2T    18        /* SOT2B */
#define EPF_2C    20        /* SCK2B */
#define EPF_3R    22        /* SIN3S */
#define EPF_3T    24        /* SOT3B */
#define EPF_3C    26        /* SCK3B */

#define EPF_4RT   0         /* RTS4E */
#define EPF_4CT   2         /* CTS4S */
#define EPF_4R    4         /* SIN4S */
#define EPF_4T    6         /* SOT4B */
#define EPF_4C    8         /* SCK4B */
#define EPF_5R    10        /* SIN5S */
#define EPF_5T    12        /* SOT5B */
#define EPF_5C    14        /* SCK5B */
#define EPF_6R    16        /* SIN6S */
#define EPF_6T    18        /* SOT6B */
#define EPF_6C    20        /* SCK6B */
#define EPF_7R    22        /* SIN7S */
#define EPF_7T    24        /* SOT7B */
#define EPF_7C    26        /* SCK7B */

/* 拡張機能設定(EPFR)・レジスタ, リロケート選択 */
#define PORT0 1
#define PORT1 2
#define PORT2 3

/***************************************
        UARTドライバ変数定義
 ***************************************/
#define SFIFOSIZE 16

#ifdef UART_0
#if (TXBUF_SZ0==0)
#define _ddr_mb9_uart_tbuf0    0
#else
VB  _ddr_mb9_uart_tbuf0[TXBUF_SZ0];
#endif
#if (RXBUF_SZ0==0)
#define _ddr_mb9_uart_rbuf0    0
#define _ddr_mb9_uart_sbuf0    0
#else
VB  _ddr_mb9_uart_rbuf0[RXBUF_SZ0];
UB  _ddr_mb9_uart_sbuf0[RXBUF_SZ0];
#endif


T_MB9_UART_DMNG _ddr_mb9_uart_data0;

#if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
    //FM4
T_MB9_UART_CMNG const _ddr_mb9_uart_const0 = {_ddr_mb9_uart_tbuf0, _ddr_mb9_uart_rbuf0,
            _ddr_mb9_uart_sbuf0, PCLK0, TXBUF_SZ0, RXBUF_SZ0, XOFF_SZ0, XON_SZ0,
            TXSEM0, RXSEM0, IRQ_MFS0_SEND, IRQ_MFS0_RECV};
#elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
T_MB9_UART_CMNG const _ddr_mb9_uart_const0 = {_ddr_mb9_uart_tbuf0, _ddr_mb9_uart_rbuf0,
            _ddr_mb9_uart_sbuf0, PCLK0, TXBUF_SZ0, RXBUF_SZ0, XOFF_SZ0, XON_SZ0,
            TXSEM0, RXSEM0, IRQ_TP37_MF_SIO_SEND0, IRQ_TP37_MF_SIO_RECV0};
#else
T_MB9_UART_CMNG const _ddr_mb9_uart_const0 = {_ddr_mb9_uart_tbuf0, _ddr_mb9_uart_rbuf0,
            _ddr_mb9_uart_sbuf0, PCLK0, TXBUF_SZ0, RXBUF_SZ0, XOFF_SZ0, XON_SZ0,
            TXSEM0, RXSEM0, IRQ_MF_SIO_SEND0, IRQ_MF_SIO_RECV0};
#endif

T_MB9_UART_MNG const _ddr_mb9_uart_mng0 = {(volatile struct t_mfs *)&REG_MFS0,
                            &_ddr_mb9_uart_const0,&_ddr_mb9_uart_data0};
#endif


#ifdef UART_1
#if (TXBUF_SZ1==0)
#define _ddr_mb9_uart_tbuf1    0
#else
VB  _ddr_mb9_uart_tbuf1[TXBUF_SZ1];
#endif
#if (RXBUF_SZ1==0)
#define _ddr_mb9_uart_rbuf1    0
#define _ddr_mb9_uart_sbuf1    0
#else
VB  _ddr_mb9_uart_rbuf1[RXBUF_SZ1];
UB  _ddr_mb9_uart_sbuf1[RXBUF_SZ1];
#endif


T_MB9_UART_DMNG _ddr_mb9_uart_data1;

#if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
    //FM4
T_MB9_UART_CMNG const _ddr_mb9_uart_const1 = {_ddr_mb9_uart_tbuf1, _ddr_mb9_uart_rbuf1,
            _ddr_mb9_uart_sbuf1, PCLK1, TXBUF_SZ1, RXBUF_SZ1, XOFF_SZ1, XON_SZ1,
            TXSEM1, RXSEM1, IRQ_MFS1_SEND, IRQ_MFS1_RECV};
#elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
T_MB9_UART_CMNG const _ddr_mb9_uart_const1 = {_ddr_mb9_uart_tbuf1, _ddr_mb9_uart_rbuf1,
            _ddr_mb9_uart_sbuf1, PCLK1, TXBUF_SZ1, RXBUF_SZ1, XOFF_SZ1, XON_SZ1,
            TXSEM1, RXSEM1, IRQ_TP37_MF_SIO_SEND1, IRQ_TP37_MF_SIO_RECV1};
#else
T_MB9_UART_CMNG const _ddr_mb9_uart_const1 = {_ddr_mb9_uart_tbuf1, _ddr_mb9_uart_rbuf1,
            _ddr_mb9_uart_sbuf1, PCLK1, TXBUF_SZ1, RXBUF_SZ1, XOFF_SZ1, XON_SZ1,
            TXSEM1, RXSEM1, IRQ_MF_SIO_SEND1, IRQ_MF_SIO_RECV1};
#endif

T_MB9_UART_MNG const _ddr_mb9_uart_mng1 = {(volatile struct t_mfs *)&REG_MFS1,
                            &_ddr_mb9_uart_const1,&_ddr_mb9_uart_data1};
#endif


#ifdef UART_2
#if (TXBUF_SZ2==0)
#define _ddr_mb9_uart_tbuf2    0
#else
VB  _ddr_mb9_uart_tbuf2[TXBUF_SZ2];
#endif
#if (RXBUF_SZ2==0)
#define _ddr_mb9_uart_rbuf2    0
#define _ddr_mb9_uart_sbuf2    0
#else
VB  _ddr_mb9_uart_rbuf2[RXBUF_SZ2];
UB  _ddr_mb9_uart_sbuf2[RXBUF_SZ2];
#endif


T_MB9_UART_DMNG _ddr_mb9_uart_data2;

#if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
    //FM4
T_MB9_UART_CMNG const _ddr_mb9_uart_const2 = {_ddr_mb9_uart_tbuf2, _ddr_mb9_uart_rbuf2,
            _ddr_mb9_uart_sbuf2, PCLK2, TXBUF_SZ2, RXBUF_SZ2, XOFF_SZ2, XON_SZ2,
            TXSEM2, RXSEM2, IRQ_MFS2_SEND, IRQ_MFS2_RECV};
#elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
T_MB9_UART_CMNG const _ddr_mb9_uart_const2 = {_ddr_mb9_uart_tbuf2, _ddr_mb9_uart_rbuf2,
            _ddr_mb9_uart_sbuf2, PCLK2, TXBUF_SZ2, RXBUF_SZ2, XOFF_SZ2, XON_SZ2,
            TXSEM2, RXSEM2, IRQ_TP37_MF_SIO_SEND2, IRQ_TP37_MF_SIO_RECV2};
#else
T_MB9_UART_CMNG const _ddr_mb9_uart_const2 = {_ddr_mb9_uart_tbuf2, _ddr_mb9_uart_rbuf2,
            _ddr_mb9_uart_sbuf2, PCLK2, TXBUF_SZ2, RXBUF_SZ2, XOFF_SZ2, XON_SZ2,
            TXSEM2, RXSEM2, IRQ_MF_SIO_SEND2, IRQ_MF_SIO_RECV2};
#endif

T_MB9_UART_MNG const _ddr_mb9_uart_mng2 = {(volatile struct t_mfs *)&REG_MFS2,
                            &_ddr_mb9_uart_const2,&_ddr_mb9_uart_data2};
#endif


#ifdef UART_3
#if (TXBUF_SZ3==0)
#define _ddr_mb9_uart_tbuf3    0
#else
VB  _ddr_mb9_uart_tbuf3[TXBUF_SZ3];
#endif
#if (RXBUF_SZ3==0)
#define _ddr_mb9_uart_rbuf3    0
#define _ddr_mb9_uart_sbuf3    0
#else
VB  _ddr_mb9_uart_rbuf3[RXBUF_SZ3];
UB  _ddr_mb9_uart_sbuf3[RXBUF_SZ3];
#endif


T_MB9_UART_DMNG _ddr_mb9_uart_data3;

#if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
    //FM4
T_MB9_UART_CMNG const _ddr_mb9_uart_const3 = {_ddr_mb9_uart_tbuf3, _ddr_mb9_uart_rbuf3,
            _ddr_mb9_uart_sbuf3, PCLK3, TXBUF_SZ3, RXBUF_SZ3, XOFF_SZ3, XON_SZ3,
            TXSEM3, RXSEM3, IRQ_MFS3_SEND, IRQ_MFS3_RECV};
#elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
T_MB9_UART_CMNG const _ddr_mb9_uart_const3 = {_ddr_mb9_uart_tbuf3, _ddr_mb9_uart_rbuf3,
            _ddr_mb9_uart_sbuf3, PCLK3, TXBUF_SZ3, RXBUF_SZ3, XOFF_SZ3, XON_SZ3,
            TXSEM3, RXSEM3, IRQ_TP37_MF_SIO_SEND3, IRQ_TP37_MF_SIO_RECV3};
#else
T_MB9_UART_CMNG const _ddr_mb9_uart_const3 = {_ddr_mb9_uart_tbuf3, _ddr_mb9_uart_rbuf3,
            _ddr_mb9_uart_sbuf3, PCLK3, TXBUF_SZ3, RXBUF_SZ3, XOFF_SZ3, XON_SZ3,
            TXSEM3, RXSEM3, IRQ_MF_SIO_SEND3, IRQ_MF_SIO_RECV3};
#endif

T_MB9_UART_MNG const _ddr_mb9_uart_mng3 = {(volatile struct t_mfs *)&REG_MFS3,
                            &_ddr_mb9_uart_const3,&_ddr_mb9_uart_data3};
#endif


#ifdef UART_4
#if (TXBUF_SZ4==0)
#define _ddr_mb9_uart_tbuf4    0
#else
VB  _ddr_mb9_uart_tbuf4[TXBUF_SZ4];
#endif
#if (RXBUF_SZ4==0)
#define _ddr_mb9_uart_rbuf4    0
#define _ddr_mb9_uart_sbuf4    0
#else
VB  _ddr_mb9_uart_rbuf4[RXBUF_SZ4];
UB  _ddr_mb9_uart_sbuf4[RXBUF_SZ4];
#endif


T_MB9_UART_DMNG _ddr_mb9_uart_data4;

#if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
    //FM4
T_MB9_UART_CMNG const _ddr_mb9_uart_const4 = {_ddr_mb9_uart_tbuf4, _ddr_mb9_uart_rbuf4,
            _ddr_mb9_uart_sbuf4, PCLK4, TXBUF_SZ4, RXBUF_SZ4, XOFF_SZ4, XON_SZ4,
            TXSEM4, RXSEM4, IRQ_MFS4_SEND, IRQ_MFS4_RECV};
#elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
T_MB9_UART_CMNG const _ddr_mb9_uart_const4 = {_ddr_mb9_uart_tbuf4, _ddr_mb9_uart_rbuf4,
            _ddr_mb9_uart_sbuf4, PCLK4, TXBUF_SZ4, RXBUF_SZ4, XOFF_SZ4, XON_SZ4,
            TXSEM4, RXSEM4, IRQ_TP37_MF_SIO_SEND4, IRQ_TP37_MF_SIO_RECV4};
#else
T_MB9_UART_CMNG const _ddr_mb9_uart_const4 = {_ddr_mb9_uart_tbuf4, _ddr_mb9_uart_rbuf4,
            _ddr_mb9_uart_sbuf4, PCLK4, TXBUF_SZ4, RXBUF_SZ4, XOFF_SZ4, XON_SZ4,
            TXSEM4, RXSEM4, IRQ_MF_SIO_SEND4, IRQ_MF_SIO_RECV4};
#endif

T_MB9_UART_MNG const _ddr_mb9_uart_mng4 = {(volatile struct t_mfs *)&REG_MFS4,
                            &_ddr_mb9_uart_const4,&_ddr_mb9_uart_data4};
#endif


#ifdef UART_5
#if (TXBUF_SZ5==0)
#define _ddr_mb9_uart_tbuf5    0
#else
VB  _ddr_mb9_uart_tbuf5[TXBUF_SZ5];
#endif
#if (RXBUF_SZ5==0)
#define _ddr_mb9_uart_rbuf5    0
#define _ddr_mb9_uart_sbuf5    0
#else
VB  _ddr_mb9_uart_rbuf5[RXBUF_SZ5];
UB  _ddr_mb9_uart_sbuf5[RXBUF_SZ5];
#endif


T_MB9_UART_DMNG _ddr_mb9_uart_data5;

#if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
    //FM4
T_MB9_UART_CMNG const _ddr_mb9_uart_const5 = {_ddr_mb9_uart_tbuf5, _ddr_mb9_uart_rbuf5,
            _ddr_mb9_uart_sbuf5, PCLK5, TXBUF_SZ5, RXBUF_SZ5, XOFF_SZ5, XON_SZ5,
            TXSEM5, RXSEM5, IRQ_MFS5_SEND, IRQ_MFS5_RECV};
#elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
T_MB9_UART_CMNG const _ddr_mb9_uart_const5 = {_ddr_mb9_uart_tbuf5, _ddr_mb9_uart_rbuf5,
            _ddr_mb9_uart_sbuf5, PCLK5, TXBUF_SZ5, RXBUF_SZ5, XOFF_SZ5, XON_SZ5,
            TXSEM5, RXSEM5, IRQ_TP37_MF_SIO_SEND5, IRQ_TP37_MF_SIO_RECV5};
#else
T_MB9_UART_CMNG const _ddr_mb9_uart_const5 = {_ddr_mb9_uart_tbuf5, _ddr_mb9_uart_rbuf5,
            _ddr_mb9_uart_sbuf5, PCLK5, TXBUF_SZ5, RXBUF_SZ5, XOFF_SZ5, XON_SZ5,
            TXSEM5, RXSEM5, IRQ_MF_SIO_SEND5, IRQ_MF_SIO_RECV5};
#endif

T_MB9_UART_MNG const _ddr_mb9_uart_mng5 = {(volatile struct t_mfs *)&REG_MFS5,
                            &_ddr_mb9_uart_const5,&_ddr_mb9_uart_data5};
#endif


#ifdef UART_6
#if (TXBUF_SZ6==0)
#define _ddr_mb9_uart_tbuf6    0
#else
VB  _ddr_mb9_uart_tbuf6[TXBUF_SZ6];
#endif
#if (RXBUF_SZ6==0)
#define _ddr_mb9_uart_rbuf6    0
#define _ddr_mb9_uart_sbuf6    0
#else
VB  _ddr_mb9_uart_rbuf6[RXBUF_SZ6];
UB  _ddr_mb9_uart_sbuf6[RXBUF_SZ6];
#endif


T_MB9_UART_DMNG _ddr_mb9_uart_data6;

#if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
    //FM4
T_MB9_UART_CMNG const _ddr_mb9_uart_const6 = {_ddr_mb9_uart_tbuf6, _ddr_mb9_uart_rbuf6,
            _ddr_mb9_uart_sbuf6, PCLK6, TXBUF_SZ6, RXBUF_SZ6, XOFF_SZ6, XON_SZ6,
            TXSEM6, RXSEM6, IRQ_MFS6_SEND, IRQ_MFS6_RECV};
#elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
T_MB9_UART_CMNG const _ddr_mb9_uart_const6 = {_ddr_mb9_uart_tbuf6, _ddr_mb9_uart_rbuf6,
            _ddr_mb9_uart_sbuf6, PCLK6, TXBUF_SZ6, RXBUF_SZ6, XOFF_SZ6, XON_SZ6,
            TXSEM6, RXSEM6, IRQ_TP37_MF_SIO_SEND6, IRQ_TP37_MF_SIO_RECV6};
#else
T_MB9_UART_CMNG const _ddr_mb9_uart_const6 = {_ddr_mb9_uart_tbuf6, _ddr_mb9_uart_rbuf6,
            _ddr_mb9_uart_sbuf6, PCLK6, TXBUF_SZ6, RXBUF_SZ6, XOFF_SZ6, XON_SZ6,
            TXSEM6, RXSEM6, IRQ_MF_SIO_SEND6, IRQ_MF_SIO_RECV6};
#endif

T_MB9_UART_MNG const _ddr_mb9_uart_mng6 = {(volatile struct t_mfs *)&REG_MFS6,
                            &_ddr_mb9_uart_const6,&_ddr_mb9_uart_data6};
#endif


#ifdef UART_7
#if (TXBUF_SZ7==0)
#define _ddr_mb9_uart_tbuf7    0
#else
VB  _ddr_mb9_uart_tbuf7[TXBUF_SZ7];
#endif
#if (RXBUF_SZ7==0)
#define _ddr_mb9_uart_rbuf7    0
#define _ddr_mb9_uart_sbuf7    0
#else
VB  _ddr_mb9_uart_rbuf7[RXBUF_SZ7];
UB  _ddr_mb9_uart_sbuf7[RXBUF_SZ7];
#endif


T_MB9_UART_DMNG _ddr_mb9_uart_data7;

#if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
    //FM4
T_MB9_UART_CMNG const _ddr_mb9_uart_const7 = {_ddr_mb9_uart_tbuf7, _ddr_mb9_uart_rbuf7,
            _ddr_mb9_uart_sbuf7, PCLK7, TXBUF_SZ7, RXBUF_SZ7, XOFF_SZ7, XON_SZ7,
            TXSEM7, RXSEM7, IRQ_MFS7_SEND, IRQ_MFS7_RECV};
#elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
T_MB9_UART_CMNG const _ddr_mb9_uart_const7 = {_ddr_mb9_uart_tbuf7, _ddr_mb9_uart_rbuf7,
            _ddr_mb9_uart_sbuf7, PCLK7, TXBUF_SZ7, RXBUF_SZ7, XOFF_SZ7, XON_SZ7,
            TXSEM7, RXSEM7, IRQ_TP37_MF_SIO_SEND7, IRQ_TP37_MF_SIO_RECV7};
#else
T_MB9_UART_CMNG const _ddr_mb9_uart_const7 = {_ddr_mb9_uart_tbuf7, _ddr_mb9_uart_rbuf7,
            _ddr_mb9_uart_sbuf7, PCLK7, TXBUF_SZ7, RXBUF_SZ7, XOFF_SZ7, XON_SZ7,
            TXSEM7, RXSEM7, IRQ_MF_SIO_SEND7, IRQ_MF_SIO_RECV7};
#endif

T_MB9_UART_MNG const _ddr_mb9_uart_mng7 = {(volatile struct t_mfs *)&REG_MFS7,
                            &_ddr_mb9_uart_const7,&_ddr_mb9_uart_data7};
#endif


/***************************************
        UARTデバイスドライバ初期化
 ***************************************/

void _ddr_mb9_uart_init(void)
{
  #ifdef UART_0
    memset(&_ddr_mb9_uart_data0, 0x00, sizeof(_ddr_mb9_uart_data0));
    loc_cpu();
    #ifdef RELOCATE0
      #if RELOCATE0 == 0
        #if FM3_SRS == 310 || FM3_SRS == 610 || FM3_SRS == 210
          REG_GPIO.ADE  &= ~(AN31);
        #elif FM3_SRS == AB40 || FM3_SRS == AA40 || FM3_SRS == A340 || FM3_SRS == A140 || FM3_SRS == 10010 || FM3_SRS == 10020 || FM3_SRS == 10030 || FM3_SRS == 10040
          REG_GPIO.ADE  &= ~(AN18 | AN17);
        #elif FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          REG_GPIO.ADE  &= ~(AN17 | AN16);              
        #elif FM3_SRS == 120 || FM3_SRS == 320 || FM3_SRS == 520
          REG_GPIO.ADE  &= ~(AN14 | AN13);
        #endif
        /* MB9AF130K,L,M,N  MB9AFA30L,M,N is no set for AN** */
        REG_GPIO.PFR2 |= (PFR_1 | PFR_2);
        REG_GPIO.PCR2 |= (PFR_1);
        REG_GPIO.EPFR07 |= ((PORT0<<EPF_0T) | (PORT0<<EPF_0R));
      #elif RELOCATE0 == 1
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.ADE  &= ~(AN03 | AN04);
          REG_GPIO.PFR1 |= (PFR_3 | PFR_4);
          REG_GPIO.PCR1 |= (PFR_3);
        #else 
          //FM3
          REG_GPIO.ADE  &= ~(AN04 | AN05);
          REG_GPIO.PFR1 |= (PFR_4 | PFR_5);
          REG_GPIO.PCR1 |= (PFR_4);
        #endif
        REG_GPIO.EPFR07 |= ((PORT1<<EPF_0T) | (PORT1<<EPF_0R));
      #elif RELOCATE0 == 2
        #if FM3_SRS == 310 || FM3_SRS == 610 || FM3_SRS == 210
          REG_GPIO.ADE  &= ~(AN20 | AN21);
        #endif
        REG_GPIO.PFRB |= (PFR_4 | PFR_5);
        REG_GPIO.PCRB |= (PFR_4);
        REG_GPIO.EPFR07 |= ((PORT2<<EPF_0T) | (PORT2<<EPF_0R));
      #endif
    #endif

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820 || defined(WISREED_SRS)
      //FM4
      _ddr_mb9_uart_data0.fcr0 = (FIFO_0R<<1)|FIFO_0T;
      if (FIFO_0R != 0)
      {
          _ddr_mb9_uart_data0.fbyte2 = RTRG_0R;
          _ddr_mb9_uart_data0.fcr1   = 0x08;
      }
    #elif FM3_SRS == 120 || FM3_SRS == 320 || FM3_SRS == 520
      //FM3
      _ddr_mb9_uart_data0.fcr0 = (FIFO_0R<<1)|FIFO_0T;
      if (FIFO_0R != 0)
      {
          _ddr_mb9_uart_data0.fbyte2 = RTRG_0R;
          _ddr_mb9_uart_data0.fcr1   = 0x08;
      } 
    #else
      _ddr_mb9_uart_data0.fcr0 = 0;
      _ddr_mb9_uart_data0.fbyte2 = 0;
      _ddr_mb9_uart_data0.fcr1   = 0;
    #endif
      
    REG_MFS0.SCR_IBCR= 0;
    REG_MFS0.SMR= 0;

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      REG_MFS0.FCR0= 0;
      REG_MFS0.FCR1= 0;    
    #endif
      
    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      //FM4
      vset_ipl(IRQ_MFS0_SEND, IPL_UART_0);
      vset_ipl(IRQ_MFS0_RECV, IPL_UART_0);
    #elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      vset_ipl(IRQ_TP37_MF_SIO_SEND0, IPL_UART_0);
      vset_ipl(IRQ_TP37_MF_SIO_RECV0, IPL_UART_0);
    #else
      vset_ipl(IRQ_MF_SIO_SEND0, IPL_UART_0);
      vset_ipl(IRQ_MF_SIO_RECV0, IPL_UART_0);
    #endif

    unl_cpu();
  #endif

  #ifdef UART_1
    memset(&_ddr_mb9_uart_data1, 0x00, sizeof(_ddr_mb9_uart_data1));
    loc_cpu();
    #ifdef RELOCATE1
      #if RELOCATE1 == 0
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.ADE  &= ~(AN19);
          REG_GPIO.PFR0 |= (PFR_A | PFR_9);
          REG_GPIO.PCR0 |= (PFR_A);
        #else 
          //FM3
          REG_GPIO.PFR5 |= (PFR_6 | PFR_7);
          REG_GPIO.PCR5 |= (PFR_6);
        #endif
        REG_GPIO.EPFR07 |= ((PORT0<<EPF_1T) | (PORT0<<EPF_1R));
      #elif RELOCATE1 == 1
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.ADE  &= ~(AN00 | AN01);
          REG_GPIO.PFR1 |= (PFR_0 | PFR_1);
          REG_GPIO.PCR1 |= (PFR_0);
        #else 
          //FM3
          REG_GPIO.ADE  &= ~(AN01 | AN02);
          REG_GPIO.PFR1 |= (PFR_1 | PFR_2);
          REG_GPIO.PCR1 |= (PFR_1);
        #endif
        REG_GPIO.EPFR07 |= ((PORT1<<EPF_1T) | (PORT1<<EPF_1R));
      #elif RELOCATE1 == 2
        REG_GPIO.PFRF |= (PFR_0 | PFR_1);
        REG_GPIO.PCRF |= (PFR_0);
        REG_GPIO.EPFR07 |= ((PORT2<<EPF_1T) | (PORT2<<EPF_1R));
      #endif
    #endif

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820 || defined(WISREED_SRS)
      //FM4
      _ddr_mb9_uart_data1.fcr0 = (FIFO_1R<<1)|FIFO_1T;
      if (FIFO_1R != 0)
      {
          _ddr_mb9_uart_data1.fbyte2 = RTRG_1R;
          _ddr_mb9_uart_data1.fcr1   = 0x08;
      }
    #elif FM3_SRS == 120 || FM3_SRS == 320 || FM3_SRS == 520
      //FM3
      _ddr_mb9_uart_data1.fcr0 = (FIFO_1R<<1)|FIFO_1T;
      if (FIFO_1R != 0)
      {
          _ddr_mb9_uart_data1.fbyte2 = RTRG_1R;
          _ddr_mb9_uart_data1.fcr1   = 0x08;
      }  
    #else
      _ddr_mb9_uart_data1.fcr0 = 0;
      _ddr_mb9_uart_data1.fbyte2 = 0;
      _ddr_mb9_uart_data1.fcr1   = 0;        
    #endif
    
    REG_MFS1.SCR_IBCR= 0;
    REG_MFS1.SMR= 0;

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      REG_MFS1.FCR0= 0;
      REG_MFS1.FCR1= 0;    
    #endif    
    
    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      //FM4
      vset_ipl(IRQ_MFS1_SEND, IPL_UART_1);
      vset_ipl(IRQ_MFS1_RECV, IPL_UART_1);
    #elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      vset_ipl(IRQ_TP37_MF_SIO_SEND1, IPL_UART_1);
      vset_ipl(IRQ_TP37_MF_SIO_RECV1, IPL_UART_1);
    #else
      vset_ipl(IRQ_MF_SIO_SEND1, IPL_UART_1);
      vset_ipl(IRQ_MF_SIO_RECV1, IPL_UART_1);
    #endif

    unl_cpu();
  #endif

  #ifdef UART_2
    memset(&_ddr_mb9_uart_data2, 0x00, sizeof(_ddr_mb9_uart_data2));
    loc_cpu();
    #ifdef RELOCATE2
      #if RELOCATE2 == 0
        REG_GPIO.PFR7 |= (PFR_2 | PFR_3);
        REG_GPIO.PCR7 |= (PFR_2);
        REG_GPIO.EPFR07 |= ((PORT0<<EPF_2T) | (PORT0<<EPF_2R));
      #elif RELOCATE2 == 1
        #if FM3_SRS == 310 || FM3_SRS == 610 || FM3_SRS == 210
          REG_GPIO.ADE  &= ~(AN29 | AN28);
        #endif
        REG_GPIO.PFR2 |= (PFR_4 | PFR_5);
        REG_GPIO.PCR2 |= (PFR_4);
        REG_GPIO.EPFR07 |= ((PORT1<<EPF_2T) | (PORT1<<EPF_2R));
      #elif RELOCATE2 == 2
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.ADE  &= ~(AN06 | AN07);
          REG_GPIO.PFR1 |= (PFR_6 | PFR_7);
          REG_GPIO.PCR1 |= (PFR_6);
        #else 
          //FM3
          REG_GPIO.ADE  &= ~(AN07 | AN08);
          REG_GPIO.PFR1 |= (PFR_7 | PFR_8);
          REG_GPIO.PCR1 |= (PFR_7);
        #endif
        REG_GPIO.EPFR07 |= ((PORT2<<EPF_2T) | (PORT2<<EPF_2R));
      #endif
    #endif

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820 || defined(WISREED_SRS)
      //FM4
      _ddr_mb9_uart_data2.fcr0 = (FIFO_2R<<1)|FIFO_2T;
      if (FIFO_2R != 0)
      {
          _ddr_mb9_uart_data2.fbyte2 = RTRG_2R;
          _ddr_mb9_uart_data2.fcr1   = 0x08;
      }
    #else
      _ddr_mb9_uart_data2.fcr0 = 0;
      _ddr_mb9_uart_data2.fbyte2 = 0;
      _ddr_mb9_uart_data2.fcr1   = 0;        
    #endif
    
    REG_MFS2.SCR_IBCR= 0;
    REG_MFS2.SMR= 0;

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      REG_MFS2.FCR0= 0;
      REG_MFS2.FCR1= 0;    
    #endif
      
    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      //FM4
      vset_ipl(IRQ_MFS2_SEND, IPL_UART_2);
      vset_ipl(IRQ_MFS2_RECV, IPL_UART_2);
    #elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      vset_ipl(IRQ_TP37_MF_SIO_SEND2, IPL_UART_2);
      vset_ipl(IRQ_TP37_MF_SIO_RECV2, IPL_UART_2);
    #else
      vset_ipl(IRQ_MF_SIO_SEND2, IPL_UART_2);
      vset_ipl(IRQ_MF_SIO_RECV2, IPL_UART_2);
    #endif
      
    unl_cpu();
  #endif

  #ifdef UART_3
    memset(&_ddr_mb9_uart_data3, 0x00, sizeof(_ddr_mb9_uart_data3));
    loc_cpu();
    #ifdef RELOCATE3
      #if RELOCATE3 == 0
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.PFR6 |= (PFR_6 | PFR_7);
          REG_GPIO.PCR6 |= (PFR_6);
          REG_GPIO.EPFR07 |= ((PORT0<<EPF_3T) | (PORT0<<EPF_3R));
        #elif FM3_SRS == 310 || FM3_SRS == 610 || FM3_SRS == 210
          REG_GPIO.PFR7 |= (PFR_5 | PFR_6);
          REG_GPIO.PCR7 |= (PFR_5);
          REG_GPIO.EPFR07 |= ((PORT0<<EPF_3T) | (PORT0<<EPF_3R));
        #else /* MB9B500, MB9AB40N, MB9AA40N, MB9A340N, MB9A140N, MB9B510R, MB9B410R, MB9B310R, MB9B110R */
          REG_GPIO.PFR6 |= (PFR_6 | PFR_7);
          REG_GPIO.PCR6 |= (PFR_6);
          REG_GPIO.EPFR07 |= ((PORT0<<EPF_3T) | (PORT0<<EPF_3R));
        #endif
      #elif RELOCATE3 == 1
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.PFR3 |= (PFR_1 | PFR_2);
          REG_GPIO.PCR3 |= (PFR_1);
        #else 
          //FM3
          #if FM3_SRS == 120 || FM3_SRS == 320 || FM3_SRS == 520
            REG_GPIO.ADE  &= ~(AN22 | AN23);
          #endif
          REG_GPIO.PFR5 |= (PFR_0 | PFR_1);
          REG_GPIO.PCR5 |= (PFR_0);
        #endif
        REG_GPIO.EPFR07 |= ((PORT1<<EPF_3T) | (PORT1<<EPF_3R));
      #elif RELOCATE3 == 2
        REG_GPIO.PFR4 |= (PFR_8 | PFR_9);
        REG_GPIO.PCR4 |= (PFR_8);
        REG_GPIO.EPFR07 |= ((PORT2<<EPF_3T) | (PORT2<<EPF_3R));
      #endif
    #endif

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      //FM4
      _ddr_mb9_uart_data3.fcr0 = (FIFO_3R<<1)|FIFO_3T;
      if (FIFO_3R != 0)
      {
          _ddr_mb9_uart_data3.fbyte2 = RTRG_3R;
          _ddr_mb9_uart_data3.fcr1   = 0x08;
      }
    #elif FM3_SRS == 120 || FM3_SRS == 320 || FM3_SRS == 520
      //FM3
      _ddr_mb9_uart_data3.fcr0 = (FIFO_3R<<1)|FIFO_3T;
      if (FIFO_3R != 0)
      {
          _ddr_mb9_uart_data3.fbyte2 = RTRG_3R;
          _ddr_mb9_uart_data3.fcr1   = 0x08;
      }      
    #else
      _ddr_mb9_uart_data3.fcr0 = 0;
      _ddr_mb9_uart_data3.fbyte2 = 0;
      _ddr_mb9_uart_data3.fcr1   = 0;        
    #endif
    
    REG_MFS3.SCR_IBCR= 0;
    REG_MFS3.SMR= 0;

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      REG_MFS3.FCR0= 0;
      REG_MFS3.FCR1= 0;    
    #endif
      
    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      //FM4
      vset_ipl(IRQ_MFS3_SEND, IPL_UART_3);
      vset_ipl(IRQ_MFS3_RECV, IPL_UART_3);
    #elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      vset_ipl(IRQ_TP37_MF_SIO_SEND3, IPL_UART_3);
      vset_ipl(IRQ_TP37_MF_SIO_RECV3, IPL_UART_3);
    #else
      vset_ipl(IRQ_MF_SIO_SEND3, IPL_UART_3);
      vset_ipl(IRQ_MF_SIO_RECV3, IPL_UART_3);
    #endif
      
    unl_cpu();
  #endif

  #ifdef UART_4
    memset(&_ddr_mb9_uart_data4, 0x00, sizeof(_ddr_mb9_uart_data4));
    loc_cpu();
    #ifdef RELOCATE4
      #if RELOCATE4 == 0
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.PFR5 |= (PFR_4 | PFR_3);
          REG_GPIO.PCR5 |= (PFR_4);
          REG_GPIO.EPFR08 |= ((PORT0<<EPF_4T) | (PORT0<<EPF_4R));
          #if CTSRTS4 == 1
            REG_GPIO.PFR5 |= (PFR_0 | PFR_1);
            REG_GPIO.PCR5 |= (PFR_0);
            REG_GPIO.EPFR08 |= ((PORT0<<EPF_4CT) | (PORT0<<EPF_4RT));
          #endif
        #elif FM3_SRS == 310 || FM3_SRS == 610 || FM3_SRS == 210
          REG_GPIO.PFRD |= (PFR_2 | PFR_1);
          REG_GPIO.PCRD |= (PFR_2);
          REG_GPIO.EPFR08 |= ((PORT0<<EPF_4T) | (PORT0<<EPF_4R));
          #if CTSRTS4 == 1
            REG_GPIO.PFRC |= (PFR_F | PFR_E);
            REG_GPIO.PCRC |= (PFR_E);
            REG_GPIO.EPFR08 |= ((PORT0<<EPF_4CT) | (PORT0<<EPF_4RT));
          #endif
        #else /* MB9B500, MB9AB40N, MB9AA40N, MB9A340N, MB9A140N, MB9B510R, MB9B410R, MB9B310R, MB9B110R, MB9AF130K,L,M,N  MB9AFA30L,M,N MB9B120M,MB9B320M,MB9B520M */
          REG_GPIO.PFR0 |= (PFR_A | PFR_B);
          REG_GPIO.PCR0 |= (PFR_A);
          REG_GPIO.EPFR08 |= ((PORT0<<EPF_4T) | (PORT0<<EPF_4R));
          #if CTSRTS4 == 1
            REG_GPIO.PFR0 |= (PFR_D | PFR_E);
            REG_GPIO.PCR0 |= (PFR_E);
            REG_GPIO.EPFR08 |= ((PORT0<<EPF_4CT) | (PORT0<<EPF_4RT));
          #endif
        #endif
      #elif RELOCATE4 == 1
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.ADE  &= ~(AN09 | AN10);
          REG_GPIO.PFR1 |= (PFR_9 | PFR_A);
          REG_GPIO.PCR1 |= (PFR_9);
          REG_GPIO.EPFR08 |= ((PORT1<<EPF_4T) | (PORT1<<EPF_4R));
          #if CTSRTS4 == 1
            REG_GPIO.ADE  &= ~(AN12 | AN13);
            REG_GPIO.PFR1 |= (PFR_C | PFR_D);
            REG_GPIO.PCR1 |= (PFR_C);
            REG_GPIO.EPFR08 |= ((PORT1<<EPF_4CT) | (PORT1<<EPF_4RT));
          #endif
        #else
          //FM3
          REG_GPIO.ADE  &= ~(AN10 | AN11);
          REG_GPIO.PFR1 |= (PFR_A | PFR_B);
          REG_GPIO.PCR1 |= (PFR_A);
          REG_GPIO.EPFR08 |= ((PORT1<<EPF_4T) | (PORT1<<EPF_4R));
          #if CTSRTS4 == 1
            REG_GPIO.ADE  &= ~(AN13 | AN14);
            REG_GPIO.PFR1 |= (PFR_D | PFR_E);
            REG_GPIO.PCR1 |= (PFR_E);
            REG_GPIO.EPFR08 |= ((PORT1<<EPF_4CT) | (PORT1<<EPF_4RT));
          #endif
        #endif
      #elif RELOCATE4 == 2
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.PFR5 |= (PFR_8 | PFR_9);
          REG_GPIO.PCR5 |= (PFR_8);
          REG_GPIO.EPFR08 |= ((PORT2<<EPF_4T) | (PORT2<<EPF_4R));
          #if CTSRTS4 == 1
            REG_GPIO.PFR5 |= (PFR_B);
            REG_GPIO.PFR3 |= (PFR_0);
            REG_GPIO.PCR5 |= (PFR_B);
            REG_GPIO.EPFR08 |= ((PORT2<<EPF_4CT) | (PORT2<<EPF_4RT));
          #endif
        #else
          //FM3
          #if FM3_SRS == AB40 || FM3_SRS == AA40 || FM3_SRS == A340 || FM3_SRS == A140 || FM3_SRS == 10010 || FM3_SRS == 10020 || FM3_SRS == 10030 || FM3_SRS == 10040
            REG_GPIO.ADE  &= ~(AN20 | AN21);
          #endif
          REG_GPIO.PFR0 |= (PFR_5 | PFR_6);
          REG_GPIO.PCR0 |= (PFR_5);
          REG_GPIO.EPFR08 |= ((PORT2<<EPF_4T) | (PORT2<<EPF_4R));
          #if CTSRTS4 == 1
            #if FM3_SRS == AB40 || FM3_SRS == AA40 || FM3_SRS == A340 || FM3_SRS == A140 || FM3_SRS == 10010 || FM3_SRS == 10020 || FM3_SRS == 10030 || FM3_SRS == 10040
              REG_GPIO.ADE  &= ~(AN23);
            #endif
            REG_GPIO.PFR0 |= (PFR_8 | PFR_9);
            REG_GPIO.PCR0 |= (PFR_9);
            REG_GPIO.EPFR08 |= ((PORT2<<EPF_4CT) | (PORT2<<EPF_4RT));
          #endif
        #endif
      #endif
    #endif

    #if FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      _ddr_mb9_uart_data4.fcr0 = 0;
      _ddr_mb9_uart_data4.fbyte2 = 0;
      _ddr_mb9_uart_data4.fcr1   = 0; 
    #else
      _ddr_mb9_uart_data4.fcr0 = (FIFO_4R<<1)|FIFO_4T;
      if (FIFO_4R != 0)
      {
          _ddr_mb9_uart_data4.fbyte2 = RTRG_4R;
          _ddr_mb9_uart_data4.fcr1   = 0x08;
      }
    #endif
    
    REG_MFS4.SCR_IBCR= 0;
    REG_MFS4.SMR= 0;
    REG_MFS4.FCR0= 0;
    REG_MFS4.FCR1= 0;

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
    //FM4
      vset_ipl(IRQ_MFS4_SEND, IPL_UART_4);
      vset_ipl(IRQ_MFS4_RECV, IPL_UART_4);
    #elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      vset_ipl(IRQ_TP37_MF_SIO_SEND4, IPL_UART_4);
      vset_ipl(IRQ_TP37_MF_SIO_RECV4, IPL_UART_4);
    #else
      vset_ipl(IRQ_MF_SIO_SEND4, IPL_UART_4);
      vset_ipl(IRQ_MF_SIO_RECV4, IPL_UART_4);
    #endif

    unl_cpu();
  #endif

  #ifdef UART_5
    memset(&_ddr_mb9_uart_data5, 0x00, sizeof(_ddr_mb9_uart_data5));
    loc_cpu();
    #ifdef RELOCATE5
      #if RELOCATE5 == 0
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.PFR6 |= (PFR_2 | PFR_1);
          REG_GPIO.PCR6 |= (PFR_2);
        #else 
          //FM3
          #if FM3_SRS == 120 || FM3_SRS == 320 || FM3_SRS == 520
            REG_GPIO.ADE  &= ~(AN21 | AN20);
          #endif
          REG_GPIO.PFR6 |= (PFR_0 | PFR_1);
          REG_GPIO.PCR6 |= (PFR_0);
        #endif
        REG_GPIO.EPFR08 |= ((PORT0<<EPF_5T) | (PORT0<<EPF_5R));
      #elif RELOCATE5 == 1
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.PFR6 |= (PFR_3 | PFR_4);
          REG_GPIO.PCR6 |= (PFR_3);
          REG_GPIO.EPFR08 |= ((PORT1<<EPF_5T) | (PORT1<<EPF_5R));
        #elif FM3_SRS == 310 || FM3_SRS == 610 || FM3_SRS == 210
          REG_GPIO.PFR9 |= (PFR_2 | PFR_3);
          REG_GPIO.PCR9 |= (PFR_2);
          REG_GPIO.EPFR08 |= ((PORT1<<EPF_5T) | (PORT1<<EPF_5R));
        #else /* MB9B500, MB9AB40N, MB9AA40N, MB9A340N, MB9A140N, MB9B510R, MB9B410R, MB9B310R, MB9B110R */
          REG_GPIO.PFR6 |= (PFR_3 | PFR_4);
          REG_GPIO.PCR6 |= (PFR_3);
          REG_GPIO.EPFR08 |= ((PORT1<<EPF_5T) | (PORT1<<EPF_5R));
        #endif
      #elif RELOCATE5 == 2
        REG_GPIO.PFR3 |= (PFR_6 | PFR_7);
        REG_GPIO.PCR3 |= (PFR_6);
        REG_GPIO.EPFR08 |= ((PORT2<<EPF_5T) | (PORT2<<EPF_5R));
      #endif
    #endif

    #if FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      _ddr_mb9_uart_data5.fcr0 = 0;
      _ddr_mb9_uart_data5.fbyte2 = 0;
      _ddr_mb9_uart_data5.fcr1   = 0; 
    #elif FM3_SRS == 120 || FM3_SRS == 320 || FM3_SRS == 520
      //FM3
      _ddr_mb9_uart_data5.fcr0 = 0;
      _ddr_mb9_uart_data5.fbyte2 = 0;
      _ddr_mb9_uart_data5.fcr1   = 0;      
    #else
      _ddr_mb9_uart_data5.fcr0 = (FIFO_5R<<1)|FIFO_5T;
      if (FIFO_5R != 0)
      {
          _ddr_mb9_uart_data5.fbyte2 = RTRG_5R;
          _ddr_mb9_uart_data5.fcr1   = 0x08;
      }
    #endif

    REG_MFS5.SCR_IBCR= 0;
    REG_MFS5.SMR= 0;
    REG_MFS5.FCR0= 0;
    REG_MFS5.FCR1= 0;

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      //FM4
      vset_ipl(IRQ_MFS5_SEND, IPL_UART_5);
      vset_ipl(IRQ_MFS5_RECV, IPL_UART_5);
    #elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      vset_ipl(IRQ_TP37_MF_SIO_SEND5, IPL_UART_5);
      vset_ipl(IRQ_TP37_MF_SIO_RECV5, IPL_UART_5);
    #else
      vset_ipl(IRQ_MF_SIO_SEND5, IPL_UART_5);
      vset_ipl(IRQ_MF_SIO_RECV5, IPL_UART_5);
    #endif

    unl_cpu();
  #endif

  #ifdef UART_6
    memset(&_ddr_mb9_uart_data6, 0x00, sizeof(_ddr_mb9_uart_data6));
    loc_cpu();
    #ifdef RELOCATE6
      #if RELOCATE6 == 0
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.PFR5 |= (PFR_5 | PFR_6);
          REG_GPIO.PCR5 |= (PFR_5);
        #else 
          //FM3
          REG_GPIO.PFR5 |= (PFR_3 | PFR_4);
          REG_GPIO.PCR5 |= (PFR_3);
        #endif
        REG_GPIO.EPFR08 |= ((PORT0<<EPF_6T) | (PORT0<<EPF_6R));
      #elif RELOCATE6 == 1
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.PFR0 |= (PFR_B | PFR_C);
          REG_GPIO.PCR0 |= (PFR_B);
        #else 
          //FM3
          REG_GPIO.PFR3 |= (PFR_3 | PFR_2);
          REG_GPIO.PCR3 |= (PFR_3);
        #endif
        REG_GPIO.EPFR08 |= ((PORT1<<EPF_6T) | (PORT1<<EPF_6R));
      #elif RELOCATE6 == 2
        REG_GPIO.PFRF |= (PFR_3 | PFR_4);
        REG_GPIO.PCRF |= (PFR_3);
        REG_GPIO.EPFR08 |= ((PORT2<<EPF_6T) | (PORT2<<EPF_6R));
      #endif
    #endif

    #if FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      _ddr_mb9_uart_data6.fcr0 = 0;
      _ddr_mb9_uart_data6.fbyte2 = 0;
      _ddr_mb9_uart_data6.fcr1   = 0; 
    #elif FM3_SRS == 120 || FM3_SRS == 320 || FM3_SRS == 520
      //FM3
      _ddr_mb9_uart_data6.fcr0 = 0;
      _ddr_mb9_uart_data6.fbyte2 = 0;
      _ddr_mb9_uart_data6.fcr1   = 0;       
    #else
      _ddr_mb9_uart_data6.fcr0 = (FIFO_6R<<1)|FIFO_6T;
      if (FIFO_6R != 0)
      {
          _ddr_mb9_uart_data6.fbyte2 = RTRG_6R;
          _ddr_mb9_uart_data6.fcr1   = 0x08;
      }
    #endif

    REG_MFS6.SCR_IBCR= 0;
    REG_MFS6.SMR= 0;
    REG_MFS6.FCR0= 0;
    REG_MFS6.FCR1= 0;

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      //FM4
      vset_ipl(IRQ_MFS6_SEND, IPL_UART_6);
      vset_ipl(IRQ_MFS6_RECV, IPL_UART_6);
    #elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      vset_ipl(IRQ_TP37_MF_SIO_SEND6, IPL_UART_6);
      vset_ipl(IRQ_TP37_MF_SIO_RECV6, IPL_UART_6);
    #else
      vset_ipl(IRQ_MF_SIO_SEND6, IPL_UART_6);
      vset_ipl(IRQ_MF_SIO_RECV6, IPL_UART_6);
    #endif

    unl_cpu();
  #endif

  #ifdef UART_7
    memset(&_ddr_mb9_uart_data7, 0x00, sizeof(_ddr_mb9_uart_data7));
    loc_cpu();
    #ifdef RELOCATE7
      #if RELOCATE7 == 0
        #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
          //FM4
          REG_GPIO.ADE  &= ~(AN23 | AN22);
          REG_GPIO.PFR0 |= (PFR_5 | PFR_6);
          REG_GPIO.PCR0 |= (PFR_5);
        #else 
          //FM3
          REG_GPIO.PFR5 |= (PFR_9 | PFR_A);
          REG_GPIO.PCR5 |= (PFR_9);
        #endif
        REG_GPIO.EPFR08 |= ((PORT0<<EPF_7T) | (PORT0<<EPF_7R));
      #elif RELOCATE7 == 1
        REG_GPIO.PFR4 |= (PFR_E | PFR_D);
        REG_GPIO.PCR4 |= (PFR_E);
        REG_GPIO.EPFR08 |= ((PORT1<<EPF_7T) | (PORT1<<EPF_7R));
      #elif RELOCATE7 == 2
        #if FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
          REG_GPIO.PFR8 |= (PFR_0 | PFR_1);
          REG_GPIO.PCR8 |= (PFR_0);
          REG_GPIO.EPFR08 |= ((PORT2<<EPF_7T) | (PORT2<<EPF_7R));
        #else
          #if FM3_SRS == 310 || FM3_SRS == 610 || FM3_SRS == 210
            REG_GPIO.ADE  &= ~(AN16 | AN17);
          #endif
          REG_GPIO.PFRB |= (PFR_0 | PFR_1);
          REG_GPIO.PCRB |= (PFR_0);
          REG_GPIO.EPFR08 |= ((PORT2<<EPF_7T) | (PORT2<<EPF_7R));
        #endif
      #endif
    #endif

    #if FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      _ddr_mb9_uart_data7.fcr0 = 0;
      _ddr_mb9_uart_data7.fbyte2 = 0;
      _ddr_mb9_uart_data7.fcr1   = 0; 
    #elif FM3_SRS == 120 || FM3_SRS == 320 || FM3_SRS == 520
      //FM3
      _ddr_mb9_uart_data7.fcr0 = 0;
      _ddr_mb9_uart_data7.fbyte2 = 0;
      _ddr_mb9_uart_data7.fcr1   = 0;      
    #else
      _ddr_mb9_uart_data7.fcr0 = (FIFO_7R<<1)|FIFO_7T;
      if (FIFO_7R != 0)
      {
          _ddr_mb9_uart_data7.fbyte2 = RTRG_7R;
          _ddr_mb9_uart_data7.fcr1   = 0x08;
      }
    #endif
      
    REG_MFS7.SCR_IBCR= 0;
    REG_MFS7.SMR= 0;
    REG_MFS7.FCR0= 0;
    REG_MFS7.FCR1= 0;

    #if FM3_SRS == 10500 || FM3_SRS == 10510 || FM3_SRS == 10520 || FM3_SRS == 10600 || FM3_SRS == 10610 || FM3_SRS == 10620 || FM3_SRS == 10700 || FM3_SRS == 10710 || FM3_SRS == 10720 || FM3_SRS == 10800 || FM3_SRS == 10810 || FM3_SRS == 10820
      //FM4
      vset_ipl(IRQ_MFS7_SEND, IPL_UART_7);
      vset_ipl(IRQ_MFS7_RECV, IPL_UART_7);
    #elif FM3_SRS == 10200 || FM3_SRS == 10210 || FM3_SRS == 10220 || FM3_SRS == 10230 || FM3_SRS == 10240 || FM3_SRS == 10250 || FM3_SRS == 10260
      vset_ipl(IRQ_TP37_MF_SIO_SEND7, IPL_UART_7);
      vset_ipl(IRQ_TP37_MF_SIO_RECV7, IPL_UART_7);
    #else
      vset_ipl(IRQ_MF_SIO_SEND7, IPL_UART_7);
      vset_ipl(IRQ_MF_SIO_RECV7, IPL_UART_7);
    #endif

    unl_cpu();
  #endif
}

#if (defined(UART_0)||defined(UART_1)||defined(UART_2)||defined(UART_3)||defined(UART_4)||defined(UART_5)||defined(UART_6)||defined(UART_7))
/***************************************
        UART受信終了確認処理
 ***************************************/

BOOL _ddr_mb9_uart_check_chr(T_COM_RCV *ReceiveData, VB chr, UB sts)
{
    BOOL flag = FALSE;

    if ((sts & (T_COM_EROR|T_COM_ERP|T_COM_ERF)) != 0)
        flag = TRUE;
    else if (ReceiveData->rcnt == 0)
        flag = TRUE;
    else if (ReceiveData->eos != 0)
    {
        if ((ReceiveData->eos->flg[0] != 0) && (ReceiveData->eos->chr[0] == chr))
            flag = TRUE;
        else if ((ReceiveData->eos->flg[1] != 0) && (ReceiveData->eos->chr[1] == chr))
            flag = TRUE;
        else if ((ReceiveData->eos->flg[2] != 0) && (ReceiveData->eos->chr[2] == chr))
            flag = TRUE;
        else if ((ReceiveData->eos->flg[3] != 0) && (ReceiveData->eos->chr[3] == chr))
            flag = TRUE;
    }
    return flag;
}

/*******************************************
        UART受信エラーステータス解析
 *******************************************/

UB _ddr_mb9_uart_getssr(UB ssr_register, T_MB9_UART_MNG const *pk_UARTmng)
{
    UB sts = 0;

    if ((ssr_register & 0x20) != 0){
        sts |= T_COM_ERP;
        pk_UARTmng->port->SSR = 0x80;
    }
    if ((ssr_register & 0x10) != 0){
        sts |= T_COM_ERF;
        pk_UARTmng->port->SSR = 0x80;
    }
    if ((ssr_register & 0x08) != 0){
        sts |= T_COM_EROR;
        pk_UARTmng->port->SSR = 0x80;
    }
    return sts;
}

/*******************************
        バッファコピー処理
 *******************************/

BOOL _ddr_mb9_uart_copy(T_MB9_UART_MNG const *pk_UARTmng, T_COM_SND *TransmiteData)
{
    for(; TransmiteData->tcnt != 0; ) {
        if (pk_UARTmng->data->tcnt < pk_UARTmng->cdata->tsize) {
            pk_UARTmng->cdata->tbuf[pk_UARTmng->data->sndp++] = *TransmiteData->tbuf++;
            TransmiteData->tcnt--;
            if (pk_UARTmng->data->sndp >= pk_UARTmng->cdata->tsize)
                pk_UARTmng->data->sndp = 0;
            pk_UARTmng->data->tcnt++;
        } else
            break;
    }
    return (TransmiteData->tcnt == 0) ? TRUE : FALSE;
}

/***********************************************
        ロカール送信バッファからの送信処理
 ***********************************************/

void _ddr_mb9_uart_send_local_buf(INT cnt, T_MB9_UART_MNG const *pk_UARTmng)
{
    INT i;

    for(i = cnt; i > 0; i--) {
        pk_UARTmng->port->RDR_TDR = *pk_UARTmng->data->SndData->tbuf++;
        if (--pk_UARTmng->data->SndData->tcnt == 0) {
            pk_UARTmng->data->SndData = 0;
            pk_UARTmng->port->SCR_IBCR &= ~0x08;
            if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                pk_UARTmng->port->FCR1 &= ~0x02;
            }
            isig_sem((ID)pk_UARTmng->cdata->tsemid);
            break;
        }
    }

    if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
        if (pk_UARTmng->port->FBYTE1 != 0x00){
            pk_UARTmng->port->FCR1 &= ~0x04;
        }
    }
}

/***********************************************
        内部送信バッファからの送信処理
 ***********************************************/

void _ddr_mb9_uart_send_drv_buf(INT cnt, T_MB9_UART_MNG const *pk_UARTmng)
{
    INT i;
    UH sndp;

    sndp = pk_UARTmng->data->sndp - pk_UARTmng->data->tcnt;
    if (pk_UARTmng->data->tcnt > pk_UARTmng->data->sndp)
        sndp += pk_UARTmng->cdata->tsize;
    for(i = cnt; i > 0; i--) {
        pk_UARTmng->port->RDR_TDR = pk_UARTmng->cdata->tbuf[sndp];
        if (++sndp >= pk_UARTmng->cdata->tsize)
            sndp = 0;
        if (--pk_UARTmng->data->tcnt == 0)
            break;
    }

    if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
        if (pk_UARTmng->port->FBYTE1 != 0x00){
            pk_UARTmng->port->FCR1 &= ~0x04;
        }
    }

    if (pk_UARTmng->data->SndData != 0) {
        if (_ddr_mb9_uart_copy(pk_UARTmng, pk_UARTmng->data->SndData) == TRUE) {
            pk_UARTmng->data->SndData = 0;
            isig_sem((ID)pk_UARTmng->cdata->tsemid);
        }
    }
    if ((pk_UARTmng->data->tcnt == 0) && (pk_UARTmng->data->SndData == 0)){
        pk_UARTmng->port->SCR_IBCR &= ~0x08;
 
        if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
            pk_UARTmng->port->FCR1 &= ~0x02;
        }
    }
}

/***********************************************
        UART送信データエンプティ割込み処理
 ***********************************************/

void _ddr_mb9_uart_txi(T_MB9_UART_MNG const *pk_UARTmng)
{
    INT cnt;

    if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
        cnt = SFIFOSIZE - pk_UARTmng->port->FBYTE1;
    } else {
      cnt = ((pk_UARTmng->port->SSR & 0x02) != 0) ? 1 : 0;
    }

    if (cnt != 0){ 
        pk_UARTmng->port->SCR_IBCR &= ~0x08;

        if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
            pk_UARTmng->port->FCR1 &= ~0x02;
        }

        if (pk_UARTmng->data->status.bit.req_xon_xoff == 1) {
            pk_UARTmng->port->RDR_TDR = (pk_UARTmng->data->status.bit.rx_xoff == 0) ? XON : XOFF;
            pk_UARTmng->data->status.bit.req_xon_xoff = 0;
            cnt--;
            if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                if (pk_UARTmng->port->FBYTE1 != 0x00){
                    pk_UARTmng->port->FCR1 &= ~0x04;
                }
            }
        }
        if (pk_UARTmng->data->status.bit.tx_xoff == 0) { 
            if (pk_UARTmng->cdata->tsize == 0) {
                if (pk_UARTmng->data->SndData != 0) {
                    _ddr_mb9_uart_send_local_buf(cnt, pk_UARTmng);
                    pk_UARTmng->port->SCR_IBCR |= 0x08;
                    if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                        pk_UARTmng->port->FCR1 |= 0x02;
                    }
                }
            } else {
                if (pk_UARTmng->data->tcnt != 0) {
                    _ddr_mb9_uart_send_drv_buf(cnt, pk_UARTmng);
                    pk_UARTmng->port->SCR_IBCR |= 0x08;
                    if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                        pk_UARTmng->port->FCR1 |= 0x02;
                    }
                }
            }
        }

        if ((pk_UARTmng->data->tcnt == 0) && (pk_UARTmng->data->status.bit.cln_wait != 0)) {
            if ((pk_UARTmng->port->SSR & 0x02) == 0) {
                pk_UARTmng->port->SCR_IBCR |= 0x08;
                if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                    pk_UARTmng->port->FCR1 |= 0x02;
                }
            } else {
                isig_sem((ID)pk_UARTmng->cdata->tsemid);
                pk_UARTmng->data->status.bit.cln_wait = 0;
            }
        }
    }
}

/***********************************
        受信XOFFのチェック処理
 ***********************************/

void _ddr_mb9_uart_chk_rxoff(T_MB9_UART_MNG const *pk_UARTmng)
{
    if ((pk_UARTmng->data->status.bit.rx_xoff == 0) &&
        (pk_UARTmng->data->rcnt >= pk_UARTmng->cdata->xoff_size)) {
        pk_UARTmng->data->status.bit.rx_xoff = 1;
        pk_UARTmng->data->status.bit.req_xon_xoff = 1;
        pk_UARTmng->port->SCR_IBCR |= 0x08;
        if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
            pk_UARTmng->port->FCR1 |= 0x02;
        }

        _ddr_mb9_uart_txi(pk_UARTmng);
    }
}

/***********************************
        受信XONのチェック処理
 ***********************************/

void _ddr_mb9_uart_chk_rxon(T_MB9_UART_MNG const *pk_UARTmng)
{
    if ((pk_UARTmng->data->status.bit.rx_xoff == 1) &&
        (pk_UARTmng->data->rcnt <= pk_UARTmng->cdata->xon_size)) {
        pk_UARTmng->data->status.bit.rx_xoff = 0;
        pk_UARTmng->data->status.bit.req_xon_xoff = 1;
        pk_UARTmng->port->SCR_IBCR |= 0x08;
        if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
            pk_UARTmng->port->FCR1 |= 0x02;
        }

        _ddr_mb9_uart_txi(pk_UARTmng);
    }
}

/*******************************************
        UART受信データフル割込み処理
 *******************************************/

void _ddr_mb9_uart_rxi(T_MB9_UART_MNG const *pk_UARTmng)
{
    UH rcvp;
    VB chr;
    UB ssr;

    if ((pk_UARTmng->data->rcnt < pk_UARTmng->cdata->rsize) ||
        (pk_UARTmng->data->RcvData != 0)) {
        for(; ; ) {
            if ((pk_UARTmng->port->SSR & 0x3C) != 0) {
                ssr = _ddr_mb9_uart_getssr(pk_UARTmng->port->SSR,pk_UARTmng);
                pk_UARTmng->data->ssr = ssr;
                chr = pk_UARTmng->port->RDR_TDR;

                if ((pk_UARTmng->data->status.bit.sft_flw == 1) && (chr == XON)) {
                    pk_UARTmng->data->status.bit.tx_xoff = 0;
                    pk_UARTmng->port->SCR_IBCR |= 0x08;
                    if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                        pk_UARTmng->port->FCR1 |= 0x02;
                    }
                    _ddr_mb9_uart_txi(pk_UARTmng);
                } else if ((pk_UARTmng->data->status.bit.sft_flw == 1) && (chr == XOFF)) {
                    pk_UARTmng->data->status.bit.tx_xoff = 1;
                    pk_UARTmng->port->SCR_IBCR &= ~0x08;

                    if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                        pk_UARTmng->port->FCR1 &= ~0x02;
                    }
                } else {
                    if (pk_UARTmng->data->RcvData != 0) {
                        pk_UARTmng->data->RcvData->rcnt--;
                        *pk_UARTmng->data->RcvData->rbuf++ = chr;
                        if (pk_UARTmng->data->RcvData->sbuf != 0)
                            *pk_UARTmng->data->RcvData->sbuf++ =  ssr;
                        if (_ddr_mb9_uart_check_chr(pk_UARTmng->data->RcvData, chr, ssr) == TRUE) {
                            pk_UARTmng->data->RcvData = 0;
                            isig_sem((ID)pk_UARTmng->cdata->rsemid);
                            if (pk_UARTmng->data->rcnt >= pk_UARTmng->cdata->rsize){
                                pk_UARTmng->port->SCR_IBCR &= ~0x10;
                                break;
                            }
                        }
                    } else if (pk_UARTmng->data->rcnt < pk_UARTmng->cdata->rsize) {
                        rcvp = pk_UARTmng->data->rcvp + pk_UARTmng->data->rcnt;
                        if (rcvp >= pk_UARTmng->cdata->rsize)
                            rcvp -= pk_UARTmng->cdata->rsize;
                        pk_UARTmng->cdata->rbuf[rcvp] = chr;
                        pk_UARTmng->cdata->sbuf[rcvp] = ssr;
                        if (pk_UARTmng->data->status.bit.sft_flw == 1)
                            _ddr_mb9_uart_chk_rxoff(pk_UARTmng);
                        if (++pk_UARTmng->data->rcnt == pk_UARTmng->cdata->rsize) {
                            pk_UARTmng->port->SCR_IBCR &= ~0x10;
                            break;
                        }
                    }
                }
            } else
                break;
        }
    } else
        pk_UARTmng->port->SCR_IBCR &= ~0x10;
}

/*******************************
        先頭文字送信処理
 *******************************/

void _ddr_mb9_uart_send_char(INT cnt, T_MB9_UART_MNG const *pk_UARTmng, T_COM_SND *TransmiteData)
{

    if (pk_UARTmng->data->status.bit.req_xon_xoff == 1) {
        pk_UARTmng->port->RDR_TDR = (pk_UARTmng->data->status.bit.rx_xoff == 0) ? XON : XOFF;
        pk_UARTmng->data->status.bit.req_xon_xoff = 0;
        cnt--;
    }
    if (pk_UARTmng->data->status.bit.tx_xoff == 0) {
        for(; cnt>0; cnt--) {
            if (TransmiteData->tcnt > 0) {
                pk_UARTmng->port->RDR_TDR = *TransmiteData->tbuf++;
                TransmiteData->tcnt--;
            } else {
                break;
            }
        }
    }
}

/***********************************************
        UART割込み処理(受信)
 ***********************************************/

void _ddr_mb9_ruart_intr(T_MB9_UART_MNG const *pk_UARTmng)
{

      _ddr_mb9_uart_rxi(pk_UARTmng);
}

/***********************************************
        UART割込み処理(送信)
 ***********************************************/
void _ddr_mb9_tuart_intr(T_MB9_UART_MNG const *pk_UARTmng)
{

      _ddr_mb9_uart_txi(pk_UARTmng);
}

/*******************************
        文字列の取り出し処理
 *******************************/

BOOL _ddr_mb9_uart_recv_strings(T_MB9_UART_MNG const *pk_UARTmng, T_COM_RCV *ReceiveData)
{
    BOOL flag;
    VB chr;
    UB sts;

    for (flag = FALSE; flag == FALSE; ) {
        if (ReceiveData->rcnt == 0)
            flag = TRUE;
        else if (pk_UARTmng->data->rcnt == 0)
            break;
        else {
            *ReceiveData->rbuf++ = chr = pk_UARTmng->cdata->rbuf[pk_UARTmng->data->rcvp];
            sts = pk_UARTmng->cdata->sbuf[pk_UARTmng->data->rcvp];
            if (ReceiveData->sbuf != 0)
                *ReceiveData->sbuf++ = sts;
            if (++pk_UARTmng->data->rcvp >= pk_UARTmng->cdata->rsize)
                pk_UARTmng->data->rcvp = 0;
            pk_UARTmng->data->rcnt--;
            ReceiveData->rcnt--;
            if (pk_UARTmng->data->status.bit.sft_flw == 1)
                _ddr_mb9_uart_chk_rxon(pk_UARTmng);
            flag = _ddr_mb9_uart_check_chr(ReceiveData, chr, sts);
        }
    }
    return flag;
}

/*******************************
        UART送信処理
 *******************************/

ER _ddr_mb9_uart_snd(T_COM_SND *TransmiteData, T_MB9_UART_MNG const *pk_UARTmng)
{
    ID tskid;
    ER ercd = E_OK;
    INT cnt;

    if ((pk_UARTmng->data->status.bit.init_flg == 0) || (sns_dpn() == TRUE))
        ercd = E_OBJ;
    else {
        get_tid(&tskid);
        dis_dsp();
        if (((pk_UARTmng->data->tlockid == 0) || (pk_UARTmng->data->tlockid == (UH)tskid)) &&
            (pk_UARTmng->data->status.bit.ena_tx == 1) &&
            (pk_UARTmng->data->SndData == 0)) {
            dis_int(pk_UARTmng->cdata->tintno);
            if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                cnt = SFIFOSIZE - pk_UARTmng->port->FBYTE1;
            } else {
                cnt = ((pk_UARTmng->port->SSR & 0x02) != 0) ? 1 : 0;
            }
            if ((pk_UARTmng->data->tcnt == 0) && (cnt > 0)){
                _ddr_mb9_uart_send_char(cnt, pk_UARTmng, TransmiteData);
            }
            if (_ddr_mb9_uart_copy(pk_UARTmng, TransmiteData) == FALSE) {
                pk_UARTmng->data->SndData = TransmiteData;
                pk_UARTmng->port->SCR_IBCR |= 0x08;
                if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                    pk_UARTmng->port->FCR1 |= 0x02;
                }
                ena_int(pk_UARTmng->cdata->tintno);
                ena_dsp();
                if ((ercd = twai_sem((ID)pk_UARTmng->cdata->tsemid, TransmiteData->time)) != E_OK) {
                    loc_cpu();
                    pk_UARTmng->port->SCR_IBCR &= ~0x08;
                    if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                        pk_UARTmng->port->FCR1 &= ~0x02;
                    }
                    pk_UARTmng->data->SndData = 0;
                    unl_cpu();
                    ercd = pol_sem((ID)pk_UARTmng->cdata->tsemid);
                }
            } else {
                if (pk_UARTmng->data->tcnt != 0) {
                    pk_UARTmng->port->SCR_IBCR |= 0x08;
                    if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
                        pk_UARTmng->port->FCR1 |= 0x02;
                    }
                }
                ena_int(pk_UARTmng->cdata->tintno);
                ena_dsp();
            }
        } else {
            ena_dsp();
            ercd = E_OBJ;
        }
    }
    return ercd;
}

/*******************************
        UART受信処理
 *******************************/

ER _ddr_mb9_uart_rcv(T_COM_RCV *ReceiveData, T_MB9_UART_MNG const *pk_UARTmng)
{
    ID tskid;
    ER ercd = E_OK;

    if ((pk_UARTmng->data->status.bit.init_flg == 0) || (sns_dpn() == TRUE) ||
        (pk_UARTmng->data->status.bit.ena_rx   == 0))
        ercd = E_OBJ;
    else {
        get_tid(&tskid);
        dis_dsp();
        if (((pk_UARTmng->data->rlockid == 0) || (pk_UARTmng->data->rlockid == (UH)tskid)) &&
            (pk_UARTmng->data->RcvData        == 0)) {
            dis_int(pk_UARTmng->cdata->rintno);
            if (_ddr_mb9_uart_recv_strings(pk_UARTmng, ReceiveData) == FALSE) {
                pk_UARTmng->data->RcvData = ReceiveData;
                pk_UARTmng->port->SCR_IBCR |= 0x10;
                ena_int(pk_UARTmng->cdata->rintno);
                ena_dsp();
                if ((ercd = twai_sem((ID)pk_UARTmng->cdata->rsemid, ReceiveData->time)) != E_OK) {
                    pk_UARTmng->data->RcvData = 0;
                    ercd = pol_sem((ID)pk_UARTmng->cdata->rsemid);
                }
            } else {
                pk_UARTmng->port->SCR_IBCR |= 0x10;
                ena_int(pk_UARTmng->cdata->rintno);
                ena_dsp();
            }
        } else {
            ena_dsp();
            ercd = E_OBJ;
        }
    }
    return ercd;
}

/***********************************
        送信バッファの送出処理
 ***********************************/

ER _ddr_mb9_uart_cln_tx_buf(T_MB9_UART_MNG const *pk_UARTmng, TMO time)
{
    ID tskid;
    ER ercd;
     
    get_tid(&tskid);
    if (((pk_UARTmng->data->tlockid == 0) ||
         (pk_UARTmng->data->tlockid == (UH)tskid)) &&
        ( pk_UARTmng->data->SndData == 0)) {
        loc_cpu();
        pk_UARTmng->port->SCR_IBCR |= 0x08;
        if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
            pk_UARTmng->port->FCR1 |= 0x02;
        }
        if (( pk_UARTmng->data->tcnt        != 0) ||
            ((pk_UARTmng->port->SSR & 0x02) == 0)) {
            pk_UARTmng->data->status.bit.cln_wait = 1;
            unl_cpu();
            ercd = twai_sem((ID)pk_UARTmng->cdata->tsemid, time);
            if (ercd != E_OK) {
                loc_cpu();
                pk_UARTmng->data->status.bit.cln_wait = 0;
                unl_cpu();
                ercd = pol_sem((ID)pk_UARTmng->cdata->tsemid);
            }
        } else {
            unl_cpu();
            ercd = E_OK;
        }

        for(;;) {
            ercd = dly_tsk(0);
            if(((pk_UARTmng->port->SSR & 0x01) != 0) || (ercd != E_OK)) {
                break;
            }
        }
    } else {
        ercd = E_OBJ;
    }
    return ercd;
}

/***************************************
        送信バッファのリセット処理
 ***************************************/

ER _ddr_mb9_uart_rst_tx_buf(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    if (((pk_UARTmng->data->tlockid == 0) ||
         (pk_UARTmng->data->tlockid == (UH)tskid)) &&
        ( pk_UARTmng->data->SndData == 0)) {    
        loc_cpu();

        pk_UARTmng->port->SCR_IBCR &= ~0x03;
        if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
            pk_UARTmng->port->FCR0 = pk_UARTmng->data->fcr0 | 0x04;
            pk_UARTmng->port->FCR0 = pk_UARTmng->data->fcr0;
        }
        pk_UARTmng->data->tcnt = 0;
        pk_UARTmng->port->SCR_IBCR |= 0x03;

        unl_cpu();
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/***************************************
        受信バッファのリセット処理
 ***************************************/

ER _ddr_mb9_uart_rst_rx_buf(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    if (((pk_UARTmng->data->rlockid == 0) ||
         (pk_UARTmng->data->rlockid == (UH)tskid)) &&
        ( pk_UARTmng->data->RcvData == 0)) {
        loc_cpu();
        
        pk_UARTmng->port->SCR_IBCR &= ~0x03;
        if ((pk_UARTmng->data->fcr0 & 0x01) != 0){
            pk_UARTmng->port->FCR0 = pk_UARTmng->data->fcr0 | 0x08;
            pk_UARTmng->port->FCR0 = pk_UARTmng->data->fcr0;
        }
        pk_UARTmng->data->rcnt = 0;
        pk_UARTmng->port->SCR_IBCR |= 0x03;

        unl_cpu();
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/*******************************
        送信の禁止処理
 *******************************/

ER _ddr_mb9_uart_dis_send(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    if (((pk_UARTmng->data->tlockid == 0) ||
         (pk_UARTmng->data->tlockid == (UH)tskid)) &&
        ( pk_UARTmng->data->SndData == 0) &&
        ( pk_UARTmng->data->tcnt    == 0)) {
        loc_cpu();
        pk_UARTmng->port->SCR_IBCR &= ~0x01;
        pk_UARTmng->data->status.bit.ena_tx = 0;
        unl_cpu();
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/*******************************
        受信の禁止処理
 *******************************/

ER _ddr_mb9_uart_dis_rcv(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    if (((pk_UARTmng->data->rlockid == 0) ||
         (pk_UARTmng->data->rlockid == (UH)tskid)) &&
        ( pk_UARTmng->data->RcvData == 0) &&
        ( pk_UARTmng->data->rcnt    == 0)) {
        loc_cpu();
        pk_UARTmng->data->status.bit.ena_rx = 0;
        pk_UARTmng->port->SCR_IBCR &= ~0x02;
        unl_cpu();
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/*******************************
        送信の許可処理
 *******************************/

ER _ddr_mb9_uart_ena_send(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    if ((pk_UARTmng->data->tlockid == 0) ||
        (pk_UARTmng->data->tlockid == (UH)tskid)) {
        loc_cpu();
        pk_UARTmng->data->status.bit.ena_tx = 1;
        pk_UARTmng->port->SCR_IBCR |= 0x01;
        unl_cpu();
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}

/*******************************
        受信の許可処理
 *******************************/

ER _ddr_mb9_uart_ena_rcv(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    if ((pk_UARTmng->data->rlockid == 0) ||
        (pk_UARTmng->data->rlockid == (UH)tskid)) {
        loc_cpu();
        pk_UARTmng->data->status.bit.ena_rx = 1;
        pk_UARTmng->port->SCR_IBCR |= 0x02;
        unl_cpu();
        ercd = E_OK;
    } else
        ercd = E_OBJ;
    return ercd;
}


/***********************************
        送信チャネルのロック処理
 ***********************************/

ER _ddr_mb9_uart_lock_trans(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    loc_cpu();
    if ((pk_UARTmng->data->tlockid == 0) ||
        (pk_UARTmng->data->SndData == 0)) {
        pk_UARTmng->data->tlockid = (UH)tskid;
        ercd = E_OK;
    } else if (pk_UARTmng->data->tlockid == (UH)tskid)
        ercd = E_OK;
    else
        ercd = E_OBJ;
    unl_cpu();
    return ercd;
}

/***********************************
        受信チャネルのロック処理
 ***********************************/

ER _ddr_mb9_uart_lock_recv(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    loc_cpu();
    if ((pk_UARTmng->data->rlockid == 0) ||
        (pk_UARTmng->data->RcvData == 0)) {
        pk_UARTmng->data->rlockid = (UH)tskid;
        ercd = E_OK;
    } else if (pk_UARTmng->data->rlockid == (UH)tskid)
        ercd = E_OK;
    else
        ercd = E_OBJ;
    unl_cpu();
    return ercd;
}

/***************************************
        送信チャネルのロック解除処理
 ***************************************/

ER _ddr_mb9_uart_unlock_trans(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    if (pk_UARTmng->data->tlockid == (UH)tskid) {
        pk_UARTmng->data->tlockid = 0;
        ercd = E_OK;
    } else if (pk_UARTmng->data->tlockid == 0)
        ercd = E_OK;
    else
        ercd = E_OBJ;
    return ercd;
}

/***************************************
        受信チャネルのロック解除処理
 ***************************************/

ER _ddr_mb9_uart_unlock_recv(T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;
    ID tskid;

    get_tid(&tskid);
    if (pk_UARTmng->data->rlockid == (UH)tskid) {
        pk_UARTmng->data->rlockid = 0;
        ercd = E_OK;
    } else if (pk_UARTmng->data->rlockid == 0)
        ercd = E_OK;
    else
        ercd = E_OBJ;
    return ercd;
}

/*******************************
        UART制御信号処理
 *******************************/

ER _ddr_mb9_uart_ctr(const T_COM_CTR *pk_SerialData, T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd = E_OK;

    if (pk_UARTmng->data->status.bit.init_flg == 0)
        ercd = E_OBJ;

    if (ercd == E_OK)
        if ((pk_SerialData->command & CLN_TXBUF) != 0)
            ercd = _ddr_mb9_uart_cln_tx_buf(pk_UARTmng, pk_SerialData->time);

    if (ercd == E_OK)
        if ((pk_SerialData->command & RST_TXBUF) != 0)
            ercd = _ddr_mb9_uart_rst_tx_buf(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & RST_RXBUF) != 0)
            ercd = _ddr_mb9_uart_rst_rx_buf(pk_UARTmng);
    
    if (ercd == E_OK)
        if ((pk_SerialData->command & STP_TX) != 0)
            ercd = _ddr_mb9_uart_dis_send(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & STP_RX) != 0)
            ercd = _ddr_mb9_uart_dis_rcv(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & STA_TX) != 0)
            ercd = _ddr_mb9_uart_ena_send(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & STA_RX) != 0)
            ercd = _ddr_mb9_uart_ena_rcv(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & LOC_TX) != 0)
            ercd = _ddr_mb9_uart_lock_trans(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & LOC_RX) != 0)
            ercd = _ddr_mb9_uart_lock_recv(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & UNL_TX) != 0)
            ercd = _ddr_mb9_uart_unlock_trans(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & UNL_RX) != 0)
            ercd = _ddr_mb9_uart_unlock_recv(pk_UARTmng);

    if (ercd == E_OK)
        if ((pk_SerialData->command & SND_BRK) != 0)
            ercd = E_PAR;

    if (ercd == E_OK)
        if ((pk_SerialData->command & ASR_RTS) != 0)
            ercd = E_PAR;

    if (ercd == E_OK)
        if ((pk_SerialData->command & NGT_RTS) != 0)
            ercd = E_PAR;

    if (ercd == E_OK)
        if ((pk_SerialData->command & ASR_DTR) != 0)
            ercd = E_PAR;

    if (ercd == E_OK)
        if ((pk_SerialData->command & NGT_DTR) != 0)
            ercd = E_PAR;
    
    return ercd;
}

/*******************************
        UART状態参照処理
 *******************************/

ER _ddr_mb9_uart_ref(T_COM_REF *pk_SerialRef, T_MB9_UART_MNG const *pk_UARTmng)
{
    UH status = 0;

    if (pk_UARTmng->data->status.bit.init_flg == 1) {
        pk_SerialRef->rxcnt = pk_UARTmng->data->rcnt;
        pk_SerialRef->txcnt = pk_UARTmng->data->tcnt;

        status |= T_COM_INIT;
        if ((pk_UARTmng->data->ssr & T_COM_EROR) != 0)
            status |= T_COM_EROR;
        if ((pk_UARTmng->data->ssr & T_COM_ERP) != 0)
            status |= T_COM_ERP;
        if ((pk_UARTmng->data->ssr & T_COM_ERF) != 0)
            status |= T_COM_ERF;
        if (pk_UARTmng->data->status.bit.ena_tx == 1)
            status |= T_COM_ENATX;
        if (pk_UARTmng->data->status.bit.ena_rx == 1)
            status |= T_COM_ENARX;
        if (pk_UARTmng->data->status.bit.rx_xoff == 1)
            status |= T_COM_RXOFF;
        if (pk_UARTmng->data->status.bit.tx_xoff == 1)
            status |= T_COM_TXOFF;
    }
    pk_SerialRef->status = status;
    return E_OK;
}

/*******************************
        UART初期化処理
 *******************************/

ER _ddr_mb9_uart_ini(const T_COM_SMOD *pk_SerialMode, T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd = E_OK;
    T_RSEM pk_rsem;
    T_MB9_UART_MSTS status;

    UH brr;
    UB smr;
    UB escr;
    UB fcr0 = 0;
    UB fcr1 = 0;
    UB fbyte2 = 0;
#ifdef LPMODE
     T_REFCLK rclk;
#endif
    
    if (pk_SerialMode != 0) {

        if (pk_UARTmng->data->status.bit.init_flg == 0) {
            status.word = 0;
#ifdef LPMODE
            _ddr_mb9a0b1k_ref_clk(CLK_PCLK_CLOCK, &rclk);
            brr =  rclk.frq / pk_SerialMode->baud - 1;
#else
            brr = pk_UARTmng->cdata->pclock / pk_SerialMode->baud - 1;
#endif
            if ((brr < 4) || (0x7FFF < brr)){
                 ercd = E_PAR;
            }

            smr = 0;
            escr = 0;     
            switch(pk_SerialMode->blen) {
                case BLEN7:
                    escr |= 0x03;
                    break;
                case BLEN6:
                    escr |= 0x02;
                    break;
                case BLEN5:
                    escr |= 0x01;
                    break;                
                case BLEN8:
                    break;                
                default:
                    ercd = E_PAR;
                    break;
            }

            switch(pk_SerialMode->par) {
                case PAR_EVEN:
                    escr |= 0x10;
                    break;
                case PAR_ODD:
                    escr |= 0x18;
                    break;
                case PAR_NONE:
                    break;                
                default:
                    ercd = E_PAR;
                    break;
            }
            
            switch(pk_SerialMode->sbit) {
                case SBIT2:
                    smr |= 0x08;
                    break;
                case SBIT1:
                    break;
                case SBIT15:
                default:
                    ercd = E_PAR;
                    break;
            }

            switch(pk_SerialMode->flow) {
                case FLW_HARD:
#ifdef UART_4
                    if (pk_UARTmng->port == &REG_MFS4) {
                        status.bit.hrd_flw = 1;
                        escr |= 0x80;
                    } else {
                        ercd = E_PAR;
                    }
#else
                    ercd = E_PAR;
#endif
                    break;
                case FLW_XON:
                    status.bit.sft_flw = 1;
                    break;
                case FLW_NONE:
                    break;
                default:
                    ercd = E_PAR;
                    break;
            }

            if (ercd == E_OK) {
                loc_cpu();
                status.bit.init_flg = 1;
                pk_UARTmng->data->status.word = status.word;

                pk_UARTmng->port->SMR |= 0x01; /* シリアルデータ出力許可ビット */
                            
                pk_UARTmng->port->SMR |= smr;
                pk_UARTmng->port->ESCR_IBSR |= escr;
                pk_UARTmng->port->BGR = brr;

                pk_UARTmng->port->SCR_IBCR |= 0x80;  /* プログラマブルクリア */ 

                if ((pk_UARTmng->data->fcr0 & 0x03) != 0){
                    pk_UARTmng->port->FBYTE1 = pk_UARTmng->data->fbyte1;
                    pk_UARTmng->port->FBYTE2 = pk_UARTmng->data->fbyte2;

                    pk_UARTmng->port->FCR0 = pk_UARTmng->data->fcr0;
                    pk_UARTmng->port->FCR1 = pk_UARTmng->data->fcr1;
                }
                
                ena_int(pk_UARTmng->cdata->rintno);
                ena_int(pk_UARTmng->cdata->tintno);
                unl_cpu();
            }
            
        } else {
            ercd = E_OBJ;
        }
      
    } else {
        if (pk_UARTmng->data->status.bit.init_flg == 1) {
            loc_cpu();
            pk_UARTmng->port->SCR_IBCR = 0x00;
            pk_UARTmng->port->SMR = 0x00;
            pk_UARTmng->port->ESCR_IBSR = 0x00;
            pk_UARTmng->port->BGR = 0x0000;
            if ((pk_UARTmng->data->fcr0 & 0x03) != 0){
                pk_UARTmng->port->FCR0 |= 0x0C;
                pk_UARTmng->port->FCR0 = 0x00;
                pk_UARTmng->port->FCR1 = 0x00;
                fcr0 = pk_UARTmng->data->fcr0;
                fcr1 = pk_UARTmng->data->fcr1;
                fbyte2 = pk_UARTmng->data->fbyte2;
            }
            if ((pk_UARTmng->data->SndData != 0) && (pk_UARTmng->data->status.bit.cln_wait != 0)) {
                if (ref_sem((ID)pk_UARTmng->cdata->tsemid, &pk_rsem) == E_OK) {
                    if (pk_rsem.wtskid != 0) {
                        rel_wai(pk_rsem.wtskid);
                    }
                    for(; pk_rsem.semcnt > 0; pk_rsem.semcnt--) {
                        pol_sem((ID)pk_UARTmng->cdata->tsemid);
                    }
                }
            }
            if (pk_UARTmng->data->RcvData == 0) {
                if (ref_sem((ID)pk_UARTmng->cdata->rsemid, &pk_rsem) == E_OK) {
                    if (pk_rsem.wtskid != 0) {
                        rel_wai(pk_rsem.wtskid);
                    }
                    for(; pk_rsem.semcnt > 0; pk_rsem.semcnt--) {
                        pol_sem((ID)pk_UARTmng->cdata->rsemid);
                    }
                }
            }
            dis_int(pk_UARTmng->cdata->tintno);
            dis_int(pk_UARTmng->cdata->rintno);
            memset(pk_UARTmng->data, 0x00, sizeof(T_MB9_UART_DMNG));
            if ((fcr0 & 0x03) != 0){
                pk_UARTmng->data->fcr0 = fcr0;
                pk_UARTmng->data->fcr1 = fcr1;
                pk_UARTmng->data->fbyte2 = fbyte2;
            }
            unl_cpu();
        }

    }
    return ercd;
}

/*******************************
      UARTドライバメイン
 *******************************/

ER _ddr_mb9_uart_drv(ID FuncID, VP pk_ControlData, T_MB9_UART_MNG const *pk_UARTmng)
{
    ER ercd;

    switch(FuncID) {
        case TA_COM_INI:
            ercd = _ddr_mb9_uart_ini((const T_COM_SMOD *)pk_ControlData, pk_UARTmng);
            break;
        case TA_COM_REF:
            ercd = _ddr_mb9_uart_ref((T_COM_REF *)pk_ControlData, pk_UARTmng);
            break;
        case TA_COM_CTR:
            ercd = _ddr_mb9_uart_ctr((const T_COM_CTR *)pk_ControlData, pk_UARTmng);
            break;
        case TA_COM_PUTS:
            ercd = _ddr_mb9_uart_snd((T_COM_SND *)pk_ControlData, pk_UARTmng);
            break;
        case TA_COM_GETS:
            ercd = _ddr_mb9_uart_rcv((T_COM_RCV *)pk_ControlData, pk_UARTmng);
            break;
        default: ercd = E_NOSPT;
    }
    return ercd;
}
#endif

