/**
 * @file main.c
 *
 * @brief The main function for CS40L25 System Test Harness
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
#include <stddef.h>
#include <stdlib.h>

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

static rth_pwle_section_t pwle_trapezoid_hc_section0 =
{
    .time = 0,
    .level = 0,
    .freq = 8
};

static rth_pwle_section_t pwle_trapezoid_hc_section1 =
{
    .time = 0,
    .level = 0,
    .freq = 8
};

static rth_pwle_section_t pwle_trapezoid_hc_section2 =
{
    .time = 8,
    .level = 1458,
    .freq = 8
};

static rth_pwle_section_t pwle_trapezoid_hc_section3 =
{
    .time = 12,
    .level = 1458,
    .freq = 400
};

static rth_pwle_section_t pwle_trapezoid_hc_section4 =
{
    .time = 8,
    .level = 0,
    .freq = 8
};

static rth_pwle_section_t pwle_trapezoid_section0 =
{
    .time = 0,
    .level = 0,
    .freq = 0x0C8
};

static rth_pwle_section_t pwle_trapezoid_section1 =
{
    .time = 0,
    .level = 0,
    .freq = 0x0C8
};

static rth_pwle_section_t pwle_trapezoid_section2 =
{
    .time = 0x3,
    .level = 0x59A,
    .freq = 0x528
};

static rth_pwle_section_t pwle_trapezoid_section3 =
{
    .time = 0x9,
    .level = 0x59A,
    .freq = 0x528
};

static rth_pwle_section_t pwle_trapezoid_section4 =
{
    .time = 0x2,
    .level = 0x0,
    .freq = 0x0C8
};

static rth_pwle_section_t pwle_long_265hz_section0 =
{
    .time = 0,
    .level = 0x1A0,
    .freq = 0x410
};

static rth_pwle_section_t pwle_long_265hz_section1 =
{
    .time = 0,
    .level = 0x1A0,
    .freq = 0x410
};

static rth_pwle_section_t pwle_long_265hz_section2 =
{
    .time = 0x1F4,
    .level = 0x3A9,
    .freq = 0x410
};

static rth_pwle_section_t pwle_long_265hz_section3 =
{
    .time = 0x4F8,
    .level = 0x54A,
    .freq = 0x6B8
};

static uint8_t pcm_data_fs_400hz[21] =
{
        0,
        39,
        75,
        103,
        121,
        127,
        121,
        103,
        75,
        39,
        0,
        216,
        180,
        152,
        134,
        128,
        134,
        152,
        180,
        216,
        0
};

static uint8_t pcm_data_220hz_long[231] =
{
  0,
  16,
  32,
  47,
  61,
  73,
  82,
  89,
  94,
  95,
  94,
  90,
  84,
  74,
  63,
  50,
  35,
  19,
  3,
  242,
  226,
  211,
  197,
  184,
  174,
  167,
  162,
  160,
  160,
  164,
  170,
  179,
  190,
  203,
  217,
  233,
  249,
  10,
  26,
  42,
  56,
  68,
  79,
  87,
  92,
  95,
  95,
  92,
  86,
  78,
  67,
  55,
  40,
  25,
  9,
  248,
  232,
  216,
  202,
  189,
  178,
  169,
  163,
  160,
  160,
  162,
  167,
  175,
  186,
  198,
  212,
  227,
  243,
  4,
  20,
  36,
  51,
  64,
  75,
  84,
  91,
  95,
  95,
  94,
  89,
  81,
  72,
  60,
  46,
  31,
  15,
  254,
  238,
  222,
  207,
  193,
  182,
  172,
  165,
  161,
  160,
  161,
  165,
  172,
  182,
  193,
  207,
  222,
  238,
  0,
  2,
  5,
  7,
  9,
  12,
  14,
  16,
  18,
  20,
  22,
  24,
  25,
  27,
  28,
  29,
  30,
  31,
  31,
  31,
  32,
  31,
  31,
  31,
  30,
  29,
  28,
  27,
  25,
  24,
  22,
  20,
  18,
  16,
  14,
  12,
  9,
  7,
  5,
  2,
  0,
  253,
  250,
  248,
  246,
  243,
  241,
  239,
  237,
  235,
  233,
  231,
  230,
  228,
  227,
  226,
  225,
  224,
  224,
  224,
  224,
  224,
  224,
  224,
  225,
  226,
  227,
  228,
  230,
  231,
  233,
  235,
  237,
  239,
  241,
  243,
  246,
  248,
  250,
  253,
  0,
  2,
  5,
  7,
  9,
  12,
  14,
  16,
  18,
  20,
  22,
  24,
  25,
  27,
  28,
  29,
  30,
  31,
  31,
  31,
  32,
  31,
  31,
  31,
  30,
  29,
  28,
  27,
  25,
  24,
  22,
  20,
  18,
  16,
  14,
  12,
  9,
  7,
  5,
  2,
  0,
  0
};

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

    bsp_set_led((state + 2), BSP_LD2_MODE_ON, 0);
    return;
}

void app_init(void)
{
    bsp_initialize(app_bsp_callback, NULL);
    app_set_sel_leds(app_state);
    bsp_dut_initialize();
    bsp_dut_reset();
    bsp_dut_trigger_haptic(BSP_DUT_TRIGGER_HAPTIC_POWER_ON, 0);

    // Boot run-time firmware
    bsp_dut_boot(false);

    return;
}

void app_process_pb(void)
{
    if (bsp_was_pb_pressed(0))
    {

          rth_pwle_section_t *pwle_trapezoid_hc[5];
          rth_pwle_section_t *pwle_trapezoid[5];
          rth_pwle_section_t *pwle_long[4];
        switch (app_state)
        {
        case 0:
            pwle_trapezoid[0] = &pwle_trapezoid_section0;
            pwle_trapezoid[1] = &pwle_trapezoid_section1;
            pwle_trapezoid[2] = &pwle_trapezoid_section2;
            pwle_trapezoid[3] = &pwle_trapezoid_section3;
            pwle_trapezoid[4] = &pwle_trapezoid_section4;
            bsp_dut_trigger_rth_pwle(false, pwle_trapezoid, 4, 0);
            bsp_set_led(1, (app_state/3), 0);
            break;
        case 1:
            pwle_trapezoid_hc[0] = &pwle_trapezoid_hc_section0;
            pwle_trapezoid_hc[1] = &pwle_trapezoid_hc_section1;
            pwle_trapezoid_hc[2] = &pwle_trapezoid_hc_section2;
            pwle_trapezoid_hc[3] = &pwle_trapezoid_hc_section3;
            pwle_trapezoid_hc[4] = &pwle_trapezoid_hc_section4;
            bsp_dut_trigger_rth_pwle(false, pwle_trapezoid_hc, 4, 0);
            bsp_set_led(1, (app_state/3), 0);
            break;
        case 2:
            pwle_long[0] = &pwle_long_265hz_section0;
            pwle_long[1] = &pwle_long_265hz_section1;
            pwle_long[2] = &pwle_long_265hz_section2;
            pwle_long[3] = &pwle_long_265hz_section3;
            bsp_dut_trigger_rth_pwle(false, pwle_long, 3, 0);
            bsp_set_led(1, (app_state/3), 0);
            break;
        case 3:
            bsp_dut_trigger_rth_pcm(pcm_data_fs_400hz, 21, 21, 0, 0);
            bsp_set_led(1, (app_state/3), 0);
            break;
        case 4:
            bsp_dut_trigger_rth_pcm(pcm_data_fs_400hz, 21, 21, 2800, 219);
            bsp_set_led(1, (app_state/3), 0);
            break;
        case 5:
            bsp_dut_trigger_rth_pcm(pcm_data_220hz_long, 231, 114, 0, 0);
            bsp_set_led(1, (app_state/3), 0);
            break;
        }

        app_set_sel_leds(app_state%3);
        app_state++;
        app_state %= 6;
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
        bsp_dut_process();
        app_process_pb();
        bsp_sleep();
    }

    exit(1);

    return 0;
}
