/***********************************************************************
	MICRO C CUBE / COMPACT, DEVICE DRIVER
	Standard Communication Interface
    Copyright (c)  2008, eForce Co., Ltd. All rights reserved.
    
    Version Information  2008.3.31: Created
 ***********************************************************************/

#include "kernel.h"
#include "DDR_COM.h"


extern ER ctr_dev(ID DevID, ID FuncID, VP_INT ControlData);


ER ini_com(ID DevID, T_COM_SMOD const *pk_SerialMode)
{
    return ctr_dev(DevID, TA_COM_INI, (VP_INT)pk_SerialMode);
}

ER ref_com(ID DevID, T_COM_REF *pk_SerialRef)
{
    return ctr_dev(DevID, TA_COM_REF, (VP_INT)pk_SerialRef);
}

ER ctr_com(ID DevID, UH command, TMO tmout)
{
    T_COM_CTR SerialData;

    SerialData.command = command;
    SerialData.time = tmout;
    return ctr_dev(DevID, TA_COM_CTR, (VP_INT)&SerialData);
}

ER putc_com(ID DevID, VB chr, TMO tmout)
{
    T_COM_SND TransmiteData;

    TransmiteData.tbuf = &chr;
    TransmiteData.tcnt = 1;
    TransmiteData.time = tmout;
    return ctr_dev(DevID, TA_COM_PUTS, (VP_INT)&TransmiteData);
}

ER puts_com(ID DevID, VB const *schr, UINT *scnt, TMO tmout)
{
    T_COM_SND TransmiteData;
    ER ercd;

    TransmiteData.tbuf = schr;
    TransmiteData.tcnt = *scnt;
    TransmiteData.time = tmout;
    ercd = ctr_dev(DevID, TA_COM_PUTS, (VP_INT)&TransmiteData);
    *scnt -= TransmiteData.tcnt;
    return ercd;
}

ER getc_com(ID DevID, VB *rbuf, UB *sbuf, TMO tmout)
{
    T_COM_RCV ReceiveData;

    ReceiveData.rbuf = rbuf;
    ReceiveData.sbuf = sbuf;
    ReceiveData.rcnt = 1;
    ReceiveData.eos  = 0;
    ReceiveData.time = tmout;
    return ctr_dev(DevID, TA_COM_GETS, (VP_INT)&ReceiveData);
}

ER gets_com(ID DevID, VB *rbuf, UB *sbuf, INT eos, UINT *rcnt, TMO tmout)
{
    T_COM_RCV ReceiveData;
    T_COM_EOS EndOfStr;
    ER ercd;

    if (eos >= 0) {
        EndOfStr.chr[0] = (VB)eos;
        EndOfStr.flg[0] = 1;
        EndOfStr.flg[1] = 0;
        EndOfStr.flg[2] = 0;
        EndOfStr.flg[3] = 0;
    }

    ReceiveData.rbuf = rbuf;
    ReceiveData.sbuf = sbuf;
    ReceiveData.rcnt = *rcnt;
    ReceiveData.eos  = (eos < 0) ? 0 : &EndOfStr;
    ReceiveData.time = tmout;
    ercd = ctr_dev(DevID, TA_COM_GETS, (VP_INT)&ReceiveData);
    *rcnt -= ReceiveData.rcnt;
    return ercd;
}
