/**
 * @file bsp_dut.h
 *
 * @brief Functions and prototypes exported by the BSP module for the cs35l41 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021 All Rights Reserved, http://www.cirrus.com/
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

#ifndef BSP_DUT_H
#define BSP_DUT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "bsp_driver_if.h"
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DUT_I2C_ADDRESS_8BIT                            (0x80)
#define BSP_DUT_ID_LEFT                                     (0x0)
#define BSP_DUT_ID_RIGHT                                    (0x1)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_initialize(void);
uint32_t bsp_dut_reset(void);
uint32_t bsp_dut_boot(bool cal_boot);
uint32_t bsp_dut_calibrate(void);
uint32_t bsp_dut_power_up(void);
uint32_t bsp_dut_power_down(void);
uint32_t bsp_dut_mute(bool is_mute);
uint32_t bsp_dut_is_processing(bool *is_processing);
uint32_t bsp_dut_hibernate(void);
uint32_t bsp_dut_wake(void);
uint32_t bsp_dut_get_id(uint8_t *id);
uint32_t bsp_dut_set_dig_gain(float db);
uint32_t bsp_dut_change_fs(uint32_t fs_hz);
uint32_t bsp_dut_process(void);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // BSP_DUT_H
