#
# ModbusMaster Libary: A portable MODBUS master for MODBUS ASCII/RTU/TCP/UDP.
# Copyright (c) 2007-2011 Christian Walter <wolti@sil.at>
# All rights reserved.
#
# $Id: Makefile,v 1.19 2014-08-23 09:45:00 embedded-solutions.cwalter Exp $
#

# --------------------------------------------------------------------------- #
# Toolchain setup
# --------------------------------------------------------------------------- #
CC        = gcc
LD        = ld
RM        = rm -f
FIND      = find
CUNIT     = /usr/local

# --------------------------------------------------------------------------- #
# Project paths  
# --------------------------------------------------------------------------- #
MBMASTER_DIR = ../mbmaster
PORT_DIR     = port
INC          = -I. \
			   -I$(CUNIT)/include/CUnit \
		  	   -I$(PORT_DIR) \
			   -I$(MBMASTER_DIR)/include 

# --------------------------------------------------------------------------- #
# This section includes compiler and linker settings.
# --------------------------------------------------------------------------- #
CPPFLAGS    = -g -O0 -fno-strict-aliasing
CPPFLAGS    += -Wall -Wcast-align -Wimplicit -Wpointer-arith -Wswitch
CPPFLAGS    += -Wredundant-decls -Wreturn-type -Wshadow -Wunused
CPPFLAGS    += -fprofile-arcs -ftest-coverage
CFLAGS      = $(CPPFLAGS) $(INC)
LDFLAGS     = -L/usr/local/lib -lcunit -lgcov -lpthread -lrt

GENDEPFLAGS = -MD -MP -MF .dep/$(@F).d
EXEEXT      = .exe

# --------------------------------------------------------------------------- #
# Make targets
# --------------------------------------------------------------------------- #

# all: ut_mbm ut_mbm_holding ut_mbm_rtu

# --------------------------------------------------------------------------- #
# Unit Test - Basic functions
# --------------------------------------------------------------------------- #
ut_mbm:
UT_MBM_CSRC   := port/mbporttimer.c port/mbportevent.c port/mbportother.c \
	ut_mbm_console.c ut_mbm_driver.c ut_mbm_common_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c $(MBMASTER_DIR)/functions/mbmfunccustom1.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_OBJ    := $(UT_MBM_CSRC:.c=.o) 
