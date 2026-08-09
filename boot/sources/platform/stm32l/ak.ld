/**
 ******************************************************************************
 * @Author: GaoKong
 * @Date:   13/8/2016
 ******************************************************************************
**/

ENTRY	(reset_handler)

MEMORY {
	FLASH (rx)	: ORIGIN = 0x08000000, LENGTH = 8K
	BSF (rx)	: ORIGIN = 0x08002000, LENGTH = 4K
	SRAM (rwx)	: ORIGIN = 0x20000000, LENGTH = 16K
}

/* heap size */
HEAP_SIZE = 0K;

SECTIONS {
	. = ALIGN(4);

	.start_boot_share_data_flash : {
		_start_boot_share_data_flash = .;
	} > BSF

	/* start of FLASH */
	.start_flash : {
		_start_flash = .;
	} > FLASH

	.isr_vector : {
		KEEP(*(.isr_vector))
	} > FLASH

	.text : {
		*(.text)
		*(.text*)
		*(.rodata)
		*(.rodata*)
		*(.glue_7) /* glue arm to thumb */
		*(.glue_7t)/* glue thumb to arm */

		KEEP (*(.init))
		KEEP (*(.fini))

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
		*(.RamFunc)
		*(.RamFunc*)
		_edata = .;
	} > SRAM

	/* non clear RAM */
	.non_clear_ram : {
		_non_clear_ram = .;
		KEEP (*(.non_clear_ram))
		KEEP (*(.non_clear_ram*))
		_enon_clear_ram = .;
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

	/* end of SRAM */
	.end_ram : {
		_end_ram = . ;
	} > SRAM

	_estack = ORIGIN(SRAM) + LENGTH(SRAM);

	/DISCARD/ : {
		libc.a ( * )
		libm.a ( * )
		libg.a ( * )
		libgcc.a ( * )
	}
}

