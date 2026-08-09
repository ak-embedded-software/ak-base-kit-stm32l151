CPPFLAGS += -I./sources/platform/stm32l/arduino/libraries/Wire
CPPFLAGS += -I./sources/platform/stm32l/arduino/libraries/Wire/utility

VPATH += sources/platform/stm32l/arduino/libraries/Wire
VPATH += sources/platform/stm32l/arduino/libraries/Wire/utility

SOURCES_CPP += sources/platform/stm32l/arduino/libraries/Wire/Wire.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/libraries/Wire/utility/twi.cpp
