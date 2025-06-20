/**
 * @file main.c
 *
 * @brief The main function for CS40L50 System Test Harness
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2025 All Rights Reserved, http://www.cirrus.com/
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
    bsp_dut_reset();

    bsp_set_ld2(BSP_LD2_MODE_ON, 0);
    bsp_dut_trigger_haptic(0, ROM_BANK);

    bsp_dut_configure_gpio_input(1);
    bsp_dut_boot();
    bsp_dut_disable_gpio_triggers();

    while (1)
    {

        if (bsp_was_pb_pressed(BSP_PB_ID_USER))
        {
            bsp_dut_trigger_haptic(0, ROM_BANK);
        }

        bsp_sleep();
    }

    exit(1);

    return ret_val;
}
