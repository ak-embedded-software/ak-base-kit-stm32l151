CPPFLAGS	+= -I./sources/networks/rf_protocols/rf24/nwk
CFLAGS	+= -I./sources/networks/rf_protocols/rf24/nwk

VPATH += sources/networks/rf_protocols/rf24/nwk

SOURCES_CPP += sources/networks/rf_protocols/rf24/nwk/nrf_data.cpp
SOURCES_CPP += sources/networks/rf_protocols/rf24/nwk/nrf_mac.cpp
SOURCES_CPP += sources/networks/rf_protocols/rf24/nwk/nrf_nwk.cpp
SOURCES_CPP += sources/networks/rf_protocols/rf24/nwk/nrf_phy.cpp
