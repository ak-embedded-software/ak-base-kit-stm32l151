#ifndef __ZB_ZCL_H__
#define __ZB_ZCL_H__

#include <stdint.h>

// General Clusters
#define ZCL_CLUSTER_ID_GEN_BASIC                             0x0000
#define ZCL_CLUSTER_ID_GEN_POWER_CFG                         0x0001
#define ZCL_CLUSTER_ID_GEN_DEVICE_TEMP_CONFIG                0x0002
#define ZCL_CLUSTER_ID_GEN_IDENTIFY                          0x0003
#define ZCL_CLUSTER_ID_GEN_GROUPS                            0x0004
#define ZCL_CLUSTER_ID_GEN_SCENES                            0x0005
#define ZCL_CLUSTER_ID_GEN_ON_OFF                            0x0006
#define ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG              0x0007
#define ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL                     0x0008
#define ZCL_CLUSTER_ID_GEN_ALARMS                            0x0009
#define ZCL_CLUSTER_ID_GEN_TIME                              0x000A
#define ZCL_CLUSTER_ID_GEN_LOCATION                          0x000B
#define ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC                0x000C
#define ZCL_CLUSTER_ID_GEN_ANALOG_OUTPUT_BASIC               0x000D
#define ZCL_CLUSTER_ID_GEN_ANALOG_VALUE_BASIC                0x000E
#define ZCL_CLUSTER_ID_GEN_BINARY_INPUT_BASIC                0x000F
#define ZCL_CLUSTER_ID_GEN_BINARY_OUTPUT_BASIC               0x0010
#define ZCL_CLUSTER_ID_GEN_BINARY_VALUE_BASIC                0x0011
#define ZCL_CLUSTER_ID_GEN_MULTISTATE_INPUT_BASIC            0x0012
#define ZCL_CLUSTER_ID_GEN_MULTISTATE_OUTPUT_BASIC           0x0013
#define ZCL_CLUSTER_ID_GEN_MULTISTATE_VALUE_BASIC            0x0014
#define ZCL_CLUSTER_ID_GEN_COMMISSIONING                     0x0015
#define ZCL_CLUSTER_ID_GEN_PARTITION                         0x0016

#define ZCL_CLUSTER_ID_OTA                                   0x0019

#define ZCL_CLUSTER_ID_GEN_POWER_PROFILE                     0x001A
#define ZCL_CLUSTER_ID_GEN_APPLIANCE_CONTROL                 0x001B

#define ZCL_CLUSTER_ID_GEN_POLL_CONTROL                      0x0020

#define ZCL_CLUSTER_ID_GREEN_POWER_PROXY                     0x0021

// Closures Clusters
#define ZCL_CLUSTER_ID_CLOSURES_SHADE_CONFIG                 0x0100
#define ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK                    0x0101
#define ZCL_CLUSTER_ID_CLOSURES_WINDOW_COVERING              0x0102

// HVAC Clusters
#define ZCL_CLUSTER_ID_HVAC_PUMP_CONFIG_CONTROL              0x0200
#define ZCL_CLUSTER_ID_HVAC_THERMOSTAT                       0x0201
#define ZCL_CLUSTER_ID_HVAC_FAN_CONTROL                      0x0202
#define ZCL_CLUSTER_ID_HVAC_DIHUMIDIFICATION_CONTROL         0x0203
#define ZCL_CLUSTER_ID_HVAC_USER_INTERFACE_CONFIG            0x0204

// Lighting Clusters
#define ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL                0x0300
#define ZCL_CLUSTER_ID_LIGHTING_BALLAST_CONFIG               0x0301

// Measurement and Sensing Clusters
#define ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT            0x0400
#define ZCL_CLUSTER_ID_MS_ILLUMINANCE_LEVEL_SENSING_CONFIG   0x0401
#define ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT            0x0402
#define ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT               0x0403
#define ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT                   0x0404
#define ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY                  0x0405
#define ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING                  0x0406

// Security and Safety (SS) Clusters
#define ZCL_CLUSTER_ID_SS_IAS_ZONE                           0x0500
#define ZCL_CLUSTER_ID_SS_IAS_ACE                            0x0501
#define ZCL_CLUSTER_ID_SS_IAS_WD                             0x0502

