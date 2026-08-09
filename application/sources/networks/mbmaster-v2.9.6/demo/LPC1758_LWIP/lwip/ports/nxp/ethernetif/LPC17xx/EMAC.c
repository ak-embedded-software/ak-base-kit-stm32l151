/*
 * MODBUS Library: NXP, FreeRTOS and lwIP Example
 * Copyright (c) 2010 Christian Walter <cwalter@embedded-solutions.at>
 * Copyright (c) 2001, Andreas Dannenberg
 * All rights reserved.
 *
 * $Id: EMAC.c,v 1.1 2011-01-02 16:15:47 embedded-solutions.cwalter Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* ----------------------- Platform includes --------------------------------*/
#include "EMAC.h"
#include "LPC17xx_reg.h"
#include "lpc17xx_netif.h"

/* ----------------------- Defines ------------------------------------------*/
#define MDC_MDIO_WORKAROUND         1
#define MDIO                        0x00000200
#define MDC                         0x00000100

/* ----------------------- Type Definitions -------------------------------*/

/* ----------------------- Global variables -------------------------------*/

/* ----------------------- Static variables -------------------------------*/
extern char     mac_adr[];
static unsigned short *rptr;
static unsigned short *tptr;
extern struct netif *lpc17xx_netif;
extern xSemaphoreHandle semEthTx;
extern xSemaphoreHandle semEthRx;
unsigned int    cnt = 0;

