#ifndef __ZB_DEFS_H__
#define __ZB_DEFS_H__

#ifndef BV
#define BV(n)      (1 << (n))
#endif

#ifndef BF
#define BF(x,b,s)  (((x) & (b)) >> (s))
#endif

#ifndef MIN
#define MIN(n,m)   (((n) < (m)) ? (n) : (m))
#endif

#ifndef MAX
#define MAX(n,m)   (((n) < (m)) ? (m) : (n))
#endif

#ifndef ABS
#define ABS(n)     (((n) < 0) ? -(n) : (n))
#endif

#define BREAK_UINT64( var, ByteNum ) \
	(uint8_t)((uint64_t)(((var) >>((ByteNum) * 8)) & 0x00FF))

#define BUILD_UINT64(Byte0, Byte1, Byte2, Byte3, Byte4, Byte5, Byte6, Byte7) \
	((uint64_t)((uint64_t)((Byte0) & 0x00FF) \
	+ ((uint64_t)((Byte1) & 0x00FF) << 8) \
	+ ((uint64_t)((Byte2) & 0x00FF) << 16) \
	+ ((uint64_t)((Byte3) & 0x00FF) << 24) \
	+ ((uint64_t)((Byte4) & 0x00FF) << 32) \
	+ ((uint64_t)((Byte5) & 0x00FF) << 40) \
	+ ((uint64_t)((Byte6) & 0x00FF) << 48) \
	+ ((uint64_t)((Byte7) & 0x00FF) << 56)))

#define BREAK_UINT32( var, ByteNum ) \
	(uint8_t)((uint32_t)(((var) >>((ByteNum) * 8)) & 0x00FF))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
	((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
	+ ((uint32_t)((Byte1) & 0x00FF) << 8) \
	+ ((uint32_t)((Byte2) & 0x00FF) << 16) \
	+ ((uint32_t)((Byte3) & 0x00FF) << 24)))

#define LSB(num) ((num) & 0xFF)

#define MSB(num) ((num) >> 8)
#define BUILD_UINT16(loByte, hiByte) \
	((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

#define BUILD_UINT8(hiByte, loByte) \
	((uint8_t)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))

#define HI_UINT8(a) (((a) >> 4) & 0x0F)
#define LO_UINT8(a) ((a) & 0x0F)

#ifndef GET_BIT
#define GET_BIT(DISCS, IDX)  (((DISCS)[((IDX) / 8)] & BV((IDX) % 8)) ? TRUE : FALSE)
#endif
#ifndef SET_BIT
#define SET_BIT(DISCS, IDX)  (((DISCS)[((IDX) / 8)] |= BV((IDX) % 8)))
#endif
#ifndef CLR_BIT
#define CLR_BIT(DISCS, IDX)  (((DISCS)[((IDX) / 8)] &= (BV((IDX) % 8) ^ 0xFF)))
#endif

#define st(x)      do { x } while (__LINE__ == -1)

#define __ZB_PACKETED __attribute__((__packed__))

#endif //__ZB_DEFS_H__
