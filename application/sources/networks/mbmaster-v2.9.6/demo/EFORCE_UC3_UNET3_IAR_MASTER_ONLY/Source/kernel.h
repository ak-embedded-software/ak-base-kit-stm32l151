/***************************************************************************
    MICRO C CUBE / COMPACT, KERNEL
    common definitions
    Copyright (c)  2008-2012, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.04.19: Created.
            2008.08.27: Corrected the error in writing.
            2008.12.09: Moved product version.
            2010.04.09: Modified the TA_FPU attribute.
            2012.05.10: Corresponded to the kernel version 2.
            2012.05.18: Added the function code.
            2012.09.19: Modified TSZ_MPF macro.
 ***************************************************************************/

#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "itron.h"

#ifndef _KERNEL_MPE_LEVEL_
#define _KERNEL_MPE_LEVEL_  0
#endif

#ifdef _UC3SYS_H_
#define EXTERN
#else
#define EXTERN  extern
#endif

/***********************************
        パケット形式
 ***********************************/

/* タスク管理機能 */

typedef struct t_rtsk {
    STAT        tskstat;
    PRI         tskpri;
    PRI         tskbpri;
    STAT        tskwait;
    ID          wobjid;
    TMO         lefttmo;
    UINT        actcnt;
    UINT        wupcnt;
    UINT        suscnt;
    VB    const *name;
} T_RTSK;

typedef struct t_rtst {
    STAT        tskstat;
    STAT        tskwait;
} T_RTST;

typedef struct t_ctsk {
    VP_INT      exinf;
    FP          task;
    SIZE        stksz;
    VP          stk;
    ID          stkno;
    PRI         itskpri;
} T_CTSK;

typedef struct t_tcb {
    RELTIM      rtime;
    TID         nid;
    TID         pid;
    UB          msts;
    UB          catr;
    TID         wobjid;
#ifdef _kernel_LARGE
    union {
        T_REG   *sp;
        T_PAR   *par;
    } ctx;
    UB          cpri;
    UB          wup;
    UB          act;
#else
    UB          cpri;
    UB          wup;
    UB          act;
    union {
        T_REG   *sp;
        T_PAR   *par;
    } ctx;
#endif
    TCB_DEPENDEND
} T_TCB;

/* 同期・通信機能（セマフォ） */

typedef struct t_rsem {
    ID          wtskid;
    UINT        semcnt;
    VB    const *name;
} T_RSEM;

typedef struct t_csem {
    UB          isemcnt;
    UB          maxsem;
}T_CSEM;

typedef struct t_sem {
    UB          semcnt;
} T_SEM;

/* 同期・通信機能（イベントフラグ） */

typedef struct t_rflg {
    ID          wtskid;
    FLGPTN      flgptn;
    VB    const *name;
} T_RFLG;

typedef struct t_cflg {
    FLGPTN      iflgptn;
} T_CFLG;

typedef struct t_flg {
    FLGPTN      flgptn;
} T_FLG;

/* 同期・通信機能（データキュー） */

typedef struct t_rdtq {
    ID          stskid;
    ID          rtskid;
    UINT        sdtqcnt;
    VB    const *name;
} T_RDTQ;

typedef struct t_cdtq {
    UINT        dtqcnt;
    VP          dtq;
} T_CDTQ;

typedef struct t_dtq {
    UH          put;
    UH          cnt;
} T_DTQ;

/* 同期・通信機能（メールボックス） */

typedef struct t_mem {
    struct t_mem    *next;
} T_MEM;

typedef struct t_msg {
    struct t_msg    *msgque;
} T_MSG;

typedef struct t_rmbx {
    ID          wtskid;
    T_MSG       *pk_msg;
    VB    const *name;
} T_RMBX;

typedef struct t_mbx {
    T_MEM       *top;
    T_MEM       *btm;
} T_MBX;

/* メモリプール管理機能（固定長メモリプール） */

typedef struct t_rmpf {
    ID          wtskid;
    UINT        fblkcnt;
    VB    const *name;
} T_RMPF;

typedef struct t_cmpf {
    UINT        blkcnt;
    UINT        blksz;
    VP          mpf;
} T_CMPF;

