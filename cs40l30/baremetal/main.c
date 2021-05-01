/**
 * @file main.c
 *
 * @brief The main function for CS40L25 System Test Harness
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
#include "hw_0_bsp.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_BUZZ                  (0)
#define APP_STATE_TRIGGER_ROM_EFFECT    (1)
#define APP_STATE_TRIGGER_RAM_EFFECT    (2)
#define APP_STATE_TRIGGER_OTP_EFFECT    (3)
#define APP_STATE_TRIGGER_LONG_BUZZ     (4)
#define APP_STATE_STOP_LONG_BUZZ        (5)
#define APP_STATE_HIBERNATE_PREVENTED   (6)
#define APP_STATE_HIBERNATE_ALLOWED     (7)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_TRIGGER_ROM_EFFECT;
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

    if (status == BSP_STATUS_FAIL)
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
    bsp_dut_initialize();
    bsp_dut_reset();

    bsp_set_ld2(BSP_LD2_MODE_ON, 0);

    while (1)
    {
        bsp_dut_process();

        if (bsp_was_pb_pressed(BSP_PB_ID_USER))
        {
            bsp_pb_pressed = true;
        }

        switch (app_state)
        {
            case APP_STATE_BUZZ:
                if (bsp_pb_pressed)
                {
                    // Control for triggering boot buzz
                    app_state++;
                }
                break;

            case APP_STATE_TRIGGER_ROM_EFFECT:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_down();
                    bsp_dut_boot(false);
                    bsp_dut_power_up();
                    bsp_dut_haptic_trigger(1);
                    app_state++;
                }
                break;

            case APP_STATE_TRIGGER_RAM_EFFECT:
                if (bsp_pb_pressed)
                {
                    bsp_dut_haptic_trigger(2);
                    app_state++;
                }
                break;

            case APP_STATE_TRIGGER_OTP_EFFECT:
                if (bsp_pb_pressed)
                {
                    bsp_dut_haptic_trigger(3);
                    app_state++;
                }
                break;

            case APP_STATE_TRIGGER_LONG_BUZZ:
                if (bsp_pb_pressed)
                {
                    bsp_dut_update_haptic_config(1);
                    bsp_dut_haptic_trigger(3);
                    app_state++;
                }
                break;

            case APP_STATE_STOP_LONG_BUZZ:
                if (bsp_pb_pressed)
                {
                    bsp_dut_haptic_trigger(BSP_TRIGGER_INDEX_STOP);
                    app_state++;
                }
                break;

            case APP_STATE_HIBERNATE_PREVENTED:
                if (bsp_pb_pressed)
                {
                    bsp_dut_prevent_hibernate();
                    app_state++;
                }
                break;

           case APP_STATE_HIBERNATE_ALLOWED:
                if (bsp_pb_pressed)
                {
                    bsp_dut_allow_hibernate();
                    app_state = APP_STATE_TRIGGER_ROM_EFFECT;
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
