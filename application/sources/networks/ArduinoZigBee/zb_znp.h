#ifndef __ZB_ZNP_H__
#define __ZB_ZNP_H__

#include <stdint.h>
#include "Stream.h"
#include "zb_defs.h"

#define CALLBACKS_DISABLED				0
#define CALLBACKS_ENABLED				1

#define CHANNEL_TRUE					1
#define CHANNEL_FALSE					0

#define CHANNEL_MASK_11					0x800
#define CHANNEL_MASK_12					0x1000
#define CHANNEL_MASK_13					0x2000
#define CHANNEL_MASK_14					0x4000
#define CHANNEL_MASK_15					0x8000
#define CHANNEL_MASK_16					0x10000
#define CHANNEL_MASK_17					0x20000
#define CHANNEL_MASK_18					0x40000
#define CHANNEL_MASK_19					0x80000
#define CHANNEL_MASK_20					0x100000
#define CHANNEL_MASK_21					0x200000
#define CHANNEL_MASK_22					0x400000
#define CHANNEL_MASK_23					0x800000
#define CHANNEL_MASK_24					0x1000000
#define CHANNEL_MASK_25					0x2000000
#define CHANNEL_MASK_26					0x4000000
#define ALL_CHANNEL_MASK				0x7FFF800

#define STARTOPT_CLEAR_CONFIG			0x01
#define STARTOPT_CLEAR_STATE			0x02
#define STARTOPT_AUTO					0x04

#define DEFAULT_STARTUP_OPTIONS			(STARTOPT_CLEAR_CONFIG + STARTOPT_CLEAR_STATE)
#define DEFAULT_CHANNEL_MASK			CHANNEL_MASK_13
#define ANY_PAN							0xFFFF

#define DEFAULT_ENDPOINT				0x1
#define DEFAULT_PROFILE_ID				0x0104
#define DEVICE_ID						0x0123
#define DEVICE_VERSION					0x89

#define LATENCY_NORMAL					0
#define LATENCY_FAST_BEACONS			1
#define LATENCY_SLOW_BEACONS			2

#define DEFAULT_TX_POWER				20
#define ZNP_SOF							0xFE

#define NORMAL_CMD_WAIT					150000
#define START_APP_CMD_WAIT				3500000

#define PAN_ID							0x00F1

#define COORDINATOR						0x00
#define ROUTER							0x01
#define END_DEVICE						0x02

#define ZCD_NV_STARTUP_OPTION			0x03
#define ZCD_NV_STARTUP_OPTION_LEN		1
#define ZCD_NV_LOGICAL_TYPE				0x87
#define ZCD_NV_LOGICAL_TYPE_LEN			1
#define ZCD_NV_ZDO_DIRECT_CB			0x8F
#define ZCD_NV_ZDO_DIRECT_CB_LEN		1
#define ZCD_NV_PANID					0x83
#define ZCD_NV_PANID_LEN				2

#define STARTOPT_CLEAR_CONFIG			0x01
#define STARTOPT_CLEAR_STATE			0x02
#define STARTOPT_AUTO					0x04

#define SYS_SET_TX_POWER				0x2114
#define SYS_RESET_IND					0x4180
#define SYS_RESET_REQ					0x4100

#define AF_REGISTER 					0x2400
#define AF_DATA_REQUEST 				0x2401
#define AF_INCOMING_MSG					0x4481
#define AF_DATA_CONFIRM					0x4480
#define AF_DATA_REQUEST_IND 			0x6401

#define ZDO_STARTUP_FROM_APP 			0x2540
#define ZDO_IEEE_ADDR_REQ 				0x2501
#define ZDO_SIMPLE_DESC_REQ 			0x2504
#define ZDO_BIND_REQ					0x2521
#define ZDO_IEEE_ADDR_RSP 				0x4581
#define ZDO_SIMPLE_DESC_RSP				0x4584
#define ZDO_BIND_RSP					0x45A1
#define ZDO_LEAVE_IND 					0x45C9
#define ZDO_STATE_CHANGE_IND 			0x45C0
#define ZDO_END_DEVICE_ANNCE_IND		0x45C1
#define ZDO_TC_DEV_IND					0x45CA
#define ZDO_MGMT_PERMIT_JOIN_RSP 		0x45B6
#define ZDO_MGMT_LEAVE_REQ				0x2534
#define ZDO_MGMT_PERMIT_JOIN_REQ		0x2536
#define ZDO_MGMT_LEAVE_RSP 				0x45B4
#define ZDO_MGMT_PERMIT_JOIN_RSP 		0x45B6
#define ZDO_MGMT_LEAVE_REQ_FLAG_NONE					0x00
#define ZDO_MGMT_LEAVE_REQ_FLAG_REJOIN					0x01
#define ZDO_MGMT_LEAVE_REQ_FLAG_REMOVE_CHILDREN			0x02

