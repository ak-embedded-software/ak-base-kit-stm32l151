# Autor: Christian Walter <cwalter@embedded-solutions.at>
# Description: LINT Tests for MODBUS ASCII

# Test ACII layer with serial API V2
lint-nt std.lnt \
	-dMBM_RTU_ENABLED=0 \
	-dMBM_SERIAL_API_VERSION=2 \
	-dMBM_ASCII_ENABLED=1 \
	-dMBM_TCP_ENABLED=0 \
        -dMBP_FORCE_SERV2PROTOTYPES=1 \
	../mbmaster/mbm.c \
	../mbmaster/ascii/mbmascii.c 

# Test ASCII layer with serial API V2 and wait after send timeout
lint-nt std.lnt \
	-dMBM_RTU_ENABLED=0 \
	-dMBM_SERIAL_API_VERSION=2 \
	-dMBM_ASCII_ENABLED=1 \
	-dMBM_TCP_ENABLED=0 \
        -dMBP_FORCE_SERV1PROTOTYPES=1 \
        -dMBM_ASCII_WAITAFTERSEND_ENABLED=1 \
	../mbmaster/mbm.c \
	../mbmaster/ascii/mbmascii.c 

# Test ASCII layer with serial API V2 and wait after send timeout
# and with backoff timer
lint-nt std.lnt \
	-dMBM_RTU_ENABLED=0 \
	-dMBM_SERIAL_API_VERSION=2 \
	-dMBM_ASCII_ENABLED=1 \
	-dMBM_TCP_ENABLED=0 \
        -dMBP_FORCE_SERV1PROTOTYPES=1 \
        -dMBM_ASCII_WAITAFTERSEND_ENABLED=1 \
        -dMBM_ASCII_BACKOF_TIME_MS=10 \
	../mbmaster/mbm.c \
	../mbmaster/ascii/mbmascii.c 
