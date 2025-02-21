/**
 * @file cs40l50_syscfg_regs.c
 *
 * @brief Register values to be applied after CS40L50 Driver boot().
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2024-2025 All Rights Reserved, http://www.cirrus.com/
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "cs40l50_syscfg_regs.h"
#include "cs40l50_spec.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/**
 * Application-specific config
 * Converted from cs40l50/config/wisce_init.txt
 * Please consult Cirrus for config appropriate to your application
 */
uint32_t cs40l50_syscfg_regs[] =
{
    0x0040, 0x0055,
    0x0040, 0x00aa,
    0x3808, 0x40000001,
    0x38ec, 0x0032,
    0x0040, 0x0000,
    0x201c, 0x0010,
    0x3800, 0x026e,
    0x2034, 0x2000000,
    0x280279c, 0x0006,
    0x280285c, 0x0000,
    0x280404c, 0x50020,
    0x2804050, 0x340200,
    0x2804054, 0x40020,
    0x2804058, 0x183201,
    0x280405c, 0x50044,
    0x2804060, 0x40100,
    0x2804064, 0xffffff
};

