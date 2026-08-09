//*****************************************************************************
//
// ETHIsr.h - Prototypes for the Ethernet controller.
//
//*****************************************************************************
#ifndef ETHISR_H_
#define ETHISR_H_


#include "inc/hw_ethernet.h"

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern          "C"
{
#endif

//*****************************************************************************
//
//! The number of ethernet ports supported by this module.
//
//*****************************************************************************
#define MAX_ETH_PORTS			(1)

//*****************************************************************************
//
//! The number of bytes of ethernet address.
//
//***************************************************************************** 
#define	ETH_HWADDR_LEN (6)

//*****************************************************************************
//
//! The maximal transmit unit.
//
//***************************************************************************** 
#define	ETH_MTU (1500)

//*****************************************************************************
//
//! The maximal transmit speed Mbit/s.
//
//*****************************************************************************         
#define	ETH_DEFAULT_LINK_SPEED 100

//*****************************************************************************
//
//! Mask for PHY interrupt(MR17) register to detect link up/down.
//
//***************************************************************************** 
#define ETH_PHY_INT_MASK ((PHY_MR17_LSCHG_IE | PHY_MR17_ANEGCOMP_IE))
#define ETH_PHY_INT_MASKED ((ETH_PHY_INT_MASK) >> 8)
#define ETH_PHY_LINK_UP PHY_MR1_LINK
#define ETH_LINK_DOWN  (PHY_MR17_LSCHG_INT)
#define ETH_LINK_UP  (PHY_MR17_LSCHG_INT | PHY_MR17_ANEGCOMP_INT)

//*****************************************************************************
//
//! Eth bit position states for device.
//
//***************************************************************************** 
#define ETH_ERROR			0x00
#define ETH_OVERFLOW		0x01
#define	ETH_LINK_OK			0x02
#define ETH_EBADF			0x03
#define ETH_ENABLED			0x04
#define ETH_EBADOPT			0x05
#define ETH_TXERROR			0x06

//*****************************************************************************
//
//! Ethernet FIFO's identifier for flushing.
//
//***************************************************************************** 
#define ETH_FLUSH_RX			0x01
#define ETH_FLUSH_TX			0x02


//*****************************************************************************
//
//! Prototypes for the APIs.
//
//*****************************************************************************
extern xSemaphoreHandle ETHRxBinSemaphore[MAX_ETH_PORTS];
extern xSemaphoreHandle ETHTxBinSemaphore[MAX_ETH_PORTS];
extern xSemaphoreHandle ETHTxAccessMutex[MAX_ETH_PORTS];
extern xSemaphoreHandle ETHRxAccessMutex[MAX_ETH_PORTS];

extern int      ETHServiceTaskInit( const unsigned long ulPort );
extern int      ETHServiceTaskEnable( const unsigned long ulPort );
extern int      ETHServiceTaskDisable( const unsigned long ulPort );
extern int      ETHServiceTaskLastError( const unsigned long ulPort );
extern int      ETHServiceTaskLinkStatus( const unsigned long ulPort );
extern int      ETHServiceTaskFlush( const unsigned long ulPort, const unsigned long flCmd );
extern int      ETHServiceTaskMACAddress( const unsigned long ulPort, unsigned char *pucMACAddr );
extern int      ETHServiceTaskEnableReceive( const unsigned long ulPort );
extern int      ETHServiceTaskPacketAvail( const unsigned long ulPort );
extern int      ETHServiceTaskWaitReady( const unsigned long ulPort );

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif                          /*ETHISR_H_ */
