#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "zb_znp.h"
#include <Arduino.h>

#if defined(TASK_ZIGBEE_EN)
#include "xprintf.h"
#endif

uint8_t zb_znp::get_sequence_send() {
	if (m_sequence_send == 0xFF) {
		m_sequence_send = 0;
	} else {
		m_sequence_send++;
	}
	return m_sequence_send;
}

int zb_znp::update() {
	int _len = m_znp_stream->available();
	if (_len) {
		for (int i = 0; i < _len; i++) {
			m_znp_buf[i] = m_znp_stream->read();
		}
		znp_frame_parser(m_znp_buf, _len);
	}
	return _len;
}

int zb_znp::write(uint8_t* data, uint32_t len) {
	return (int)m_znp_stream->write(data, len);
}

int zb_znp::read(uint8_t* data, uint32_t len) {
	return (int)m_znp_stream->readBytes(data, len);
}

uint8_t zb_znp::set_tc_require_key_exchange(uint8_t bdb_trust_center_require_key_exchange) {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(APP_CNF_BDB_SET_TC_REQUIRE_KEY_EXCHANGE);
	i++;
	m_znp_buf[i] = LSB(APP_CNF_BDB_SET_TC_REQUIRE_KEY_EXCHANGE);
	i++;
	m_znp_buf[i] = bdb_trust_center_require_key_exchange;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(APP_CNF_BDB_SET_TC_REQUIRE_KEY_EXCHANGE_SRSP);
}

uint8_t zb_znp::znp_frame_parser(uint8_t* data, uint32_t len) {
	uint8_t ch, ret = ZNP_PARSER_IDLE;
	uint32_t rx_remain;

	while (len) {
		ch = *data++;
		len--;

		switch (znp_frame.state) {
		case SOP_STATE:
			if (SOF_CHAR == ch) {
				znp_frame.state = LEN_STATE;
				ret = ZNP_PARSER_DOING;
			}
			break;

		case LEN_STATE:
			ret = ZNP_PARSER_DOING;
			znp_frame.zigbee_msg.len = ch;
			znp_frame.zigbee_msg_data_index = 0;
			znp_frame.state = CMD0_STATE;
			break;

		case CMD0_STATE:
			ret = ZNP_PARSER_DOING;
			znp_frame.zigbee_msg.cmd0 = ch;
			znp_frame.state = CMD1_STATE;
			break;

		case CMD1_STATE:
			ret = ZNP_PARSER_DOING;
			znp_frame.zigbee_msg.cmd1 = ch;
			if (znp_frame.zigbee_msg.len) {
				znp_frame.state = DATA_STATE;
			} else {
				znp_frame.state = FCS_STATE;
			}
			break;

		case DATA_STATE: {
			ret = ZNP_PARSER_DOING;
			znp_frame.zigbee_msg.data[znp_frame.zigbee_msg_data_index++] = ch;

			rx_remain = znp_frame.zigbee_msg.len - znp_frame.zigbee_msg_data_index;

			if (len >= rx_remain) {
				memcpy((uint8_t*) (znp_frame.zigbee_msg.data + znp_frame.zigbee_msg_data_index), data, rx_remain);
				znp_frame.zigbee_msg_data_index += rx_remain;
				len -= rx_remain;
				data += rx_remain;
			} else {
				memcpy((uint8_t*) (znp_frame.zigbee_msg.data + znp_frame.zigbee_msg_data_index), data, len);
				znp_frame.zigbee_msg_data_index += len;
				len = 0;
			}

			if (znp_frame.zigbee_msg.len == znp_frame.zigbee_msg_data_index) {
				znp_frame.state = FCS_STATE;
			}
		}
			break;

		case FCS_STATE: {
			znp_frame.state = SOP_STATE;

			znp_frame.frame_fcs = ch;

			if (znp_frame.frame_fcs == znp_frame_calc_fcs(znp_frame.zigbee_msg)) {

				if (!znp_frame.zigbee_msg_denied_handle) {
					zigbee_message_handler(znp_frame.zigbee_msg);
				}

				ret = ZNP_PARSER_DONE;
			} else {
				/* TODO: handle checksum incorrect */
				ret = ZNP_PARSER_ERR;
			}
		}
			break;

		default:
			break;
		}
	}

	return ret;
}

