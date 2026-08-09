//*****************************************************************************
//
// ETHIsr.c - Driver for Ethernet controller.
//
//*****************************************************************************

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "inc/hw_ethernet.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/ethernet.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/flash.h"

#include "netif/ETHIsr.h"

void ETH0IntHandler( void );

//*****************************************************************************
//
// This structure represents status of SSI device and port(ETH0,ETH1,..).
// Every state is defined in one bit. Access to these bits is via special function of
// Cortex M3, bit band mapping.
//
//*****************************************************************************
static volatile unsigned long ETHDevice[MAX_ETH_PORTS];

//*****************************************************************************
//
//! Ethernet peripheral identification.
//
//*****************************************************************************
static const unsigned long ETHPeripheral[MAX_ETH_PORTS] = { SYSCTL_PERIPH_ETH, };

//*****************************************************************************
//
//! Ethernet peripheral identification.
//
//*****************************************************************************
static void (*ETHISR[MAX_ETH_PORTS])( void ) = { ETH0IntHandler, };

//*****************************************************************************
//
//! Ethernet peripheral pin gate.
//
//*****************************************************************************
static const unsigned long ETHPeripheralGate[MAX_ETH_PORTS] = { SYSCTL_PERIPH_GPIOF, };

//*****************************************************************************
//
//! The base address for the Ethernet associated with a port.
//
//*****************************************************************************
static const unsigned long ETHBase[MAX_ETH_PORTS] = { ETH_BASE, };

//*****************************************************************************
//
//! The port address for the ETH associated pins.
//
//*****************************************************************************
#if defined( PART_LM3S6965 )
static const unsigned long ETHPortBase[MAX_ETH_PORTS] = { GPIO_PORTF_BASE, };
#elif defined ( PART_LM3S8962 )
static const unsigned long ETHPortBase[MAX_ETH_PORTS] = { GPIO_PORTF_BASE, };
#else
#error "part not supported"
#endif

//*****************************************************************************
//
//! The pins associated with ETH peripheral.
//
//*****************************************************************************
#if defined( PART_LM3S6965 )
static const unsigned long ETHPins[MAX_ETH_PORTS] = { GPIO_PIN_2 | GPIO_PIN_3, };
#elif defined ( PART_LM3S8962 )
static const unsigned long ETHPins[MAX_ETH_PORTS] = { GPIO_PIN_2 | GPIO_PIN_3, };
#else
#error "part not supported"
#endif
//*****************************************************************************
//
//! The interrupt for the ETH associated with a port.
//
//*****************************************************************************
static const unsigned long ETHInterrupt[MAX_ETH_PORTS] = { INT_ETH, };

//*****************************************************************************
//
// Informs service task about RX event from interrupt routine
//
//*****************************************************************************
xSemaphoreHandle ETHRxBinSemaphore[MAX_ETH_PORTS] = { NULL };

//*****************************************************************************
//
// Informs service task about TX event from interrupt routine
//
//*****************************************************************************
xSemaphoreHandle ETHTxBinSemaphore[MAX_ETH_PORTS] = { NULL };

//*****************************************************************************
//
// Prevents Tx simultaneously accessing devices from different tasks
//
//*****************************************************************************
xSemaphoreHandle ETHTxAccessMutex[MAX_ETH_PORTS] = { NULL };

//*****************************************************************************
//
// Prevents Rx simultaneously accessing devices from different tasks
//
//*****************************************************************************
xSemaphoreHandle ETHRxAccessMutex[MAX_ETH_PORTS] = { NULL };

