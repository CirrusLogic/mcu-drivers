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
#include "platform_bsp.h"
#include <stddef.h>
#include <stdlib.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

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

void app_process_initial_switch_state(void)
{
    uint8_t temp_state = 0;

    bsp_get_switch_state_changes(&temp_state, NULL);

    bsp_dut_power_down();   // Exit BHM
    if (temp_state & 0x8)
    {
        // Boot calibration firmware
        bsp_dut_boot(true);
        bsp_dut_power_up();
        bsp_dut_calibrate();
        bsp_set_led(0, BSP_LD2_MODE_BLINK, 5);
    }
    else
    {
        // Boot run-time firmware
        bsp_dut_boot(false);
        bsp_dut_power_up();
        bsp_set_led(0, BSP_LD2_MODE_ON, 0);
    }

    return;
}

void app_process_switches(void)
{
    uint8_t temp_state = 0;
    uint8_t temp_mask = 0;

    bsp_get_switch_state_changes(&temp_state, &temp_mask);

    if (temp_mask & 0x8)
    {
        if (temp_state & 0x8)
        {
            // Boot calibration firmware
            bsp_set_led(0, BSP_LD2_MODE_BLINK, 1);
            bsp_dut_power_down();
            bsp_dut_boot(true);
            bsp_dut_power_up();
            bsp_dut_calibrate();
            bsp_set_led(0, BSP_LD2_MODE_BLINK, 5);
        }
        else
        {
            // Boot run-time firmware
            bsp_set_led(0, BSP_LD2_MODE_BLINK, 1);
            bsp_dut_power_down();
            bsp_dut_boot(false);
            bsp_dut_power_up();
            bsp_set_led(0, BSP_LD2_MODE_ON, 0);
        }
    }
    else if (temp_mask & 0x7)
    {
        bsp_dut_wake();
        bsp_dut_trigger_haptic((temp_state & 0x7), 0);
        bsp_dut_hibernate();
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
    bsp_dut_trigger_haptic(BSP_DUT_TRIGGER_HAPTIC_POWER_ON, 0);

    app_process_initial_switch_state();

    while (1)
    {
        bsp_dut_process();
        app_process_switches();
        bsp_sleep();
    }

    exit(1);

    return ret_val;
}
