#include "Adafruit_oled_drv.h"

static unsigned char frame_buffer[FBSIZE];

Adafruit_oled_drv::Adafruit_oled_drv() : Adafruit_GFX(WIDTH, HEIGHT) {
	m_sda		   = OLED_I2C_DATA;
	m_scl		   = OLED_I2C_CLK;
	m_res		   = OLED_I2C_RES;
	m_pFramebuffer = 0;
}
Adafruit_oled_drv::~Adafruit_oled_drv() {}

bool Adafruit_oled_drv::initialize() {
	// setup the pin mode
	pinMode(m_sda, OUTPUT);
	pinMode(m_scl, OUTPUT);
	pinMode(m_res, OUTPUT);

	digitalWrite(m_res, LOW);
	delay(100); // 100ms delay for the screen to power on.
	digitalWrite(m_res, HIGH);
	
	// malloc the framebuffer.
	m_pFramebuffer = frame_buffer;
	if (m_pFramebuffer == 0) {
		return false;
	}
	memset(m_pFramebuffer, 0, FBSIZE);	  // clear it.

	// write command to the screen registers.
#if defined(SH1106_DRIVER_EN)
	writeCommand(SH1106_DISPLAY_OFF);			// 0xAE
	writeCommand(SH1106_SET_DISPLAY_CLOCK_DIV);	// 0xD5
	writeCommand(0x80);							// the suggested ratio 0x80
	writeCommand(SH1106_SET_MULTIPLEX);			// 0xA8
	writeCommand(0x3F);
	writeCommand(SH1106_SET_DISPLAY_OFFSET);	  // 0xD3
	writeCommand(0x00);						  // no offset

	writeCommand(SH1106_SET_START_LINE | 0x0);	// line #0 0x40
	writeCommand(SH1106_CHARGE_PUMP);			// 0x8D
	writeCommand(0x14);
	writeCommand(SH1106_MEMORY_MODE);	// 0x20
	writeCommand(0x00);					// 0x0 act like ks0108
	writeCommand(SH1106_SEGREMAP | 0x1);
	writeCommand(SH1106_COM_SCAN_DEC);
	writeCommand(SH1106_SET_COM_PINS);	// 0xDA
	writeCommand(0x12);
	writeCommand(SH1106_SET_CONTRAST);	 // 0x81
	writeCommand(0xCF);
	writeCommand(SH1106_SET_PRECHARGE);	  // 0xd9
	writeCommand(0xF1);
	writeCommand(SH1106_SET_VCOM_DETECT);	   // 0xDB
	writeCommand(0x40);
	writeCommand(SH1106_DISPLAY_ALL_ON_RESUME);	 // 0xA4
	writeCommand(SH1106_NORMAL_DISPLAY);			 // 0xA6
	writeCommand(SH1106_DISPLAY_ON);				 //--turn on oled panel

	delay(10);	  // wait for the screen loaded.

#elif defined(SSD1306_DRIVER_EN)
	writeCommand(SSD1306_DISPLAY_OFF);		 // display off
	writeCommand(SSD1306_MEMORY_MODE);		 // Set Memory Addressing Mode
	writeCommand(0x10);						 // 00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	writeCommand(SSD1306_SET_START_LINE);		 // Set Page Start Address for Page Addressing Mode,0-7
	writeCommand(0xB0);						 // Set COM Output Scan Direction
	writeCommand(SSD1306_SET_CONTRAST);		 //---set low column address
	writeCommand(0xCF);						 //---set high column address
	writeCommand(SSD1306_SEGREMAP | 0x1);	 //--set start line address
	writeCommand(SSD1306_NORMAL_DISPLAY);	 //--set contrast control register
	writeCommand(SSD1306_SET_MULTIPLEX);
	writeCommand(0x3F);							 //--set segment re-map 0 to 127
	writeCommand(SSD1306_COM_SCAN_DEC);			 //--set normal display
	writeCommand(SSD1306_SET_DISPLAY_OFFSET);		 //--set multiplex ratio(1 to 64)
	writeCommand(0x00);							 //
	writeCommand(SSD1306_SET_DISPLAY_CLOCK_DIV);	 // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	writeCommand(0x80);							 //-set display offset
	writeCommand(SSD1306_SET_PRECHARGE);			 //-not offset
	writeCommand(0xF1);							 //--set display clock divide ratio/oscillator frequency
	writeCommand(SSD1306_SET_COM_PINS);			 //--set divide ratio
	writeCommand(0x12);							 //--set pre-charge period
	writeCommand(SSD1306_SET_VCOM_DETECT);		 //
	writeCommand(0x40);							 //--set com pins hardware configuration
	writeCommand(SSD1306_CHARGE_PUMP);			 //--set vcomh
	writeCommand(0x14);							 // 0x20,0.77xVcc
	writeCommand(SSD1306_DISPLAY_ON);	//--turn on oled panel

	delay(10);	  // wait for the screen loaded.

#elif defined(SSD1309_DRIVER_EN)
	writeCommand(SSD1309_DISPLAY_OFF);
	writeCommand(SSD1309_MEMORY_MODE);
	writeCommand(0x02);	   // page addressing mode
	writeCommand(SSD1309_SET_DISPLAY_START_LINE_BASE);
	writeCommand(SSD1309_SEG_REMAP_FLIP);
	writeCommand(SSD1309_COM_SCAN_DEC);
	writeCommand(SSD1309_SET_MULTIPLEX);
	writeCommand(0x3F);
	writeCommand(SSD1309_SET_DISPLAY_OFFSET);
	writeCommand(0x00);
	writeCommand(SSD1309_SET_DISPLAY_CLOCK_DIV);
	writeCommand(0x80);
	writeCommand(SSD1309_SET_PRECHARGE);
	writeCommand(0xF1);
	writeCommand(SSD1309_SET_COM_PINS);
	writeCommand(0x12);
	writeCommand(SSD1309_SET_VCOM_DETECT);
	writeCommand(0x40);
	writeCommand(SSD1309_SET_CONTRAST);
	writeCommand(0xCF);
	writeCommand(SSD1309_CHARGE_PUMP);
	writeCommand(0x14);
	writeCommand(SSD1309_DISPLAY_ALL_ON_RESUME);
	writeCommand(SSD1309_NORMAL_DISPLAY);
	writeCommand(SSD1309_DISPLAY_ON);

	delay(10);	  // wait for the screen loaded.
#else
#error "Don't know oled driver type."
#endif

	return true;
}

