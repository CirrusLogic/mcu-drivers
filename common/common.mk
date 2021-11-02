##############################################################################
#
# Common resources for Alt-OS driver makefiles
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

# Variables for shell commands
RM = rm -rf

# Common variables
CFLAGS =
ASM_FLAGS =
LD_FLAGS =
LDFLAGS =
ARFLAGS =
PLFLAGS =
C_SRCS =
ASM_SRCS =
INCLUDES =
DRIVER_OBJS =
OBJS =
ASM_OBJS =
LIBS =
BUILD_PATHS =

# Print info, only if not cleaning
# don't delete this, it's ugly, but it needs 2 blank lines
define newline


endef

# Create directories for the build detritus
define mkdir_rule
.PHONY: $(1)
$(1):
	@mkdir -p $(1)
endef

# Create the 'build' directory
define add_build_dir_rules
.PHONY: build_path
build_path:
	@echo -------------------------------------------------------------------------------
	@echo CREATING build folders.
	@mkdir -p $(1)

$(eval BUILD_PATH_RULE=build_path)
$(foreach path,$(2),$(eval $(call mkdir_rule,$(path))))
endef

# Create a target for each .o files, depending on its corresponding .c file
define obj_rule
$(1): $(2)
	@echo -------------------------------------------------------------------------------
	@echo COMPILING $(2)
	$(CC) $(CFLAGS) $(INCLUDES) -MD -MP -MT $(1) -MF $(subst .o,.d,$(1)) $(2) -o $(1)
endef
define add_unit_test_obj_rules
$(foreach obj,$(OBJS),$(eval $(call obj_rule,$(obj),$(subst $(BUILD_DIR),$(REPO_PATH),$(obj:.o=.c)))))
endef
define add_platform_obj_rules
$(foreach obj,$(filter-out $(PLATFORM_MCU_BUILD_PATH)%,$(OBJS)),$(eval $(call obj_rule,$(obj),$(subst $(BUILD_DIR),$(REPO_PATH),$(obj:.o=.c)))))
$(foreach obj,$(filter $(PLATFORM_MCU_BUILD_PATH)%,$(OBJS)),$(eval $(call obj_rule,$(obj),$(subst $(PLATFORM_MCU_BUILD_PATH),$(PLATFORM_MCU_SRC_PATH),$(obj:.o=.c)))))
endef

# Create a target for each .o files, depending on its corresponding .c file
define driver_obj_rule
$(1): $(2)
	@echo -------------------------------------------------------------------------------
	@echo COMPILING $(2)
	$(BULLSEYE_ON_CMD)
	$(CC) $(CFLAGS) $(INCLUDES) -MD -MP -MT $(1) -MF $(subst .o,.d,$(1)) $(2) -o $(1)
	$(BULLSEYE_OFF_CMD)
endef
define add_driver_obj_rules
    $(foreach obj,$(DRIVER_OBJS),$(eval $(call driver_obj_rule,$(obj),$(subst $(BUILD_DIR),$(REPO_PATH),$(obj:.o=.c)))))
endef

# Create a target for each .o file, depending on its corresponding .s file
define asm_obj_rule
$(1): $(2)
	@echo -------------------------------------------------------------------------------
	@echo ASSEMBLING $(2)
	$(AS) $(ASM_FLAGS) $(INCLUDES) -MD -MP -MT $(1) -MF $(subst .o,.d,$(1)) $(2) -o $(1)
endef
define add_asm_obj_rules
    $(foreach obj,$(filter-out $(PLATFORM_MCU_BUILD_PATH)%,$(ASM_OBJS)),$(eval $(call asm_obj_rule,$(obj),$(subst $(BUILD_DIR),$(REPO_PATH),$(obj:.o=.s)))))
    $(foreach obj,$(filter $(PLATFORM_MCU_BUILD_PATH)%,$(ASM_OBJS)),$(eval $(call asm_obj_rule,$(obj),$(subst $(PLATFORM_MCU_BUILD_PATH),$(PLATFORM_MCU_SRC_PATH),$(obj:.o=.s)))))
