/***********************************************************************
    Serial driver configuration
 ***********************************************************************/

#ifndef _DDR_MB9BF_USART_CFG_H_
#define _DDR_MB9BF_USART_CFG_H_

#include "MB9BFxxx.h"
#include "kernel_id.h"

#define FM3_SRS    610

/* UART settings */

#define UART_0
#define TXBUF_SZ0   1472
#define RXBUF_SZ0   1472
#define XOFF_SZ0    192
#define XON_SZ0     64
#define TXSEM0      ID_UART0_TSEM
#define RXSEM0      ID_UART0_RSEM
#define IPL_UART_0  240
#define PCLK0       72000000
#define RELOCATE0   0

#endif /* _DDR_MB9BF_USART_CFG_H_ */

/* end */
