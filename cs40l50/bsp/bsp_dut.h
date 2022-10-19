/**
 * @file bsp_dut.h
 *
 * @brief Functions and prototypes exported by the BSP module for the cs35l56 platform.
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

#ifndef HW_0_BSP_DUT_H
#define HW_0_BSP_DUT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "bsp_driver_if.h"
#include "cs40l50_ext.h"
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DUT_I2C_ADDRESS_8BIT                            (0x60)

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
uint32_t bsp_dut_boot(void);
uint32_t bsp_dut_process(void);
uint32_t bsp_dut_calibrate(void);
uint32_t bsp_dut_timeout_ticks_set(uint32_t ms);
uint32_t bsp_dut_hibernate(void);
uint32_t bsp_dut_wake(void);
uint32_t bsp_dut_set_click_compensation(bool f0_enable, bool redc_enable);
uint32_t bsp_dut_trigger_haptic(uint8_t waveform, cs40l50_wavetable_bank_t bank);
uint32_t bsp_dut_trigger_rth_pwle(bool is_simple, rth_pwle_section_t **pwle_data, uint8_t num_sections, uint8_t repeat);
uint32_t bsp_dut_trigger_rth_pcm(uint8_t *pcm_data, uint32_t num_sections, uint16_t buffer, uint16_t f0, uint16_t redc);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // HW_0_BSP_DUT_H
