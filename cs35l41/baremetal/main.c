/**
 * @file main.c
 *
 * @brief The main function for CS35L41 System Test Harness
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
#define APP_STATE_CAL_PDN               (0)
#define APP_STATE_PDN                   (1)
#define APP_STATE_CHANGE_44P1KHZ        (2)
#define APP_STATE_CHANGE_48KHZ          (3)
#define APP_STATE_PUP                   (4)
#define APP_STATE_MUTE                  (5)
#define APP_STATE_HIBERNATE             (6)
#define APP_STATE_WAKE                  (7)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_audio_state = APP_STATE_CAL_PDN;
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

    bsp_set_ld2(BSP_LD2_MODE_ON, 0);

    while (1)
    {
        bsp_dut_process();
        if (bsp_was_pb_pressed(BSP_PB_ID_USER))
        {
            bsp_pb_pressed = true;
        }

        switch (app_audio_state)
        {
            case APP_STATE_CAL_PDN:
                if (bsp_pb_pressed)
                {
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_play_record(BSP_PLAY_SILENCE);
                    bsp_dut_reset();
                    bsp_dut_boot(true);
                    bsp_dut_power_up();
                    bsp_dut_calibrate();
                    bsp_dut_power_down();
                    app_audio_state = APP_STATE_PDN;
                }
                break;

            case APP_STATE_PDN:
                if (bsp_pb_pressed)
                {
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_dut_reset();
                    bsp_dut_boot(false);
                    uint8_t dut_id;
                    bsp_dut_get_id(&dut_id);
                    if (dut_id == BSP_DUT_ID_LEFT)
                    {
                        bsp_dut_set_dig_gain(-6);
                    }
                    else
                    {
                        bsp_dut_set_dig_gain(-10);
                    }
                    bsp_dut_power_up();
                    app_audio_state = APP_STATE_CHANGE_44P1KHZ;
                }
                break;

            case APP_STATE_CHANGE_44P1KHZ:
                if (bsp_pb_pressed)
                {
                    bool is_processing = false;
                    bsp_dut_is_processing(&is_processing);

                    bsp_dut_change_fs(BSP_AUDIO_FS_44100_HZ);
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_44100_HZ);
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);

                    bsp_dut_is_processing(&is_processing);

                    if (is_processing)
                    {
                        app_audio_state = APP_STATE_CHANGE_48KHZ;
                    }
                }
                break;

            case APP_STATE_CHANGE_48KHZ:
                if (bsp_pb_pressed)
                {
                    bool is_processing = false;
                    bsp_dut_is_processing(&is_processing);

                    bsp_dut_change_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_stop();
                    bsp_audio_set_fs(BSP_AUDIO_FS_48000_HZ);
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);

                    bsp_dut_is_processing(&is_processing);

                    if (is_processing)
                    {
                        app_audio_state = APP_STATE_PUP;
                    }
                }
                break;

            case APP_STATE_PUP:
                if (bsp_pb_pressed)
                {
                    bsp_dut_mute(true);
                    app_audio_state = APP_STATE_MUTE;
                }
                break;

            case APP_STATE_MUTE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_down();
                    app_audio_state = APP_STATE_HIBERNATE;
                }
                break;

            case APP_STATE_HIBERNATE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_hibernate();
                    app_audio_state = APP_STATE_WAKE;
                }
                break;

            case APP_STATE_WAKE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_wake();
                    app_audio_state = APP_STATE_PDN;
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
