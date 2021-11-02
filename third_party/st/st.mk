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

##############################################################################
# Variable Assignments
##############################################################################

# Includes

# Assign paths
ifeq ($(origin STM32CUBEF4_PATH), undefined)
$(eval STM32CUBEF4_PATH = $(REPO_PATH)/third_party/st/STM32CubeF4)
endif
STM32CUBEF4_BUILD_PATH = $(BUILD_DIR)/third_party/st/STM32CubeF4
PLATFORM_MCU_SRC_PATH = $(STM32CUBEF4_PATH)
PLATFORM_MCU_BUILD_PATH = $(STM32CUBEF4_BUILD_PATH)

# Assign flags
CFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs
CFLAGS += -DUSE_HAL_DRIVER -DSTM32F401xE
ifeq ($(USE_FREERTOS), 1)
    CFLAGS += -DUSE_CMSIS_OS
else
    CFLAGS += -DNO_OS
endif

ASM_FLAGS += -mcpu=cortex-m4 -c -x assembler-with-cpp --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb

LDFLAGS += -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nosys.specs --specs=nano.specs

# Assign sources
PLATFORM_MCU_C_SRCS =
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c_ex.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2c.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s_ex.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_i2s.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
ifeq ($(USE_FREERTOS), 1)
    PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c
    PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Middlewares/Third_Party/FreeRTOS/Source/tasks.c
    PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Middlewares/Third_Party/FreeRTOS/Source/queue.c
    PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
    PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Middlewares/Third_Party/FreeRTOS/Source/list.c
    PLATFORM_MCU_C_SRCS += $(STM32CUBEF4_PATH)/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
endif

PLATFORM_MCU_ASM_SRCS =
PLATFORM_MCU_ASM_SRCS += $(STM32CUBEF4_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f401xe.s

# Assign includes
INCLUDES += -I$(STM32CUBEF4_PATH)/Drivers/CMSIS/Core/Include
INCLUDES += -I$(STM32CUBEF4_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
INCLUDES += -I$(STM32CUBEF4_PATH)/Drivers/STM32F4xx_HAL_Driver/Inc
INCLUDES += -I$(STM32CUBEF4_PATH)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
INCLUDES += -I$(STM32CUBEF4_PATH)/Middlewares/Third_Party/FreeRTOS/Source/include
INCLUDES += -I$(STM32CUBEF4_PATH)/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F