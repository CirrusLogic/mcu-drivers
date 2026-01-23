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
// #include "drivers/haptics/cs40l50.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>
#include <sys/_stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define LOG_LEVEL 4

#include "cs40l50_bsp.h"

LOG_MODULE_REGISTER(main);

const struct device *cs40l50 = DEVICE_DT_GET(DT_NODELABEL(haptic1));

#if CONFIG_SHELL
#define CS40L50_HELP                SHELL_HELP("CS40L50 haptics commands", NULL)
#define CS40L50_LIST_WT             SHELL_HELP("List Effects in Main Wavetable", NULL)
#define CS40L50_MANUAL_TRIGGER      SHELL_HELP("Play Effect with Given Intensity, Repeat Count, and Retrigger Period (ms)", "<effect_name> <intensity> <repeats> <retrigger_period>")

// Helper function to convert and validate shell input is int values in valid range
static int convert_int_input(const struct shell *sh, char *input, uint32_t *output, uint32_t min, uint32_t max)
{
    char *endptr;
    if (!input)
    {
        return -1;
    }
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

static int convert_effect_name_input(const struct shell *sh, char *input, uint32_t *idx)
{
    if (input == NULL)
    {
        printf("Error: Invalid Input\n");
        return -1;
    }

    for (int i = 0; i < pwleCount; i++)
    {
        if(strcasecmp(input, pwleList[i]->name) == 0)
        {
            *idx = i;
            return 0;
        }
    }
    printf("Error: Effect name '%s' not found\n", input);
    return -1;
}

static int cmd_list_wt(const struct shell *sh, size_t argc, char **argv)
{
    printf("\n---Wavetable Waveform List---\n");
    for(int i = 1; i < pwleCount; i++)
    {
        uint32_t length_ms = pwleList[i]->length_us / 1000;
        uint32_t length_ms_dec = (pwleList[i]->length_us % 1000) / 10;
        printf("Name : \"%s\", Duration : %d.%02d ms\n", pwleList[i]->name, length_ms, length_ms_dec);
    }
    return 0;
}

static int cmd_manual_trigger(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;

    //Only use one OWT index for triggered waveforms
    //TODO: Preempting OWT trigger calls use new OWT index because we can't delete an index currently being played
    uint32_t idx;
    cs40l50_get_num_owt_wf(cs40l50, &idx);
    if(idx > 0)
    {
        haptics_cs40l50_delete_owt(cs40l50, idx-1);
    }

    uint32_t waveform_idx;
    ret = convert_effect_name_input(sh, argv[1], &waveform_idx);
    if(ret)
        return -1;
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

    struct cs40l50_owt_section_params section =
    {
        .nested_repeats = nested_repeats,
        .waveform_idx = waveform_idx,
        .amplitude = amplitude,
        .delay = delay,
        .owt_subwave = 0,
        .rom_subwave = 0,
        .duration_present = 0,
        .duration = 0
    };

    haptics_cs40l50_write_owt_composite_one_section(cs40l50, section);

    cs40l50_get_num_owt_wf(cs40l50, &idx);
    haptics_cs40l50_trigger_owt(cs40l50, idx-1);

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    cs40l50_cmds,
    SHELL_CMD_ARG(list_wt, NULL, CS40L50_LIST_WT, cmd_list_wt, 1, 0),
    SHELL_CMD_ARG(manual_trigger, NULL, CS40L50_MANUAL_TRIGGER, cmd_manual_trigger, 5, 0),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(cs40l50, &cs40l50_cmds, "CS40L50 shell commands", NULL);
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

    struct cs40l50_haptic_source_config hap_cfg = {
        .index = CS40L50_HAPTIC_ROM_CLICK_14_VCM,
        .bank = ROM_BANK,
    };
    cs40l50_set_haptic_cfg(cs40l50, &hap_cfg);

    if (!cs40l50)
    {
        LOG_ERR("CS40L50 device not found");
        return -ENODEV;
    }
    else if (!device_is_ready(cs40l50))
    {
        LOG_ERR("CS40L50 device %s is not ready", cs40l50->name);
        return -EIO;
    }
    else
    {
        LOG_INF("Found CS40L50 device %s", cs40l50->name);
    }

    while (1)
    {
        if (gpio_pin_get_dt(&button))
        {
            haptics_cs40l50_trigger_owt(cs40l50, 0);
            while (gpio_pin_get_dt(&button))
                ;
        }
        k_msleep(10);
    }
    return 0;
}
