CFLAGS	+= -I./sources/networks/rf_protocols/rf24/hal
CPPFLAGS	+= -I./sources/networks/rf_protocols/rf24/hal

VPATH += sources/networks/rf_protocols/rf24/hal

SOURCES += sources/networks/rf_protocols/rf24/hal/hal_nrf_hw.c
SOURCES += sources/networks/rf_protocols/rf24/hal/hal_nrf_l01.c
