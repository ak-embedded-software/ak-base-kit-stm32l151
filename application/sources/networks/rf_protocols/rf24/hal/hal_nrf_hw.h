/** @file
 *
 * Header file defining hardware dependent functions for nRF24LU1.
 *
 * @addtogroup nordic_hal_nrf
 *
 * @{
 * @defgroup nordic_hal_nrf_hw nRF24L01 HW dependents.
 * @{
 *
 * $Rev: 1731 $
 *
 */

#ifndef _HAL_NRF_AK_H_
#define _HAL_NRF_AK_H_

#include "sys_ctrl.h"
#include "sys_io.h"

/** Macro that set radio's CSN line LOW.
 *
 */
#define CSN_LOW() do { nrf24l01_csn_low(); } while(0)

/** Macro that set radio's CSN line HIGH.
 *
 */
#define CSN_HIGH() do { nrf24l01_csn_high(); } while(0)

/** Macro that set radio's CE line LOW.
 *
 */
#define CE_LOW() do { nrf24l01_ce_low(); } while(0)

/** Macro that set radio's CE line HIGH.
 *
 */
#define CE_HIGH() do { nrf24l01_ce_high(); } while(0)

/**
 * Pulses the CE to nRF24L01 for at least 10 us
 */
#define CE_PULSE() do { \
  uint8_t count; \
  count = 20; \
  CE_HIGH();  \
  while(count--) \
    ; \
  CE_LOW();  \
  } while(0)

#endif

/** @} */
/** @} */
