CFLAGS += -I./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/inc

VPATH += sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src

# C source files
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_gpio.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_rcc.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_usart.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_spi.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/misc.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_exti.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_syscfg.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_tim.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_adc.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_rtc.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_pwr.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_flash.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_iwdg.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_comp.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_dac.c
SOURCES += ./sources/platform/stm32l/Libraries/STM32L1xx_StdPeriph_Driver/src/stm32l1xx_i2c.c
