##############################################################################
# @author: An Nguyen Khanh
# @date:   05/2/2017
##############################################################################
#NOTE:
# Learn Makefiles at https://makefiletutorial.com/
# using find PATH -type d -print to list subdirectories recursively

# Utilitis define
Print = @echo "~"

# Name of build target ex: dungeon-game
NAME_MODULE = dungeon-game
PROJECT = $(NAME_MODULE)

GCC_PATH ?= /usr
PROGRAMER_PATH		= $(HOME)/Workspace/Tools/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin
OPENOCD_CFG_PATH	= /usr/local/share/openocd/scripts/board/stm32ldiscovery.cfg

# App start address, that need sync with declare in linker file and interrupt vector table.
APP_START_ADDR_VAL = 0x08003000
APP_ADDR_OPTION = -DAPP_START_ADDR=$(APP_START_ADDR_VAL)

# Define console baudrate, that is used to configure UART.
SYS_CONSOLE_BAUDRATE = 115200
CONSOLE_BAUDRATE_DEF = -DSYS_CONSOLE_BAUDRATE=$(SYS_CONSOLE_BAUDRATE)

#|---------------------------------------------------------------------------------------------------|
#| OPTIMIZE LEVELS                                                                                   |
#|------------|----------------------------------|--------------|---------|------------|-------------|
#|   option   | optimization levels              |execution time|code size|memory usage|complile time|
#|------------|----------------------------------|--------------|---------|------------|-------------|
#|   -O0      | compilation time                 |     (+)      |   (+)   |     (-)    |    (-)      |
#| -O1 || -O  | code size && execution time      |     (-)      |   (-)   |     (+)    |    (+)      |
#|   -O2      | more code size && execution time |     (--)     |         |     (+)    |    (++)     |
#|   -O3      | more code size && execution time |     (---)    |         |     (+)    |    (+++)    |
#|   -Os      | code size                        |              |   (--)  |            |    (++)     |
#|  -Ofast    | O3 with none math cals           |     (---)    |         |     (+)    |    (+++)    |
#|------------|----------------------------------|--------------|---------|------------|-------------|
OPTIMIZE_OPTION = -g -Os

# Release build option, that is used in fatal function, restart system when fatal, and debug levels.
RELEASE_OPTION = -URELEASE

# Enable PIN (using led life) interrupt analyzer function [entry IRQ -> led life ON] and [exit IRQ -> led life OFF].
IRQ_DEBUG_OPTION = -UAK_IO_IRQ_ANALYZER

# Interface compile option.
IF_LINK_OPTION = -DIF_LINK_UART_EN

# Include sources file
include sources/ak/Makefile.mk
include sources/app/Makefile.mk
include sources/common/Makefile.mk
include sources/driver/Makefile.mk
include sources/sys/Makefile.mk
include sources/networks/Makefile.mk
include sources/platform/stm32l/Makefile.mk

OBJECTS_DIR = build_$(NAME_MODULE)
TARGET = $(OBJECTS_DIR)/$(NAME_MODULE).axf

SOURCES		+=
SOURCES_	 = $(shell find $(SOURCES) -type f -printf "%f\n")
OBJECTS		+= $(patsubst %.c, $(OBJECTS_DIR)/%.o, $(SOURCES_))

SOURCES_CPP	+=
SOURCES_CPP_	 = $(shell find $(SOURCES_CPP) -type f -printf "%f\n")
OBJECTS		+= $(patsubst %.cpp, $(OBJECTS_DIR)/%.o, $(SOURCES_CPP_))

SOURCES_ASM	+=
SOURCES_ASM_	 = $(shell find $(SOURCES_ASM) -type f -printf "%f\n")
OBJECTS		+= $(patsubst %.s, $(OBJECTS_DIR)/%.o, $(SOURCES_ASM_))