#define UTIL_GET_DEVICE_INFO 			0x2700
#define UTIL_GET_DEVICE_INFO_RESPONSE 	0x6700

#define ZB_START_REQUEST 				0x2600
#define ZB_READ_CONFIGURATION 			0x2604
#define ZB_WRITE_CONFIGURATION			0x2605
#define ZB_GET_DEVICE_INFO				0x2606
#define ZB_PERMIT_JOINING_REQUEST 		0x2608
#define ZB_APP_REGISTER_REQUEST 		0x260A
#define ZB_READ_CONFIGURATION_RSP 		0x6604
#define ZB_START_CONFIRM 				0x4680
#define ZB_RECEIVE_DATA_INDICATION 		0x4687

#define APP_CNF_SET_ALLOWREJOIN_TC_POLICY				0x2F03
#define	APP_CNF_BDB_START_COMMISSIONING					0x2F05
#define APP_CNF_BDB_SET_CHANNEL							0x2F08
#define APP_CNF_BDB_SET_TC_REQUIRE_KEY_EXCHANGE			0x2F09
#define APP_CNF_BDB_SET_TC_REQUIRE_KEY_EXCHANGE_SRSP	0x6F09
#define APP_CNF_BDB_COMMISSIONING_NOTIFICATION			0x4F80

#define	COMMISSIONING_MODE_STEERING			0x02
#define	COMMISSIONING_MODE_INFORMATION		0x04

#define ZNP_RET_OK							0x00
#define ZNP_RET_NG							0xFF

#define	DISABLE_PERMIT_JOIN					0x00
#define	TIME_ENABLE_PERMIT_JOIN				0x3C	//60 seconds

#define SHORT_ADDRESS_COORDINATOR			0x0000
#define ALL_ROUTER_AND_COORDINATOR			0xFFFC

#define SECURITY_MODE_OFF					0x00
#define SECURITY_MODE_PRECONFIGURED_KEYS	0x01
#define SECURITY_MODE_COORD_DIST_KEYS		0x02

#define ZCD_NV_PRECFGKEY					0x62
#define ZCD_NV_PRECFGKEY_LEN				16
#define ZCD_NV_PRECFGKEYS_ENABLE			0x63
#define ZCD_NV_PRECFGKEYS_ENABLE_LEN		0x01
#define ZCD_NV_SECURITY_MODE				0x64
#define ZCD_NV_SECURITY_MODE_LEN			0x01

typedef enum {
	ADDRESS_NOT_PRESENT = 0,
	GROUP_ADDRESS,
	ADDRESS_16_BIT,
	ADDRESS_64_BIT,
	BROADCAST = 0x0F,
} BindAddrMode;

typedef struct {
	uint16_t short_address;
	uint8_t device_address[8];
	uint8_t flags;
} __ZB_PACKETED zdo_mgmt_leave_req_t;

typedef struct  {
	uint16_t dst_address;
	uint8_t dst_endpoint;
	uint8_t src_endpoint;
	uint16_t cluster_id;
	uint8_t trans_id;
	uint8_t options;
	uint8_t radius;
	uint8_t len;
	uint8_t *data;
} __ZB_PACKETED af_data_request_t;

typedef struct {
	uint8_t src_short_addr[2];
	uint8_t src_ieee_addr[8];
	uint8_t src_endpoint;
	uint16_t cluster_id;
	uint8_t dst_mode;
	uint8_t dst_address[8];
	uint8_t dst_endpoint;
} __ZB_PACKETED binding_req_t;

#define SOP_STATE      0x00
#define LEN_STATE      0x01
#define CMD0_STATE     0x02
#define CMD1_STATE     0x03
#define DATA_STATE     0x04
#define FCS_STATE      0x05