// Protocol Interfaces
#define ZCL_CLUSTER_ID_PI_GENERIC_TUNNEL                     0x0600
#define ZCL_CLUSTER_ID_PI_BACNET_PROTOCOL_TUNNEL             0x0601
#define ZCL_CLUSTER_ID_PI_ANALOG_INPUT_BACNET_REG            0x0602
#define ZCL_CLUSTER_ID_PI_ANALOG_INPUT_BACNET_EXT            0x0603
#define ZCL_CLUSTER_ID_PI_ANALOG_OUTPUT_BACNET_REG           0x0604
#define ZCL_CLUSTER_ID_PI_ANALOG_OUTPUT_BACNET_EXT           0x0605
#define ZCL_CLUSTER_ID_PI_ANALOG_VALUE_BACNET_REG            0x0606
#define ZCL_CLUSTER_ID_PI_ANALOG_VALUE_BACNET_EXT            0x0607
#define ZCL_CLUSTER_ID_PI_BINARY_INPUT_BACNET_REG            0x0608
#define ZCL_CLUSTER_ID_PI_BINARY_INPUT_BACNET_EXT            0x0609
#define ZCL_CLUSTER_ID_PI_BINARY_OUTPUT_BACNET_REG           0x060A
#define ZCL_CLUSTER_ID_PI_BINARY_OUTPUT_BACNET_EXT           0x060B
#define ZCL_CLUSTER_ID_PI_BINARY_VALUE_BACNET_REG            0x060C
#define ZCL_CLUSTER_ID_PI_BINARY_VALUE_BACNET_EXT            0x060D
#define ZCL_CLUSTER_ID_PI_MULTISTATE_INPUT_BACNET_REG        0x060E
#define ZCL_CLUSTER_ID_PI_MULTISTATE_INPUT_BACNET_EXT        0x060F
#define ZCL_CLUSTER_ID_PI_MULTISTATE_OUTPUT_BACNET_REG       0x0610
#define ZCL_CLUSTER_ID_PI_MULTISTATE_OUTPUT_BACNET_EXT       0x0611
#define ZCL_CLUSTER_ID_PI_MULTISTATE_VALUE_BACNET_REG        0x0612
#define ZCL_CLUSTER_ID_PI_MULTISTATE_VALUE_BACNET_EXT        0x0613
#define ZCL_CLUSTER_ID_PI_11073_PROTOCOL_TUNNEL              0x0614

// Advanced Metering Initiative (SE) Clusters
#define ZCL_CLUSTER_ID_SE_PRICING                            0x0700
#define ZCL_CLUSTER_ID_SE_LOAD_CONTROL                       0x0701
#define ZCL_CLUSTER_ID_SE_SIMPLE_METERING                    0x0702
#define ZCL_CLUSTER_ID_SE_MESSAGE                            0x0703
#define ZCL_CLUSTER_ID_SE_SE_TUNNELING                       0x0704
#define ZCL_CLUSTER_ID_SE_PREPAYMENT                         0x0705
#define ZCL_CLUSTER_ID_SE_ENERGY_MGMT                        0x0706
#define ZCL_CLUSTER_ID_SE_TOU_CALENDAR                       0x0707
#define ZCL_CLUSTER_ID_SE_DEVICE_MGMT                        0x0708
#define ZCL_CLUSTER_ID_SE_EVENTS                             0x0709
#define ZCL_CLUSTER_ID_SE_MDU_PAIRING                        0x070A

#define ZCL_CLUSTER_ID_GEN_KEY_ESTABLISHMENT                 0x0800

#define ZCL_CLUSTER_ID_HA_APPLIANCE_IDENTIFICATION           0x0B00
#define ZCL_CLUSTER_ID_HA_METER_IDENTIFICATION               0x0B01
#define ZCL_CLUSTER_ID_HA_APPLIANCE_EVENTS_ALERTS            0x0B02
#define ZCL_CLUSTER_ID_HA_APPLIANCE_STATISTICS               0x0B03
#define ZCL_CLUSTER_ID_HA_ELECTRICAL_MEASUREMENT             0x0B04
#define ZCL_CLUSTER_ID_HA_DIAGNOSTIC                         0x0B05

// Light Link cluster
#define ZCL_CLUSTER_ID_LIGHT_LINK                           0x1000

/*** Frame Control bit mask ***/
#define ZCL_FRAME_CONTROL_TYPE                          0x03
#define ZCL_FRAME_CONTROL_MANU_SPECIFIC                 0x04
#define ZCL_FRAME_CONTROL_DIRECTION                     0x08
#define ZCL_FRAME_CONTROL_DISABLE_DEFAULT_RSP           0x10

/*** Frame Types ***/
#define ZCL_FRAME_TYPE_PROFILE_CMD                      0x00
#define ZCL_FRAME_TYPE_SPECIFIC_CMD                     0x01

/*** Frame Client/Server Directions ***/
#define ZCL_FRAME_CLIENT_SERVER_DIR                     0x00
#define ZCL_FRAME_SERVER_CLIENT_DIR                     0x01

/*** Chipcon Manufacturer Code ***/
#define CC_MANUFACTURER_CODE                            0x1001

