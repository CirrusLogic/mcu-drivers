##############################################################################
#
# Makefile for CS40L5X Alt-OS driver
#
##############################################################################
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

##############################################################################
# Variable Assignments
##############################################################################

# Cache the make globals before any created here
MAKE_PRE_SESSION_GLOBALS := $(.VARIABLES)

# Include common.mk
REPO_PATH = $(abspath ../..)
include $(REPO_PATH)/common/common.mk

# Assign paths and variables
PART_NUM = cs40l5x
BUILD_VARIANT := CS40L51
BUILD_NUM_UPPER = $(shell echo "$(BUILD_VARIANT)" | tr '[:lower:]' '[:upper:]')
BUILD_NUM_LOWER = $(shell echo "$(BUILD_VARIANT)" | tr '[:upper:]' '[:lower:]')

PROJECT ?= basic_sample

$(eval $(call assign_paths))
TARGET_PATH = $(DRIVER_PATH)/samples/$(MAKECMDGOALS)
SAMPLE_PATH = $(TARGET_PATH)/$(PROJECT)
TUNING_PATH = $(SAMPLE_PATH)/tunings
ZEPHYR_APP_PATH = $(DRIVER_PATH)/samples/zephyr/$(PROJECT)
ZEPHYR_BUILD_PATH = $(BUILD_DIR)/zephyr/$(PROJECT)
ZEPHYR_TUNING_BINS = $(wildcard $(ZEPHYR_APP_PATH)/tunings/*.bin)
ZEPHYR_VENV = $(REPO_PATH)/../.venv/bin/activate
PLATFORM_TARGETS = system_test freertos baremetal blackstar
VALID_TARGETS = unit_test $(PLATFORM_TARGETS)
$(eval $(call eval_targets))
$(eval $(call eval_optimization_level))

# Include platform_bsp.mk
ifdef IS_NOT_UNIT_TEST
    ifeq ($(MAKECMDGOALS), blackstar)
        PLATFORM=blackstar
    else
	    PLATFORM=eestm32int
	endif
	include $(REPO_PATH)/common/platform_bsp/platform_bsp.mk
endif

# Assign firmware and configuration script variables
#HALO_FIRMWARE_FILE = ROM
ifeq ($(MAKECMDGOALS), baremetal)
FIRMWARE_CONV_TARGET = export
FIRMWARE_HEADER = ./$(BUILD_NUM_LOWER)_firmware.h
BSP_NAME = bsp_cs40l5x_baremetal
HALO_FIRMWARE_WMDR = $(TUNING_PATH)/cs40l5x_wt.bin --preserve-filename
else
FIRMWARE_CONV_TARGET = fw_img_v2
FIRMWARE_HEADER = ./$(BUILD_NUM_LOWER)_fw_img.h
HALO_FIRMWARE_WMDR = $(TUNING_PATH)/cs40l5x_wt.bin
BSP_NAME = bsp_cs40l5x
endif

HALO_FIRMWARE_FILE = $(BUILD_NUM_UPPER)_Rev4.0.14.wmfw
WISCE_SCRIPT = $(CONFIG_PATH)/wisce_init.txt
MISC_SCRIPTS = hwt_to_waveform_converter

# Assign toolchain and toolchain flags
$(eval $(call assign_toolchain))
$(eval $(call assign_toolchain_flags))
ifdef IS_NOT_UNIT_TEST
    CFLAGS += -DCONFIG_USE_MULTICHANNEL_UART

    ifeq ($(SEMIHOSTING), 1)
        CFLAGS += -DSEMIHOSTING
    endif

endif

ifeq ($(MAKECMDGOALS), baremetal)
    CFLAGS += -DCS40L5X_BAREMETAL
endif

CFLAGS += -DCS40L5X_FIRMWARE_$(BUILD_NUM_UPPER)

# Assign sources and includes for driver library
DRIVER_SRCS = $(DRIVER_PATH)/cs40l5x.c
DRIVER_SRCS += $(CONFIG_PATH)/cs40l5x_syscfg_regs.c
ifeq ($(MAKECMDGOALS), baremetal)

else
    DRIVER_SRCS += $(COMMON_PATH)/fw_img.c
endif
DRIVER_SRCS += $(COMMON_PATH)/regmap.c
INCLUDES += -I$(HALO_FIRMWARE_PATH)
DRIVER_SRCS += $(CONFIG_PATH)/waveforms.c

# Assign sources and includes for target project build
$(eval $(call assign_sources_includes))
ifeq ($(MAKECMDGOALS), unit_test)
    C_SRCS += $(TARGET_PATH)/test_cs40l5x.c
    C_SRCS += $(TARGET_PATH)/mock_bsp.c
	C_SRCS += $(TARGET_PATH)/main.c

    ADD_OBJ_RULES = add_unit_test_obj_rules
else ifdef IS_NOT_UNIT_TEST
    ifeq ($(MAKECMDGOALS), system_test)
		C_SRCS += $(SAMPLE_PATH)/$(BUILD_NUM_LOWER)_fw_img.c
        C_SRCS += $(TARGET_PATH)/test_cs40l5x.c
    else
		ifeq ($(MAKECMDGOALS), baremetal)
			C_SRCS += $(SAMPLE_PATH)/$(BUILD_NUM_LOWER)_firmware.c
		else
			C_SRCS += $(SAMPLE_PATH)/$(BUILD_NUM_LOWER)_fw_img.c
		endif
		INCLUDES += -I$(SAMPLE_PATH)
	endif
	C_SRCS += $(SAMPLE_PATH)/$(BSP_NAME).c
	C_SRCS += $(SAMPLE_PATH)/main.c

	INCLUDES += -I$(TARGET_PATH)
    ADD_OBJ_RULES = add_platform_obj_rules
endif

# Assign automatic variables for target recipes below
$(eval $(call assign_objs))
$(eval $(call assign_build_paths))

# Assign bullseye vars (if a bullseye build)
$(eval $(call add_bullseye_vars))
BULLSEYE_INCLUDE_PATHS += $(REPO_PATH)/cs40l5x/
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/cs40l5x/samples/unit_test/
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/cs40l5x/samples/system_test/

# Print out make variables
$(if $(filter-out clean, $(MAKECMDGOALS)), $(eval $(call print_vars, MAKE_PRE_SESSION_GLOBALS, $(MAKE_PRE_SESSION_GLOBALS))))

##############################################################################
# Target Rules
##############################################################################

.PHONY: default all build_path clean zephyr zephyr_syscfg_regs zephyr_firmware
default: print_targets

# Create directories for the build detritus
$(eval $(call add_build_dir_rules, $(BUILD_DIR), $(BUILD_PATHS)))

# Add any bullseye includes/excludes (if a bullseye build)
$(eval $(call add_bullseye_rules, $(BULLSEYE_INCLUDE_PATHS), $(BULLSEYE_EXCLUDE_PATHS)))

# Code generation rules
$(eval $(call add_wisce_script_rule,cs40l5x))

.NOTPARALLEL: firmware_converter
firmware_converter:
	@test -d "$(SAMPLE_PATH)" || { echo "Project directory not found: $(SAMPLE_PATH)"; exit 1; }
	@echo -------------------------------------------------------------------------------
	@echo GENERATING $(BUILD_NUM_LOWER)_img
	cd $(HALO_FIRMWARE_PATH) && python ../../tools/firmware_converter/firmware_converter.py $(FIRMWARE_CONV_TARGET) $(BUILD_NUM_LOWER) $(HALO_FIRMWARE_FILE) --wmdr $(HALO_FIRMWARE_WMDR) --output-directory "$(SAMPLE_PATH)"

hwt_to_waveform_converter:
	@echo -------------------------------------------------------------------------------
	@echo GENERATING waveforms
	cd $(DRIVER_PATH) && python ../tools/hwt_to_waveform/hwt_to_waveform_converter.py cs40l50 config/waveforms.hwt --output-directory $(CONFIG_PATH)

# Compilation rules
$(eval $(call $(ADD_OBJ_RULES), $(OBJS)))
$(eval $(call add_driver_obj_rules))
$(eval $(call add_asm_obj_rules))
$(eval $(call add_driver_library_rule))

$(eval $(call add_unit_test_rule))
$(eval $(call add_platform_target_rule, $(PLATFORM_TARGETS)))

zephyr: zephyr_firmware
	@test -d "$(ZEPHYR_APP_PATH)" || { echo "Zephyr project not found: $(ZEPHYR_APP_PATH)"; exit 1; }
	@test -f "$(ZEPHYR_APP_PATH)/boards/$(BUILD_NUM_LOWER).overlay" || { echo "Zephyr overlay not found for $(BUILD_VARIANT): $(ZEPHYR_APP_PATH)/boards/$(BUILD_NUM_LOWER).overlay"; exit 1; }
	@if [ "$${VIRTUAL_ENV:-}" != "$(patsubst %/bin/activate,%,$(ZEPHYR_VENV))" ]; then \
		. "$(ZEPHYR_VENV)" || exit $$?; \
	fi; \
	west build -p always -b nucleo_f401re -d "$(ZEPHYR_BUILD_PATH)" "$(ZEPHYR_APP_PATH)" -- \
		-DCONFIG_HAPTICS_$(BUILD_NUM_UPPER)=y \
		-DDTC_OVERLAY_FILE=$(ZEPHYR_APP_PATH)/boards/$(BUILD_NUM_LOWER).overlay

zephyr_firmware: zephyr_syscfg_regs
	@test -f "$(HALO_FIRMWARE_PATH)/$(HALO_FIRMWARE_FILE)" || { echo "Firmware not found: $(HALO_FIRMWARE_PATH)/$(HALO_FIRMWARE_FILE)"; exit 1; }
	python $(REPO_PATH)/tools/firmware_converter/firmware_converter.py export $(BUILD_NUM_LOWER) "$(HALO_FIRMWARE_PATH)/$(HALO_FIRMWARE_FILE)" --output-directory "$(ZEPHYR_APP_PATH)/src" $(if $(strip $(ZEPHYR_TUNING_BINS)),--wmdr $(ZEPHYR_TUNING_BINS),)

zephyr_syscfg_regs:
	@test -d "$(ZEPHYR_APP_PATH)/src" || { echo "Zephyr project source directory not found: $(ZEPHYR_APP_PATH)/src"; exit 1; }
	python $(REPO_PATH)/tools/wisce_script_converter/wisce_script_converter.py -c c_array -p cs40l5x -i $(WISCE_SCRIPT) -o $(ZEPHYR_APP_PATH)/src

print_targets:
	@echo ERROR:  No valid target specified.
	@echo
	@echo Valid targets:
	@echo       freertos
	@echo       baremetal
	@echo		blackstar
	@echo       system_test
	@echo       unit_test
	@echo       zephyr PROJECT=<application>
	@echo
	@echo Valid command line variables:
	@echo
	@echo		PROJECT=basic_sample 	\(all targets\)
	@echo		PROJECT=audio2haptics 	\(zephyr\)
	@echo		PROJECT=diagnostics 	\(zephyr\)
	@echo		PROJECT=owt_demo 		\(zephyr\)
	@echo		PROJECT=sample_app 		\(zephyr\)
	@echo
	@echo		BULLSEYE=0	\(will disable Bullseye coverage\)
	@echo		BULLSEYE=1	\(will enable Bullseye coverage\)
	@echo
	@echo       OPTIMIZATION_LEVEL=0    \(configure for -O0 optimization level\)
	@echo       OPTIMIZATION_LEVEL=1    \(configure for -O1 optimization level\)
	@echo       OPTIMIZATION_LEVEL=2    \(configure for -O2 optimization level\)
	@echo       OPTIMIZATION_LEVEL=3    \(configure for -O3 optimization level\)
	@echo       OPTIMIZATION_LEVEL=s    \(configure for -Os optimization level\)
	@echo
	@echo       BUILD_VARIANT=CS40L50
	@echo       BUILD_VARIANT=CS40L51
	@echo       BUILD_VARIANT=CS40L52
	@echo       BUILD_VARIANT=CS40L53
	@echo       BUILD_VARIANT=CS40L50_PC
	@echo

clean:
	$(RM) $(BUILD_DIR)
