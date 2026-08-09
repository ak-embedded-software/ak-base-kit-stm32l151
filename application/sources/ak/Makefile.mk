include sources/ak/ak.cfg.mk

CFLAGS += -I./sources/ak/inc
CPPFLAGS += -I./sources/ak/inc

VPATH += sources/ak/src

SOURCES += sources/ak/src/fsm.c
SOURCES += sources/ak/src/tsm.c
SOURCES += sources/ak/src/task.c
SOURCES += sources/ak/src/timer.c
SOURCES += sources/ak/src/message.c
