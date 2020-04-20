/**
 * @file system_test_hw_0_bsp.h
 *
 * @brief Functions and prototypes exported by the BSP module for the system_test_hw_0 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
 *
 */

#ifndef SYSTEM_TEST_HW_0_BSP_H
#define SYSTEM_TEST_HW_0_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "bsp_driver_if.h"
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#if defined(TARGET_CS35L41)
#define BSP_INCLUDE_AMP
#elif defined (TARGET_CS40L25)
#define BSP_INCLUDE_HAPTIC
#endif

#define BSP_DEV_ID_NULL                 (0)
#ifdef TARGET_CS35L41
#define BSP_AMP_DEV_ID                  (1)
#endif
#ifdef TARGET_CS40L25
#define BSP_AMP_DEV_ID                  (2)
#endif

#define BSP_GPIO_ID_LD2                 (0)
#ifdef TARGET_CS35L41
#define BSP_GPIO_ID_CS35L41_RESET       (1)
#define BSP_GPIO_ID_CS35L41_INT         (2)
#endif
#ifdef TARGET_CS40L25
#define BSP_GPIO_ID_CS40L25_RESET       (1)
#define BSP_GPIO_ID_CS40L25_INT         (2)
#endif
#define BSP_GPIO_ID_USER_PB             (3)

#define BSP_PB_ID_USER                  (0)

#define BSP_PLAY_SILENCE                (0)
#define BSP_PLAY_STEREO_1KHZ_20DBFS     (1)
#define BSP_PLAY_STEREO_100HZ_20DBFS    (2)
#define BSP_PLAY_STEREO_PATTERN         (3)

#define BOOT_AMP_TYPE_NO_FW             (0)
#define BOOT_AMP_TYPE_NO_TUNE           (1)
#define BOOT_AMP_TYPE_CALIBRATION_TUNE  (2)
#define BOOT_AMP_TYPE_NORMAL_TUNE       (3)

#define BOOT_HAPTIC_TYPE_NO_BIN         (1 << 0)
#define BOOT_HAPTIC_TYPE_WT             (1 << 1)
#define BOOT_HAPTIC_TYPE_CLAB           (1 << 2)
#define BOOT_HAPTIC_TYPE_CAL            (1 << 3)

#ifdef TARGET_CS40L25
#define BSP_HAPTIC_CONTROL_GET_HALO_HEARTBEAT (8)
#define BSP_HAPTIC_CONTROL_SET_BHM_BUZZ_TRIGGER (9)
#define BSP_HAPTIC_CONTROL_SET_TRIGGER_INDEX (12)
#define BSP_HAPTIC_CONTROL_SET_TRIGGER_MS (13)
#define BSP_HAPTIC_CONTROL_SET_TIMEOUT_MS (14)
#define BSP_HAPTIC_CONTROL_SET_GPIO_ENABLE (15)
#define BSP_HAPTIC_CONTROL_SET_GPIO1_BUTTON_DETECT (16)
#define BSP_HAPTIC_CONTROL_SET_GPIO2_BUTTON_DETECT (17)
#define BSP_HAPTIC_CONTROL_SET_GPIO3_BUTTON_DETECT (18)
#define BSP_HAPTIC_CONTROL_SET_GPIO4_BUTTON_DETECT (19)
#define BSP_HAPTIC_CONTROL_SET_GPI_GAIN_CONTROL (20)
#define BSP_HAPTIC_CONTROL_SET_CTRL_PORT_GAIN_CONTROL (21)
#define BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_PRESS (22)
#define BSP_HAPTIC_CONTROL_SET_GPIO2_INDEX_BUTTON_PRESS (23)
#define BSP_HAPTIC_CONTROL_SET_GPIO3_INDEX_BUTTON_PRESS (24)
#define BSP_HAPTIC_CONTROL_SET_GPIO4_INDEX_BUTTON_PRESS (25)
#define BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_RELEASE (26)
#define BSP_HAPTIC_CONTROL_SET_GPIO2_INDEX_BUTTON_RELEASE (27)
#define BSP_HAPTIC_CONTROL_SET_GPIO3_INDEX_BUTTON_RELEASE (28)
#define BSP_HAPTIC_CONTROL_SET_GPIO4_INDEX_BUTTON_RELEASE (29)
#define BSP_HAPTIC_CONTROL_SET_CLAB_ENABLED (30)
#endif

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
uint32_t bsp_initialize(bsp_app_callback_t *cb, void *cb_arg);
#ifdef TARGET_CS35L41
uint32_t bsp_amp_initialize(void);
uint32_t bsp_amp_boot(uint8_t boot_type);
uint32_t bsp_amp_calibrate(void);
uint32_t bsp_amp_power_up(void);
uint32_t bsp_amp_power_down(void);
uint32_t bsp_amp_mute(bool is_mute);
uint32_t bsp_amp_is_processing(bool *is_processing);
uint32_t bsp_amp_process(void);
#endif
#ifdef TARGET_CS40L25
uint32_t bsp_haptic_initialize(uint8_t boot_type);
uint32_t bsp_haptic_reset();
uint32_t bsp_haptic_boot(bool cal_boot);
uint32_t bsp_haptic_calibrate(void);
uint32_t bsp_haptic_power_up(void);
uint32_t bsp_haptic_power_down(void);
uint32_t bsp_haptic_mute(bool is_mute);
uint32_t bsp_haptic_is_processing(bool *is_processing);
uint32_t bsp_haptic_process(void);
uint32_t bsp_haptic_control(uint32_t id, uint32_t arg);
#endif
uint32_t bsp_audio_play(uint8_t content);
uint32_t bsp_audio_play_record(uint8_t content);
uint32_t bsp_audio_pause(void);
uint32_t bsp_audio_resume(void);
uint32_t bsp_audio_stop(void);
bool bsp_was_pb_pressed(uint8_t pb_id);
void bsp_sleep(void);
uint32_t bsp_register_pb_cb(uint32_t pb_id, bsp_app_callback_t cb, void *cb_arg);
uint32_t bsp_toggle_gpio(uint32_t gpio_id);
/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // SYSTEM_TEST_HW_0_BSP_H
