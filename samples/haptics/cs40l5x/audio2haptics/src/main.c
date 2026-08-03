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
#include <ctype.h>

#define LOG_LEVEL 4

#include "cs40l5x_bsp.h"

LOG_MODULE_REGISTER(main);

const struct device *cs40l5x = DEVICE_DT_GET(DT_NODELABEL(haptic1));

#if CONFIG_SHELL
#define CS40L5X_HELP          SHELL_HELP("CS40L5X haptics commands", NULL)
#define CS40L5X_START_A2H     SHELL_HELP("Begin A2H streaming", NULL)
#define CS40L5X_STOP_A2H      SHELL_HELP("Stop A2H streaming", NULL)
#define CS40L5X_BASIC_TRIGGER SHELL_HELP("Perform a normal wavetable trigger", "<ROM/RAM> <index>")
#define CS40L5X_VIBECOMP_TRIGGER SHELL_HELP("Stop A2H streaming", "<ROM/RAM> <index> <speed>")

// Helper function to convert and validate shell input is int values in valid range
static int convert_int_input(const struct shell *sh, char *input, uint32_t *output, uint32_t min,
                 uint32_t max)
{
    char *endptr;
    if (!input) {
        return -1;
    }
    if ((input[0] == '0' && input[1] == 'x') || (input[0] == '0' && input[1] == 'X')) {
        for (int i = 2; input[i] != '\0'; i++) {
            if (!isxdigit(input[i])) {
                shell_error(sh, "Error: Invalid input parameter %s\n", input);
                return -1;
            }
        }

        *output = (uint32_t)strtol(input, &endptr, 16);
        if (*output < min || *output > max) {
            printf("Error: Invalid input %s, input must be number between %d and %d\n",
                   input, min, max);
            return -1;
        }
        return 0;
    } else {
        for (int i = 0; input[i] != '\0'; i++) {
            if (!isdigit(input[i])) {
                shell_error(sh, "Error: Invalid input parameter %s\n", input);
                return -1;
            }
        }

        *output = (uint32_t)strtol(input, &endptr, 0);
        if (*output < min || *output > max) {
            printf("Error: Invalid input %s, input must be number between %d and %d\n",
                   input, min, max);
            return -1;
        }
        return 0;
    }
}

static int cmd_start_A2H(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    printf("\nStarting ASP streaming...\n");
    ret = bsp_cs40l5x_start_i2s(cs40l5x);
    if (ret) {
        LOG_ERR("Problem starting A2H");
    }
    return ret;
}

static int cmd_stop_A2H(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    printf("\nStopping ASP streaming...\n");
    ret = bsp_cs40l5x_stop_i2s(cs40l5x);
    if (ret) {
        LOG_ERR("Problem stopping A2H");
    }
    return ret;
}

static int cmd_basic_trigger(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    struct cs40l5x_haptic_source_config hap_cfg;

    if (strcmp(argv[1], "ROM") == 0) {
        hap_cfg.bank = ROM_BANK;
    } else if (strcmp(argv[1], "RAM") == 0) {
        hap_cfg.bank = RAM_BANK;
    } else {
        printf("Invalid input for ROM/RAM bank\n");
        return -1;
    }
    ret = convert_int_input(sh, argv[2], &hap_cfg.index, 0, 30);
    ret = cs40l5x_set_haptic_cfg(cs40l5x, &hap_cfg);
    if (ret) {
        return ret;
    }

    ret = haptics_start_output(cs40l5x);
    if (ret) {
        return ret;
    }
    return ret;
}

static int cmd_vibecomp_trigger(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    struct cs40l5x_haptic_source_config hap_cfg;

    if (strcmp(argv[1], "ROM") == 0) {
        hap_cfg.bank = ROM_BANK;
    } else if (strcmp(argv[1], "RAM") == 0) {
        hap_cfg.bank = RAM_BANK;
    } else {
        printf("Invalid input for ROM/RAM bank\n");
        return -1;
    }

    ret = convert_int_input(sh, argv[2], &hap_cfg.index, 0, 30);
    ret = cs40l5x_set_haptic_cfg(cs40l5x, &hap_cfg);
    if (ret) {
        return ret;
    }

    uint32_t speed;
    ret = convert_int_input(sh, argv[3], &speed, 0, 255);
    ret = cs40l5x_set_vibecomp_speed(cs40l5x, speed);

    ret = haptics_start_output(cs40l5x);
    if (ret) {
        return ret;
    }
    return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    cs40l5x_cmds, SHELL_CMD_ARG(start_A2H, NULL, CS40L5X_START_A2H, cmd_start_A2H, 1, 0),
    SHELL_CMD_ARG(stop_A2H, NULL, CS40L5X_STOP_A2H, cmd_stop_A2H, 1, 0),
    SHELL_CMD_ARG(basic_trigger, NULL, CS40L5X_BASIC_TRIGGER, cmd_basic_trigger, 3, 0),
    SHELL_CMD_ARG(vibecomp_trigger, NULL, CS40L5X_VIBECOMP_TRIGGER, cmd_vibecomp_trigger, 4, 0),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(cs40l5x, &cs40l5x_cmds, "CS40L5X shell commands", NULL);
#endif /* CONFIG_SHELL */

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

int main(void)
{
    int ret;

    if (!gpio_is_ready_dt(&button)) {
        LOG_ERR("Error: button device %s is not ready", button.port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure %s pin %d", ret, button.port->name,
               button.pin);
        return 0;
    }
    char in_char;
    const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(usart2));
    struct cs40l5x_haptic_source_config hap_cfg = {
        .index = CS40L5X_HAPTIC_ROM_CLICK_1_VCM,
        .bank = ROM_BANK,
    };
    cs40l5x_set_haptic_cfg(cs40l5x, &hap_cfg);

    if (!cs40l5x) {
        LOG_ERR("CS40L5X device not found");
        return -ENODEV;
    } else if (!device_is_ready(cs40l5x)) {
        LOG_ERR("CS40L5X device %s is not ready", cs40l5x->name);
        return -EIO;
    } else {
        LOG_INF("Found CS40L5X device %s", cs40l5x->name);
    }

    while (1) {

        ret = uart_poll_in(uart, &in_char);

        if (ret == 0) {
            ret = haptics_start_output(cs40l5x);
            if (ret < 0) {
                LOG_ERR("Failed to start output: %d", ret);
                return ret;
            }
        }
        k_msleep(1);
    }

    return 0;
}
