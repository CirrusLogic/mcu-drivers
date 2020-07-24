##############################################################################
#
# Makefile for OTP Unpacker
#
##############################################################################
# TODO:  enter license info
#
#
#
#

# Variables for shell commands
RM              = rm -rf

# Variables for primary paths
ROOT_PATH       = $(abspath ../../../)
REPO_PATH       = $(abspath ../../)
BUILD_DIR       = $(REPO_PATH)/build
MAKEFILE_PATH   = $(PWD)

# Initialize source/dependency variables
ASM_SRCS =
C_SRCS =
OBJS =
ASM_OBJS =
INCLUDES =
LIBS =

# Initialize toolchain arguments
ifeq ($(BULLSEYE), 1)
	BULLSEYE_ON_CMD = bullseye_on
	BULLSEYE_OFF_CMD = bullseye_off
else
	BULLSEYE_ON_CMD =
	BULLSEYE_OFF_CMD =
endif

# Process command line variables per make goal

# Assign toolchain, shell executable names required per target
ifeq ($(MAKECMDGOALS), unit_test)
    CC          = gcc
    LD          = gcc
    AR          = ar
    SIZE        = size
    IS_VALID_BUILD = YES
else ifeq ($(MAKECMDGOALS), libotp_unpacker)
    CC          = gcc
    LD          = gcc
    AR          = ar
    SIZE        = size
    IS_VALID_BUILD = YES
else ifeq ($(MAKECMDGOALS), baremetal)
    CC          = arm-none-eabi-gcc
    LD          = arm-none-eabi-ld
    AR          = arm-none-eabi-ar
    AS          = arm-none-eabi-gcc
    OBJCOPY     = arm-none-eabi-objcopy
    OBJDUMP     = arm-none-eabi-objdump
    SIZE        = arm-none-eabi-size
    IS_VALID_BUILD = YES
endif

# Assign compiler, assembler, linker flags required per target
ifeq ($(MAKECMDGOALS), unit_test)
    #CFLAGS      += -Werror -Wall -g -std=c99
    CFLAGS      += -g
    OUTPUT_NAME = unit_test
    OUTPUT_EXT  = exe
    LIBS        = -lm
    ARFLAGS     = rcs
    LDFLAGS     =
    ASM_FLAGS   =
else ifeq ($(MAKECMDGOALS), baremetal)
    DEBUG_OPTIONS += -DDEBUG
    DEBUG_OPTIONS += -g3

    # use = or += not := else dependencies expand to nil
    CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs
    CFLAGS += -Werror -Wall --std=gnu11
    CFLAGS += -fno-diagnostics-show-caret
    CFLAGS += -fdata-sections -ffunction-sections -fstack-usage
    CFLAGS += -DUSE_HAL_DRIVER -DSTM32F401xE
    CFLAGS += $(DEBUG_OPTIONS)
    CFLAGS += $(APP_OPTIONS)
    CFLAGS += -DNO_OS
    OUTPUT_NAME =
    OUTPUT_EXT  =
    ARFLAGS     = rcs
    ASM_FLAGS = -mcpu=cortex-m4 -c -x assembler-with-cpp --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
    ASM_FLAGS += $(DEBUG_OPTIONS)
    LDFLAGS += -T"$(TEST_HW_PATH)/STM32F401RETX_FLASH.ld"
    LDFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nosys.specs --specs=nano.specs
    LDFLAGS += -Wl,-Map="$(BUILD_DIR)/baremetal.map"
    LDFLAGS += -Wl,--gc-sections
    LDFLAGS += -static
    LDFLAGS += -Wl,--start-group -lc -lm -Wl,--end-group
endif

# Assign sources, includes, paths for otp_unpacker library
DRIVER_PATH = $(REPO_PATH)/cs35l41
COMMON_PATH = $(REPO_PATH)/common
OTP_UNPACKER_PATH = $(REPO_PATH)/cs35l41/otp_unpacker
OTP_UNPACKER_SRCS = $(OTP_UNPACKER_PATH)/otp_unpacker.c $(DRIVER_PATH)/cs35l41.c
INCLUDES += -I$(COMMON_PATH) -I$(DRIVER_PATH) -I$(OTP_UNPACKER_PATH)  
OTP_UNPACKER_LIBRARY = $(BUILD_DIR)/cs35l41/otp_unpacker/libotp_unpacker.a
LIBS += $(OTP_UNPACKER_LIBRARY)

ifeq ($(MAKECMDGOALS), unit_test)
    APP_PATH = $(REPO_PATH)/cs35l41/otp_unpacker/unit_test
    C_SRCS += $(APP_PATH)/main.c
    C_SRCS += $(APP_PATH)/test_otp_unpacker.c
    C_SRCS += $(REPO_PATH)/third_party/unity/unity.c
    C_SRCS += $(REPO_PATH)/third_party/unity/unity_fixture.c
    INCLUDES += -I$(APP_PATH)
    INCLUDES += -I$(REPO_PATH)/third_party/unity
