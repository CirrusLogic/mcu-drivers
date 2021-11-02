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
#include "platform_bsp.h"
#include <stddef.h>
#include <stdlib.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_UNINITIALIZED         (0)
#define APP_STATE_LID_CLOSED            (1)
#define APP_STATE_LID_OPEN              (2)
#define APP_STATE_FIRST_BUTTON_PRESS    (3)
#define APP_STATE_FINAL_BUTTON_PRESS    (4)

#define HAPTIC_BUTTON_PRESS_MAX         (1)

#define HAPTIC_BUTTON_PRESS_DURATION_MS (100)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_UNINITIALIZED;
static bool bsp_pb_pressed = false;
static uint8_t haptic_button_presses = 0;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
void app_bsp_callback(uint32_t status, void *arg)
{
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
    bsp_dut_enable_vamp(false);
    bsp_dut_reset();
    bsp_dut_trigger_gpio1(HAPTIC_BUTTON_PRESS_DURATION_MS);
    bsp_dut_power_down();
    bsp_dut_boot(false);
    //bsp_dut_update_haptic_config(1);
    bsp_dut_power_up();
    bsp_dut_process();
    bsp_dut_hibernate();

    bsp_set_ld2(BSP_LD2_MODE_ON, 0);

    app_state = APP_STATE_LID_CLOSED;

    while (1)
    {
        bsp_dut_process();

        if (bsp_was_pb_pressed(BSP_PB_ID_USER))
        {
            bsp_pb_pressed = true;
        }

        switch (app_state)
        {
            case APP_STATE_LID_CLOSED:
                if (bsp_pb_pressed)
                {
                    bsp_pb_pressed = false;
                    bsp_dut_wake();
                    bsp_set_ld2(BSP_LD2_MODE_BLINK, 2);
                    haptic_button_presses = 0;
                    app_state = APP_STATE_LID_OPEN;
                }
                break;

            case APP_STATE_LID_OPEN:
                if (bsp_pb_pressed)
                {
                    bsp_pb_pressed = false;
                    haptic_button_presses++;
                    bsp_dut_enable_vamp(true);
                    bsp_dut_trigger_gpio1(HAPTIC_BUTTON_PRESS_DURATION_MS);
                    bsp_dut_enable_vamp(false); // Disable VAMP and wait 5ms for it to fall

                    app_state = APP_STATE_FIRST_BUTTON_PRESS;
                }
                break;

            case APP_STATE_FIRST_BUTTON_PRESS:
                if (bsp_pb_pressed)
                {
                    bsp_pb_pressed = false;
                    haptic_button_presses++;
                    bsp_dut_enable_vamp(true);
                    bsp_dut_trigger_gpio1(HAPTIC_BUTTON_PRESS_DURATION_MS);
                    bsp_dut_enable_vamp(false); // Disable VAMP and wait 5ms for it to fall
                    bsp_dut_discharge_vamp();

                    app_state = APP_STATE_FINAL_BUTTON_PRESS;
                }
                break;

            case APP_STATE_FINAL_BUTTON_PRESS:
                if (bsp_pb_pressed)
                {
                    bsp_pb_pressed = false;
                    bsp_dut_hibernate();
                    bsp_set_ld2(BSP_LD2_MODE_ON, 0);
                    app_state = APP_STATE_LID_CLOSED;
                }
                break;

            default:
                break;
        }

        bsp_sleep();
    }

    exit(1);

    return ret_val;
}