/*** Foundation Command IDs ***/
#define ZCL_CMD_READ                                    0x00
#define ZCL_CMD_READ_RSP                                0x01
#define ZCL_CMD_WRITE                                   0x02
#define ZCL_CMD_WRITE_UNDIVIDED                         0x03
#define ZCL_CMD_WRITE_RSP                               0x04
#define ZCL_CMD_WRITE_NO_RSP                            0x05
#define ZCL_CMD_CONFIG_REPORT                           0x06
#define ZCL_CMD_CONFIG_REPORT_RSP                       0x07
#define ZCL_CMD_READ_REPORT_CFG                         0x08
#define ZCL_CMD_READ_REPORT_CFG_RSP                     0x09
#define ZCL_CMD_REPORT                                  0x0a
#define ZCL_CMD_DEFAULT_RSP                             0x0b
#define ZCL_CMD_DISCOVER_ATTRS                          0x0c
#define ZCL_CMD_DISCOVER_ATTRS_RSP                      0x0d
#define ZCL_CMD_DISCOVER_CMDS_RECEIVED                  0x11
#define ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP              0x12
#define ZCL_CMD_DISCOVER_CMDS_GEN                       0x13
#define ZCL_CMD_DISCOVER_CMDS_GEN_RSP                   0x14
#define ZCL_CMD_DISCOVER_ATTRS_EXT                      0x15
#define ZCL_CMD_DISCOVER_ATTRS_EXT_RSP                  0x16

#define ZCL_CMD_MAX           ZCL_CMD_DISCOVER_ATTRS_EXT_RSP

// define reporting constant
#define ZCL_REPORTING_OFF     0xFFFF  // turn off reporting (maxReportInt)

// define command direction flag masks
#define CMD_DIR_SERVER_GENERATED          0x01
#define CMD_DIR_CLIENT_GENERATED          0x02
#define CMD_DIR_SERVER_RECEIVED           0x04
#define CMD_DIR_CLIENT_RECEIVED           0x08

/*** Data Types ***/
#define ZCL_DATATYPE_NO_DATA                            0x00
#define ZCL_DATATYPE_DATA8                              0x08
#define ZCL_DATATYPE_DATA16                             0x09
#define ZCL_DATATYPE_DATA24                             0x0a
#define ZCL_DATATYPE_DATA32                             0x0b
#define ZCL_DATATYPE_DATA40                             0x0c
#define ZCL_DATATYPE_DATA48                             0x0d
#define ZCL_DATATYPE_DATA56                             0x0e
#define ZCL_DATATYPE_DATA64                             0x0f
#define ZCL_DATATYPE_BOOLEAN                            0x10
#define ZCL_DATATYPE_BITMAP8                            0x18
#define ZCL_DATATYPE_BITMAP16                           0x19
#define ZCL_DATATYPE_BITMAP24                           0x1a
#define ZCL_DATATYPE_BITMAP32                           0x1b
#define ZCL_DATATYPE_BITMAP40                           0x1c
#define ZCL_DATATYPE_BITMAP48                           0x1d
#define ZCL_DATATYPE_BITMAP56                           0x1e
#define ZCL_DATATYPE_BITMAP64                           0x1f
#define ZCL_DATATYPE_UINT8                              0x20
#define ZCL_DATATYPE_UINT16                             0x21
#define ZCL_DATATYPE_UINT24                             0x22
#define ZCL_DATATYPE_UINT32                             0x23
#define ZCL_DATATYPE_UINT40                             0x24
#define ZCL_DATATYPE_UINT48                             0x25
#define ZCL_DATATYPE_UINT56                             0x26
#define ZCL_DATATYPE_UINT64                             0x27
#define ZCL_DATATYPE_INT8                               0x28
#define ZCL_DATATYPE_INT16                              0x29
#define ZCL_DATATYPE_INT24                              0x2a
#define ZCL_DATATYPE_INT32                              0x2b
#define ZCL_DATATYPE_INT40                              0x2c
#define ZCL_DATATYPE_INT48                              0x2d
#define ZCL_DATATYPE_INT56                              0x2e
#define ZCL_DATATYPE_INT64                              0x2f
#define ZCL_DATATYPE_ENUM8                              0x30
#define ZCL_DATATYPE_ENUM16                             0x31
#define ZCL_DATATYPE_SEMI_PREC                          0x38
#define ZCL_DATATYPE_SINGLE_PREC                        0x39
#define ZCL_DATATYPE_DOUBLE_PREC                        0x3a
#define ZCL_DATATYPE_OCTET_STR                          0x41
#define ZCL_DATATYPE_CHAR_STR                           0x42
#define ZCL_DATATYPE_LONG_OCTET_STR                     0x43
#define ZCL_DATATYPE_LONG_CHAR_STR                      0x44
#define ZCL_DATATYPE_ARRAY                              0x48
#define ZCL_DATATYPE_STRUCT                             0x4c
#define ZCL_DATATYPE_SET                                0x50
#define ZCL_DATATYPE_BAG                                0x51
#define ZCL_DATATYPE_TOD                                0xe0
#define ZCL_DATATYPE_DATE                               0xe1
#define ZCL_DATATYPE_UTC                                0xe2
#define ZCL_DATATYPE_CLUSTER_ID                         0xe8
#define ZCL_DATATYPE_ATTR_ID                            0xe9
#define ZCL_DATATYPE_BAC_OID                            0xea
#define ZCL_DATATYPE_IEEE_ADDR                          0xf0
#define ZCL_DATATYPE_128_BIT_SEC_KEY                    0xf1
#define ZCL_DATATYPE_UNKNOWN                            0xff

