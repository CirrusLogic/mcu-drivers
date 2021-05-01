
/**
 * @file cs40l30_sym.h
 *
 * @brief Master table of known firmware symbols for the CS40L30 Driver module
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
 * Command:  ..\..\tools\firmware_converter\firmware_converter.py --command fw_img_v1 --part cs40l30 --sym-input ..\cs40l30_sym.h --sym-output ..\cs40l30_sym.h --wmfw Stumpy_RAM_Rev1.0.18.wmfw --wmdr StumpyWavetable.bin
 *
 *
 */

#ifndef CS40L30_SYM_H
#define CS40L30_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L30_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS40L30_ALGORITHM_FIRMWARE_HAPTICS
#define CS40L30_ALGORITHM_BUZZGEN
#define CS40L30_ALGORITHM_PM
/** @} */

/**
 * @defgroup CS40L30_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE_HAPTICS
#define CS40L30_SYM_FIRMWARE_HAPTICS_HALO_STATE         (0x1)
#define CS40L30_SYM_FIRMWARE_HAPTICS_HALO_HEARTBEAT     (0x2)
#define CS40L30_SYM_FIRMWARE_HAPTICS_TUNING_FLAGS       (0x3)
// BUZZGEN
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS1_BUZZ_FREQ     (0x4)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS1_BUZZ_LEVEL    (0x5)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS1_BUZZ_DURATION (0x6)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS2_BUZZ_FREQ     (0x7)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS2_BUZZ_LEVEL    (0x8)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS2_BUZZ_DURATION (0x9)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS3_BUZZ_FREQ     (0xa)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS3_BUZZ_LEVEL    (0xb)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS3_BUZZ_DURATION (0xc)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS4_BUZZ_FREQ     (0xd)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS4_BUZZ_LEVEL    (0xe)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS4_BUZZ_DURATION (0xf)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS5_BUZZ_FREQ     (0x10)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS5_BUZZ_LEVEL    (0x11)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS5_BUZZ_DURATION (0x12)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS6_BUZZ_FREQ     (0x13)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS6_BUZZ_LEVEL    (0x14)
#define CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS6_BUZZ_DURATION (0x15)
// PM
#define CS40L30_SYM_PM_POWER_ON_SEQUENCE                (0x16)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L30_SYM_H

