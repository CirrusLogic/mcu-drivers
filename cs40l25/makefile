##############################################################################
#
# Makefile for CS40L25 Alt-OS driver
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
ROOT_PATH       = $(abspath ../../)
REPO_PATH       = $(abspath ../)
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
CFLAGS = -DTARGET_CS40L25

# Process command line variables per make goal
ifeq ($(MAKECMDGOALS), driver_lib_cm4)
	BULLSEYE_INCLUSIONS =
	BULLSEYE_ON_CMD =
	BULLSEYE_OFF_CMD =
else
    ifeq ($(BULLSEYE), 1)
    	BULLSEYE_INCLUSIONS = bullseye_inclusions
    	BULLSEYE_ON_CMD = cov01 --on
    	BULLSEYE_OFF_CMD = cov01 --off
    else
    	BULLSEYE_ON_CMD =
    	BULLSEYE_OFF_CMD =
    endif
endif

# Assign toolchain, shell executable names required per target
ifeq ($(MAKECMDGOALS), unit_test)
    CC          = gcc
    LD          = gcc
    AR          = ar
    SIZE        = size
    IS_VALID_BUILD = YES
else ifeq ($(MAKECMDGOALS), driver_lib_cm4)
    CC          = arm-none-eabi-gcc
    LD          = arm-none-eabi-ld
    AR          = arm-none-eabi-ar
    AS          = arm-none-eabi-as
    OBJCOPY     = arm-none-eabi-objcopy
    OBJDUMP     = arm-none-eabi-objdump
    SIZE        = arm-none-eabi-size
    IS_VALID_BUILD = YES
else ifeq ($(MAKECMDGOALS), system_test)
    CC          = arm-none-eabi-gcc
    LD          = arm-none-eabi-ld
    AR          = arm-none-eabi-ar
    #AS          = arm-none-eabi-as
    AS          = arm-none-eabi-gcc
    OBJCOPY     = arm-none-eabi-objcopy
    OBJDUMP     = arm-none-eabi-objdump
    SIZE        = arm-none-eabi-size
    IS_VALID_BUILD = YES
else ifeq ($(MAKECMDGOALS), baremetal)
    CC          = arm-none-eabi-gcc
    LD          = arm-none-eabi-ld
    AR          = arm-none-eabi-ar
    #AS          = arm-none-eabi-as
    AS          = arm-none-eabi-gcc
    OBJCOPY     = arm-none-eabi-objcopy
    OBJDUMP     = arm-none-eabi-objdump
    SIZE        = arm-none-eabi-size
    IS_VALID_BUILD = YES
else ifeq ($(MAKECMDGOALS), freertos)
    CC          = arm-none-eabi-gcc
    LD          = arm-none-eabi-ld
    AR          = arm-none-eabi-ar
    #AS          = arm-none-eabi-as
    AS          = arm-none-eabi-gcc
    OBJCOPY     = arm-none-eabi-objcopy
    OBJDUMP     = arm-none-eabi-objdump
    SIZE        = arm-none-eabi-size
    IS_VALID_BUILD = YES
endif

# Assign compiler, assembler, linker flags required per target
ifeq ($(MAKECMDGOALS), unit_test)
    CFLAGS      += -g -std=c99
ifeq ($(BULLSEYE), 1)
	CFLAGS += -DUSE_BULLSEYE
endif
    # include Calibration-related code
    CFLAGS += -DINCLUDE_CAL
    OUTPUT_NAME = unit_test
    OUTPUT_EXT  = exe
    LIBS        = -lm
    ARFLAGS     = rcs
    LDFLAGS     =
    ASM_FLAGS   =
