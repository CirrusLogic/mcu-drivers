/**
 * @file main.c
 *
 * @brief The main function for CS40L25 System Test Harness
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2023 All Rights Reserved, http://www.cirrus.com/
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
#include "cs40l26_ext.h"
#include <stddef.h>
#include <stdlib.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_BUZZ           (0)
#define APP_STATE_OWT_TRIGGER    (1)
#define APP_STATE_START_STOP_I2S (2)
#define APP_STATE_CALIBRATE      (3)
#define APP_STATE_DYNAMIC_F0     (4)


/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_state = APP_STATE_BUZZ;
static bool bsp_pb_pressed = false;

uint32_t pwle1[] =
{
    0x0000000C,
    0x00000003,
    0x0000000B,
    0x00800328,
    0x00006400,
    0x00400000,
    0x00004B00,
    0x00000014,
    0x00004B00,
    0x00001904,
    0x00004B00,
    0x00000030,
    0x00004B00,
    0x00000000
};

uint8_t pwle1_size = 14;

uint32_t pcm1[] =
{
    0x00000008,
    0x00000003,
    0x00000045,
    0x008000C9,
    0x00000000,
    0x00000A13,
    0x001D252D,
    0x0033393C,
    0x003F403F,
    0x003C3933,
    0x002D251D,
    0x00130A00,
    0x00F5ECE2,
    0x00DAD2CC,
    0x00C6C3C0,
    0x00C0C0C3,
    0x00C6CCD2,
    0x00DAE2EC,
    0x00F50009,
    0x00131D25,
    0x002D3339,
    0x003C3F3F,
    0x003F3C39,
    0x00332D25,
    0x001D130A,
    0x00FFF6EC,
    0x00E2DAD2,
    0x00CCC6C3,
    0x00C0BFC0,
    0x00C3C6CC,
    0x00D2DAE2,
    0x00ECF500,
    0x000A131D,
    0x00252D33,
    0x00393C3F,
    0x00403F3C,
    0x0039332D,
    0x00251D13,
    0x000A00F5,
    0x00ECE2DA,
    0x00D2CCC6,
    0x00C3C0C0,
    0x00C0C3C6,
    0x00CCD2DA,
    0x00E2ECF6,
    0x00FF0A13,
    0x001D252D,
    0x0033393C,
    0x003F3F3F,
    0x003C3933,
    0x002D251D,
    0x00130AFF,
    0x00F5ECE2,
    0x00DAD2CC,
    0x00C6C3C0,
    0x00C0C0C3,
    0x00C6CCD2,
    0x00DAE2EC,
    0x00F5000A,
    0x00131D25,
    0x002D3339,
    0x003C3F40,
    0x003F3C39,
    0x00332D25,
    0x001D130A,
    0x0000F5EC,
    0x00E2DAD2,
    0x00CCC6C3,
    0x00C0C0C0,
    0x00C3C6CC,
    0x00D2DAE2,
    0x00ECF500
};

uint8_t pcm1_size = 72;

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
    bsp_dut_wake();

    bsp_set_ld2(BSP_LD2_MODE_ON, 0);

    bsp_dut_trigger_haptic(0, BUZZ_BANK);
    bsp_set_timer(100, NULL, NULL);
    bsp_dut_trigger_haptic(3, ROM_BANK);

    bsp_dut_boot(false);
    bsp_dut_wake();
    bsp_dut_hibernate();

    while (1)
    {
        bsp_dut_wake();
        bsp_dut_process();
        bsp_dut_hibernate();

        if (bsp_was_pb_pressed(BSP_PB_ID_USER))
        {
            bsp_pb_pressed = true;
        }

        switch (app_state)
        {
            case APP_STATE_BUZZ:
                if (bsp_pb_pressed)
                {
                    bsp_dut_reset();
                    bsp_dut_boot(false);
                    bsp_dut_load_wavetable();
                    bsp_dut_configure_gpi(2);
                    bsp_dut_configure_gpi_mute(2, 1);
                    bsp_dut_enable_gpi_mute(1);
                    bsp_dut_buzzgen_set(0x100, 0x32, 200, 1);
                    bsp_dut_trigger_haptic(1, BUZZ_BANK);
                    do
                    {
                        bsp_dut_process();
                    } while (bsp_processing_haptic);
                    bsp_dut_buzzgen_set(0x100, 0x32, 20, 2);
                    bsp_dut_trigger_haptic(2, BUZZ_BANK);
                    do
                    {
                        bsp_dut_process();
                    } while (bsp_processing_haptic);
                    bsp_dut_trigger_haptic(3, RAM_BANK);
                    do
                    {
                        bsp_dut_process();
                    } while (bsp_processing_haptic);
                    bsp_dut_enable_gpi_mute(0);
                    bsp_dut_hibernate();
                    app_state++;
                }
                break;
            case APP_STATE_OWT_TRIGGER:
                if (bsp_pb_pressed)
                {
                    bsp_dut_wake();
                    bsp_dut_reset();
                    bsp_dut_boot(false);
                    bsp_dut_load_wavetable();
                    bsp_dut_owt_reset_table();
                    bsp_set_timer(1000, NULL, NULL);
                    bsp_dut_owt_upload_effect(pcm1, pcm1_size);
                    bsp_dut_owt_upload_effect(pwle1, pwle1_size);
                    bsp_dut_trigger_haptic(0, OWT_BANK);
                    bsp_set_timer(300, NULL, NULL);
                    bsp_dut_trigger_haptic(1, OWT_BANK);
                    app_state++;
                }
            case APP_STATE_START_STOP_I2S:
                if (bsp_pb_pressed)
                {
                    bsp_dut_wake();
                    bsp_dut_reset();
                    bsp_dut_boot(false);
                    bsp_dut_wake();
                    bsp_dut_load_wavetable();
                    bsp_dut_trigger_haptic(3, RAM_BANK);
                    bsp_audio_play_record(BSP_I2S_PORT_PRIMARY, BSP_PLAY_STEREO_100HZ_20DBFS);
                    bsp_dut_start_i2s();
                    bsp_set_timer(300, NULL, NULL);
                    bsp_dut_stop_i2s();
                    bsp_audio_stop(BSP_I2S_PORT_PRIMARY);
                    bsp_dut_hibernate();
                    app_state++;
                }
                break;
            case APP_STATE_CALIBRATE:
                if (bsp_pb_pressed)
                {
                    bsp_dut_reset();
                    bsp_dut_boot(true);
                    bsp_dut_calibrate();
                    bsp_dut_hibernate();
                    app_state++;
                }
                break;
            case APP_STATE_DYNAMIC_F0:
                if (bsp_pb_pressed)
                {
                    bsp_dut_wake();
                    bsp_dut_reset();
                    bsp_dut_boot(true);
                    bsp_dut_wake();
                    bsp_dut_load_wavetable();
                    bsp_dut_dynamic_calibrate(3);
                    bsp_dut_hibernate();
                    app_state = APP_STATE_BUZZ;
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