#define SOF_CHAR			0xFE
#define ZNP_PARSER_IDLE		0x00
#define ZNP_PARSER_DOING	0x01
#define ZNP_PARSER_ERR		0x02
#define ZNP_PARSER_DONE		0x03

#define FRAME_DATA_SIZE		256

typedef struct {
	uint8_t len;
	uint8_t cmd0;
	uint8_t cmd1;
	uint8_t data[FRAME_DATA_SIZE];
} __ZB_PACKETED zigbee_msg_t;

typedef struct {
	zigbee_msg_t zigbee_msg;
	uint8_t zigbee_msg_data_index;
	uint8_t zigbee_msg_denied_handle;
	uint8_t state;
	uint8_t frame_fcs;
} __ZB_PACKETED znp_frame_t;

class zb_znp {
public:
	zb_znp(Stream* znpStream) {
		znp_frame.state = SOP_STATE; //init znp_frame buffer state
		m_znp_stream = znpStream;
	}

	~zb_znp() {}

	znp_frame_t znp_frame;

	int update();

	int write(uint8_t*, uint32_t);
	int read(uint8_t*, uint32_t);

	static int zigbee_message_handler(zigbee_msg_t&);

	uint8_t soft_reset();
	void hard_reset();

	/* (opt = 0)
	 * to normal start coodinator, and keep configures and joined devices.
	 * (opt = 1)
	 * to force start coodinator, reset configures to default and clean all joined devices.
	 */
	uint8_t start_coordinator(uint8_t opt);

	/* (opt = 0)
	 * to normal start router, and keep configures.
	 * (opt = 1)
	 * to force start router, reset configures to default.
	 * (opt = 2)
	 * to auto start router.
	 */
	uint8_t start_router(uint8_t opt);

	uint8_t bdb_start_commissioning(uint8_t mode_config, uint8_t mode_receiving, uint8_t flag_waiting = 0);
	uint8_t util_get_device_info();
	uint8_t zdo_mgmt_leave_req(uint16_t short_add, uint8_t ieee_addr[8], uint8_t flags);
	uint8_t set_permit_joining_req(uint16_t short_addr, uint8_t timeout, uint8_t flag_waiting = 0);
	uint8_t get_mac_addr_req(uint16_t short_addr, uint8_t req_type, uint8_t start_index);
	uint8_t send_af_data_req(af_data_request_t af_data_request);
	uint8_t set_security_mode(uint8_t security_mode);
	uint8_t set_security_key(uint8_t* key);
	uint8_t zb_get_device_info(uint8_t param, uint8_t* rx_buffer, uint32_t* len);
	uint8_t zb_read_configuration(uint8_t config_id, uint8_t* rx_buffer, uint32_t* len);
	uint8_t zdo_binding_req(binding_req_t binding_req);
	uint8_t zdo_simple_desc_req(uint16_t dst_addr, uint8_t dst_enpoint);
	uint8_t app_cnf_set_allowrejoin_tc_policy(uint8_t mode);
	uint8_t set_panid(uint16_t pan_id);
	uint8_t set_zigbee_device_type(uint8_t dev_type);
	uint8_t set_transmit_power(uint8_t tx_power_db);
	uint8_t set_channel_mask(uint8_t primary, uint32_t channelmask);
	uint8_t af_register_generic_application(uint8_t endpoint);
	uint8_t zb_app_register_request();
	uint8_t zb_start_request();
	uint8_t zdo_start_application();
	uint8_t set_callbacks(uint8_t cb);
	uint8_t set_startup_options(uint8_t opt);
	uint8_t set_tc_require_key_exchange(uint8_t bdb_trust_center_require_key_exchange);

	uint8_t znp_frame_parser(uint8_t* data, uint32_t len);

private:
#define ZNP_BUF_SIZE		128
	uint8_t m_znp_buf[ZNP_BUF_SIZE];

	Stream* m_znp_stream;
	uint8_t m_sequence_send;
	uint8_t get_sequence_send();

	uint8_t waiting_for_message(uint16_t cmd);
	uint8_t waiting_for_status(uint16_t cmd, uint8_t status);
	uint8_t get_msg_return(uint16_t cmd, uint8_t status, uint8_t* rx_buffer, uint32_t* len);

	uint8_t calc_fcs(uint8_t* msg, uint8_t len);
	uint8_t znp_frame_calc_fcs(zigbee_msg_t&);
};

#endif //__ZB_ZNP_H__
