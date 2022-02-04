/**
 * @file main.c
 *
 * @brief The main function for CS47L63 System Test Harness
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
#define APP_STATE_UNINITIALIZED         (0)
#define APP_STATE_STANDBY               (1)
#define APP_STATE_TG_HP                 (2)
#define APP_STATE_STANDBY2              (3)
#define APP_STATE_DSP_PRELOAD_PT        (4)
#define APP_STATE_TG_DSP_HP             (5)
#define APP_STATE_MIC_DSP_HP            (6)
#define APP_STATE_DSP_DISABLE           (7)

#define AUDIO_CONTROL_FLAG_PB_PRESSED   (1 << 0)
#define APP_FLAG_BSP_NOTIFICATION       (1 << 1)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_STANDBY;
static TaskHandle_t AudioControlTaskHandle = NULL;
static TaskHandle_t AudioEventTaskHandle = NULL;

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
        xTaskNotifyFromISR(AudioEventTaskHandle,
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

    xTaskNotifyFromISR(AudioControlTaskHandle,
                       (int32_t) arg,
                       eSetBits,
                       &xHigherPriorityTaskWoken);

    return;
}

void app_init(void)
{
    bsp_initialize(app_bsp_notification_callback, (void *) APP_FLAG_BSP_NOTIFICATION);
    bsp_register_pb_cb(BSP_PB_ID_USER, app_bsp_pb_callback, (void *) AUDIO_CONTROL_FLAG_PB_PRESSED);
    bsp_dut_initialize();

    bsp_set_ld2(BSP_LD2_MODE_ON, 0);

    return;
}

static void AudioControlThread(void *argument)
{
    uint32_t flags = 0;

    for (;;)
    {

        /* Wait to be notified of an interrupt. */
        xTaskNotifyWait(pdFALSE,    /* Don't clear bits on entry. */
                        -1,
                        &flags, /* Stores the notified value. */
                        portMAX_DELAY);

        switch (app_state)
        {
            case APP_STATE_STANDBY:
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_TG_HP_EN);
                    app_state++;
                }
                break;

            case APP_STATE_TG_HP:
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_TG_HP_DIS);
                    app_state++;
                }
                break;

            case APP_STATE_STANDBY2:
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_DSP_PRELOAD_PT_EN);
                    app_state++;
                }
                break;

            case APP_STATE_DSP_PRELOAD_PT:
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_TG_DSP_HP_EN);
                    app_state++;
                }
                break;

            case APP_STATE_TG_DSP_HP:
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_TG_DSP_HP_DIS);
                    bsp_dut_use_case(BSP_USE_CASE_MIC_DSP_HP_EN);
                    app_state++;
                }
                break;

            case APP_STATE_MIC_DSP_HP:
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_MIC_DSP_HP_DIS);
                    bsp_dut_use_case(BSP_USE_CASE_DSP_PRELOAD_PT_DIS);
                    app_state = APP_STATE_STANDBY;
                }
                break;

            default:
                break;
        }

        flags = 0;
    }
}

static void AudioEventThread(void *argument)
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

    xTaskCreate(AudioControlThread,
                "AudioControlTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                tskIDLE_PRIORITY,
                &AudioControlTaskHandle);

    xTaskCreate(AudioEventThread,
                "AudioEventTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                (tskIDLE_PRIORITY + 1),
                &AudioEventTaskHandle);

    app_init();

    bsp_dut_reset();

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);

    return ret_val;
}
