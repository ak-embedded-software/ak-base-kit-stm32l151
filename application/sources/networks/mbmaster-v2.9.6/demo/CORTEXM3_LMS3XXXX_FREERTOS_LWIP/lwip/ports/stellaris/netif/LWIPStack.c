//*****************************************************************************
//
// Include standard library declarations.
//
//*****************************************************************************
#include <string.h>

//*****************************************************************************
//
// Include FreeRTOS declarations.
//
//*****************************************************************************
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//*****************************************************************************
//
// lwIP API Header Files
//
//*****************************************************************************
#include "lwip/init.h"
#include "lwip/api.h"
#include "lwip/netifapi.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "lwip/mem.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/dhcp.h"
#include "netif/etharp.h"
#include "lwip/autoip.h"
#include "lwip/dhcp.h"

//*****************************************************************************
//
// Luminary Micro DriverLib Header Files required for this interface driver.
//
//*****************************************************************************
#include "inc/hw_ethernet.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/ethernet.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"

//#include "Apps/httpd.h"

#include "netif/ETHIsr.h"
#include "netif/LWIPStack.h"

// Sanity Check:  This interface driver will NOT work if the following defines are incorrect.
#if (PBUF_LINK_HLEN != 16)
#warning "PBUF_LINK_HLEN must be 16 for this interface driver!"
#endif
#if (ETH_PAD_SIZE != 2)
#warning "ETH_PAD_SIZE must be 2 for this interface driver!"
#endif

// Forward declarations.
static void     ethernetif_input( void *pParams );
static struct pbuf *low_level_input( struct netif *netif );
static err_t    low_level_output( struct netif *netif, struct pbuf *p );
static err_t    low_level_transmit( struct netif *netif, struct pbuf *p );
void            httpd_init( void );

void            UART1printf( const char *pcString, ... );
//*****************************************************************************
//
// The lwIP network interface structure for the Stellaris Ethernet MAC.
//
//*****************************************************************************
static struct netif lwip_netif;

//*****************************************************************************
//
// In this function, the hardware should be initialized.
// Called from stellarisif_init().
//
// @param netif the already initialized lwip network interface structure
//        for this ethernetif
//!
//! \return None.
//!
//*****************************************************************************
static err_t
low_level_init( struct netif *netif )
{

    ETHServiceTaskDisable( 0 );

    // set MAC hardware address length
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    LWIP_DEBUGF( NETIF_DEBUG, ( "low_level_transmit: frame sent\n" ) );

    // set MAC hardware address
    ETHServiceTaskMACAddress( 0, &( netif->hwaddr[0] ) );

    LWIP_DEBUGF( NETIF_DEBUG,
                 ( "low_level_init: MAC address is %" X8_F "%" X8_F "%" X8_F "%" X8_F "%" X8_F "%" X8_F "\n",
                   netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2], netif->hwaddr[3], netif->hwaddr[4],
                   netif->hwaddr[5] ) );

    // maximum transfer unit
    netif->mtu = ETH_MTU;

    LWIP_DEBUGF( NETIF_DEBUG, ( "low_level_init: MTU set to %" U16_F "\n", netif->mtu ) );

    // device capabilities
    // don't set NETIF_FLAG_ETHARP if this device is not an ethernet one
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    // Create the task that handles the incoming packets.
    if( pdPASS ==
        xTaskCreate( ethernetif_input, ( signed portCHAR * )"ETH_INPUT", netifINTERFACE_TASK_STACK_SIZE,
                     ( void * )netif, netifINTERFACE_TASK_PRIORITY, NULL ) )
    {
        ETHServiceTaskEnable( 0 );
        LWIP_DEBUGF( NETIF_DEBUG, ( "low_level_input: Waiting for Ethernet to become ready\n" ) );
        ETHServiceTaskWaitReady( 0 );
        netif->flags |= NETIF_FLAG_LINK_UP;

        LWIP_DEBUGF( NETIF_DEBUG, ( "low_level_init: Ethernet link is up\n" ) );

        return ERR_OK;

    }
    else
    {
        LWIP_DEBUGF( NETIF_DEBUG, ( "low_level_init: create receive task error\n" ) );
        return ERR_IF;
    }
}

