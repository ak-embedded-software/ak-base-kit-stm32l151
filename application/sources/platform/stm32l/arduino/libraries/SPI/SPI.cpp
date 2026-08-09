#include "SPI.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_rcc.h"

SPIClass::SPIClass(void) {
	spi_clock = 1000000;
	spi_bitorder = MSBFIRST;
	spi_datamode = SPI_MODE0;
}

SPIClass::SPIClass(uint8_t module) {
	(void)module;
	spi_clock = 1000000;
	spi_bitorder = MSBFIRST;
	spi_datamode = SPI_MODE0;
}

void SPIClass::io_config() {
	GPIO_InitTypeDef  GPIO_InitStructure;

	/*!< SPI GPIO Periph clock enable */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	/*!< SPI Periph clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	/*!< Configure SPI pins: SCK */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
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
}

void SPIClass::begin() {

	io_config();

	/*!< SPI Config */
	SPI_DeInit(SPI1);
	spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spi_init.SPI_Mode = SPI_Mode_Master;
	spi_init.SPI_DataSize = SPI_DataSize_8b;
	spi_init.SPI_NSS = SPI_NSS_Soft;
	spi_init.SPI_CRCPolynomial = 7;

	switch (spi_datamode) {
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

	if (spi_bitorder == MSBFIRST) {
		spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
	}
	else {
		spi_init.SPI_FirstBit = SPI_FirstBit_LSB;
	}

	switch (spi_clock) {
	case 125000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
		break;
	case 250000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
		break;
	case 500000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
		break;
	case 1000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
		break;
	case 2000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
		break;
	case 4000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
		break;
	case 8000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
		break;
	case 16000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
		break;
	default:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
		break;
	}


	SPI_Init(SPI1, &spi_init);
	SPI_Cmd(SPI1, ENABLE); /*!< SPI enable */
}

void SPIClass::end() {
	/* disable SPI module */
	SPI_Cmd(SPI1, DISABLE);
}

void SPIClass::beginTransaction(SPISettings settings) {
	spi_clock = settings.setting_clock;
	spi_bitorder = settings.setting_bitorder;
	spi_datamode = settings.setting_datamode;

	switch (spi_datamode) {
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

	if (spi_bitorder == MSBFIRST) {
		spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
	}
	else {
		spi_init.SPI_FirstBit = SPI_FirstBit_LSB;
	}

	switch (spi_clock) {
	case 125000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
		break;
	case 250000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
		break;
	case 500000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
		break;
	case 1000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
		break;
	case 2000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
		break;
	case 4000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
		break;
	case 8000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
		break;
	case 16000000:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
		break;
	default:
		spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
		break;
	}

	SPI_Init(SPI1, &spi_init);
}

void SPIClass::endTransaction(void) {

}

void SPIClass::setBitOrder(uint8_t ssPin, uint8_t bitOrder) {
	(void)ssPin;
	setBitOrder(bitOrder);
}

void SPIClass::setBitOrder(uint8_t bitOrder) {
	spi_bitorder = bitOrder;
	if (spi_bitorder == MSBFIRST) {
		spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
	}
	else {
		spi_init.SPI_FirstBit = SPI_FirstBit_LSB;
	}
	SPI_Init(SPI1, &spi_init);
}

void SPIClass::setDataMode(uint8_t mode) {
	/*
	SPI_MODE0	0	0	Falling	Rising
	SPI_MODE1	0	1	Rising	Falling
	SPI_MODE2	1	0	Rising	Falling
	SPI_MODE3	1	1	Falling	Rising
	*/

	spi_datamode = mode;

	switch (spi_datamode) {
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
	spi_init.SPI_BaudRatePrescaler = (uint16_t)divider;
	SPI_Init(SPI1, &spi_init);
}

uint8_t SPIClass::transfer(uint8_t data) {
	unsigned long rxtxData = data;

	if (spi_bitorder == LSBFIRST) {
		asm("rbit %0, %1" : "=r" (rxtxData) : "r" (rxtxData));	// reverse order of 32 bits
		asm("rev %0, %1" : "=r" (rxtxData) : "r" (rxtxData));	// reverse order of bytes to get original bits into lowest byte
	}

	/* waiting send idle then send data */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI1, (uint8_t)rxtxData);

	/* waiting conplete rev data */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	rxtxData = (uint8_t)SPI_I2S_ReceiveData(SPI1);

	if (spi_bitorder == LSBFIRST) {
		asm("rbit %0, %1" : "=r" (rxtxData) : "r" (rxtxData));	// reverse order of 32 bits
		asm("rev %0, %1" : "=r" (rxtxData) : "r" (rxtxData));	// reverse order of bytes to get original bits into lowest byte
	}

	return (uint8_t)rxtxData;
}

uint16_t SPIClass::transfer16(uint16_t data) {
	uint16_t ret = 0;
	if (spi_bitorder == LSBFIRST) {
		ret = transfer((uint8_t)(data&0xFF)) & 0xFF;
		ret = (transfer((uint8_t)(data>>8)) << 8) | ret;
	}
	else {
		ret = (transfer((uint8_t)(data>>8)) << 8);
		ret = (transfer((uint8_t)(data&0xFF)) & 0xFF) | ret;
	}
	return ret;
}

void SPIClass::transfer(void *buf, uint32_t count) {
	for (uint32_t i = 0; i<count; i++) {
		transfer(*((uint8_t*)buf+i));
	}
}

void SPIClass::setModule(uint8_t module) {
	(void)module;
	begin();
}

SPIClass SPI;
