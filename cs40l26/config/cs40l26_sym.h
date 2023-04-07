
/**
 * @file cs40l26_sym.h
 *
 * @brief Master table of known firmware symbols for the CS40L26 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2023 All Rights Reserved, http://www.cirrus.com/
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
 * firmware_converter.py SDK version: 4.8.0 - internal
 * Command:  ../../tools/firmware_converter/firmware_converter.py fw_img_v2 cs40l26 CS40L26_Rev7.2.15.wmfw --generic-sym --wmdr FW_ROM_wavetable.bin
 *
 *
 */

#ifndef CS40L26_SYM_H
#define CS40L26_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L26_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS40L26_ALGORITHM_FIRMWARE_CS40L26
#define CS40L26_ALGORITHM_A2H
#define CS40L26_ALGORITHM_DYNAMIC_F0
#define CS40L26_ALGORITHM_BUZZGEN
#define CS40L26_ALGORITHM_EVENT_HANDLER
#define CS40L26_ALGORITHM_VIBEGEN
#define CS40L26_ALGORITHM_SVC
#define CS40L26_ALGORITHM_GPIO
#define CS40L26_ALGORITHM_MDSYNC
#define CS40L26_ALGORITHM_PM
#define CS40L26_ALGORITHM_MAILBOX
#define CS40L26_ALGORITHM_FW_RAM_EXT
#define CS40L26_ALGORITHM_HAPTICS_LOGGER
#define CS40L26_ALGORITHM_EVENT_LOGGER
/** @} */

/**
 * @defgroup CS40L26_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE_CS40L26
#define CS40L26_SYM_FIRMWARE_HALO_STATE                             (0x2)
#define CS40L26_SYM_FIRMWARE_HALO_HEARTBEAT                         (0x3)
// BUZZGEN
#define CS40L26_SYM_BUZZGEN_BUZZ_EFFECTS1_BUZZ_FREQ                 (0x1d)
// VIBEGEN
#define CS40L26_SYM_VIBEGEN_NUM_OF_WAVES                            (0x3e)
#define CS40L26_SYM_VIBEGEN_COMPENSATION_ENABLE                     (0x44)
#define CS40L26_SYM_VIBEGEN_WAVE_XM_TABLE                           (0x45)
#define CS40L26_SYM_VIBEGEN_OWT_SIZE_XM                             (0x47)
#define CS40L26_SYM_VIBEGEN_OWT_NEXT_XM                             (0x48)
// DYNAMIC_F0
#define CS40L26_SYM_DYNAMIC_F0_DYNAMIC_F0_ENABLED                   (0x17)
#define CS40L26_SYM_DYNAMIC_F0_IMONRINGPPTHRESHOLD                  (0x18)
#define CS40L26_SYM_DYNAMIC_F0_FRME_SKIP                            (0x19)
#define CS40L26_SYM_DYNAMIC_F0_NUM_PEAKS_TOFIND                     (0x1a)
#define CS40L26_SYM_DYNAMIC_F0_DYN_F0_TABLE                         (0x1b)
// GPIO
#define CS40L26_SYM_GPIO_GPI_ENABLE_BITMASK                         (0x161)
#define CS40L26_SYM_GPIO_GPO_ENABLE_BITMASK                         (0x162)
// PM
#define CS40L26_SYM_PM_PM_TIMER_TIMEOUT_TICKS                       (0x16f)
#define CS40L26_SYM_PM_PM_CUR_STATE                                 (0x170)
#define CS40L26_SYM_PM_POWER_ON_SEQUENCE                            (0x173)
// MAILBOX
#define CS40L26_SYM_MAILBOX_QUEUE_WT                                (0x179)
#define CS40L26_SYM_MAILBOX_QUEUE_RD                                (0x17a)
// FW_RAM_EXT
#define CS40L26_SYM_FW_RAM_EXT_GPI_PMIC_MUTE_ENABLE                 (0x18b)
// A2H
#define CS40L26_SYM_A2H_A2HEN                                       (0x1bf)
/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L26_SYM_H

