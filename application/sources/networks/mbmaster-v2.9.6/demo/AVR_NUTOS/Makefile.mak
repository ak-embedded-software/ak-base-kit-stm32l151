#
# MODBUS Library: Nut/OS port
#
# Copyright (c) 2009 Christian Walter <cwalter@embedded-solutions.at>
#
# $Id: Makefile.mak,v 1.1 2010-02-21 19:23:06 embedded-so.embedded-solutions.1 Exp $
#
# -------------------------

PROJ = demo

include nutos/Makedefs

MODBUS_SRCS = $(addprefix ../../mbmaster/, mbm.c functions/mbmfuncinput.c functions/mbmfuncholding.c common/mbutils.c rtu/mbmrtu.c rtu/mbmcrc.c ascii/mbmascii.c tcp/mbmtcp.c )
SRCS =  $(PROJ).c $(MODBUS_SRCS) port/mbportserial.c port/mbporttimer.c port/mbportevent.c port/mbportother.c
OBJS =  $(SRCS:.c=.o)
LIBS =  $(LIBDIR)/nutinit.o -lnutcrt -lnutarch -lnutdev -lnutos -lnutdev -lnutarch 
TARG =  $(PROJ).hex
INCLAST := $(INCLAST)  -Iport -I../../mbmaster/include 
all: $(OBJS) $(TARG) $(ITARG) $(DTARG)

include nutos/Makerules


reset:
	$(BURN) -cauto -datmega128 -j
	
clean:
	-rm -f $(OBJS)
	-rm -f $(TARG) $(ITARG) $(DTARG)
	-rm -f $(PROJ).eep
	-rm -f $(PROJ).obj
	-rm -f $(PROJ).map
	-rm -f $(PROJ).dbg
	-rm -f $(PROJ).cof
	-rm -f $(PROJ).mp
	-rm -f $(SRCS:.c=.lst)
	-rm -f $(SRCS:.c=.lis)
	-rm -f $(SRCS:.c=.s)
	-rm -f $(SRCS:.c=.bak)
	-rm -f $(SRCS:.c=.i)