uint8_t zb_znp::znp_frame_calc_fcs(zigbee_msg_t& zigbee_msg) {
	uint8_t i;
	uint8_t xorResult;
	xorResult = zigbee_msg.len ^ zigbee_msg.cmd0 ^ zigbee_msg.cmd1;
	for (i = 0; i < zigbee_msg.len; i++)
		xorResult = xorResult ^ zigbee_msg.data[i];
	return (xorResult);
}

uint8_t zb_znp::waiting_for_message(uint16_t cmd) {
	uint8_t _rx_buffer[ZNP_BUF_SIZE];
	int _rx_read_len;
	int _retry_time = 50;
	znp_frame.zigbee_msg_denied_handle = 1;
	while (_retry_time > 0) {
		_rx_read_len = read(_rx_buffer, ZNP_BUF_SIZE);
		if (znp_frame_parser(_rx_buffer, _rx_read_len) == ZNP_PARSER_DONE) {
			if (BUILD_UINT16(znp_frame.zigbee_msg.cmd1, znp_frame.zigbee_msg.cmd0) == cmd) {
				znp_frame.zigbee_msg_denied_handle = 0;
				return znp_frame.zigbee_msg.data[0];
			}
		}
		_retry_time--;
		//xprintf("waiting_for_message _retry_time: 0x%04X %d\n", cmd,  _retry_time);
	}
	znp_frame.zigbee_msg_denied_handle = 0;
	return ZNP_RET_NG;
}

uint8_t zb_znp::waiting_for_status(uint16_t cmd, uint8_t status) {
	uint8_t _rx_buffer[ZNP_BUF_SIZE];
	int _rx_read_len;
	int _retry_time = 50;
	znp_frame.zigbee_msg_denied_handle = 1;
	while (_retry_time > 0) {
		_rx_read_len = read(_rx_buffer, ZNP_BUF_SIZE);
		if (znp_frame_parser(_rx_buffer, _rx_read_len) == ZNP_PARSER_DONE) {
			if (BUILD_UINT16(znp_frame.zigbee_msg.cmd1, znp_frame.zigbee_msg.cmd0) == cmd &&
					znp_frame.zigbee_msg.data[0] == status) {
				znp_frame.zigbee_msg_denied_handle = 0;
				return znp_frame.zigbee_msg.data[0];
			}
		}
		_retry_time--;
		//xprintf("waiting_for_status _retry_time: 0x%04X %d\n", cmd,  _retry_time);
	}
	znp_frame.zigbee_msg_denied_handle = 0;
	return ZNP_RET_NG;
}

uint8_t zb_znp::get_msg_return(uint16_t cmd, uint8_t status, uint8_t* _rx_buffer, uint32_t* len) {
	int _rx_read_len;
	int _retry_time = 50;
	znp_frame.zigbee_msg_denied_handle = 1;
	while (_retry_time > 0) {
		_rx_read_len = read(_rx_buffer, ZNP_BUF_SIZE);
		if (znp_frame_parser(_rx_buffer, _rx_read_len) == ZNP_PARSER_DONE) {
			if (BUILD_UINT16(znp_frame.zigbee_msg.cmd1, znp_frame.zigbee_msg.cmd0) == cmd &&
					znp_frame.zigbee_msg.data[0] == status) {
				memcpy(_rx_buffer, znp_frame.zigbee_msg.data, znp_frame.zigbee_msg.len);
				*len = znp_frame.zigbee_msg.len;

				znp_frame.zigbee_msg_denied_handle = 0;
				return ZNP_RET_OK;
			}
		}
		_retry_time--;
		//xprintf("get_msg_return _retry_time: 0x%04X %d\n", cmd,  _retry_time);
	}
	znp_frame.zigbee_msg_denied_handle = 0;
	return ZNP_RET_NG;
}

uint8_t zb_znp::calc_fcs(uint8_t* msg, uint8_t len) {
	uint8_t result = 0;
	while (len--) {
		result ^= *msg++;
	}
	return result;
}

uint8_t zb_znp::app_cnf_set_allowrejoin_tc_policy(uint8_t mode) {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(APP_CNF_SET_ALLOWREJOIN_TC_POLICY);
	i++;
	m_znp_buf[i] = LSB(APP_CNF_SET_ALLOWREJOIN_TC_POLICY);
	i++;
	m_znp_buf[i] = mode;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;
	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(APP_CNF_SET_ALLOWREJOIN_TC_POLICY | 0x6000);
}

