#ifndef LWIPSTACK_H_
#define LWIPSTACK_H_

#define netifINTERFACE_TASK_STACK_SIZE				( 350 )
#define netifINTERFACE_TASK_PRIORITY				( configMAX_PRIORITIES)
#define netifBUFFER_WAIT_ATTEMPTS					10
#define netifBUFFER_WAIT_DELAY						(10 / portTICK_RATE_MS)
#define IFNAME0 'l'
#define IFNAME1 'm'
#define ETH_BLOCK_TIME_WAITING_FOR_INPUT_MS (5000)

typedef struct
{
    unsigned long   IPAddr;
    unsigned long   NetMask;
    unsigned long   GWAddr;
    unsigned long   IPMode;
} IP_CONFIG;

//*****************************************************************************
//
// IP Address Acquisition Modes
//
//*****************************************************************************
#define IPADDR_USE_STATIC       0
#define IPADDR_USE_DHCP         1
#define IPADDR_USE_AUTOIP       2

#define IP2LONG(a,b,c,d) ( (((a) << 24) & 0xFF000000) | \
						   (((b) << 16) & 0x00FF0000) | \
						   (((c) <<  8) & 0x0000FF00) | \
						   (((d) <<  0) & 0x000000FF) )

//*****************************************************************************
//
//! Prototypes for the APIs.
//
//*****************************************************************************
extern void     LWIPServiceTaskInit( void *pvParameters );
#if NETIF_DEBUG
void            stellarisif_debug_print( struct pbuf *p );
#else
#define stellarisif_debug_print(p)
#endif /* NETIF_DEBUG */

#endif /*LWIPSTACK_H_ */
