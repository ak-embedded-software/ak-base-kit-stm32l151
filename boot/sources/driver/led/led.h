/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   15/09/2016
 ******************************************************************************
**/
#ifndef __LED_H__
#define __LED_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define LED_STATUS_OFF          (0x00)
#define LED_STATUS_ON           (0x01)

#define LED_BLINK_DISABLE       (0x00)
#define LED_BLINK_ENABLE        (0x01)

typedef void (*pf_led_ctrl)();

typedef struct {
	/* led attribute */
	uint8_t     blink_enable;
	uint8_t     status;
	uint32_t    duty;
	uint32_t    freq;
	uint32_t    counter;

	/* led control function */
	pf_led_ctrl pf_ctrl;
	pf_led_ctrl pf_on;
	pf_led_ctrl pf_off;
} led_t;

void led_init(led_t* led, pf_led_ctrl init, pf_led_ctrl on, pf_led_ctrl off);
void led_on(led_t* led);
void led_off(led_t* led);
void led_toggle(led_t* led);
void led_blink_set(led_t* led, uint32_t freq, uint32_t duty);
void led_blink_reset(led_t* led);
void led_blink_polling(led_t* led); /* this function must be called periodically 10ms */

#ifdef __cplusplus
}
#endif

#endif //__LED_H__
