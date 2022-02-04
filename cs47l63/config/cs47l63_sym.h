
/**
 * @file cs47l63_sym.h
 *
 * @brief Master table of known firmware symbols for the CS47L63 Driver module
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
 * firmware_converter.py version: 3.1.0
 * Command:  ../../tools/firmware_converter/firmware_converter.py --sym-output ../cs47l63_sym.h fw_img_v2 --generic-sym cs47l63 shelleypassthru_HALO.wmfw
 *
 *
 */

#ifndef CS47L63_SYM_H
#define CS47L63_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS47L63_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS47L63_ALGORITHM_FIRMWARE
#define CS47L63_ALGORITHM_SIMPLEGAIN
/** @} */

/**
 * @defgroup CS47L63_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE
#define CS47L63_SYM_FIRMWARE_SYSTEM_CONFIG_XM_STRUCT_T  (0x1)
#define CS47L63_SYM_FIRMWARE_SYS_ENABLE                 (0x2)
#define CS47L63_SYM_FIRMWARE_HALO_STATE                 (0x3)
#define CS47L63_SYM_FIRMWARE_HALO_HEARTBEAT             (0x4)
#define CS47L63_SYM_FIRMWARE_AUDIO_BLK_SIZE             (0x5)
#define CS47L63_SYM_FIRMWARE_MP_ALL_CYCLE_CNT           (0x6)
#define CS47L63_SYM_FIRMWARE_MP_N1_CNT_VALS             (0x7)
#define CS47L63_SYM_FIRMWARE_SYSTEM_CONFIG_YM_STRUCT_T  (0x8)
#define CS47L63_SYM_FIRMWARE_DUMMY                      (0x9)
// SIMPLEGAIN
#define CS47L63_SYM_SIMPLEGAIN_SIMPLEGAIN_XM_STRUCT_T   (0xa)
#define CS47L63_SYM_SIMPLEGAIN_ALGORITHM_ENABLE         (0xb)
#define CS47L63_SYM_SIMPLEGAIN_SIMPLEGAIN_YM_STRUCT_T   (0xc)
#define CS47L63_SYM_SIMPLEGAIN_DUMMY                    (0xd)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS47L63_SYM_H

