/**
 * @file cs35l41_fs_switch_syscfg.h
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

#ifndef CS35L41_FS_SWITCH_SYSCFG_H
#define CS35L41_FS_SWITCH_SYSCFG_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "stdint.h"
#include "cs35l41_syscfg_regs.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define CS35L41_FS_48KHZ_SYSCFG_REGS_TOTAL      (6)
#define CS35L41_FS_44P1KHZ_SYSCFG_REGS_TOTAL    (6)

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern const uint32_t cs35l41_fs_48kHz_syscfg[];
extern const uint32_t cs35l41_fs_44p1kHz_syscfg[];

#ifdef __cplusplus
}
#endif

#endif // CS35L41_FS_SWITCH_SYSCFG_H

