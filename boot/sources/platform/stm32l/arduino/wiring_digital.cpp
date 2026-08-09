#include "Arduino.h"
#include "io_cfg.h"
#include "sys_dbg.h"

void pinMode(uint8_t pin, uint8_t mode) {
	switch (pin) {
	case SSD1306_CLK_PIN:
		if (mode == INPUT) {
			ssd1306_clk_input_mode();
		}
		else if (mode == OUTPUT) {
			ssd1306_clk_output_mode();
		}
		else if (mode == INPUT_PULLUP) {
			ssd1306_clk_input_mode();
		}
		else {
			//FATAL("AR", 0x01);
		}
		break;

	case SSD1306_DATA_PIN:
		if (mode == INPUT) {
			ssd1306_data_input_mode();
		}
		else if (mode == OUTPUT) {
			ssd1306_data_output_mode();
		}
		else if (mode == INPUT_PULLUP) {
			ssd1306_clk_input_mode();
		}
		else {
			//FATAL("AR", 0x01);
		}
		break;

	default:
		//FATAL("AR", 0xF1);
		break;
	}
}

void digitalWrite(uint8_t pin, uint8_t val) {
	switch (pin) {
	case SSD1306_CLK_PIN:
		if (val == HIGH) {
			ssd1306_clk_digital_write_high();
		}
		else if (val == LOW) {
			ssd1306_clk_digital_write_low();
		}
		else {
			//FATAL("AR", 0x02);
		}
		break;

	case SSD1306_DATA_PIN:
		if (val == HIGH) {
			ssd1306_data_digital_write_high();
		}
		else if (val == LOW) {
			ssd1306_data_digital_write_low();
		}
		else {
			//FATAL("AR", 0x02);
		}
		break;

	default:
		//FATAL("AR", 0xF2);
		break;
	}
}

int digitalRead(uint8_t pin) {
	int val = 0;
	switch (pin) {
	case SSD1306_DATA_PIN:
		val = ssd1306_data_digital_read();
		break;

	default:
		//FATAL("AR", 0xF3);
		break;
	}
	return val;
}
