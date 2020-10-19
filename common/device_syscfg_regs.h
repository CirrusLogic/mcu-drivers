/**
 * @file device_syscfg_regs.h
 *
 * @brief Driver Syscfg Regs module typedefs and function prototypes
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
 */

#ifndef DEVICE_SYSCFG_REGS_H
#define DEVICE_SYSCFG_REGS_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdio.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
typedef struct
{
    uint32_t address;
    uint32_t mask;
    uint32_t value;
    char *name;
} syscfg_reg_list_entry_t;

typedef struct
{
    const char *chip_name_uc;
    const char *chip_name_lc;
    const char *header_filename;
    const char *header_filename_uc;
    const char *source_filename;

    uint32_t *cleared_regs;
    uint32_t *set_regs;
    syscfg_reg_list_entry_t *reg_list;
    uint32_t reg_list_total;
} syscfg_reg_descriptor_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
void configure_syscfg_reg_descriptor(syscfg_reg_descriptor_t *d);
void set_device_syscfg(void);
uint32_t apply_device_syscfg(uint32_t *reg_vals);
void add_device_header_defines(FILE *fp, syscfg_reg_descriptor_t *d);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // DEVICE_SYSCFG_REGS_H
