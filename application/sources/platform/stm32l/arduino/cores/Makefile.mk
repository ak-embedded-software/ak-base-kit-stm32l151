include sources/platform/stm32l/arduino/cores/stm32/Makefile.mk

# header path
CPPFLAGS	+= -I./sources/platform/stm32l/arduino/cores

# sources path
VPATH += sources/platform/stm32l/arduino/cores

# cpp files
SOURCES_CPP += sources/platform/stm32l/arduino/cores/wiring_digital.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/cores/wiring_shift.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/cores/Print.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/cores/Stream.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/cores/WString.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/cores/WMath.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/cores/itoa.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/cores/IPAddress.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/cores/HardwareSerial.cpp
SOURCES_CPP += sources/platform/stm32l/arduino/cores/HardwareSerial2.cpp