#define HARD_RESET				0x00
#define SOFT_RESET				0x01
uint8_t zb_znp::soft_reset() {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(SYS_RESET_REQ);
	i++;
	m_znp_buf[i] = LSB(SYS_RESET_REQ);
	i++;
	m_znp_buf[i] = SOFT_RESET;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(SYS_RESET_IND);
}

void zb_znp::hard_reset() {

}

uint8_t zb_znp::set_startup_options(uint8_t opt) {
	int8_t i = 0;
	if (opt > (STARTOPT_CLEAR_CONFIG + STARTOPT_CLEAR_STATE)) {
		return ZNP_RET_NG;
	}

	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = LSB(ZB_WRITE_CONFIGURATION);
	i++;

	m_znp_buf[i] = ZCD_NV_STARTUP_OPTION;
	i++;
	m_znp_buf[i] = ZCD_NV_STARTUP_OPTION_LEN;
	i++;
	m_znp_buf[i] = opt;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(ZB_WRITE_CONFIGURATION | 0x6000);
}

uint8_t zb_znp::set_panid(uint16_t pan_id) {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = LSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = ZCD_NV_PANID;
	i++;
	m_znp_buf[i] = ZCD_NV_PANID_LEN;
	i++;
	m_znp_buf[i] = LSB(pan_id);
	i++;
	m_znp_buf[i] = MSB(pan_id);
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;
	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}
	return waiting_for_message(ZB_WRITE_CONFIGURATION | 0x6000);
}

uint8_t zb_znp::set_zigbee_device_type(uint8_t dev_type) {
	uint8_t i = 0;
	if (dev_type > END_DEVICE) {
		return ZNP_RET_NG;
	}
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = LSB(ZB_WRITE_CONFIGURATION);
	i++;

	m_znp_buf[i] = ZCD_NV_LOGICAL_TYPE;
	i++;
	m_znp_buf[i] = ZCD_NV_LOGICAL_TYPE_LEN;
	i++;
	m_znp_buf[i] = dev_type;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;
	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(ZB_WRITE_CONFIGURATION | 0x6000);
}

uint8_t zb_znp::set_transmit_power(uint8_t tx_power_db) {
	uint8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(SYS_SET_TX_POWER);
	i++;
	m_znp_buf[i] = LSB(SYS_SET_TX_POWER);
	i++;
	m_znp_buf[i] = tx_power_db;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;
	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	uint8_t znpresult = waiting_for_message(SYS_SET_TX_POWER | 0x6000);
	if (znpresult) {
		return ZNP_RET_OK;
	}
	return ZNP_RET_NG;
}

uint8_t zb_znp::set_channel_mask(uint8_t primary, uint32_t channelmask) {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(APP_CNF_BDB_SET_CHANNEL);
	i++;
	m_znp_buf[i] = LSB(APP_CNF_BDB_SET_CHANNEL);
	i++;

	m_znp_buf[i] = primary;
	i++;
	m_znp_buf[i] = LSB(channelmask);
	i++;
	m_znp_buf[i] = (channelmask & 0xFF00) >> 8;
	i++;
	m_znp_buf[i] = (channelmask & 0xFF0000) >> 16;
	i++;
	m_znp_buf[i] = channelmask >> 24;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(APP_CNF_BDB_SET_CHANNEL | 0x6000);
}

uint8_t zb_znp::af_register_generic_application(uint8_t endpoint) {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(AF_REGISTER);
	i++;
	m_znp_buf[i] = LSB(AF_REGISTER);
	i++;
	m_znp_buf[i] = endpoint;
	i++;
	m_znp_buf[i] = LSB(DEFAULT_PROFILE_ID);
	i++;
	m_znp_buf[i] = MSB(DEFAULT_PROFILE_ID);
	i++;
	m_znp_buf[i] = LSB(DEVICE_ID);
	i++;
	m_znp_buf[i] = MSB(DEVICE_ID);
	i++;
	m_znp_buf[i] = DEVICE_VERSION;
	i++;
	m_znp_buf[i] = LATENCY_NORMAL;
	i++;
	m_znp_buf[i] = 0;
	i++;				// number of binding input clusters
	m_znp_buf[i] = 0;
	i++;				// number of binding output clusters
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(AF_REGISTER | 0x6000);
}

