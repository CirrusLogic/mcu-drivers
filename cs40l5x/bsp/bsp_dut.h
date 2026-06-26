/**
 * @file bsp_dut.h
 *
 * @brief Functions and prototypes exported by the BSP module for the cs35l56 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2025-2026 All Rights Reserved, http://www.cirrus.com/
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
#include "cs40l5x.h"
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
/**
 * @brief Initialize driver data structures and IRQ GPIO and callback function
 *
 * - Sets all driver data structures to 0 in cs40l5x_initialize
 * - Applies optional one time config, maps IRQ GPIO and registers IRQ callback function to driver
 * in cs40l5x_configure
 *
 * @return
 * - BSP_STATUS_FAIL        If an invalid cs40l5x_t object is passed to cs40l5x_initialize
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_initialize(void);

/**
 * @brief Perform a hardware reset and wait for OTP boot
 *
 * - Toggles reset pin for required reset time, reads back DEV/REV ID and checks revision, then
 * polls for OTP boot before continuing
 *
 * @return
 * - BSP_STATUS_FAIL        If reset fails to see a proper boot, or sees an invalid hardware
 * revision
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_reset(void);

/**
 * @brief Shutdown DSP and perform firmware loading
 *
 * - Send shutdown mbox command, then wait for PM state to switch to shutdown
 * - Perform firmware and coefficient loading
 * - Reenable DSP clock and wait for OTP boot
 * - Enable ASP streaming by default
 *
 * @return
 * - BSP_STATUS_FAIL        On register read/write failure
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_boot(void);

/**
 * @brief Processes driver events
 *
 * - This is a wrapper function for the driver's cs40l5x_process, which performs event handling when
 * an IRQ is detected
 *
 * @return
 * - BSP_STATUS_FAIL        On cs40l5x_process failure
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_process(void);

/**
 * @brief Perform device calibration
 *
 * - This is a wrapper function for the driver's cs40l5x_calibrate
 *
 * @return
 * - BSP_STATUS_FAIL        On cs40l5x_calibrate failure
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_calibrate(void);

/**
 * @brief Set standby to hibernation timeout time
 *
 * @param [in] ms       timeout time in ms
 * @return
 * - BSP_STATUS_FAIL        On cs40l5x_timeout_ticks_set failure
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_timeout_ticks_set(uint32_t ms);

/**
 * @brief Configure driver to allow auto-hibernate
 *
 * - Update power state to hibernate, configure PM_STATE to allow hibernation
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to update cs40l5x power state
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_hibernate(void);

/**
 * @brief Configure driver to prevent auto-hibernate
 *
 * - Update power state to wake, configure PM_STATE to prevent hibernation
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to update cs40l5x power state
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_wake(void);

/**
 * @brief Enable/Disable click compensation for F0 and ReDC
 *
 * @param [in] f0_enable    True to enable F0 click compensation, False to disable
 * @param [in] redc_enable  True to enable ReDC click compensation, False to disable
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to write click compensation enable register
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_set_click_compensation(bool f0_enable, bool redc_enable);

/**
 * @brief Enable/Disable i2c broadcast
 *
 * @param [in] enable       True to enable i2c broadcast, False to disable
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to write I2C_BROADCAST register
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_set_broadcast_en(bool enable);

/**
 * @brief Configure one of GPIO1-13 as input
 *
 * @param gpio              GPIO number (1-13)
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to write corresponding GPIO ctrl register
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_configure_gpio_input(unsigned int gpio);

/**
 * @brief Disable GPIO edge haptic effect triggering for all GPIO event registers
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to write GPIO event registers
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_disable_gpio_triggers(void);

/**
 * @brief Configure haptic effect triggering on GPIO
 *
 * @param [in] gpio             GPIO Index on which to trigger effect
 * @param [in] rth              True if effect is a RTH waveform, else False
 * @param [in] attenuation      Attenuation in dB where 0 is 0 dB, 1 is -1 dB, 2 is -2 dB, etc. with limit of -7 dB
 * @param [in] ram              True if effect is a RAM waveform, else False
 * @param [in] plybck_index     Index of effect to be triggered (0x0-0x1 for RTH, 0x00-0x7F for Wavetable)
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to configure GPIO trigger
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_configure_gpio_trigger(cs40l5x_gpio_bank_t gpio, bool rth,
                                        uint8_t attenuation, bool ram, uint8_t plybck_index);

/**
 * @brief Enables/Disables dynamic F0 with threshold set in driver config
 *
 * @param [in] enable       True if enable dynamic F0, else False
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to configure dynamic F0
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_dynamic_f0_set_enable(bool enable);

/**
 * @brief Trigger waveform in ROM/RAM wavetable
 *
 * - This sends a waveform trigger function to the DSP mailbox based on the given bank and waveform index
 *
 * @param [in] waveform     Waveform index in wavetable
 * @param [in] bank         Wavetable location of waveform to be played (ROM_BANK or RAM_BANK)
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On unknown waveform bank or failure to write trigger command to mbox
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_trigger_haptic(uint8_t waveform, cs40l5x_wavetable_bank_t bank);

/**
 * @brief Store and trigger a PWLE waveform via the OWT
 *
 * @param [in] is_simple    True if PWLE contains a single section, else False
 * @param [in] pwle_data    PWLE metadata data structure from waveforms.c/h, generated from hwt_to_waveform_converter.py
 * @param [in] num_sections Number of PWLE sections in stored OWT PWLE waveform
 * @param [in] repeat       Number of repeats in stored OWT PWLE waveform
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to store or trigger the PWLE waveform in the OWT
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_trigger_rth_pwle(bool is_simple, const rth_pwle_section_t **pwle_data, uint8_t num_sections, uint8_t repeat);

/**
 * @brief Store and trigger a PCM waveform via the OWT
 *
 * @param [in] pcm_data     Array of PCM data to be packed and stored into OWT
 * @param [in] num_sections Number of PCM data sections in stored OWT PCM waveform
 * @param [in] buffer       Size of array of PCM data
 * @param [in] f0           F0 value to store in PCM header in OWT
 * @param [in] redc         ReDC value to store in PCM header in OWT
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to store or trigger the PCM waveform in the OWT
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_trigger_rth_pcm(uint8_t *pcm_data, uint32_t num_sections, uint16_t buffer, uint16_t f0, uint16_t redc);

/**
 * @brief Read back the dynamic F0 entry at a given index
 *
 * @param [in] index Effect index for which to read back the dynamic F0
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        On failure to read back dynamic F0 entry
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_dynamic_calibrate(uint8_t index);

/**
 * @brief Check for HALO_STATE Running and no global error flag
 *
 * @return uint32_t
 * - BSP_STATUS_FAIL        If HALO_STATE != Running or global error flag detected
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_check_error(void);

/**
 * @brief Sets the global attenuation of the haptic output
 *
 * This function takes in the attenuation of haptic triggers in a dB, converts it to a s21.2 fixed point value, then writes it to
 * HAPTICS_SYSTEM_SOURCE_ATTENUATION in the system firmware.
 *
 * @param [in] atten Attenuation value to set for haptic output in s22.2 format
 *
 * @return
 * - CS40L5X_STATUS_OK if:
 *      - HAPTICS_SYSTEM_SOURCE_ATTENUATION successfully written with new value
 * - otherwise, returns CS40L50_STATUS_FAIL
 */
uint32_t bsp_dut_configure_attenuation(uint32_t atten_db);
/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // HW_0_BSP_DUT_H