endef

# Print all vars created during the makefile (minus the functions)
define print_vars
$(info )
$(info ===============================================================================)
$(info PATH:)
$(info $(PATH))
$(foreach v, \
    $(sort $(filter-out $(2) $(1) print_vars \
    newline \
    mkdir_rule \
    add_build_dir_rules \
    obj_rule \
    add_unit_test_obj_rules \
    add_platform_obj_rules \
    asm_obj_rule \
    add_asm_obj_rules \
    driver_obj_rule \
    add_driver_obj_rules \
    setup_bullseye \
    add_bullseye_rules \
    add_wisce_script_rule \
    add_create_syscfg_regs_rule \
    assign_paths \
    eval_optimization_level \
	eval_targets \
    assign_toolchain \
	assign_toolchain_flags \
	add_unit_test_rule \
	add_platform_target_rule \
	assign_sources_includes \
    add_driver_library_rule \
	assign_objs \
	assign_build_paths \
    add_bullseye_vars,$(.VARIABLES))), \
    $(info -------------------------------------------------------------------------------) \
    $(info $(v): $(addprefix $(newline),$($(v))))) \
    $(info -------------------------------------------------------------------------------)
$(info ===============================================================================)
$(info )
endef

# Add rule for including/excluding directories from Bullseye's attention
define add_bullseye_rules
bullseye_inclusions:
	@echo -------------------------------------------------------------------------------
	@echo BULLSEYE INCLUSIONS
	covselect --deleteAll
	covselect --add $(foreach source_path, $(1), $(source_path))
	covselect --add $(foreach source_path, $(2), !$(source_path))
endef

# Add rule for calling wisce_script_converter
define add_wisce_script_rule
$(eval CREATE_SYSCFG_REGS_TARGET=wisce_to_syscfg_reg_converter)
.PHONY: wisce_to_syscfg_reg_converter
wisce_to_syscfg_reg_converter:
	@echo -------------------------------------------------------------------------------
	@echo GENERATING $(1)_syscfg_regs
	cd $(CONFIG_PATH) && python3 ../../tools/wisce_script_converter/wisce_script_converter.py -c c_array -p $(1) -i $(WISCE_SCRIPT) -o .
endef

define add_create_syscfg_regs_rule
$(eval $(call add_wisce_script_rule,$(1)))
endef

# Assign common paths
define assign_paths
BUILD_DIR = $(REPO_PATH)/build
DRIVER_PATH = $(REPO_PATH)/$(PART_NUM)
HALO_FIRMWARE_PATH = $(REPO_PATH)/$(PART_NUM)/fw
APP_PATH = $(REPO_PATH)/$(PART_NUM)/$(MAKECMDGOALS)
CONFIG_PATH = $(REPO_PATH)/$(PART_NUM)/config
COMMON_PATH = $(REPO_PATH)/common
endef

# Evaluate OPTIMIZATION_LEVEL make argument
define eval_optimization_level
ifeq ($(OPTIMIZATION_LEVEL), 0)
    DEBUG_OPTIONS =
    OPTIMIZATION_OPTIONS=-O0
else ifeq ($(OPTIMIZATION_LEVEL), 1)
    DEBUG_OPTIONS =
    OPTIMIZATION_OPTIONS=-O1
else ifeq ($(OPTIMIZATION_LEVEL), 2)
    DEBUG_OPTIONS =
    OPTIMIZATION_OPTIONS=-O2
else ifeq ($(OPTIMIZATION_LEVEL), 3)
    DEBUG_OPTIONS =
    OPTIMIZATION_OPTIONS=-O3
else ifeq ($(OPTIMIZATION_LEVEL), s)
    DEBUG_OPTIONS =
    OPTIMIZATION_OPTIONS=-Os
else
    DEBUG_OPTIONS += -DDEBUG
    DEBUG_OPTIONS += -g3
    OPTIMIZATION_OPTIONS=-O0
endif
endef

