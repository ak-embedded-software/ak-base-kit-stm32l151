ifeq ($(IF_NETWORK_NRF24_OPTION),-DIF_NETWORK_NRF24_EN)
include sources/networks/rf_protocols/Makefile.mk
endif

ifeq ($(IF_LINK_OPTION),-DIF_LINK_UART_EN)
include sources/networks/net/link/Makefile.mk
endif

ifeq ($(TASK_ZIGBEE_OPTION),-DTASK_ZIGBEE_EN)
include sources/networks/ArduinoZigBee/Makefile.mk
endif

ifeq ($(TASK_MBMASTER_OPTION),-DTASK_MBMASTER_EN)
include sources/networks/mbmaster-v2.9.6/Makefile.mk
endif
