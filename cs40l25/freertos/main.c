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
#include <stdlib.h>
#include "hw_0_bsp.h"
#include "FreeRTOS.h"
#include "task.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_UNINITIALIZED             (0)
#define APP_STATE_PUP                       (1)
#define APP_STATE_BUZZ                      (2)
#define APP_STATE_PDN                       (3)
#define APP_STATE_BOOTED_CAL                (4)
#define APP_STATE_POWER_UP_CAL              (5)
#define APP_STATE_CAL_DONE                  (6)
#define APP_STATE_PDN_2                     (7)
#define APP_STATE_BOOTED                    (8)
#define APP_STATE_POWER_UP                  (9)
#define APP_STATE_POWER_UP_NO_GPI           (10)
#define APP_STATE_DYNAMIC_F0                (11)
#define APP_STATE_POWER_UP_GPI              (12)
#define APP_STATE_I2S_ENABLED               (13)
#define APP_STATE_I2S_ENABLED_PLAY_TONE     (14)
#define APP_STATE_I2S_DISABLED              (15)
#define APP_STATE_AUDIO_STOPPED             (16)
#define APP_STATE_HIBERNATE                 (17)
#define APP_STATE_WAKE                      (18)

#define HAPTIC_CONTROL_FLAG_PB_PRESSED      (1 << 0)
#define APP_FLAG_BSP_NOTIFICATION           (1 << 1)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_PUP;
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
            case APP_STATE_PUP:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
#ifndef CONFIG_TEST_OPEN_LOOP
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_BHM_BUZZ_TRIGGER, (void *) 0x1);
#endif
                    app_state++;
                }
                break;

            case APP_STATE_BUZZ:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_down();
                    app_state++;
                }
                break;

            case APP_STATE_PDN:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_boot(true);
                    app_state++;
                }
                break;

            case APP_STATE_BOOTED_CAL:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_up();
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP_CAL:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_calibrate();
                    app_state++;
                }
                break;

            case APP_STATE_CAL_DONE:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_down();
                    app_state++;
                }
                break;

            case APP_STATE_PDN_2:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_boot(false);
                    app_state++;
                }
                break;

            case APP_STATE_BOOTED:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_up();
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_control(BSP_HAPTIC_CONTROL_GET_HALO_HEARTBEAT, (void *) 0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_CLAB_ENABLED, (void *) 0x0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_TRIGGER_INDEX, (void *) 0x1);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO_ENABLE, (void *) 0x0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO1_BUTTON_DETECT, (void *) 0x0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO2_BUTTON_DETECT, (void *) 0x0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO3_BUTTON_DETECT, (void *) 0x0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO4_BUTTON_DETECT, (void *) 0x0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPI_GAIN_CONTROL, (void *) 0x0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_CTRL_PORT_GAIN_CONTROL, (void *) 0x0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_PRESS, (void *) 0x3);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_RELEASE, (void *) 0x4);
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP_NO_GPI:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_CLAB_ENABLED, (void *) 0x1);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_TIMEOUT_MS, (void *) 1000);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_GET_HALO_HEARTBEAT, (void *) 0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_TRIGGER_MS, (void *) 0x0);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO_ENABLE, (void *) 0x1);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO1_BUTTON_DETECT, (void *) 0x1);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO2_BUTTON_DETECT, (void *) 0x1);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO3_BUTTON_DETECT, (void *) 0x1);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO4_BUTTON_DETECT, (void *) 0x1);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_PRESS, (void *) 0x1);
                    bsp_dut_control(BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_RELEASE, (void *) 0x2);
                    app_state++;
                }
                break;

            case APP_STATE_DYNAMIC_F0:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
#ifdef DYNAMIC_F0_SUPPORTED
                    bsp_dut_dynamic_calibrate();
#endif
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP_GPI:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_start_i2s();
                    app_state++;
                }
                break;

            case APP_STATE_I2S_ENABLED:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    app_state++;
                }
                break;

            case APP_STATE_I2S_ENABLED_PLAY_TONE:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_stop_i2s();
                    app_state++;
                }
                break;

            case APP_STATE_I2S_DISABLED:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_audio_stop();
                    app_state++;
                }
                break;

            case APP_STATE_AUDIO_STOPPED:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_hibernate();
                    app_state++;
                }
                break;

            case APP_STATE_HIBERNATE:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_wake();
                    app_state++;
                }
                break;

            case APP_STATE_WAKE:
                if (flags & HAPTIC_CONTROL_FLAG_PB_PRESSED)
                {
                    bsp_dut_power_down();
                    app_state = APP_STATE_PDN;
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

    app_init();

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

    bsp_dut_reset();

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    for (;;);

    return ret_val;
}