else ifeq ($(MAKECMDGOALS), driver_lib_cm4)
    OPTIMIZATION_OPTIONS += -Os

    # include Calibration-related code
    CFLAGS += -DINCLUDE_CAL

    # stop winging about type-punned pointers breaking strict-aliasing rules
    CFLAGS += -fno-strict-aliasing
    CFLAGS += -ffreestanding # C99 standard n1570: A conforming freestanding implementation shall accept any strictly conforming program in which the use of the features specified in the library clause (clause 7) is confined to the contents of the standard headers <float.h>, <iso646.h>, <limits.h>, <stdalign.h>, <stdarg.h>, <stdbool.h>, <stddef.h>, <stdint.h>, and <stdnoreturn.h>.

    # use = or += not := else dependencies expand to nil
    CFLAGS += -mcpu=cortex-m4 -mthumb
    CFLAGS += -W -Werror -Wall --std=gnu99
    CFLAGS += -fno-diagnostics-show-caret
    CFLAGS += -fdata-sections -ffunction-sections
    CFLAGS += -funsigned-char -funsigned-bitfields
    CFLAGS += $(OPTIMIZATION_OPTIONS)
    CFLAGS += $(APP_OPTIONS)
    OUTPUT_NAME =
    OUTPUT_EXT  =
    ARFLAGS     = rcs
    LDFLAGS     =
    ASM_FLAGS   =
else ifeq ($(MAKECMDGOALS), system_test)
    DEBUG_OPTIONS += -DDEBUG
    DEBUG_OPTIONS += -g3

    # include Calibration-related code
    CFLAGS += -DINCLUDE_CAL

    # use = or += not := else dependencies expand to nil
    CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs
    CFLAGS += --std=gnu11
    CFLAGS += -fno-diagnostics-show-caret
    CFLAGS += -fdata-sections -ffunction-sections -fstack-usage
    CFLAGS += -DUSE_HAL_DRIVER -DSTM32F401xE
    CFLAGS += $(DEBUG_OPTIONS)
    CFLAGS += $(APP_OPTIONS)
    CFLAGS += -DSEGGER_RTT_SECTION=\"SEGGER_RTT_SECTION\"
    CFLAGS += -DNO_OS
    CFLAGS += -DUNITY_INCLUDE_CONFIG_H
ifeq ($(BULLSEYE), 1)
	CFLAGS += -DUSE_BULLSEYE
endif
    OUTPUT_NAME =
    OUTPUT_EXT  =
    ARFLAGS     = rcs
    ASM_FLAGS = -mcpu=cortex-m4 -c -x assembler-with-cpp --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
    ASM_FLAGS += $(DEBUG_OPTIONS)
    LDFLAGS += -T"$(TEST_HW_PATH)/STM32F401RETX_FLASH.ld"
    LDFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nosys.specs --specs=nano.specs
    LDFLAGS += -Wl,-Map="$(BUILD_DIR)/system_test.map"
    LDFLAGS += -Wl,--gc-sections
    LDFLAGS += -static
    LDFLAGS += -Wl,--start-group -lc -lm -Wl,--end-group
else ifeq ($(MAKECMDGOALS), baremetal)
    DEBUG_OPTIONS += -DDEBUG
    DEBUG_OPTIONS += -g3

    # include Calibration-related code
    CFLAGS += -DINCLUDE_CAL

    # use = or += not := else dependencies expand to nil
    CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs
    CFLAGS += -Werror -Wall --std=gnu11
    CFLAGS += -fno-diagnostics-show-caret
    CFLAGS += -fdata-sections -ffunction-sections -fstack-usage
    CFLAGS += -DUSE_HAL_DRIVER -DSTM32F401xE
    CFLAGS += $(DEBUG_OPTIONS)
    CFLAGS += $(APP_OPTIONS)
    CFLAGS += -DNO_OS
ifeq ($(SEMIHOSTING), 1)
    CFLAGS += -DSEMIHOSTING
endif
    OUTPUT_NAME =
    OUTPUT_EXT  =
    ARFLAGS     = rcs
    ASM_FLAGS = -mcpu=cortex-m4 -c -x assembler-with-cpp --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
    ASM_FLAGS += $(DEBUG_OPTIONS)
    LDFLAGS += -T"$(TEST_HW_PATH)/STM32F401RETX_FLASH.ld"
    LDFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nosys.specs --specs=nano.specs