/**
 * Called from ethernetif_input
 *
 * This function will read a single packet from the Stellaris ethernet
 * interface, if available, and return a pointer to a pbuf.  The timestamp
 * of the packet will be placed into the pbuf structure.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return pointer to pbuf packet if available, NULL otherswise.
 */
static struct pbuf *
low_level_input( struct netif *netif )
{
    struct pbuf    *p, *q;
    u16_t           len;
    u32_t           temp;
    int             i;
    unsigned long  *ptr;
#if LWIP_PTPD
    u32_t           time_s, time_ns;

    /* Get the current timestamp if PTPD is enabled */
    lwIPHostGetTime( &time_s, &time_ns );
#endif

    /* Check if a packet is available, if not, return NULL packet. */
    if( ( HWREG( ETH_BASE + MAC_O_NP ) & MAC_NP_NPR_M ) == 0 )
    {
        int             err = ETHServiceTaskLastError( 0 );
        if( ( ETH_ERROR & err ) && ( ETH_OVERFLOW & err ) )
        {
            LWIP_DEBUGF( NETIF_DEBUG, ( "low_level_input: Ethernet overflow\n" ) );
            LINK_STATS_INC( link.drop );
        }
        return ( NULL );
    }

        /**
	 * Obtain the size of the packet and put it into the "len" variable.
	 * Note:  The length returned in the FIFO length position includes the
	 * two bytes for the length + the 4 bytes for the FCS.
	 *
	 */
    temp = HWREG( ETH_BASE + MAC_O_DATA );
    len = temp & 0xFFFF;

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc( PBUF_RAW, len, PBUF_POOL );

    /* If a pbuf was allocated, read the packet into the pbuf. */
    if( p != NULL )
    {
        /* Place the first word into the first pbuf location. */
        *( unsigned long * )p->payload = temp;
        p->payload = ( char * )( p->payload ) + 4;
        p->len -= 4;

        /* Process all but the last buffer in the pbuf chain. */
        q = p;
        while( q != NULL )
        {
            /* Setup a byte pointer into the payload section of the pbuf. */
            ptr = q->payload;

                        /**
			 * Read data from FIFO into the current pbuf
			 * (assume pbuf length is modulo 4)
			 *
			 */
            for( i = 0; i < q->len; i += 4 )
            {
                *ptr++ = HWREG( ETH_BASE + MAC_O_DATA );
            }

            /* Link in the next pbuf in the chain. */
            q = q->next;
        }

        /* Restore the first pbuf parameters to their original values. */
        p->payload = ( char * )( p->payload ) - 4;
        p->len += 4;

        /* Adjust the link statistics */
        LINK_STATS_INC( link.recv );

#if LWIP_PTPD
        // Place the timestamp in the PBUF
        p->time_s = time_s;
        p->time_ns = time_ns;
#endif
    }

    // If no pbuf available, just drain the RX fifo.
    else
    {
        for( i = 4; i < len; i += 4 )
        {
            temp = HWREG( ETH_BASE + MAC_O_DATA );
        }

        // Adjust the link statistics
        LINK_STATS_INC( link.memerr );
        LINK_STATS_INC( link.drop );
    }

    return ( p );
}

/**
 * The function low_level_init creates a thread with this function.
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_receive() that
 * should handle the actual reception of bytes from the network
 * interface.Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */

static void
ethernetif_input( void *pParams )
{
    struct netif   *netif;
    struct ethernetif *ethernetif;
    struct pbuf    *p;

    netif = ( struct netif * )pParams;
    ethernetif = netif->state;

    for( ;; )
    {
        do
        {
            // move received packet into a new pbuf
            p = low_level_input( netif );

            if( ( p == NULL ) && ( 0 == ETHServiceTaskPacketAvail( 0 ) ) )
            {
                // Actually enables only RX interrupt
                ETHServiceTaskEnableReceive( 0 );

                // No packet could be read.  Wait a for an interrupt to tell us
                // there is more data available.
                xSemaphoreTake( ETHRxBinSemaphore[0],
                                ( portTickType ) ( ETH_BLOCK_TIME_WAITING_FOR_INPUT_MS / portTICK_RATE_MS ) );
            }

        }
        while( p == NULL );

        LWIP_DEBUGF( NETIF_DEBUG, ( "ethernetif_input: frame received\n" ) );

        if( ERR_OK != netif->input( p, netif ) )
        {
            LWIP_DEBUGF( NETIF_DEBUG, ( "ethernetif_input: input error\n" ) );
            pbuf_free( p );
            p = NULL;
        }
    }
}

/**
 * Called from ethernetif_init
 *
 * This function with either place the packet into the Stellaris transmit fifo,
 * or will place the packet in the interface PBUF Queue for subsequent
 * transmission when the transmitter becomes idle.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 */
static err_t
low_level_output( struct netif *netif, struct pbuf *p )
{
    err_t           status;

    // Bump the reference count on the pbuf to prevent it from being
    // freed till we are done with it.
    pbuf_ref( p );

    // Prevent from simultaneously writing to ETH TX FIFO
    xSemaphoreTake( ETHTxAccessMutex[0], ( portTickType ) portMAX_DELAY );

    // If the transmitter is idle, send the pbuf now.
    if( ( ( HWREG( ETH_BASE + MAC_O_TR ) & MAC_TR_NEWTX ) == 0 ) )
    {
        // Send packet via eth controller
        status = low_level_transmit( netif, p );
    }
    else
    {
        LWIP_DEBUGF( NETIF_DEBUG, ( "low_level_output: Ethernet transmitter busy\n" ) );
        // Enable generating transmit interrupt for eth. controller
        EthernetIntEnable( ETH_BASE, ETH_INT_TX );

        // Waiting for finishing transmitting from interrupt routine
        xSemaphoreTake( ETHTxBinSemaphore[0], ( portTickType ) portMAX_DELAY );

        // Send packet via eth controller
        status = low_level_transmit( netif, p );

        // Disable generating transmit interrupt for eth. controller
        EthernetIntDisable( ETH_BASE, ETH_INT_TX );
    }

    // Release mutex
    xSemaphoreGive( ETHTxAccessMutex[0] );

    pbuf_free( p );

    return status;
}

/**
 * Called from low_level_output
 *
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf might be
 * chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 * @note This function MUST be called with interrupts disabled or with the
 *       Stellaris Ethernet transmit fifo protected.
 */
