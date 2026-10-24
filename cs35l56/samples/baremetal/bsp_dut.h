/**
 * @file bsp_dut.h
 *
 * @brief Functions and prototypes exported by the BSP module for the cs35l56 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2026 All Rights Reserved, http://www.cirrus.com/
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
#include "cs35l56.h"
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DUT_I2C_ADDRESS_8BIT (0x60)

#define AUDIO_PCM_RATE_48K      48000
#define AUDIO_PCM_WIDTH_32_BITS 32
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
 * - Sets all driver data structures to 0 in cs35l56_initialize
 * - Applies optional one time config, maps IRQ GPIO and registers IRQ callback function to driver
 * in cs35l56_configure
 *
 * @return
 * - BSP_STATUS_FAIL        If an invalid cs35l56_t object is passed to cs35l56_initialize
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
 * - This is a wrapper function for the driver's cs35l56_process, which performs event handling when
 * an IRQ is detected
 *
 * @return
 * - BSP_STATUS_FAIL        On cs35l56_process failure
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_process(void);

/**
 * @brief Perform device calibration
 *
 * - This is a wrapper function for the driver's cs35l56_calibrate
 *
 * @return
 * - BSP_STATUS_FAIL        On cs35l56_calibrate failure
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_calibrate(void);

/**
 * @brief Set standby to hibernation timeout time
 *
 * @param [in] ms       timeout time in ms
 * @return
 * - BSP_STATUS_FAIL        On cs35l56_timeout_ticks_set failure
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_dut_timeout_ticks_set(uint32_t ms);

/**
 * @brief Send a PLAY command to the DSP mailbox to begin streaming
 *
 * @return
 * - BSP_STATUS_FAIL        On regmap_write failure
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_start_streaming(void);

/**
 * @brief Send a PAUSE command to the DSP mailbox to stop streaming
 *
 * - BSP_STATUS_FAIL        On regmap_write failure
 * - BSP_STATUS_OK          Otherwise
 */
uint32_t bsp_pause_streaming(void);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // HW_0_BSP_DUT_H