ifeq ($(SEMIHOSTING), 1)
    LDFLAGS += --specs=rdimon.specs -lrdimon
endif
    LDFLAGS += -Wl,-Map="$(BUILD_DIR)/baremetal.map"
    LDFLAGS += -Wl,--gc-sections
    LDFLAGS += -static
    LDFLAGS += -Wl,--start-group -lc -lm -Wl,--end-group
else ifeq ($(MAKECMDGOALS), freertos)
    DEBUG_OPTIONS += -DDEBUG
    DEBUG_OPTIONS += -g3

    # include Calibration-related code
    CFLAGS += -DINCLUDE_CAL

    # use = or += not := else dependencies expand to nil
    CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs
    CFLAGS += -Werror -Wall --std=gnu11
    CFLAGS += -fno-diagnostics-show-caret
    CFLAGS += -fdata-sections -ffunction-sections -fstack-usage
    CFLAGS += -DUSE_HAL_DRIVER -DSTM32F401xE
    CFLAGS += $(DEBUG_OPTIONS)
    CFLAGS += $(APP_OPTIONS)
    CFLAGS += -DUSE_CMSIS_OS
ifeq ($(SEMIHOSTING), 1)
    CFLAGS += -DSEMIHOSTING
endif
    OUTPUT_NAME =
    OUTPUT_EXT  =
    ARFLAGS     = rcs
    ASM_FLAGS = -mcpu=cortex-m4 -c -x assembler-with-cpp --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
    ASM_FLAGS += $(DEBUG_OPTIONS)
    LDFLAGS += -T"$(TEST_HW_PATH)/STM32F401RETX_FLASH.ld"
    LDFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nosys.specs --specs=nano.specs
ifeq ($(SEMIHOSTING), 1)
    LDFLAGS += --specs=rdimon.specs -lrdimon
endif
    LDFLAGS += -Wl,-Map="$(BUILD_DIR)/freertos.map"
    LDFLAGS += -Wl,--gc-sections
    LDFLAGS += -static
    LDFLAGS += -Wl,--start-group -lc -lm -Wl,--end-group
endif

# Assign sources, includes, paths for driver library
DRIVER_PATH = $(REPO_PATH)/cs40l25
COMMON_PATH = $(REPO_PATH)/common
HALO_FIRMWARE_PATH = $(REPO_PATH)/cs40l25/fw_8_5_2
DRIVER_SRCS = $(DRIVER_PATH)/cs40l25.c $(DRIVER_PATH)/cs40l25_syscfg_regs.c $(COMMON_PATH)/fw_img_v1.c
INCLUDES += -I$(DRIVER_PATH) -I$(COMMON_PATH) -I$(HALO_FIRMWARE_PATH)
DRIVER_LIBRARY = $(BUILD_DIR)/cs40l25/libcs40l25.a
LIBS += $(DRIVER_LIBRARY)

# Assign list of application source files required per target
ifeq ($(MAKECMDGOALS), unit_test)
    APP_PATH = $(REPO_PATH)/cs40l25/unit_test
    C_SRCS += $(APP_PATH)/main.c
    C_SRCS += $(APP_PATH)/mock_bsp.c
    C_SRCS += $(APP_PATH)/test_cs40l25.c
    C_SRCS += $(REPO_PATH)/third_party/unity/unity.c
    C_SRCS += $(REPO_PATH)/third_party/unity/unity_fixture.c
    INCLUDES += -I$(APP_PATH)
    INCLUDES += -I$(REPO_PATH)/third_party/unity
endif

