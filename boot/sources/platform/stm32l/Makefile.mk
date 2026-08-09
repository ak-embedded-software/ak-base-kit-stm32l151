include sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/Makefile.mk
include sources/platform/stm32l/Libraries/CMSIS/Makefile.mk
include sources/platform/stm32l/arduino/Makefile.mk

LDFILE = sources/platform/stm32l/ak.ld

CFLAGS   += -I./sources/platform/stm32l
CPPFLAGS += -I./sources/platform/stm32l

VPATH += sources/platform/stm32l

# C source files
SOURCES += sources/platform/stm32l/stm32l.c
SOURCES += sources/platform/stm32l/system.c
SOURCES += sources/platform/stm32l/sys_cfg.c
SOURCES += sources/platform/stm32l/io_cfg.c
SOURCES += sources/platform/stm32l/system_stm32l1xx.c
