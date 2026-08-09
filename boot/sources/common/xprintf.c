/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   13/08/2016
 ******************************************************************************
**/
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include "xprintf.h"

#define CR_CRLF								(1)

void (*xfunc_out)(uint8_t );

void xputc(uint8_t c) {
	if (CR_CRLF && (c == (uint8_t)'\n')) {
		xfunc_out('\r');
	}
	if (xfunc_out) {
		xfunc_out(c);
	}
}

void xprintf(const char *fmt, ...) {
	va_list va_args;
	uint32_t num, digit;
	int32_t i;
	int32_t zero_padding = 0;
	uint32_t format_lenght = 0;
	int32_t base;
	int32_t minus;
	int8_t num_stack[11];
	uint8_t* ps;

	va_start(va_args, fmt);

	while (*fmt) {
		switch (*fmt) {
		case '%':
			zero_padding = 0;
			if (fmt[1] == '0') {
				zero_padding = 1;
				++fmt;
			}
			format_lenght = 0;
			while (*++fmt) {
				switch (*fmt) {
				case '%':
					xputc(*fmt);
					goto next_loop;

				case 'c':
					xputc(va_arg(va_args, int32_t));
					goto next_loop;

				case 'd':
				case 'X':
				case 'x':
					minus = 0;
					num = va_arg(va_args, uint32_t);
					if (*fmt == 'd') {
						base = 10;
						if (num & (uint32_t)0x80000000) {
							num = -(int32_t)num;
							minus = 1;
						}
					} else {
						base = 16;
					}
					for (digit = 0; digit < sizeof(num_stack);) {
						num_stack[digit++] = num%base;
						num /= base;
						if (num == 0) break;
					}
					if (minus) num_stack[digit++] = 0x7F;
					if (format_lenght > digit) {
						int8_t paddingint8_t = ' ';
						format_lenght -= digit;
						if (zero_padding)
							paddingint8_t = '0';
						while (format_lenght--) {
							xputc(paddingint8_t);
						}
					}
					for (i = digit-1; i >= 0; i--) {
						if (num_stack[i] == 0x7F) {
							xputc('-');
						} else if (num_stack[i] > 9) {
							xputc(num_stack[i]-10 + 'A');
						} else {
							xputc(num_stack[i] + '0');
						}
					}
					goto next_loop;

				case 's':
					ps = va_arg(va_args, uint8_t*);
					while(*ps) {
						xputc(*ps++);
					}
					goto next_loop;

				default:
					if (*fmt >= '0' && *fmt <= '9') {
						format_lenght = format_lenght*10 + (*fmt-'0');
					} else {
						goto exit;
					}
				}
			}

			if (*fmt == 0) {
				goto exit;
			}

			break;

		default:
			xputc(*fmt);
			break;
		}
next_loop:
		fmt++;
	}
exit:
	va_end(va_args);
}
