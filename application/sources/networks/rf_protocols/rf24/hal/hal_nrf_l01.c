/* Copyright (c) 2007 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT. 
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRENTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 * $LastChangedRevision: 2132 $
 */ 

 /** @file
 *
 * @author Runar Kjellhaug
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "nordic_common.h"
#include "hal_nrf.h"
#include "hal_nrf_reg.h"

#define HAL_NRF_SET_BIT(pos) ((uint8_t) (1<<( (uint8_t) (pos) )))
#define UINT8(t) ((uint8_t) (t))

void hal_nrf_set_irq_mode(hal_nrf_irq_source_t int_source, bool irq_state)
{
  if(irq_state)
  {
	hal_nrf_write_reg(CONFIG, hal_nrf_read_reg(CONFIG) & ~HAL_NRF_SET_BIT(int_source));
  }
  else
  {
	hal_nrf_write_reg(CONFIG, hal_nrf_read_reg(CONFIG) | HAL_NRF_SET_BIT(int_source));
  }
}

uint8_t hal_nrf_get_clear_irq_flags(void)
{
  return hal_nrf_write_reg(STATUS, (BIT_6|BIT_5|BIT_4)) & (BIT_6|BIT_5|BIT_4);
}

void hal_nrf_clear_irq_flag(hal_nrf_irq_source_t int_source)
{
  hal_nrf_write_reg(STATUS, HAL_NRF_SET_BIT(int_source));
}
 
bool hal_nrf_get_irq_mode(uint8_t int_type)
{
  if(hal_nrf_read_reg(CONFIG) & HAL_NRF_SET_BIT(int_type))
    return false;
  else
    return true;
}

uint8_t hal_nrf_get_irq_flags(void)
{
  return hal_nrf_nop() & (BIT_6|BIT_5|BIT_4);
}

void hal_nrf_set_crc_mode(hal_nrf_crc_mode_t crc_mode)
{
  hal_nrf_write_reg(CONFIG, (hal_nrf_read_reg(CONFIG) & ~(BIT_3|BIT_2)) | (UINT8(crc_mode)<<2));
}

void hal_nrf_open_pipe(hal_nrf_address_t pipe_num, bool auto_ack)
{
  switch(pipe_num)
  {
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
    case HAL_NRF_PIPE2:
    case HAL_NRF_PIPE3:
    case HAL_NRF_PIPE4:
    case HAL_NRF_PIPE5:
	  hal_nrf_write_reg(EN_RXADDR, hal_nrf_read_reg(EN_RXADDR) | HAL_NRF_SET_BIT(pipe_num));

      if(auto_ack)
		hal_nrf_write_reg(EN_AA, hal_nrf_read_reg(EN_AA) | HAL_NRF_SET_BIT(pipe_num));
      else
		hal_nrf_write_reg(EN_AA, hal_nrf_read_reg(EN_AA) & ~HAL_NRF_SET_BIT(pipe_num));
      break;

    case HAL_NRF_ALL:
	  hal_nrf_write_reg(EN_RXADDR, ((uint8_t)~(BIT_7|BIT_6)));

      if(auto_ack)
		hal_nrf_write_reg(EN_AA, ((uint8_t)~(BIT_7|BIT_6)));
      else
        hal_nrf_write_reg(EN_AA, 0);
      break;
      
    default:
      break;
  }
}

void hal_nrf_close_pipe(hal_nrf_address_t pipe_num)
{
  switch(pipe_num)
  {
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
    case HAL_NRF_PIPE2:
    case HAL_NRF_PIPE3:
    case HAL_NRF_PIPE4:
    case HAL_NRF_PIPE5:
	  hal_nrf_write_reg(EN_RXADDR, hal_nrf_read_reg(EN_RXADDR) & ~HAL_NRF_SET_BIT(pipe_num));
	  hal_nrf_write_reg(EN_AA, hal_nrf_read_reg(EN_AA) & ~HAL_NRF_SET_BIT(pipe_num));
      break;
    
    case HAL_NRF_ALL:
      hal_nrf_write_reg(EN_RXADDR, 0);
      hal_nrf_write_reg(EN_AA, 0);
      break;
      
    default:
      break;
  }
}

void hal_nrf_set_address(hal_nrf_address_t address, uint8_t *addr)
{
  switch(address)
  {
    case HAL_NRF_TX:
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
      hal_nrf_write_multibyte_reg((uint8_t) address, addr, 0);
      break;

    case HAL_NRF_PIPE2:
    case HAL_NRF_PIPE3:
    case HAL_NRF_PIPE4:
    case HAL_NRF_PIPE5:
      hal_nrf_write_reg(RX_ADDR_P0 + (uint8_t) address, *addr);
      break;

    default:
      break;
  }
}

void hal_nrf_set_auto_retr(uint8_t retr, uint16_t delay)
{
  hal_nrf_write_reg(SETUP_RETR, (((delay/250)-1)<<4) | retr);
}

void hal_nrf_set_address_width(hal_nrf_address_width_t address_width)
{
  hal_nrf_write_reg(SETUP_AW, (UINT8(address_width) - 2));
}

void hal_nrf_set_rx_pload_width(uint8_t pipe_num, uint8_t pload_width)
{
  hal_nrf_write_reg(RX_PW_P0 + pipe_num, pload_width);
}

uint8_t hal_nrf_get_crc_mode(void)
{
  return (hal_nrf_read_reg(CONFIG) & (BIT_3|BIT_2)) >> CRCO;
}

uint8_t hal_nrf_get_pipe_status(uint8_t pipe_num)
{
  uint8_t en_rx, en_aa;

  en_rx = hal_nrf_read_reg(EN_RXADDR) & (1<<pipe_num);
  en_aa = hal_nrf_read_reg(EN_AA) & (1<<pipe_num);

  en_rx >>= pipe_num;
  en_aa >>= pipe_num;

  return (en_aa << 1) + en_rx;
}

uint8_t hal_nrf_get_address(uint8_t address, uint8_t *addr)
{
  switch(address)
  {
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
    case HAL_NRF_TX:
      return hal_nrf_read_multibyte_reg(address, addr);

    default:
      *addr = hal_nrf_read_reg(RX_ADDR_P0 + address);
      return hal_nrf_get_address_width();
  }
}

uint8_t hal_nrf_get_auto_retr_status(void)
{
  return hal_nrf_read_reg(OBSERVE_TX);
}

uint8_t hal_nrf_get_packet_lost_ctr(void)
{
  return (hal_nrf_read_reg(OBSERVE_TX) & (BIT_7|BIT_6|BIT_5|BIT_4)) >> 4;
}

uint8_t hal_nrf_get_address_width(void)
{
  return (hal_nrf_read_reg(SETUP_AW) + 2);
}

uint8_t hal_nrf_get_rx_pload_width(uint8_t pipe_num)
{
  return hal_nrf_read_reg(RX_PW_P0 + pipe_num);
}

void hal_nrf_set_operation_mode(hal_nrf_operation_mode_t op_mode)
{
  if(op_mode == HAL_NRF_PRX)
  {
    hal_nrf_write_reg(CONFIG, (hal_nrf_read_reg(CONFIG) | (1<<PRIM_RX)));
  }
  else
  {
    hal_nrf_write_reg(CONFIG, (hal_nrf_read_reg(CONFIG) & ~(1<<PRIM_RX)));
  }
}

void hal_nrf_set_power_mode(hal_nrf_pwr_mode_t pwr_mode)
{
  if(pwr_mode == HAL_NRF_PWR_UP)
  {
    hal_nrf_write_reg(CONFIG, (hal_nrf_read_reg(CONFIG) | (1<<PWR_UP)));
  }
  else
  {
    hal_nrf_write_reg(CONFIG, (hal_nrf_read_reg(CONFIG) & ~(1<<PWR_UP)));
  }
}

void hal_nrf_set_rf_channel(uint8_t channel)
{
  hal_nrf_write_reg(RF_CH, channel);
}

void hal_nrf_set_output_power(hal_nrf_output_power_t power)
{
  hal_nrf_write_reg(RF_SETUP, (hal_nrf_read_reg(RF_SETUP) & ~((1<<RF_PWR1)|(1<<RF_PWR0))) | (UINT8(power)<<1));
}

void hal_nrf_set_datarate(hal_nrf_datarate_t datarate)
{
  if(datarate == HAL_NRF_1MBPS)
  {
    hal_nrf_write_reg(RF_SETUP, (hal_nrf_read_reg(RF_SETUP) & ~(1<<RF_DR)));
  }
  else
  {
    hal_nrf_write_reg(RF_SETUP, (hal_nrf_read_reg(RF_SETUP) | (1<<RF_DR)));
  }
}

uint8_t hal_nrf_get_operation_mode(void)
{
  return (hal_nrf_read_reg(CONFIG) & (1<<PRIM_RX)) >> PRIM_RX;
}

uint8_t hal_nrf_get_power_mode(void)
{
  return (hal_nrf_read_reg(CONFIG) & (1<<PWR_UP)) >> PWR_UP;
}

uint8_t hal_nrf_get_rf_channel(void)
{
  return hal_nrf_read_reg(RF_CH);
}

uint8_t hal_nrf_get_output_power(void)
{
  return (hal_nrf_read_reg(RF_SETUP) & ((1<<RF_PWR1)|(1<<RF_PWR0))) >> RF_PWR0;
}

uint8_t hal_nrf_get_datarate(void)
{
  return (hal_nrf_read_reg(RF_SETUP) & (1<<RF_DR)) >> RF_DR;
}

bool hal_nrf_rx_fifo_empty(void)
{
 if(hal_nrf_get_rx_data_source()==7)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool hal_nrf_rx_fifo_full(void)
{
  return (bool)((hal_nrf_read_reg(FIFO_STATUS) >> RX_EMPTY) & 1);
}

bool hal_nrf_tx_fifo_empty(void)
{
  return (bool)((hal_nrf_read_reg(FIFO_STATUS) >> TX_EMPTY) & 1);
}

bool hal_nrf_tx_fifo_full(void)
{
  return (bool)((hal_nrf_read_reg(FIFO_STATUS) >> TX_FIFO_FULL) & 1);
}

uint8_t hal_nrf_get_tx_fifo_status(void)
{
  return ((hal_nrf_read_reg(FIFO_STATUS) & ((1<<TX_FIFO_FULL)|(1<<TX_EMPTY))) >> 4);
}

uint8_t hal_nrf_get_rx_fifo_status(void)
{
  return (hal_nrf_read_reg(FIFO_STATUS) & ((1<<RX_FULL)|(1<<RX_EMPTY)));
}

uint8_t hal_nrf_get_fifo_status(void)
{
  return hal_nrf_read_reg(FIFO_STATUS);
}

uint8_t hal_nrf_get_transmit_attempts(void)
{
  return hal_nrf_read_reg(OBSERVE_TX) & (BIT_3|BIT_2|BIT_1|BIT_0);
}

bool hal_nrf_get_carrier_detect(void)
{
  return hal_nrf_read_reg(CD) & 1;
}

void hal_nrf_write_tx_pload(uint8_t *tx_pload, uint8_t length)
{
  hal_nrf_write_multibyte_reg(UINT8(HAL_NRF_TX_PLOAD), tx_pload, length);
}

void hal_nrf_setup_dyn_pl(uint8_t setup)
{
  hal_nrf_write_reg(DYNPD, setup & ~0xC0); 
}

void hal_nrf_enable_dynamic_pl(void)
{
  hal_nrf_write_reg(FEATURE, (hal_nrf_read_reg(FEATURE) | 0x04));   
}

void hal_nrf_disable_dynamic_pl(void)
{
  hal_nrf_write_reg(FEATURE, (hal_nrf_read_reg(FEATURE) & ~0x04));   
}

void hal_nrf_enable_ack_pl(void)
{
  hal_nrf_write_reg(FEATURE, (hal_nrf_read_reg(FEATURE) | 0x02));   
}

void hal_nrf_disable_ack_pl(void)
{
  hal_nrf_write_reg(FEATURE, (hal_nrf_read_reg(FEATURE) & ~0x02));   
}

void hal_nrf_enable_dynamic_ack(void)
{
  hal_nrf_write_reg(FEATURE, (hal_nrf_read_reg(FEATURE) | 0x01));   
}

void hal_nrf_disable_dynamic_ack(void)
{
  hal_nrf_write_reg(FEATURE, (hal_nrf_read_reg(FEATURE) & ~0x01));   
}

void hal_nrf_write_ack_pload(uint8_t pipe, uint8_t *tx_pload, uint8_t length)
{
  CSN_LOW();

  hal_nrf_rw(WR_ACK_PLOAD | pipe);
  while(length--)
  {
    hal_nrf_rw(*tx_pload++);
  }

  CSN_HIGH();
}

uint8_t hal_nrf_read_rx_pl_w()
{
  uint8_t temp;
  
  CSN_LOW();

  hal_nrf_rw(RD_RX_PLOAD_W);
  temp = hal_nrf_rw(0);
  CSN_HIGH();

  return temp;
}

void hal_nrf_lock_unlock()
{
  CSN_LOW();

  hal_nrf_rw(LOCK_UNLOCK);             
  hal_nrf_rw(0x73);

  CSN_HIGH();
}

uint8_t hal_nrf_get_rx_data_source(void)
{
  return ((hal_nrf_nop() & (BIT_3|BIT_2|BIT_1)) >> 1);
}

// Fixed: returns length==0 and pipe==7 means FIFO empty

uint16_t hal_nrf_read_rx_pload(uint8_t *rx_pload)
{
  return hal_nrf_read_multibyte_reg(UINT8(HAL_NRF_RX_PLOAD), rx_pload);
}

void hal_nrf_reuse_tx(void)
{
  hal_nrf_write_reg(REUSE_TX_PL, 0);
}

bool hal_nrf_get_reuse_tx_status(void)
{
  return (bool)((hal_nrf_get_fifo_status() & (1<<TX_REUSE)) >> TX_REUSE);
}

void hal_nrf_flush_rx(void)
{
  hal_nrf_write_reg(FLUSH_RX, 0);
}

void hal_nrf_flush_tx(void)
{
  hal_nrf_write_reg(FLUSH_TX, 0);
}

uint8_t hal_nrf_nop(void)
{
  return hal_nrf_write_reg(NOP,0);
}

void hal_nrf_set_pll_mode(hal_nrf_pll_mode_t pll_mode)
{
  if(pll_mode == HAL_NRF_PLL_LOCK)
  {
    hal_nrf_write_reg(RF_SETUP, (hal_nrf_read_reg(RF_SETUP) | (1<<PLL_LOCK)));
  }
  else
  {
    hal_nrf_write_reg(RF_SETUP, (hal_nrf_read_reg(RF_SETUP) & ~(1<<PLL_LOCK)));
  }
}

hal_nrf_pll_mode_t hal_nrf_get_pll_mode(void)
{
  return (hal_nrf_pll_mode_t)((hal_nrf_read_reg(RF_SETUP) & (1<<PLL_LOCK)) >> PLL_LOCK);
}

void hal_nrf_set_lna_gain(hal_nrf_lna_mode_t lna_gain)
{
  if(lna_gain == HAL_NRF_LNA_HCURR)
  {
    hal_nrf_write_reg(RF_SETUP, (hal_nrf_read_reg(RF_SETUP) | (1<<LNA_HCURR)));
  }
  else
  {
    hal_nrf_write_reg(RF_SETUP, (hal_nrf_read_reg(RF_SETUP) & ~(1<<LNA_HCURR)));
  }
}

hal_nrf_lna_mode_t hal_nrf_get_lna_gain(void)
{
  return (hal_nrf_lna_mode_t) ( (hal_nrf_read_reg(RF_SETUP) & (1<<LNA_HCURR)) >> LNA_HCURR );
}

uint8_t hal_nrf_read_reg(uint8_t reg)
{
uint8_t temp;
  CSN_LOW();
  hal_nrf_rw(reg);
  temp = hal_nrf_rw(0);
  CSN_HIGH();

  return temp;
}

uint8_t hal_nrf_write_reg(uint8_t reg, uint8_t value)
{
  uint8_t retval;
  CSN_LOW();
  if(reg < WRITE_REG_CMD)   // i.e. this is a register access
  {
	retval = hal_nrf_rw(WRITE_REG_CMD + reg);
    hal_nrf_rw(value);
  }
  else            // single byte cmd OR future command/register access
  {
    if(!(reg == FLUSH_TX) && !(reg == FLUSH_RX) && !(reg == REUSE_TX_PL) && !(reg == NOP))
    {
      retval = hal_nrf_rw(reg);
      hal_nrf_rw(value);
    }
    else          // single byte L01 command
    {
      retval = hal_nrf_rw(reg);
    }
  }
  CSN_HIGH();

  return retval;
}

uint16_t hal_nrf_read_multibyte_reg(uint8_t reg, uint8_t *pbuf)
{
uint8_t ctr, length;
  switch(reg)
  {
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
    case HAL_NRF_TX:
      length = ctr = hal_nrf_get_address_width();
      CSN_LOW();
      hal_nrf_rw(RX_ADDR_P0 + reg);
      break;
      
    case HAL_NRF_RX_PLOAD:
      if( (reg = hal_nrf_get_rx_data_source()) < 7)
      {
        length = ctr = hal_nrf_read_rx_pl_w();

        CSN_LOW();
        hal_nrf_rw(RD_RX_PLOAD);
      }
      else
      {
       ctr = length = 0;
      }
      break;

    default:
      ctr = length = 0;
      break;
  }

  while(ctr--)
  {
    *pbuf++ = hal_nrf_rw(0);
  }

  CSN_HIGH();

  return (((uint16_t) reg << 8) | length);
}

void hal_nrf_write_multibyte_reg(uint8_t reg, uint8_t *pbuf, uint8_t length)
{
  switch(reg)
  {
    case HAL_NRF_PIPE0:
    case HAL_NRF_PIPE1:
    case HAL_NRF_TX:
      length = hal_nrf_get_address_width();
      CSN_LOW();
	  hal_nrf_rw(WRITE_REG_CMD + RX_ADDR_P0 + reg);
      break;
      
    case HAL_NRF_TX_PLOAD:
      CSN_LOW();
      hal_nrf_rw(WR_TX_PLOAD);
      break;      
    default:
      break;
  }

  while(length--)
  {
    hal_nrf_rw(*pbuf++);
  }

  CSN_HIGH();
}