uint8_t zb_znp::zb_app_register_request() {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_APP_REGISTER_REQUEST);
	i++;
	m_znp_buf[i] = LSB(ZB_APP_REGISTER_REQUEST);
	i++;
	m_znp_buf[i] = DEFAULT_ENDPOINT;
	i++;
	m_znp_buf[i] = LSB(DEFAULT_PROFILE_ID);
	i++;
	m_znp_buf[i] = MSB(DEFAULT_PROFILE_ID);
	i++;
	m_znp_buf[i] = LSB(DEVICE_ID);
	i++;
	m_znp_buf[i] = MSB(DEVICE_ID);
	i++;
	m_znp_buf[i] = DEVICE_VERSION;
	i++;
	m_znp_buf[i] = LATENCY_NORMAL;
	i++;
	m_znp_buf[i] = 0;
	i++;				// number of binding input clusters
	m_znp_buf[i] = 0;
	i++;				// number of binding output clusters

	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(ZB_APP_REGISTER_REQUEST | 0x6000);
}

uint8_t zb_znp::zb_start_request() {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_START_REQUEST);
	i++;
	m_znp_buf[i] = LSB(ZB_START_REQUEST);
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(ZB_START_CONFIRM);
}

uint8_t zb_znp::zdo_start_application() {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZDO_STARTUP_FROM_APP);
	i++;
	m_znp_buf[i] = LSB(ZDO_STARTUP_FROM_APP);
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}
	return waiting_for_status(ZDO_STATE_CHANGE_IND, 0x09);
}

uint8_t zb_znp::set_callbacks(uint8_t cb) {
	if ((cb != CALLBACKS_ENABLED) && (cb != CALLBACKS_DISABLED)) {
		return ZNP_RET_NG;
	}
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = LSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = ZCD_NV_ZDO_DIRECT_CB;
	i++;
	m_znp_buf[i] = ZCD_NV_ZDO_DIRECT_CB_LEN;
	i++;
	m_znp_buf[i] = cb;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}
	return waiting_for_message(ZB_WRITE_CONFIGURATION | 0x6000);
}

uint8_t  zb_znp::start_coordinator(uint8_t opt) {
	uint8_t znpResult;

	//Serial.print("start_coordinator\n");

	hard_reset();

	znpResult = soft_reset();
	if (znpResult == ZNP_RET_NG) {
		//Serial.print("ERROR: reset ZNP \n");
		return znpResult;
	}
	//Serial.print("soft_reset\n");

	if (opt == 0) {
		//Serial.print("Skipping startup option !\n");
		for (int i = 1; i < 0x20; i++) {
			if (i == 0x01 || i == 0x0A) {
				znpResult = af_register_generic_application(i);
				if (znpResult != ZNP_RET_OK) {
					//Serial.print("ERROR: af register ");
					//Serial.print(i);
					//Serial.print("\n");
					return znpResult;
				}
			}
		}
		znpResult = bdb_start_commissioning(COMMISSIONING_MODE_STEERING, 1, 0);		// 0x02 is Network Steering
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: Network Steering \n");
			return znpResult;
		}

	} else {
		znpResult = set_startup_options(DEFAULT_STARTUP_OPTIONS);
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: startup option. \n");
			return znpResult;
		}
		//Serial.println("set_startup_options");

		znpResult = soft_reset();
		if (znpResult == ZNP_RET_NG) {
			//Serial.print("ERROR: reset ZNP \n");
			return znpResult;
		}
		//Serial.println("soft_reset");

		znpResult = set_panid((uint16_t) PAN_ID);
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: PAN ID \n");
			return znpResult;
		}
		//Serial.println("set_panid");

		znpResult = set_zigbee_device_type(COORDINATOR);
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: Device type \n");
			return znpResult;
		}
		//Serial.println("set_zigbee_device_type");

		//Set primary channel mask & disable secondary channel mask
		znpResult = set_channel_mask(CHANNEL_TRUE, (uint32_t) DEFAULT_CHANNEL_MASK);
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: set channel mask \n");
			return znpResult;
		}
		//Serial.println("set_channel_mask");

		znpResult = set_channel_mask(CHANNEL_FALSE, (uint32_t) 0);
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: set channel mask \n");
			return znpResult;
		}
		//Serial.println("set_channel_mask");

		znpResult = app_cnf_set_allowrejoin_tc_policy(1);
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: set allow join \n");
			return znpResult;
		}
		//Serial.println("app_cnf_set_allowrejoin_tc_policy");

		znpResult = set_transmit_power(DEFAULT_TX_POWER);
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: set transmit power \n");
			return znpResult;
		}
		//Serial.println("set_transmit_power");
		// Set ZCD_NV_ZDO_DIRECT_CB
		znpResult = set_callbacks(CALLBACKS_ENABLED);
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: set callback \n");
			return znpResult;
		}
		//Serial.println("set_callbacks");

		for (int i = 1; i < 0x20; i++) {
			if (i == 0x01 || i == 0x0A) {
				znpResult = af_register_generic_application(i);
				if (znpResult != ZNP_RET_OK) {
					//Serial.print("ERROR: af register ");
					//Serial.print(i);
					//Serial.print("\n");
					return znpResult;
				}
			}
		}
		//Serial.println("af_register_generic_application");
		// Start commissioning using network formation as parameter to start coordinator
		znpResult = bdb_start_commissioning(COMMISSIONING_MODE_INFORMATION, 2, 0);		// 0x04 is Network Formation
		if (znpResult != ZNP_RET_OK) {
			//Serial.print("ERROR: Network Steering \n");
			return znpResult;
		}
		//Serial.println("bdb_start_commissioning");
	}

	znpResult = set_tc_require_key_exchange(0);
	if (znpResult != ZNP_RET_OK) {
		//Serial.print("ERROR: set TC key exchange \n");
		return znpResult;
	}
	//Serial.println("set_tc_require_key_exchange");

	znpResult = set_permit_joining_req(SHORT_ADDRESS_COORDINATOR, DISABLE_PERMIT_JOIN, 0);
	if (znpResult != ZNP_RET_OK) {
		//Serial.print("ERROR: disabled permit join \n");
		return znpResult;
	}
	//Serial.println("set_permit_joining_req");

	/*get MAC address of coordinator*/
	get_mac_addr_req(SHORT_ADDRESS_COORDINATOR, 0x00, 0x00);
	//Serial.println("get_mac_addr_req");

	return ZNP_RET_OK;
}

