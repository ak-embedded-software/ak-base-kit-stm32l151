include sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/Makefile.mk
include sources/platform/stm32l/Libraries/CMSIS/Makefile.mk
include sources/platform/stm32l/arduino/Makefile.mk

LDFILE = sources/platform/stm32l/ak.ld

CFLAGS += -I./sources/platform/stm32l

CPPFLAGS += -I./sources/platform/stm32l

VPATH += sources/platform/stm32l

# C source files
SOURCES += sources/platform/stm32l/platform.c
SOURCES += sources/platform/stm32l/system.c
SOURCES += sources/platform/stm32l/sys_cfg.c
SOURCES += sources/platform/stm32l/io_cfg.c
SOURCES += sources/platform/stm32l/system_stm32l1xx.c

# C++ source files
SOURCES_CPP += sources/platform/stm32l/mini_cpp.cpp

# ASM source files
SOURCES_ASM += sources/platform/stm32l/sys_ctrl.s
