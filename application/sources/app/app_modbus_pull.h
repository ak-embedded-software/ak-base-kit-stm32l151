#ifndef __APP_MODBUS_PULL_H
#define __APP_MODBUS_PULL_H

#include <stdint.h>

#include "port.h"

#include "mbport.h"
#include "mbm.h"
#include "mbtypes.h"
#include "mbportlayer.h"

/*----------------------------------------------------------------------------*
 *  DECLARE: Common definitions
 *  Note:
 *----------------------------------------------------------------------------*/
#define REG_VAL_DEFAULT             (0xFFFF)
#define MB_READ_FAILED_RETRY_MAX    ( 3 )

/* Enumarics -----------------------------------------------------------------*/
enum eMB_FuncCode {
	MODBUS_FUNCTION_NONE 						= 0,
	MODBUS_FUNCTION_READ_COILS 					= 1,
	MODBUS_FUNCTION_READ_DISCRETE_INPUT 		= 2,
	MODBUS_FUNCTION_READ_REGISTERS 				= 3,
	MODBUS_FUNCTION_READ_INPUT_REGISTER 		= 4,
	MODBUS_FUNCTION_WRITE_COIL 					= 5,
	MODBUS_FUNCTION_WRITE_REGISTER 				= 6,
	MODBUS_FUNCTION_WRITE_MULTIPLE_COILS 		= 15,
	MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS	= 16
};

enum eMB_DataType {
	UTF8 = 1,			/* Alphanumeric */
	INT16,				/* Signed Integer, 16 bits */
	INT16U,				/* Unsigned Integer, 16 bits */
	INT32L,				/* Signed Integer, 32 bits, Little Endian */
	INT32UL,			/* Unsigned Integer, 32 bits, Little Endian */
	INT64L,				/* Signed Integer, 64 bits, Little Endian */
	FLOAT32,			/* Floating Point, 32 bits */
	FLOAT64,			/* Floating Point, 64 bits */
	BITMAP,
	DATETIME,			/* DateTime */
	DATE,				/* Date */
	TIME,				/* Time */
	PORTAL,
	FQ_FP_PF,			/* Four Quadrant Floating Point Power Factor */
	INT16U10,			/* Unsigned Integer, 16 bits */
	INT16U256,			/* Unsigned Integer, 16 bits */
	INT32B,				/* Signed Integer, 32 bits, Big Endian */
	INT32UB,			/* Unsigned Integer, 32 bits, Big Endian */
	INT64UL,			/* Signed Integer, 64 bits, Little Endian */
	INT64B,				/* Signed Integer, 64 bits, Big Endian */
	INT64UB,			/* Unsigned Integer, 64 bits, Big Endian */
	INT16U100,			/* Unsigned Integer, 16 bits */
	INT16Ux10,			/* Unsigned Integer, 16 bits */
	INT16Ux100,			/* Unsigned Integer, 16 bits */
	INT16100,			/* Signed Integer, 16 bits */
	INT1610,			/* Signed Integer, 16 bits */
	INT8U,				/* Unsigned Integer, 8 bits */
	INT8U10,			/* Unsigned Integer, 8 bits */
	INT8U40,			/* Unsigned Integer, 8 bits */
	INT16UL10,			/* Unsigned Integer, 16 bits, Little Endian */
	INT32UL3600000,
	INT16U1000
};

/* Typedef -------------------------------------------------------------------*/
typedef struct RegisterDeviceStructure {
	USHORT regAddress;
	ULONG regValue;
	uint8_t funcCode;
	double ratio;
	const int8_t* unit;
	uint8_t typeData;
	bool isView;
	const int8_t* regDescribe;
} __AK_PACKETED MB_DevRegStruct_t;

typedef struct ModbusDeviceStructure {
	uint8_t tId;
	ULONG tBaud;
	MB_DevRegStruct_t *listRegDevice;
	uint16_t listRegAmount;
} __AK_PACKETED MB_DeviceStruct_t;

/* Extern variables ----------------------------------------------------------*/
extern MB_DeviceStruct_t MB_ES35SW_TH_Sensor; /* https://epcb.vn/products/cam-bien-nhiet-do-do-am-rs485-modbus-rtu-es35-sw */
extern MB_DeviceStruct_t MB_LHIO404_IO_Device; /* https://epcb.vn/products/relay-4-kenh-dau-ra-io-giao-tiep-rs485-rs232-cong-nghiep-lh-io-01 */

/* Function prototypes -------------------------------------------------------*/
extern void updateDataModbusDevice(MB_DeviceStruct_t *mbDevice);

#endif
