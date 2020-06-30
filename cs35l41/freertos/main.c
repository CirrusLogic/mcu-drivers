/**
 * @file main.c
 *
 * @brief The main function for CS35L41 System Test Harness
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
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stddef.h>
#include <stdlib.h>
#include "hw_0_bsp.h"
#include "FreeRTOS.h"
#include "task.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_CAL_PDN               (0)
#define APP_STATE_CAL_BOOTED            (1)
#define APP_STATE_CAL_PUP               (2)
#define APP_STATE_CALIBRATED            (3)
#define APP_STATE_PDN                   (4)
#define APP_STATE_BOOTED                (5)
#define APP_STATE_PUP                   (6)
#define APP_STATE_MUTE                  (7)
#define APP_STATE_UNMUTE                (8)
#define APP_STATE_HIBERNATE             (9)
#define APP_STATE_WAKE                  (10)
#define APP_STATE_CHECK_PROCESSING      (11)

#define AMP_CONTROL_FLAG_PB_PRESSED         (1 << 0)
#define APP_FLAG_BSP_NOTIFICATION           (1 << 1)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_audio_state = APP_STATE_CAL_PDN;
static TaskHandle_t AmpControlTaskHandle = NULL;
static TaskHandle_t AmpEventTaskHandle = NULL;

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
                    bsp_audio_play_record(BSP_PLAY_SILENCE);
                    bsp_dut_reset();
                    bsp_dut_boot(true);
                    app_audio_state = APP_STATE_CAL_BOOTED;
                }
                break;

            case APP_STATE_CAL_BOOTED:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_up();
                    app_audio_state = APP_STATE_CAL_PUP;
                }
                break;

            case APP_STATE_CAL_PUP:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_calibrate();
                    app_audio_state = APP_STATE_CALIBRATED;
                }
                break;

            case APP_STATE_CALIBRATED:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_down();
                    app_audio_state = APP_STATE_PDN;
                }
                break;

            case APP_STATE_PDN:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_audio_stop();
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_dut_reset();
                    bsp_dut_boot(false);
                    app_audio_state = APP_STATE_BOOTED;
                }
                break;

            case APP_STATE_BOOTED:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_up();
                    app_audio_state = APP_STATE_CHECK_PROCESSING;
                }
                break;

            case APP_STATE_CHECK_PROCESSING:
                if (flags & AMP_CONTROL_FLAG_PB_PRESSED)
                {
                    bool is_processing = false;
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
                    bsp_dut_mute(false);
                    app_audio_state = APP_STATE_UNMUTE;
                }
                break;

            case APP_STATE_UNMUTE:
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

    app_init();

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);

    return ret_val;
}
