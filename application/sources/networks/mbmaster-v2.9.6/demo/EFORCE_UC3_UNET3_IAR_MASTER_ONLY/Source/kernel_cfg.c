/***********************************************************************
    Kernel configuration

 ***********************************************************************/

#include "kernel.h"
#include "Cortex-M3.h"

extern void _ddr_init(void);

/*******************************
  Inner initialize function
 *******************************/
void _kernel_initial(void)
{
    _kernel_change_level(0);
    _ddr_init();
}

/*******************************
  Various management table
 *******************************/

UB const _kernel_atrtbl[] = {
0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x42, 
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x50, 
0x50, 0x50, 0x50, 0x62, 0x62, 0x60, 0x60, 0x60, 0x60, 0x90, 
0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 
0x90, 0xa0, 0x30, };

extern void inti_tsk(VP_INT exinf);
long long _kernel_task1_stk[0x400/sizeof(long long)];
T_CTSK const _kernel_task1 = {0,
            (FP)inti_tsk, 0x400, (VP)_kernel_task1_stk, 0, 1};

extern void dev_snd_tsk(VP_INT exinf);
long long _kernel_task2_stk[0x200/sizeof(long long)];
T_CTSK const _kernel_task2 = {0,
            (FP)dev_snd_tsk, 0x200, (VP)_kernel_task2_stk, 0, 2};

extern void dev_rcv_tsk(VP_INT exinf);
long long _kernel_task3_stk[0x300/sizeof(long long)];
T_CTSK const _kernel_task3 = {0,
            (FP)dev_rcv_tsk, 0x300, (VP)_kernel_task3_stk, 0, 2};

extern void dev_ctl_tsk(VP_INT exinf);
long long _kernel_task4_stk[0x200/sizeof(long long)];
T_CTSK const _kernel_task4 = {0,
            (FP)dev_ctl_tsk, 0x200, (VP)_kernel_task4_stk, 0, 2};

extern void net_tim_tsk(VP_INT exinf);
long long _kernel_task5_stk[0x200/sizeof(long long)];
T_CTSK const _kernel_task5 = {0,
            (FP)net_tim_tsk, 0x200, (VP)_kernel_task5_stk, 0, 2};

extern void update_tsk(VP_INT exinf);
long long _kernel_task6_stk[0x100/sizeof(long long)];
T_CTSK const _kernel_task6 = {0,
            (FP)update_tsk, 0x100, (VP)_kernel_task6_stk, 0, 3};

extern void serial_snd_rcv_tsk(VP_INT exinf);
long long _kernel_task7_stk[0x100/sizeof(long long)];
T_CTSK const _kernel_task7 = {0,
            (FP)serial_snd_rcv_tsk, 0x100, (VP)_kernel_task7_stk, 0, 4};

extern void lan_snd_rcv_tsk(VP_INT exinf);
long long _kernel_task8_stk[0x100/sizeof(long long)];
T_CTSK const _kernel_task8 = {0,
            (FP)lan_snd_rcv_tsk, 0x100, (VP)_kernel_task8_stk, 0, 4};

extern void cnf_mgt_tsk(VP_INT exinf);
long long _kernel_task9_stk[0x100/sizeof(long long)];
T_CTSK const _kernel_task9 = {0,
            (FP)cnf_mgt_tsk, 0x100, (VP)_kernel_task9_stk, 0, 4};

extern void task_mbmaster(VP_INT exinf);
long long _kernel_task10_stk[0x400/sizeof(long long)];
T_CTSK const _kernel_task10 = {0,
            (FP)task_mbmaster, 0x400, (VP)_kernel_task10_stk, 0, 1};

T_CSEM const _kernel_sem1 = {1, 1};

T_CSEM const _kernel_sem2 = {0, 1};

T_CSEM const _kernel_sem3 = {0, 1};

T_CSEM const _kernel_sem4 = {0, 1};

T_CFLG const _kernel_flg1 = {0x0};

T_CFLG const _kernel_flg2 = {0x0};

T_CFLG const _kernel_flg3 = {0x0};

T_CFLG const _kernel_flg4 = {0x0};

T_CFLG const _kernel_flg5 = {0x0};

T_CFLG const _kernel_flg6 = {0x0};

long long _kernel_mpf1_buf[(8*1576)/sizeof(long long)];
T_CMPF const _kernel_mpf1 = {8, 1576, (VP)_kernel_mpf1_buf};

extern void cyc_func1(VP_INT exinf);
T_CCYC const _kernel_cyc1 = {0, (FP)cyc_func1, 5, 0};