typedef struct t_mpf {
    T_MEM       *top;
    UINT        blkcnt;
} T_MPF;

/* 時間管理機能（周期ハンドラ） */

typedef struct t_rcyc {
    STAT        cycstat;
    RELTIM      lefttim;
    VB    const *name;
} T_RCYC;

typedef struct t_ccyc {
    VP_INT      exinf;
    FP          cychdr;
    RELTIM      cyctim;
    RELTIM      cycphs;
} T_CCYC;

typedef struct t_cyc {
    RELTIM      rtime;
    TID         nid;
    TID         pid;
    UB          msts;
} T_CYC;

/* システム管理機能 */

typedef struct t_rsys {
    UINT        dummy;
} T_RSYS;

/* システム構成管理機能 */

typedef struct t_rcfg {
    UH          tick;
    UH          tskpri_max;
    UH          id_max;
} T_RCFG;

typedef struct t_rver {
    UH          maker;
    UH          prid;
    UH          spver;
    UH          prver;
    UH          prno[4];
} T_RVER;

/* システム管理 */

typedef struct t_wtid {
    TID         nid;
    TID         pid;
} T_WTID;

typedef struct t_cnstbl {
    UB   const *atrtbl;
    void const * const *inftbl;
    FP          ctrtim;
    FP          sysidl;
    T_WTID      *waique;
    VP   const *ctrtbl;
    VB   const * const *objname;
    UH          prid;
    UH          prver;
    UH          tick;
    TID         tskpri_max;
    TID         id_max;
    CNSTBL_DEPENDEND
} T_CNSTBL;

/***********************************
        定数
 ***********************************/

/* オブジェクト属性 */

#define TA_NULL     0u
#define TA_HLNG     0x00u
#define TA_USR      0x01u
#define TA_ACT      0x02u
#define TA_RSTR     0x04u

#define TA_FPU      0x08u

#define TA_TFIFO    0x00u
#define TA_TPRI     0x01u

#define TA_MFIFO    0x00u
#define TA_MPRI     0x02u

#define TA_WSGL     0x00u
#define TA_WMUL     0x02u
#define TA_CLR      0x04u

#define TA_INHERIT  0x02u
#define TA_CEILING  0x03u

#define TA_STA      0x02u
#define TA_PHS      0x04u

/* タイムアウト指定 */

#define TMO_POL     0L
#define TMO_FEVR    -1L

/* システムコールの動作モード */

#define TWF_ANDW    0x00u
#define TWF_ORW     0x01u

/* オブジェクトの状態 */

#define TTS_RUN     0x01u
#define TTS_RDY     0x02u
#define TTS_WAI     0x04u
#define TTS_DMT     0x10u

#define TTW_SLP     0x0001u
#define TTW_DLY     0x0002u
#define TTW_SEM     0x0004u
#define TTW_FLG     0x0008u
#define TTW_SDTQ    0x0010u
#define TTW_RDTQ    0x0020u
#define TTW_MBX     0x0040u
#define TTW_MTX     0x0080u
#define TTW_SMBF    0x0100u
#define TTW_RMBF    0x0200u
#define TTW_CAL     0x0400u
#define TTW_ACP     0x0800u
#define TTW_RDV     0x1000u
#define TTW_MPF     0x2000u
#define TTW_MPL     0x4000u
#define TTW_STK     0x8000u

#define TTEX_ENA    0x00u
#define TTEX_DIS    0x01u

#define TCYC_STP    0x00u
#define TCYC_STA    0x01u

#define TALM_STP    0x00u
#define TALM_STA    0x01u

#define TOVR_STP    0x00u
#define TOVR_STA    0x01u

/* その他の定数 */

#define TSK_SELF    0
#define TSK_NONE    0

#define TPRI_SELF   0
#define TPRI_INI    0

#define TKERNEL_MAKER   0x0000u
#define TKERNEL_SPVER   0x5403u


/***********************************
        構成定数とマクロ
 ***********************************/

/* キューイング/ネスト回数の最大値 */

#define TMAX_ACTCNT 255
#define TMAX_WUPCNT 255
#define TMAX_MAXSEM 255

