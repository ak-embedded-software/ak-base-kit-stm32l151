#ifndef __SPI_H__
#define __SPI_H__

#include <stdint.h>
#include "stm32l1xx.h"
#include "stm32l1xx_spi.h"

#define SPI_CLOCK_DIV2      SPI_BaudRatePrescaler_2
#define SPI_CLOCK_DIV4      SPI_BaudRatePrescaler_4
#define SPI_CLOCK_DIV8      SPI_BaudRatePrescaler_8
#define SPI_CLOCK_DIV16     SPI_BaudRatePrescaler_16
#define SPI_CLOCK_DIV32     SPI_BaudRatePrescaler_32
#define SPI_CLOCK_DIV64     SPI_BaudRatePrescaler_64
#define SPI_CLOCK_DIV128    SPI_BaudRatePrescaler_128
#define SPI_CLOCK_DIV256    SPI_BaudRatePrescaler_256

/*
    SPI_MODE0	0	0	Falling	Rising
    SPI_MODE1	0	1	Rising	Falling
    SPI_MODE2	1	0	Rising	Falling
    SPI_MODE3	1	1	Falling	Rising
*/
#define SPI_MODE0           0x00
#define SPI_MODE1           0x80
#define SPI_MODE2           0x40
#define SPI_MODE3           0xC0

#ifndef LSBFIRST
#define LSBFIRST 0
#endif
#ifndef MSBFIRST
#define MSBFIRST 1
#endif


class SPISettings {
public:
  SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {
	  spi_config(clock, bitOrder, dataMode);
  }

  SPISettings() {
	spi_config(4000000, MSBFIRST, SPI_MODE0);
  }
private:
  uint32_t setting_clock;
  uint8_t setting_bitorder;
  uint8_t setting_datamode;

  void spi_config(uint32_t clock, uint8_t bitOrder, uint8_t dataMode) {
	setting_clock = clock;
	setting_bitorder = bitOrder;
	setting_datamode = dataMode;
  }

  friend class SPIClass;
};


class SPIClass {
private:
	uint32_t spi_clock;
	uint8_t spi_bitorder;
	uint8_t spi_datamode;
	bool nss_soft;
	SPI_InitTypeDef   spi_init;

	void io_config();

public:
    SPIClass(void);
    SPIClass(uint8_t);

    void begin();
    void end();

	void beginTransaction(SPISettings settings);
	void endTransaction(void);

    void setBitOrder(uint8_t);
    void setBitOrder(uint8_t, uint8_t);

    void setDataMode(uint8_t);
    void setClockDivider(uint8_t);

    uint8_t transfer(uint8_t);
	uint16_t transfer16(uint16_t data);
	void transfer(void *buf, uint32_t count);

    void setModule(uint8_t);
};

extern SPIClass SPI;

#endif //__SPI_H__
