/*
 * MODBUS Library: NXP, FreeRTOS and lwIP Example
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * $Id: lpc17xx_netif.h,v 1.1 2011-01-02 16:15:47 embedded-solutions.cwalter Exp $
 */


#ifndef LPC17XX_NETIF_H_
#define LPC17XX_NETIF_H_

/* ----------------------- Platform includes --------------------------------*/
#include "lwip/debug.h"
#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "netif/etharp.h"

/* ----------------------- Function prototypes ------------------------------*/
err_t           lpc17xx_netif_init( struct netif *netif );
void            lpc17xx_netif_input( void *pvArg );

#endif
