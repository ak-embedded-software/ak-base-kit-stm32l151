/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   13/8/2016
 ******************************************************************************
**/

ENTRY	(reset_handler)

MEMORY {
	BSF (rx)	: ORIGIN = 0x08002000, LENGTH = 4K
	FLASH (rx)	: ORIGIN = 0x08003000, LENGTH = 116K
	SRAM (rwx)	: ORIGIN = 0x20000000, LENGTH = 16K
}

/* heap size */
HEAP_SIZE = 2K;

SECTIONS {
	.start_boot_share_data_flash : {
		_start_boot_share_data_flash = .;
	} > BSF

	/* start of FLASH */
	.start_flash : {
		_start_flash = .;
	} > FLASH

	.isr_vector : {
		_isr_vector = .;
		KEEP(*(.isr_vector))
		. = ALIGN(4);
	} > FLASH

	.text : {
		. = ALIGN(4);
		*(.text)
		*(.text*)
		*(.rodata)
		*(.rodata*)
		*(.glue_7) /* glue arm to thumb */
		*(.glue_7t)/* glue thumb to arm */

		KEEP (*(.init))
		KEEP (*(.fini))

		. = ALIGN(4);
		_etext = .;
	} > FLASH

	/* constructor data for object C++ */

	.preinit_array : {
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(SORT(.preinit_array.*)))
		KEEP (*(.preinit_array*))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	} > FLASH

	.init_array : {
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);
	} > FLASH

	.fini_array : {
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(.fini_array*))
		KEEP (*(SORT(.fini_array.*)))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} > FLASH

	/* end of FLASH */
	.end_flash : {
		_end_flash = .;
	} > FLASH

	/* start SRAM */
	.start_ram : {
		_start_ram = . ;
	} > SRAM

	.data : AT(_end_flash) {
		_data = .;
		_ldata = LOADADDR (.data);
		*(vtable)
		*(.data)
		*(.data*)
		*(.ram_func)
		*(.ram_func*)
		_edata = .;
	} > SRAM

	.bss : {
		_bss = .;
		*(.bss)
		*(.bss*)
		*(COMMON)
		_ebss = .;
	} > SRAM

	.heap : {
		__heap_start__ = . ;
		. = . + HEAP_SIZE;
		. = ALIGN(4);
		__heap_end__ = . ;
	} > SRAM

	/* non clear RAM */
	.non_clear_ram (NOLOAD) : {
		_non_clear_ram = .;
		KEEP (*(.non_clear_ram))
		KEEP (*(.non_clear_ram*))
		_enon_clear_ram = .;
	} > SRAM  

	/* end of SRAM */
	.end_ram : {
		_end_ram = . ;
	} > SRAM

	.sstack : {
		_sstack = . ;
	} > SRAM

	_estack = ORIGIN(SRAM) + LENGTH(SRAM);

	/DISCARD/ : {
		libc.a ( * )
		libm.a ( * )
		libg.a ( * )
		libgcc.a ( * )
	}
}
