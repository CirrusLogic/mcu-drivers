
/**
 * @file cs40l30_cal_sym.h
 *
 * @brief Master table of known firmware symbols for the CS40L30_CAL Driver module
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
 * firmware_converter.py version: 3.0.0
 * Command:  ..\..\tools\firmware_converter\firmware_converter.py --command fw_img_v1 --part cs40l30 --suffix cal --sym-input ..\cs40l30_cal_sym.h --sym-output ..\cs40l30_cal_sym.h --wmfw Stumpy_CAL_Rev1.1.3.wmfw
 *
 *
 */

#ifndef CS40L30_CAL_SYM_H
#define CS40L30_CAL_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L30_CAL_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS40L30_CAL_ALGORITHM_FIRMWARE_STUMPY_CALIB
/** @} */

/**
 * @defgroup CS40L30_CAL_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE_STUMPY_CALIB
#define CS40L30_CAL_SYM_FIRMWARE_STUMPY_CALIB_HALO_STATE        (0x1)
#define CS40L30_CAL_SYM_FIRMWARE_STUMPY_CALIB_HALO_HEARTBEAT    (0x2)
#define CS40L30_CAL_SYM_FIRMWARE_STUMPY_CALIB_TUNING_FLAGS      (0x3)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L30_CAL_SYM_H

