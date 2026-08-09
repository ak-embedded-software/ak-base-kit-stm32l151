CFLAGS		+= -I./sources/common/container
CPPFLAGS	+= -I./sources/common/container

VPATH += sources/common/container

SOURCES += sources/common/container/log_queue.c
SOURCES += sources/common/container/fifo.c
SOURCES += sources/common/container/ring_buffer.c
