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

#define MSBFIRST            1
#define LSBFIRST            0

class SPIClass {
private:
    uint8_t SSIModule;
    uint8_t SSIBitOrder;

public:
    SPIClass(void);
    SPIClass(uint8_t);

    void begin();
    void end();

    void setBitOrder(uint8_t);
    void setBitOrder(uint8_t, uint8_t);

    void setDataMode(uint8_t);

    void setClockDivider(uint8_t);

    uint8_t transfer(uint8_t);

    void setModule(uint8_t);
};

extern SPIClass SPI;

#endif //__SPI_H__
