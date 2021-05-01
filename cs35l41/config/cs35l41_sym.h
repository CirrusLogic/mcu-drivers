
/**
 * @file cs35l41_sym.h
 *
 * @brief Master table of known firmware symbols for the CS35L41 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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
 * Command:  ..\..\tools\firmware_converter\firmware_converter.py --command fw_img_v1 --part cs35l41 --sym-input ..\cs35l41_sym.h --sym-output ..\cs35l41_sym.h --wmfw halo_cspl_RAM_revB2_29.33.0.wmfw
 *
 *
 */

#ifndef CS35L41_SYM_H
#define CS35L41_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS35L41_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS35L41_ALGORITHM_FIRMWARE_HALO_CSPL
#define CS35L41_ALGORITHM_CSPL
/** @} */

/**
 * @defgroup CS35L41_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE_HALO_CSPL
#define CS35L41_SYM_FIRMWARE_HALO_CSPL_HALO_STATE       (0x1)
#define CS35L41_SYM_FIRMWARE_HALO_CSPL_HALO_HEARTBEAT   (0x2)
// CSPL
#define CS35L41_SYM_CSPL_CSPL_STATE                     (0x3)
#define CS35L41_SYM_CSPL_CSPL_TEMPERATURE               (0x4)
#define CS35L41_SYM_CSPL_CAL_R                          (0x5)
#define CS35L41_SYM_CSPL_CAL_AMBIENT                    (0x6)
#define CS35L41_SYM_CSPL_CAL_STATUS                     (0x7)
#define CS35L41_SYM_CSPL_CAL_CHECKSUM                   (0x8)
#define CS35L41_SYM_CSPL_CAL_R_SELECTED                 (0x9)
#define CS35L41_SYM_CSPL_CAL_SET_STATUS                 (0xa)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L41_SYM_H

