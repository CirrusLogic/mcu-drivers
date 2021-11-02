/**
 * @file main.c
 *
 * @brief The main function for CS40L25 System Test Harness
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019, 2021 All Rights Reserved, http://www.cirrus.com/
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
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stddef.h>
#include <stdlib.h>
#include "platform_bsp.h"
#include "FreeRTOS.h"
#include "task.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_BUZZ          (0)
#define APP_STATE_CALIBRATE     (1)
#define APP_STATE_CONFIG_0      (2)
#define APP_STATE_CONFIG_1      (3)
#define APP_STATE_DYNAMIC_F0    (4)
#define APP_STATE_START_I2S     (5)
#define APP_STATE_STOP_I2S      (6)
#define APP_STATE_WAKE          (7)

#define HAPTIC_CONTROL_FLAG_PB_PRESSED      (1 << 0)
#define APP_FLAG_BSP_NOTIFICATION           (1 << 1)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_BUZZ;
static TaskHandle_t HapticControlTaskHandle = NULL;
static TaskHandle_t HapticEventTaskHandle = NULL;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
void app_bsp_notification_callback(uint32_t status, void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (status == BSP_STATUS_FAIL)
    {
        exit(1);
    }
    else if (status == BSP_STATUS_DUT_EVENTS)
    {
        xTaskNotifyFromISR(HapticEventTaskHandle,
                           (int32_t) arg,
                           eSetBits,
                           &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD();
        }
    }

    return;
}

void app_bsp_pb_callback(uint32_t status, void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (status != BSP_STATUS_OK)
    {
        exit(1);
    }

    xTaskNotifyFromISR(HapticControlTaskHandle,
                       (int32_t) arg,
                       eSetBits,
                       &xHigherPriorityTaskWoken);

    return;
}

void app_init(void)
{
    bsp_initialize(app_bsp_notification_callback, (void *) APP_FLAG_BSP_NOTIFICATION);
    bsp_register_pb_cb(BSP_PB_ID_USER, app_bsp_pb_callback, (void *) HAPTIC_CONTROL_FLAG_PB_PRESSED);
    bsp_dut_initialize();
    bsp_dut_reset();

    bsp_set_ld2(BSP_LD2_MODE_ON, 0);

    return;
}

static void HapticControlThread(void *argument)
{
    uint32_t flags = 0;

    for (;;)
    {

        /* Wait to be notified of an interrupt. */
        xTaskNotifyWait(pdFALSE,    /* Don't clear bits on entry. */
                        HAPTIC_CONTROL_FLAG_PB_PRESSED,
                        &flags, /* Stores the notified value. */
                        portMAX_DELAY);

        switch (app_state)
        {
            case APP_STATE_BUZZ:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
#ifndef CONFIG_OPEN_LOOP
                    bsp_dut_trigger_haptic(BSP_DUT_TRIGGER_HAPTIC_POWER_ON, 0);
#endif
                    app_state++;
                }
                break;

            case APP_STATE_CALIBRATE:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_down();
                    bsp_dut_boot(true);
                    bsp_dut_power_up();
                    bsp_dut_calibrate();
                    app_state++;
                }
                break;

            case APP_STATE_CONFIG_0:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bool has_processed;

                    bsp_dut_power_down();
                    bsp_dut_boot(false);
                    bsp_dut_update_haptic_config(0);
                    bsp_dut_enable_haptic_processing(false);
                    bsp_dut_power_up();
                    bsp_dut_has_processed(&has_processed);
                    bsp_dut_trigger_haptic(0x1, 0);
                    app_state++;
                }
                break;

            case APP_STATE_CONFIG_1:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bool has_processed;

                    bsp_dut_has_processed(&has_processed);
                    bsp_dut_update_haptic_config(1);
                    bsp_dut_enable_haptic_processing(true);
                    bsp_dut_trigger_haptic(0x0, 1000);
                    app_state++;
                }
                break;

            case APP_STATE_DYNAMIC_F0:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_dynamic_calibrate();
                    app_state++;
                }
                break;

            case APP_STATE_START_I2S:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
#if CONFIG_8K_I2S
                    bsp_audio_set_fs(BSP_AUDIO_FS_8000_HZ);
#endif
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_dut_start_i2s();
                    app_state++;
                }
                break;

            case APP_STATE_STOP_I2S:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_stop_i2s();
                    bsp_audio_stop();
                    bsp_dut_hibernate();
                    app_state++;
                }
                break;

            case APP_STATE_WAKE:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_wake();
                    bsp_dut_power_down();
                    app_state = APP_STATE_CALIBRATE;
                }
                break;

            default:
                break;
        }

        flags = 0;
    }
}

static void HapticEventThread(void *argument)
{
    uint32_t flags = 0;

    for (;;)
    {
        /* Wait to be notified of an interrupt. */
        xTaskNotifyWait(pdFALSE,    /* Don't clear bits on entry. */
                        APP_FLAG_BSP_NOTIFICATION,
                        &flags, /* Stores the notified value. */
                        portMAX_DELAY);

        bsp_dut_process();

        flags = 0;
    }
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

int main(void)
{
    int ret_val = 0;

    xTaskCreate(HapticControlThread,
                "HapticControlTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                tskIDLE_PRIORITY,
                &HapticControlTaskHandle);

    xTaskCreate(HapticEventThread,
                "HapticEventTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                (tskIDLE_PRIORITY + 1),
                &HapticEventTaskHandle);

    app_init();

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);

    return ret_val;
}