#define SEC_KEY_LEN  16  // 128/8 octets (128-bit key is standard for ZigBee)

/*** Error Status Codes ***/
#define ZCL_STATUS_SUCCESS                              0x00
#define ZCL_STATUS_FAILURE                              0x01
// 0x02-0x7D are reserved.
#define ZCL_STATUS_NOT_AUTHORIZED                       0x7E
#define ZCL_STATUS_MALFORMED_COMMAND                    0x80
#define ZCL_STATUS_UNSUP_CLUSTER_COMMAND                0x81
#define ZCL_STATUS_UNSUP_GENERAL_COMMAND                0x82
#define ZCL_STATUS_UNSUP_MANU_CLUSTER_COMMAND           0x83
#define ZCL_STATUS_UNSUP_MANU_GENERAL_COMMAND           0x84
#define ZCL_STATUS_INVALID_FIELD                        0x85
#define ZCL_STATUS_UNSUPPORTED_ATTRIBUTE                0x86
#define ZCL_STATUS_INVALID_VALUE                        0x87
#define ZCL_STATUS_READ_ONLY                            0x88
#define ZCL_STATUS_INSUFFICIENT_SPACE                   0x89
#define ZCL_STATUS_DUPLICATE_EXISTS                     0x8a
#define ZCL_STATUS_NOT_FOUND                            0x8b
#define ZCL_STATUS_UNREPORTABLE_ATTRIBUTE               0x8c
#define ZCL_STATUS_INVALID_DATA_TYPE                    0x8d
#define ZCL_STATUS_INVALID_SELECTOR                     0x8e
#define ZCL_STATUS_WRITE_ONLY                           0x8f
#define ZCL_STATUS_INCONSISTENT_STARTUP_STATE           0x90
#define ZCL_STATUS_DEFINED_OUT_OF_BAND                  0x91
#define ZCL_STATUS_INCONSISTENT                         0x92
#define ZCL_STATUS_ACTION_DENIED                        0x93
#define ZCL_STATUS_TIMEOUT                              0x94
#define ZCL_STATUS_ABORT                                0x95
#define ZCL_STATUS_INVALID_IMAGE                        0x96
#define ZCL_STATUS_WAIT_FOR_DATA                        0x97
#define ZCL_STATUS_NO_IMAGE_AVAILABLE                   0x98
#define ZCL_STATUS_REQUIRE_MORE_IMAGE                   0x99

// 0xbd-bf are reserved.
#define ZCL_STATUS_HARDWARE_FAILURE                     0xc0
#define ZCL_STATUS_SOFTWARE_FAILURE                     0xc1
#define ZCL_STATUS_CALIBRATION_ERROR                    0xc2
// 0xc3-0xff are reserved.
#define ZCL_STATUS_CMD_HAS_RSP                          0xFF // Non-standard status (used for Default Rsp)

/*** Attribute Access Control - bit masks ***/
#define ACCESS_CONTROL_READ                             0x01  // attribute can be read
#define ACCESS_CONTROL_WRITE                            0x02  // attribute can be written
#define ACCESS_REPORTABLE                               0x04  // indicate attribute is reportable
#define ACCESS_CONTROL_COMMAND                          0x08
#define ACCESS_CONTROL_AUTH_READ                        0x10
#define ACCESS_CONTROL_AUTH_WRITE                       0x20
#define ACCESS_CLIENT                                   0x80  // TI unique, indicate client side attribute

// Access Control as reported OTA via DiscoveryAttributesExtended
#define ACCESS_CONTROLEXT_MASK                          0x07  // read/write/reportable bits same as above

#define ZCL_ATTR_ID_MAX                                 0xFFFF

