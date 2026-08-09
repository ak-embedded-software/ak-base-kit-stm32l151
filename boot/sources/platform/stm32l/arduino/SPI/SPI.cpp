#include "SPI.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_rcc.h"

#define NOT_ACTIVE      (0xFF)

SPIClass::SPIClass(void) {
	SSIModule = NOT_ACTIVE;
	SSIBitOrder = MSBFIRST;
}

SPIClass::SPIClass(uint8_t module) {
	SSIModule = module;
	SSIBitOrder = MSBFIRST;
}

void SPIClass::begin() {
	GPIO_InitTypeDef  GPIO_InitStructure;
	SPI_InitTypeDef   SPI_InitStructure;

	/*!< SPI GPIO Periph clock enable */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	/*!< SPI Periph clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	/*!< Configure SPI pins: SCK */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*!< Configure SPI pins: MISO */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*!< Configure SPI pins: MOSI */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Connect PXx to SPI_SCK */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);

	/* Connect PXx to SPI_MISO */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);

	/* Connect PXx to SPI_MOSI */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

	/*!< SPI Config */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;

	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Cmd(SPI1, ENABLE); /*!< SPI enable */
}

void SPIClass::end() {
	/* disable SPI module */
}

void SPIClass::setBitOrder(uint8_t ssPin, uint8_t bitOrder) {
	(void)ssPin;
	SSIBitOrder = bitOrder;
}

void SPIClass::setBitOrder(uint8_t bitOrder) {
	SSIBitOrder = bitOrder;
}

void SPIClass::setDataMode(uint8_t mode) {
	/*
	SPI_MODE0	0	0	Falling	Rising
	SPI_MODE1	0	1	Rising	Falling
	SPI_MODE2	1	0	Rising	Falling
	SPI_MODE3	1	1	Falling	Rising
	*/
	SPI_InitTypeDef   spi_init;

	switch(mode) {
	case SPI_MODE0:
		spi_init.SPI_CPOL = SPI_CPOL_Low;
		spi_init.SPI_CPHA = SPI_CPHA_1Edge;
		break;

	case SPI_MODE1:
		spi_init.SPI_CPOL = SPI_CPOL_Low;
		spi_init.SPI_CPHA = SPI_CPHA_2Edge;
		break;

	case SPI_MODE2:
		spi_init.SPI_CPOL = SPI_CPOL_High;
		spi_init.SPI_CPHA = SPI_CPHA_1Edge;
		break;

	case SPI_MODE3:
		spi_init.SPI_CPOL = SPI_CPOL_High;
		spi_init.SPI_CPHA = SPI_CPHA_2Edge;
		break;

	default:
		spi_init.SPI_CPOL = SPI_CPOL_Low;
		spi_init.SPI_CPHA = SPI_CPHA_1Edge;
		break;
	}

	SPI_Init(SPI1, &spi_init);
}

void SPIClass::setClockDivider(uint8_t divider){
	SPI_InitTypeDef   spi_init;
	spi_init.SPI_BaudRatePrescaler = (uint16_t)divider;
	SPI_Init(SPI1, &spi_init);
}

uint8_t SPIClass::transfer(uint8_t data) {
	unsigned long rxtxData = data;

	if(SSIBitOrder == LSBFIRST) {
		asm("rbit %0, %1" : "=r" (rxtxData) : "r" (rxtxData));	// reverse order of 32 bits
		asm("rev %0, %1" : "=r" (rxtxData) : "r" (rxtxData));	// reverse order of bytes to get original bits into lowest byte
	}

	/* waiting send idle then send data */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI1, (uint8_t)rxtxData);

	/* waiting conplete rev data */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	rxtxData = (uint8_t)SPI_I2S_ReceiveData(SPI1);

	if (SSIBitOrder == LSBFIRST) {
		asm("rbit %0, %1" : "=r" (rxtxData) : "r" (rxtxData));	// reverse order of 32 bits
		asm("rev %0, %1" : "=r" (rxtxData) : "r" (rxtxData));	// reverse order of bytes to get original bits into lowest byte
	}

	return (uint8_t)rxtxData;
}

void SPIClass::setModule(uint8_t module) {
	SSIModule = module;
	begin();
}

SPIClass SPI;