void Adafruit_oled_drv::clear(bool isUpdateHW) {
	memset(m_pFramebuffer, 0, FBSIZE);	  // clear the back buffer.
	if (isUpdateHW)
		update();	 // update the hw immediately
}

void Adafruit_oled_drv::display_on() {
#if defined(SH1106_DRIVER_EN)
	writeCommand(SH1106_DISPLAY_ON);
#elif defined(SSD1306_DRIVER_EN)
	writeCommand(SSD1306_DISPLAY_ON);
#elif defined(SSD1309_DRIVER_EN)
	writeCommand(SSD1309_DISPLAY_ON);
#else
#error "Don't know oled driver type."
#endif
}

void Adafruit_oled_drv::display_off() {
#if defined(SH1106_DRIVER_EN)
	writeCommand(SH1106_DISPLAY_OFF);
#elif defined(SSD1306_DRIVER_EN)
	writeCommand(SSD1306_DISPLAY_OFF);
#elif defined(SSD1309_DRIVER_EN)
	writeCommand(SSD1309_DISPLAY_OFF);
#else
#error "Don't know oled driver type."
#endif
}

const unsigned char* Adafruit_oled_drv::getFrameBuffer() const {
	return m_pFramebuffer;
}

unsigned int Adafruit_oled_drv::getFrameBufferSize() const {
	return FBSIZE;
}

void Adafruit_oled_drv::writeCommand(unsigned char cmd) {
	startIIC();
	writeByte(0x78);	// Slave address,SA0=0
	writeByte(0x00);	// write command
	writeByte(cmd);
	stopIIC();
}

void Adafruit_oled_drv::writeByte(unsigned char b) {
	unsigned char i;
	for (i = 0; i < 8; i++) {
		if ((b << i) & 0x80) {
			digitalWrite(m_sda, HIGH);
		}
		else {
			digitalWrite(m_sda, LOW);
		}
		digitalWrite(m_scl, HIGH);
		digitalWrite(m_scl, LOW);
	}

#if defined(SH1106_DRIVER_EN)
	digitalWrite(m_sda, LOW);
#elif defined(SSD1309_DRIVER_EN) || defined(SSD1306_DRIVER_EN)
	digitalWrite(m_sda, HIGH);
#else
#error "Don't know oled driver type."
#endif

	digitalWrite(m_scl, HIGH);
	digitalWrite(m_scl, LOW);
}

void Adafruit_oled_drv::startIIC() {
	digitalWrite(m_scl, HIGH);
	digitalWrite(m_sda, HIGH);
	digitalWrite(m_sda, LOW);
	digitalWrite(m_scl, LOW);
}
void Adafruit_oled_drv::stopIIC() {
	digitalWrite(m_scl, LOW);
	digitalWrite(m_sda, LOW);
	digitalWrite(m_scl, HIGH);
	digitalWrite(m_sda, HIGH);
}