/* ビットパターンのビット数 */

#define TBIT_FLGPTN _kernel_INT_BIT

/* 必要なメモリ領域のサイズ */

#define TSZ_DTQ(i)      ((i)*(sizeof(VP_INT)))

#define TSZ_MPF(i,j)    ((i)*(((j)+(_kernel_ALIGN_SIZE-1))&(~(_kernel_ALIGN_SIZE-1))))


/***********************************
        エラーコード
 ***********************************/

#define E_SYS       -5          /* 0xFFFB */
#define E_NOSPT     -9          /* 0xFFF7 */
#define E_RSFN      -10         /* 0xFFF6 */
#define E_RSATR     -11         /* 0xFFF5 */
#define E_PAR       -17         /* 0xFFEF */
#define E_ID        -18         /* 0xFFEE */
#define E_CTX       -25         /* 0xFFE7 */
#define E_MACV      -26         /* 0xFFE6 */
#define E_OACV      -27         /* 0xFFE5 */
#define E_ILUSE     -28         /* 0xFFE4 */
#define E_NOMEM     -33         /* 0xFFDF */
#define E_NOID      -34         /* 0xFFDE */
#define E_OBJ       -41         /* 0xFFD7 */
#define E_NOEXS     -42         /* 0xFFD6 */
#define E_QOVR      -43         /* 0xFFD5 */
#define E_RLWAI     -49         /* 0xFFCF */
#define E_TMOUT     -50         /* 0xFFCE */
#define E_DLT       -51         /* 0xFFCD */
#define E_CLS       -52         /* 0xFFCC */
#define E_WBLK      -57         /* 0xFFC7 */
#define E_BOVR      -58         /* 0xFFC6 */


/***********************************
        機能コード
 ***********************************/

#define TFN_ACT_TSK     -0x07   /* act_tsk                                  */
#define TFN_IACT_TSK    -0x71   /* iact_tsk                                 */
#define TFN_STA_TSK     -0x09   /* sta_tsk                                  */
#define TFN_ROT_RDQ     -0x55   /* rot_rdq                                  */
#define TFN_IROT_RDQ    -0x79   /* irot_rdq                                 */
#define TFN_SET_FLG     -0x2B   /* set_flg                                  */
#define TFN_ISET_FLG    -0x76   /* iset_flg                                 */
#define TFN_IVSIG_OVR   -0xF7   /* ivsig_ovr                                */
#define TFN_SIG_SEM     -0x23   /* sig_sem                                  */
#define TFN_ISIG_SEM    -0x75   /* isig_sem                                 */
#define TFN_ISIG_TIM    -0x7D   /* isig_tim                                 */
#define TFN_PSND_DTQ    -0x36   /* psnd_dtq                                 */
#define TFN_FSND_DTQ    -0x38   /* fsnd_dtq                                 */
#define TFN_IPSND_DTQ   -0x77   /* ipsnd_dtq                                */
#define TFN_IFSND_DTQ   -0x78   /* ifsnd_dtq                                */
#define TFN_WUP_TSK     -0x13   /* wup_tsk                                  */
#define TFN_IWUP_TSK    -0x72   /* iwup_tsk                                 */
#define TFN_REL_WAI     -0x15   /* rel_wai                                  */
#define TFN_IREL_WAI    -0x73   /* irel_wai                                 */
#define TFN_REL_MPF     -0x47   /* rel_mpf                                  */
#define TFN_GET_MPF     -0x49   /* get_mpf                                  */
#define TFN_PGET_MPF    -0x4A   /* pget_mpf                                 */
#define TFN_TGET_MPF    -0x4B   /* tget_mpf                                 */
#define TFN_REL_MPL     -0xA3   /* rel_mpl                                  */
#define TFN_GET_MPL     -0xA5   /* get_mpl                                  */
#define TFN_PGET_MPL    -0xA6   /* pget_mpl                                 */
#define TFN_TGET_MPL    -0xA7   /* tget_mpl                                 */


/***************************************
        システム初期化スタート関数
 ***************************************/

EXTERN  ER      start_uC3(void);

