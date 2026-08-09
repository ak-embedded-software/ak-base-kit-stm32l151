/***********************************************************************
    Ethernet driver configuration for CH1
 ***********************************************************************/

#ifndef _DDR_M3_FM3_ETH_CFG1_H_
#define _DDR_M3_FM3_ETH_CFG1_H_

#include "COMMONDEF.h"
#include "MB9BFxxx.h"

#define FM3_SRS    610


#ifdef __cplusplus
extern "C" {
#endif

/*
    Configure PHY Address
*/
#define ETH_PHY_ADDR1    0x1

/*
    Configure MDC Clock
*/
#define ETH_MDC_CLK1     MACMIIAR_CR_62

/*
    Configure Interrupt priority level
*/
#define IPL_ETHERNET1    240

/*
    Configure MII Mode
--------------------------------
                1 = Enable RMII
*/
#define ETH_RMII_MODE1   1

/*
    Configure PHY Mode
--------------------------------
    (Default)   0 = Auto select mode
                1 = 10M Half Duplex manual mode
                2 = 10M Full/Half (Duplex auto select mode)
                3 = 100M Half Duplex manual mode
                4 = 100M Full/Half (Duplex auto select mode)
*/
#define ETH_PHY_MODE1    0

/*
    Configure Address Filter Mode
--------------------------------
    (Default)   0 = Filter disable (Perfect filtering)
                1 = promiscuous mode (receive all packets)
                2 = multicast filter mode
*/
#define ETH_FILTER_MODE1    0

/*
    Configure Rx DMA Descriptor count
--------------------------------
*/
#define ETH_TXDESC_CNT1    2
#define ETH_RXDESC_CNT1    4

/*
    Configure Hardware checksum offloading
--------------------------------
                0 = Disable Tx & Rx Hardware checksum
                1 = Enable Tx Hardware checksum
                2 = Enable Rx Hardware checksum
    (Default)   3 = Enable Tx & Rx Hardware checksum
*/
#define ETH_CSUM_MODE1    3

#ifdef __cplusplus
}
#endif
#endif