void Adafruit_oled_drv::drawPixel(int16_t x, int16_t y, uint16_t color) {
	unsigned char row;
	unsigned char offset;
	unsigned char preData;	  // previous data.
	unsigned char val;
	int16_t index;

	if ((x < 0) || (x >= width()) || (y < 0) || (y >= height()) || (m_pFramebuffer == 0))
		return;

	// get the previous data;
	row		= y / 8;
	offset	= y % 8;
	index	= row * width() + x;
	preData = m_pFramebuffer[index];

	// set pixel;
	val = 1 << offset;
	if (color != 0) {	 // white! set bit.
		m_pFramebuffer[index] = preData | val;
	}
	else {	  // black! clear bit.
		m_pFramebuffer[index] = preData & (~val);
	}
}

void Adafruit_oled_drv::startDataSequence() {
	startIIC();
	writeByte(0x78);
	writeByte(0x40);	// write data
}

void Adafruit_oled_drv::update() {
#if defined(SH1106_DRIVER_EN)
	const byte pageCount = HEIGHT >> 3;
	const byte chunkCount = 8;
	const byte chunkWidth = 132 >> 3;
	const byte rowOffset = 0;
	const byte colOffset = OLED_COL_OFFSET;
	int p = 0;

	writeCommand(SH1106_SET_LOW_COLUMN | (colOffset & 0x0F));
	writeCommand(SH1106_SET_HIGH_COLUMN | (colOffset >> 4));
	writeCommand(SH1106_SET_START_LINE | 0x0);

	for (byte page = 0; page < pageCount; page++) {
		writeCommand(SH1106_SET_PAGE_ADDRESS + page + rowOffset);
		writeCommand(colOffset & 0x0F);
		writeCommand(0x10 | (colOffset >> 4));

		for (byte chunk = 0; chunk < chunkCount; chunk++) {
			startDataSequence();
			for (byte col = 0; col < chunkWidth; col++, p++) {
				writeByte(m_pFramebuffer[p]);
			}
			stopIIC();
		}
	}

#elif defined(SSD1306_DRIVER_EN)
	unsigned int fbIndex = 0;
	for (unsigned int page = 0; page < MAXROW; page++) {
		writeCommand(0xB0 + page);
		writeCommand(SSD1306_SET_LOW_COLUMN);
		writeCommand(SSD1306_SET_HIGH_COLUMN);

		startDataSequence();
		for (unsigned int col = 0; col < WIDTH; col++) {
			writeByte(m_pFramebuffer[fbIndex++]);
		}
		stopIIC();
	}

#elif defined(SSD1309_DRIVER_EN)
	unsigned int fbIndex = 0;
	for (unsigned int page = 0; page < MAXROW; page++) {
		writeCommand(0xB0 + page);
		writeCommand(0x00);
		writeCommand(0x10);

		startDataSequence();
		for (unsigned int col = 0; col < WIDTH; col++) {
			writeByte(m_pFramebuffer[fbIndex++]);
		}
		stopIIC();
	}
#else
#error "Don't know oled driver type."
#endif
}

void Adafruit_oled_drv::updateRow(int rowID) {
	if (rowID >= 0 && rowID < MAXROW && m_pFramebuffer) {	 // this part is faster than else.
		unsigned int index = 0;
		unsigned char x = 0;

#if defined(SH1106_DRIVER_EN)
		const unsigned char colOffset = OLED_COL_OFFSET;
		const unsigned char lowerCol = colOffset & 0x0F;
		const unsigned char higherCol = 0x10 | (colOffset >> 4);
#elif defined(SSD1306_DRIVER_EN)
		const unsigned char lowerCol = SSD1306_SET_LOW_COLUMN;
		const unsigned char higherCol = SSD1306_SET_HIGH_COLUMN;
#elif defined(SSD1309_DRIVER_EN)
		const unsigned char lowerCol = 0x00;
		const unsigned char higherCol = 0x10;
#else
#error "Don't know oled driver type."
#endif

		// set the positio
		startIIC();
		writeByte(0x78);	// Slave address,SA0=0
		writeByte(0x00);	// write command

		writeByte(0xb0 + rowID);
		writeByte(higherCol);
		writeByte(lowerCol);

		stopIIC();

		// start painting the buffer.
		startDataSequence();
		for (x = 0; x < WIDTH; x++) {
			index = rowID * WIDTH + x;
			writeByte(m_pFramebuffer[index]);
		}
		stopIIC();
	}
}

void Adafruit_oled_drv::updateRow(int startID, int endID) {
	unsigned char y = 0;
	for (y = startID; y < endID; y++) {
		updateRow(y);
	}
}
