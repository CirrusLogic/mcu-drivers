/**
 * @file main.c
 *
 * @brief The main function for CS40L50 System Test Harness
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022 All Rights Reserved, http://www.cirrus.com/
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
#include "rth_types.h"
#include "waveforms.h"
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
uint8_t app_state = 0;

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

void app_set_sel_leds(uint8_t state)
{
    bsp_set_led(2, BSP_LD2_MODE_OFF, 0);
    bsp_set_led(3, BSP_LD2_MODE_OFF, 0);
    bsp_set_led(4, BSP_LD2_MODE_OFF, 0);

    switch (state)
    {
    case 0:
        break;
    case 1:
        bsp_set_led(2, BSP_LD2_MODE_ON, 0);
        break;
    case 2:
        bsp_set_led(3, BSP_LD2_MODE_ON, 0);
        break;
    case 3:
        bsp_set_led(2, BSP_LD2_MODE_ON, 0);
        bsp_set_led(3, BSP_LD2_MODE_ON, 0);
        break;
    case 4:
        bsp_set_led(4, BSP_LD2_MODE_ON, 0);
        break;
    case 5:
        bsp_set_led(4, BSP_LD2_MODE_ON, 0);
        bsp_set_led(2, BSP_LD2_MODE_ON, 0);
        break;
    case 6:
        bsp_set_led(4, BSP_LD2_MODE_ON, 0);
        bsp_set_led(3, BSP_LD2_MODE_ON, 0);
        break;
    case 7:
        bsp_set_led(4, BSP_LD2_MODE_ON, 0);
        bsp_set_led(3, BSP_LD2_MODE_ON, 0);
        bsp_set_led(2, BSP_LD2_MODE_ON, 0);
        break;
    default:
        break;
    };
    return;
}

void app_init(void)
{
    bsp_initialize(app_bsp_callback, NULL);
    app_set_sel_leds(app_state);
    bsp_set_led(1, BSP_LD2_MODE_OFF, 0);
    bsp_dut_initialize();
    bsp_dut_reset();
    bsp_dut_calibrate();
    bsp_dut_trigger_haptic(23, ROM_BANK);
    bsp_driver_if_g->set_timer(7, NULL, NULL);
    bsp_dut_set_click_compensation(true, true);

    return;
}

void app_process_pb(void)
{
    if (bsp_was_pb_pressed(0))
    {
        bsp_dut_wake();
        switch (app_state)
        {
        case 0:
          /**
           * Trapezoidal PWLE click waveform - Full cycle
           * Ramp up - Sine Chirp, 50Hz to 330Hz, 0FS to 0.33FS in 0.50ms
           * Base - Sine, 330Hz, 0.33FS, 2.50ms
           * Ramp down - Sine Chirp, 330Hz to 50Hz, 0.33FS to 0FS in 0.50ms
           *
           */
            bsp_dut_trigger_rth_pwle(false, pwle1, pwle_1_size, 0);
            break;
        case 1:
          /**
           * Trapezoidal PWLE click waveform - Half cycle
           * Ramp up - Sine Chirp, 50Hz to 100Hz, 0FS to 0.41FS in 0.75ms
           * Base - Sine, 100Hz, 0.41FS, 4.00ms
           * Ramp down - Sine Chirp, 100Hz to 50Hz, 0.41FS to 0FS in 0.75ms
           *
           */
            bsp_dut_trigger_rth_pwle(false, pwle2, pwle_2_size, 0);
            break;
        case 2:
          /**
           * Long PWLE buzz waveform
           * Sine, 125ms, 180Hz, 0.13FS to 0.29FS, 168 half cycles, 265Hz, 0.29FS to 0.42FS
           *
           */
            bsp_dut_trigger_rth_pwle(false, pwle3, pwle_3_size, 0);
            break;
        case 3:
          /**
           * Short PCM click waveform
           * Sine, 1cycle, 400Hz, .46FS
           *
           */
            bsp_dut_trigger_rth_pcm(pcm_1_data, pcm_1_data_size, pcm_1_data_size, 0, 0);
            break;
        case 4:
          /**
           * Short PCM click waveform with click compensation
           * Sine, 1cycle, 240Hz, 0.34FS
           *
           */
            bsp_dut_trigger_rth_pcm(pcm_1_data, pcm_1_data_size, pcm_1_data_size, pcm_1_f0, pcm_1_redc);
            break;
        case 5:
          /**
           * Long PCM buzz waveform
           * Sine, 3cycles, 220Hz, 0.49FS, 1.5cycles, 100Hz, 0.16FS
           *
           */
            bsp_dut_trigger_rth_pcm(pcm_2_data, pcm_2_data_size, 114, 0, 0);
            break;
        }

        app_set_sel_leds((app_state+1)%7);
        app_state++;
        app_state %= 6;
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
    app_init();

    while (1)
    {
        app_process_pb();
        bsp_sleep();
    }

    exit(1);

    return 0;
}