CC              = $(GCC_PATH)/bin/arm-none-eabi-gcc
CPP             = $(GCC_PATH)/bin/arm-none-eabi-g++
AR              = $(GCC_PATH)/bin/arm-none-eabi-ar
AS              = $(CC) -x assembler-with-cpp
LD              = $(GCC_PATH)/bin/arm-none-eabi-ld
OBJCOPY         = $(GCC_PATH)/bin/arm-none-eabi-objcopy
OBJDUMP         = $(GCC_PATH)/bin/arm-none-eabi-objdump
OBJNM           = $(GCC_PATH)/bin/arm-none-eabi-nm
ARM_GDB         = $(GCC_PATH)/bin/arm-none-eabi-gdb
ARM_SIZE        = $(GCC_PATH)/bin/arm-none-eabi-size

# Set the compiler CPU/FPU options.
# https://launchpadlibrarian.net/177524521/readme.txt
CPU = -mthumb -march=armv7-m
FPU = -mfloat-abi=soft

LIBC		= $(shell $(CC) $(CPU) $(FPU) -print-file-name=libc.a)
LIBM		= $(shell $(CC) $(CPU) $(FPU) -print-file-name=libm.a)
LIBFPU		= $(shell $(CC) $(CPU) $(FPU) -print-file-name=libg.a)
LIBRDPMON	= $(shell $(CC) $(CPU) $(FPU) -print-file-name=libnosys.a)
LIBSTDCPP_NANO	= $(shell $(CPP) $(CPU) $(FPU) -print-file-name=libstdc++_nano.a)
LIBGCC		= $(shell $(CC) $(CPU) $(FPU) -print-libgcc-file-name)
LIBGCOV		= $(shell $(CC) $(CPU) $(FPU) -print-file-name=libgcov.a)

LIB_PATH += -L$(dir $(LIBC))
LIB_PATH += -L$(dir $(LIBGCC))

# Interface option
IF_OPTION +=	$(IF_LINK_OPTION)		\

# OLED controller select. Adafruit_oled_drv has separate init sequences for
# SH1106 / SSD1306 / SSD1309 and #errors if none is selected.
# The AK base kit v3 carries an SSD1309.
OLED_OPTION +=				\
		-DSSD1309_DRIVER_EN	\
		-USH1106_DRIVER_EN	\

# Console log compile option
CONSOLE_OPTION +=			\
		-DLOGIN_PRINT_EN	\
		-DSYS_PRINT_EN		\
		-DAPP_PRINT_EN		\
		-DSYS_DBG_EN		\
		-DAPP_DBG_EN		\

# NOTE: -DAPP_DBG_SIG_EN prints every dispatched signal over UART (blocking xprintf).
# With a 100ms game tick posting to 5 tasks this measurably delays the game loop.
# Re-enable only when debugging signal flow.
#		-DAPP_DBG_SIG_EN

GENERAL_FLAGS +=			\
		$(RTOS_DEV_OPTION)	\
		$(CONSOLE_BAUDRATE_DEF)	\
		$(CONSOLE_OPTION)	\
		$(IRQ_DEBUG_OPTION)	\
		$(WARNING_OPTION)	\
		$(OPTIMIZE_OPTION)	\
		$(RELEASE_OPTION)	\
		$(APP_ADDR_OPTION)	\
		$(USB_OPTION)		\
		$(IF_OPTION)		\
		$(OLED_OPTION)		\
		-DUSE_STDPERIPH_DRIVER	\
		-DSTM32L1XX_MD		\
		-DSTM32L_PLATFORM	\
		-DUSE_EXTERNAL_FLASH	\

COMPILER_FLAGS +=			\
		$(CPU)			\
		$(FPU)			\
		-ffunction-sections	\
		-fdata-sections		\
		-fstack-usage		\
		-MD			\
		-Wall			\
		-c			\

# C compiler flags
CFLAGS +=				\
		$(GENERAL_FLAGS)	\
		$(COMPILER_FLAGS)	\
		-std=c99		\

# C++ compiler flags
CPPFLAGS +=				\
		$(GENERAL_FLAGS)	\
		$(COMPILER_FLAGS)	\
		-std=c++11		\
		-fno-rtti		\
		-fno-exceptions		\
		-fno-use-cxa-atexit	\

# linker flags
LDFLAGS	=	-Map=$(OBJECTS_DIR)/$(PROJECT).map	\
		--gc-sections	\
		$(LIB_PATH)	\
		$(LIBC) $(LIBM) $(LIBSTDCPP_NANO) $(LIBGCC) $(LIBGCOV) $(LIBFPU) $(LIBRDPMON)

