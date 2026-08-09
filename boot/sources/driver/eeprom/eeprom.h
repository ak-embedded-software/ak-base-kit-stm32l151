#ifndef __EEPROM_H__
#define __EEPROM_H__

#if defined (__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#define EEPROM_DRIVER_OK        (0x00)
#define EEPROM_DRIVER_NG        (0x01)

extern uint8_t  eeprom_read(uint32_t, uint8_t*, uint32_t);
extern uint8_t  eeprom_write(uint32_t, uint8_t*, uint32_t);
extern uint8_t  eeprom_erase(uint32_t address, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif //__EEPROM_H__