static err_t
low_level_transmit( struct netif *netif, struct pbuf *p )
{
    int             iBuf;
    unsigned char  *pucBuf;
    unsigned long  *pulBuf;
    struct pbuf    *q;
    int             iGather;
    unsigned long   ulGather;
    unsigned char  *pucGather;

    if( 0 == ETHServiceTaskLinkStatus( 0 ) )
    {
        // ~ bitwise negation, all bit except NETIF_FLAG_LINK_UP set to 1 and AND with current flag
        netif->flags &= ~NETIF_FLAG_LINK_UP;
        LWIP_DEBUGF( NETIF_DEBUG, ( "low_level_transmit: link is down\n" ) );
        LINK_STATS_INC( link.err );
        return ( ERR_IF );
    }
    else
    {
        netif->flags |= NETIF_FLAG_LINK_UP;
    }

    /**
	 * Fill in the first two bytes of the payload data (configured as padding
	 * with ETH_PAD_SIZE = 2) with the total length of the payload data
	 * (minus the Ethernet MAC layer header).
	 *
	 */
    *( ( unsigned short * )( p->payload ) ) = p->tot_len - 16;

    /* Initialize the gather register. */
    iGather = 0;
    pucGather = ( unsigned char * )&ulGather;
    ulGather = 0;

    /* Copy data from the pbuf(s) into the TX Fifo. */
    for( q = p; q != NULL; q = q->next )
    {
        /* Intialize a char pointer and index to the pbuf payload data. */
        pucBuf = ( unsigned char * )q->payload;
        iBuf = 0;

        /**
		 * If the gather buffer has leftover data from a previous pbuf
		 * in the chain, fill it up and write it to the Tx FIFO.
		 *
		 */
        while( ( iBuf < q->len ) && ( iGather != 0 ) )
        {
            /* Copy a byte from the pbuf into the gather buffer. */
            pucGather[iGather] = pucBuf[iBuf++];

            /* Increment the gather buffer index modulo 4. */
            iGather = ( ( iGather + 1 ) % 4 );
        }

        /**
		 * If the gather index is 0 and the pbuf index is non-zero,
		 * we have a gather buffer to write into the Tx FIFO.
		 *
		 */
        if( ( iGather == 0 ) && ( iBuf != 0 ) )
        {
            HWREG( ETH_BASE + MAC_O_DATA ) = ulGather;
            ulGather = 0;
        }

        /* Initialze a long pointer into the pbuf for 32-bit access. */
        pulBuf = ( unsigned long * )&pucBuf[iBuf];

        /**
		 * Copy words of pbuf data into the Tx FIFO, but don't go past
		 * the end of the pbuf.
		 *
		 */
        while( ( iBuf + 4 ) <= q->len )
        {
            HWREG( ETH_BASE + MAC_O_DATA ) = *pulBuf++;
            iBuf += 4;
        }

        /**
		 * Check if leftover data in the pbuf and save it in the gather
		 * buffer for the next time.
		 *
		 */
        while( iBuf < q->len )
        {
            /* Copy a byte from the pbuf into the gather buffer. */
            pucGather[iGather] = pucBuf[iBuf++];

            /* Increment the gather buffer index modulo 4. */
            iGather = ( ( iGather + 1 ) % 4 );
        }
    }

    /* Send any leftover data to the FIFO. */
    HWREG( ETH_BASE + MAC_O_DATA ) = ulGather;

    /* Wakeup the transmitter. */
    HWREG( ETH_BASE + MAC_O_TR ) = MAC_TR_NEWTX;

    LWIP_DEBUGF( NETIF_DEBUG, ( "low_level_transmit: frame sent\n" ) );

    LINK_STATS_INC( link.xmit );

    return ( ERR_OK );
}

/**
 * This function is passed to netif_add
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
ethernetif_init( struct netif * netif )
{
    LWIP_ASSERT( "netif != NULL", ( netif != NULL ) );

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
    NETIF_INIT_SNMP( netif, snmp_ifType_ethernet_csmacd, ETH_DEFAULT_LINK_SPEED );

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...)
     */
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    return low_level_init( netif );
}