all: create $(TARGET)

create:
	$(Print) CREATE $(OBJECTS_DIR) folder
	@mkdir -p $(OBJECTS_DIR)

$(TARGET): $(OBJECTS) $(LIBC) $(LIBM) $(LIBSTDCPP_NANO) $(LIBGCC) $(LIBGCOV) $(LIBFPU) $(LIBRDPMON)
	$(Print) LD $@
	@$(LD) --entry reset_handler -T $(LDFILE) $(LDFLAGS) -o $(@) $(^)
	$(Print) OBJCOPY $(@:.axf=.bin)
	@$(OBJCOPY) -O binary $(@) $(@:.axf=.bin)
	@$(OBJCOPY) -O binary $(@) $(@:.axf=.out)
	@$(OBJCOPY) -O binary $(@) $(@:.axf=.elf)
	@$(ARM_SIZE) $(TARGET)

$(OBJECTS_DIR)/%.o: %.c
	$(Print) CC $@
	@$(CC) $(CFLAGS) -o $@ $<

$(OBJECTS_DIR)/%.o: %.cpp
	$(Print) CXX $@
	@$(CPP) $(CPPFLAGS) -o $@ $<

$(OBJECTS_DIR)/%.o: %.s
	$(Print) AS $@
	@$(AS) $(CFLAGS) -o $@ $<

.PHONY: help
help:
	$(Print) "[make flash dev=/dev/tty*] burn firmware via ak bootloader"
	$(Print) "             Note: This option is needed install ak-flash. Link download & install: https://github.com/ak-embedded-software/ak-flash"
	$(Print) "             Example: make flash dev=/dev/ttyUSB0"
	$(Print) "[make flash] burn firmware via st-link"
	$(Print) "[make factory] burn factory firmware via st-link"
	$(Print) "[make com dev=/dev/tty*] console AK Embedded Base Kit"
	$(Print) "[make debug gdb=ddd] gdb debug using ddd"
	$(Print) "[make debug] gdb debug"
	$(Print) "[make clean] clean build project folder"
	$(Print) "[make asm] generate asm file"
	$(Print) "[make sym] list symbol from objects file"

.PHONY: flash
flash: all
	$(Print) BURNING $(TARGET:.axf=.bin) to target
ifdef dev
	ak-flash $(dev) $(TARGET:.axf=.bin) $(APP_START_ADDR_VAL)
else
	$(PROGRAMER_PATH)/STM32_Programmer.sh -c port=SWD -w $(TARGET:.axf=.bin) $(APP_START_ADDR_VAL) -rst
endif

.PHONY: factory
factory:
ifdef dev
	ak-flash $(dev) ../resources/bin/ak-base-kit-stm32l151-application.bin $(APP_START_ADDR_VAL)
else
	$(PROGRAMER_PATH)/STM32_Programmer.sh -c port=SWD -w ../resources/bin/ak-base-kit-stm32l151-application.bin $(APP_START_ADDR_VAL) -rst
endif

.PHONY: com
com:
ifdef dev
	minicom -D $(dev) -b 115200
else
	$(Print) "Example: make com dev=/dev/ttyUSB0"
endif

.PHONY: debug
debug: all
	xterm -e openocd -f $(OPENOCD_PATH)/tcl/board/stm32ldiscovery.cfg &
ifeq ($(gdb),ddd)
	$(Print) DBG using DDD
	ddd --debugger $(ARM_GDB) --command=stm32l_init.gdb $(TARGET)
else
	$(Print) DBG GDB
	$(ARM_GDB) -w --command=stm32l_init.gdb $(TARGET)
endif

.PHONY: clean
clean:
	$(Print) CLEAN $(OBJECTS_DIR) folder
	@rm -rf $(OBJECTS_DIR)

asm: $(TARGET)
	$(Print) disassemble-all $(<:.axf=.asm)
	@$(OBJDUMP) -D $(<) >> $(<:.axf=.asm)

sym: $(TARGET)
	$(Print) export object name $(<:.axf=.sym)
	$(OBJNM) --size-sort --print-size $(<) >> $(<:.axf=.sym)
