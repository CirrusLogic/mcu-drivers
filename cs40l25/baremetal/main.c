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
#define APP_STATE_BUZZ          (0)
#define APP_STATE_CALIBRATE     (1)
#define APP_STATE_CONFIG_0      (2)
#define APP_STATE_CONFIG_1      (3)
#define APP_STATE_DYNAMIC_F0    (4)
#define APP_STATE_START_I2S     (5)
#define APP_STATE_STOP_I2S      (6)
#define APP_STATE_WAKE          (7)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_BUZZ;
static bool bsp_pb_pressed = false;

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
#ifndef CONFIG_OPEN_LOOP
                    bsp_dut_trigger_haptic(BSP_DUT_TRIGGER_HAPTIC_POWER_ON, 0);
#endif
                    app_state++;
                }
                break;

            case APP_STATE_CALIBRATE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_down();
                    bsp_dut_boot(true);
                    bsp_dut_power_up();
                    bsp_dut_calibrate();
                    app_state++;
                }
                break;

            case APP_STATE_CONFIG_0:
                if (bsp_pb_pressed)
                {
                    bool has_processed;

                    bsp_dut_power_down();
                    bsp_dut_boot(false);
                    bsp_dut_update_haptic_config(0);
                    bsp_dut_enable_haptic_processing(false);
                    bsp_dut_power_up();
                    bsp_dut_has_processed(&has_processed);
                    bsp_dut_trigger_haptic(0x1, 0);
                    app_state++;
                }
                break;

            case APP_STATE_CONFIG_1:
                if (bsp_pb_pressed)
                {
                    bool has_processed;

                    bsp_dut_has_processed(&has_processed);
                    bsp_dut_update_haptic_config(1);
                    bsp_dut_enable_haptic_processing(true);
                    bsp_dut_trigger_haptic(0x0, 1000);
                    app_state++;
                }
                break;

            case APP_STATE_DYNAMIC_F0:
                if (bsp_pb_pressed)
                {
                    bsp_dut_dynamic_calibrate();
                    app_state++;
                }
                break;

            case APP_STATE_START_I2S:
                if (bsp_pb_pressed)
                {
#if CONFIG_8K_I2S
                    bsp_audio_set_fs(BSP_AUDIO_FS_8000_HZ);
#endif
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_dut_start_i2s();
                    app_state++;
                }
                break;

            case APP_STATE_STOP_I2S:
                if (bsp_pb_pressed)
                {
                    bsp_dut_stop_i2s();
                    bsp_audio_stop();
                    bsp_dut_hibernate();
                    app_state++;
                }
                break;

           case APP_STATE_WAKE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_wake();
                    bsp_dut_power_down();
                    app_state = APP_STATE_CALIBRATE;
                }
                break;

            default:
                break;
        }

        bsp_pb_pressed = false;

        bsp_sleep();
    }

    exit(1);

    return ret_val;
}
