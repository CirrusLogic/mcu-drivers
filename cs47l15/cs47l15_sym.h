
/**
 * @file cs47l15_sym.h
 *
 * @brief Master table of known firmware symbols for the CS47L15 Driver module
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
 */

#ifndef CS47L15_SYM_H
#define CS47L15_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS47L15_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS47L15_ALGORITHM_FIRMWARE
#define CS47L15_ALGORITHM_EG_ALGORITHM

#define CS47L15_ALGORITHM_MP3_DEC
/** @} */

/**
 * @defgroup CS47L15_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE
#define CS47L15_SYM_FIRMWARE_SYSTEM_CONFIG_XM_STRUCT_T          (0x1)
#define CS47L15_SYM_FIRMWARE_SYS_ENABLE                         (0x2)
#define CS47L15_SYM_FIRMWARE_PLUGIN_VISIBLE_FIRMWARE_ID         (0x3)
#define CS47L15_SYM_FIRMWARE_PLUGIN_VISIBLE_FIRMWARE_VERSION    (0x4)
#define CS47L15_SYM_FIRMWARE_ADSP2_STATE                        (0x5)
#define CS47L15_SYM_FIRMWARE_ADSP2_WATCHDOG                     (0x6)
#define CS47L15_SYM_FIRMWARE_DMA_BUFFER_SIZE                    (0x7)
#define CS47L15_SYM_FIRMWARE_RDMA_CONTROL_TABLE                 (0x8)
#define CS47L15_SYM_FIRMWARE_WDMA_CONTROL_TABLE                 (0x9)
#define CS47L15_SYM_FIRMWARE_BUILD_JOB_NAME                     (0xa)
#define CS47L15_SYM_FIRMWARE_BUILD_JOB_NUMBER                   (0xb)
#define CS47L15_SYM_FIRMWARE_SYSTEM_CONFIG_ZM_STRUCT_T          (0xc)
#define CS47L15_SYM_FIRMWARE_MAX_SYSTEM_GAIN                    (0xd)
// EG_ALGORITHM
#define CS47L15_SYM_EG_ALGORITHM_EG_ALGORITHM_XM_STRUCT_T       (0x1001)
#define CS47L15_SYM_EG_ALGORITHM_EG_ALGORITHM_ENABLE            (0x1002)
#define CS47L15_SYM_EG_ALGORITHM_DUMMY_XM_CONFIG                (0x1003)
#define CS47L15_SYM_EG_ALGORITHM_EG_ALGORITHM_YM_STRUCT_T       (0x1004)
#define CS47L15_SYM_EG_ALGORITHM_DUMMY_YM_CONFIG                (0x1005)
#define CS47L15_SYM_EG_ALGORITHM_EG_ALGORITHM_ZM_STRUCT_T       (0x1006)
#define CS47L15_SYM_EG_ALGORITHM_EG_ALGORITHM_GAIN              (0x1007)
// MP3_DEC
#define CS47L15_SYM_MP3_DEC_MP3_DEC_XM_STRUCT_T                 (0x2001)
#define CS47L15_SYM_MP3_DEC_RING_BUFF_ADDRESS                   (0x2002)
#define CS47L15_SYM_MP3_DEC_NO_OF_CHANNELS                      (0x2003)
#define CS47L15_SYM_MP3_DEC_BITRATE                             (0x2004)
#define CS47L15_SYM_MP3_DEC_SAMPLE_RATE                         (0x2005)
#define CS47L15_SYM_MP3_DEC_HIGH_WATERMARK_LEVEL_PERCENTAGE     (0x2006)
#define CS47L15_SYM_MP3_DEC_PLAY_CONTROL                        (0x2007)
#define CS47L15_SYM_MP3_DEC_MP3_DEC_YM_STRUCT_T                 (0x2008)
#define CS47L15_SYM_MP3_DEC_MP3_DEC_ZM_STRUCT_T                 (0x2009)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS47L15_SYM_H