endif

ifeq ($(MAKECMDGOALS), baremetal)
    APP_PATH = $(REPO_PATH)/cs35l41/otp_unpacker/baremetal
    TEST_HW_PATH = $(COMMON_PATH)/system_test_hw_0
    C_SRCS += $(APP_PATH)/main.c
    C_SRCS += $(TEST_HW_PATH)/hw_0_bsp.c
    C_SRCS += $(TEST_HW_PATH)/test_tone_tables.c
    C_SRCS += $(TEST_HW_PATH)/stm32f4xx_it.c
ifneq ($(SEMIHOSTING), 1)
    C_SRCS += $(TEST_HW_PATH)/syscalls.c
endif
    C_SRCS += $(TEST_HW_PATH)/sysmem.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    C_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
    ASM_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f401xe.s
    INCLUDES += -I$(APP_PATH)
    INCLUDES += -I$(REPO_PATH)/cs35l41/bsp
    INCLUDES += -I$(TEST_HW_PATH)
    INCLUDES += -I$(REPO_PATH)/third_party/arm/cmsis/Include
    INCLUDES += -I$(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    INCLUDES += -I$(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Inc
endif


# Defines after all target-specific variables set
OBJS = $(subst $(REPO_PATH),$(BUILD_DIR),$(C_SRCS:.c=.o))
ASM_OBJS = $(subst $(REPO_PATH),$(BUILD_DIR),$(ASM_SRCS:.s=.o))
OTP_UNPACKER_OBJS = $(subst $(REPO_PATH),$(BUILD_DIR),$(OTP_UNPACKER_SRCS:.c=.o))
BUILD_PATHS = $(sort $(dir $(OTP_UNPACKER_OBJS)))
BUILD_PATHS += $(sort $(dir $(OBJS)))
BUILD_PATHS += $(sort $(dir $(ASM_OBJS)))

# Print info, only if not cleaning
# don't delete this, it's ugly, but it needs 2 blank lines
define newline


endef

ifdef IS_VALID_BUILD
$(info )
$(info ===============================================================================)
$(info PATH:)
$(info $(PATH))
$(info -------------------------------------------------------------------------------)
$(info ROOT_PATH:       $(ROOT_PATH))
$(info REPO_PATH:       $(REPO_PATH))
$(info DRIVER_PATH:     $(DRIVER_PATH))
$(info APP_PATH:        $(APP_PATH))
$(info BUILD_DIR:       $(BUILD_DIR))
$(info BUILD_PATHS:     $(addprefix $(newline),$(BUILD_PATHS)))
$(info -------------------------------------------------------------------------------)
$(info INCLUDES: $(addprefix $(newline),$(INCLUDES)))
$(info -------------------------------------------------------------------------------)
$(info OTP_UNPACKER_SRCS: $(addprefix $(newline),$(OTP_UNPACKER_SRCS)))
$(info -------------------------------------------------------------------------------)
$(info OTP_UNPACKER_OBJS: $(addprefix $(newline),$(OTP_UNPACKER_OBJS)))
$(info -------------------------------------------------------------------------------)
$(info C_SRCS: $(addprefix $(newline),$(C_SRCS)))
$(info -------------------------------------------------------------------------------)
$(info ASM_SRCS: $(addprefix $(newline),$(ASM_SRCS)))
$(info -------------------------------------------------------------------------------)
$(info OBJS: $(addprefix $(newline),$(OBJS)))
$(info -------------------------------------------------------------------------------)
$(info ASM_OBJS: $(addprefix $(newline),$(ASM_OBJS)))
$(info -------------------------------------------------------------------------------)
$(info DEFINES: $(addprefix $(newline),$(DEFINES)))
$(info -------------------------------------------------------------------------------)
$(info CFLAGS: $(addprefix $(newline),$(CFLAGS)))
$(info -------------------------------------------------------------------------------)
$(info ASM_FLAGS: $(addprefix $(newline),$(ASM_FLAGS)))
$(info -------------------------------------------------------------------------------)
$(info LDFLAGS: $(addprefix $(newline),$(LDFLAGS)))
$(info ===============================================================================)
$(info )
endif

# Now for all the targets

.PHONY: default all build_path clean
default: print_targets
#all: build_path $(BUILD_PATHS)

# Create the 'build' directory
build_path:
	@echo -------------------------------------------------------------------------------
	@echo CREATING build folders.
	@mkdir -p $(BUILD_DIR)

# Create directories for the build detritus
define mkdir_rule
.PHONY: $(1)
$(1):
	@mkdir -p $(1)
endef
$(foreach path,$(BUILD_PATHS),$(eval $(call mkdir_rule,$(path))))

# Create a target for each .o files, depending on its corresponding .c file
define obj_rule
$(1): $(2) $(MAKEFILE_PATH)/makefile
	@echo -------------------------------------------------------------------------------
	@echo COMPILING $(2) #to $(1)
	$(CC) $(CFLAGS) $(INCLUDES) -MD -MP -MT $(1) -MF $(subst .o,.d,$(1)) -c $(2) -o $(1)
endef
$(foreach obj,$(OTP_UNPACKER_OBJS),$(eval $(call obj_rule,$(obj),$(subst $(BUILD_DIR),$(REPO_PATH),$(obj:.o=.c)))))

# Create a target for each .o files, depending on its corresponding .c file
define obj_rule
$(1): $(2) $(MAKEFILE_PATH)/makefile
	@echo -------------------------------------------------------------------------------
	@echo COMPILING $(2) #to $(1)
	$(CC) $(CFLAGS) $(INCLUDES) -MD -MP -MT $(1) -MF $(subst .o,.d,$(1)) -c $(2) -o $(1)
endef
$(foreach obj,$(OBJS),$(eval $(call obj_rule,$(obj),$(subst $(BUILD_DIR),$(REPO_PATH),$(obj:.o=.c)))))

# Create a target for each .o file, depending on its corresponding .s file
define asm_obj_rule
$(1): $(2) $(MAKEFILE_PATH)/makefile
	@echo -------------------------------------------------------------------------------
	@echo ASSEMBLING $(2) #to $(1)
	$(AS) $(ASM_FLAGS) $(INCLUDES) -MD -MP -MT $(1) -MF $(subst .o,.d,$(1)) -c $(2) -o $(1)
endef
$(foreach obj,$(ASM_OBJS),$(eval $(call asm_obj_rule,$(obj),$(subst $(BUILD_DIR),$(REPO_PATH),$(obj:.o=.s)))))

# Enable bullseye build
bullseye_on:
	@echo -------------------------------------------------------------------------------
	@echo BULLSEYE ON
	cov01 --on
	covselect --deleteAll
	covselect --add !$(REPO_PATH)/cs35l41/
	covselect --add !$(REPO_PATH)/cs35l41/unit_test/
	covselect --add $(REPO_PATH)/cs35l41/otp_unpacker/
	covselect --add !$(REPO_PATH)/cs35l41/otp_unpacker/unit_test/
	covselect --add !$(REPO_PATH)/cs35l41/system_test/
	covselect --add !$(REPO_PATH)/build/
	covselect --add !$(REPO_PATH)/common/
	covselect --add !$(REPO_PATH)/tools/
	covselect --add !$(REPO_PATH)/third_party/

# Disable bullseye build
bullseye_off:
	@echo -------------------------------------------------------------------------------
	@echo BULLSEYE OFF
	cov01 --off

libotp_unpacker: build_path $(BUILD_PATHS) $(OTP_UNPACKER_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo ARCHIVING $(OTP_UNPACKER_LIBRARY)
	$(AR) $(ARFLAGS) $(OTP_UNPACKER_LIBRARY) $(OTP_UNPACKER_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo SIZE of $(OTP_UNPACKER_LIBRARY)
	$(SIZE) -t $(OTP_UNPACKER_LIBRARY)

unit_test_pre_build: build_path $(BUILD_PATHS) $(BULLSEYE_ON_CMD)

unit_test_build: libotp_unpacker $(OBJS)
	@echo -------------------------------------------------------------------------------
	@echo LINKING $@
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $(BUILD_DIR)/unit_test.exe

unit_test_post_build: $(BULLSEYE_OFF_CMD)

unit_test: unit_test_pre_build unit_test_build unit_test_post_build
	@echo -------------------------------------------------------------------------------
	@echo RUNNING $@
	@echo -------------------------------------------------------------------------------
	$(BUILD_DIR)/unit_test.exe

baremetal: build_path $(BUILD_PATHS) libotp_unpacker $(OBJS) $(ASM_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo LINKING $@
	$(CC) $(LDFLAGS) $(OBJS) $(ASM_OBJS) $(LIBS) -o $(BUILD_DIR)/baremetal.elf
	@echo -------------------------------------------------------------------------------
	@echo SIZE of baremetal.elf
	$(SIZE) -t $(BUILD_DIR)/baremetal.elf

print_targets:
	@echo ERROR:  No valid target specified.
	@echo
	@echo Valid targets:
	@echo       libotp_unpacker
	@echo		unit_test
	@echo       baremetal
	@echo
	@echo Valid command line variables:
	@echo		BULLSEYE=0	\(will disable Bullseye coverage\)
	@echo		BULLSEYE=1	\(will enable Bullseye coverage\)

clean:
	$(RM) $(BUILD_DIR)
