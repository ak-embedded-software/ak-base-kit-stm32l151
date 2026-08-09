/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   15/09/2016
 ******************************************************************************
**/
#ifndef __GPIO_OUTPUT_H__
#define __GPIO_OUTPUT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define GPIO_OUTPUT_OFF          (0x00)
#define GPIO_OUTPUT_ON           (0x01)

#define GPIO_OUTPUT_TOGGLE_DISABLE       (0x00)
#define GPIO_OUTPUT_TOGGLE_ENABLE        (0x01)

typedef void (*pf_gpio_output_ctrl)();

typedef struct {
	/* gpio_output attribute */
	uint8_t     toggle_enable;
	uint8_t     status;
	uint32_t    duty;
	uint32_t    freq;
	uint32_t    counter;

	/* gpio_output control function */
	pf_gpio_output_ctrl pf_ctrl;
	pf_gpio_output_ctrl pf_on;
	pf_gpio_output_ctrl pf_off;
} gpio_output_t;

void gpio_output_init(gpio_output_t* gpio_output, pf_gpio_output_ctrl init, pf_gpio_output_ctrl on, pf_gpio_output_ctrl off);
void gpio_output_on(gpio_output_t* gpio_output);
void gpio_output_off(gpio_output_t* gpio_output);
void gpio_output_toggle(gpio_output_t* gpio_output);

#ifdef __cplusplus
}
#endif

#endif //__GPIO_OUTPUT_H__
