#stm32 libs path
CPPFLAGS	+= -I./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/inc
CPPFLAGS	+= -I./sources/platform/stm32l/Libraries/CMSIS/Device/ST/STM32L1xx/Include
CPPFLAGS	+= -I./sources/platform/stm32l/Libraries/CMSIS/Include

CFLAGS		+= -I./sources/platform/stm32l/arduino/cores/stm32
CPPFLAGS	+= -I./sources/platform/stm32l/arduino/cores/stm32

VPATH += sources/platform/stm32l/arduino/cores/stm32

SOURCES += sources/platform/stm32l/arduino/cores/stm32/dtostrf.c
SOURCES += sources/platform/stm32l/arduino/cores/stm32/hooks.c
