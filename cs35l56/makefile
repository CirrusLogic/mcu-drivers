##############################################################################
#
# Makefile for CS35L56 Alt-OS driver
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
PART_NUM = cs35l56
$(eval $(call assign_paths))
PLATFORM_TARGETS = baremetal cl_mcu
VALID_TARGETS = unit_test $(PLATFORM_TARGETS)
$(eval $(call eval_targets))
$(eval $(call eval_optimization_level))

# Include platform_bsp.mk
ifdef IS_NOT_UNIT_TEST
    ifeq ($(MAKECMDGOALS), blackstar)
        PLATFORM=blackstar
    else ifeq ($(MAKECMDGOALS), cl_mcu)
        PLATFORM=cl_mcu
    else
        ifeq ($(MAKECMDGOALS), blackstack)
            PLATFORM=blackstar
        else
            PLATFORM=eestm32int
        endif
    endif
    include $(REPO_PATH)/common/platform_bsp/platform_bsp.mk
endif

# Assign firmware and configuration script variables
#HALO_FIRMWARE_FILE = ROM
ifeq ($(MAKECMDGOALS), baremetal)
FIRMWARE_CONV_TARGET = export
FIRMWARE_HEADER = ./cs35l56_firmware.h
HALO_FIRMWARE_WMDR = CS35L56_LT.bin CS35L56_RT.bin --preserve-filename
else  ifeq ($(MAKECMDGOALS), cl_mcu)
FIRMWARE_CONV_TARGET = export
FIRMWARE_HEADER = ./cs35l56_firmware.h
HALO_FIRMWARE_WMDR = CS35L56_LT.bin CS35L56_RT.bin --preserve-filename
endif
HALO_FIRMWARE_FILE = CS35L56_Rev4.7.2.wmfw
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
    CFLAGS += -DCS35L56_BAREMETAL
endif

# Assign sources and includes for driver library
DRIVER_SRCS = $(DRIVER_PATH)/cs35l56.c
ifeq ($(MAKECMDGOALS), baremetal)

else
    DRIVER_SRCS += $(COMMON_PATH)/fw_img.c
endif
DRIVER_SRCS += $(COMMON_PATH)/regmap.c
INCLUDES += -I$(HALO_FIRMWARE_PATH)

# Assign sources and includes for target project build
$(eval $(call assign_sources_includes))
C_SRCS += $(APP_PATH)/main.c

ifeq ($(MAKECMDGOALS), cl_mcu)
    C_SRCS += $(DRIVER_PATH)/bsp/bsp_cl_mcu.c
endif

ifeq ($(MAKECMDGOALS), baremetal)
    C_SRCS += $(HALO_FIRMWARE_PATH)/cs35l56_firmware.c
    C_SRCS += $(DRIVER_PATH)/bsp/bsp_cs35l56_baremetal.c
endif

INCLUDES += -I$(DRIVER_PATH)/bsp
ADD_OBJ_RULES = add_platform_obj_rules

# Assign automatic variables for target recipes below
$(eval $(call assign_objs))
$(eval $(call assign_build_paths))

# Assign bullseye vars (if a bullseye build)
$(eval $(call add_bullseye_vars))
BULLSEYE_INCLUDE_PATHS += $(REPO_PATH)/cs35l56/
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/cs35l56/bsp/

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

.NOTPARALLEL: firmware_converter
firmware_converter:
	@echo -------------------------------------------------------------------------------
	@echo GENERATING cs35l56_fw_img
	cd $(HALO_FIRMWARE_PATH) && python3 ../../tools/firmware_converter/firmware_converter.py $(FIRMWARE_CONV_TARGET) cs35l56 $(HALO_FIRMWARE_FILE) --wmdr $(HALO_FIRMWARE_WMDR)

# Compilation rules
$(eval $(call $(ADD_OBJ_RULES), $(OBJS)))
$(eval $(call add_driver_obj_rules))
$(eval $(call add_asm_obj_rules))
$(eval $(call add_driver_library_rule))

$(eval $(call add_unit_test_rule))
$(eval $(call add_platform_target_rule, $(PLATFORM_TARGETS)))

print_targets:
	@echo ERROR:  No valid target specified.
	@echo
	@echo Valid targets:
	@echo       baremetal cl_mcu
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
	@echo

clean:
	$(RM) $(BUILD_DIR)
