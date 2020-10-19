/**
 * @file main.c
 *
 * @brief The main function for CS40L25 System Test Harness
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
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
#include "hw_0_bsp.h"
#include <stddef.h>
#include <stdlib.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_UNINITIALIZED         (0)
#define APP_STATE_PUP                   (1)
#define APP_STATE_BUZZ                  (2)
#define APP_STATE_PDN                   (3)
#define APP_STATE_BOOTED_CAL            (4)
#define APP_STATE_POWER_UP_CAL          (5)
#define APP_STATE_CAL_DONE              (6)
#define APP_STATE_PDN_2                 (7)
#define APP_STATE_BOOTED                (8)
#define APP_STATE_POWER_UP              (9)
#define APP_STATE_POWER_UP_NO_GPI       (10)
#define APP_STATE_DYNAMIC_F0            (11)
#define APP_STATE_POWER_UP_GPI          (12)
#define APP_STATE_PLAY_TONE             (13)
#define APP_STATE_PLAY_TONE_I2S_ENABLED (14)
#define APP_STATE_I2S_DISABLED          (15)
#define APP_STATE_AUDIO_STOPPED         (16)
#define APP_STATE_HIBERNATE             (17)
#define APP_STATE_WAKE                  (18)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_UNINITIALIZED;
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
    app_state++;

    while (1)
    {
        bsp_dut_process();
        if (bsp_was_pb_pressed(BSP_PB_ID_USER))
        {
            bsp_pb_pressed = true;
        }
        switch (app_state)
        {
            case APP_STATE_PUP:
                if (bsp_pb_pressed)
                {
#ifndef CONFIG_TEST_OPEN_LOOP
                    bsp_dut_trigger_haptic(BSP_DUT_TRIGGER_HAPTIC_POWER_ON, 0);
#endif
                    app_state++;
                }
                break;

            case APP_STATE_BUZZ:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_down();
                    app_state++;
                }
                break;

            case APP_STATE_PDN:
                if (bsp_pb_pressed)
                {
                    bsp_dut_boot(true);
                    app_state++;
                }
                break;

            case APP_STATE_BOOTED_CAL:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_up();
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP_CAL:
                if (bsp_pb_pressed)
                {
                    bsp_dut_calibrate();
                    app_state++;
                }
                break;

            case APP_STATE_CAL_DONE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_down();
                    app_state++;
                }
                break;

            case APP_STATE_PDN_2:
                if (bsp_pb_pressed)
                {
                    bsp_dut_boot(false);
                    bsp_dut_update_haptic_config(0);
                    bsp_dut_enable_haptic_processing(false);
                    app_state++;
                }
                break;

            case APP_STATE_BOOTED:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_up();
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP:
                if (bsp_pb_pressed)
                {
                    bool has_processed;
                    bsp_dut_has_processed(&has_processed);
                    bsp_dut_trigger_haptic(0x1, 0);
                    app_state++;
                }
                break;

            case APP_STATE_POWER_UP_NO_GPI:
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

            case APP_STATE_POWER_UP_GPI:
                if (bsp_pb_pressed)
                {
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    app_state++;
                }
                break;
            case APP_STATE_PLAY_TONE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_start_i2s();
                    app_state++;
                }
                break;
            case APP_STATE_PLAY_TONE_I2S_ENABLED:
                if (bsp_pb_pressed)
                {
                    bsp_dut_stop_i2s();
                    app_state++;
                }
                break;
            case APP_STATE_I2S_DISABLED:
                if (bsp_pb_pressed)
                {
                    bsp_audio_stop();
                    app_state++;
                }
                break;
            case APP_STATE_AUDIO_STOPPED:
                if (bsp_pb_pressed)
                {
                    bsp_dut_hibernate();
                    app_state++;
                }
                break;

           case APP_STATE_HIBERNATE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_wake();
                    app_state++;
                }
                break;

          case APP_STATE_WAKE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_down();
                    app_state = APP_STATE_PDN;
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
