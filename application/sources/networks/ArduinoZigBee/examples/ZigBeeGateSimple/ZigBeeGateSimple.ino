#include <zb_znp.h>
#include <zb_zcl.h>
#include <SoftwareSerial.h>

/***
 * HƯỚNG DẪN SỬ DỤNG DEMO
 *
 * Sau khi nạp cho arduino
 * B1: Mở terminal của arduino, chỉnh baudrate về 115200
 * B2: Trên terminal, gửi 1 để yêu cầu cho phép thiết bị mới tham gia mạng.
 * B3: Nhấn giữ nút paring trên thiết bị/cảm biến cho đến khi đèn của thiết bị/cảm biến sáng lên thì nhả nút.
 * B4: Đợi vài giây đến khi nhận đc thông báo từ terminal, lúc này thông tin thiết bị/cảm biến sẽ show lên terminal
 *
 */

//#define DBG_ZB_FRAME

SoftwareSerial znp_serial(2, 3);
zb_znp zigbee_network(&znp_serial);

/* Biến xử lý điều khiển switch */
uint8_t control_switch_cmd_seq = 0;
uint16_t control_switch_address = 0;

int zb_znp::zigbee_message_handler(zigbee_msg_t& zigbee_msg) {
	/* zigbee start debug message */
	Serial.print("[ZB msg] len: ");
	Serial.print(zigbee_msg.len);
	Serial.print(" cmd0: ");
	Serial.print(zigbee_msg.cmd0, HEX);
	Serial.print(" cmd1: ");
	Serial.print(zigbee_msg.cmd1, HEX);
	Serial.print(" data: ");
	for (int i = 0; i < zigbee_msg.len; i++) {
		Serial.print(zigbee_msg.data[i], HEX);
		Serial.print(" ");
	}
	Serial.println("");
	/* zigbee stop debug message */

	uint16_t zigbee_cmd = BUILD_UINT16(zigbee_msg.cmd1, zigbee_msg.cmd0);

	switch(zigbee_cmd) {
	case AF_INCOMING_MSG: {
		afIncomingMSGPacket_t* st_af_incoming_msg = (afIncomingMSGPacket_t*)zigbee_msg.data;
		Serial.println("AF_INCOMING_MSG");

		switch (st_af_incoming_msg->cluster_id) {
		case ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY: {
			Serial.println("ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY");
			uint16_t retHum = (uint16_t) ((st_af_incoming_msg->payload[st_af_incoming_msg->len - 1] * 256) + \
					st_af_incoming_msg->payload[st_af_incoming_msg->len - 2]);

			// Ví dụ: retHum = 6789, thì giá trị trả về là 67,89 %
			Serial.print(retHum / 100); // Lấy Trước dấu phẩy -> 67
			Serial.print(",");
			Serial.println(retHum % 100); // Lấy sau dấu phẩy -> 89
		}
			break;

		case ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT: {
			Serial.println("ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT");
			uint16_t retTemp = (uint16_t) ((st_af_incoming_msg->payload[st_af_incoming_msg->len - 1] * 256) + \
					st_af_incoming_msg->payload[st_af_incoming_msg->len - 2]);

			// Ví dụ: retTemp = 2723, thì giá trị trả về là 27,23 *C
			Serial.print(retTemp/100); // Lấy Trước dấu phẩy -> 27
			Serial.print(",");
			Serial.println(retTemp%100); // Lấy sau dấu phẩy -> 23
		}
			break;

		case ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING: {
			Serial.println("ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING");
			uint8_t retOccu = st_af_incoming_msg->payload[st_af_incoming_msg->len - 1];
			Serial.println(retOccu);
		}
			break;

		case ZCL_CLUSTER_ID_GEN_ON_OFF:
			Serial.println("ZCL_CLUSTER_ID_GEN_ON_OFF");
			uint8_t retGenOnOff;
			if (st_af_incoming_msg->len > 9) {
				control_switch_address = st_af_incoming_msg->src_addr;
				retGenOnOff = st_af_incoming_msg->payload[st_af_incoming_msg->len - 8];
				Serial.println(retGenOnOff);
			}
			else {
				retGenOnOff = st_af_incoming_msg->payload[st_af_incoming_msg->len - 1];
				Serial.println(retGenOnOff);
			}
			break;

		default:
			break;
		}
	}
		break;
	}
}

void setup() {
	Serial.begin(115200);
	znp_serial.begin(115200);
	znp_serial.setTimeout(100);

	/* Khởi động coodinatior */
	Serial.println("\nstart_coordinator(1)");
	if (zigbee_network.start_coordinator(1) == 0) {
		Serial.println("OK");
	}
	else {
		Serial.println("NG");
	}
}

