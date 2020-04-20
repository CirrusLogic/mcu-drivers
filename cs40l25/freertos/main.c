/**
 * @file main.c
 *
 * @brief The main function for CS40L25 System Test Harness
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
#include "system_test_hw_0_bsp.h"
#include "FreeRTOS.h"
#include "task.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
//#define ONLY_PB_THREAD
//#define INCLUDE_CALIBRATION

#define APP_AUDIO_STATE_CALIBRATING     (0)
#define APP_AUDIO_STATE_BOOTING         (1)
#define APP_AUDIO_STATE_PDN             (2)
#define APP_AUDIO_STATE_PUP             (3)
#define APP_AUDIO_STATE_MUTE            (4)
#define APP_AUDIO_STATE_UNMUTE          (5)

#define APP_AMP_AUDIO_THREAD_FLAG_PB_PRESSED    (1 << 0)
#define APP_AMP_AUDIO_THREAD_FLAG_BSP_CB        (1 << 1)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_audio_state = APP_AUDIO_STATE_PDN;
static bool is_calibrated = false;
static TaskHandle_t AppAmpAudioTaskHandle = NULL;
static TaskHandle_t BspTaskHandle = NULL;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
void app_bsp_callback(uint32_t status, void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (status != BSP_STATUS_OK)
    {
        exit(1);
    }

    xTaskNotifyFromISR(AppAmpAudioTaskHandle,
                       (int32_t) arg,
                        eSetBits,
                        &xHigherPriorityTaskWoken);

    return;
}

void app_init(void)
{
    bsp_initialize(app_bsp_callback, (void *) APP_AMP_AUDIO_THREAD_FLAG_BSP_CB);
    bsp_register_pb_cb(BSP_PB_ID_USER, app_bsp_callback, (void *) APP_AMP_AUDIO_THREAD_FLAG_PB_PRESSED);
    bsp_haptic_initialize(BOOT_HAPTIC_TYPE_WT);

#ifdef INCLUDE_CALIBRATION
    bsp_audio_play_record(BSP_PLAY_SILENCE);
    bsp_haptic_boot(false);
    is_calibrated = false;
#else
    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
    bsp_haptic_boot(false);
    is_calibrated = true;
#endif
    app_audio_state = APP_AUDIO_STATE_PDN;

    return;
}

#ifdef ONLY_PB_THREAD
static void AppShowPbPressedThread(void *argument)
{
    uint32_t flags;

    for (;;)
    {
        /* Wait to be notified of an interrupt. */
        xTaskNotifyWait(pdFALSE,    /* Don't clear bits on entry. */
                        APP_AMP_AUDIO_THREAD_FLAG_PB_PRESSED,
                        &flags, /* Stores the notified value. */
                        portMAX_DELAY);
        if (flags & APP_AMP_AUDIO_THREAD_FLAG_PB_PRESSED)
        {
            bsp_toggle_gpio(BSP_GPIO_ID_LD2);
        }
    }

    return;
}
#else
static void AppAmpAudioThread(void *argument)
{
    uint32_t flags;

    for (;;)
    {
        /* Wait to be notified of an interrupt. */
        xTaskNotifyWait(pdFALSE,    /* Don't clear bits on entry. */
                        APP_AMP_AUDIO_THREAD_FLAG_PB_PRESSED | APP_AMP_AUDIO_THREAD_FLAG_BSP_CB,
                        &flags, /* Stores the notified value. */
                        portMAX_DELAY);

        switch (app_audio_state)
        {
            case APP_AUDIO_STATE_CALIBRATING:
                if (flags & APP_AMP_AUDIO_THREAD_FLAG_BSP_CB)
                {
                    is_calibrated = true;
                    bsp_audio_stop();
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_haptic_boot(false);
                    app_audio_state = APP_AUDIO_STATE_BOOTING;
                }
                break;

            case APP_AUDIO_STATE_BOOTING:
                if (flags & APP_AMP_AUDIO_THREAD_FLAG_BSP_CB)
                {
                    app_audio_state = APP_AUDIO_STATE_PDN;
                }
                break;

            case APP_AUDIO_STATE_PDN:
                if (flags & APP_AMP_AUDIO_THREAD_FLAG_PB_PRESSED)
                {
                    bsp_haptic_power_up();
                    app_audio_state = APP_AUDIO_STATE_PUP;
                }
                break;

            case APP_AUDIO_STATE_PUP:
                if ((flags & APP_AMP_AUDIO_THREAD_FLAG_PB_PRESSED) && (is_calibrated))
                {
                    bsp_haptic_mute(true);
                    app_audio_state = APP_AUDIO_STATE_MUTE;
                }
                else if ((flags == APP_AMP_AUDIO_THREAD_FLAG_BSP_CB) && (!is_calibrated))
                {
                    bsp_haptic_calibrate();
                    app_audio_state = APP_AUDIO_STATE_CALIBRATING;
                }
                break;

            case APP_AUDIO_STATE_MUTE:
                if (flags & APP_AMP_AUDIO_THREAD_FLAG_PB_PRESSED)
                {
                    bsp_haptic_mute(false);
                    app_audio_state = APP_AUDIO_STATE_UNMUTE;
                }
                break;

            case APP_AUDIO_STATE_UNMUTE:
                if (flags & APP_AMP_AUDIO_THREAD_FLAG_PB_PRESSED)
                {
                    bsp_haptic_power_down();
                    app_audio_state = APP_AUDIO_STATE_PDN;
                }
                break;

            default:
                break;
        }
    }
}

static void AppBspThread(void *argument)
{
    for (;;)
    {
        bsp_haptic_process();
    }
}
#endif

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

int main(void)
{
    int ret_val = 0;

    app_init();

#ifdef ONLY_PB_THREAD
    xTaskCreate(AppShowPbPressedThread,
                "AppShowPbPressedTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                tskIDLE_PRIORITY,
                &AppAmpAudioTaskHandle);
#else
    xTaskCreate(AppAmpAudioThread,
                "AppAmpAudioTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                tskIDLE_PRIORITY,
                &AppAmpAudioTaskHandle);
    xTaskCreate(AppBspThread,
                "BspTask",
                configMINIMAL_STACK_SIZE,
                (void *) NULL,
                tskIDLE_PRIORITY,
                &BspTaskHandle);
#endif

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);

    return ret_val;
}
