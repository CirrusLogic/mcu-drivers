##############################################################################
#
# Makefile for Platform BSP Module
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

# Assign variables
ifeq ($(PLATFORM), eestm32int)
else ifeq ($(PLATFORM), live_oak)
else ifeq ($(PLATFORM), holdout)
else ifeq ($(PLATFORM), blackstar)
else ifeq ($(PLATFORM), cl_mcu)
else
    $(error Invalid PLATFORM configuration given!)
endif

# Includes
include $(REPO_PATH)/third_party/st/st.mk

# Assign paths
PLATFORM_PATH = $(REPO_PATH)/common/$(PLATFORM)

# Assign flags
ifeq ($(PLATFORM), eestm32int)
    LDFLAGS += -T"$(REPO_PATH)/common/platform_bsp/eestm32int/STM32F401RETX_FLASH.ld"
else ifeq ($(PLATFORM), live_oak)
    LDFLAGS += -T"$(REPO_PATH)/common/platform_bsp/live_oak/STM32F401CDYX_FLASH.ld"
else ifeq ($(PLATFORM), holdout)
    LDFLAGS += -T"$(REPO_PATH)/common/platform_bsp/holdout/STM32F401CDYX_FLASH.ld"
else ifeq ($(PLATFORM), blackstar)
    LDFLAGS += -T"$(REPO_PATH)/common/platform_bsp/blackstar/STM32F401CDYX_FLASH.ld"
else ifeq ($(PLATFORM), cl_mcu)
    LDFLAGS += -T"$(REPO_PATH)/common/platform_bsp/cl_mcu/STM32H563IIKX_FLASH.ld"
endif

# Assign sources
C_SRCS += $(REPO_PATH)/common/platform_bsp/syscalls.c
C_SRCS += $(REPO_PATH)/common/platform_bsp/sysmem.c
ifeq ($(PLATFORM), cl_mcu)
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/i2c.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/icache.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/spi.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/stm32h5xx_hal_timebase_tim.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/cl_gpio.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/mspp_hooks_gpio.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/mspp_hooks_i2ccon.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/mspp_hooks_i3cctrl.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/mspp_hooks_logger.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/mspp_hooks_mutex.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/mspp_hooks_spictrl.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/mspp_hooks_tick.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/gpio.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/i3c.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/memorymap.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/stm32h5xx_hal_msp.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/cl_mcu/Core/Src/usart.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/$(PLATFORM)/stm32h5xx_it.c
else 
    C_SRCS += $(REPO_PATH)/common/platform_bsp/$(PLATFORM)/platform_bsp.c
    C_SRCS += $(REPO_PATH)/common/platform_bsp/$(PLATFORM)/stm32f4xx_it.c
endif
ifeq ($(PLATFORM), eestm32int)
	C_SRCS += $(REPO_PATH)/common/platform_bsp/test_tone_tables.c
endif

# Assign includes
INCLUDES += -I$(REPO_PATH)/common/platform_bsp
INCLUDES += -I$(REPO_PATH)/common/platform_bsp/$(PLATFORM)

ifeq ($(PLATFORM), cl_mcu)
    INCLUDES += -I$(REPO_PATH)/common/platform_bsp/$(PLATFORM)/Core/Inc
endif
