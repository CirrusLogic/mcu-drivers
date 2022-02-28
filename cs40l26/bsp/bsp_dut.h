/**
 * @file bsp_dut.h
 *
 * @brief Functions and prototypes exported by the BSP module for the cs40l26 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2022 All Rights Reserved, http://www.cirrus.com/
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
#include "cs40l26_ext.h"
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DUT_I2C_ADDRESS_8BIT                            (0x80)

#define BSP_DUT_TRIGGER_HAPTIC_POWER_ON                     (0xFF)

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
uint32_t bsp_dut_boot();
uint32_t bsp_dut_calibrate(void);
uint32_t bsp_dut_power_up(void);
uint32_t bsp_dut_power_down(void);
uint32_t bsp_dut_hibernate(void);
uint32_t bsp_dut_wake(void);
uint32_t bsp_dut_trigger_haptic(uint8_t waveform, bool is_rom);
uint32_t bsp_dut_trigger_rth_pwle(bool is_simple, rth_pwle_section_t **pwle_data, uint8_t num_sections, uint8_t repeat);
uint32_t bsp_dut_trigger_rth_pcm(uint8_t *pcm_data, uint32_t num_sections, uint16_t buffer, uint16_t f0, uint16_t redc);
uint32_t bsp_dut_has_processed(bool *has_processed);
uint32_t bsp_dut_enable_haptic_processing(bool enable);
uint32_t bsp_dut_dynamic_calibrate(uint8_t index);
uint32_t bsp_dut_process(void);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // BSP_DUT_H