#ifdef _UC3SYS_H_
extern T_CNSTBL const _kernel_cnstbl;
extern void _kernel_initial(void);
#else
void _kernel_initial(void);
#endif

/***********************************
        システムコール
 ***********************************/

/* タスク管理機能 */

EXTERN  ER      iact_tsk(ID tskid);
#define iact_tsk(p1)    (act_tsk(p1))
EXTERN  ER      act_tsk(ID tskid);
EXTERN  ER_UINT can_act(ID tskid);
EXTERN  ER      sta_tsk(ID tskid, VP_INT stacd);
EXTERN  void    ext_tsk(void);
EXTERN  ER      ter_tsk(ID tskid);
EXTERN  ER      chg_pri(ID tskid, PRI tskpri);
EXTERN  ER      get_pri(ID tskid, PRI *p_tskpri);
EXTERN  ER      ref_tsk(ID tskid, T_RTSK *pk_rtsk);
EXTERN  ER      ref_tst(ID tskid, T_RTST *pk_rtst);

/* タスク付属同期機能 */

EXTERN  ER      slp_tsk(void);
#define slp_tsk()   (tslp_tsk(TMO_FEVR))
EXTERN  ER      tslp_tsk(TMO tmout);
EXTERN  ER      iwup_tsk(ID tskid);
#define iwup_tsk(p1)    (wup_tsk(p1))
EXTERN  ER      wup_tsk(ID tskid);
EXTERN  ER_UINT can_wup(ID tskid);
EXTERN  ER      irel_wai(ID tskid);
#define irel_wai(p1)    (rel_wai(p1))
EXTERN  ER      rel_wai(ID tskid);
EXTERN  ER      dly_tsk(RELTIM dlytim);

/* 同期・通信機能（セマフォ） */

EXTERN  ER      isig_sem(ID semid);
#define isig_sem(p1)    (sig_sem(p1))
EXTERN  ER      sig_sem(ID semid);
EXTERN  ER      wai_sem(ID semid);
EXTERN  ER      pol_sem(ID semid);
#define pol_sem(p1) (twai_sem((p1),(TMO_POL)))
#define wai_sem(p1) (twai_sem((p1),(TMO_FEVR)))
EXTERN  ER      twai_sem(ID semid, TMO tmout);
EXTERN  ER      ref_sem(ID semid, T_RSEM *pk_rsem);

/* 同期・通信機能（イベントフラグ） */

EXTERN  ER      iset_flg(ID flgid, FLGPTN setptn);
#define iset_flg(p1,p2) (set_flg((p1),(p2)))
EXTERN  ER      set_flg(ID flgid, FLGPTN setptn);
EXTERN  ER      clr_flg(ID flgid, FLGPTN clrptn);
EXTERN  ER      wai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn);
EXTERN  ER      pol_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn);
#define pol_flg(p1,p2,p3,p4)    (twai_flg((p1),(p2),(p3),(p4),(TMO_POL)))
#define wai_flg(p1,p2,p3,p4)    (twai_flg((p1),(p2),(p3),(p4),(TMO_FEVR)))
EXTERN  ER      twai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout);
EXTERN  ER      ref_flg(ID flgid, T_RFLG *pk_rflg);

/* 同期・通信機能（データキュー） */

EXTERN  ER      ipsnd_dtq(ID dtqid, VP_INT data);
EXTERN  ER      snd_dtq(ID dtqid, VP_INT data);
EXTERN  ER      psnd_dtq(ID dtqid, VP_INT data);
#define ipsnd_dtq(p1,p2) (tsnd_dtq((p1),(p2),(TMO_POL)))
#define psnd_dtq(p1,p2)  (tsnd_dtq((p1),(p2),(TMO_POL)))
#define snd_dtq(p1,p2)   (tsnd_dtq((p1),(p2),(TMO_FEVR)))
EXTERN  ER      tsnd_dtq(ID dtqid, VP_INT data, TMO tmout);
EXTERN  ER      ifsnd_dtq(ID dtqid, VP_INT data);
#define ifsnd_dtq(p1,p2) (fsnd_dtq((p1),(p2)))
EXTERN  ER      fsnd_dtq(ID dtqid, VP_INT data);
EXTERN  ER      rcv_dtq(ID dtqid, VP_INT *p_data);
EXTERN  ER      prcv_dtq(ID dtqid, VP_INT *p_data);
#define prcv_dtq(p1,p2) (trcv_dtq((p1),(p2),(TMO_POL)))
#define rcv_dtq(p1,p2)  (trcv_dtq((p1),(p2),(TMO_FEVR)))
EXTERN  ER      trcv_dtq(ID dtqid, VP_INT *p_data, TMO tmout);
EXTERN  ER      ref_dtq(ID dtqid, T_RDTQ *pk_rdtq);

