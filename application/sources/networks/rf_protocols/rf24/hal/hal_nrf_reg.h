/* Copyright (c) 2006 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 * $Rev: 1731 $
 *
 */

/** @file
 * Register definitions for the nRF HAL module
 * @defgroup nordic_hal_nrf_reg nRF24L01 Register definitions
 * @{
 * @ingroup nordic_hal_nrf
 * Header file defining register mapping with bit definitions.\ This file is radio-chip dependent, and are included with the hal_nrf.h
 */

#ifndef HAL_NRF_REG_H__
#define HAL_NRF_REG_H__

/** @name - Instruction Set - */
//@{
/* nRF24L01 Instruction Definitions */
#define WRITE_REG_CMD   0x20  /**< Register write command */
#define RD_RX_PLOAD_W   0x60  /**< Read RX payload command */
#define RD_RX_PLOAD   0x61  /**< Read RX payload command */
#define WR_TX_PLOAD   0xA0  /**< Write TX payload command */
#define WR_ACK_PLOAD  0xA8  /**< Write ACK payload command */
#define WR_NAC_TX_PLOAD 0xB0  /**< Write ACK payload command */
#define FLUSH_TX      0xE1  /**< Flush TX register command */
#define FLUSH_RX      0xE2  /**< Flush RX register command */
#define REUSE_TX_PL   0xE3  /**< Reuse TX payload command */
#define LOCK_UNLOCK   0x50  /**< Lock/unlcok exclusive features */

#define NOP           0xFF  /**< No Operation command, used for reading status register */
//@}

/** @name  - Register Memory Map - */
//@{
/* nRF24L01 * Register Definitions * */
#define CONFIG        0x00  /**< nRF24L01 config register */
#define EN_AA         0x01  /**< nRF24L01 enable Auto-Acknowledge register */
#define EN_RXADDR     0x02  /**< nRF24L01 enable RX addresses register */
#define SETUP_AW      0x03  /**< nRF24L01 setup of address width register */
#define SETUP_RETR    0x04  /**< nRF24L01 setup of automatic retransmission register */
#define RF_CH         0x05  /**< nRF24L01 RF channel register */
#define RF_SETUP      0x06  /**< nRF24L01 RF setup register */
#define STATUS        0x07  /**< nRF24L01 status register */
#define OBSERVE_TX    0x08  /**< nRF24L01 transmit observe register */
#define CD            0x09  /**< nRF24L01 carrier detect register */
#define RX_ADDR_P0    0x0A  /**< nRF24L01 receive address data pipe0 */
#define RX_ADDR_P1    0x0B  /**< nRF24L01 receive address data pipe1 */
#define RX_ADDR_P2    0x0C  /**< nRF24L01 receive address data pipe2 */
#define RX_ADDR_P3    0x0D  /**< nRF24L01 receive address data pipe3 */
#define RX_ADDR_P4    0x0E  /**< nRF24L01 receive address data pipe4 */
#define RX_ADDR_P5    0x0F  /**< nRF24L01 receive address data pipe5 */
#define TX_ADDR       0x10  /**< nRF24L01 transmit address */
#define RX_PW_P0      0x11  /**< nRF24L01 \# of bytes in rx payload for pipe0 */
#define RX_PW_P1      0x12  /**< nRF24L01 \# of bytes in rx payload for pipe1 */
#define RX_PW_P2      0x13  /**< nRF24L01 \# of bytes in rx payload for pipe2 */
#define RX_PW_P3      0x14  /**< nRF24L01 \# of bytes in rx payload for pipe3 */
#define RX_PW_P4      0x15  /**< nRF24L01 \# of bytes in rx payload for pipe4 */
#define RX_PW_P5      0x16  /**< nRF24L01 \# of bytes in rx payload for pipe5 */
#define FIFO_STATUS   0x17  /**< nRF24L01 FIFO status register */
#define DYNPD         0x1C  /**< nRF24L01 Dynamic payload setup */
#define FEATURE       0x1D  /**< nRF24L01 Exclusive feature setup */

//@}

/* nRF24L01 related definitions */
/* Interrupt definitions */
/* Operation mode definitions */

/** An enum describing the radio's irq sources.
 *
 */
typedef enum {
    HAL_NRF_MAX_RT = 4,     /**< Max retries interrupt */
    HAL_NRF_TX_DS,          /**< TX data sent interrupt */
    HAL_NRF_RX_DR           /**< RX data received interrupt */
} hal_nrf_irq_source_t;

/* Operation mode definitions */
/** An enum describing the radio's power mode.
 *
 */
typedef enum {
    HAL_NRF_PTX,            /**< Primary TX operation */
    HAL_NRF_PRX             /**< Primary RX operation */
} hal_nrf_operation_mode_t;

/** An enum describing the radio's power mode.
 *
 */
typedef enum {
    HAL_NRF_PWR_DOWN,       /**< Device power-down */
    HAL_NRF_PWR_UP          /**< Device power-up */
} hal_nrf_pwr_mode_t;

/** An enum describing the radio's output power mode's.
 *
 */
