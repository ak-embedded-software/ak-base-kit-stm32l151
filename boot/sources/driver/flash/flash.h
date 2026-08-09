#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define FLASH_DRIVER_OK			(0x00)
#define FLASH_DRIVER_NG			(0x01)

#define FLASH_SECTOR_SIZE			(0x1000)
#define FLASH_BLOCK_64K_SIZE		(0x10000)

extern uint8_t flash_read(uint32_t address, uint8_t* pbuf, uint32_t len);
extern uint8_t flash_write(uint32_t address, uint8_t* pbuf, uint32_t len);
extern uint8_t flash_erase_sector(uint32_t address);
extern uint8_t flash_erase_block_32k(uint32_t address);
extern uint8_t flash_erase_block_64k(uint32_t address);
extern uint8_t flash_erase_full();

#ifdef __cplusplus
}
#endif

#endif //__FLASH_H__
