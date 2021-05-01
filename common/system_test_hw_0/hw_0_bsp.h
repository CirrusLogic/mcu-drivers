/**
 * @file hw_0_bsp.h
 *
 * @brief Functions and prototypes exported by the BSP module for the HW ID0 platform.
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

#ifndef HW_0_BSP_H
#define HW_0_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "bsp_driver_if.h"
#include <stdbool.h>
#include <stddef.h>
#include "hw_0_bsp_dut.h"
#include <stdio.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DEV_ID_NULL                 (0)
#define BSP_DUT_DEV_ID                  (1)
#define BSP_LN2_DEV_ID                  (2)
#define BSP_DUT_DEV_ID_SPI2             (3)

#define BSP_GPIO_ID_NULL                (0)
#define BSP_GPIO_ID_DUT_CDC_RESET       (1)
#define BSP_GPIO_ID_DUT_DSP_RESET       (2)
#define BSP_GPIO_ID_DUT_CDC_INT         (3)
#define BSP_GPIO_ID_DUT_DSP_INT         (4)
#define BSP_GPIO_ID_LN2_CDC_GPIO1       (5)
#define BSP_GPIO_ID_GF_GPIO7            (6)
#define BSP_GPIO_ID_GF_GPIO2            (7)

#define BSP_SUPPLY_ID_LN2_DCVDD         (1)

#define BSP_PB_ID_USER                  (0)

#define BSP_PLAY_SILENCE                (0)
#define BSP_PLAY_STEREO_1KHZ_20DBFS     (1)
#define BSP_PLAY_STEREO_100HZ_20DBFS    (2)
#define BSP_PLAY_STEREO_PATTERN         (3)

#define BSP_BUS_TYPE_I2C                (0)
#define BSP_BUS_TYPE_SPI                (1)

#define BSP_STATUS_DUT_EVENTS           (2)

#define BSP_AUDIO_FS_48000_HZ           (48000)
#define BSP_AUDIO_FS_44100_HZ           (44100)

#define BSP_LD2_MODE_OFF                (0)
#define BSP_LD2_MODE_ON                 (1)
#define BSP_LD2_MODE_BLINK              (2)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
typedef void (*bsp_app_callback_t)(uint32_t status, void *arg);

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern bool trigger_audio_change;
extern bool bsp_write_process_done;
extern FILE* test_file;
extern FILE* coverage_file;

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_initialize(bsp_app_callback_t cb, void *cb_arg);
uint32_t bsp_audio_set_fs(uint32_t fs_hz);
uint32_t bsp_audio_play(uint8_t content);
uint32_t bsp_audio_play_record(uint8_t content);
uint32_t bsp_audio_pause(void);
uint32_t bsp_audio_resume(void);
uint32_t bsp_audio_stop(void);
uint32_t bsp_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg);
bool     bsp_was_pb_pressed(uint8_t pb_id);
void     bsp_sleep(void);
uint32_t bsp_register_pb_cb(uint32_t pb_id, bsp_app_callback_t cb, void *cb_arg);
void     bsp_notification_callback(uint32_t event_flags, void *arg);
uint32_t bsp_i2c_write(uint32_t bsp_dev_id,
                       uint8_t *write_buffer,
                       uint32_t write_length,
                       bsp_callback_t cb,
                       void *cb_arg);
void*    bsp_malloc(size_t size);
void     bsp_free(void* ptr);
uint32_t bsp_set_ld2(uint8_t mode, uint32_t blink_100ms);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // HW_0_BSP_H
