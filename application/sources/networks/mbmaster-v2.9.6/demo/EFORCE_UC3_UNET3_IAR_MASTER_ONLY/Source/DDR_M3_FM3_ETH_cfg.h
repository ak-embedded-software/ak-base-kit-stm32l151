/***********************************************************************
    Ethernet driver configuration for CH0
 ***********************************************************************/

#ifndef _DDR_M3_FM3_ETH_CFG_H_
#define _DDR_M3_FM3_ETH_CFG_H_

#include "COMMONDEF.h"
#include "MB9BFxxx.h"

#define FM3_SRS    610


#ifdef __cplusplus
extern "C" {
#endif

/*
    Configure PHY Address
*/
#define ETH_PHY_ADDR    0x0

/*
    Configure External PHY CLK
--------------------------------
               0 = Internal
    (Default)  1 = External
*/
#define ETH_EXT_CLK    1

/*
    Configure PHY Clock Source
--------------------------------
    (Default)  0 = CLKMO
               1 = Internal PLL source CLKMO
               2 = CLKPLL
*/
#define ETH_CLK_SRC    0

/*
    Configure UBSR register
*/
#define ETH_CLK_UBSR    6

/*
    Configure UPLLK register
*/
#define ETH_CLK_UPLLK    0

/*
    Configure UPLLN register
*/
#define ETH_CLK_UPLLN    49

/*
    Configure UPLLM register
*/
#define ETH_CLK_UPLLM    7

/*
    Configure MDC Clock
*/
#define ETH_MDC_CLK     MACMIIAR_CR_62

/*
    Configure Interrupt priority level
*/
#define IPL_ETHERNET    240

/*
    Configure MII Mode
--------------------------------
    (Default)   0 = Enable MII
                1 = Enable RMII
*/
#define ETH_RMII_MODE   1

/*
    Configure PHY Mode
--------------------------------
    (Default)   0 = Auto select mode
                1 = 10M Half Duplex manual mode
                2 = 10M Full/Half (Duplex auto select mode)
                3 = 100M Half Duplex manual mode
                4 = 100M Full/Half (Duplex auto select mode)
*/
#define ETH_PHY_MODE    0

/*
    Configure Address Filter Mode
--------------------------------
    (Default)   0 = Filter disable (Perfect filtering)
                1 = promiscuous mode (receive all packets)
                2 = multicast filter mode
*/
#define ETH_FILTER_MODE    2

/*
    Configure Rx DMA Descriptor count
--------------------------------
*/
#define ETH_TXDESC_CNT    2
#define ETH_RXDESC_CNT    4

/*
    Configure Hardware checksum offloading
--------------------------------
                0 = Disable Tx & Rx Hardware checksum
                1 = Enable Tx Hardware checksum
                2 = Enable Rx Hardware checksum
    (Default)   3 = Enable Tx & Rx Hardware checksum
*/
#define ETH_CSUM_MODE    3

#ifdef __cplusplus
}
#endif
#endif