uint8_t zb_znp::start_router(uint8_t opt) {
	uint8_t znpCmdExeResult;
	uint8_t zb_read_configuration_buf[ZNP_BUF_SIZE];
	int zb_read_configuration_buf_len;

	//Serial.print("start_router()\n");

	hard_reset();

	znpCmdExeResult = soft_reset();
	if (znpCmdExeResult == ZNP_RET_NG) {
		//Serial.print("[Start Router] ERR: soft_reset()\n");
		return znpCmdExeResult;
	}
	//Serial.print("[Start Router] OK: soft_reset()\n");

	znpCmdExeResult = zb_read_configuration(ZCD_NV_PANID, zb_read_configuration_buf, (uint32_t*) &zb_read_configuration_buf_len);
	if (znpCmdExeResult == ZNP_RET_OK) {
		//00 83 02 FF FF -> set config
		//00 83 02 F1 00 -> NOT set config
		//Serial.print("zb_read_configuration OK !!!\n");
	}
	else {
		//Serial.print("zb_read_configuration ERR !!!\n");
	}

	if (opt == 0 || (opt == 2 && zb_read_configuration_buf[3] == 0xF1)) {
		//Serial.print("Skipping startup option !\n");

		znpCmdExeResult = soft_reset();
		if (znpCmdExeResult == ZNP_RET_NG) {
			//Serial.print("[Start Router] ERR: soft_reset()\n");
			return znpCmdExeResult;
		}
		//Serial.print("[Start Router] OK: soft_reset()\n");

		for (int i = 1; i < 0x20; i++) {
			if (i == 0x01 || i == 0x0A) {
				znpCmdExeResult = af_register_generic_application(i);
				if (znpCmdExeResult != ZNP_RET_OK) {
					//Serial.print("[Start Router] ERR: af_register_generic_application\n");
					return znpCmdExeResult;
				}
			}
		}
		//Serial.print("[Start Router] OK: af_register_generic_application\n");

		znpCmdExeResult = bdb_start_commissioning(COMMISSIONING_MODE_STEERING, 1, 10);		// 0x02 is Network Steering

		if (znpCmdExeResult != ZNP_RET_OK) {
			//Serial.print("[Start Router] OK: bdb_start_commissioning\n");
			return znpCmdExeResult;
		}
	}
	else {
		znpCmdExeResult = soft_reset();
		if (znpCmdExeResult == ZNP_RET_NG) {
			//Serial.print("[Start Coodinator] ERR: soft_reset()\n");
			return znpCmdExeResult;
		}
		//Serial.print("[Start Coodinator] OK: soft_reset()\n");

		znpCmdExeResult = set_startup_options(DEFAULT_STARTUP_OPTIONS);
		if (znpCmdExeResult != ZNP_RET_OK) {
			//Serial.print("[Start Router] ERR: set_startup_options(DEFAULT_STARTUP_OPTIONS)\n");
			return znpCmdExeResult;
		}
		//Serial.print("[Start Router] OK: set_startup_options(DEFAULT_STARTUP_OPTIONS)\n");

		znpCmdExeResult = soft_reset();
		if (znpCmdExeResult == ZNP_RET_NG) {
			//Serial.print("[Start Router] ERR: set_reset()\n");
			return znpCmdExeResult;
		}
		//Serial.print("[Start Router] OK: set_reset()\n");

		znpCmdExeResult = set_zigbee_device_type(ROUTER);
		if (znpCmdExeResult != ZNP_RET_OK) {
			//Serial.print("[Start Router] ERR: set_zigbee_device_type(ROUTER)\n");
			return znpCmdExeResult;
		}
		//Serial.print("[Start Router] OK: set_zigbee_device_type(ROUTER)\n");

		znpCmdExeResult = soft_reset();
		if (znpCmdExeResult == ZNP_RET_NG) {
			//Serial.print("[Start Router] ERR: set_reset()\n");
			return znpCmdExeResult;
		}
		//Serial.print("[Start Router] OK: set_reset()\n");

		//Set primary channel mask & disable secondary channel mask
		znpCmdExeResult = set_channel_mask(CHANNEL_TRUE, (uint32_t) DEFAULT_CHANNEL_MASK);
		if (znpCmdExeResult != ZNP_RET_OK) {
			//Serial.print("[Start Router] ERR: set_channel_mask(CHANNEL_TRUE, (uint32_t) DEFAULT_CHANNEL_MASK)\n");
			return znpCmdExeResult;
		}
		//Serial.print("[Start Router] OK: set_channel_mask(CHANNEL_TRUE, (uint32_t) DEFAULT_CHANNEL_MASK)\n");

		znpCmdExeResult = set_channel_mask(CHANNEL_FALSE, (uint32_t) 0);
		if (znpCmdExeResult != ZNP_RET_OK) {
			//Serial.print("[Start Router] ERR: set_channel_mask(CHANNEL_FALSE, (uint32_t) 0)\n");
			return znpCmdExeResult;
		}
		//Serial.print("[Start Router] OK: set_channel_mask(CHANNEL_FALSE, (uint32_t) 0)\n");

		for (int i = 1; i < 0x20; i++) {
			if (i == 0x01 || i == 0x0A) {
				znpCmdExeResult = af_register_generic_application(i);
				if (znpCmdExeResult != ZNP_RET_OK) {
					return znpCmdExeResult;
				}
			}
		}
		//Serial.print("[Start Router] OK: af_register_generic_application()\n");

		znpCmdExeResult = bdb_start_commissioning(COMMISSIONING_MODE_STEERING, 1, 10);
		if (znpCmdExeResult != ZNP_RET_OK) {
			//Serial.print("[Start Router] ERR: bdb_start_commissioning(COMMISSIONING_MODE_STEERING, 1, 10)\n");
			return znpCmdExeResult;
		}
		//Serial.print("[Start Router] OK: bdb_start_commissioning(COMMISSIONING_MODE_STEERING, 1, 10)\n");
	}
	return ZNP_RET_OK;
}

