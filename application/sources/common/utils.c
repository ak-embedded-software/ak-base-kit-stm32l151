/**
 ******************************************************************************
 * @author: GaoKong
 * @date:   13/08/2016
 ******************************************************************************
**/

#include "utils.h"
#include <stdint.h>

uint32_t str_len(const int8_t *str) {
	uint8_t length = 0;
	while (str[length] != 0) {
		length++;
	}
	return length;
}

void* mem_set(void *str, int c,size_t size) {
	uint8_t *ptr = (uint8_t *)str;
	const uint8_t ch = c;
	while(size--)
		*ptr++ = ch;
	return str;
}

void* mem_cpy(void *dst,const void *str, size_t size) {
	uint8_t *ptr = (uint8_t *)str;
	uint8_t *ptr2 = (uint8_t *)dst;
	while(size--)
		*ptr2++ = *ptr++;
	return dst;
}

int mem_cmp(const void * ptr1, const void * ptr2, size_t num) {
	uint8_t *pmem1 = (uint8_t *)ptr1;
	uint8_t *pmem2 = (uint8_t *)ptr2;

	while (*pmem1++ == *pmem2++) {
		if (num == 0) {
			break;
		}
		num--;
	}

	if (num != 0) {
		if (*pmem1 > *pmem2) {
			return 1;
		}
		else {
			return -1;
		}
	}

	return 0;
}

uint8_t mem_read(uint32_t address, uint8_t* data, uint32_t len) {
	uint8_t *src = (uint8_t *)address;
	uint8_t *dat = (uint8_t *)data;

	while (len--) {
		*dat++ = *src++;
	}

	return 0;
}

uint8_t mem_write(uint32_t address, uint8_t* data, uint32_t len) {
	uint8_t *dst = (uint8_t *)address;
	uint8_t *dat = (uint8_t *)data;

	while (len--) {
		*dst++ = *dat++;
	}

	return 0;
}

int32_t str_cmp(const int8_t *str1, const int8_t *str2) {
	while (*str1 == *str2) {
		if (*str1 == '\0' || *str2 == '\0')
			break;
		str1++;
		str2++;
	}

	if (*str1 == '\0' && *str2 == '\0')
		return 0;
	else
		return -1;
}

int8_t * str_cpy(int8_t * destination, const int8_t * source) {
	uint32_t len = 0;
	uint32_t index = 0;

	if (source == (int8_t*)0) {
		return (int8_t*)0;
	}

	if (destination == (int8_t*)0) {
		return (int8_t*)0;
	}

	len = str_len(source);

	for (index = 0; index <= len; index++) {
		*(destination + index) = *(source + index);
	}

	return destination;
}
