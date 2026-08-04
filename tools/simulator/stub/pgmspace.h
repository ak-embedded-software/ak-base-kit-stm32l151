#ifndef __PGMSPACE_H__
#define __PGMSPACE_H__
#define PROGMEM
#define PSTR(s) (s)
#define pgm_read_byte(a) (*(const unsigned char*)(a))
#define pgm_read_word(a) (*(const unsigned short*)(a))
#endif
