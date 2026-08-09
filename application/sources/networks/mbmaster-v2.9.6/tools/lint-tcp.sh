# Autor: Christian Walter <cwalter@embedded-solutions.at>
# Description: LINT Tests for MODBUS TCP

# Test TCP layer
lint-nt std.lnt \
	-dMBM_RTU_ENABLED=0 \
	-dMBM_ASCII_ENABLED=0 \
	-dMBM_TCP_ENABLED=1 \
	../mbmaster/mbm.c \
	../mbmaster/tcp/mbmtcp.c 