typedef enum {
    HAL_NRF_18DBM,          /**< Output power set to -18dBm */
    HAL_NRF_12DBM,          /**< Output power set to -12dBm */
    HAL_NRF_6DBM,           /**< Output power set to -6dBm  */
    HAL_NRF_0DBM            /**< Output power set to 0dBm   */
} hal_nrf_output_power_t;

/** An enum describing the radio's on-air datarate.
 *
 */
typedef enum {
    HAL_NRF_1MBPS,          /**< Datarate set to 1 Mbps  */
    HAL_NRF_2MBPS           /**< Datarate set to 2 Mbps  */
} hal_nrf_datarate_t;

/** An enum describing the radio's PLL mode.
 *
 */
typedef enum {
    HAL_NRF_PLL_UNLOCK,     /**< PLL unlocked, normal operation  */
    HAL_NRF_PLL_LOCK        /**< PLL locked, test mode  */
} hal_nrf_pll_mode_t;

/** An enum describing the radio's LNA mode.
 *
 */
typedef enum {
    HAL_NRF_LNA_LCURR,      /**< LNA set to low current mode */
    HAL_NRF_LNA_HCURR       /**< LNA set to high current mode */
} hal_nrf_lna_mode_t;

/** An enum describing the radio's CRC mode.
 *
 */
typedef enum {
    HAL_NRF_CRC_OFF,        /**< CRC check disabled */
    HAL_NRF_CRC_8BIT = 2,   /**< CRC check set to 8-bit */
    HAL_NRF_CRC_16BIT       /**< CRC check set to 16-bit */
} hal_nrf_crc_mode_t;

/** An enum describing the read/write payload command.
 *
 */
typedef enum {
    HAL_NRF_TX_PLOAD = 7,   /**< TX payload definition */
    HAL_NRF_RX_PLOAD,        /**< RX payload definition */
    HAL_NRF_ACK_PLOAD
} hal_nrf_pload_command_t;

/** An enum describing the nRF24L01 pipe addresses and TX address.
 *
 */
typedef enum {
    HAL_NRF_PIPE0,              /**< Select pipe0 */
    HAL_NRF_PIPE1,              /**< Select pipe1 */
    HAL_NRF_PIPE2,              /**< Select pipe2 */
    HAL_NRF_PIPE3,              /**< Select pipe3 */
    HAL_NRF_PIPE4,              /**< Select pipe4 */
    HAL_NRF_PIPE5,              /**< Select pipe5 */
    HAL_NRF_TX,                 /**< Refer to TX address*/
    HAL_NRF_ALL = 0xFF          /**< Close or open all pipes*/
                                /**< @see hal_nrf_set_address @see hal_nrf_get_address
                                 @see hal_nrf_open_pipe  @see hal_nrf_close_pipe */
} hal_nrf_address_t;

/** An enum describing the radio's address width.
 *
 */
typedef enum {
    HAL_NRF_AW_3BYTES = 3,      /**< Set address width to 3 bytes */
    HAL_NRF_AW_4BYTES,          /**< Set address width to 4 bytes */
    HAL_NRF_AW_5BYTES           /**< Set address width to 5 bytes */
} hal_nrf_address_width_t;


/** @name CONFIG register bit definitions */
//@{

#define MASK_RX_DR    6     /**< CONFIG register bit 6 */
#define MASK_TX_DS    5     /**< CONFIG register bit 5 */
#define MASK_MAX_RT   4     /**< CONFIG register bit 4 */
#define EN_CRC        3     /**< CONFIG register bit 3 */
#define CRCO          2     /**< CONFIG register bit 2 */
#define PWR_UP        1     /**< CONFIG register bit 1 */
#define PRIM_RX       0     /**< CONFIG register bit 0 */
//@}

/** @name RF_SETUP register bit definitions */
//@{
#define PLL_LOCK      4     /**< RF_SETUP register bit 4 */
#define RF_DR         3     /**< RF_SETUP register bit 3 */
#define RF_PWR1       2     /**< RF_SETUP register bit 2 */
#define RF_PWR0       1     /**< RF_SETUP register bit 1 */
#define LNA_HCURR     0     /**< RF_SETUP register bit 0 */
//@}

/* STATUS 0x07 */
/** @name STATUS register bit definitions */
//@{
#define RX_DR         6     /**< STATUS register bit 6 */
#define TX_DS         5     /**< STATUS register bit 5 */
#define MAX_RT        4     /**< STATUS register bit 4 */
#define TX_FULL       0     /**< STATUS register bit 0 */
//@}

/* FIFO_STATUS 0x17 */
/** @name FIFO_STATUS register bit definitions */
//@{
#define TX_REUSE      6     /**< FIFO_STATUS register bit 6 */
#define TX_FIFO_FULL  5     /**< FIFO_STATUS register bit 5 */
#define TX_EMPTY      4     /**< FIFO_STATUS register bit 4 */
#define RX_FULL       1     /**< FIFO_STATUS register bit 1 */
#define RX_EMPTY      0     /**< FIFO_STATUS register bit 0 */
//@}

#endif // HAL_NRF_REG_H__
/** @} */