//*****************************************************************************
//
//! Initializes the lwIP TCP/IP stack.
//! Call it from your main to initialize the lwip
//!
//! \param pucMAC is a pointer to a six byte array containing the MAC
//! address to be used for the interface.
//! \param ulIPAddr is the IP address to be used (static).
//! \param ulNetMask is the network mask to be used (static).
//! \param ulGWAddr is the Gateway address to be used (static).
//! \param ulIPMode is the IP Address Mode.  \b IPADDR_USE_STATIC will force
//! static IP addressing to be used, \b IPADDR_USE_DHCP will force DHCP with
//! fallback to Link Local (Auto IP), while \b IPADDR_USE_AUTOIP will force
//! Link Local only.
//!
//! This function performs initialization of the lwIP TCP/IP stack for the
//! Stellaris Ethernet MAC, including DHCP and/or AutoIP, as configured.
//!
//! \return None.
//
//*****************************************************************************
void
LWIPServiceTaskInit( void *pvParameters )
{
    struct ip_addr  ip_addr;
    struct ip_addr  net_mask;
    struct ip_addr  gw_addr;

    IP_CONFIG      *ipCfg = ( IP_CONFIG * ) pvParameters;

    LWIP_ASSERT( "pvParameters != NULL", ( pvParameters != NULL ) );

    // Check the parameters.
#if LWIP_DHCP && LWIP_AUTOIP
    ASSERT( ( ipCfg->IPMode == IPADDR_USE_STATIC ) ||
            ( ipCfg->IPMode == IPADDR_USE_DHCP ) || ( ipCfg->IPMode == IPADDR_USE_AUTOIP ) )
#elif LWIP_DHCP
    ASSERT( ( ipCfg->IPMode == IPADDR_USE_STATIC ) || ( ipCfg->IPMode == IPADDR_USE_DHCP ) )
#elif LWIP_AUTOIP
    ASSERT( ( ipCfg->IPMode == IPADDR_USE_STATIC ) || ( ipCfg->IPMode == IPADDR_USE_AUTOIP ) )
#else
    ASSERT( ipCfg->IPMode == IPADDR_USE_STATIC )
#endif
        // Start the TCP/IP thread & init stuff
        tcpip_init( NULL, NULL );

    vTaskDelay( 100 / portTICK_RATE_MS );

    // Setup the network address values.
    if( ipCfg->IPMode == IPADDR_USE_STATIC )
    {
        ip_addr.addr = htonl( ipCfg->IPAddr );
        net_mask.addr = htonl( ipCfg->NetMask );
        gw_addr.addr = htonl( ipCfg->GWAddr );
    }
#if LWIP_DHCP || LWIP_AUTOIP
    else
    {
        ip_addr.addr = 0;
        net_mask.addr = 0;
        gw_addr.addr = 0;
    }
#endif

    // Create, configure and add the Ethernet controller interface with
    // default settings.
    // WARNING: This must only be run after the OS has been started.
    // Typically this is the case, however, if not, you must place this
    // in a post-OS initialization
    // @SEE http://lwip.wikia.com/wiki/Initialization_using_tcpip.c
    netif_add( &lwip_netif, &ip_addr, &net_mask, &gw_addr, NULL, ethernetif_init, tcpip_input );
    netif_set_default( &lwip_netif );

    // Start DHCP, if enabled.
#if LWIP_DHCP
    if( ipCfg->IPMode == IPADDR_USE_DHCP )
    {
        LWIP_DEBUGF( DHCP_DEBUG, ( "----- Starting DHCP client -----\n" ) );
        dhcp_start( &lwip_netif );
    }
#endif

    // Start AutoIP, if enabled and DHCP is not.
#if LWIP_AUTOIP
    if( ipCfg->IPMode == IPADDR_USE_AUTOIP )
    {
        autoip_start( &lwip_netif );
    }
#endif

    if( ipCfg->IPMode == IPADDR_USE_STATIC )
    {
        // Bring the interface up.
        netif_set_up( &lwip_netif );
    }

    vTaskDelay( 1000 / portTICK_RATE_MS );

    while( 0 == netif_is_up( &lwip_netif ) )
    {
        vTaskDelay( 5000 / portTICK_RATE_MS );
    }
}

//*****************************************************************************
//
//! Returns the IP configuration for this interface.
//!
//! This function will read and return the currently assigned IP configuration for
//! the Stellaris Ethernet interface.
//!
//! \return ERR_OK if the interface is initialized
//!             ERR_IFF if the interface is not initialized
//
//*****************************************************************************
err_t
LWIPServiceTaskIPConfigGet( struct netif *netif, IP_CONFIG * ipCfg )
{
    LWIP_ASSERT( "netif != NULL", ( netif != NULL ) );

    if( ( netif == NULL ) || ( !( netif_is_up( netif ) ) ) )
    {
        return ERR_IF;
    }

    ipCfg->IPAddr = ( unsigned long )netif->ip_addr.addr;
    ipCfg->NetMask = ( unsigned long )netif->netmask.addr;
    ipCfg->GWAddr = ( unsigned long )netif->gw.addr;

    if( netif->flags & NETIF_FLAG_DHCP )
    {
        ipCfg->IPMode = IPADDR_USE_DHCP;
    }
    else
    {
        ipCfg->IPMode = IPADDR_USE_STATIC;
    }
    return ERR_OK;

}

