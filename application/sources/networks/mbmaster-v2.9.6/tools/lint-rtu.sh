# Autor: Christian Walter <cwalter@embedded-solutions.at>
# Description: LINT Tests for MODBUS RTU

# Test RTU layer with serial API V2
lint-nt std.lnt \
	-dMBM_RTU_ENABLED=1 \
	-dMBM_SERIAL_API_VERSION=2 \
	-dMBM_ASCII_ENABLED=0 \
	-dMBM_TCP_ENABLED=0 \
        -dMBP_FORCE_SERV2PROTOTYPES=1 \
	../mbmaster/mbm.c \
	../mbmaster/rtu/mbmrtu.c \
	../mbmaster/rtu/mbmcrc.c 

# Test RTU layer with serial API V2 and wait after send timeout
lint-nt std.lnt \
	-dMBM_RTU_ENABLED=1 \
	-dMBM_SERIAL_API_VERSION=2 \
	-dMBM_ASCII_ENABLED=0 \
	-dMBM_TCP_ENABLED=0 \
        -dMBP_FORCE_SERV1PROTOTYPES=1 \
        -dMBM_RTU_WAITAFTERSEND_ENABLED=1 \
	../mbmaster/mbm.c \
	../mbmaster/rtu/mbmrtu.c \
	../mbmaster/rtu/mbmcrc.c 

# Test RTU layer with serial API V1
lint-nt std.lnt \
	-dMBM_RTU_ENABLED=1 \
	-dMBM_SERIAL_API_VERSION=1 \
	-dMBM_ASCII_ENABLED=0 \
	-dMBM_TCP_ENABLED=0 \
        -dMBP_FORCE_SERV1PROTOTYPES=1 \
	../mbmaster/mbm.c \
	../mbmaster/rtu/mbmrtu.c \
	../mbmaster/rtu/mbmcrc.c 

# Test RTU layer with serial API V1 and wait after send timeout
lint-nt std.lnt \
	-dMBM_RTU_ENABLED=1 \
	-dMBM_SERIAL_API_VERSION=1 \
	-dMBM_ASCII_ENABLED=0 \
	-dMBM_TCP_ENABLED=0 \
        -dMBP_FORCE_SERV1PROTOTYPES=1 \
        -dMBM_RTU_WAITAFTERSEND_ENABLED=1 \
	../mbmaster/mbm.c \
	../mbmaster/rtu/mbmrtu.c \
	../mbmaster/rtu/mbmcrc.c 