// Used by Configure Reporting Command
#define ZCL_SEND_ATTR_REPORTS                           0x00
#define ZCL_EXPECT_ATTR_REPORTS                         0x01

// Predefined Maximum String Length
#define MAX_UTF8_STRING_LEN                             50

// Used by zclReadWriteCB_t callback function
#define ZCL_OPER_LEN                                    0x00 // Get length of attribute value to be read
#define ZCL_OPER_READ                                   0x01 // Read attribute value
#define ZCL_OPER_WRITE                                  0x02 // Write new attribute value

/*********************************************************************
 * MACROS
 */
#define zcl_ProfileCmd( a )         ( (a) == ZCL_FRAME_TYPE_PROFILE_CMD )
#define zcl_ClusterCmd( a )         ( (a) == ZCL_FRAME_TYPE_SPECIFIC_CMD )

#define zcl_ServerCmd( a )          ( (a) == ZCL_FRAME_CLIENT_SERVER_DIR )
#define zcl_ClientCmd( a )          ( (a) == ZCL_FRAME_SERVER_CLIENT_DIR )

#define UNICAST_MSG( msg )          ( (msg)->wasBroadcast == false && (msg)->groupId == 0 )

// Padding needed if buffer has odd number of octects in length
#define PADDING_NEEDED( bufLen )    ( (bufLen) % 2 )

// Check for Cluster IDs
#define ZCL_CLUSTER_ID_GEN( id )      ( /* (id) >= ZCL_CLUSTER_ID_GEN_BASIC &&*/ \
	(id) <= ZCL_CLUSTER_ID_GEN_COMMISSIONING )
#define ZCL_CLUSTER_ID_CLOSURES( id ) ( (id) >= ZCL_CLUSTER_ID_CLOSURES_SHADE_CONFIG && \
	(id) <= ZCL_CLUSTER_ID_CLOSURES_WINDOW_COVERING )
#define ZCL_CLUSTER_ID_HVAC( id )     ( (id) >= ZCL_CLUSTER_ID_HVAC_PUMP_CONFIG_CONTROL && \
	(id) <= ZCL_CLUSTER_ID_HVAC_USER_INTERFACE_CONFIG )
#define ZCL_CLUSTER_ID_LIGHTING( id ) ( (id) >= ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL && \
	(id) <= ZCL_CLUSTER_ID_LIGHTING_BALLAST_CONFIG )
#define ZCL_CLUSTER_ID_MS( id )       ( (id) >= ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT && \
	(id) <= ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING )
#define ZCL_CLUSTER_ID_SS( id )       ( (id) >= ZCL_CLUSTER_ID_SS_IAS_ZONE && \
	(id) <= ZCL_CLUSTER_ID_SS_IAS_WD )
#define ZCL_CLUSTER_ID_KEY( id )      ( (id) == ZCL_CLUSTER_ID_GEN_KEY_ESTABLISHMENT )
#define ZCL_CLUSTER_ID_SE( id )       ( (id) >= ZCL_CLUSTER_ID_SE_PRICING && \
	(id) <= ZCL_CLUSTER_ID_SE_MDU_PAIRING )
#define ZCL_CLUSTER_ID_PI( id )       ( (id) >= ZCL_CLUSTER_ID_PI_GENERIC_TUNNEL && \
	(id) <= ZCL_CLUSTER_ID_PI_11073_PROTOCOL_TUNNEL )
#define ZCL_CLUSTER_ID_LL( id )       ( (id) == ZCL_CLUSTER_ID_LIGHT_LINK )
#define ZCL_CLUSTER_ID_PART( id )     ( (id) == ZCL_CLUSTER_ID_GEN_PARTITION )

#define ZCL_CLUSTER_ID_PC( id )       ( (id) == ZCL_CLUSTER_ID_GEN_POLL_CONTROL )

#define ZCL_CLUSTER_ID_EM( id )	      ( (id) == ZCL_CLUSTER_ID_HA_ELECTRICAL_MEASUREMENT )

#define ZCL_CLUSTER_ID_DIAG( id )     ( (id) == ZCL_CLUSTER_ID_HA_DIAGNOSTIC )

#define ZCL_CLUSTER_ID_MI( id )       ( (id) == ZCL_CLUSTER_ID_HA_METER_IDENTIFICATION )

#define ZCL_CLUSTER_ID_APPL_C( id )   ( (id) == ZCL_CLUSTER_ID_GEN_APPLIANCE_CONTROL )

#define ZCL_CLUSTER_ID_APPL_I( id )   ( (id) == ZCL_CLUSTER_ID_HA_APPLIANCE_IDENTIFICATION )

#define ZCL_CLUSTER_ID_APPL_STAT( id ) ( (id) == ZCL_CLUSTER_ID_HA_APPLIANCE_STATISTICS )

