# header path
CPPFLAGS += -I./sources/platform/stm32l/arduino
CPPFLAGS += -I./sources/platform/stm32l/arduino/SPI
CPPFLAGS += -I./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/inc
CPPFLAGS += -I./sources/platform/stm32l/Libraries/CMSIS/Device/ST/STM32L1xx/Include
CPPFLAGS += -I./sources/platform/stm32l/Libraries/CMSIS/Include

CFLAGS   += -I./sources/platform/stm32l/arduino
CFLAGS   += -I./sources/platform/stm32l/arduino/SPI
CFLAGS   += -I./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/inc
CFLAGS   += -I./sources/platform/stm32l/Libraries/CMSIS/Device/ST/STM32L1xx/Include
CFLAGS   += -I./sources/platform/stm32l/Libraries/CMSIS/Include

# source path
VPATH += sources/platform/stm32l/arduino
VPATH += sources/platform/stm32l/arduino/SPI

# CPP source files
SOURCES_CPP += sources/platform/stm32l/arduino/SPI/SPI.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/wiring_digital.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/wiring_shift.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/Print.cpp
