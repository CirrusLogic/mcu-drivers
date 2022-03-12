##############################################################################
#
# Makefile for CS35L41 Alt-OS driver
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
REPO_PATH = $(abspath ../)
include $(REPO_PATH)/common/common.mk

# Assign paths and variables
PART_NUM = cs35l41
$(eval $(call assign_paths))
PLATFORM_TARGETS = system_test baremetal freertos
VALID_TARGETS = unit_test $(PLATFORM_TARGETS)
$(eval $(call eval_targets))
$(eval $(call eval_optimization_level))

# Include platform_bsp.mk
ifdef IS_NOT_UNIT_TEST
    PLATFORM=eestm32int
    include $(REPO_PATH)/common/platform_bsp/platform_bsp.mk
endif

# Assign firmware and configuration script variables
HALO_FIRMWARE_FILE = halo_cspl_RAM_revB2_29.45.0.wmfw
HALO_CAL_FIRMWARE_FILE = halo_cspl_RAM_revB2_29.45.0.wmfw
HALO_CAL_FIRMWARE_WMDR = Protect_Lite_cal_6.43.0_7.0ohm_delta1ohm_L41_revB2.bin
WISCE_SCRIPT = $(CONFIG_PATH)/wisce_init.txt

# Assign toolchain and toolchain flags
$(eval $(call assign_toolchain))
$(eval $(call assign_toolchain_flags))
ifdef IS_NOT_UNIT_TEST
    CFLAGS += -DUSE_CS35L41_SPI
    CFLAGS += -DCONFIG_USE_MULTICHANNEL_UART

    ifeq ($(SEMIHOSTING), 1)
        CFLAGS += -DSEMIHOSTING
    endif
endif

# Assign sources and includes for driver library
DRIVER_SRCS = $(DRIVER_PATH)/cs35l41.c
DRIVER_SRCS += $(CONFIG_PATH)/cs35l41_fs_switch_syscfg.c
DRIVER_SRCS += $(CONFIG_PATH)/cs35l41_syscfg_regs.c
DRIVER_SRCS += $(COMMON_PATH)/fw_img.c
DRIVER_SRCS += $(COMMON_PATH)/regmap.c
DRIVER_SRCS += $(DRIVER_PATH)/cs35l41_ext.c
INCLUDES += -I$(HALO_FIRMWARE_PATH)

# Assign sources and includes for target project build
$(eval $(call assign_sources_includes))
C_SRCS += $(APP_PATH)/main.c
ifeq ($(MAKECMDGOALS), unit_test)
    C_SRCS += $(APP_PATH)/test_cs35l41.c
    C_SRCS += $(APP_PATH)/mock_bsp.c

    ADD_OBJ_RULES = add_unit_test_obj_rules
else ifdef IS_NOT_UNIT_TEST
    C_SRCS += $(DRIVER_PATH)/bsp/bsp_cs35l41.c
    C_SRCS += $(HALO_FIRMWARE_PATH)/cs35l41_fw_img.c
    C_SRCS += $(HALO_FIRMWARE_PATH)/cs35l41_tune_fw_img.c
    C_SRCS += $(HALO_FIRMWARE_PATH)/cs35l41_cal_fw_img.c
    C_SRCS += $(HALO_FIRMWARE_PATH)/cs35l41_tune_48_fw_img.c
    C_SRCS += $(HALO_FIRMWARE_PATH)/cs35l41_tune_44p1_fw_img.c
    C_SRCS += $(COMMON_PATH)/bridge/bridge.c

    ifeq ($(MAKECMDGOALS), system_test)
        C_SRCS += $(APP_PATH)/test_cs35l41.c
    endif

    INCLUDES += -I$(DRIVER_PATH)/bsp
    INCLUDES += -I$(HALO_FIRMWARE_PATH)
    INCLUDES += -I$(COMMON_PATH)/bridge

    ADD_OBJ_RULES = add_platform_obj_rules
endif

# Assign automatic variables for target recipes below
$(eval $(call assign_objs))
$(eval $(call assign_build_paths))

# Assign bullseye vars (if a bullseye build)
$(eval $(call add_bullseye_vars))
BULLSEYE_INCLUDE_PATHS += $(REPO_PATH)/cs35l41/
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/cs35l41/bsp/
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/cs35l41/unit_test/
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/cs35l41/system_test/

# Print out make variables
$(if $(filter-out clean, $(MAKECMDGOALS)), $(eval $(call print_vars, MAKE_PRE_SESSION_GLOBALS, $(MAKE_PRE_SESSION_GLOBALS))))

##############################################################################
# Target Rules
##############################################################################

.PHONY: default all build_path clean
default: print_targets

# Create directories for the build detritus
$(eval $(call add_build_dir_rules, $(BUILD_DIR), $(BUILD_PATHS)))

# Add any bullseye includes/excludes (if a bullseye build)
$(eval $(call add_bullseye_rules, $(BULLSEYE_INCLUDE_PATHS), $(BULLSEYE_EXCLUDE_PATHS)))

# Code generation rules
$(eval $(call add_wisce_script_rule,cs35l41))

.NOTPARALLEL: firmware_converter
firmware_converter:
	@echo -------------------------------------------------------------------------------
	@echo GENERATING cs35l41_fw_img
	cd $(HALO_FIRMWARE_PATH) && python3 ../../tools/firmware_converter/firmware_converter.py fw_img_v2 cs35l41 $(HALO_FIRMWARE_FILE) --sym-input $(CONFIG_PATH)/cs35l41_sym.h
	cd $(HALO_FIRMWARE_PATH) && python3 ../../tools/firmware_converter/firmware_converter.py fw_img_v2 cs35l41 $(HALO_CAL_FIRMWARE_FILE) --suffix cal --sym-input $(CONFIG_PATH)/cs35l41_sym.h --wmdr-only --wmdr $(HALO_CAL_FIRMWARE_WMDR)
	cd $(HALO_FIRMWARE_PATH) && python3 ../../tools/firmware_converter/firmware_converter.py fw_img_v2 cs35l41 halo_cspl_RAM_revB2_29.45.0.wmfw --sym-input $(CONFIG_PATH)/cs35l41_sym.h --wmdr-only --suffix tune --wmdr Protect_Lite_full_6.43.0_7.0ohm_delta1ohm_L41_revB2.bin

# Compilation rules
$(eval $(call $(ADD_OBJ_RULES), $(OBJS)))
$(eval $(call add_driver_obj_rules))
$(eval $(call add_asm_obj_rules))
$(eval $(call add_driver_library_rule))

# Final target rules
$(eval $(call add_unit_test_rule))
$(eval $(call add_platform_target_rule, $(PLATFORM_TARGETS)))

print_targets:
	@echo ERROR:  No valid target specified.
	@echo
	@echo Valid targets:
	@echo       baremetal
	@echo       freertos
	@echo       system_test
	@echo       unit_test
	@echo
	@echo Valid command line variables:
	@echo		BULLSEYE=0	\(will disable Bullseye coverage\)
	@echo		BULLSEYE=1	\(will enable Bullseye coverage\)
	@echo
	@echo       OPTIMIZATION_LEVEL=0    \(configure for -O0 optimization level\)
	@echo       OPTIMIZATION_LEVEL=1    \(configure for -O1 optimization level\)
	@echo       OPTIMIZATION_LEVEL=2    \(configure for -O2 optimization level\)
	@echo       OPTIMIZATION_LEVEL=3    \(configure for -O3 optimization level\)
	@echo       OPTIMIZATION_LEVEL=s    \(configure for -Os optimization level\)

clean:
	$(RM) $(BUILD_DIR)