#define ZCL_CLUSTER_ID_APPL_E_A( id ) ( (id) == ZCL_CLUSTER_ID_HA_APPLIANCE_EVENTS_ALERTS )

#define ZCL_CLUSTER_ID_APPL_S( id )   ( (id) == ZCL_CLUSTER_ID_HA_APPLIANCE_STATISTICS )

#define ZCL_CLUSTER_ID_PP( id )       ( (id) == ZCL_CLUSTER_ID_GEN_POWER_PROFILE )

#define ZCL_CLUSTER_ID_DL( id )       ( (id) == ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK )

#define Z_EXTADDR_LEN   8

/*********************************************************************
 * TYPEDEFS
 */
typedef enum
{
	ZCL_PROC_SUCCESS = 0,                 // Message was processed
	ZCL_PROC_INVALID ,                    // Format or parameter was wrong
	ZCL_PROC_EP_NOT_FOUND,                // Endpoint descriptor not found
	ZCL_PROC_NOT_OPERATIONAL,             // Can't respond to this command
	ZCL_PROC_INTERPAN_FOUNDATION_CMD,     // INTER-PAN and Foundation Command (not allowed)
	ZCL_PROC_NOT_SECURE,                  // Security was required but the message is not secure
	ZCL_PROC_MANUFACTURER_SPECIFIC,       // Manufacturer Specific command - not handled
	ZCL_PROC_MANUFACTURER_SPECIFIC_DR,    // Manufacturer Specific command - not handled, but default response sent
	ZCL_PROC_NOT_HANDLED,                 // No default response was sent and the message was not handled
	ZCL_PROC_NOT_HANDLED_DR,              // default response was sent and the message was not handled
} zclProcMsgStatus_t;

typedef enum _ZdpStatus {
	ZDP_STATUS__SUCCESS = 0,
	ZDP_STATUS__INVALID_REQTYPE = 128,
	ZDP_STATUS__DEVICE_NOT_FOUND = 129,
	ZDP_STATUS__INVALID_EP = 130,
	ZDP_STATUS__NOT_ACTIVE = 131,
	ZDP_STATUS__NOT_SUPPORTED = 132,
	ZDP_STATUS__TIMEOUT = 133,
	ZDP_STATUS__NO_MATCH = 134,
	ZDP_STATUS__NO_ENTRY = 136,
	ZDP_STATUS__NO_DESCRIPTOR = 137,
	ZDP_STATUS__INSUFFICIENT_SPACE = 138,
	ZDP_STATUS__NOT_PERMITTED = 139,
	ZDP_STATUS__TABLE_FULL = 140,
	ZDP_STATUS__NOT_AUTHORIZED = 141,
	ZDP_STATUS__BINDING_TABLE_FULL = 142
} ZdpStatus;

typedef struct
{
	uint32_t srcaddr;
	ZdpStatus status;
} ZdoMgmtPermitJoinRspInd_t;

typedef struct
{
	uint16_t nwkAddr;
	uint8_t  extAddr[Z_EXTADDR_LEN];
	uint16_t parentAddr;
} ZDO_TC_Device_t;

typedef struct
{
	uint16_t      SrcAddr;
	uint16_t      nwkAddr;
	uint8_t       extAddr[Z_EXTADDR_LEN];
	uint8_t       capabilities;
} ZDO_DeviceAnnce_t;

typedef struct
{
	uint8_t status; // Status is either Success (0) or Failure (1).
	uint8_t endpoint; // Endpoint of the device
	uint8_t transID; // Specifies the transaction sequence number of the message
} afDataConfirm_t;

typedef struct  __attribute__((__packed__))
{
	uint16_t group_id;
	uint16_t cluster_id;
	uint16_t src_addr;
	uint8_t src_endpoint;
	uint8_t dst_endpoint;
	uint8_t was_broadcast;
	uint8_t link_quality;
	uint8_t security_use;
	uint32_t time_stamp;
	uint8_t trans_seq_num;
	uint8_t len;
	uint8_t payload[128];
} afIncomingMSGPacket_t;

// ZCL header - frame control field
typedef struct
{
	unsigned char type:2;
	unsigned char manuSpecific:1;
	unsigned char direction:1;
	unsigned char disableDefaultRsp:1;
	unsigned char reserved:3;
} zclFrameControl_t;

// ZCL header
typedef struct
{
	zclFrameControl_t fc;
	uint16_t            manuCode;
	uint8_t             transSeqNum;
	uint8_t             commandID;
} zclFrameHdr_t;

//#ifdef ZCL_READ
// Read Attribute Command format
typedef struct
{
	uint8_t numAttr;            // number of attributes in the list
	uint16_t attrID[];           // supported attributes list - this structure should
	// be allocated with the appropriate number of attributes.
} zclReadCmd_t;

