/**
 * @file cs35l41_fs_switch_syscfg.c
 *
 * @brief Register values to be applied after CS35L41 Driver boot().
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020-2021 All Rights Reserved, http://www.cirrus.com/
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
#include "cs35l41_fs_switch_syscfg.h"

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
const uint32_t cs35l41_fs_48kHz_syscfg[] =
{
    0x00002C04, 0x00000430,
    0x00002C0C, 0x00000003,
    0x00004804, 0x00000021,
};

const uint32_t cs35l41_fs_44p1kHz_syscfg[] =
{
    0x00002C04, 0x000003F0,
    0x00002C0C, 0x0000000B,
    0x00004804, 0x0000001F,
};
