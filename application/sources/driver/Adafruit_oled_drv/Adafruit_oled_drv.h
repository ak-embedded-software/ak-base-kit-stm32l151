#ifndef __ADAFRUIT_OLED_DRV_H
#define __ADAFRUIT_OLED_DRV_H

#include <stdlib.h>
#include "Adafruit_GFX.h"

#if defined (SH1106_DRIVER_EN)
#define OLED_COL_OFFSET 2
#else
#define OLED_COL_OFFSET 0
#endif

enum SH1106_CMD_SET {
	SH1106_SET_LOW_COLUMN						= 0x00,
	SH1106_EXTERNAL_VCC							= 0x01,
	SH1106_SWITCHCAP_VCC						= 0x02,
	SH1106_SET_HIGH_COLUMN						= 0x10,
	SH1106_MEMORY_MODE							= 0x20,
	SH1106_COLUMN_ADDR							= 0x21,
	SH1106_PAGE_ADDR							= 0x22,
	SH1106_RIGHT_HORIZONTAL_SCROLL				= 0x26,
	SH1106_LEFT_HORIZONTAL_SCROLL				= 0x27,
	SH1106_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL = 0x29,
	SH1106_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL	= 0x2A,
	SH1106_DEACTIVATE_SCROLL					= 0x2E,
	SH1106_ACTIVATE_SCROLL						= 0x2F,
	SH1106_SET_START_LINE						= 0x40,
	SH1106_SET_CONTRAST							= 0x81,
	SH1106_CHARGE_PUMP							= 0x8D,
	SH1106_SEGREMAP								= 0xA0,
	SH1106_SET_SEGMENT_REMAP					= 0xA1,
	SH1106_SET_VERTICAL_SCROLL_AREA				= 0xA3,
	SH1106_DISPLAY_ALL_ON_RESUME				= 0xA4,
	SH1106_OUTPUT_FOLLOWS_RAM					= 0xA4,
	SH1106_DISPLAY_ALL_ON						= 0xA5,
	SH1106_NORMAL_DISPLAY						= 0xA6,
	SH1106_INVERT_DISPLAY						= 0xA7,
	SH1106_SET_MULTIPLEX						= 0xA8,
	SH1106_DISPLAY_OFF							= 0xAE,
	SH1106_DISPLAY_ON							= 0xAF,
	SH1106_SET_PAGE_ADDRESS						= 0xB0,
	SH1106_COM_SCAN_INC							= 0xC0,
	SH1106_COM_SCAN_DEC							= 0xC8,
	SH1106_SET_DISPLAY_OFFSET					= 0xD3,
	SH1106_SET_DISPLAY_CLOCK_DIV				= 0xD5,
	SH1106_SET_PRECHARGE						= 0xD9,
	SH1106_SET_COM_PINS							= 0xDA,
	SH1106_SET_VCOM_DETECT						= 0xDB,
};

enum SSD1306_CMD_SET {
	SSD1306_SET_LOW_COLUMN						 = 0x00,
	SSD1306_EXTERNAL_VCC						 = 0x01,
	SSD1306_SWITCHCAP_VCC						 = 0x02,
	SSD1306_SET_HIGH_COLUMN						 = 0x10,
	SSD1306_MEMORY_MODE							 = 0x20,
	SSD1306_COLUMN_ADDR							 = 0x21,
	SSD1306_PAGE_ADDR							 = 0x22,
	SSD1306_RIGHT_HORIZONTAL_SCROLL				 = 0x26,
	SSD1306_LEFT_HORIZONTAL_SCROLL				 = 0x27,
	SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL = 0x29,
	SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL	 = 0x2A,
	SSD1306_DEACTIVATE_SCROLL					 = 0x2E,
	SSD1306_ACTIVATE_SCROLL						 = 0x2F,
	SSD1306_SET_START_LINE						 = 0x40,
	SSD1306_SET_CONTRAST						 = 0x81,
	SSD1306_CHARGE_PUMP							 = 0x8D,
	SSD1306_SEGREMAP							 = 0xA0,
	SSD1306_SET_VERTICAL_SCROLL_AREA			 = 0xA3,
	SSD1306_DISPLAY_ALL_ON_RESUME				 = 0xA4,
	SSD1306_DISPLAY_ALL_ON						 = 0xA5,
	SSD1306_NORMAL_DISPLAY						 = 0xA6,
	SSD1306_INVERT_DISPLAY						 = 0xA7,
	SSD1306_SET_MULTIPLEX						 = 0xA8,
	SSD1306_DISPLAY_OFF							 = 0xAE,
	SSD1306_DISPLAY_ON							 = 0xAF,
	SSD1306_COM_SCAN_INC						 = 0xC0,
	SSD1306_COM_SCAN_DEC						 = 0xC8,
	SSD1306_SET_DISPLAY_OFFSET					 = 0xD3,
	SSD1306_SET_DISPLAY_CLOCK_DIV				 = 0xD5,
	SSD1306_SET_PRECHARGE						 = 0xD9,
	SSD1306_SET_COM_PINS						 = 0xDA,
	SSD1306_SET_VCOM_DETECT						 = 0xDB,
};