void const * const _kernel_inftbl[] = {
(void const *)&_kernel_task1,
(void const *)&_kernel_task2,
(void const *)&_kernel_task3,
(void const *)&_kernel_task4,
(void const *)&_kernel_task5,
(void const *)&_kernel_task6,
(void const *)&_kernel_task7,
(void const *)&_kernel_task8,
(void const *)&_kernel_task9,
(void const *)&_kernel_task10,
(void const *)&_kernel_sem1,
(void const *)&_kernel_sem2,
(void const *)&_kernel_sem3,
(void const *)&_kernel_sem4,
(void const *)&_kernel_flg1,
(void const *)&_kernel_flg2,
(void const *)&_kernel_flg3,
(void const *)&_kernel_flg4,
(void const *)&_kernel_flg5,
(void const *)&_kernel_flg6,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
(void const *)&_kernel_mpf1,
(void const *)&_kernel_cyc1,
};

T_TCB _kernel_tcb[10];
UB _kernel_sem[4];
FLGPTN _kernel_flg[6];
T_MBX _kernel_mbx[12];
T_MPF _kernel_mpf[1];
T_CYC _kernel_cyc[1];
T_WTID _kernel_waique[42];

__root const UB _kernel_nm_right = 1;
VB const * const _kernel_objname[] = {
(VB const *)0,
(VB const *)0,
(VB const *)0,
(VB const *)0,
(VB const *)0,
(VB const *)0,
(VB const *)0,
(VB const *)0,
(VB const *)0,
"ID_INIT_TSK",
"ID_ETH_SND_TSK",
"ID_ETH_RCV_TSK",
"ID_ETH_CTL_TSK",
"ID_TCP_TIM_TSK",
"ID_UPDATE_TSK",
"ID_SERIAL_SND_RCV_TSK",
"ID_LAN_SND_RCV_TSK",
"ID_CNF_MGT_TSK",
"ID_TASK_MBMASTER",
"ID_TCP_SEM",
"ID_UART0_TSEM",
"ID_UART0_RSEM",
"ID_MODBUS_SEM",
"ID_ETH_RCV_FLG",
"ID_ETH_SND_FLG",
"ID_POWERUP_FLG",
"ID_TIMER_START_FLG",
"ID_MODBUS_FLG1",
"ID_MODBUS_FLG2",
"ID_ETH_SND_MBX",
"ID_ETH_RCV_MBX",
"ID_SERIAL_LAN_CVT_MBX",
"ID_LAN_SERIAL_CVT_MBX",
"ID_SERIAL_SND_MBX",
"ID_SERIAL_RCV_MBX",
"ID_LAN_SND_MBX",
"ID_LAN_RCV_MBX",
"ID_CNF_REQ_MBX",
"ID_CNF_OUTPUT_MBX",
"ID_UPDATE_REQ_MBX",
"ID_UPDATE_ANS_MBX",
"ID_TCP_MPF",
"ID_CYC1",
};

VP const _kernel_ctrtbl[] = {
(VP)&_kernel_tcb[0],
(VP)&_kernel_tcb[1],
(VP)&_kernel_tcb[2],
(VP)&_kernel_tcb[3],
(VP)&_kernel_tcb[4],
(VP)&_kernel_tcb[5],
(VP)&_kernel_tcb[6],
(VP)&_kernel_tcb[7],
(VP)&_kernel_tcb[8],
(VP)&_kernel_tcb[9],
(VP)&_kernel_sem[0],
(VP)&_kernel_sem[1],
(VP)&_kernel_sem[2],
(VP)&_kernel_sem[3],
(VP)&_kernel_flg[0],
(VP)&_kernel_flg[1],
(VP)&_kernel_flg[2],
(VP)&_kernel_flg[3],
(VP)&_kernel_flg[4],
(VP)&_kernel_flg[5],
(VP)&_kernel_mbx[0],
(VP)&_kernel_mbx[1],
(VP)&_kernel_mbx[2],
(VP)&_kernel_mbx[3],
(VP)&_kernel_mbx[4],
(VP)&_kernel_mbx[5],
(VP)&_kernel_mbx[6],
(VP)&_kernel_mbx[7],
(VP)&_kernel_mbx[8],
(VP)&_kernel_mbx[9],
(VP)&_kernel_mbx[10],
(VP)&_kernel_mbx[11],
(VP)&_kernel_mpf[0],
(VP)&_kernel_cyc[0],
};

T_CNSTBL const _kernel_cnstbl = {
_kernel_atrtbl,
_kernel_inftbl-9,
0,
0,
_kernel_waique,
_kernel_ctrtbl-9,
_kernel_objname,
TKERNEL_PRID,
TKERNEL_PRVER,
1,
8,
42
};

/* end */
