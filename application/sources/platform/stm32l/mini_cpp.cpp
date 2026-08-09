#include <cstdlib>
#include <sys/types.h>
#include <stdio.h>
#include "sys_dbg.h"
#include "sys_ctrl.h"
#include "message.h"

/*
 * The default pulls in 70K of garbage
 */
namespace __gnu_cxx {

void __verbose_terminate_handler() {
	for (;;);
}

}

void*   __dso_handle = (void*) &__dso_handle;

/*
 * The default pulls in about 12K of garbage
 */
extern "C" void __cxa_pure_virtual() {
	FATAL("C++", 0x01);
}

extern "C" void __cxa_deleted_virtual() {
	FATAL("C++", 0x02);
}

/*
 * EABI builds can generate reams of stack unwind code for system generated exceptions
 * e.g. (divide-by-zero). Since we don't support exceptions we'll wrap out these
 * symbols and save a lot of flash space.
 */

extern "C" void __wrap___aeabi_unwind_cpp_pr0() {}
extern "C" void __wrap___aeabi_unwind_cpp_pr1() {}
extern "C" void __wrap___aeabi_unwind_cpp_pr2() {}

/*
 * Implement C++ new/delete operators using the heap
 */
void *operator new(size_t size) {
	return ak_malloc(size);
}

void *operator new[](size_t size) {
	return ak_malloc(size);
}

void operator delete(void *p) {
	ak_free(p);
}

void operator delete[](void *p) {
	ak_free(p);
}

/*
 * sbrk function for getting space for malloc and friends
 */

/* start heap region */
extern uint32_t __heap_start__;

extern "C" {
caddr_t _sbrk (uint32_t incr) {
	static uint8_t* heap = NULL;
	uint8_t* prev_heap;

	if (heap == NULL) {
		heap = (uint8_t*)((uint32_t)&__heap_start__);
	}

	prev_heap = heap;
	heap += incr;

	return (caddr_t) prev_heap;
}
} /* extern "C" */

extern "C" {
int fputc(int ch, FILE *f) {
	(void)f;
	sys_ctrl_shell_put_char(ch);
	/* Your implementation of fputc(). */
	return ch;
}
int ferror(FILE *f) {
	(void)f;
	/* Your implementation of ferror(). */
	return 0;
}
}