/* 同期・通信機能（メールボックス） */

EXTERN  ER      snd_mbx(ID mbxid, T_MSG *pk_msg);
EXTERN  ER      rcv_mbx(ID mbxid, T_MSG **ppk_msg);
EXTERN  ER      prcv_mbx(ID mbxid, T_MSG **ppk_msg);
#define prcv_mbx(p1,p2) (trcv_mbx((p1),(p2),(TMO_POL)))
#define rcv_mbx(p1,p2)  (trcv_mbx((p1),(p2),(TMO_FEVR)))
EXTERN  ER      trcv_mbx(ID mbxid, T_MSG **ppk_msg, TMO tmout);
EXTERN  ER      ref_mbx(ID mbxid, T_RMBX *pk_rmbx);

/* メモリプール管理機能（固定長メモリプール） */

EXTERN  ER      get_mpf(ID mpfid, VP *p_blk);
EXTERN  ER      pget_mpf(ID mpfid, VP *p_blk);
#define pget_mpf(p1,p2) (tget_mpf((p1),(p2),(TMO_POL)))
#define get_mpf(p1,p2)  (tget_mpf((p1),(p2),(TMO_FEVR)))
EXTERN  ER      tget_mpf(ID mpfid, VP *p_blk, TMO tmout);
EXTERN  ER      rel_mpf(ID mpfid, VP p_blk);
EXTERN  ER      ref_mpf(ID mpfid, T_RMPF *pk_rmpf);

/* 時間管理機能（システム時刻管理） */

EXTERN  ER      set_tim(SYSTIM *p_systim);
EXTERN  ER      get_tim(SYSTIM *p_systim);
EXTERN  ER      isig_tim(void);
EXTERN  UW      vget_tms(void);

/* 時間管理機能（周期ハンドラ） */

EXTERN  ER      sta_cyc(ID cycid);
EXTERN  ER      stp_cyc(ID cycid);
EXTERN  ER      ref_cyc(ID cycid, T_RCYC *pk_rcyc);

/* システム状態管理機能 */

EXTERN  ER      irot_rdq(PRI tskpri);
#define irot_rdq(p1)    (rot_rdq(p1))
EXTERN  ER      rot_rdq(PRI tskpri);
EXTERN  ER      iget_tid(ID *p_tskid);
#define iget_tid(p1)    (get_tid(p1))
EXTERN  ER      get_tid(ID *p_tskid);
EXTERN  ER      iloc_cpu(void);
#define iloc_cpu(p1)    (loc_cpu(p1))
EXTERN  ER      loc_cpu(void);
EXTERN  ER      iunl_cpu(void);
#define iunl_cpu(p1)    (unl_cpu(p1))
EXTERN  ER      unl_cpu(void);
EXTERN  ER      dis_dsp(void);
EXTERN  ER      ena_dsp(void);
EXTERN  BOOL    sns_ctx(void);
EXTERN  BOOL    sns_loc(void);
EXTERN  BOOL    sns_dsp(void);
EXTERN  BOOL    sns_dpn(void);
EXTERN  ER      ref_sys(T_RSYS *pk_rsys);
EXTERN  ER      vloc_cpu(void);
EXTERN  ER      vunl_cpu(void);

/* 割込み管理機能 */

EXTERN  ER      chg_ims(IMASK imask);
EXTERN  ER      get_ims(IMASK *p_imask);

/* システム構成管理機能 */

EXTERN  ER      ref_cfg(T_RCFG *pk_rcfg);
EXTERN  ER      ref_ver(T_RVER *pk_rver);

#endif
