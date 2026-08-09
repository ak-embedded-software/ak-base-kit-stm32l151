include sources/app/screens/Makefile.mk
include sources/app/game/game_loneblade/Makefile.mk

CFLAGS		+= -I./sources/app
CFLAGS		+= -I./sources/app/game/game_loneblade
CPPFLAGS	+= -I./sources/app
CPPFLAGS	+= -I./sources/app/game/game_loneblade

VPATH += sources/app
VPATH += sources/app/game/game_loneblade

# CPP source files
SOURCES_CPP += sources/app/task_system.cpp
SOURCES_CPP += sources/app/app.cpp
SOURCES_CPP += sources/app/app_data.cpp
SOURCES_CPP += sources/app/app_non_clear_ram.cpp
SOURCES_CPP += sources/app/app_bsp.cpp
SOURCES_CPP += sources/app/app_modbus_pull.cpp
SOURCES_CPP += sources/app/shell.cpp

SOURCES_CPP += sources/app/task_shell.cpp
SOURCES_CPP += sources/app/task_life.cpp
SOURCES_CPP += sources/app/task_fw.cpp
SOURCES_CPP += sources/app/task_list.cpp
SOURCES_CPP += sources/app/task_display.cpp

ifeq ($(TASK_ZIGBEE_OPTION),-DTASK_ZIGBEE_EN)
SOURCES_CPP += sources/app/task_zigbee.cpp
endif

SOURCES_CPP += sources/app/task_if.cpp
SOURCES_CPP += sources/app/task_rf24_if.cpp
SOURCES_CPP += sources/app/task_uart_if.cpp
SOURCES_CPP += sources/app/task_dbg.cpp