uint8_t zb_znp::bdb_start_commissioning(uint8_t mode_config, uint8_t mode_receiving, uint8_t flag_waiting) {
	(void)mode_receiving;
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(APP_CNF_BDB_START_COMMISSIONING);
	i++;
	m_znp_buf[i] = LSB(APP_CNF_BDB_START_COMMISSIONING);
	i++;
	m_znp_buf[i] = mode_config;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	if (flag_waiting == 0) {
		return waiting_for_status(APP_CNF_BDB_COMMISSIONING_NOTIFICATION, ZNP_RET_OK);
	}
	return ZNP_RET_OK;
}

uint8_t zb_znp::util_get_device_info() {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(UTIL_GET_DEVICE_INFO);
	i++;
	m_znp_buf[i] = LSB(UTIL_GET_DEVICE_INFO);
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return ZNP_RET_OK;
}

uint8_t zb_znp::zdo_mgmt_leave_req(uint16_t short_add, uint8_t ieee_addr[8], uint8_t flags) {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZDO_MGMT_LEAVE_REQ);
	i++;
	m_znp_buf[i] = LSB(ZDO_MGMT_LEAVE_REQ);
	i++;
	m_znp_buf[i] = LSB(short_add);
	i++;
	m_znp_buf[i] = MSB(short_add);
	i++;
	memcpy((uint8_t*) &m_znp_buf[i], ieee_addr, 8);
	i += 8;
	m_znp_buf[i] = flags;
	i++;

	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return ZNP_RET_OK;
}

