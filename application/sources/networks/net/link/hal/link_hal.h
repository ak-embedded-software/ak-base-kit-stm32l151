#ifndef __LINK_HAL_H__
#define __LINK_HAL_H__

#include <stdint.h>

#define LINK_HAL_HANDLED	1
#define LINK_HAL_IGNORED	0

typedef uint8_t (*pf_link_hal_write_byte)(uint8_t);
typedef uint8_t (*pf_link_hal_rev_byte)(uint8_t);

extern pf_link_hal_write_byte plink_hal_write_byte;
extern pf_link_hal_rev_byte plink_hal_rev_byte;

extern void link_hal_reg_write_byte(pf_link_hal_write_byte);
extern void link_hal_reg_rev_byte(pf_link_hal_rev_byte);

#endif //__LINK_HAL_H__
