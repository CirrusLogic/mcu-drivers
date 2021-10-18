
/**
 * @file cs47l35_dsp2_sym.h
 *
 * @brief Master table of known firmware symbols for the CS47L35_DSP2 Driver module
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
 * firmware_converter.py SDK version: 4.7.0 - internal
 * Command:  ../../tools/firmware_converter/firmware_converter.py fw_img_v2 cs47l35_dsp2 cs47l35_silkcoder_dsp2_010103.wmfw --sym-output ../config/cs47l35_sym.h --generic-sym
 *
 *
 */

#ifndef CS47L35_DSP2_SYM_H
#define CS47L35_DSP2_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS47L35_DSP2_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS47L35_DSP2_ALGORITHM_FIRMWARE
#define CS47L35_DSP2_ALGORITHM_SILK_ENCODER
#define CS47L35_DSP2_ALGORITHM_SILK_DECODER
/** @} */

/**
 * @defgroup CS47L35_DSP2_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
// FIRMWARE
#define CS47L35_DSP2_SYM_FIRMWARE_SYSTEM_CONFIG_XM_STRUCT_T         (0x1)
#define CS47L35_DSP2_SYM_FIRMWARE_SYS_ENABLE                        (0x2)
#define CS47L35_DSP2_SYM_FIRMWARE_PLUGIN_VISIBLE_FIRMWARE_ID        (0x3)
#define CS47L35_DSP2_SYM_FIRMWARE_PLUGIN_VISIBLE_FIRMWARE_VERSION   (0x4)
#define CS47L35_DSP2_SYM_FIRMWARE_ADSP2_STATE                       (0x5)
#define CS47L35_DSP2_SYM_FIRMWARE_ADSP2_WATCHDOG                    (0x6)
#define CS47L35_DSP2_SYM_FIRMWARE_DMA_BUFFER_SIZE                   (0x7)
#define CS47L35_DSP2_SYM_FIRMWARE_RDMA_CONTROL_TABLE                (0x8)
#define CS47L35_DSP2_SYM_FIRMWARE_WDMA_CONTROL_TABLE                (0x9)
#define CS47L35_DSP2_SYM_FIRMWARE_BUILD_JOB_NAME                    (0xa)
#define CS47L35_DSP2_SYM_FIRMWARE_BUILD_JOB_NUMBER                  (0xb)
#define CS47L35_DSP2_SYM_FIRMWARE_SYSTEM_CONFIG_ZM_STRUCT_T         (0xc)
#define CS47L35_DSP2_SYM_FIRMWARE_MAX_SYSTEM_GAIN                   (0xd)
// SILK_ENCODER
#define CS47L35_DSP2_SYM_SILK_ENCODER_SILK_ENCODER_XM_STRUCT_T      (0xe)
#define CS47L35_DSP2_SYM_SILK_ENCODER_RING_BUFF_ADDRESS             (0xf)
#define CS47L35_DSP2_SYM_SILK_ENCODER_ERROR_LOG_ADDRESS             (0x10)
#define CS47L35_DSP2_SYM_SILK_ENCODER_BITRATE_BPS                   (0x11)
#define CS47L35_DSP2_SYM_SILK_ENCODER_PACKET_LOSS_PERC              (0x12)
#define CS47L35_DSP2_SYM_SILK_ENCODER_USE_DTX                       (0x13)
#define CS47L35_DSP2_SYM_SILK_ENCODER_SAMPLING_RATE                 (0x14)
#define CS47L35_DSP2_SYM_SILK_ENCODER_CHANNELS                      (0x15)
#define CS47L35_DSP2_SYM_SILK_ENCODER_COMPLEXITY                    (0x16)
#define CS47L35_DSP2_SYM_SILK_ENCODER_USE_VBR                       (0x17)
#define CS47L35_DSP2_SYM_SILK_ENCODER_USE_INBANDFEC                 (0x18)
#define CS47L35_DSP2_SYM_SILK_ENCODER_HIGH_WATERMARK_LEVEL          (0x19)
#define CS47L35_DSP2_SYM_SILK_ENCODER_SILK_ENCODER_YM_STRUCT_T      (0x1a)
#define CS47L35_DSP2_SYM_SILK_ENCODER_DUMMY_YM_CONFIG               (0x1b)
#define CS47L35_DSP2_SYM_SILK_ENCODER_SILK_ENCODER_ZM_STRUCT_T      (0x1c)
#define CS47L35_DSP2_SYM_SILK_ENCODER_SILK_ENCODER_GAIN             (0x1d)
// SILK_DECODER
#define CS47L35_DSP2_SYM_SILK_DECODER_SILK_DECODER_XM_STRUCT_T      (0x1e)
#define CS47L35_DSP2_SYM_SILK_DECODER_RING_BUFF_ADDRESS             (0x1f)
#define CS47L35_DSP2_SYM_SILK_DECODER_ERROR_LOG_ADDRESS             (0x20)
#define CS47L35_DSP2_SYM_SILK_DECODER_SAMPLING_RATE                 (0x21)
#define CS47L35_DSP2_SYM_SILK_DECODER_CHANNELS                      (0x22)
#define CS47L35_DSP2_SYM_SILK_DECODER_COMPLEXITY                    (0x23)
#define CS47L35_DSP2_SYM_SILK_DECODER_FRAME_SIZE                    (0x24)
#define CS47L35_DSP2_SYM_SILK_DECODER_HIGH_WATERMARK_LEVEL          (0x25)
#define CS47L35_DSP2_SYM_SILK_DECODER_SILK_DECODER_YM_STRUCT_T      (0x26)
#define CS47L35_DSP2_SYM_SILK_DECODER_DUMMY_YM_CONFIG               (0x27)
#define CS47L35_DSP2_SYM_SILK_DECODER_SILK_DECODER_ZM_STRUCT_T      (0x28)
#define CS47L35_DSP2_SYM_SILK_DECODER_SILK_DECODER_GAIN             (0x29)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS47L35_DSP2_SYM_H

