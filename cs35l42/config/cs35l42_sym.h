
/**
 * @file cs35l42_sym.h
 *
 * @brief Master table of known firmware symbols for the CS35L42 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022, 2024 All Rights Reserved, http://www.cirrus.com/
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
 * firmware_converter.py SDK version: 4.12.0 - internal
 * Command:  ../../tools/firmware_converter/firmware_converter.py fw_img_v2 cs35l42 CS35L42_L43_Rev7.13.1.wmfw
 *
 *
 */

#ifndef CS35L42_SYM_H
#define CS35L42_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS35L42_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS35L42_ALGORITHM_FIRMWARE_PLAYBACK
#define CS35L42_ALGORITHM_PM
#define CS35L42_ALGORITHM_PROTECT_LITE
/** @} */

/**
 * @defgroup CS35L42_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE_PLAYBACK
#define CS35L42_SYM_FIRMWARE_PLAYBACK_HALO_HEARTBEAT                                                 (0x01)
// PM
#define CS35L42_SYM_PM_PM_TIMER_TIMEOUT_TICKS                                                        (0x02)
#define CS35L42_SYM_PM_PM_CUR_STATE                                                                  (0x03)
#define CS35L42_SYM_PM_POWER_ON_SEQUENCE                                                             (0x04)
// PROTECT_LITE
#define CS35L42_SYM_PROTECT_LITE_PROTECT_LITE_CTRL_PROTECT_LITE_ENABLE                               (0x05)
#define CS35L42_SYM_PROTECT_LITE_PROTECT_LITE_CTRL_CALIBRATION_ENABLE                                (0x06)
#define CS35L42_SYM_PROTECT_LITE_CALIB_DIAG_VAR_ARRAY_CAL_AMBIENT_TEMPERATURE                        (0x07)
#define CS35L42_SYM_PROTECT_LITE_PROTECT_PILOT_TONE_PEART_CMPST_0_SINEGENERATORSENSE_0_THRESHOLD     (0x08)
#define CS35L42_SYM_PROTECT_LITE_R_CALIB_0_FIRST_RUN                                                 (0x09)
#define CS35L42_SYM_PROTECT_LITE_RE_CALIB_SELECTOR_CMPST_0_RECALIBSELECTOR_0_SEL_RE_CAL              (0x0a)
#define CS35L42_SYM_PROTECT_LITE_VAR_ARRAY_IMPEDANCE_MEASURE_STATUS                                  (0x0b)
#define CS35L42_SYM_PROTECT_LITE_VAR_ARRAY_MEASURED_IMPEDANCE_CALIBRATION                            (0x0c)
#define CS35L42_SYM_PROTECT_LITE_VAR_ARRAY_INITIAL_CALI_IMPEDANCE                                    (0x0d)
#define CS35L42_SYM_PROTECT_LITE_VAR_ARRAY_CHECK_SUM_CALIBRATION                                     (0x0e)
/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L42_SYM_H