uint8_t zb_znp::set_permit_joining_req(uint16_t short_addr, uint8_t timeout, uint8_t flag_waiting) {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_PERMIT_JOINING_REQUEST);
	i++;
	m_znp_buf[i] = LSB(ZB_PERMIT_JOINING_REQUEST);
	i++;
	m_znp_buf[i] = LSB(short_addr);
	i++;
	m_znp_buf[i] = MSB(short_addr);
	i++;
	m_znp_buf[i] = timeout;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	if (flag_waiting == 0) {
		return waiting_for_message(ZDO_MGMT_PERMIT_JOIN_RSP);
	}

	return ZNP_RET_OK;
}

uint8_t zb_znp::get_mac_addr_req(uint16_t short_addr, uint8_t req_type, uint8_t start_index) {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZDO_IEEE_ADDR_REQ);
	i++;
	m_znp_buf[i] = LSB(ZDO_IEEE_ADDR_REQ);
	i++;
	m_znp_buf[i] = LSB(short_addr);
	i++;
	m_znp_buf[i] = MSB(short_addr);
	i++;
	m_znp_buf[i] = req_type;
	i++;
	m_znp_buf[i] = start_index;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return ZNP_RET_OK;
}

uint8_t zb_znp::send_af_data_req(af_data_request_t af_data_request) {
	int32_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(AF_DATA_REQUEST);
	i++;
	m_znp_buf[i] = LSB(AF_DATA_REQUEST);
	i++;
	m_znp_buf[i] = LSB(af_data_request.dst_address);
	i++;
	m_znp_buf[i] = MSB(af_data_request.dst_address);
	i++;
	m_znp_buf[i] = af_data_request.dst_endpoint;
	i++;
	m_znp_buf[i] = af_data_request.src_endpoint;
	i++;
	m_znp_buf[i] = MSB(af_data_request.cluster_id);
	i++;
	m_znp_buf[i] = LSB(af_data_request.cluster_id);
	i++;
	m_znp_buf[i] = af_data_request.trans_id;
	i++;
	m_znp_buf[i] = af_data_request.options;
	i++;
	m_znp_buf[i] = af_data_request.radius;
	i++;
	m_znp_buf[i] = af_data_request.len;
	i++;
	memcpy((uint8_t*) &m_znp_buf[i], af_data_request.data, af_data_request.len);
	if (m_znp_buf[i] & 0x04) {
		m_znp_buf[i + 3] = get_sequence_send();
	} else {
		m_znp_buf[i + 1] = get_sequence_send();
	}
	i += af_data_request.len;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return ZNP_RET_OK;
}

uint8_t zb_znp::set_security_mode(uint8_t security_mode) {
	if (security_mode > SECURITY_MODE_COORD_DIST_KEYS) {
		return ZNP_RET_NG;
	}
	int32_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = LSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = ZCD_NV_SECURITY_MODE;
	i++;
	m_znp_buf[i] = ZCD_NV_SECURITY_MODE_LEN;
	i++;
	m_znp_buf[i] = (security_mode > 0);
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	if (waiting_for_message(ZB_WRITE_CONFIGURATION | 0x6000) == ZNP_RET_NG) {
		return ZNP_RET_NG;
	}

	if (security_mode != SECURITY_MODE_OFF) {
		i = 0;
		m_znp_buf[i] = ZNP_SOF;
		i++;
		m_znp_buf[i] = 0;
		i++;
		m_znp_buf[i] = MSB(ZB_WRITE_CONFIGURATION);
		i++;
		m_znp_buf[i] = LSB(ZB_WRITE_CONFIGURATION);
		i++;
		m_znp_buf[i] = ZCD_NV_PRECFGKEYS_ENABLE;
		i++;
		m_znp_buf[i] = ZCD_NV_PRECFGKEYS_ENABLE_LEN;
		i++;
		m_znp_buf[i] = (security_mode == SECURITY_MODE_PRECONFIGURED_KEYS);
		i++;
		m_znp_buf[1] = i - 4;
		m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
		i++;

		if (write(m_znp_buf, i) < 0) {
			return ZNP_RET_NG;
		}

		return waiting_for_message(ZB_WRITE_CONFIGURATION | 0x6000);
	}
	return ZNP_RET_OK;
}

