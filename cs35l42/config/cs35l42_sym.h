
/**
 * @file cs35l42_sym.h
 *
 * @brief Master table of known firmware symbols for the CS35L42 Driver module
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
/** @} */

/**
 * @defgroup CS35L42_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE_PLAYBACK
#define CS35L42_SYM_FIRMWARE_PLAYBACK_HALO_HEARTBEAT                           (0x1)
// PM
#define CS35L42_SYM_PM_PM_TIMER_TIMEOUT_TICKS                                  (0x2)
#define CS35L42_SYM_PM_PM_CUR_STATE                                            (0x3)
#define CS35L42_SYM_PM_POWER_ON_SEQUENCE                                       (0x4)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L42_SYM_H

