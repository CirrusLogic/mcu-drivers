/*
 * Copyright (c) Cirrus Logic 2026 All Rights Reserved, http://www.cirrus.com/
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
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/i2c.h>
// #include "drivers/haptics/cs40l5x.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <sys/_stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <ctype.h>

#define LOG_LEVEL 4

#include "cs40l5x_bsp.h"

LOG_MODULE_REGISTER(main);

const struct device *cs40l5x = DEVICE_DT_GET(DT_NODELABEL(haptic1));

#if CONFIG_SHELL
#define CS40L5X_HELP                        SHELL_HELP("CS40L5X haptics commands", NULL)
#define CS40L5X_LIST_WT                     SHELL_HELP("List Effects in Main Wavetable", NULL)
#define CS40L5X_EFFECT_MSFT_ID_TRIGGER      SHELL_HELP("Play Effect indicated by Microsoft ID with Given Intensity, Repeat Count, Retrigger Period (ms), and max waveform playback time (ms)", "<msft_ID> <intensity> <repeats> <retrigger_period> <cutoff_time>")
#define CS40L5X_EFFECT_NAME_TRIGGER         SHELL_HELP("Play Effect indicated by effect name with Given Intensity, Repeat Count, and Retrigger Period (ms), and max waveform playback time (ms)", "<effect_name> <intensity> <repeats> <retrigger_period> <cutoff_time>")

// Helper function to convert and validate shell input is int values in valid range
static int convert_int_input(const struct shell *sh, char *input, uint32_t *output, uint32_t min, uint32_t max)
{
    char *endptr;
    if (!input)
    {
        return -1;
    }
    if((input[0] == '0' && input[1] == 'x') || (input[0] == '0' && input[1] == 'X'))
    {
        for (int i = 2; input[i] != '\0'; i++)
        {
            if (!isxdigit(input[i]))
            {
                shell_error(sh, "Error: Invalid input parameter %s\n", input);
                return -1;
            }
        }

        *output = (uint32_t)strtol(input, &endptr, 16);
        if (*output < min || *output > max)
        {
            printf("Error: Invalid input %s, input must be number between %d and %d\n", input, min, max);
            return -1;
        }
        return 0;
    }
    else
    {
        for (int i = 0; input[i] != '\0'; i++)
        {
            if (!isdigit(input[i]))
            {
                shell_error(sh, "Error: Invalid input parameter %s\n", input);
                return -1;
            }
        }

        *output = (uint32_t)strtol(input, &endptr, 0);
        if (*output < min || *output > max)
        {
            printf("Error: Invalid input %s, input must be number between %d and %d\n", input, min, max);
            return -1;
        }
        return 0;
    }
}

static int cmd_list_wt(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    ret = bsp_cs40l5x_list_host_initiated_effects(cs40l5x);
    return ret;
}

static int cmd_effect_name_trigger(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;

    HIH_effect effect_data;
    effect_data.label = EFFECT_NAME;
    strcpy(effect_data.HIH_effect_identifier.effectName, argv[1]);

    uint32_t amplitude;
    ret = convert_int_input(sh, argv[2], &amplitude, 1, 200);
    if(ret)
        return -1;
    uint32_t nested_repeats;
    ret = convert_int_input(sh, argv[3], &nested_repeats, 0, 255);
    if(ret)
        return -1;
    uint32_t delay;
    ret = convert_int_input(sh, argv[4], &delay, 0, 10000);
    if(ret)
        return -1;
    uint32_t cutoff_time;
    ret = convert_int_input(sh, argv[5], &cutoff_time, 0, 0xFFFFFF);
    if(ret)
        return -1;


    ret = bsp_cs40l5x_host_initiated_trigger(cs40l5x, effect_data, amplitude, nested_repeats, delay, cutoff_time);
    return ret;
}

static int cmd_effect_msft_id_trigger(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;

    uint32_t msft_id;
    ret = convert_int_input(sh, argv[1], &msft_id, 0, 0x10000);
    if(ret)
        return -1;

    HIH_effect effect_data;
    effect_data.label = EFFECT_MSFT_ID;
    effect_data.HIH_effect_identifier.msft_ID = msft_id;

    uint32_t amplitude;
    ret = convert_int_input(sh, argv[2], &amplitude, 1, 200);
    if(ret)
        return -1;
    uint32_t nested_repeats;
    ret = convert_int_input(sh, argv[3], &nested_repeats, 0, 255);
    if(ret)
        return -1;
    uint32_t delay;
    ret = convert_int_input(sh, argv[4], &delay, 0, 10000);
    if(ret)
        return -1;
    uint32_t cutoff_time;
    ret = convert_int_input(sh, argv[5], &cutoff_time, 0, 0xFFFFFF);
    if(ret)
        return -1;

    ret = bsp_cs40l5x_host_initiated_trigger(cs40l5x, effect_data, amplitude, nested_repeats, delay, cutoff_time);
    return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    cs40l5x_cmds,
    SHELL_CMD_ARG(list_wt, NULL, CS40L5X_LIST_WT, cmd_list_wt, 1, 0),
    SHELL_CMD_ARG(effect_msft_id_trigger, NULL, CS40L5X_EFFECT_MSFT_ID_TRIGGER, cmd_effect_msft_id_trigger, 6, 0),
    SHELL_CMD_ARG(effect_name_trigger, NULL, CS40L5X_EFFECT_NAME_TRIGGER, cmd_effect_name_trigger, 6, 0),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(cs40l5x, &cs40l5x_cmds, "CS40L5X shell commands", NULL);
#endif /* CONFIG_SHELL */

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
                                                              {0});

int main(void)
{
    int ret;

    if (!gpio_is_ready_dt(&button))
    {
        printk("Error: button device %s is not ready\n",
               button.port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0)
    {
        printk("Error %d: failed to configure %s pin %d\n",
               ret, button.port->name, button.pin);
        return 0;
    }

    struct cs40l5x_haptic_source_config hap_cfg = {
        .index = CS40L5X_HAPTIC_ROM_CLICK_14_VCM,
        .bank = ROM_BANK,
    };
    cs40l5x_set_haptic_cfg(cs40l5x, &hap_cfg);

    if (!cs40l5x)
    {
        LOG_ERR("CS40L5X device not found");
        return -ENODEV;
    }
    else if (!device_is_ready(cs40l5x))
    {
        LOG_ERR("CS40L5X device %s is not ready", cs40l5x->name);
        return -EIO;
    }
    else
    {
        LOG_INF("Found CS40L5X device %s", cs40l5x->name);
    }

    while (1)
    {
        if (gpio_pin_get_dt(&button))
        {
            ret = haptics_start_output(cs40l5x);
            while (gpio_pin_get_dt(&button))
                ;
        }
        k_msleep(10);
    }
    return 0;
}
