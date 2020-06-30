/**
 * @file main.c
 *
 * @brief The main function for CS35L41 System Test Harness
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
#include "hw_0_bsp.h"
#include <stddef.h>
#include <stdlib.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define APP_STATE_CAL_PDN               (0)
#define APP_STATE_CAL_BOOTED            (1)
#define APP_STATE_CAL_PUP               (2)
#define APP_STATE_CALIBRATED            (3)
#define APP_STATE_PDN                   (4)
#define APP_STATE_BOOTED                (5)
#define APP_STATE_PUP                   (6)
#define APP_STATE_MUTE                  (7)
#define APP_STATE_UNMUTE                (8)
#define APP_STATE_HIBERNATE             (9)
#define APP_STATE_WAKE                  (10)
#define APP_STATE_CHECK_PROCESSING      (11)

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
    bsp_dut_initialize();

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
                    bsp_audio_play_record(BSP_PLAY_SILENCE);
                    bsp_dut_reset();
                    bsp_dut_boot(true);
                    app_audio_state = APP_STATE_CAL_BOOTED;
                }
                break;

            case APP_STATE_CAL_BOOTED:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_up();
                    app_audio_state = APP_STATE_CAL_PUP;
                }
                break;

            case APP_STATE_CAL_PUP:
                if (bsp_pb_pressed)
                {
                    bsp_dut_calibrate();
                    app_audio_state = APP_STATE_CALIBRATED;
                }
                break;

            case APP_STATE_CALIBRATED:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_down();
                    app_audio_state = APP_STATE_PDN;
                }
                break;

            case APP_STATE_PDN:
                if (bsp_pb_pressed)
                {
                    bsp_audio_stop();
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_dut_reset();
                    bsp_dut_boot(false);
                    app_audio_state = APP_STATE_BOOTED;
                }
                break;

            case APP_STATE_BOOTED:
                if (bsp_pb_pressed)
                {
                    bsp_dut_power_up();
                    app_audio_state = APP_STATE_CHECK_PROCESSING;
                }
                break;

            case APP_STATE_CHECK_PROCESSING:
                if (bsp_pb_pressed)
                {
                    bool is_processing = false;
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
                    bsp_dut_mute(false);
                    app_audio_state = APP_STATE_UNMUTE;
                }
                break;

            case APP_STATE_UNMUTE:
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
                    app_audio_state = APP_STATE_CAL_PDN;
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
