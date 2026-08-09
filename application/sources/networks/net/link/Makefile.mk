include sources/networks/net/link/hal/Makefile.mk

CPPFLAGS	+= -I./sources/networks/net/link

VPATH += sources/networks/net/link

SOURCES_CPP += sources/networks/net/link/link.cpp
SOURCES_CPP += sources/networks/net/link/link_mac.cpp
SOURCES_CPP += sources/networks/net/link/link_phy.cpp
SOURCES_CPP += sources/networks/net/link/link_data.cpp
