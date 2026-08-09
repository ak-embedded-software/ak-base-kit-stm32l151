/* Copyright (c) 2006 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of 
 * Nordic Semiconductor. The use, copying, transfer or disclosure of such
 * information is prohibited except by express written agreement with 
 * Nordic Semiconductor.
 */

/** @file
 * Implementation of #hal_nrf_rw.
 * #hal_nrf_rw is an SPI function which is hardware dependent. This file
 * contains an implementation for nRF24LU1.
 */


#include <stdint.h>

#include "hal_nrf.h"

uint8_t hal_nrf_rw(uint8_t value) {
	return nrf24l01_spi_rw(value);
}