// Read Attribute Response Status record
typedef struct
{
	uint16_t attrID;            // attribute ID
	uint8_t status;            // should be ZCL_STATUS_SUCCESS or error
	uint8_t  dataType;          // attribute data type
	uint8_t  dataLen;			//len of data
	uint8_t  *data;             // this structure is allocated, so the data is HERE
	// - the size depends on the attribute data type
} zclReadRspStatus_t;

// Read Attribute Response Command format
typedef struct
{
	uint8_t              numAttr;     // number of attributes in the list
	zclReadRspStatus_t attrList[];  // attribute status list
} zclReadRspCmd_t;
//#endif // ZCL_READ

// Write Attribute record
typedef struct
{
	uint16_t attrID;             // attribute ID
	uint8_t  dataType;           // attribute data type
	uint8_t  *attrData;          // this structure is allocated, so the data is HERE
	//  - the size depends on the attribute data type
} zclWriteRec_t;

// Write Attribute Command format
typedef struct
{
	uint8_t         numAttr;     // number of attribute records in the list
	zclWriteRec_t attrList[];  // attribute records
} zclWriteCmd_t;

// Write Attribute Status record
typedef struct
{
	uint8_t  status;             // should be ZCL_STATUS_SUCCESS or error
	uint16_t attrID;             // attribute ID
} zclWriteRspStatus_t;

// Write Attribute Response Command format
typedef struct
{
	uint8_t               numAttr;     // number of attribute status in the list
	zclWriteRspStatus_t attrList[];  // attribute status records
} zclWriteRspCmd_t;

// Configure Reporting Command format
typedef struct
{
	uint8_t  direction;          // to send or receive reports of the attribute
	uint16_t attrID;             // attribute ID
	uint8_t  dataType;           // attribute data type
	uint16_t minReportInt;       // minimum reporting interval
	uint16_t maxReportInt;       // maximum reporting interval, 0xFFFF=off
	uint16_t timeoutPeriod;      // timeout period
	uint8_t  *reportableChange;  // reportable change (only applicable to analog data type)
	// - the size depends on the attribute data type
} zclCfgReportRec_t;

typedef struct
{
	uint8_t             numAttr;    // number of attribute IDs in the list
	zclCfgReportRec_t attrList[]; // attribute ID list
} zclCfgReportCmd_t;

// Attribute Status record
typedef struct
{
	uint8_t  status;             // should be ZCL_STATUS_SUCCESS or error
	uint8_t  direction;          // whether attributes are reported or reports of attributes are received
	uint16_t attrID;             // attribute ID
} zclCfgReportStatus_t;

// Configure Reporting Response Command format
typedef struct
{
	uint8_t                numAttr;    // number of attribute status in the list
	zclCfgReportStatus_t attrList[]; // attribute status records
} zclCfgReportRspCmd_t;

// Read Reporting Configuration Command format
typedef struct
{
	uint8_t  direction; // to send or receive reports of the attribute
	uint16_t attrID;    // attribute ID
} zclReadReportCfgRec_t;

typedef struct
{
	uint8_t                 numAttr;    // number of attributes in the list
	zclReadReportCfgRec_t attrList[]; // attribute ID list
} zclReadReportCfgCmd_t;

// Attribute Reporting Configuration record
typedef struct
{
	uint8_t  status;             // status field
	uint8_t  direction;          // to send or receive reports of the attribute
	uint16_t attrID;             // attribute ID
	uint8_t  dataType;           // attribute data type
	uint16_t minReportInt;       // minimum reporting interval
	uint16_t maxReportInt;       // maximum reporting interval
	uint16_t timeoutPeriod;      // timeout period
	uint8_t  *reportableChange;  // reportable change (only applicable to analog data type)
	// - the size depends on the attribute data type
} zclReportCfgRspRec_t;

// Read Reporting Configuration Response Command format
typedef struct
{
	uint8_t                numAttr;    // number of records in the list
	zclReportCfgRspRec_t attrList[]; // attribute reporting configuration list
} zclReadReportCfgRspCmd_t;

// Attribute Report
typedef struct
{
	uint16_t attrID;             // atrribute ID
	uint8_t  dataType;           // attribute data type
	uint8_t  dataLen;			  // len of data
	uint8_t  *Data;          // this structure is allocated, so the data is HERE
	// - the size depends on the data type of attrID
} zclReport_t;

// Report Attributes Command format
typedef struct
{
	uint8_t       numAttr;       // number of reports in the list
	zclReport_t attrList[];    // attribute report list
} zclReportCmd_t;