//*****************************************************************************
//
//! Returns the local MAC/HW address for this interface.
//!
//! \param pucMAC is a pointer to an array of bytes used to store the MAC
//! address.
//!
//! This function will read the currently assigned MAC address into the array
//! passed in \e pucMAC.
//!
//! \return ERR_OK if the interface is initialized
//!             ERR_IFF if the interface is not initialized
//
//*****************************************************************************
err_t
LWIPServiceTaskMACGet( struct netif * netif, unsigned char *pucMAC )
{
    LWIP_ASSERT( "netif != NULL", ( netif != NULL ) );

    if( netif == NULL )
    {
        return ERR_IF;
    }

    memcpy( pucMAC, &( netif->hwaddr[0] ), ETH_HWADDR_LEN );
    return ERR_OK;

}

//*****************************************************************************
//
//! Change the configuration of the lwIP network interface.
//!
//! \param ulIPAddr is the new IP address to be used (static).
//! \param ulNetMask is the new network mask to be used (static).
//! \param ulGWAddr is the new Gateway address to be used (static).
//! \param ulIPMode is the IP Address Mode.  \b IPADDR_USE_STATIC 0 will force
//! static IP addressing to be used, \b IPADDR_USE_DHCP will force DHCP with
//! fallback to Link Local (Auto IP), while \b IPADDR_USE_AUTOIP will force
//! Link Local only.
//!
//! This function will evaluate the new configuration data.  If necessary, the
//! interface will be brought down, reconfigured, and then brought back up
//! with the new configuration.
//!
//! \return None.
//
//*****************************************************************************
void
lwIPNetworkConfigChange( struct netif *netif, IP_CONFIG * ipCfg )
{
    struct ip_addr  ip_addr;
    struct ip_addr  net_mask;
    struct ip_addr  gw_addr;

    IP_CONFIG       currentIPConfig;

    // Check the parameters.
#if LWIP_DHCP && LWIP_AUTOIP
    ASSERT( ( ipCfg->IPMode == IPADDR_USE_STATIC ) ||
            ( ipCfg->IPMode == IPADDR_USE_DHCP ) || ( ipCfg->IPMode == IPADDR_USE_AUTOIP ) )
#elif LWIP_DHCP
    ASSERT( ( ipCfg->IPMode == IPADDR_USE_STATIC ) || ( ipCfg->IPMode == IPADDR_USE_DHCP ) )
#elif LWIP_AUTOIP
    ASSERT( ( ipCfg->IPMode == IPADDR_USE_STATIC ) || ( ipCfg->IPMode == IPADDR_USE_AUTOIP ) )
#else
    ASSERT( ipCfg->IPMode == IPADDR_USE_STATIC )
#endif
        // Setup the network address values.
        if( ipCfg->IPMode == IPADDR_USE_STATIC )
    {
        ip_addr.addr = htonl( ipCfg->IPAddr );
        net_mask.addr = htonl( ipCfg->NetMask );
        gw_addr.addr = htonl( ipCfg->GWAddr );
    }
#if LWIP_DHCP || LWIP_AUTOIP
    else
    {
        ip_addr.addr = 0;
        net_mask.addr = 0;
        gw_addr.addr = 0;
    }
#endif

    // Switch on the current IP Address Aquisition mode.
    LWIPServiceTaskIPConfigGet( netif, &currentIPConfig );

    switch ( currentIPConfig.IPMode )
    {
        // Static IP

    case IPADDR_USE_STATIC:
        {
            // Set the new address parameters.  This will change the address
            // configuration in lwIP, and if necessary, will reset any links
            // that are active.  This is valid for all three modes.
            //
            netif_set_addr( netif, &ip_addr, &net_mask, &gw_addr );

            // If we are going to DHCP mode, then start the DHCP server now.
#if LWIP_DHCP
            if( ipCfg->IPMode == IPADDR_USE_DHCP )
            {
                dhcp_start( netif );
            }
#endif
            // If we are going to AutoIP mode, then start the AutoIP process
            // now.
#if LWIP_AUTOIP
            if( ipCfg->IPMode == IPADDR_USE_AUTOIP )
            {
                autoip_start( netif );
            }
#endif
            // And we're done.
            break;
        }

        // DHCP (with AutoIP fallback).
#if LWIP_DHCP
    case IPADDR_USE_DHCP:
        {
            //
            // If we are going to static IP addressing, then disable DHCP and
            // force the new static IP address.
            //
            if( ipCfg->IPMode == IPADDR_USE_STATIC )
            {
                dhcp_stop( netif );
                // SEE bug http://savannah.nongnu.org/bugs/?22804
                netif->flags &= ~NETIF_FLAG_DHCP;
                netif_set_addr( netif, &ip_addr, &net_mask, &gw_addr );
            }
            // If we are going to AUTO IP addressing, then disable DHCP, set
            // the default addresses, and start AutoIP.
#if LWIP_AUTOIP
            else if( ipCfg->IPMode == IPADDR_USE_AUTOIP )
            {
                dhcp_stop( netif );
                netif_set_addr( netif, &ip_addr, &net_mask, &gw_addr );
                autoip_start( netif );
            }
#endif
            break;
        }
#endif
        // AUTOIP
#if LWIP_AUTOIP
    case IPADDR_USE_AUTOIP:
        {
            //
            // If we are going to static IP addressing, then disable AutoIP and
            // force the new static IP address.
            //
            if( ipCfg->IPMode == IPADDR_USE_STATIC )
            {
                autoip_stop( netif );
                netif_set_addr( netif, &ip_addr, &net_mask, &gw_addr );
            }

            //
            // If we are going to DHCP addressing, then disable AutoIP, set the
            // default addresses, and start dhcp.
            //
#if LWIP_DHCP
            else if( ipCfg->IPMode == IPADDR_USE_AUTOIP )
            {
                autoip_stop( netif );
                netif_set_addr( netif, &ip_addr, &net_mask, &gw_addr );
                dhcp_start( netif );
            }
#endif
            break;
        }
#endif
    }
}