ifeq ($(MAKECMDGOALS), system_test)
    APP_PATH = $(REPO_PATH)/cs40l25/system_test
    TEST_HW_PATH = $(COMMON_PATH)/system_test_hw_0
    C_SRCS += $(APP_PATH)/main.c
    C_SRCS += $(APP_PATH)/test_cs40l25.c
    C_SRCS += $(TEST_HW_PATH)/hw_0_bsp.c
    C_SRCS += $(TEST_HW_PATH)/test_tone_tables.c
    C_SRCS += $(TEST_HW_PATH)/stm32f4xx_it.c
    C_SRCS += $(TEST_HW_PATH)/sysmem.c
    C_SRCS += $(REPO_PATH)/cs40l25/bsp/hw_0_bsp_cs40l25.c
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
    C_SRCS += $(REPO_PATH)/third_party/rtt/gcc_io.c
    C_SRCS += $(REPO_PATH)/third_party/rtt/gcc_io_rtt.c
    C_SRCS += $(REPO_PATH)/third_party/rtt/SEGGER_RTT.c
    C_SRCS += $(REPO_PATH)/third_party/rtt/SEGGER_RTT_printf.c
ifeq ($(BULLSEYE), 1)
    C_SRCS += $(REPO_PATH)/third_party/bullseye/src/libcov-cirrusrtt.c
