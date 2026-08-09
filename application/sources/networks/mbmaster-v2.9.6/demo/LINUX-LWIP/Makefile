#
# MODBUS Library: Linux/lwIP port
#
# Copyright (c) 2014 Christian Walter
#
# $Id: Makefile,v 1.1 2014-08-23 11:48:22 embedded-solutions.cwalter Exp $
#
# ---------------------------------------------------------------------------

CC          = gcc
CXX         = g++
OBJCOPY	    = objcopy
SIZE	    = size
INSIGHT     = insight

LWIPDIR     = ./lwip/src
LWIPARCH    = ./lwip-port
INCLUDE     = -I. -Iport -I../../mbmaster/include \
	-I$(LWIPDIR)/include -I$(LWIPARCH)/include -I$(LWIPDIR)/include/ipv4 \
	-I$(LWIPDIR)

COREFILES=$(LWIPDIR)/core/mem.c $(LWIPDIR)/core/memp.c $(LWIPDIR)/core/netif.c \
	$(LWIPDIR)/core/pbuf.c $(LWIPDIR)/core/raw.c $(LWIPDIR)/core/stats.c \
	$(LWIPDIR)/core/sys.c $(LWIPDIR)/core/tcp.c $(LWIPDIR)/core/tcp_in.c \
	$(LWIPDIR)/core/tcp_out.c $(LWIPDIR)/core/udp.c $(LWIPDIR)/core/dhcp.c \
	$(LWIPDIR)/core/init.c $(LWIPDIR)/core/timers.c $(LWIPDIR)/core/def.c

CORE4FILES=$(wildcard $(LWIPDIR)/core/ipv4/*.c) $(LWIPDIR)/core/ipv4/inet.c

APIFILES=$(LWIPDIR)/api/api_lib.c $(LWIPDIR)/api/api_msg.c $(LWIPDIR)/api/tcpip.c \
	$(LWIPDIR)/api/err.c $(LWIPDIR)/api/sockets.c $(LWIPDIR)/api/netbuf.c $(LWIPDIR)/api/netdb.c

NETIFFILES=$(LWIPDIR)/netif/etharp.c $(LWIPDIR)/netif/slipif.c

ARCHFILES=$(wildcard $(LWIPARCH)/*.c $(LWIPARCH)/netif/tapif.c $(LWIPARCH)/netif/tunif.c $(LWIPARCH)/netif/unixif.c $(LWIPARCH)/netif/list.c $(LWIPARCH)/netif/tcpdump.c)

CFLAGS	    = -MD -g3 -Wall -pedantic -DIPv4 -DLWIP_DEBUG -DMBS_TCP_MAX_INSTANCES=2 -DMBP_TCP_DEBUG=1 -Dlinux -DNO_FREERTOS -DERRNO
CFLAGS      += $(INCLUDE)
ASFLAGS     = -MD 
LDFLAGS     = -lpthread -lrt

TGT         = demo-tcp
MB_SRC      = $(addprefix ../../mbmaster/, mbm.c functions/mbmfuncinput.c functions/mbmfuncholding.c common/mbutils.c tcp/mbmtcp.c ) \
	$(addprefix port/, mbporteventposix.c mbportother.c mbporttimerposix.c mbporttcp.c)

CSRC        = demo-tcp.c $(MB_SRC)  $(COREFILES) $(CORE4FILES) $(APIFILES) $(NETIFFILES) $(ARCHFILES)

ASRC        = 
OBJS        = $(CSRC:.c=.o) $(ASRC:.S=.o)
DEPS        = $(OBJS:.o=.d) 
BIN         = $(TGT)

.PHONY: clean all

all: clean $(BIN)

debug:
	$(INSIGHT) --se=$(TGT)

$(BIN): $(OBJS) $(NOLINK_OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

clean:
	rm -f $(DEPS)
	rm -f $(OBJS) $(NOLINK_OBJS)
	rm -f $(BIN) $(TGT).map

# ---------------------------------------------------------------------------
# rules for code generation
# ---------------------------------------------------------------------------
%.o:    %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o:    %.S
	$(CC) $(ASFLAGS) -o $@ -c $<

# ---------------------------------------------------------------------------
#  # compiler generated dependencies
# ---------------------------------------------------------------------------
-include $(DEPS)