//*****************************************************************************
//
//! Handles the ETH interrupt. 
//! Put it into the interrupt vector of Stellaris
//!
//! This function is called when either of the ETH generate an interrupt.
//! An interrupt will be generated when data is received, transmitted, rx overflow
//! becomes or on link status change.
//!
//! \return None.
//
//*****************************************************************************
void
ETH0IntHandler( void )
{
    static portBASE_TYPE xHigherPriorityTaskWoken;

    unsigned long   ulStatus;

    // Read and Clear the interrupt.
    ulStatus = EthernetIntStatus( ETHBase[0], false );
    EthernetIntClear( ETHBase[0], ulStatus );

    // See if RX event occured.
    if( ulStatus & ETH_INT_RX )
    {
        // Disable Ethernet RX Interrupt.
        EthernetIntDisable( ETH_BASE, ETH_INT_RX );

        HWREGBITW( &ETHDevice[0], ETH_ERROR ) = 0;
        xSemaphoreGiveFromISR( ETHRxBinSemaphore[0], &xHigherPriorityTaskWoken );
    }

    // See if TXERR event occured.
    if( ulStatus & ETH_INT_TXER )
    {
        HWREGBITW( &ETHDevice[0], ETH_ERROR ) = 1;
        HWREGBITW( &ETHDevice[0], ETH_TXERROR ) = 1;
        xHigherPriorityTaskWoken = 0;
    }

    // See if TX event occured.
    if( ulStatus & ETH_INT_TX )
    {
        HWREGBITW( &ETHDevice[0], ETH_ERROR ) = 0;
        xSemaphoreGiveFromISR( ETHTxBinSemaphore[0], &xHigherPriorityTaskWoken );
    }

    // See if RX overflow event occured.
    if( ulStatus & ETH_INT_RXOF )
    {
        // Set error and flag
        HWREGBITW( &ETHDevice[0], ETH_ERROR ) = 1;
        HWREGBITW( &ETHDevice[0], ETH_OVERFLOW ) = 1;

        xSemaphoreGiveFromISR( ETHRxBinSemaphore[0], &xHigherPriorityTaskWoken );
    }

    // See if PHY event occured.
    if( ulStatus & ETH_INT_PHY )
    {
        // Something important was happened with network
        // no need to worry about while loop in EthernetPHYRead
        // Ethernet PHY Management Register 17 - Interrupt Control/Status
        // Read and Clear the interrupt.
        unsigned long   phyStatus = EthernetPHYRead( ETHBase[0], PHY_MR17 );

        switch ( phyStatus & ETH_PHY_INT_MASKED )
        {
        case ETH_LINK_DOWN:
            HWREGBITW( &ETHDevice[0], ETH_ERROR ) = 0;
            HWREGBITW( &ETHDevice[0], ETH_LINK_OK ) = 0;
            break;
        case ETH_LINK_UP:
            HWREGBITW( &ETHDevice[0], ETH_ERROR ) = 0;
            HWREGBITW( &ETHDevice[0], ETH_LINK_OK ) = 1;
            break;
        }
        // no need immediately to switch context
        xHigherPriorityTaskWoken = 0;
    }
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

//*****************************************************************************
//
//! Initializes the ethernet controller and driver.
//! Call this function on the your main function.
//!
//! This function initializes and configures the ethernet controller.
//!
//! \return 0 or -1 if error.
//
//*****************************************************************************
int
ETHServiceTaskInit( const unsigned long ulPort )
{
    unsigned char   hwaddr[ETH_HWADDR_LEN];
    unsigned long   ulUser0, ulUser1;

    if( ulPort < MAX_ETH_PORTS )
    {
        // Check if peripheral is present
        if( false == SysCtlPeripheralPresent( ETHPeripheral[ulPort] ) )
            return -1;

        // Initialize semaphores and mutexes.
        ETHRxBinSemaphore[ulPort] = xSemaphoreCreateCounting( 1, 0 );
        ETHTxBinSemaphore[ulPort] = xSemaphoreCreateCounting( 1, 0 );
        ETHTxAccessMutex[ulPort] = xSemaphoreCreateMutex(  );
        ETHRxAccessMutex[ulPort] = xSemaphoreCreateMutex(  );

        // Enable peripheral, other fault is generated
        SysCtlPeripheralEnable( ETHPeripheral[ulPort] );
        SysCtlPeripheralReset( ETHPeripheral[ulPort] );
        SysCtlPeripheralEnable( ETHPeripheralGate[ulPort] );

        // Enable Port for Ethernet LEDs.
        //  LED0        Bit 3   Output
        //  LED1        Bit 2   Output
        GPIODirModeSet( ETHPortBase[ulPort], ETHPins[ulPort], GPIO_DIR_MODE_HW );
        GPIOPadConfigSet( ETHPortBase[ulPort], ETHPins[ulPort], GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD );

        // Configure the hardware MAC address for Ethernet Controller filtering of
        // incoming packets.
        //
        // For the LM3S6965 Evaluation Kit, the MAC address will be stored in the
        // non-volatile USER0 and USER1 registers.  These registers can be read
        // using the FlashUserGet function, as illustrated below.
        //
        FlashUserGet( &ulUser0, &ulUser1 );
        if( ( ulUser0 == 0xffffffff ) || ( ulUser1 == 0xffffffff ) )
        {
            // TODO: do something...
        }

        hwaddr[0] = ( ( ulUser0 >> 0 ) & 0xff );
        hwaddr[1] = ( ( ulUser0 >> 8 ) & 0xff );
        hwaddr[2] = ( ( ulUser0 >> 16 ) & 0xff );
        hwaddr[3] = ( ( ulUser1 >> 0 ) & 0xff );
        hwaddr[4] = ( ( ulUser1 >> 8 ) & 0xff );
        hwaddr[5] = ( ( ulUser1 >> 16 ) & 0xff );

        // set MAC hardware address
        EthernetMACAddrSet( ETHBase[ulPort], &( hwaddr[0] ) );

        // Ethernet controller is a little complicated, all is done in user defined
        // task(thread), thus no open is needed.
        return ( 0 );
    }
    return -1;
}

//!*****************************************************************************
//!
//! Flush Ethernet FIFO's.
//! Call it from your main function.
//!
//!
//! \param ulPort is the Ethernet port number to be accessed.
//! \param flCmd specifies which buffer, RX, TX or both (RX | TX).
//!
//! This function deletes bytes from ring buffer.
//!
//! \return 0 or -1 if error.
//
//*****************************************************************************
int
ETHServiceTaskFlush( const unsigned long ulPort, const unsigned long flCmd )
{
    if( ( ulPort < MAX_ETH_PORTS ) && ( HWREGBITW( &ETHDevice[ulPort], ETH_ENABLED ) ) )
    {
        // Checks, if flCmd contains valid command
        if( !( flCmd & ( ETH_FLUSH_RX | ETH_FLUSH_TX ) ) )
        {
            HWREGBITW( &ETHDevice[ulPort], ETH_ERROR ) = 1;
            HWREGBITW( &ETHDevice[ulPort], ETH_EBADOPT ) = 1;
            return ( -1 );
        }

        if( flCmd & ETH_FLUSH_RX )
        {
            // Access to shared variable
            xSemaphoreTake( ETHRxAccessMutex[ulPort], ( portTickType ) portMAX_DELAY );

            // Disable ethernet receiver, MACRCTL register.
            // BitBand region:  RXEN - Enable Receiver, bit 0
            HWREGBITW( ETHBase[ulPort] + MAC_O_RCTL, 0 ) = 0;

            // Reset receive FIFO, It is recommended that the receiver be disabled (RXEN = 0), before a
            // reset is initiated (RSTFIFO = 1). This sequence flushes and resets the RX FIFO.
            //MACRCTL register, RSTFIFO clear Receive FIFO, bit 4
            HWREGBITW( ETHBase[ulPort] + MAC_O_RCTL, 4 ) = 1;

            // This bit is automatically cleared when read.
            HWREGBITW( ETHBase[ulPort] + MAC_O_RCTL, 4 );

            // Enable ethernet receiver, MACRCTL register.
            // BitBand region:  RXEN - Enable Receiver, bit 0
            HWREGBITW( ETHBase[ulPort] + MAC_O_RCTL, 0 ) = 1;

            // Release mutex
            xSemaphoreGive( ETHRxAccessMutex[ulPort] );
        }

        if( flCmd & ETH_FLUSH_TX )
        {
            // Access to shared variable
            xSemaphoreTake( ETHTxAccessMutex[ulPort], ( portTickType ) portMAX_DELAY );

            // See if Ethernet is currently transmitting  a frame,
            while( MAC_TR_NEWTX == HWREG( ETH_BASE + MAC_O_TR ) )
            {
                /*
                 * vTaskDelay() does not provide a good method of controlling the frequency
                 * of a cyclical task as the path taken through the code, as well as other task and
                 * interrupt activity, will effect the frequency at which vTaskDelay() gets called
                 * and therefore the time at which the task next executes.
                 */
                // The shortest time to wait with respect to other tasks. If you need
                // immediate reaction, remove this function.
                vTaskDelay( 1 );
            }
            // Reset write FIFO pointer, MACRIS/MACIACK register.
            // BitBand region:  TXER - Setting this bit clears the TXER interrupt in the MACRIS register and
            // resets the TX FIFO write pointer, bit 1
            HWREGBITW( ETHBase[ulPort] + MAC_O_IACK, 1 ) = 1;

            // Release mutex
            xSemaphoreGive( ETHTxAccessMutex[ulPort] );
        }



        //return ok
        return 0;
    }
    HWREGBITW( &ETHDevice[ulPort], ETH_ERROR ) = 1;
    HWREGBITW( &ETHDevice[ulPort], ETH_EBADF ) = 1;
    return ( -1 );

}

//!*****************************************************************************
//!
//! Enables transmitting and receiving.
//! This function is called from low_level_init in ETHIsr.c
//! 
//! \param ulPort is the ETH port number to be accessed.
//!
//! \return 0 or -1 if error.
//
//*****************************************************************************
int
ETHServiceTaskEnable( unsigned long ulPort )
{
    unsigned long   temp;
    unsigned long   phyStatus;

    if( ulPort < MAX_ETH_PORTS )
    {
        // Do whatever else is needed to initialize interface.
        // Disable and clear all Ethernet Interrupts.
        EthernetIntDisable( ETHBase[ulPort], ( ETH_INT_PHY | ETH_INT_MDIO | ETH_INT_RXER
                                               | ETH_INT_RXOF | ETH_INT_TX | ETH_INT_TXER | ETH_INT_RX ) );

        temp = EthernetIntStatus( ETHBase[ulPort], false );
        EthernetIntClear( ETHBase[ulPort], temp );

        // Initialize the Ethernet Controller.
        EthernetInitExpClk( ETHBase[ulPort], SysCtlClockGet(  ) );

        /*
         * Configure the Ethernet Controller for normal operation.
         * - Enable TX Duplex Mode
         * - Enable TX Padding
         * - Enable TX CRC Generation
         * - Enable RX Multicast Reception
         */
        EthernetConfigSet( ETHBase[ulPort], ( ETH_CFG_TX_DPLXEN | ETH_CFG_TX_CRCEN
                                              | ETH_CFG_TX_PADEN | ETH_CFG_RX_AMULEN ) );

        // Enable the Ethernet Controller transmitter and receiver.
        EthernetEnable( ETHBase[ulPort] );

        // Determine if link is up
        // no need to worry about while loop in EthernetPHYRead
        // Ethernet PHY Management Register 1 - Status
        phyStatus = EthernetPHYRead( ETH_BASE, PHY_MR1 );
        if( phyStatus & ETH_PHY_LINK_UP )
        {
            // set link up flag
            HWREGBITW( &ETHDevice[ulPort], ETH_LINK_OK ) = 1;
        }
        else
        {
            HWREGBITW( &ETHDevice[ulPort], ETH_LINK_OK ) = 0;
        }

        // Configure the Ethernet PHY interrupt management register.
        EthernetPHYWrite( ETHBase[ulPort], PHY_MR17, ETH_PHY_INT_MASK );

        // Set the interrupt handler
        IntRegister(  ETHInterrupt[ulPort], ETHISR[ulPort]);

        // Enable the Ethernet Interrupt handler.
        IntEnable( ETHInterrupt[ulPort] );

        // Set interrupt priority to number higher than configMAX_SYSCALL_INTERRUPT_PRIORITY
        // defined in FreeRTOSConfig.h, see www.freertos.org
        IntPrioritySet( ETHInterrupt[ulPort], SET_SYSCALL_INTERRUPT_PRIORITY( 6 ) );

        // Enable Ethernet RX, PHY and RXOF Packet Interrupts.
        EthernetIntEnable( ETHBase[ulPort], ETH_INT_RX | ETH_INT_PHY | ETH_INT_RXOF | ETH_INT_TXER );

        HWREGBITW( &ETHDevice[ulPort], ETH_ENABLED ) = 1;

        return 0;
    }
    return ( -1 );
}

//!*****************************************************************************
//!
//! 
//! This function is called from low_level_init function in LWIPStack.c
//!
//!
int
ETHServiceTaskWaitReady( const unsigned long ulPort )
{
    if( ( ulPort < MAX_ETH_PORTS ) && ( HWREGBITW( &ETHDevice[ulPort], ETH_ENABLED ) ) )
    {
        // See if Ethernet completed autonegation,
        while( !( PHY_MR1_ANEGC & EthernetPHYRead( ETHBase[0], PHY_MR1 ) ) )
        {
            /*
             * vTaskDelay() does not provide a good method of controlling the frequency
             * of a cyclical task as the path taken through the code, as well as other task and
             * interrupt activity, will effect the frequency at which vTaskDelay() gets called
             * and therefore the time at which the task next executes.
             */
            // The shortest time to wait with respect to other tasks. If you need
            // immediate reaction, remove this function.
            vTaskDelay( 1 );
        }
        return ( 0 );
    }
    HWREGBITW( &ETHDevice[ulPort], ETH_ERROR ) = 1;
    HWREGBITW( &ETHDevice[ulPort], ETH_EBADF ) = 1;
    return ( -1 );
}

//*****************************************************************************
//
//! Disables transmitting and receiving.
//! This function is called from low_level_init function in LWIPStack.c
//!
//! \param ulPort is the Ethernet port number to be accessed.
//!
//! Disables Ethernet peripheral
//!
//!
//! \return 0 or -1 if error.
//
//*****************************************************************************
int
ETHServiceTaskDisable( const unsigned long ulPort )
{
    if( ( ulPort < MAX_ETH_PORTS ) && ( HWREGBITW( &ETHDevice[ulPort], ETH_ENABLED ) ) )
    {
        // Disable the ETH transmit and receive interrupts.
        IntDisable( ETHInterrupt[ulPort] );

        // Disable the ETH transmit and receive interrupts.
        EthernetIntDisable( ETHBase[ulPort], ( ETH_INT_RX | ETH_INT_PHY | ETH_INT_RXOF ) );

        // Unregister interrupt handler
        IntUnregister( ETHInterrupt[ulPort] );

        // Disable the ETH.
        EthernetDisable( ETHBase[ulPort] );

        // Clear all flags
        ETHDevice[ulPort] = 0;

        return ( 0 );
    }
    return ( -1 );
}

//*****************************************************************************
//
//! Return error status.
//! This function is called from low_level_input function in LWIPStack.c
//!
//! \param ulPort is the Ethernet port number to be accessed.
//!
//! This function returns last error and clears error flags
//!
//! \return int or -1 if error.
//
//*****************************************************************************
int
ETHServiceTaskLastError( const unsigned long ulPort )
{
    if( ( ulPort < MAX_ETH_PORTS ) && ( HWREGBITW( &ETHDevice[ulPort], ETH_ENABLED ) ) )
    {
        unsigned long   err = ETHDevice[ulPort];

        // Clear all flags except linkup flag and enabled device
        ETHDevice[ulPort] &= ( 0x01 << ETH_LINK_OK ) | ( 0x01 << ETH_ENABLED );

        return ( int )err;
    }
    HWREGBITW( &ETHDevice[ulPort], ETH_ERROR ) = 1;
    HWREGBITW( &ETHDevice[ulPort], ETH_EBADF ) = 1;
    return ( -1 );
}

//*****************************************************************************
//
//! Return link status.
//! This function is called from low_level_transmit function in LWIPStack.c
//!
//! \param ulPort is the Ethernet port number to be accessed.
//!
//! This function returns status of link. If is up or down.
//!
//! \return int or -1 if error.
//
//*****************************************************************************
int
ETHServiceTaskLinkStatus( const unsigned long ulPort )
{
    if( ( ulPort < MAX_ETH_PORTS ) && ( HWREGBITW( &ETHDevice[ulPort], ETH_ENABLED ) ) )
    {
        return ( int )HWREGBITW( &ETHDevice[ulPort], ETH_LINK_OK );
    }
    HWREGBITW( &ETHDevice[ulPort], ETH_ERROR ) = 1;
    HWREGBITW( &ETHDevice[ulPort], ETH_EBADF ) = 1;
    return ( -1 );
}

//*****************************************************************************
//
//! Return MAC adress.
//! This function is called from low_level_init function in LWIPStack.c
//!
//! \param ulPort is the Ethernet port number to be accessed.
//!
//! This function returns status of link. If is up or down.
//!
//! \return int or -1 if error.
//
//*****************************************************************************
int
ETHServiceTaskMACAddress( const unsigned long ulPort, unsigned char *pucMACAddr )
{
    if( ( ulPort < MAX_ETH_PORTS ) )
    {
        EthernetMACAddrGet( ETHBase[ulPort], pucMACAddr );
        return ( int )( 0 );
    }
    HWREGBITW( &ETHDevice[ulPort], ETH_ERROR ) = 1;
    HWREGBITW( &ETHDevice[ulPort], ETH_EBADF ) = 1;
    return ( -1 );
}

//!*****************************************************************************
//!
//! Check for packet available from the Ethernet controller.
//! This function is called from ethernetif_input function in LWIPStack.c
//!
//!
//! \param ulBase is the base address of the controller.
//!
//! The Ethernet controller provides a register that contains the number of
//! packets available in the receive FIFO.  When the last bytes of a packet are
//! successfully received (that is, the frame check sequence bytes), the packet
//! count is incremented.  Once the packet has been fully read (including the
//! frame check sequence bytes) from the FIFO, the packet count will be
//! decremented.
//!
//! \return 0,1 or -1 if error.
//
//*****************************************************************************
int
ETHServiceTaskPacketAvail( const unsigned long ulPort )
{
    if( ulPort < MAX_ETH_PORTS )
    {
        // Return the availability of packets.
        return ( ( HWREG( ETHBase[ulPort] + MAC_O_NP ) & MAC_NP_NPR_M ) ? 1 : 0 );
    }
    HWREGBITW( &ETHDevice[ulPort], ETH_ERROR ) = 1;
    HWREGBITW( &ETHDevice[ulPort], ETH_EBADF ) = 1;
    return ( -1 );
}

//!*****************************************************************************
//!
//! 
//! This function is called from ethernetif_input function in LWIPStack.c
//!
//!
int
ETHServiceTaskEnableReceive( const unsigned long ulPort )
{
    if( ulPort < MAX_ETH_PORTS )
    {
        EthernetIntEnable( ETHBase[ulPort], ETH_INT_RX );
        return 0;
    }
    HWREGBITW( &ETHDevice[ulPort], ETH_ERROR ) = 1;
    HWREGBITW( &ETHDevice[ulPort], ETH_EBADF ) = 1;
    return ( -1 );
}
