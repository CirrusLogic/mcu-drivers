/**
 * @file main.c
 *
 * @brief The main function for CS35L41 System Test Harness
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019, 2021-2022 All Rights Reserved, http://www.cirrus.com/
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
#include "bridge.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_CAL_PDN               (0)
#define APP_STATE_PDN                   (1)
#define APP_STATE_CHANGE_44P1KHZ        (2)
#define APP_STATE_CHANGE_48KHZ          (3)
#define APP_STATE_PUP                   (4)
#define APP_STATE_MUTE                  (5)
#define APP_STATE_HIBERNATE             (6)
#define APP_STATE_WAKE                  (7)

#define AMP_CONTROL_FLAG_PB_PRESSED         (1 << 0)
#define APP_FLAG_BSP_NOTIFICATION           (1 << 1)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_audio_state = APP_STATE_CAL_PDN;
static TaskHandle_t AmpControlTaskHandle = NULL;
static TaskHandle_t AmpEventTaskHandle = NULL;
static TaskHandle_t BridgeTaskHandle = NULL;

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

    for (;;)
    {
        /* Wait to be notified of an interrupt. */
        xTaskNotifyWait(pdFALSE,    /* Don't clear bits on entry. */
                        AMP_CONTROL_FLAG_PB_PRESSED,
                        &flags, /* Stores the notified value. */
                        portMAX_DELAY);

        switch (app_audio_state)
        {
            case APP_STATE_CAL_PDN:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_play_record(BSP_PLAY_SILENCE);
                    bsp_dut_reset();
                    bsp_dut_boot(true);
                    bsp_dut_power_up();
                    bsp_dut_calibrate();
                    bsp_dut_power_down();
                    app_audio_state = APP_STATE_PDN;
                }
                break;

            case APP_STATE_PDN:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_dut_reset();
                    bsp_dut_boot(false);
                    uint8_t dut_id;
                    bsp_dut_get_id(&dut_id);
                    if (dut_id == BSP_DUT_ID_LEFT)
                    {
                        bsp_dut_set_dig_gain(-6);
                    }
                    else
                    {
                        bsp_dut_set_dig_gain(-10);
                    }
                    bsp_dut_power_up();
                    app_audio_state = APP_STATE_CHANGE_44P1KHZ;
                }
                break;

            case APP_STATE_CHANGE_44P1KHZ:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bool is_processing = false;
                    bsp_dut_is_processing(&is_processing);

                    bsp_dut_change_fs(BSP_AUDIO_FS_44100_HZ);
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_44100_HZ);
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);

                    bsp_dut_is_processing(&is_processing);

                    if (is_processing)
                    {
                        app_audio_state = APP_STATE_CHANGE_48KHZ;
                    }
                }
                break;

            case APP_STATE_CHANGE_48KHZ:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bool is_processing = false;
                    bsp_dut_is_processing(&is_processing);

                    bsp_dut_change_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);

                    bsp_dut_is_processing(&is_processing);

                    if (is_processing)
                    {
                        app_audio_state = APP_STATE_PUP;
                    }
                }
                break;

            case APP_STATE_PUP:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_mute(true);
                    app_audio_state = APP_STATE_MUTE;
                }
                break;

            case APP_STATE_MUTE:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_down();
                    app_audio_state = APP_STATE_HIBERNATE;
                }
                break;

            case APP_STATE_HIBERNATE:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_hibernate();
                    app_audio_state = APP_STATE_WAKE;
                }
                break;

            case APP_STATE_WAKE:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_wake();
                    app_audio_state = APP_STATE_CAL_PDN;
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

static void BridgeThread(void *argument)
{
    const TickType_t pollingTime = pdMS_TO_TICKS(5);
    while(true)
    {
        bridge_process();
        vTaskDelay(pollingTime);
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

    xTaskCreate(BridgeThread,
                "BridgeTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                tskIDLE_PRIORITY,
                &BridgeTaskHandle);

    app_init();

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);

    return ret_val;
}
