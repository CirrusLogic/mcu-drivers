/**
 * @file hw_0_bsp.h
 *
 * @brief Functions and prototypes exported by the BSP module for the HW ID0 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
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
#include "hw_0_bsp_dut.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DEV_ID_NULL                 (0)
#define BSP_DUT_DEV_ID                  (1)
#define BSP_LN2_DEV_ID                  (2)

#define BSP_GPIO_ID_DUT_RESET           (1)
#define BSP_GPIO_ID_DUT_INT             (2)

#define BSP_PB_ID_USER                  (0)

#define BSP_PLAY_SILENCE                (0)
#define BSP_PLAY_STEREO_1KHZ_20DBFS     (1)
#define BSP_PLAY_STEREO_100HZ_20DBFS    (2)
#define BSP_PLAY_STEREO_PATTERN         (3)

#define BSP_BUS_TYPE_I2C                (0)

#define BSP_STATUS_DUT_EVENTS           (2)

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

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_initialize(bsp_app_callback_t cb, void *cb_arg);
uint32_t bsp_audio_play(uint8_t content);
uint32_t bsp_audio_play_record(uint8_t content);
uint32_t bsp_audio_pause(void);
uint32_t bsp_audio_resume(void);
uint32_t bsp_audio_stop(void);
bool bsp_was_pb_pressed(uint8_t pb_id);
void bsp_sleep(void);
uint32_t bsp_register_pb_cb(uint32_t pb_id, bsp_app_callback_t cb, void *cb_arg);
void bsp_notification_callback(uint32_t event_flags, void *arg);
uint32_t bsp_i2c_write(uint32_t bsp_dev_id,
                       uint8_t *write_buffer,
                       uint32_t write_length,
                       bsp_callback_t cb,
                       void *cb_arg);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // HW_0_BSP_H