uint8_t zb_znp::set_security_key(uint8_t* key) {
	int32_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = LSB(ZB_WRITE_CONFIGURATION);
	i++;
	m_znp_buf[i] = ZCD_NV_PRECFGKEY;
	i++;
	m_znp_buf[i] = ZCD_NV_PRECFGKEY_LEN;
	i++;
	memcpy((uint8_t*) &m_znp_buf[i], key, ZCD_NV_PRECFGKEY_LEN);
	i += ZCD_NV_PRECFGKEY_LEN;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return waiting_for_message(ZB_WRITE_CONFIGURATION | 0x6000);
}

uint8_t zb_znp::zb_get_device_info(uint8_t param, uint8_t* rx_buffer, uint32_t* len) {
	uint32_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_GET_DEVICE_INFO);
	i++;
	m_znp_buf[i] = LSB(ZB_GET_DEVICE_INFO);
	i++;
	m_znp_buf[i] = param;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return get_msg_return((ZB_GET_DEVICE_INFO | 0x6000), param, rx_buffer, len);
}

uint8_t zb_znp::zb_read_configuration(uint8_t config_id, uint8_t* rx_buffer, uint32_t* len) {
	int8_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZB_READ_CONFIGURATION);
	i++;
	m_znp_buf[i] = LSB(ZB_READ_CONFIGURATION);
	i++;
	m_znp_buf[i] = config_id;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;
	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return get_msg_return(ZB_READ_CONFIGURATION_RSP, ZNP_RET_OK, rx_buffer, len);
}

uint8_t zb_znp::zdo_binding_req(binding_req_t binding_req) {
	int32_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZDO_BIND_REQ);
	i++;
	m_znp_buf[i] = LSB(ZDO_BIND_REQ);
	i++;
	m_znp_buf[i] = binding_req.src_short_addr[0];
	i++;
	m_znp_buf[i] = binding_req.src_short_addr[1];
	i++;
	memcpy((uint8_t*) &m_znp_buf[i], (uint8_t*) binding_req.src_ieee_addr, 8);
	i += 8;
	m_znp_buf[i] = binding_req.src_endpoint;
	i++;
	m_znp_buf[i] = LSB(binding_req.cluster_id);
	i++;
	m_znp_buf[i] = MSB(binding_req.cluster_id);
	i++;
	m_znp_buf[i] = binding_req.dst_mode;
	i++;
	if (binding_req.dst_mode == ADDRESS_64_BIT) {
		memcpy((uint8_t*) &m_znp_buf[i], (uint8_t*) binding_req.dst_address, 8);
		i += 8;
	} else {
		memcpy((uint8_t*) &m_znp_buf[i], (uint8_t*) binding_req.dst_address, 2);
		i += 2;
	}
	m_znp_buf[i] = binding_req.dst_endpoint;
	i++;

	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return ZNP_RET_OK;
}

uint8_t zb_znp::zdo_simple_desc_req(uint16_t dst_addr, uint8_t dst_enpoint) {
	int32_t i = 0;
	m_znp_buf[i] = ZNP_SOF;
	i++;
	m_znp_buf[i] = 0;
	i++;
	m_znp_buf[i] = MSB(ZDO_SIMPLE_DESC_REQ);
	i++;
	m_znp_buf[i] = LSB(ZDO_SIMPLE_DESC_REQ);
	i++;
	m_znp_buf[i] = LSB(dst_addr); //dst_addr
	i++;
	m_znp_buf[i] = MSB(dst_addr); // dst_addr
	i++;
	m_znp_buf[i] = LSB(dst_addr); // NWKAddrOfInterest
	i++;
	m_znp_buf[i] = MSB(dst_addr); //NWKAddrOfInterest
	i++;
	m_znp_buf[i] = dst_enpoint;
	i++;
	m_znp_buf[1] = i - 4;
	m_znp_buf[i] = calc_fcs((uint8_t *) &m_znp_buf[1], (i - 1));
	i++;

	if (write(m_znp_buf, i) < 0) {
		return ZNP_RET_NG;
	}

	return ZNP_RET_OK;
}