UT_MBM_TARGET := ut_mbm
ut_mbm        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2
ut_mbm        : $(UT_MBM_OBJ)
	$(CC) $(UT_MBM_OBJ) -o $(UT_MBM_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - Read/Write Holding Registers
# --------------------------------------------------------------------------- #
ut_mbm_holding: 
UT_MBM_HOLDING_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c \
	ut_mbm_console.c ut_mbm_holding_driver.c ut_mbm_common_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_HOLDING_OBJ    := $(UT_MBM_HOLDING_CSRC:.c=.o) 
UT_MBM_HOLDING_TARGET := ut_mbm_holding
ut_mbm_holding        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2
ut_mbm_holding        : $(UT_MBM_HOLDING_OBJ)
	$(CC) $(UT_MBM_HOLDING_OBJ) -o $(UT_MBM_HOLDING_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - Read Input Registers
# --------------------------------------------------------------------------- #
ut_mbm_input: 
UT_MBM_INPUT_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c \
	ut_mbm_console.c ut_mbm_input_driver.c ut_mbm_common_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncinput.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_INPUT_OBJ    := $(UT_MBM_INPUT_CSRC:.c=.o) 
UT_MBM_INPUT_TARGET := ut_mbm_input
ut_mbm_input        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2
ut_mbm_input        : $(UT_MBM_INPUT_OBJ)
	$(CC) $(UT_MBM_INPUT_OBJ) -o $(UT_MBM_INPUT_TARGET) $(LDFLAGS)


# --------------------------------------------------------------------------- #
# Unit Test - Read Discrete Inputs
# --------------------------------------------------------------------------- #
ut_mbm_discrete:
UT_MBM_DISCRETE_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c \
	ut_mbm_console.c ut_mbm_disc_driver.c ut_mbm_common_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncdisc.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_DISCRETE_OBJ    := $(UT_MBM_DISCRETE_CSRC:.c=.o) 
UT_MBM_DISCRETE_TARGET := ut_mbm_discrete
ut_mbm_discrete        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2
ut_mbm_discrete        : $(UT_MBM_DISCRETE_OBJ)
	$(CC) $(UT_MBM_DISCRETE_OBJ) -o $(UT_MBM_DISCRETE_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - Coils
# --------------------------------------------------------------------------- #
ut_mbm_coils:
UT_MBM_COILS_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c \
	ut_mbm_console.c ut_mbm_coils_driver.c ut_mbm_common_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfunccoils.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_COILS_OBJ    := $(UT_MBM_COILS_CSRC:.c=.o) 
UT_MBM_COILS_TARGET := ut_mbm_coils
ut_mbm_coils        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2
ut_mbm_coils        : $(UT_MBM_COILS_OBJ)
	$(CC) $(UT_MBM_COILS_OBJ) -o $(UT_MBM_COILS_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - Files
# --------------------------------------------------------------------------- #
ut_mbm_files:
UT_MBM_FILES_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c \
	ut_mbm_console.c ut_mbm_files_driver.c ut_mbm_common_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncfiles.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_FILES_OBJ    := $(UT_MBM_FILES_CSRC:.c=.o) 
UT_MBM_FILES_TARGET := ut_mbm_files
ut_mbm_files        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2
ut_mbm_files        : $(UT_MBM_FILES_OBJ)
	$(CC) $(UT_MBM_FILES_OBJ) -o $(UT_MBM_FILES_TARGET) $(LDFLAGS)
# --------------------------------------------------------------------------- #
# Unit Test - RTU layer      
# --------------------------------------------------------------------------- #
ut_mbm_rtu:
UT_MBM_RTU_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_rtu_driver.c ut_mbm_rtu_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/rtu/mbmrtu.c $(MBMASTER_DIR)/rtu/mbmcrc.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_RTU_OBJ    := $(UT_MBM_RTU_CSRC:.c=.o) 
UT_MBM_RTU_TARGET := ut_mbm_rtu
ut_mbm_rtu        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=1 -DMBM_TCP_ENABLED=0 -DMBM_TEST_DISABLE_TIMEOUTS=1 -DMBM_TEST_INSTANCES=2 -DMBM_TEST_DISABLE_RTU_TIMEOUTS=1
ut_mbm_rtu        : $(UT_MBM_RTU_OBJ)
	$(CC) $(UT_MBM_RTU_OBJ) -o $(UT_MBM_RTU_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - RTU layer statistic     
# --------------------------------------------------------------------------- #
ut_mbm_rtu_stat:
UT_MBM_RTU_STAT_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_rtu_stat_driver.c ut_mbm_rtu_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/rtu/mbmrtu.c $(MBMASTER_DIR)/rtu/mbmcrc.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_RTU_STAT_OBJ    := $(UT_MBM_RTU_STAT_CSRC:.c=.o) 
UT_MBM_RTU_STAT_TARGET := ut_mbm_rtu_stat
ut_mbm_rtu_stat   : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=1 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2 -DMBM_TEST_DISABLE_RTU_TIMEOUTS=0 -DMBM_ENABLE_STATISTICS_INTERFACE=1
ut_mbm_rtu_stat   : $(UT_MBM_RTU_STAT_OBJ)
	$(CC) $(UT_MBM_RTU_STAT_OBJ) -o $(UT_MBM_RTU_STAT_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - RTU layer protocol analyzer     
# --------------------------------------------------------------------------- #
ut_mbm_rtu_analyzer:
UT_MBM_RTU_ANALYZER_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_rtu_analyzer_driver.c ut_mbm_rtu_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/rtu/mbmrtu.c $(MBMASTER_DIR)/rtu/mbmcrc.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_RTU_ANALYZER_OBJ    := $(UT_MBM_RTU_ANALYZER_CSRC:.c=.o) 
UT_MBM_RTU_ANALYZER_TARGET := ut_mbm_rtu_analyzer
ut_mbm_rtu_analyzer   : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=1 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2 -DMBM_TEST_DISABLE_RTU_TIMEOUTS=0 -DMBM_ENABLE_PROT_ANALYZER_INTERFACE=1
ut_mbm_rtu_analyzer   : $(UT_MBM_RTU_ANALYZER_OBJ)
	$(CC) $(UT_MBM_RTU_ANALYZER_OBJ) -o $(UT_MBM_RTU_ANALYZER_TARGET) $(LDFLAGS)
    
# --------------------------------------------------------------------------- #
# Unit Test - RTU layer V2 API      
# --------------------------------------------------------------------------- #
ut_mbm_rtu_v2:
UT_MBM_RTU_V2_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_rtu_v2_driver.c ut_mbm_rtu_v2_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/rtu/mbmrtu.c $(MBMASTER_DIR)/rtu/mbmcrc.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_RTU_V2_OBJ    := $(UT_MBM_RTU_V2_CSRC:.c=.o) 
UT_MBM_RTU_V2_TARGET := ut_mbm_rtu_v2
ut_mbm_rtu_v2        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=1 -DMBM_TCP_ENABLED=0 -DMBM_TEST_DISABLE_TIMEOUTS=1 -DMBM_TEST_INSTANCES=2 -DMBM_TEST_DISABLE_RTU_TIMEOUTS=1 -DMBM_SERIAL_API_VERSION=2
ut_mbm_rtu_v2        : $(UT_MBM_RTU_V2_OBJ)
	$(CC) $(UT_MBM_RTU_V2_OBJ) -o $(UT_MBM_RTU_V2_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - RTU layer V2 API statistic    
# --------------------------------------------------------------------------- #
ut_mbm_rtu_v2_stat:
UT_MBM_RTU_V2_STAT_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_rtu_stat_driver.c ut_mbm_rtu_v2_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/rtu/mbmrtu.c $(MBMASTER_DIR)/rtu/mbmcrc.c \
	$(MBMASTER_DIR)/common/mbutils.c
UT_MBM_RTU_V2_STAT_OBJ    := $(UT_MBM_RTU_V2_STAT_CSRC:.c=.o) 
UT_MBM_RTU_V2_STAT_TARGET := ut_mbm_rtu_v2_stat
ut_mbm_rtu_v2_stat        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=1 -DMBM_TCP_ENABLED=0 -DMBM_TEST_DISABLE_TIMEOUTS=0 -DMBM_TEST_INSTANCES=2 -DMBM_TEST_DISABLE_RTU_TIMEOUTS=1 -DMBM_SERIAL_API_VERSION=2 -DMBM_ENABLE_STATISTICS_INTERFACE=1
ut_mbm_rtu_v2_stat        : $(UT_MBM_RTU_V2_STAT_OBJ)
	$(CC) $(UT_MBM_RTU_V2_STAT_OBJ) -o $(UT_MBM_RTU_V2_STAT_TARGET) $(LDFLAGS)
    
# --------------------------------------------------------------------------- #
# Unit Test - ASCII layer      
# --------------------------------------------------------------------------- #
ut_mbm_ascii:
UT_MBM_ASCII_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_ascii_driver.c ut_mbm_ascii_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/ascii/mbmascii.c $(MBMASTER_DIR)/common/mbutils.c
UT_MBM_ASCII_OBJ    := $(UT_MBM_ASCII_CSRC:.c=.o) 
UT_MBM_ASCII_TARGET := ut_mbm_ascii
ut_mbm_ascii        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=1 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_DISABLE_TIMEOUTS=1 -DMBM_TEST_INSTANCES=2
ut_mbm_ascii        : $(UT_MBM_ASCII_OBJ)
	$(CC) $(UT_MBM_ASCII_OBJ) -o $(UT_MBM_ASCII_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - ASCII layer statistic 
# --------------------------------------------------------------------------- #
ut_mbm_ascii_stat:
UT_MBM_ASCII_STAT_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_ascii_stat_driver.c ut_mbm_ascii_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/ascii/mbmascii.c $(MBMASTER_DIR)/common/mbutils.c
UT_MBM_ASCII_STAT_OBJ    := $(UT_MBM_ASCII_STAT_CSRC:.c=.o) 
UT_MBM_ASCII_STAT_TARGET := ut_mbm_ascii_stat
ut_mbm_ascii_stat        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=1 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2 -DMBM_ENABLE_STATISTICS_INTERFACE=1
ut_mbm_ascii_stat        : $(UT_MBM_ASCII_STAT_OBJ)
	$(CC) $(UT_MBM_ASCII_STAT_OBJ) -o $(UT_MBM_ASCII_STAT_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - ASCII layer protocol analyzer 
# --------------------------------------------------------------------------- #
ut_mbm_ascii_analyzer:
UT_MBM_ASCII_ANALYZER_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_ascii_analyzer_driver.c ut_mbm_ascii_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/ascii/mbmascii.c $(MBMASTER_DIR)/common/mbutils.c
UT_MBM_ASCII_ANALYZER_OBJ    := $(UT_MBM_ASCII_ANALYZER_CSRC:.c=.o) 
UT_MBM_ASCII_ANALYZER_TARGET := ut_mbm_ascii_analyzer
ut_mbm_ascii_analyzer        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=1 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_INSTANCES=2 -DMBM_TEST_DISABLE_TIMEOUTS=1 -DMBM_ENABLE_PROT_ANALYZER_INTERFACE=1
ut_mbm_ascii_analyzer        : $(UT_MBM_ASCII_ANALYZER_OBJ)
	$(CC) $(UT_MBM_ASCII_ANALYZER_OBJ) -o $(UT_MBM_ASCII_ANALYZER_TARGET) $(LDFLAGS)
    
# --------------------------------------------------------------------------- #
# Unit Test - ASCII layer V2 API     
# --------------------------------------------------------------------------- #
ut_mbm_ascii_v2:
UT_MBM_ASCII_V2_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_ascii_v2_driver.c ut_mbm_ascii_v2_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/ascii/mbmascii.c $(MBMASTER_DIR)/common/mbutils.c
UT_MBM_ASCII_V2_OBJ    := $(UT_MBM_ASCII_V2_CSRC:.c=.o) 
UT_MBM_ASCII_V2_TARGET := ut_mbm_ascii_v2
ut_mbm_ascii_v2        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=1 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_DISABLE_TIMEOUTS=1 -DMBM_TEST_INSTANCES=2 -DMBM_SERIAL_API_VERSION=2
ut_mbm_ascii_v2        : $(UT_MBM_ASCII_V2_OBJ)
	$(CC) $(UT_MBM_ASCII_V2_OBJ) -o $(UT_MBM_ASCII_V2_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - ASCII layer V2 API statistic     
# --------------------------------------------------------------------------- #
ut_mbm_ascii_v2_stat:
UT_MBM_ASCII_V2_STAT_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c port/mbportserial.c \
	ut_mbm_console.c ut_mbm_ascii_stat_driver.c ut_mbm_ascii_v2_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
	$(MBMASTER_DIR)/ascii/mbmascii.c $(MBMASTER_DIR)/common/mbutils.c
UT_MBM_ASCII_V2_STAT_OBJ    := $(UT_MBM_ASCII_V2_STAT_CSRC:.c=.o) 
UT_MBM_ASCII_V2_STAT_TARGET := ut_mbm_ascii_v2_stat
ut_mbm_ascii_v2_stat        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=1 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_TEST_DISABLE_TIMEOUTS=0 -DMBM_TEST_INSTANCES=2 -DMBM_SERIAL_API_VERSION=2 -DMBM_ENABLE_STATISTICS_INTERFACE=1
ut_mbm_ascii_v2_stat        : $(UT_MBM_ASCII_V2_STAT_OBJ)
	$(CC) $(UT_MBM_ASCII_V2_STAT_OBJ) -o $(UT_MBM_ASCII_V2_STAT_TARGET) $(LDFLAGS)
    
# --------------------------------------------------------------------------- #
# Unit Test - TCP layer     
# --------------------------------------------------------------------------- #
ut_mbm_tcp:
UT_MBM_TCP_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c \
	ut_mbm_console.c ut_mbm_tcp_driver.c ut_mbm_tcp_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
        $(MBMASTER_DIR)/functions/mbmfuncinput.c \
	$(MBMASTER_DIR)/tcp/mbmtcp.c $(MBMASTER_DIR)/common/mbutils.c
UT_MBM_TCP_OBJ    := $(UT_MBM_TCP_CSRC:.c=.o) 
UT_MBM_TCP_TARGET := ut_mbm_tcp
ut_mbm_tcp        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=1 -DMBM_TEST_DISABLE_TIMEOUTS=1 -DMBM_TEST_INSTANCES=2
ut_mbm_tcp        : $(UT_MBM_TCP_OBJ)
	$(CC) $(UT_MBM_TCP_OBJ) -o $(UT_MBM_TCP_TARGET) $(LDFLAGS)
    
# --------------------------------------------------------------------------- #
# Unit Test - TCP layer statistics
# --------------------------------------------------------------------------- #
ut_mbm_tcp_stat:
UT_MBM_TCP_STAT_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c \
	ut_mbm_console.c ut_mbm_tcp_stat_driver.c ut_mbm_tcp_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
        $(MBMASTER_DIR)/functions/mbmfuncinput.c \
	$(MBMASTER_DIR)/tcp/mbmtcp.c $(MBMASTER_DIR)/common/mbutils.c
UT_MBM_TCP_STAT_OBJ    := $(UT_MBM_TCP_STAT_CSRC:.c=.o) 
UT_MBM_TCP_STAT_TARGET := ut_mbm_tcp_stat
ut_mbm_tcp_stat        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=1 -DMBM_TEST_DISABLE_TIMEOUTS=0 -DMBM_TEST_INSTANCES=2 -DMBM_ENABLE_STATISTICS_INTERFACE=1
ut_mbm_tcp_stat       : $(UT_MBM_TCP_STAT_OBJ)
	$(CC) $(UT_MBM_TCP_STAT_OBJ) -o $(UT_MBM_TCP_STAT_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - TCP layer analyzer
# --------------------------------------------------------------------------- #
ut_mbm_tcp_analyzer:
UT_MBM_TCP_ANALYZER_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c \
	ut_mbm_console.c ut_mbm_tcp_analyzer_driver.c ut_mbm_tcp_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
        $(MBMASTER_DIR)/functions/mbmfuncinput.c \
	$(MBMASTER_DIR)/tcp/mbmtcp.c $(MBMASTER_DIR)/common/mbutils.c
UT_MBM_TCP_ANALYZER_OBJ    := $(UT_MBM_TCP_ANALYZER_CSRC:.c=.o) 
UT_MBM_TCP_ANALYZER_TARGET := ut_mbm_tcp_analyzer
ut_mbm_tcp_analyzer    : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=1 -DMBM_TEST_DISABLE_TIMEOUTS=1 -DMBM_TEST_INSTANCES=2 -DMBM_ENABLE_PROT_ANALYZER_INTERFACE=1
ut_mbm_tcp_analyzer    : $(UT_MBM_TCP_ANALYZER_OBJ)
	$(CC) $(UT_MBM_TCP_ANALYZER_OBJ) -o $(UT_MBM_TCP_ANALYZER_TARGET) $(LDFLAGS)

# --------------------------------------------------------------------------- #
# Unit Test - UDP layer     
# --------------------------------------------------------------------------- #
ut_mbm_udp:
UT_MBM_UDP_CSRC   := port/mbporttimer.c port/mbportevent.c \
	port/mbportother.c \
	ut_mbm_console.c ut_mbm_udp_driver.c ut_mbm_udp_stubs.c \
	$(MBMASTER_DIR)/mbm.c $(MBMASTER_DIR)/functions/mbmfuncholding.c \
        $(MBMASTER_DIR)/functions/mbmfuncinput.c \
	$(MBMASTER_DIR)/udp/mbmudp.c $(MBMASTER_DIR)/common/mbutils.c
UT_MBM_UDP_OBJ    := $(UT_MBM_UDP_CSRC:.c=.o) 
UT_MBM_UDP_TARGET := ut_mbm_udp
ut_mbm_udp        : CFLAGS:=$(CFLAGS) -DMBM_ASCII_ENABLED=0 -DMBM_RTU_ENABLED=0 -DMBM_TCP_ENABLED=0 -DMBM_UDP_ENABLED=1 -DMBM_TEST_DISABLE_TIMEOUTS=0 -DMBM_TEST_INSTANCES=2
ut_mbm_udp        : $(UT_MBM_UDP_OBJ)
	$(CC) $(UT_MBM_UDP_OBJ) -o $(UT_MBM_UDP_TARGET) $(LDFLAGS)
	
# --------------------------------------------------------------------------- #
# Make rules  
# --------------------------------------------------------------------------- #

# Compile: create object files from C source files. 
$(COBJ) : %.o : %.c
	$(CC) -c $(CFLAGS) $(GENDEPFLAGS) $< -o $@ 

# Target: clean project.
clean: 
	rm -f .dep/*
	$(RM) -f $(UT_MBM_TARGET)$(EXEEXT) $(UT_MBM_OBJ)
	$(RM) -f $(UT_MBM_HOLDING_TARGET)$(EXEEXT) $(UT_MBM_HOLDING_OBJ)
	$(RM) -f $(UT_MBM_INPUT_TARGET)$(EXEEXT) $(UT_MBM_INPUT_OBJ)
	$(RM) -f $(UT_MBM_COILS_TARGET)$(EXEEXT) $(UT_MBM_COILS_OBJ)
	$(RM) -f $(UT_MBM_FILES_TARGET)$(EXEEXT) $(UT_MBM_FILES_OBJ)
	$(RM) -f $(UT_MBM_DISCRETE_TARGET)$(EXEEXT) $(UT_MBM_DISCRETE_OBJ)
	$(RM) -f $(UT_MBM_RTU_TARGET)$(EXEEXT) $(UT_MBM_RTU_OBJ)
	$(RM) -f $(UT_MBM_RTU_V2_TARGET)$(EXEEXT) $(UT_MBM_RTU_V2_OBJ)
	$(RM) -f $(UT_MBM_ASCII_TARGET)$(EXEEXT) $(UT_MBM_ASCII_OBJ)
	$(RM) -f $(UT_MBM_ASCII_V2_TARGET)$(EXEEXT) $(UT_MBM_ASCII_V2_OBJ)
	$(RM) -f $(UT_MBM_TCP_TARGET) $(UT_MBM_TCP_OBJ)
	$(RM) -f $(UT_MBM_TCP_ANALYZER_TARGET)$(EXEEXT) $(UT_MBM_TCP_ANALYZER_OBJ)
	$(RM) -f $(UT_MBM_ASCII_ANALYZER_TARGET)$(EXEEXT) $(UT_MBM_ASCII_ANALYZER_OBJ)
	$(RM) -f $(UT_MBM_RTU_ANALYZER_TARGET)$(EXEEXT) $(UT_MBM_RTU_ANALYZER_OBJ)
	$(RM) -f $(UT_MBM_TCP_STAT_TARGET)$(EXEEXT) $(UT_MBM_TCP_STAT_OBJ)
	$(RM) -f $(UT_MBM_ASCII_STAT_TARGET)$(EXEEXT) $(UT_MBM_ASCII_STAT_OBJ)
	$(RM) -f $(UT_MBM_RTU_STAT_TARGET)$(EXEEXT) $(UT_MBM_RTU_STAT_OBJ)
	$(RM) -f $(UT_MBM_ASCII_V2_STAT_TARGET)$(EXEEXT) $(UT_MBM_ASCII_V2_STAT_OBJ)
	$(RM) -f $(UT_MBM_RTU_V2_STAT_TARGET)$(EXEEXT) $(UT_MBM_RTU_V2_STAT_OBJ)
	$(RM) -f $(UT_MBM_UDP_TARGET)$(EXEEXT) $(UT_MBM_UDP_OBJ)

	$(FIND) .. \( -name "*.gcda" -o -name "*.gcno" -o -name "*.gcov" \) -exec rm {} \;

# Include the dependency files.
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# Listing of phony targets.
.PHONY : all clean

