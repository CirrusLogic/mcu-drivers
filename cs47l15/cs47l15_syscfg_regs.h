/**
 * @file cs47l15_syscfg_regs.h
 *
 * @brief Register values to be applied after CS47L15 Driver boot().
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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
 * wisce_to_syscfg_reg_converter.py version: 1.0.0
 * Command:  ../tools/wisce_script_converter/wisce_script_converter.py -c c_array -p cs47l15 -i wisce_init.txt -o .
 *
 */

#ifndef CS47L15_SYSCFG_REGS_H
#define CS47L15_SYSCFG_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "stdint.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define CS47L15_SYSCFG_REGS_TOTAL    (14)

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
typedef struct
{
    uint32_t address;
    uint32_t mask;
    uint32_t value;
} syscfg_reg_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern const syscfg_reg_t cs47l15_syscfg_regs[];

#ifdef __cplusplus
}
#endif

#endif // CS47L15_SYSCFG_REGS_H

