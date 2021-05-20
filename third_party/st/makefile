##############################################################################
#
# Makefile for STM32F4 HAL SDK
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

ifeq ($(origin STM32CUBEF4_PATH), undefined)
STM32CUBEF4_PATH = $(REPO_PATH)/third_party/st/STM32CubeF4
endif
STM32CUBEF4_BUILD_PATH = $(BUILD_DIR)/third_party/st/STM32CubeF4

include $(REPO_PATH)/third_party/st/st.include

# Initialize toolchain arguments

# Process command line variables per make goal

# Assign toolchain, shell executable names required per target
CC          = arm-none-eabi-gcc
LD          = arm-none-eabi-ld
AR          = arm-none-eabi-ar
AS          = arm-none-eabi-gcc
OBJCOPY     = arm-none-eabi-objcopy
OBJDUMP     = arm-none-eabi-objdump
SIZE        = arm-none-eabi-size

ifeq ($(MAKECMDGOALS), st_lib)
    IS_VALID_BUILD = YES
endif

# Assign compiler, assembler, linker flags required per target
DEBUG_OPTIONS += -DDEBUG
DEBUG_OPTIONS += -g3
CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs
CFLAGS += -Werror -Wall --std=gnu11
CFLAGS += -fno-diagnostics-show-caret
CFLAGS += -fdata-sections -ffunction-sections -fstack-usage
CFLAGS += -frecord-gcc-switches
CFLAGS += $(ST_CFLAGS)
CFLAGS += $(DEBUG_OPTIONS)
OUTPUT_NAME =
OUTPUT_EXT  =
ARFLAGS     = rcs
ASM_FLAGS = -mcpu=cortex-m4 -c -x assembler-with-cpp --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
ASM_FLAGS += $(DEBUG_OPTIONS)

ifeq ($(USE_FREERTOS), 1)
    CFLAGS += -DUSE_CMSIS_OS
else
    CFLAGS += -DNO_OS
endif

# Assign sources, includes, paths for STM32F4 HAL library
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
C_SRCS += $(STM32CUBEF4_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c

ASM_SRCS += $(STM32CUBEF4_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f401xe.s

INCLUDES += $(ST_INCLUDES)
INCLUDES += -I$(STM32F4XX_HAL_CONF_H_PATH)

# Assign list of application source files required per target
ifeq ($(USE_FREERTOS), 1)
    C_SRCS += $(FREERTOS_PATH)/CMSIS_RTOS/cmsis_os.c
    C_SRCS += $(FREERTOS_PATH)/tasks.c
    C_SRCS += $(FREERTOS_PATH)/queue.c
    C_SRCS += $(FREERTOS_PATH)/portable/GCC/ARM_CM4F/port.c
    C_SRCS += $(FREERTOS_PATH)/list.c
    C_SRCS += $(FREERTOS_PATH)/portable/MemMang/heap_4.c

    INCLUDES += -I$(FREERTOSCONFIG_H_PATH)
endif

# Defines after all target-specific variables set
OBJS = $(subst $(STM32CUBEF4_PATH),$(STM32CUBEF4_BUILD_PATH),$(C_SRCS:.c=.o))
ASM_OBJS = $(subst $(STM32CUBEF4_PATH),$(STM32CUBEF4_BUILD_PATH),$(ASM_SRCS:.s=.o))
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
$(info ROOT_PATH:        $(ROOT_PATH))
$(info REPO_PATH:        $(REPO_PATH))
$(info BUILD_DIR:        $(BUILD_DIR))
$(info STM32CUBEF4_PATH: $(STM32CUBEF4_PATH))
$(info BUILD_PATHS:      $(addprefix $(newline),$(BUILD_PATHS)))
$(info -------------------------------------------------------------------------------)
$(info INCLUDES: $(addprefix $(newline),$(INCLUDES)))
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
$(info USE_FREERTOS: $(USE_FREERTOS))
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
	@echo COMPILING $(2)
	$(CC) $(CFLAGS) $(INCLUDES) -MD -MP -MT $(1) -MF $(subst .o,.d,$(1)) -c $(2) -o $(1)
endef
$(foreach obj,$(OBJS),$(eval $(call obj_rule,$(obj),$(subst $(STM32CUBEF4_BUILD_PATH),$(STM32CUBEF4_PATH),$(obj:.o=.c)))))

# Create a target for each .o file, depending on its corresponding .s file
define asm_obj_rule
$(1): $(2) $(MAKEFILE_PATH)/makefile
	@echo -------------------------------------------------------------------------------
	@echo ASSEMBLING $(2) #to $(1)
	$(AS) $(ASM_FLAGS) $(INCLUDES) -MD -MP -MT $(1) -MF $(subst .o,.d,$(1)) -c $(2) -o $(1)
endef
$(foreach obj,$(ASM_OBJS),$(eval $(call asm_obj_rule,$(obj),$(subst $(STM32CUBEF4_BUILD_PATH),$(STM32CUBEF4_PATH),$(obj:.o=.s)))))

# Create the STM32F4 static library and print the resource usage
st_lib: build_path $(BUILD_PATHS) $(OBJS) $(ASM_OBJS)
	@echo -------------------------------------------------------------------------------
	@echo ARCHIVING $(ST_LIBRARY)
	$(AR) $(ARFLAGS) $(ST_LIBRARY) $(OBJS) $(ASM_OBJS)

print_targets:
	@echo ERROR:  No valid target specified.
	@echo
	@echo Valid targets:
	@echo       st_lib
	@echo
	@echo Valid command line variables:
	@echo       STM32CUBEF4_PATH 			\(define where to find the STM32CubeF4 repo\)
	@echo       STM32F4XX_HAL_CONF_H_PATH	\(define where to find st32f4xx_hal_conf.h\)
	@echo
	@echo		USE_FREERTOS=0				\(will not include FreeRTOS\)
	@echo		USE_FREERTOS=1				\(will include FreeRTOS\)
	@echo       FREERTOSCONFIG_H_PATH		\(define where to find FreeRTOSConfig.h\)

clean:
	$(RM) $(BUILD_DIR)
