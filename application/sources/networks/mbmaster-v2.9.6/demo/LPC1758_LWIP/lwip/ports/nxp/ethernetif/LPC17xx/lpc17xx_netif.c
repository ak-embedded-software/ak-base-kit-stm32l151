/*
 * MODBUS Library: NXP, FreeRTOS and lwIP Example
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * Copyright (c) 2001, Andreas Dannenberg
 * All rights reserved.
 *
 * $Id: lpc17xx_netif.c,v 1.1 2011-01-02 16:15:47 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* ----------------------- Platform includes --------------------------------*/
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "netif/etharp.h"
#include "netif/ppp_oe.h"
#include "lpc17xx_netif.h"
#include "emac.h"

/* ----------------------- Defines ------------------------------------------*/
#define IFNAME0                             'n'
#define IFNAME1                             'x'

#define netifINTERFACE_TASK_STACK_SIZE      ( 256 )
#define netifINTERFACE_TASK_PRIORITY        ( configMAX_PRIORITIES )

#define ETH_MTU		                       1500

/* ----------------------- Global variables -------------------------------*/
xSemaphoreHandle semEthTx;
xSemaphoreHandle semEthRx;

/* ----------------------- Start implementation ---------------------------*/
static void
low_level_init( struct netif *netif )
{
    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set MAC hardware address */
    netif->hwaddr[5] = MYMAC_1;
    netif->hwaddr[4] = MYMAC_2;
    netif->hwaddr[3] = MYMAC_3;
    netif->hwaddr[2] = MYMAC_4;
    netif->hwaddr[1] = MYMAC_5;
    netif->hwaddr[0] = MYMAC_6;

    /* maximum transfer unit */
    netif->mtu = ETH_MTU;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    /* Do whatever else is needed to initialize interface. */
    semEthTx = xSemaphoreCreateCounting( NUM_TX_FRAG, NUM_TX_FRAG );
    semEthRx = xSemaphoreCreateCounting( NUM_RX_FRAG, 0 );

    sys_thread_new( "lpc17xx_eth", lpc17xx_netif_input, netif, netifINTERFACE_TASK_STACK_SIZE, netifINTERFACE_TASK_PRIORITY );

    Init_EMAC(  );
}

static          err_t
low_level_output( struct netif *netif, struct pbuf *p )
{
    struct pbuf    *q;

    pbuf_ref( p );

#if ETH_PAD_SIZE
    pbuf_header( p, -ETH_PAD_SIZE );    /* drop the padding word */
#endif

    if( pdTRUE == xSemaphoreTake( semEthTx, portMAX_DELAY ) )
    {
        RequestSend( p->tot_len );

        for( q = p; q != NULL; q = q->next )
        {
            /* Send the data from the pbuf to the interface, one pbuf at a
               time. The size of the data in each pbuf is kept in the ->len
               variable. */
            CopyToFrame_EMAC_Start( q->payload, q->len );
        }
        CopyToFrame_EMAC_End(  );
    }

    pbuf_free( p );

#if ETH_PAD_SIZE
    pbuf_header( p, ETH_PAD_SIZE );     /* reclaim the padding word */
#endif

    LINK_STATS_INC( link.xmit );

    return ERR_OK;
}

static struct pbuf *
low_level_input( struct netif *netif )
{
    struct pbuf    *p, *q;
    u16_t           len;

    /* Obtain the size of the packet and put it into the "len"
       variable. */

    len = StartReadFrame(  );

#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE;        /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc( PBUF_RAW, len, PBUF_POOL );

    if( p != NULL )
    {

#if ETH_PAD_SIZE
        pbuf_header( p, -ETH_PAD_SIZE );        /* drop the padding word */
#endif


        /* We iterate over the pbuf chain until we have read the entire
         * packet into the pbuf. */
        for( q = p; q != NULL; q = q->next )
        {
            /* Read enough bytes to fill this pbuf in the chain. The
             * available data in the pbuf is given by the q->len
             * variable. */
            CopyFromFrame_EMAC( q->payload, q->len );
        }
        /* acknowledge that packet has been read */
        EndReadFrame(  );

#if ETH_PAD_SIZE
        pbuf_header( p, ETH_PAD_SIZE ); /* reclaim the padding word */
#endif

        LINK_STATS_INC( link.recv );
    }
    else
    {
        EndReadFrame(  );
        LINK_STATS_INC( link.memerr );
        LINK_STATS_INC( link.drop );
    }

    return p;
}

void
lpc17xx_netif_input( void *pvArg )
{
    struct netif   *netif = pvArg;
    struct eth_hdr *ethhdr;
    struct pbuf    *p;

    for( ;; )
    {
        if( pdTRUE == xSemaphoreTake( semEthRx, portMAX_DELAY ) )
        {
            /* move received packet into a new pbuf */
            p = low_level_input( netif );
            /* no packet could be read, silently ignore this */
            if( p == NULL )
            {
                continue;
            }
            /* points to packet payload, which starts with an Ethernet header */
            ethhdr = p->payload;

            switch ( htons( ethhdr->type ) )
            {
                /* IP or ARP packet? */
            case ETHTYPE_IP:
            case ETHTYPE_ARP:
#if PPPOE_SUPPORT
                /* PPPoE packet? */
            case ETHTYPE_PPPOEDISC:
            case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
                /* full packet send to tcpip_thread to process */
                if( netif->input( p, netif ) != ERR_OK )
                {
                    LWIP_DEBUGF( NETIF_DEBUG, ( "lpc17xx_netif_input: IP input error\n" ) );
                    pbuf_free( p );
                    p = NULL;
                }
                break;
            default:
                pbuf_free( p );
                p = NULL;
                break;
            }
        }
    }
}

err_t
lpc17xx_netif_init( struct netif *netif )
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
    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100000000 );

    netif->state = NULL;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    /* initialize the hardware */
    low_level_init( netif );

    return ERR_OK;
}
