/**
 * @file main.c
 *
 * @brief The main function for CS35L42 System Test Harness
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022 All Rights Reserved, http://www.cirrus.com/
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
#include "semphr.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_CALIBRATE    (1)
#define APP_STATE_PLAY         (2)
#define APP_STATE_PLAY_GAIN    (3)
#define APP_STATE_STOP         (4)
#define APP_STATE_HIBERNATE    (5)
#define APP_STATE_WAKE         (6)

#define AMP_CONTROL_FLAG_PB_PRESSED         (1 << 0)
#define APP_FLAG_BSP_NOTIFICATION           (1 << 1)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_audio_state = APP_STATE_CALIBRATE;
static TaskHandle_t AmpControlTaskHandle = NULL;
static TaskHandle_t AmpEventTaskHandle = NULL;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
SemaphoreHandle_t mutex_boot;

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
        xTaskNotifyFromISR(AmpEventTaskHandle,
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

    xTaskNotifyFromISR(AmpControlTaskHandle,
                       (int32_t) arg,
                       eSetBits,
                       &xHigherPriorityTaskWoken);

    return;
}

void app_init(void)
{
    bsp_initialize(app_bsp_notification_callback, (void *) APP_FLAG_BSP_NOTIFICATION);
    bsp_register_pb_cb(BSP_PB_ID_USER, app_bsp_pb_callback, (void *) AMP_CONTROL_FLAG_PB_PRESSED);
    bsp_dut_initialize();

    bsp_set_ld2(BSP_LD2_MODE_ON, 0);

    return;
}

static void AmpControlThread(void *argument)
{
    uint32_t flags;
    cs35l42_t *temp_driver;
    bsp_dut_get_driver_handle((void **) &temp_driver);
    static fw_img_boot_state_t cs35l42_boot_state;
    static fw_img_boot_state_t cs35l42_cal_boot_state;

    for (;;)
    {
        /* Wait to be notified of an interrupt. */
        xTaskNotifyWait(pdFALSE,    /* Don't clear bits on entry. */
                        AMP_CONTROL_FLAG_PB_PRESSED,
                        &flags, /* Stores the notified value. */
                        portMAX_DELAY);

        switch (app_audio_state)
        {
            case APP_STATE_CALIBRATE:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_play_record(BSP_PLAY_SILENCE);
                    bsp_dut_reset();
                    bsp_dut_boot(temp_driver, &cs35l42_boot_state, cs35l42_fw_img, false);
                    // Load additional bin file for performing calibration
                    bsp_dut_boot(temp_driver, &cs35l42_cal_boot_state, cs35l42_cal_fw_img, true);
                    bsp_dut_power_up();
                    bsp_dut_calibrate();
                    bsp_dut_power_down();
                    app_audio_state++;
                }
                break;

            case APP_STATE_PLAY:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_play(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_dut_reset();
                    bsp_dut_boot(temp_driver, &cs35l42_boot_state, cs35l42_fw_img, false);
                    bsp_dut_power_up();
                    app_audio_state++;
                }
                break;

            case APP_STATE_PLAY_GAIN:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_play(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_dut_reset();
                    bsp_dut_boot(temp_driver, &cs35l42_boot_state, cs35l42_fw_img, false);
                    bsp_dut_set_dig_gain(-6);
                    bsp_dut_power_up();
                    app_audio_state++;
                }
                break;

            case APP_STATE_STOP:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_down();
                    app_audio_state++;
                }
                break;

            case APP_STATE_HIBERNATE:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_hibernate();
                    app_audio_state++;
                }
                break;

            case APP_STATE_WAKE:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_wake();
                    app_audio_state = APP_STATE_CALIBRATE;
                }
                break;

            default:
                break;
        }

        flags = 0;
    }
}

static void AmpEventThread(void *argument)
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

    xTaskCreate(AmpControlThread,
                "AmpControlTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                tskIDLE_PRIORITY,
                &AmpControlTaskHandle);
    xTaskCreate(AmpEventThread,
                "AmpEventTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                tskIDLE_PRIORITY,
                &AmpEventTaskHandle);

    mutex_boot = xSemaphoreCreateMutex();
    if( mutex_boot == NULL )
    {
        return BSP_STATUS_FAIL; /* There was insufficient heap memory available for the mutex to be created. */
    }

    app_init();

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);

    return ret_val;
}
