/**
 * @file bsp_dut.h
 *
 * @brief Functions and prototypes exported by the BSP module for the cs35l56 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022-2026 All Rights Reserved, http://www.cirrus.com/
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
#include "cs40l50.h"
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DUT_I2C_ADDRESS_8BIT                            (0x68)

#define BSP_DUT_ATTEN_DB_0                                  (0U)
#define BSP_DUT_ATTEN_DB_6                                  (6U)
#define BSP_DUT_ATTEN_DB_12                                 (12U)
#define BSP_DUT_ATTEN_DB_18                                 (18U)
#define BSP_DUT_ATTEN_DB_24                                 (24U)
#define BSP_DUT_ATTEN_DB_30                                 (30U)

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
uint32_t bsp_dut_set_broadcast_en(bool enable);
uint32_t bsp_dut_configure_gpio_input(unsigned int gpio);
uint32_t bsp_dut_disable_gpio_triggers(void);
uint32_t bsp_dut_configure_gpio_trigger(cs40l50_gpio_bank_t gpio, bool rth,
                                        uint8_t attenuation, bool ram, uint8_t plybck_index);
uint32_t bsp_dut_dynamic_f0_set_enable(bool enable);
uint32_t bsp_dut_trigger_haptic(uint8_t waveform, cs40l50_wavetable_bank_t bank);
uint32_t bsp_dut_trigger_rth_pwle(bool is_simple, const rth_pwle_section_t **pwle_data, uint8_t num_sections, uint8_t repeat);
uint32_t bsp_dut_trigger_rth_pcm(uint8_t *pcm_data, uint32_t num_sections, uint16_t buffer, uint16_t f0, uint16_t redc);
uint32_t bsp_dut_dynamic_calibrate(uint8_t index);
/**
 * @brief Sets the global attenuation of the haptic output
 *
 * This function takes in the attenuation of haptic triggers in a dB, converts it to a s21.2 fixed point value, then writes it to
 * HAPTICS_SYSTEM_SOURCE_ATTENUATION in the system firmware.
 *
 * @param [in] atten Attenuation value to set for haptic output in s22.2 format
 *
 * @return
 * - CS40L50_STATUS_OK if:
 *      - HAPTICS_SYSTEM_SOURCE_ATTENUATION successfully written with new value
 * - otherwise, returns CS40L50_STATUS_FAIL
 */
uint32_t bsp_dut_configure_attenuation(uint32_t atten_db);
/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // HW_0_BSP_DUT_H
