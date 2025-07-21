# Makefile for ElectrometerBootloader Project
# Builds main project, modules, and config table
# Assumes IAR Embedded Workbench 8.0 and Python 3

# Directories
PROJ_DIR := .
BUILD_DIR := $(PROJ_DIR)/build
SRC_DIR := $(PROJ_DIR)/src
LINKER_DIR := $(PROJ_DIR)/linker
SCRIPTS_DIR := $(PROJ_DIR)/scripts
PROGRAM_DIR := D:/ProgramFiles/IAR Systems/Embedded Workbench 8.0

# IAR Tools
IARBUILD := $(PROGRAM_DIR)/common/bin/iarbuild.exe
IELFTOOL := $(PROGRAM_DIR)/arm/bin/ielftool.exe

# Python
PYTHON := python3

# Project Files
MAIN_EWP := $(PROJ_DIR)/ElectrometerBootloader.ewp
MODULE1_EWP := $(PROJ_DIR)/module1.ewp
MODULE2_EWP := $(PROJ_DIR)/module2.ewp
CONFIG_SCRIPT := $(SCRIPTS_DIR)/generate_config_table.py

# Output Files
MAIN_OUT := $(BUILD_DIR)/main.out
MAIN_BIN := $(BUILD_DIR)/main.bin
MAIN_MAP := $(BUILD_DIR)/main.map
MODULE1_OUT := $(BUILD_DIR)/module1.out
MODULE1_BIN := $(BUILD_DIR)/module1.bin
MODULE1_MAP := $(BUILD_DIR)/module1.map
MODULE2_OUT := $(BUILD_DIR)/module2.out
MODULE2_BIN := $(BUILD_DIR)/module2.bin
MODULE2_MAP := $(BUILD_DIR)/module2.map
CONFIG_BIN := $(BUILD_DIR)/config_table.bin

# Compiler/Linker Options
IAR_FLAGS := -f $(LINKER_DIR)/v85xx.icf
MODULE_FLAGS := -f $(LINKER_DIR)/module.icf

# Ensure build directory exists
$(shell mkdir -p $(BUILD_DIR))

# Targets
all: $(MAIN_BIN) $(MODULE1_BIN) $(MODULE2_BIN) $(CONFIG_BIN)

# Build main project
$(MAIN_OUT): $(SRC_DIR)/main.c $(SRC_DIR)/core_app.c $(SRC_DIR)/jump_table.h $(LINKER_DIR)/v85xx.icf
	@if not exist $(SRC_DIR)/main.c (echo Error: $(SRC_DIR)/main.c not found && exit 1)
	"$(IARBUILD)" $(MAIN_EWP) -build Debug
	@if not exist $(MAIN_OUT) (echo Error: Failed to generate $(MAIN_OUT) && exit 1)

$(MAIN_BIN): $(MAIN_OUT)
	"$(IELFTOOL)" --bin "$(MAIN_OUT)" "$(MAIN_BIN)"
	@if not exist "$(MAIN_BIN)" (echo Error: Failed to generate $(MAIN_BIN) && exit 1)

# Build module1
$(MODULE1_OUT): $(SRC_DIR)/module1.c $(SRC_DIR)/jump_table.h $(LINKER_DIR)/module.icf
	"$(IARBUILD)" "$(MODULE1_EWP)" -build Debug
	@if not exist "$(MODULE1_OUT)" (echo Error: Failed to generate $(MODULE1_OUT) && exit 1)

$(MODULE1_BIN): $(MODULE1_OUT)
	"$(IELFTOOL)" --bin "$(MODULE1_OUT)" "$(MODULE1_BIN)"
	@if not exist "$(MODULE1_BIN)" (echo Error: Failed to generate $(MODULE1_BIN) && exit 1)

# Build module2
$(MODULE2_OUT): $(SRC_DIR)/module2.c $(SRC_DIR)/jump_table.h $(LINKER_DIR)/module.icf
	"$(IARBUILD)" "$(MODULE2_EWP)" -build Debug
	@if not exist "$(MODULE2_OUT)" (echo Error: Failed to generate $(MODULE2_OUT) && exit 1)

$(MODULE2_BIN): $(MODULE2_OUT)
	"$(IELFTOOL)" --bin "$(MODULE2_OUT)" "$(MODULE2_BIN)"
	@if not exist "$(MODULE2_BIN)" (echo Error: Failed to generate $(MODULE2_BIN) && exit 1)

# Generate config table
$(CONFIG_BIN): $(MODULE1_BIN) $(MODULE1_MAP) $(MODULE2_BIN) $(MODULE2_MAP)
	$(PYTHON) "$(CONFIG_SCRIPT)" "$(MODULE1_MAP)" "$(MODULE2_MAP)" "$(MODULE1_BIN)" "$(MODULE2_BIN)"
	@if not exist "$(CONFIG_BIN)" (echo Error: Failed to generate $(CONFIG_BIN) && exit 1)

# Clean build directory
clean:
	if exist "$(BUILD_DIR)" (del /Q "$(BUILD_DIR)\*" & rmdir /S /Q "$(BUILD_DIR)")
	"$(IARBUILD)" "$(MAIN_EWP)" -clean Debug
	"$(IARBUILD)" "$(MODULE1_EWP)" -clean Debug
	"$(IARBUILD)" "$(MODULE2_EWP)" -clean Debug

# Phony targets
.PHONY: all clean