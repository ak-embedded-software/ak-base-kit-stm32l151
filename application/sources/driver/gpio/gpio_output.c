/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   15/09/2016
 ******************************************************************************
**/
#include "gpio_output.h"
#include "port.h"

void gpio_output_init(gpio_output_t* gpio_output, pf_gpio_output_ctrl init, pf_gpio_output_ctrl on, pf_gpio_output_ctrl off) {
	gpio_output->counter        = (uint32_t)0;
	gpio_output->pf_ctrl        = init;
	gpio_output->pf_on          = on;
	gpio_output->pf_off         = off;

	/* init hardware for gpio_output */
	if (gpio_output->pf_ctrl) {
		gpio_output->pf_ctrl();
	}

	/* set default gpio_output is off */
	if (gpio_output->pf_off) {
		gpio_output->pf_off();
	}
}

void gpio_output_on(gpio_output_t* gpio_output) {
	gpio_output->status = GPIO_OUTPUT_ON;
	if (gpio_output->pf_on) {
		gpio_output->pf_on();
	}
}

void gpio_output_off(gpio_output_t* gpio_output) {
	gpio_output->status = GPIO_OUTPUT_OFF;
	if (gpio_output->pf_off) {
		gpio_output->pf_off();
	}
}

void gpio_output_toggle(gpio_output_t* gpio_output) {
	if (gpio_output->status == GPIO_OUTPUT_ON) {
		gpio_output_off(gpio_output);
	}
	else if (gpio_output->status == GPIO_OUTPUT_OFF) {
		gpio_output_on(gpio_output);
	}
}
