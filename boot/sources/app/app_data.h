#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>
#include "app.h"

/******************************************************************************
* Commom data structure for transceiver data
*******************************************************************************/
#define FIRMWARE_PSK		0x1A2B3C4D

typedef struct {
	uint8_t is_power_on_reset;
} boot_app_share_data_t;

extern boot_app_share_data_t boot_app_share_data;

#endif //__APP_DATA_H__