# Check for whether the target is clean, unit_test, or others
define eval_targets
ifneq ($(filter $(MAKECMDGOALS),$(VALID_TARGETS)),)
    IS_VALID_BUILD = 1
    ifneq ($(MAKECMDGOALS), unit_test)
        IS_NOT_UNIT_TEST = 1
    endif
endif
ifeq ($(MAKECMDGOALS), freertos)
    USE_FREERTOS = 1
endif
endef

# Assign toolchain variables
define assign_toolchain
ifeq ($(MAKECMDGOALS), unit_test)
    CC          = gcc
    LD          = ld
    AR          = ar
    SIZE        = size
else ifdef IS_NOT_UNIT_TEST
    CC          = arm-none-eabi-gcc
    LD          = arm-none-eabi-ld
    AR          = arm-none-eabi-ar
    AS          = arm-none-eabi-gcc
    OBJCOPY     = arm-none-eabi-objcopy
    OBJDUMP     = arm-none-eabi-objdump
    SIZE        = arm-none-eabi-size
endif
endef

# Assign the common toolchain flags for common targets
define assign_toolchain_flags
ifeq ($(MAKECMDGOALS), unit_test)
    CFLAGS += -g -std=c99 -c
    CFLAGS += -DUSE_BULLSEYE
    CFLAGS += $(DEBUG_OPTIONS)
    CFLAGS += $(OPTIMIZATION_OPTIONS)

    ARFLAGS += rcs

    PLFLAGS += -EL -r
else ifdef IS_NOT_UNIT_TEST
    ifneq ($(MAKECMDGOALS), system_test)
        CFLAGS += -Werror -Wall 
    endif
    CFLAGS += --std=gnu11
    CFLAGS += -fno-diagnostics-show-caret
    CFLAGS += -fdata-sections -ffunction-sections -fstack-usage
    CFLAGS += -frecord-gcc-switches
    CFLAGS += -c
    CFLAGS += $(DEBUG_OPTIONS)
    CFLAGS += $(OPTIMIZATION_OPTIONS)
    ifeq ($(MAKECMDGOALS), freertos)
        CFLAGS += -DUSE_CMSIS_OS
    else
        CFLAGS += -DNO_OS
    endif
    ifeq ($(MAKECMDGOALS), system_test)
        CFLAGS += -DUNITY_INCLUDE_CONFIG_H
        CFLAGS += -DUSE_BULLSEYE
    endif

    ASM_FLAGS += -c -x assembler-with-cpp
    ASM_FLAGS += $(DEBUG_OPTIONS)

    ARFLAGS += rcs

    PLFLAGS += -EL -r

    LDFLAGS += -Wl,-Map="$(BUILD_DIR)/$(MAKECMDGOALS).map"
    LDFLAGS += -Wl,--gc-sections
    LDFLAGS += -static
    LDFLAGS += -Wl,--start-group -lc -lm -Wl,--end-group
endif
endef

# Add make rule for unit_test target
define add_unit_test_rule
unit_test: build_path $(BUILD_PATHS) $(LIBS) $(OBJS)
	@echo -------------------------------------------------------------------------------
	@echo LINKING $@
	$(BULLSEYE_ON_CMD)
	$(CC) $(LDFLAGS) $(OBJS) -lm $(LIBS) -o $(BUILD_DIR)/unit_test.exe
	$(BULLSEYE_OFF_CMD)
	@echo -------------------------------------------------------------------------------
	@echo RUNNING $@
	@echo -------------------------------------------------------------------------------
	$(BUILD_DIR)/unit_test.exe
endef

