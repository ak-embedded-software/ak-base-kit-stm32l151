/* kernel include */
#include "ak.h"
#include "message.h"
#include "timer.h"
#include "fsm.h"

/* driver include */
#include "led.h"
#include "button.h"
#include "flash.h"

/* app include */
#include "app.h"
#include "app_dbg.h"
#include "app_bsp.h"
#include "app_flash.h"
#include "app_data.h"
#include "app_non_clear_ram.h"
#include "app_modbus_pull.h"

/* sys include */
#include "sys_io.h"
#include "sys_ctrl.h"


/*----------------------------------------------------------------------------*
 *  Device name: Cảm biến nhiệt độ, độ ẩm RS485 Modbus RTU ES35-SW (SHT35)
 *  Product link: https://epcb.vn/products/cam-bien-nhiet-do-do-am-rs485-modbus-rtu-es35-sw
 *  Note:
 *----------------------------------------------------------------------------*/
static MB_DevRegStruct_t Es35SW_Registers[] = {
	{ 0  , REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_REGISTERS, 0.1, (const int8_t*)"*C", INT16U10, true , (const int8_t*)"Temperature value"},
	{ 1  , REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_REGISTERS, 0.1, (const int8_t*)"RH", INT16U10, true , (const int8_t*)"Humidity value"   },
	{ 100, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_REGISTERS,   1, (const int8_t*)"NA", INT16U10, false, (const int8_t*)"Device address"   },
	{ 101, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_REGISTERS,   1, (const int8_t*)"NA", INT16U10, false, (const int8_t*)"Device baudrate"  },
};

MB_DeviceStruct_t MB_ES35SW_TH_Sensor = {
	.tId = 2,
	.tBaud = 9600,
	.listRegDevice = Es35SW_Registers,
	.listRegAmount = sizeof(Es35SW_Registers) / sizeof(Es35SW_Registers[0])
};

/*----------------------------------------------------------------------------*
 *  Device name: Relay 4 kênh IO giao tiếp RS485/RS232 công nghiệp LH-IO-01
 *  Product link: https://epcb.vn/products/relay-4-kenh-dau-ra-io-giao-tiep-rs485-rs232-cong-nghiep-lh-io-01
 *  Note:
 *----------------------------------------------------------------------------*/
static MB_DevRegStruct_t MB_LHIO404_IO_Device_Registers[] = {
	{ 1, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_COILS         , 1, (const int8_t*)"NA", INT8U, true, (const int8_t*)"Ouput 2 status"},
	{ 2, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_COILS         , 1, (const int8_t*)"NA", INT8U, true, (const int8_t*)"Ouput 3 status"},
	{ 0, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_COILS         , 1, (const int8_t*)"NA", INT8U, true, (const int8_t*)"Ouput 1 status"},
	{ 3, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_COILS         , 1, (const int8_t*)"NA", INT8U, true, (const int8_t*)"Ouput 4 status"},
	{ 0, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_DISCRETE_INPUT, 1, (const int8_t*)"NA", INT8U, true, (const int8_t*)"Input 1 status"},
	{ 1, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_DISCRETE_INPUT, 1, (const int8_t*)"NA", INT8U, true, (const int8_t*)"Input 2 status"},
	{ 2, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_DISCRETE_INPUT, 1, (const int8_t*)"NA", INT8U, true, (const int8_t*)"Input 3 status"},
	{ 3, REG_VAL_DEFAULT, MODBUS_FUNCTION_READ_DISCRETE_INPUT, 1, (const int8_t*)"NA", INT8U, true, (const int8_t*)"Input 4 status"},
};

MB_DeviceStruct_t MB_LHIO404_IO_Device = {
	.tId = 3,
	.tBaud = 9600,
	.listRegDevice = MB_LHIO404_IO_Device_Registers,
	.listRegAmount = sizeof(MB_LHIO404_IO_Device_Registers) / sizeof(MB_LHIO404_IO_Device_Registers[0])
};

/* Private functions prototypes -----------------------------------------------*/
static eMBErrorCode appMBMasterRead(UCHAR slAddr, uint8_t funCode, USHORT addReg, USHORT *buf);
#if 0
static eMBErrorCode appMBMasterWrite(UCHAR slAddr, uint8_t funCode, USHORT addReg, USHORT val);
#endif

/* Function implementation ---------------------------------------------------*/
void updateDataModbusDevice(MB_DeviceStruct_t *mbDevice) {
	uint8_t retryCount = 0;
	USHORT regVal;
	USHORT regAddr;
	uint8_t funCode;

	for (uint16_t regIndex = 0; regIndex < mbDevice->listRegAmount; ++regIndex) {
		funCode = mbDevice->listRegDevice[regIndex].funcCode;
		regAddr = mbDevice->listRegDevice[regIndex].regAddress;
		regVal = 0;

		eMBErrorCode errCode = appMBMasterRead(mbDevice->tId, funCode, regAddr, &regVal);

		if (errCode != MB_ENOERR) {
			mbDevice->listRegDevice[regIndex].regValue = REG_VAL_DEFAULT;
			++retryCount;
		}
		else {
			mbDevice->listRegDevice[regIndex].regValue = regVal;
		}

		if (retryCount > MB_READ_FAILED_RETRY_MAX) {
			break;
		}
	}
}

eMBErrorCode appMBMasterRead(UCHAR slAddr, uint8_t funCode, USHORT addReg, USHORT *buf) {
	eMBErrorCode errCodeRet = MB_ENOERR;

	switch (funCode) {
	case MODBUS_FUNCTION_READ_COILS: {
		errCodeRet = eMBMReadCoils(xMBMMaster, slAddr, addReg, 1, (UBYTE*)buf);
	}
		break;

	case MODBUS_FUNCTION_READ_DISCRETE_INPUT: {
		errCodeRet = eMBMReadDiscreteInputs(xMBMMaster, slAddr, addReg, 1, (UBYTE*)buf);
	}
		break;

	case MODBUS_FUNCTION_READ_REGISTERS: {
		errCodeRet = eMBMReadHoldingRegisters(xMBMMaster, slAddr, addReg, 1, buf);
	}
		break;

	case MODBUS_FUNCTION_READ_INPUT_REGISTER: {
		errCodeRet = eMBMReadInputRegisters(xMBMMaster, slAddr, addReg, 1, buf);
	}
		break;

	default:
		errCodeRet = MB_EINVAL;
		break;
	}

	return errCodeRet;
}

#if 0
eMBErrorCode appMBMasterWrite(UCHAR slAddr, uint8_t funCode, USHORT addReg, USHORT val) {
	eMBErrorCode errCodeRet = MB_ENOERR;

	switch (funCode) {
	case MODBUS_FUNCTION_WRITE_REGISTER: {
		errCodeRet = eMBMWriteSingleRegister(xMBMMaster, slAddr, addReg, val);
	}
		break;

	case MODBUS_FUNCTION_WRITE_COIL: {
		errCodeRet = eMBMWriteSingleCoil(xMBMMaster, slAddr, addReg, val);
	}

	case MODBUS_FUNCTION_WRITE_MULTIPLE_COILS: {
		
	}
		break;

	case MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS: {
		
	}
		break;

	default:
		errCodeRet = MB_EINVAL;
		break;
	}

	return errCodeRet;
}
#endif
