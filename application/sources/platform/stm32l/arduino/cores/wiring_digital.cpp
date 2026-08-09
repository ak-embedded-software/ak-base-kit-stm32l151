#include "Arduino.h"
#include "io_cfg.h"
#include "sys_dbg.h"

void pinMode(uint8_t pin, uint8_t mode) {
	switch (pin) {
	case OLED_CLK_PIN:
		if (mode == INPUT) {
			oled_clk_input_mode();
		}
		else if (mode == OUTPUT) {
			oled_clk_output_mode();
		}
		else if (mode == INPUT_PULLUP) {
			oled_clk_input_mode();
		}
		else {
			FATAL("AR", 0x01);
		}
		break;

	case OLED_DATA_PIN:
		if (mode == INPUT) {
			oled_data_input_mode();
		}
		else if (mode == OUTPUT) {
			oled_data_output_mode();
		}
		else if (mode == INPUT_PULLUP) {
			oled_data_input_mode();
		}
		else {
			FATAL("AR", 0x01);
		}
		break;

	case OLED_RES_PIN:
		if (mode == INPUT) {
			oled_res_input_mode();
		}
		else if (mode == OUTPUT) {
			oled_res_output_mode();
		}
		else if (mode == INPUT_PULLUP) {
			oled_res_input_mode();
		}
		else {
			FATAL("AR", 0x01);
		}
		break;

	default:
		FATAL("AR", 0xF1);
		break;
	}
}

void digitalWrite(uint8_t pin, uint8_t val) {
	switch (pin) {
	case OLED_CLK_PIN:
		if (val == HIGH) {
			oled_clk_digital_write_high();
		}
		else if (val == LOW) {
			oled_clk_digital_write_low();
		}
		else {
			FATAL("AR", 0x02);
		}
		break;

	case OLED_DATA_PIN:
		if (val == HIGH) {
			oled_data_digital_write_high();
		}
		else if (val == LOW) {
			oled_data_digital_write_low();
		}
		else {
			FATAL("AR", 0x02);
		}
		break;

	case OLED_RES_PIN:
		if (val == HIGH) {
			oled_res_digital_write_high();
		}
		else if (val == LOW) {
			oled_res_digital_write_low();
		}
		else {
			FATAL("AR", 0x02);
		}
		break;

	default:
		FATAL("AR", 0xF2);
		break;
	}
}

int digitalRead(uint8_t pin) {
	int val = 0;
	switch (pin) {
	case OLED_CLK_PIN: {
		val = oled_clk_digital_read();
	}
		break;

	case OLED_DATA_PIN: {
		val = oled_data_digital_read();
	}
		break;

	case OLED_RES_PIN: {
		val = oled_res_digital_read();
	}
		break;
	default:
		FATAL("AR", 0xF3);
		break;
	}
	return val;
}
