
/**
 * @file cs47l63_sym.h
 *
 * @brief Master table of known firmware symbols for the CS47L63 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021, 2023 All Rights Reserved, http://www.cirrus.com/
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
 * firmware_converter.py version: 3.2.0
 * Command:  ../../tools/firmware_converter/firmware_converter.py --sym-output ../config/cs47l63_sym.h fw_img_v2 --generic-sym cs47l63 CS47L63_SCH_SCC_MSBC_S_000A00.wmfw
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
#define CS47L63_ALGORITHM_SENSORY_XM
#define CS47L63_ALGORITHM_SCC
/** @} */

/**
 * @defgroup CS47L63_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE
#define CS47L63_SYM_FIRMWARE_SYSTEM_CONFIG_XM_STRUCT_T          (0x1)
#define CS47L63_SYM_FIRMWARE_HALO_STATE                         (0x2)
#define CS47L63_SYM_FIRMWARE_HEARTBEAT                          (0x3)
#define CS47L63_SYM_FIRMWARE_EVENT_CTRL                         (0x4)
#define CS47L63_SYM_FIRMWARE_INIT_FEATURES                      (0x5)
#define CS47L63_SYM_FIRMWARE_AVAIL_FEATURES                     (0x6)
#define CS47L63_SYM_FIRMWARE_ENABLED_FEATURES                   (0x7)
#define CS47L63_SYM_FIRMWARE_COMMAND                            (0x8)
#define CS47L63_SYM_FIRMWARE_PARAM                              (0x9)
#define CS47L63_SYM_FIRMWARE_STATUS                             (0xa)
#define CS47L63_SYM_FIRMWARE_ERROR                              (0xb)
#define CS47L63_SYM_FIRMWARE_SYSTEM_CONFIG_YM_STRUCT_T          (0xc)
#define CS47L63_SYM_FIRMWARE_ENABLED_BUFFERS                    (0xd)
// SENSORY_XM
#define CS47L63_SYM_SENSORY_XM_SENSORY_ALGORITHM_XM_STRUCT_T    (0xe)
#define CS47L63_SYM_SENSORY_XM_LIBVERSION                       (0xf)
#define CS47L63_SYM_SENSORY_XM_RECOGNITIONCOUNT                 (0x10)
#define CS47L63_SYM_SENSORY_XM_WORDID                           (0x11)
#define CS47L63_SYM_SENSORY_XM_WORDLENGTH                       (0x12)
#define CS47L63_SYM_SENSORY_XM_WORDTAIL                         (0x13)
#define CS47L63_SYM_SENSORY_XM_WORDEND                          (0x14)
#define CS47L63_SYM_SENSORY_XM_LPSDPOWER                        (0x15)
#define CS47L63_SYM_SENSORY_XM_INITIALIZED                      (0x16)
#define CS47L63_SYM_SENSORY_XM_FINALSCORE                       (0x17)
#define CS47L63_SYM_SENSORY_XM_INTERNAL_ERROR                   (0x18)
#define CS47L63_SYM_SENSORY_XM_SENSORY_ALG_GAIN                 (0x19)
#define CS47L63_SYM_SENSORY_XM_SDETTYPE                         (0x1a)
#define CS47L63_SYM_SENSORY_XM_DCBLOCKENABLE                    (0x1b)
#define CS47L63_SYM_SENSORY_XM_SVTHRESHOLDOFFSET                (0x1c)
#define CS47L63_SYM_SENSORY_XM_PARAMAOFFSET                     (0x1d)
#define CS47L63_SYM_SENSORY_XM_EPQMINSNR                        (0x1e)
#define CS47L63_SYM_SENSORY_XM_INCLUDETRIGGERPHRASE             (0x1f)
#define CS47L63_SYM_SENSORY_XM_P_GRAMMAR_DATA                   (0x20)
#define CS47L63_SYM_SENSORY_XM_P_MODEL_DATA                     (0x21)
// SCC
#define CS47L63_SYM_SCC_SCC_CONFIG_XM_STRUCT_T                  (0x22)
#define CS47L63_SYM_SCC_SCC_STATE                               (0x23)
#define CS47L63_SYM_SCC_HOST_BUFFER_RAW                         (0x24)
#define CS47L63_SYM_SCC_INCLUDETRIGGERPHRASE                    (0x25)
#define CS47L63_SYM_SCC_TPFCOUNT                                (0x26)
#define CS47L63_SYM_SCC_HOSTREARM                               (0x27)
#define CS47L63_SYM_SCC_INITDISARMED                            (0x28)
#define CS47L63_SYM_SCC_VTE1_VTEID                              (0x29)
#define CS47L63_SYM_SCC_VTE1_COUNTER                            (0x2a)
#define CS47L63_SYM_SCC_VTE1_PHRASEID                           (0x2b)
#define CS47L63_SYM_SCC_VTE1_PHRASESCORE                        (0x2c)
#define CS47L63_SYM_SCC_VTE1_CAPDELAYMS                         (0x2d)
#define CS47L63_SYM_SCC_VTE1_TRIGGERPOINT                       (0x2e)
#define CS47L63_SYM_SCC_VTE1_CAPPREAMBLEMS                      (0x2f)
#define CS47L63_SYM_SCC_VTE1_BUFFERINST                         (0x30)
#define CS47L63_SYM_SCC_VTE1_VTEVERSION                         (0x31)
#define CS47L63_SYM_SCC_VTE2_VTEID                              (0x32)
#define CS47L63_SYM_SCC_VTE2_COUNTER                            (0x33)
#define CS47L63_SYM_SCC_VTE2_PHRASEID                           (0x34)
#define CS47L63_SYM_SCC_VTE2_PHRASESCORE                        (0x35)
#define CS47L63_SYM_SCC_VTE2_CAPDELAYMS                         (0x36)
#define CS47L63_SYM_SCC_VTE2_TRIGGERPOINT                       (0x37)
#define CS47L63_SYM_SCC_VTE2_CAPPREAMBLEMS                      (0x38)
#define CS47L63_SYM_SCC_VTE2_BUFFERINST                         (0x39)
#define CS47L63_SYM_SCC_VTE2_VTEVERSION                         (0x3a)
#define CS47L63_SYM_SCC_SCCMANAGEACKCTRL                        (0x3b)
#define CS47L63_SYM_SCC_SCC_PARAM_VTEID                         (0x3c)
#define CS47L63_SYM_SCC_SCC_PARAM_PHRASEID                      (0x3d)
#define CS47L63_SYM_SCC_SCC_STATUS                              (0x3e)
#define CS47L63_SYM_SCC_SCC_CONTROL                             (0x3f)
#define CS47L63_SYM_SCC_SCC_ERROR                               (0x40)
#define CS47L63_SYM_SCC_GAIN_VTE_DB                             (0x41)
#define CS47L63_SYM_SCC_GAIN_PACKER_DB                          (0x42)
#define CS47L63_SYM_SCC_EXT_ADD_PREAMBLE                        (0x43)
#define CS47L63_SYM_SCC_VTE1_ADD_PREAMBLE                       (0x44)
#define CS47L63_SYM_SCC_VTE2_ADD_PREAMBLE                       (0x45)
#define CS47L63_SYM_SCC_VTE_ERROR                               (0x46)
#define CS47L63_SYM_SCC_SCC_CONFIG_YM_STRUCT_T                  (0x47)
#define CS47L63_SYM_SCC_SCC_NUM_MICS                            (0x48)
#define CS47L63_SYM_SCC_TRIGGER_ONLY_MODE                       (0x49)
#define CS47L63_SYM_SCC_ENABLED_FEATURES                        (0x4a)
#define CS47L63_SYM_SCC_BUFFER_FORMAT                           (0x4b)
#define CS47L63_SYM_SCC_ENABLED_BUFFERS                         (0x4c)
#define CS47L63_SYM_SCC_RAW_BUFFER_SIZE_XM                      (0x4d)
#define CS47L63_SYM_SCC_RAW_BUFFER_SIZE_YM                      (0x4e)
#define CS47L63_SYM_SCC_SBC_BITPOOL                             (0x4f)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS47L63_SYM_H

