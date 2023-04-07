/**
 * @file main.c
 *
 * @brief The main function for CS47L63 System Test Harness
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2023 All Rights Reserved, http://www.cirrus.com/
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
#include "debug.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_UNINITIALIZED           (0)
#define APP_STATE_STANDBY                 (1)
#define APP_STATE_SCC_RECORD_P16          (2)
#define APP_STATE_SCC_MANUAL_TRIGGER_P16  (3)
#define APP_STATE_SCC_PROCESS_IRQ_P16     (4)
#define APP_STATE_STANDBY2                (5)
#define APP_STATE_SCC_RECORD_MSBC         (6)
#define APP_STATE_SCC_MANUAL_TRIGGER_MSBC (7)
#define APP_STATE_SCC_PROCESS_IRQ_MSBC    (8)

#define AUDIO_CONTROL_FLAG_PB_PRESSED   (1 << 0)
#define APP_FLAG_BSP_NOTIFICATION       (1 << 1)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_STANDBY;
static TaskHandle_t AudioControlTaskHandle = NULL;
static TaskHandle_t AudioEventTaskHandle = NULL;
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
        xTaskNotifyFromISR(AudioEventTaskHandle,
                           (int32_t) arg,
                           eSetBits,
                           &xHigherPriorityTaskWoken);
        xTaskNotifyFromISR(AudioControlTaskHandle,
                           (int32_t) arg,
                           eSetBits,
                           NULL);
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

    if (xHigherPriorityTaskWoken == pdTRUE)
    {
        portYIELD();
    }

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
                        0);

        switch (app_state)
        {
            case APP_STATE_STANDBY:
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_SCC_RECORD_PACKED16);
                    app_state++;
                }
                break;

            case APP_STATE_SCC_RECORD_P16:
            case APP_STATE_SCC_RECORD_MSBC:
                if (bsp_process_irq)
                {
                    // Triggered by phrase so switch state.
                    if (bsp_dut_use_case(BSP_USE_CASE_SCC_TRIGGERED) != BSP_STATUS_FAIL)
                    {
                        app_state += 2;
                    }
                }
                else if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_SCC_MANUAL_TRIGGER);
                    app_state++;
                }
                break;

            case APP_STATE_SCC_MANUAL_TRIGGER_P16:
            case APP_STATE_SCC_MANUAL_TRIGGER_MSBC:
                // Triggered either by phrase or button press so switch state.
                bsp_dut_use_case(BSP_USE_CASE_SCC_TRIGGERED);
                app_state++;
                break;

            case APP_STATE_SCC_PROCESS_IRQ_P16:
                if (bsp_process_irq)
                {
                    bsp_dut_use_case(BSP_USE_CASE_SCC_PROCESS_IRQ);
                }
                if (bsp_process_i2s)
                {
                    bsp_dut_use_case(BSP_USE_CASE_SCC_PROCESS_I2S);
                }
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_SCC_STOP_RECORDING);
                    app_state++;
                }
                break;

            case APP_STATE_STANDBY2:
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_SCC_RECORD_MSBC);
                    app_state++;
                }
                break;

            case APP_STATE_SCC_PROCESS_IRQ_MSBC:
                if (bsp_process_irq)
                {
                    bsp_dut_use_case(BSP_USE_CASE_SCC_PROCESS_IRQ);
                }
                if (bsp_process_i2s)
                {
                    bsp_dut_use_case(BSP_USE_CASE_SCC_PROCESS_I2S);
                }
                if (flags & AUDIO_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_use_case(BSP_USE_CASE_SCC_STOP_RECORDING);
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
                        pdFALSE,
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

    xTaskCreate(BridgeThread,
                "BridgeTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                tskIDLE_PRIORITY,
                &BridgeTaskHandle);

    app_init();

    bsp_dut_reset();

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);

    return ret_val;
}
