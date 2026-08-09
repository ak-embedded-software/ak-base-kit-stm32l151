/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   15/09/2016
 ******************************************************************************
**/
#include "led.h"
#include "sys_io.h"

void led_init(led_t* led, pf_led_ctrl init, pf_led_ctrl on, pf_led_ctrl off) {
	led->blink_enable   = LED_BLINK_DISABLE;
	led->counter        = (uint32_t)0;
	led->pf_ctrl        = init;
	led->pf_on          = on;
	led->pf_off         = off;

	/* init hardware for led */
	if (led->pf_ctrl) {
		led->pf_ctrl();
	}

	/* set default led is off */
	if (led->pf_off) {
		led->pf_off();
	}
}

void led_on(led_t* led) {
	led->status = LED_STATUS_ON;
	if (led->pf_on) {
		led->pf_on();
	}
}

void led_off(led_t* led) {
	led->status = LED_STATUS_OFF;
	if (led->pf_off) {
		led->pf_off();
	}
}

void led_toggle(led_t* led) {
	if (led->status == LED_STATUS_ON) {
		led_off(led);
	}
	else if (led->status == LED_STATUS_OFF) {
		led_on(led);
	}
}

void led_blink_set(led_t* led, uint32_t freq, uint32_t duty) {
	led->freq = freq;
	led->duty = duty;

	ENTRY_CRITICAL();
	led->blink_enable = LED_BLINK_ENABLE;
	EXIT_CRITICAL();
}

void led_blink_reset(led_t* led) {
	ENTRY_CRITICAL();
	led->blink_enable = LED_BLINK_DISABLE;
	EXIT_CRITICAL();
}

void led_blink_polling(led_t* led) {
	if (led->blink_enable == LED_BLINK_ENABLE) {

		led->counter++;

		if (led->counter <= led->duty) {
			if (led->status != LED_STATUS_ON) {
				led_on(led);
			}
		}
		else if (led->counter < led->freq) {
			if (led->status != LED_STATUS_OFF) {
				led_off(led);
			}
		}
		else if (led->counter == led->freq) {
			led->counter = 0;
		}
	}
}
