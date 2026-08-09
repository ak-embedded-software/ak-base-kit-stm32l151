#ifndef __TASK_ZIGBEE__
#define __TASK_ZIGBEE__

#include "message.h"

#if defined(TASK_ZIGBEE_EN)
#include "zb_zcl.h"
#include "zb_znp.h"

extern zb_znp zigbee_network;
#endif

#define ATTRID_XIAOMI_SENS_STATUS_REPORT	0xFF01

/*
65281 - 0xFF01 report:
{ '1': 3069,		= Battery
  '4': 5117,
  '5': 62,
  '6': 0,
  '10': 0,
  '100': 2045,	= Temperatemure
  '101': 3837 	= Humidity
}

GW_ZIGBEE_ZCL_CMD_HANDLER
[pOutgoingMsg] short_addr = 0xe86c
[pOutgoingMsg] cluster_id = 0x0000
[pOutgoingMsg] group_id = 0x0000
[pOutgoingMsg] cmd = 0x0a
[pOutgoingMsg] attrID = 0xff01
[pOutgoingMsg] dataType = 0x42
[pOutgoingMsg] dataLen = 28
[pOutgoingMsg] data: 1b 01 21 77 0b 04 21 a8 43 05 21 1a 0c 06 24 00 00 00 00 00 64 29 4a 0c 65 21 70 1b

1b -> Dum
01 -> Type Battery
	21 -> ZCL_DATATYPE_UINT16
	77 0b -> Value
04 -> Type Dum1
	21 -> ZCL_DATATYPE_UINT16
	a8 43 -> Value
05 -> Type Dum2
	21 -> ZCL_DATATYPE_UINT16
	1a 0c -> Value
06 -> Type Dum3
	24 -> ZCL_DATATYPE_UINT40
	00 00 00 00 00 -> Value
64 -> Temperatemure
	29 -> ZCL_DATATYPE_INT16
	4a 0c -> Value
65 -> Humidity
	21 -> ZCL_DATATYPE_UINT16
	70 1b -> Value
*/

typedef struct {
	uint8_t unkown;

	uint8_t battery;
	/**/ uint8_t battery_type;
	/**/ uint16_t battery_value;

	uint8_t dum_1;
	/**/ uint8_t dum_1_type;
	/**/ uint16_t dum_1_value;

	uint8_t dum_2;
	/**/ uint8_t dum_2_type;
	/**/ uint16_t dum_2_value;

	uint8_t dum_3;
	/**/ uint8_t dum_3_type;
	/**/ uint8_t dum_3_value[5];

	uint8_t temperatemure;
	/**/ uint8_t temperatemure_type;
	/**/ uint16_t temperatemure_value;

	uint8_t humidity;
	/**/ uint8_t humidity_type;
	/**/ uint16_t humidity_value;

} __AK_PACKETED xiaomi_sens_status_report_t;

#endif //__TASK_ZIGBEE__
