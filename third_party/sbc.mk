##############################################################################
#
# Makefile for Bluetooth SBC Library
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
ifeq ($(origin SBC_PATH), undefined)
$(eval SBC_PATH = $(REPO_PATH)/third_party/sbc/sbc)
endif
SBC_BUILD_PATH = $(BUILD_DIR)/third_party/sbc/sbc

# Assign flags
CFLAGS += -DUSE_SBC

# Assign sources
SBC_C_SRCS =
SBC_C_SRCS += $(SBC_PATH)/sbc.c
SBC_C_SRCS += $(SBC_PATH)/sbc_primitives.c
SBC_C_SRCS += $(SBC_PATH)/sbc_primitives_armv6.c
SBC_C_SRCS += $(SBC_PATH)/sbc_primitives_iwmmxt.c
SBC_C_SRCS += $(SBC_PATH)/sbc_primitives_mmx.c
SBC_C_SRCS += $(SBC_PATH)/sbc_primitives_neon.c
SBC_C_SRCS += $(SBC_PATH)/sbc_primitives_sse.c

# Assign includes
INCLUDES += -I$(SBC_PATH)