enum SSD1309_CMD_SET {
	SSD1309_MEMORY_MODE							 = 0x20,
	SSD1309_COLUMN_ADDR							 = 0x21,
	SSD1309_PAGE_ADDR							 = 0x22,
	SSD1309_RIGHT_HORIZONTAL_SCROLL				 = 0x26,
	SSD1309_LEFT_HORIZONTAL_SCROLL				 = 0x27,
	SSD1309_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL = 0x29,
	SSD1309_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL	 = 0x2A,
	SSD1309_DEACTIVATE_SCROLL					 = 0x2E,
	SSD1309_ACTIVATE_SCROLL						 = 0x2F,
	SSD1309_SET_DISPLAY_START_LINE_BASE			 = 0x40,
	SSD1309_SET_CONTRAST						 = 0x81,
	SSD1309_CHARGE_PUMP							 = 0x8D,
	SSD1309_SEG_REMAP_NORMAL					 = 0xA0,
	SSD1309_SEG_REMAP_FLIP						 = 0xA1,
	SSD1309_SET_VERTICAL_SCROLL_AREA			 = 0xA3,
	SSD1309_DISPLAY_ALL_ON_RESUME				 = 0xA4,
	SSD1309_DISPLAY_ALL_ON_IGNORE				 = 0xA5,
	SSD1309_NORMAL_DISPLAY						 = 0xA6,
	SSD1309_INVERT_DISPLAY						 = 0xA7,
	SSD1309_SET_MULTIPLEX						 = 0xA8,
	SSD1309_DISPLAY_OFF							 = 0xAE,
	SSD1309_DISPLAY_ON							 = 0xAF,
	SSD1309_COM_SCAN_INC						 = 0xC0,
	SSD1309_COM_SCAN_DEC						 = 0xC8,
	SSD1309_SET_DISPLAY_OFFSET					 = 0xD3,
	SSD1309_SET_DISPLAY_CLOCK_DIV				 = 0xD5,
	SSD1309_SET_PRECHARGE						 = 0xD9,
	SSD1309_SET_COM_PINS						 = 0xDA,
	SSD1309_SET_VCOM_DETECT						 = 0xDB,
};

#if defined (STM32L_PLATFORM)
#include "Arduino.h"
#include "io_cfg.h"
#endif

#define OLED_I2C_CLK    OLED_CLK_PIN	
#define OLED_I2C_DATA   OLED_DATA_PIN
#define OLED_I2C_RES    OLED_RES_PIN

#define BLACK 0
#define WHITE 1

//common parameters
#define WIDTH 128
#define HEIGHT 64
#define FBSIZE 1024 //128x8
#define MAXROW 8

class Adafruit_oled_drv : public Adafruit_GFX{
public:
	Adafruit_oled_drv();
	~Adafruit_oled_drv();
	//initialized the ssd1306 in the setup function
	virtual bool initialize();

	//update the framebuffer to the screen.
	virtual void update();
	//totoally 8 rows on this screen in vertical direction.
	virtual void updateRow(int rowIndex);
	virtual void updateRow(int startRow, int endRow);
	
	//draw one pixel on the screen.
	virtual void drawPixel(int16_t x, int16_t y, uint16_t color);

	//clear the screen
	void clear(bool isUpdateHW=false);

	//on screen
	void display_on();

	//off screen
	void display_off();

	const unsigned char* getFrameBuffer() const;
	unsigned int getFrameBufferSize() const;

protected:
	//write one byte to the screen.
	void writeByte(unsigned char  b);
	void writeCommand(unsigned char  cmd);

	//atomic control function
	void startIIC();//turn on the IIC
	void stopIIC();//turn off the IIC.
	void startDataSequence();

protected:
	int m_sda;
	int m_scl;
	int m_res;
	unsigned char* m_pFramebuffer;
};

#endif //__ADAFRUIT_OLED_DRV_H
