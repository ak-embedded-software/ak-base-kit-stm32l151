/***********************************************************************
    Device Driver Manager
 ***********************************************************************/

#include "kernel.h"
#include "device_id.h"
#include "DDR_COM.h"
#include "DDR_MB9BF_UART.h"

ER ctr_dev(ID DevID, ID FuncID, VP_INT ControlData);

extern ER _ddr_mb9_uart_drv(ID, VP_INT, T_MB9_UART_MNG const *);
extern T_MB9_UART_MNG const _ddr_mb9_uart_mng0;

ER ctr_dev(ID DevID, ID FuncID, VP_INT pk_ControlData)
{
    ER ercd;

    switch(DevID) {
        case DID_UART0: ercd = _ddr_mb9_uart_drv(FuncID, pk_ControlData, &_ddr_mb9_uart_mng0);
            break;
        default: ercd = E_NOSPT;
    }
    return ercd;
}

/* end */