/* ----------------------- Start implementation ---------------------------*/
void
ENET_IRQHandler( void )
{
    static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    u32_t           status = LPC_EMAC->IntStatus;

    if( status & ( 1 << 3 ) )
    {
        xSemaphoreGiveFromISR( semEthRx, &xHigherPriorityTaskWoken );
        cnt++;
    }

    if( status & ( 1 << 7 ) )
    {
        xSemaphoreGiveFromISR( semEthTx, &xHigherPriorityTaskWoken );
    }

    LPC_EMAC->IntClear = status;

    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

#if MDC_MDIO_WORKAROUND

static void
delay( void )
{
}

static void
output_MDIO( unsigned int val, unsigned int n )
{
    /* Output a value to the MII PHY management interface. */
    for( val <<= ( 32 - n ); n; val <<= 1, n-- )
    {
        if( val & 0x80000000 )
        {
            LPC_GPIO2->FIOSET = MDIO;
        }
        else
        {
            LPC_GPIO2->FIOCLR = MDIO;
        }
        delay(  );
        LPC_GPIO2->FIOSET = MDC;
        delay(  );
        LPC_GPIO2->FIOCLR = MDC;
    }
}

static void
turnaround_MDIO( void )
{
    /* Turnaround MDO is tristated. */
    LPC_GPIO2->FIODIR &= ~MDIO;
    LPC_GPIO2->FIOSET = MDC;
    delay(  );
    LPC_GPIO2->FIOCLR = MDC;
    delay(  );
}

static unsigned int
input_MDIO( void )
{
    /* Input a value from the MII PHY management interface. */
    unsigned int    i, val = 0;

    for( i = 0; i < 16; i++ )
    {
        val <<= 1;
        LPC_GPIO2->FIOSET = MDC;
        delay(  );
        LPC_GPIO2->FIOCLR = MDC;
        if( LPC_GPIO2->FIOPIN & MDIO )
        {
            val |= 1;
        }
    }
    return ( val );
}
#endif

static void
write_PHY( unsigned int PhyReg, unsigned short Value )
{
    /* Write a data 'Value' to PHY register 'PhyReg'. */

#if MDC_MDIO_WORKAROUND
    /* Software MII Management for LPC175x. */
    /* Remapped MDC on P2.8 and MDIO on P2.9 do not work. */
    LPC_GPIO2->FIODIR |= MDIO;

    /* 32 consecutive ones on MDO to establish sync */
    output_MDIO( 0xFFFFFFFF, 32 );

    /* start code (01), write command (01) */
    output_MDIO( 0x05, 4 );

    /* write PHY address */
    output_MDIO( DP83848C_DEF_ADR >> 8, 5 );

    /* write the PHY register to write */
    output_MDIO( PhyReg, 5 );

    /* turnaround MDIO (1,0) */
    output_MDIO( 0x02, 2 );

    /* write the data value */
    output_MDIO( Value, 16 );

    /* turnaround MDO is tristated */
    turnaround_MDIO(  );
#else
    unsigned int    tout;

    /* Hardware MII Management for LPC176x devices. */
    LPC_EMAC->MADR = DP83848C_DEF_ADR | PhyReg;
    LPC_EMAC->MWTD = Value;

    /* Wait utill operation completed */
    for( tout = 0; tout < MII_WR_TOUT; tout++ )
    {
        if( ( LPC_EMAC->MIND & MIND_BUSY ) == 0 )
        {
            break;
        }
    }
#endif
}

static unsigned short
read_PHY( unsigned int PhyReg )
{
    /* Read a PHY register 'PhyReg'. */


#if MDC_MDIO_WORKAROUND
    unsigned int    val;

    /* Software MII Management for LPC175x. */
    /* Remapped MDC on P2.8 and MDIO on P2.9 does not work. */
    LPC_GPIO2->FIODIR |= MDIO;

    /* 32 consecutive ones on MDO to establish sync */
    output_MDIO( 0xFFFFFFFF, 32 );

    /* start code (01), read command (10) */
    output_MDIO( 0x06, 4 );

    /* write PHY address */
    output_MDIO( DP83848C_DEF_ADR >> 8, 5 );

    /* write the PHY register to write */
    output_MDIO( PhyReg, 5 );

    /* turnaround MDO is tristated */
    turnaround_MDIO(  );

    /* read the data value */
    val = input_MDIO(  );

    /* turnaround MDIO is tristated */
    turnaround_MDIO(  );
#else
    unsigned int    tout, val;

    LPC_EMAC->MADR = DP83848C_DEF_ADR | PhyReg;
    LPC_EMAC->MCMD = MCMD_READ;

    /* Wait until operation completed */
    for( tout = 0; tout < MII_RD_TOUT; tout++ )
    {
        if( ( LPC_EMAC->MIND & MIND_BUSY ) == 0 )
        {
            break;
        }
    }
    LPC_EMAC->MCMD = 0;
    val = LPC_EMAC->MRDD;
#endif
    return ( val );
}

void
rx_descr_init( void )
{
    unsigned int    i;

    for( i = 0; i < NUM_RX_FRAG; i++ )
    {
        RX_DESC_PACKET( i ) = RX_BUF( i );
        RX_DESC_CTRL( i ) = RCTRL_INT | ( ETH_FRAG_SIZE - 1 );
        RX_STAT_INFO( i ) = 0;
        RX_STAT_HASHCRC( i ) = 0;
    }

    /* Set EMAC Receive Descriptor Registers. */
    LPC_EMAC->RxDescriptor = RX_DESC_BASE;
    LPC_EMAC->RxStatus = RX_STAT_BASE;
    LPC_EMAC->RxDescriptorNumber = NUM_RX_FRAG - 1;

    /* Rx Descriptors Point to 0 */
    LPC_EMAC->RxConsumeIndex = 0;
}

void
tx_descr_init( void )
{
    unsigned int    i;

    for( i = 0; i < NUM_TX_FRAG; i++ )
    {
        TX_DESC_PACKET( i ) = TX_BUF( i );
        TX_DESC_CTRL( i ) = 0;
        TX_STAT_INFO( i ) = 0;
    }

    /* Set EMAC Transmit Descriptor Registers. */
    LPC_EMAC->TxDescriptor = TX_DESC_BASE;
    LPC_EMAC->TxStatus = TX_STAT_BASE;
    LPC_EMAC->TxDescriptorNumber = NUM_TX_FRAG - 1;

    /* Tx Descriptors Point to 0 */
    LPC_EMAC->TxProduceIndex = 0;
}

void
Init_EMAC( void )
{
    unsigned int    regv, tout, id1, id2;

    /* Power Up the EMAC controller. */
    LPC_SC->PCONP |= 0x40000000;

    LPC_PINCON->PINSEL2 = 0x50150105;
#if MDC_MDIO_WORKAROUND
    /* LPC175x devices, use software MII management. */
    LPC_PINCON->PINSEL4 &= ~0x000F0000;
    LPC_GPIO2->FIODIR |= MDC;
#else
    LPC_PINCON->PINSEL3 &= ~0x0000000F;
    LPC_PINCON->PINSEL3 |= 0x00000005;
#endif

    /* Reset all EMAC internal modules. */
    LPC_EMAC->MAC1 = MAC1_RES_TX | MAC1_RES_MCS_TX | MAC1_RES_RX | MAC1_RES_MCS_RX | MAC1_SIM_RES | MAC1_SOFT_RES;
    LPC_EMAC->Command = CR_REG_RES | CR_TX_RES | CR_RX_RES;

    /* A short delay after reset. */
    for( tout = 100; tout; tout-- );

    /* Initialize MAC control registers. */
    LPC_EMAC->MAC1 = MAC1_PASS_ALL;
    LPC_EMAC->MAC2 = MAC2_CRC_EN | MAC2_PAD_EN;
    LPC_EMAC->MAXF = ETH_MAX_FLEN;
    LPC_EMAC->CLRT = CLRT_DEF;
    LPC_EMAC->IPGR = IPGR_DEF;

    /* Enable Reduced MII interface. */
    LPC_EMAC->Command = CR_RMII | CR_PASS_RUNT_FRM;

    /* Reset Reduced MII Logic. */
    LPC_EMAC->SUPP = SUPP_RES_RMII;
    for( tout = 100; tout; tout-- );
    LPC_EMAC->SUPP = 0;

    /* Put the DP83848C in reset mode */
    write_PHY( PHY_REG_BMCR, 0x8000 );

    /* Wait for hardware reset to end. */
    for( tout = 0; tout < 0x100000; tout++ )
    {
        regv = read_PHY( PHY_REG_BMCR );
        if( !( regv & 0x8000 ) )
        {
            /* Reset complete */
            break;
        }
    }

    /* Check if this is a DP83848C PHY. */
    id1 = read_PHY( PHY_REG_IDR1 );
    id2 = read_PHY( PHY_REG_IDR2 );
    if( ( ( id1 << 16 ) | ( id2 & 0xFFF0 ) ) == DP83848C_ID )
    {
        /* Configure the PHY device */

        /* Use autonegotiation about the link speed. */
        write_PHY( PHY_REG_BMCR, PHY_AUTO_NEG );
        /* Wait to complete Auto_Negotiation. */
        for( tout = 0; tout < 0x100000; tout++ )
        {
            regv = read_PHY( PHY_REG_BMSR );
            if( regv & 0x0020 )
            {
                /* Autonegotiation Complete. */
                break;
            }
        }
    }

    /* Check the link status. */
    for( tout = 0; tout < 0x10000; tout++ )
    {
        regv = read_PHY( PHY_REG_STS );
        if( regv & 0x0001 )
        {
            /* Link is on. */
            break;
        }
    }

    /* Configure Full/Half Duplex mode. */
    if( regv & 0x0004 )
    {
        /* Full duplex is enabled. */
        LPC_EMAC->MAC2 |= MAC2_FULL_DUP;
        LPC_EMAC->Command |= CR_FULL_DUP;
        LPC_EMAC->IPGT = IPGT_FULL_DUP;
    }
    else
    {
        /* Half duplex mode. */
        LPC_EMAC->IPGT = IPGT_HALF_DUP;
    }

    /* Configure 100MBit/10MBit mode. */
    if( regv & 0x0002 )
    {
        /* 10MBit mode. */
        LPC_EMAC->SUPP = 0;
    }
    else
    {
        /* 100MBit mode. */
        LPC_EMAC->SUPP = SUPP_SPEED;
    }

    /* Set the Ethernet MAC Address registers */
    LPC_EMAC->SA0 = ( MYMAC_1 << 8 ) | MYMAC_2;
    LPC_EMAC->SA1 = ( MYMAC_3 << 8 ) | MYMAC_4;
    LPC_EMAC->SA2 = ( MYMAC_5 << 8 ) | MYMAC_6;

    /* Initialize Tx and Rx DMA Descriptors */
    rx_descr_init(  );
    tx_descr_init(  );

    /* Receive Broadcast and Perfect Match Packets */
    LPC_EMAC->RxFilterCtrl = RFC_BCAST_EN | RFC_PERFECT_EN;

    /* Enable EMAC interrupts. */
    LPC_EMAC->IntEnable = INT_RX_DONE | INT_TX_DONE | INT_RX_OVERRUN;

    /* Reset all interrupts */
    LPC_EMAC->IntClear = 0xFFFF;

    /* Enable receive and transmit mode of MAC Ethernet core */
    LPC_EMAC->Command |= ( CR_RX_EN | CR_TX_EN );
    LPC_EMAC->MAC1 |= MAC1_REC_EN;

    NVIC_SetPriority( ENET_IRQn, configEMAC_INTERRUPT_PRIORITY );
    NVIC_EnableIRQ( ENET_IRQn );
}

unsigned short
ReadFrame_EMAC( void )
{
    return ( *rptr++ );
}

void
CopyFromFrame_EMAC( void *Dest, unsigned short Size )
{
    unsigned short *piDest;

    piDest = Dest;
    while( Size > 1 )
    {
        *piDest++ = ReadFrame_EMAC(  );
        Size -= 2;
    }

    if( Size )
    {
        *( unsigned char * )piDest = ( char )ReadFrame_EMAC(  );
    }
}

void
DummyReadFrame_EMAC( unsigned short Size )
{
    while( Size > 1 )
    {
        ReadFrame_EMAC(  );
        Size -= 2;
    }
}

unsigned short
StartReadFrame( void )
{
    unsigned short  RxLen;
    unsigned int    idx;

    idx = LPC_EMAC->RxConsumeIndex;
    RxLen = ( RX_STAT_INFO( idx ) & RINFO_SIZE ) - 3;
    rptr = ( unsigned short * )RX_DESC_PACKET( idx );
    return ( RxLen );
}

void
EndReadFrame( void )
{
    unsigned int    idx;

    /* DMA free packet. */
    idx = LPC_EMAC->RxConsumeIndex;
    if( ++idx == NUM_RX_FRAG )
    {
        idx = 0;
    }
    LPC_EMAC->RxConsumeIndex = idx;
}

unsigned int
CheckFrameReceived( void )
{
    unsigned int uiRet = 0;
    if( LPC_EMAC->RxProduceIndex != LPC_EMAC->RxConsumeIndex )
    {
        uiRet = 1;
    }
    return uiRet;
}

void
RequestSend( unsigned short FrameSize )
{
    unsigned int    uiIdx;

    uiIdx = LPC_EMAC->TxProduceIndex;
    tptr = ( unsigned short * )TX_DESC_PACKET( uiIdx );
    TX_DESC_CTRL( uiIdx ) = FrameSize | TCTRL_LAST | TCTRL_INT;
}

unsigned int
Rdy4Tx( void )
{
    return 1;
}

void
WriteFrame_EMAC( unsigned short Data )
{
    *tptr++ = Data;
}

void
CopyToFrame_EMAC( void *Source, unsigned int Size )
{
    unsigned short *piSource;
    unsigned int    idx;

    piSource = Source;
    Size = ( Size + 1 ) & 0xFFFE;       // round Size up to next even number
    while( Size > 0 )
    {
        WriteFrame_EMAC( *piSource++ );
        Size -= 2;
    }

    idx = LPC_EMAC->TxProduceIndex;
    if( ++idx == NUM_TX_FRAG )
    {
        idx = 0;
    }
    LPC_EMAC->TxProduceIndex = idx;
}

void
CopyToFrame_EMAC_Start( void *Source, unsigned int Size )
{
    unsigned short *piSource;

    piSource = Source;
    Size = ( Size + 1 ) & 0xFFFE;
    while( Size > 0 )
    {
        WriteFrame_EMAC( *piSource++ );
        Size -= 2;
    }
}

void
CopyToFrame_EMAC_End( void )
{
    unsigned int    idx;

    idx = LPC_EMAC->TxProduceIndex;
    if( ++idx == NUM_TX_FRAG )
    {
        idx = 0;
    }
    LPC_EMAC->TxProduceIndex = idx;
}
