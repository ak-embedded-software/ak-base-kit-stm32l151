CFLAGS += -I./sources/platform/stm32l/Libraries/STM32_USB-FS-Device_Driver/inc
CPPFLAGS += -I./sources/platform/stm32l/Libraries/STM32_USB-FS-Device_Driver/inc

VPATH += sources/platform/stm32l/Libraries/STM32_USB-FS-Device_Driver/src

# C source files
SOURCES += sources/platform/stm32l/Libraries/STM32_USB-FS-Device_Driver/src/usb_core.c
SOURCES += sources/platform/stm32l/Libraries/STM32_USB-FS-Device_Driver/src/usb_init.c
SOURCES += sources/platform/stm32l/Libraries/STM32_USB-FS-Device_Driver/src/usb_int.c
SOURCES += sources/platform/stm32l/Libraries/STM32_USB-FS-Device_Driver/src/usb_mem.c
SOURCES += sources/platform/stm32l/Libraries/STM32_USB-FS-Device_Driver/src/usb_regs.c
SOURCES += sources/platform/stm32l/Libraries/STM32_USB-FS-Device_Driver/src/usb_sil.c