# Add rule for building a target with a hardware platform
define add_platform_target_rule
$(1): build_path $(BUILD_PATHS) firmware_converter $(LIBS) $(OBJS) $(PLATFORM_OBJS) $(ASM_OBJS) $(PLATFORM_ASM_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo LINKING $@
	$(CC) $(LDFLAGS) $(OBJS) $(PLATFORM_OBJS) $(ASM_OBJS) $(PLATFORM_ASM_OBJS) $(LIBS) -o $(BUILD_DIR)/$(MAKECMDGOALS).elf
	@echo -------------------------------------------------------------------------------
	@echo SIZE of $(MAKECMDGOALS).elf
	$(SIZE) -t $(BUILD_DIR)/$(MAKECMDGOALS).elf
endef

# Assign common source, includes, and libraries
define assign_sources_includes
    INCLUDES += -I$(REPO_PATH)
    INCLUDES += -I$(DRIVER_PATH)
    INCLUDES += -I$(CONFIG_PATH)
    INCLUDES += -I$(APP_PATH)
    INCLUDES += -I$(COMMON_PATH)

    DRIVER_LIBRARY = $(BUILD_DIR)/$(PART_NUM)/lib$(PART_NUM).a
	LIBS += $(BUILD_DIR)/$(PART_NUM)/lib$(PART_NUM).a
ifeq ($(MAKECMDGOALS), unit_test)
    C_SRCS += $(REPO_PATH)/third_party/unity/unity.c
    C_SRCS += $(REPO_PATH)/third_party/unity/unity_fixture.c

    INCLUDES += -I$(REPO_PATH)/third_party/unity
else ifeq ($(MAKECMDGOALS), system_test)
    C_SRCS += $(REPO_PATH)/third_party/bullseye/src/libcov-altos.c
    C_SRCS += $(REPO_PATH)/third_party/unity/unity.c
    C_SRCS += $(REPO_PATH)/third_party/unity/unity_fixture.c

    INCLUDES += -I$(REPO_PATH)/third_party/bullseye/inc
    INCLUDES += -I$(REPO_PATH)/third_party/unity
endif
endef

# Add rule for building driver library
define add_driver_library_rule
$(DRIVER_LIBRARY): $(BUILD_PATH_RULE) $(BUILD_PATHS) $(BULLSEYE_INCLUSIONS) $(CREATE_SYSCFG_REGS_TARGET) $(DRIVER_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo ARCHIVING $(DRIVER_LIBRARY)
	$(AR) $(ARFLAGS) $(DRIVER_LIBRARY) $(DRIVER_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo SIZE of $(DRIVER_LIBRARY)
	$(SIZE) -t $(DRIVER_LIBRARY)
endef

# Assign object files (OBJS) for driver library, c sources, and assembly sources
define assign_objs
DRIVER_OBJS += $(subst $(REPO_PATH),$(BUILD_DIR),$(DRIVER_SRCS:.c=.o))
OBJS += $(subst $(REPO_PATH),$(BUILD_DIR),$(C_SRCS:.c=.o))
OBJS += $(subst $(PLATFORM_MCU_SRC_PATH),$(PLATFORM_MCU_BUILD_PATH),$(PLATFORM_MCU_C_SRCS:.c=.o))
ASM_OBJS += $(subst $(REPO_PATH),$(BUILD_DIR),$(ASM_SRCS:.s=.o))
ASM_OBJS += $(subst $(PLATFORM_MCU_SRC_PATH),$(PLATFORM_MCU_BUILD_PATH),$(PLATFORM_MCU_ASM_SRCS:.s=.o))
endef

# Assign the build paths for each type of OBJ list
define assign_build_paths
BUILD_PATHS += $(sort $(dir $(DRIVER_OBJS)))
BUILD_PATHS += $(sort $(dir $(OBJS)))
BUILD_PATHS += $(sort $(dir $(ASM_OBJS)))
endef

# Assign Bullseye-related variables
define add_bullseye_vars
BULLSEYE_INCLUDE_PATHS =
BULLSEYE_INCLUDE_PATHS += $(REPO_PATH)/common/
BULLSEYE_EXCLUDE_PATHS =
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/common/platform_bsp/
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/build/
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/tools/
BULLSEYE_EXCLUDE_PATHS += $(REPO_PATH)/third_party/
$(if $(filter $(BULLSEYE),1), $(eval BULLSEYE_INCLUSIONS=bullseye_inclusions) $(eval BULLSEYE_ON_CMD=cov01 --on) $(eval BULLSEYE_OFF_CMD=cov01 --off),)
endef
