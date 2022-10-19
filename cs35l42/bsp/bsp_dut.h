/**
 * @file bsp_dut.h
 *
 * @brief Functions and prototypes exported by the BSP module for the cs35l42 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022 All Rights Reserved, http://www.cirrus.com/
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
#include "platform_bsp.h"
#include "cs35l42.h"
#include "cs35l42_fw_img.h"
#include "cs35l42_cal_fw_img.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DUT_I2C_ADDRESS_8BIT                            (0x80)

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
uint32_t bsp_dut_boot(cs35l42_t *driver,
                      fw_img_boot_state_t *boot_state,
                      const uint8_t *fw_img,
                      bool is_wmdr_only);
uint32_t bsp_dut_power_up(void);
uint32_t bsp_dut_power_down(void);
uint32_t bsp_dut_hibernate(void);
uint32_t bsp_dut_wake(void);
uint32_t bsp_dut_set_dig_gain(float gain_db);
uint32_t bsp_dut_mute(bool is_mute);
uint32_t bsp_dut_calibrate(void);
uint32_t bsp_dut_process(void);
uint32_t bsp_dut_ping(void);
uint32_t bsp_dut_get_driver_handle(void **driver);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // BSP_DUT_H