/* ký tự tạm để xử lý yêu cầu từ terminal */
char serial_cmd;

void loop() {
	/* hàm update() phải được gọi trong vòng lặp để xử lý các gói tin nhận được từ ZigBee Shield */
	zigbee_network.update();

	/* Kiểm tra / thực hiện các lệnh từ terminal */
	if (Serial.available()) {
		serial_cmd = Serial.read();

		switch(serial_cmd) {
			/* Cấu hình lại coodinator, đưa các cáu hình về mặc định.
			 * Chú ý: list thiết bị đã tham gia vào mạng trước đó sẽ bị mất */
		case '0': {
			Serial.println("\nstart_coordinator(1)");
			if (zigbee_network.start_coordinator(1) == 0) {
				Serial.println("OK");
			}
			else {
				Serial.println("NG");
			}
		}
			break;

			/* Cho phép thiết bị tham gia vào mạng */
		case '1': {
			Serial.println("set_permit_joining_req");
			/* ALL_ROUTER_AND_COORDINATOR -> cho phép thiết bị tham gia mạng từ Coodinator (ZigBee Shield)
			 * hoặc qua router (router thường là các thiết bị đc cấp điện, như ổ cắm, công tắc, bóng đèn ...
			 * 60, sau 60s nếu không có thiết bị tham gia mạng, coodinator sẽ trở về mode hoạt động bình thường
			 * người dùng muốn thêm thiết bị mới phải yêu cấu thêm lần nữa
			 * 1 , đợi thiết bị join thành công, mới thoát khỏi hàm, nếu 0, sự kiện có thiết bị mới tham gia mạng
			 * sẽ được nhận ở hàm callback int zb_znp::zigbee_message_handler(zigbee_msg_t& zigbee_msg)
			 */
			zigbee_network.set_permit_joining_req(ALL_ROUTER_AND_COORDINATOR, 60, 1);
		}
			break;

			/* yêu cầu Toggle công tắc */
		case '3': {
			Serial.println("TOOGLE Switch Req !\n");
			/*
			 * Frame Control, Transaction Sequence Number, Value control
			 * Value control -> 0x00: off, 0x01: on, 0x02: toogle
			*/
			if (control_switch_address) {
				uint8_t st_buffer[3] = { /* Frame control */ 0x01,
										 /* Transaction Sequence Number */0x00,  /* control_switch_cmd_seq++ */
										 /* Value Control */ 0x02}; /* Value Control [ 0x00:OFF , 0x01:ON , 0x02:TOOGLE ] */
				st_buffer[1] = control_switch_cmd_seq++;

				af_data_request_t st_af_data_request;
				st_af_data_request.cluster_id    = ZCL_CLUSTER_ID_PI_GENERIC_TUNNEL;
				st_af_data_request.dst_address   = control_switch_address;
				st_af_data_request.dst_endpoint  = 0x01;
				st_af_data_request.src_endpoint  = 0x01;
				st_af_data_request.trans_id      = 0x00;
				st_af_data_request.options       = 0x10;
				st_af_data_request.radius        = 0x0F;
				st_af_data_request.len           = sizeof(st_buffer);
				st_af_data_request.data          = st_buffer;

				zigbee_network.send_af_data_req(st_af_data_request);
			}
			else {
				Serial.println("Please join Switch !\n");
			}
		}
			break;

			/******************************************************************
			 *  Ví dụ:
			 * gửi data từ Gateway(coodinator) đến các thiết bị / cảm biến
			 * các thông số cần thiết cho quá trình này bao gồm
			 * 1. short address, là địa chỉ đc coodinator cấp khi thiết bị / cảm biến join vào mạng
			 * 2. độ dài của mảng data cần truyền
			 * 3. data

		case 's': {
			uint8_t st_buffer[10];
			af_data_request_t st_af_data_request;
			st_af_data_request.cluster_id    = ZCL_CLUSTER_ID_PI_GENERIC_TUNNEL;
			st_af_data_request.dst_address   = [ Địa chỉ đích của thiết bị / sensor ] ví du: control_switch_address
			st_af_data_request.dst_endpoint  = 0x01;
			st_af_data_request.src_endpoint  = 0x01;
			st_af_data_request.trans_id      = 0x00;
			st_af_data_request.options       = 0x10;
			st_af_data_request.radius        = 0x0F;
			st_af_data_request.len           = [ Độ dài data cần gửi đi ] ví dụ: sizeof(st_buffer)
			st_af_data_request.data          = [ data ] ví dụ: st_buffer
			zigbee_network.send_af_data_req(st_af_data_request);
		}
			break;
			********************************************************************/

		default:
			break;
		}
	}
}
