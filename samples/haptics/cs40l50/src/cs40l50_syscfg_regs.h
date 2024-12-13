/**
 * @file cs40l50_syscfg_regs.h
 *
 * @brief Register values to be applied after CS40L50 Driver boot().
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2024 All Rights Reserved, http://www.cirrus.com/
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
 * wisce_to_syscfg_reg_converter.py SDK version: 4.21.0 - internal
 * Command:  ../../tools/wisce_script_converter/wisce_script_converter.py -c c_array -p cs40l50 -i /Users/drhodes/Documents/code/cross-compile/nfs-share/altos/driver/cs40l50/config/wisce_init.txt -o .
 *
 */

#ifndef CS40L50_SYSCFG_REGS_H
#define CS40L50_SYSCFG_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "stdint.h"
//#include "regmap.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define CS40L50_SYSCFG_REGS_TOTAL (44)

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern uint32_t cs40l50_syscfg_regs[];

#ifdef __cplusplus
}
#endif

#endif // CS40L50_SYSCFG_REGS_H

