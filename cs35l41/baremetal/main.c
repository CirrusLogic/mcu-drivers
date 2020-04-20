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
#include "system_test_hw_0_bsp.h"
#include <stddef.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
//#define INCLUDE_CALIBRATION

#define APP_AUDIO_STATE_CALIBRATING     (0)
#define APP_AUDIO_STATE_BOOTING         (1)
#define APP_AUDIO_STATE_PDN             (2)
#define APP_AUDIO_STATE_PUP             (3)
#define APP_AUDIO_STATE_MUTE            (4)
#define APP_AUDIO_STATE_UNMUTE          (5)
/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static uint8_t app_audio_state = APP_AUDIO_STATE_PDN;
static bool app_bsp_cb_called = false;
static bool bsp_pb_pressed = false;
static bool is_calibrated = false;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
void app_bsp_callback(uint32_t status, void *arg)
{
    app_bsp_cb_called = true;

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
    bsp_amp_initialize();

#ifdef INCLUDE_CALIBRATION
    bsp_audio_play_record(BSP_PLAY_SILENCE);
    bsp_amp_boot(BOOT_AMP_TYPE_CALIBRATION_TUNE);
    is_calibrated = false;
#else
    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
    bsp_amp_boot(BOOT_AMP_TYPE_NORMAL_TUNE);
    is_calibrated = true;
#endif
    app_audio_state = APP_AUDIO_STATE_PDN;

    while (1)
    {
        bsp_amp_process();
        if (bsp_was_pb_pressed(BSP_PB_ID_USER))
        {
            bsp_pb_pressed = true;
        }

        switch (app_audio_state)
        {
            case APP_AUDIO_STATE_CALIBRATING:
                if (app_bsp_cb_called)
                {
                    is_calibrated = true;
                    bsp_audio_stop();
                    bsp_audio_play_record(BSP_PLAY_STEREO_1KHZ_20DBFS);
                    bsp_amp_boot(BOOT_AMP_TYPE_NORMAL_TUNE);
                    app_audio_state = APP_AUDIO_STATE_BOOTING;
                }
                break;

            case APP_AUDIO_STATE_BOOTING:
                if (app_bsp_cb_called)
                {
                    app_audio_state = APP_AUDIO_STATE_PDN;
                }
                break;

            case APP_AUDIO_STATE_PDN:
                if (bsp_pb_pressed)
                {
                    bsp_amp_power_up();
                    app_audio_state = APP_AUDIO_STATE_PUP;
                }
                break;

            case APP_AUDIO_STATE_PUP:
                if ((bsp_pb_pressed) && (is_calibrated))
                {
                    bsp_amp_mute(true);
                    app_audio_state = APP_AUDIO_STATE_MUTE;
                }
                else if ((app_bsp_cb_called) && (!is_calibrated))
                {
                    bsp_amp_calibrate();
                    app_audio_state = APP_AUDIO_STATE_CALIBRATING;
                }
                break;

            case APP_AUDIO_STATE_MUTE:
                if (bsp_pb_pressed)
                {
                    bsp_amp_mute(false);
                    app_audio_state = APP_AUDIO_STATE_UNMUTE;
                }
                break;

            case APP_AUDIO_STATE_UNMUTE:
                if (bsp_pb_pressed)
                {
                    bsp_amp_power_down();
                    app_audio_state = APP_AUDIO_STATE_PDN;
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