// Default Response Command format
typedef struct
{
	uint8_t  commandID;
	uint8_t  statusCode;
} zclDefaultRspCmd_t;

// Discover Attributes and Attributes Extended Command format
typedef struct
{
	uint16_t startAttr;          // specifies the minimum value of the identifier
	// to begin attribute discovery.
	uint8_t  maxAttrIDs;         // maximum number of attribute IDs that are to be
	// returned in the resulting response command.
} zclDiscoverAttrsCmd_t;

// Attribute Report info
typedef struct
{
	uint16_t attrID;             // attribute ID
	uint8_t  dataType;           // attribute data type (see Table 17 in Spec)
} zclDiscoverAttrInfo_t;

// Discover Attributes Response Command format
typedef struct
{
	uint8_t             discComplete; // whether or not there're more attributes to be discovered
	uint8_t             numAttr;      // number of attributes in the list
	zclDiscoverAttrInfo_t attrList[];   // supported attributes list
} zclDiscoverAttrsRspCmd_t;

// String Data Type
typedef struct
{
	uint8_t strLen;
	uint8_t *pStr;
} UTF8String_t;

// Command format for the following:
// Discover Commands Received, Discover Commands Generated
typedef struct
{
	uint8_t startCmdID;
	uint8_t maxCmdID;
} zclDiscoverCmdsCmd_t;

// Command format for the following:
// Discover Commands Received Response Command, Discover Commands Generated Response
typedef struct
{
	uint8_t discComplete;
	uint8_t cmdType;    // either ZCL_CMD_DISCOVER_CMDS_GEN or ZCL_CMD_DISCOVER_CMDS_RECEIVED
	uint8_t numCmd;     // number of provided commands
	uint8_t *pCmdID;    // variable length array
} zclDiscoverCmdsCmdRsp_t;

// Discover Attributes Extended Response Command format
typedef struct
{
	uint16_t attrID;
	uint8_t attrDataType;
	uint8_t attrAccessControl;
} zclExtAttrInfo_t;

typedef struct
{
	uint8_t discComplete;
	uint8_t numAttr;                  // number of attributes provided
	zclExtAttrInfo_t  aExtAttrInfo[];    // variable length array
} zclDiscoverAttrsExtRsp_t;

/*********************************************************************
 * Plugins
 */

// Incoming ZCL message, this buffer will be allocated, cmd will point to the
// the command record.
typedef struct
{
	afIncomingMSGPacket_t 		*msg;        // incoming message
	zclFrameHdr_t     		hdr;         // ZCL header parsed
	uint8_t                 	*pData;      // pointer to data after header
	uint16_t                	pDataLen;    // length of remaining data
	void                  	*attrCmd;    // pointer to the parsed attribute or command
} zclIncoming_t;

// Outgoing ZCL Cluster Specific Commands
typedef struct
{
	zclFrameHdr_t hdr;
	uint16_t        realClusterID;
	uint16_t        attrID;
	void          *cmdStruct;
	uint8_t         cmdLen;
	uint8_t         *cmdData;
} zclOutgoingCmd_t;

typedef struct
{
	uint16_t 	group_id;
	uint16_t    short_addr;
	uint16_t    cluster_id;
	uint8_t 	cmd;
	uint16_t 	attrID;            // attribute ID
	uint8_t  	dataType;          // attribute data type
	uint8_t  	dataLen;			//len of data
	uint8_t  	*data;             // this structure is allocated, so the data is HERE
} zclOutgoingMsg_t;

/*for IAS zone status*/
#define IAS_ZONE_ALARM1					0x0001
#define IAS_ZONE_ALARM2					0x0002
#define IAS_ZONE_TAMPER					0x0004
#define IAS_ZONE_BATTERY				0x0008
#define IAS_ZONE_SUPERVISION_RESPORTS	0x0010
#define IAS_ZONE_RESTORE_RESPORTS		0x0020
#define IAS_ZONE_TROUBLE				0x0040
#define IAS_ZONE_AC						0x0080
#define IAS_ZONE_TEST					0x0100
#define IAS_ZONE_BATTERY_DEFECT			0x0200

// Parse received command
typedef struct
{
	uint8_t  endpoint;
	uint16_t dataLen;
	uint8_t  *pData;
} zclParseCmd_t;

extern zclProcMsgStatus_t zcl_ProcessMessageMSG( afIncomingMSGPacket_t *pkt );
extern uint8_t *zclParseHdr( zclFrameHdr_t *hdr, uint8_t *pData );
extern uint16_t zclGetAttrDataLength(uint8_t dataType, uint8_t *pData, uint16_t len);
extern uint8_t zclGetDataTypeLength(uint8_t dataType);

#endif // __ZB_ZCL_H__