endif
    C_SRCS += $(REPO_PATH)/third_party/unity/unity.c
    C_SRCS += $(REPO_PATH)/third_party/unity/unity_fixture.c
    ASM_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f401xe.s
    INCLUDES += -I$(APP_PATH)
    INCLUDES += -I$(REPO_PATH)/cs40l25/bsp
    INCLUDES += -I$(TEST_HW_PATH)
    INCLUDES += -I$(REPO_PATH)/third_party/arm/cmsis/Include
    INCLUDES += -I$(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    INCLUDES += -I$(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Inc
    INCLUDES += -I$(REPO_PATH)/third_party/rtt
ifeq ($(BULLSEYE), 1)
    INCLUDES += -I$(REPO_PATH)/third_party/bullseye/inc
endif
    INCLUDES += -I$(REPO_PATH)/third_party/unity
endif

ifeq ($(MAKECMDGOALS), baremetal)
    APP_PATH = $(REPO_PATH)/cs40l25/baremetal
    TEST_HW_PATH = $(COMMON_PATH)/system_test_hw_0
    C_SRCS += $(APP_PATH)/main.c
    C_SRCS += $(TEST_HW_PATH)/hw_0_bsp.c
    C_SRCS += $(TEST_HW_PATH)/test_tone_tables.c
    C_SRCS += $(TEST_HW_PATH)/stm32f4xx_it.c
ifneq ($(SEMIHOSTING), 1)
    C_SRCS += $(TEST_HW_PATH)/syscalls.c
endif
    C_SRCS += $(TEST_HW_PATH)/sysmem.c
    C_SRCS += $(REPO_PATH)/cs40l25/bsp/hw_0_bsp_cs40l25.c
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
    INCLUDES += -I$(REPO_PATH)/cs40l25/bsp
    INCLUDES += -I$(TEST_HW_PATH)
    INCLUDES += -I$(REPO_PATH)/third_party/arm/cmsis/Include
    INCLUDES += -I$(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    INCLUDES += -I$(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Inc
endif

ifeq ($(MAKECMDGOALS), freertos)
    APP_PATH = $(REPO_PATH)/cs40l25/freertos
    TEST_HW_PATH = $(COMMON_PATH)/system_test_hw_0
    FREERTOS_PATH = $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Middlewares/Third_Party/FreeRTOS/Source
    C_SRCS += $(APP_PATH)/main.c
    C_SRCS += $(TEST_HW_PATH)/hw_0_bsp.c
    C_SRCS += $(TEST_HW_PATH)/test_tone_tables.c
    C_SRCS += $(TEST_HW_PATH)/stm32f4xx_it.c
ifneq ($(SEMIHOSTING), 1)
    C_SRCS += $(TEST_HW_PATH)/syscalls.c
endif
    C_SRCS += $(TEST_HW_PATH)/sysmem.c
    C_SRCS += $(REPO_PATH)/cs40l25/bsp/hw_0_bsp_cs40l25.c
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
    C_SRCS += $(FREERTOS_PATH)/CMSIS_RTOS/cmsis_os.c
    C_SRCS += $(FREERTOS_PATH)/tasks.c
    C_SRCS += $(FREERTOS_PATH)/queue.c
    C_SRCS += $(FREERTOS_PATH)/portable/GCC/ARM_CM4F/port.c
    C_SRCS += $(FREERTOS_PATH)/list.c
    C_SRCS += $(FREERTOS_PATH)/portable/MemMang/heap_1.c
    ASM_SRCS += $(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f401xe.s
    INCLUDES += -I$(APP_PATH)
    INCLUDES += -I$(REPO_PATH)/cs40l25/bsp
    INCLUDES += -I$(TEST_HW_PATH)
    INCLUDES += -I$(REPO_PATH)/third_party/arm/cmsis/Include
    INCLUDES += -I$(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    INCLUDES += -I$(REPO_PATH)/third_party/st/STM32Cube_FW_F4_V1.24.1/Drivers/STM32F4xx_HAL_Driver/Inc
    INCLUDES += -I$(FREERTOS_PATH)/CMSIS_RTOS -I$(FREERTOS_PATH)/include -I$(FREERTOS_PATH)/portable/GCC/ARM_CM4F
endif

# Defines after all target-specific variables set
OBJS = $(subst $(REPO_PATH),$(BUILD_DIR),$(C_SRCS:.c=.o))
ASM_OBJS = $(subst $(REPO_PATH),$(BUILD_DIR),$(ASM_SRCS:.s=.o))
DRIVER_OBJS = $(subst $(REPO_PATH),$(BUILD_DIR),$(DRIVER_SRCS:.c=.o))
BUILD_PATHS = $(sort $(dir $(DRIVER_OBJS)))
BUILD_PATHS += $(sort $(dir $(OBJS)))
BUILD_PATHS += $(sort $(dir $(ASM_OBJS)))

#CFLAGS += $(INCLUDES)

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
$(info DRIVER_SRCS: $(addprefix $(newline),$(DRIVER_SRCS)))
$(info -------------------------------------------------------------------------------)
$(info DRIVER_OBJS: $(addprefix $(newline),$(DRIVER_OBJS)))
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
$(info -------------------------------------------------------------------------------)
$(info BULLSEYE: $(BULLSEYE))
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
define driver_obj_rule
$(1): $(2) $(MAKEFILE_PATH)/makefile create_syscfg_regs
	@echo -------------------------------------------------------------------------------
	@echo COMPILING $(2)
	$(BULLSEYE_ON_CMD)
	$(CC) $(CFLAGS) $(INCLUDES) -frecord-gcc-switches -MD -MP -MT $(1) -MF $(subst .o,.d,$(1)) -c $(2) -o $(1)
	$(BULLSEYE_OFF_CMD)
endef
$(foreach obj,$(DRIVER_OBJS),$(eval $(call driver_obj_rule,$(obj),$(subst $(BUILD_DIR),$(REPO_PATH),$(obj:.o=.c)))))

# Create a target for each .o files, depending on its corresponding .c file
define obj_rule
$(1): $(2) $(MAKEFILE_PATH)/makefile create_syscfg_regs
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
bullseye_inclusions:
	@echo -------------------------------------------------------------------------------
	@echo BULLSEYE INCLUSIONS
	covselect --deleteAll
	covselect --add $(REPO_PATH)/cs40l25/
	covselect --add !$(REPO_PATH)/cs40l25/bsp/
	covselect --add !$(REPO_PATH)/cs40l25/unit_test/
	covselect --add !$(REPO_PATH)/cs40l25/system_test/
	covselect --add $(REPO_PATH)/common/
	covselect --add !$(REPO_PATH)/common/system_test_hw_0/
	covselect --add !$(REPO_PATH)/build/
	covselect --add !$(REPO_PATH)/tools/
	covselect --add !$(REPO_PATH)/third_party/
	
create_syscfg_regs: build_path ../common/create_syscfg_regs.c device_syscfg_regs.c 
	@echo -------------------------------------------------------------------------------
	@echo COMPILING create_syscfg_regs
	gcc -Werror -Wall -w -I../common -c cs40l25_spec.c
	gcc -Werror -Wall -w -I../common -c device_syscfg_regs.c
	gcc -Werror -Wall -std=c99 -I../cs40l25 -c ../common/create_syscfg_regs.c -o ./create_syscfg_regs.o
	gcc -o create_syscfg_regs.exe create_syscfg_regs.o device_syscfg_regs.o cs40l25_spec.o
	@echo -------------------------------------------------------------------------------
	@echo EXECUTING create_syscfg_regs	
	./create_syscfg_regs.exe
	rm -f *.o *.exe	

# Create the driver static library and print the resource usage
driver_lib_cm4: build_path $(BUILD_PATHS) create_syscfg_regs $(DRIVER_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo ARCHIVING $(DRIVER_LIBRARY)
	$(AR) $(ARFLAGS) $(DRIVER_LIBRARY) $(DRIVER_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo SIZE of $(DRIVER_LIBRARY)
	$(SIZE) -t $(DRIVER_LIBRARY)

unit_test: build_path $(BUILD_PATHS) $(BULLSEYE_INCLUSIONS) driver_lib_cm4 $(OBJS)
	@echo -------------------------------------------------------------------------------
	@echo LINKING $@
	$(BULLSEYE_ON_CMD)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $(BUILD_DIR)/unit_test.exe
	$(BULLSEYE_OFF_CMD)
	@echo -------------------------------------------------------------------------------
	@echo RUNNING $@
	@echo -------------------------------------------------------------------------------
	$(BUILD_DIR)/unit_test.exe

system_test: build_path $(BUILD_PATHS) $(BULLSEYE_INCLUSIONS) driver_lib_cm4 $(OBJS) $(ASM_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo LINKING $@
	$(CC) $(LDFLAGS) $(OBJS) $(ASM_OBJS) $(LIBS) -o $(BUILD_DIR)/system_test.elf
	@echo -------------------------------------------------------------------------------
	@echo SIZE of system_test.elf
	$(SIZE) -t $(BUILD_DIR)/system_test.elf

baremetal: build_path $(BUILD_PATHS) driver_lib_cm4 $(OBJS) $(ASM_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo LINKING $@
	$(CC) $(LDFLAGS) $(OBJS) $(ASM_OBJS) $(LIBS) -o $(BUILD_DIR)/baremetal.elf
	@echo -------------------------------------------------------------------------------
	@echo SIZE of baremetal.elf
	$(SIZE) -t $(BUILD_DIR)/baremetal.elf

freertos: build_path $(BUILD_PATHS) driver_lib_cm4 $(OBJS) $(ASM_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo LINKING $@
	$(CC) $(LDFLAGS) $(OBJS) $(ASM_OBJS) $(LIBS) -o $(BUILD_DIR)/freertos.elf
	@echo -------------------------------------------------------------------------------
	@echo SIZE of freertos.elf
	$(SIZE) -t $(BUILD_DIR)/freertos.elf

print_targets:
	@echo ERROR:  No valid target specified.
	@echo
	@echo Valid targets:
	@echo       unit_test
	@echo       driver_lib_cm4
	@echo       system_test
	@echo       baremetal
	@echo       freertos
	@echo
	@echo Valid command line variables:
	@echo		BULLSEYE=0	\(will disable Bullseye coverage\)
	@echo		BULLSEYE=1	\(will enable Bullseye coverage\)

clean:
	$(RM) $(BUILD_DIR)
