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
#include "system_test_hw_0_bsp.h"
#include <stddef.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
//#define INCLUDE_CALIBRATION

#define APP_STATE_UNINITIALIZED   (0)
#define APP_STATE_PUP             (1)
#define APP_STATE_BUZZ            (2)
#define APP_STATE_PDN             (3)
#define APP_STATE_BOOTED_CAL      (4)
#define APP_STATE_POWER_UP_CAL    (5)
#define APP_STATE_CAL_DONE        (6)
#define APP_STATE_PDN_2           (7)
#define APP_STATE_BOOTED          (8)
#define APP_STATE_POWER_UP        (9)
#define APP_STATE_POWER_UP_NO_GPI (10)
#define APP_STATE_POWER_UP_GPI    (11)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_UNINITIALIZED;
static bool app_bsp_cb_called = false;
static bool bsp_pb_pressed = false;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
void app_bsp_callback(uint32_t status, void *arg)
{
    app_bsp_cb_called = true;

    if (status != BSP_STATUS_OK)
    {
        exit(1);
    }

    return;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * @brief The Main Entry Point from __main
 *  By this time, the RAM RW-Data section has been initialized by the ARM-provided __main function.
 *
 * @return N/A (does not return)
 */
int main(void)
{
    int ret_val = 0;

    bsp_initialize(app_bsp_callback, NULL);
    bsp_haptic_initialize(BOOT_HAPTIC_TYPE_CAL | BOOT_HAPTIC_TYPE_WT | BOOT_HAPTIC_TYPE_CLAB);

    bsp_haptic_reset();
    app_state++;

    while (1)
    {
        bsp_haptic_process();
        if (bsp_was_pb_pressed(BSP_PB_ID_USER))
        {
            bsp_pb_pressed = true;
        }
        switch (app_state)
        {
            case APP_STATE_PUP:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_BHM_BUZZ_TRIGGER, 0x1);
                    app_state++;
                }
                break;

            case APP_STATE_BUZZ:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_power_down();
                    app_state++;
                }
                break;

            case APP_STATE_PDN:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_boot(true);
                    app_state++;
                }
                break;

            case APP_STATE_BOOTED_CAL:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_power_up();
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP_CAL:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_calibrate();
                    app_state++;
                }
                break;

            case APP_STATE_CAL_DONE:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_power_down();
                    app_state++;
                }
                break;

            case APP_STATE_PDN_2:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_boot(false);
                    app_state++;
                }
                break;

            case APP_STATE_BOOTED:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_power_up();
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_GET_HALO_HEARTBEAT, NULL);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_CLAB_ENABLED, 0x0);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_TRIGGER_INDEX, 0x1);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO_ENABLE, 0x0);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO1_BUTTON_DETECT, 0x0);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO2_BUTTON_DETECT, 0x0);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO3_BUTTON_DETECT, 0x0);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO4_BUTTON_DETECT, 0x0);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPI_GAIN_CONTROL, 0x0);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_CTRL_PORT_GAIN_CONTROL, 0x0);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_PRESS, 0x3);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_RELEASE, 0x4);
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP_NO_GPI:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_CLAB_ENABLED, 0x1);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_TIMEOUT_MS, 1000);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_GET_HALO_HEARTBEAT, NULL);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_TRIGGER_MS, 0x0);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO_ENABLE, 0x1);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO1_BUTTON_DETECT, 0x1);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO2_BUTTON_DETECT, 0x1);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO3_BUTTON_DETECT, 0x1);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO4_BUTTON_DETECT, 0x1);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_PRESS, 0x1);
                    bsp_haptic_control(BSP_HAPTIC_CONTROL_SET_GPIO1_INDEX_BUTTON_RELEASE, 0x2);
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP_GPI:
                if (bsp_pb_pressed)
                {
                    bsp_haptic_power_down();
                    app_state = APP_STATE_PDN;
                }
                break;

            default:
                break;
        }

        app_bsp_cb_called = false;
        bsp_pb_pressed = false;

        bsp_sleep();
    }

    exit(1);

    return ret_val;
}
