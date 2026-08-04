# Bản chạy thử Dungeon trên máy tính.
#
#   make        biên dịch ra ./dungeon-sim
#   make run    chạy kiểu Terminal
#   make web    chạy kiểu trình duyệt, mở http://localhost:8080
#   make clean  dọn
#
# Chỉ cần clang (macOS đã có sẵn sau khi cài Xcode Command Line Tools) hoặc
# g++. Không cần arm-none-eabi-gcc, không cần thư viện ngoài.

SRC_ROOT := ../../application/sources
BUILD    := build
TARGET   := dungeon-sim

CXX      ?= c++
CC       ?= cc

INC := \
	-I. -Istub \
	-I$(SRC_ROOT)/ak/inc \
	-I$(SRC_ROOT)/app \
	-I$(SRC_ROOT)/app/screens \
	-I$(SRC_ROOT)/app/game/dungeon_game \
	-I$(SRC_ROOT)/common \
	-I$(SRC_ROOT)/common/container \
	-I$(SRC_ROOT)/sys \
	-I$(SRC_ROOT)/driver/eeprom \
	-I$(SRC_ROOT)/driver/buzzer \
	-I$(SRC_ROOT)/platform/stm32l

DEF := -DAPP_DBG_EN -DAPP_PRINT_EN -DSYS_PRINT_EN -DSYS_DBG_EN -DLOGIN_PRINT_EN

WARN := -Wall -Wextra -Wno-unused-parameter -Wno-write-strings \
        -Wno-unused-variable -Wno-unused-function -Wno-missing-field-initializers

CFLAGS   := -std=gnu99  -O1 -g $(INC) $(DEF) $(WARN)
CXXFLAGS := -std=c++11  -O1 -g $(INC) $(DEF) $(WARN)

# ---- nguồn thật của firmware, không sao chép, không sửa ----
C_SRC := \
	$(wildcard $(SRC_ROOT)/ak/src/*.c) \
	$(SRC_ROOT)/common/utils.c \
	$(wildcard $(SRC_ROOT)/common/container/*.c)

CXX_SRC := \
	$(SRC_ROOT)/common/screen_manager.cpp \
	$(SRC_ROOT)/common/view_render.cpp \
	$(SRC_ROOT)/common/view_item.cpp \
	$(SRC_ROOT)/app/task_list.cpp \
	$(SRC_ROOT)/app/app_eeprom.cpp \
	$(wildcard $(SRC_ROOT)/app/game/dungeon_game/*.cpp) \
	$(wildcard $(SRC_ROOT)/app/screens/*.cpp) \
	sim_platform.cpp \
	sim_core.cpp \
	sim_main.cpp

OBJ := $(patsubst %,$(BUILD)/%.o,$(notdir $(C_SRC)) $(notdir $(CXX_SRC)))

VPATH := $(sort $(dir $(C_SRC) $(CXX_SRC)))

.PHONY: all run web clean

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "  LD    $@"
	@$(CXX) $(OBJ) -o $@

$(BUILD)/%.c.o: %.c | $(BUILD)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.cpp.o: %.cpp | $(BUILD)
	@echo "  CXX   $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)

run: $(TARGET)
	./$(TARGET)

web: $(TARGET)
	./$(TARGET) --web

clean:
	rm -rf $(BUILD) $(TARGET) $(TARGET).dSYM