#if NETIF_DEBUG
/* Print an IP header by using LWIP_DEBUGF
 * @param p an IP packet, p->payload pointing to the IP header
 */
void
stellarisif_debug_print( struct pbuf *p )
{
    struct eth_hdr *ethhdr = ( struct eth_hdr * )p->payload;
    u16_t          *plen = ( u16_t * ) p->payload;

    LWIP_DEBUGF( NETIF_DEBUG, ( "ETH header:\n" ) );
    LWIP_DEBUGF( NETIF_DEBUG, ( "Packet Length:%5" U16_F " \n", *plen ) );
    LWIP_DEBUGF( NETIF_DEBUG,
                 ( "Destination: %02" X8_F "-%02" X8_F "-%02" X8_F "-%02" X8_F "-%02" X8_F "-%02" X8_F "\n",
                   ethhdr->dest.addr[0], ethhdr->dest.addr[1], ethhdr->dest.addr[2], ethhdr->dest.addr[3],
                   ethhdr->dest.addr[4], ethhdr->dest.addr[5] ) );
    LWIP_DEBUGF( NETIF_DEBUG,
                 ( "Source: %02" X8_F "-%02" X8_F "-%02" X8_F "-%02" X8_F "-%02" X8_F "-%02" X8_F "\n",
                   ethhdr->src.addr[0], ethhdr->src.addr[1], ethhdr->src.addr[2], ethhdr->src.addr[3],
                   ethhdr->src.addr[4], ethhdr->src.addr[5] ) );
    LWIP_DEBUGF( NETIF_DEBUG, ( "Packet Type:0x%04" U16_F " \n", ethhdr->type ) );
}
#endif /* NETIF_DEBUG */
