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
#include "owt.h"

#define LOG_LEVEL 4

#include "cs40l5x_bsp.h"

LOG_MODULE_REGISTER(main);

const struct device *cs40l5x = DEVICE_DT_GET(DT_NODELABEL(haptic1));

#if CONFIG_SHELL
#define CS40L5X_HELP SHELL_HELP("CS40L5X haptics commands", NULL)
#define CS40L5X_STORE_PCM                                                                          \
    SHELL_HELP("Store Example Sine Wave PCM Effect in Open Wavetable",                         \
           "<frequency_hz> <length_ms> <fs_hz>")
#define CS40L5X_STORE_COMPOSITE                                                                    \
    SHELL_HELP("Store Composite Effect in Open Wavetable after prompting for parameters", NULL)
#define CS40L5X_STORE_FF_CIRRUS SHELL_HELP("Store Compiste Effect in OWT based on ff_cirrus", "<ff_cirrus_string>")
#define CS40L5X_STORE_PWLE                                                                         \
    SHELL_HELP("Store PWLE Effect in Open Wavetable after prompting for parameters", NULL)
#define CS40L5X_TRIGGER SHELL_HELP("Trigger an Effect in Open Wavetable", "<owt_wf_index>")
#define CS40L5X_GET_NUM_OWT_EFFECTS SHELL_HELP("Get Number of Effects in Open Wavetable", NULL)
#define CS40L5X_DUMP_REGS           SHELL_HELP("Dump consecutive registers", "<addr> <num_words>")

// Shell helper function to receive line input, up to len characters
static char *shell_getline(const struct shell *shell, char *buf, const size_t len)
{
    fflush(stdout);
    if (!buf) {
        return NULL;
    }

    char *p = buf;
    memset(buf, 0, len);
    for (size_t i = 0; i < (len - 1); i++) {
        char c;
        size_t cnt;

        shell->iface->api->read(shell->iface, &c, sizeof(c), &cnt);
        while (cnt == 0) {
            k_msleep(100);
            shell->iface->api->read(shell->iface, &c, sizeof(c), &cnt);
        }
        shell->iface->api->write(shell->iface, &c, sizeof(c), &cnt);
        if (c == '\n' || c == '\r') {
            break;
        }
        *p++ = c;
    }

    return buf;
}

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

static int cmd_store_pcm_owt(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    uint32_t frequency;
    uint32_t length;
    uint32_t fs;

    ARG_UNUSED(argc);

    ret = convert_int_input(sh, argv[1], &frequency, 1, UINT32_MAX);
    if (ret) {
        return ret;
    }

    ret = convert_int_input(sh, argv[2], &length, 1, UINT32_MAX);
    if (ret) {
        return ret;
    }

    ret = convert_int_input(sh, argv[3], &fs, 1, UINT32_MAX);
    if (ret) {
        return ret;
    }

    uint32_t buf_size = fs * length / 1000;
    uint8_t sample_rate;
    switch (fs) {
    case (8000):
        sample_rate = CS40L5X_OWT_SAMPLE_RATE_8K;
        break;
    case (4000):
        sample_rate = CS40L5X_OWT_SAMPLE_RATE_4K;
        break;
    case (24000):
        sample_rate = CS40L5X_OWT_SAMPLE_RATE_24K;
        break;
    case (48000):
        sample_rate = CS40L5X_OWT_SAMPLE_RATE_48K;
        break;
    default:
        LOG_ERR("Invalid sample rate for PCM!");
        return -1;
    }

    uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * buf_size);

    // Fill buffer with sine data at frequency
    uint32_t sin_idx = 0;
    uint32_t increment = (sizeof(sineLookupTable) * frequency) / fs;
    for (int buf_idx = 0; buf_idx < buf_size; buf_idx++) {
        buffer[buf_idx] = sineLookupTable[sin_idx % sizeof(sineLookupTable)];
        sin_idx += increment;
    }

    ret = bsp_cs40l5x_write_owt_pcm(cs40l5x, sample_rate, buffer, buf_size);
    free(buffer);
    if (ret) {
        LOG_ERR("Problem writing PCM");
        return ret;
    }
    return 0;
}

static int cmd_store_composite_owt(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    uint32_t input_arg;
    const uint32_t read_buf_len = 255;
    char read_buf[read_buf_len];
    memset(read_buf, 0, read_buf_len);

    uint8_t num_waveforms = NUM_WAVEFORMS_DEFAULT;
    uint8_t repeats = REPEATS_DEFAULT;

    printf("----Constructing Composite Header----\n\n");

    printf("Provide Number of Waveforms: ");
    shell_getline(sh, read_buf, read_buf_len);
    ret = convert_int_input(sh, read_buf, &input_arg, 1, 255);
    if (ret) {
        return ret;
    }
    printf("\n%d waveforms\n", input_arg);
    num_waveforms = input_arg;

    printf("Provide Number of overall repeats: ");
    shell_getline(sh, read_buf, read_buf_len);
    ret = convert_int_input(sh, read_buf, &input_arg, 0, 255);
    if (ret) {
        return ret;
    }
    printf("\n%d overall repeats\n", input_arg);
    repeats = input_arg;

    bsp_cs40l5x_write_owt_composite_header(cs40l5x, num_waveforms, repeats);

    printf("----Constructing Composite Sections----\n\n");
    for (int i = 0; i < num_waveforms; i++) {
        printf("----Constructing Section %d----\n", i);
        struct cs40l5x_owt_composite_section_params section;

        printf("Provide ID of waveform %d: ", i);
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 8);
        if (ret) {
            return ret;
        }
        printf("\nWaveform ID: %d\n", input_arg);
        section.waveform_idx = input_arg + 1;

        printf("Provide Number of nested repeats of waveform %d: ", i);
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 255);
        if (ret) {
            return ret;
        }
        printf("\nNested Repeats: %d\n", input_arg);
        section.nested_repeats = input_arg;

        printf("Provide amplitude %d: ", i);
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 1, 200);
        if (ret) {
            return ret;
        }
        printf("\nAmplitude: %d\n", input_arg);
        section.amplitude = input_arg;

        printf("Provide Delay after waveform %d (in ms): ", i);
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 10000);
        if (ret) {
            return ret;
        }
        printf("\nDelay: %d ms\n", input_arg);
        section.delay = input_arg;

#ifdef ADDITIONAL_OWT_WF_PARAMETERS
        printf("Is this an OWT subwaveform? (1 for yes, 0 for no): ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 1);
        if (ret) {
            return ret;
        }
        printf("\nIs this an OWT subwaveform? ");
        input_arg ? printf("Yes\n") : printf("No\n");
        section.owt_subwave = input_arg;

        printf("Is this a ROM subwaveform? (1 for yes, 0 for no): ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 1);
        if (ret) {
            return ret;
        }
        printf("\nIs this a ROM subwaveform? ");
        input_arg ? printf("Yes\n") : printf("No\n");
        section.rom_subwave = input_arg;

        printf("Is duration present? (1 for yes, 0 for no): ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 1);
        if (ret) {
            return ret;
        }
        printf("\nIs duration present? ");
        input_arg ? printf("Yes\n") : printf("No\n");
        section.duration_present = input_arg;

        if (section.duration_present) {
            printf("Provide Duration after waveform %d (in ms): ", i);
            shell_getline(sh, read_buf, read_buf_len);
            ret = convert_int_input(sh, read_buf, &input_arg, 0, 65534);
            if (ret) {
                return ret;
            }
            section.duration = input_arg;
        } else {
            section.duration = 0;
        }
        printf("\nDuration: %d\n", section.duration);
#endif // ADDITIONAL_OWT_WF_PARAMETERS

        bsp_cs40l5x_write_owt_composite_section(cs40l5x, section);
    }

    bsp_cs40l5x_push_owt_composite(cs40l5x);
    return 0;
}

static int cmd_store_pwle_owt(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    uint32_t input_arg;
    const uint32_t read_buf_len = 255;
    char read_buf[read_buf_len];
    memset(read_buf, 0, read_buf_len);

    uint8_t next_first_byte;
    uint32_t wf_length = WF_LENGTH_DEFAULT;
    uint8_t repeats = REPEATS_DEFAULT;
    uint16_t wait_time = WAIT_TIME_DEFAULT;
    uint8_t num_sections = PW_LIN_SECTIONS_DEFAULT;

    printf("----Constructing PWLE Header----\n");

    printf("\nProvide Number of Repeats: ");
    shell_getline(sh, read_buf, read_buf_len);
    ret = convert_int_input(sh, read_buf, &input_arg, 0, 255);
    if (ret) {
        return ret;
    }
    printf("\n%d repeats\n", input_arg);
    repeats = input_arg;

    printf("Provide wait time between repeats (ms): ");
    shell_getline(sh, read_buf, read_buf_len);
    ret = convert_int_input(sh, read_buf, &input_arg, 0, 1023);
    if (ret) {
        return ret;
    }
    printf("\n%d ms wait time\n", input_arg);
    wait_time = input_arg;

    printf("Provide number of sections (1-254): ");
    shell_getline(sh, read_buf, read_buf_len);
    ret = convert_int_input(sh, read_buf, &input_arg, 1, 254);
    if (ret) {
        return ret;
    }
    printf("\n%d sections\n", input_arg);
    num_sections = input_arg + 1;

    bsp_cs40l5x_write_owt_pwle_header(cs40l5x, &next_first_byte, wf_length, repeats, wait_time,
                      num_sections);
    struct cs40l5x_owt_pwle_section_params section;
    memset(&section, 0, sizeof(section));

    printf("Provide initial PWLE level: ");
    shell_getline(sh, read_buf, read_buf_len);
    ret = convert_int_input(sh, read_buf, &input_arg, 0, 0xFFF);
    if (ret) {
        return ret;
    }
    printf("\nInitial Level: %d\n", input_arg);
    section.level = input_arg;

    bsp_cs40l5x_write_owt_pwle_section(cs40l5x, true, &next_first_byte, section);

    printf("----Constructing PWLE Sections----\n\n");
    for (int i = 0; i < num_sections - 1; i++) {
        printf("----Constructing Section %d----\n", i);
        memset(&section, 0, sizeof(section));

        printf("Provide length of waveform in ms: ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 16383);
        if (ret) {
            return ret;
        }
        printf("Length of waveform in ms: %d\n", input_arg);
        section.time = input_arg * 4;

        printf("Provide Level of waveform: ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 0xFFF);
        if (ret) {
            return ret;
        }
        printf("\nWF Level: %d\n", input_arg);
        section.level = input_arg;

        printf("Provide Frequency: ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 0xFFF);
        if (ret) {
            return ret;
        }
        printf("\nFrequency: %d\n", input_arg);
        section.frequency = input_arg;

#ifdef ADDITIONAL_OWT_WF_PARAMETERS
        printf("Chirp Mode? (1 for yes, 0 for no): ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 1);
        if (ret) {
            return ret;
        }
        input_arg ? printf("\nYes\n") : printf("\nNo\n");
        section.chirp_mode = input_arg;

        printf("Half Cycle Definition? (1 for yes, 0 for no): ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 1);
        if (ret) {
            return ret;
        }
        input_arg ? printf("\nYes\n") : printf("\nNo\n");
        section.half_cycle_def = input_arg;

        printf("Extended Frequency? (1 for yes, 0 for no): ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 1);
        if (ret) {
            return ret;
        }
        input_arg ? printf("\nYes\n") : printf("\nNo\n");
        section.ext_frequency = input_arg;

        printf("F0-Relative Frequency? (1 for yes, 0 for no): ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 1);
        if (ret) {
            return ret;
        }
        input_arg ? printf("\nYes\n") : printf("\nNo\n");
        section.F0_relative_freq = input_arg;

        printf("Phase Offset? (1 for yes, 0 for no): ");
        shell_getline(sh, read_buf, read_buf_len);
        ret = convert_int_input(sh, read_buf, &input_arg, 0, 1);
        if (ret) {
            return ret;
        }
        input_arg ? printf("\nYes\n") : printf("\nNo\n");
        section.phase_offset = input_arg;
#endif // ADDITIONAL_OWT_WF_PARAMETERS

        bsp_cs40l5x_write_owt_pwle_section(cs40l5x, false, &next_first_byte, section);
    }
    bsp_cs40l5x_push_owt_pwle(cs40l5x, &next_first_byte);
    return 0;
}

static int cmd_store_ff_cirrus(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    ret = get_owt_data(cs40l5x, argv[1]);
    return 0;
}

static int cmd_trigger_owt(const struct shell *sh, size_t argc, char **argv)
{
    char *endptr;
    uint32_t idx = strtol(argv[1], &endptr, 10);
    return haptics_cs40l5x_trigger_owt(cs40l5x, idx);
}

static int cmd_get_num_owt_effects(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t num;
    uint32_t ret = bsp_cs40l5x_get_num_owt_wf(cs40l5x, &num);
    if (ret) {
        return ret;
    }
    printf("\nOWT currently contains %d effects.\n", num);
    return 0;
}


static int cmd_dump_regs(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t ret;
    uint32_t addr;
    uint32_t num_words;

    ret = convert_int_input(sh, argv[1], &addr, 0, UINT32_MAX);
    if (ret) {
        return ret;
    }

    ret = convert_int_input(sh, argv[2], &num_words, 0, UINT16_MAX);
    if (ret) {
        return ret;
    }

    return bsp_cs40l5x_dump_regs(cs40l5x, addr, num_words);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    cs40l5x_cmds,
    SHELL_CMD_ARG(store_pcm_owt, NULL, CS40L5X_STORE_PCM, cmd_store_pcm_owt, 4, 0),
    SHELL_CMD_ARG(store_composite_owt, NULL, CS40L5X_STORE_COMPOSITE, cmd_store_composite_owt,
              1, 0),
    SHELL_CMD_ARG(store_ff_cirrus, NULL, CS40L5X_STORE_FF_CIRRUS, cmd_store_ff_cirrus,
              2, 0),
    SHELL_CMD_ARG(store_pwle_owt, NULL, CS40L5X_STORE_PWLE, cmd_store_pwle_owt, 1, 0),
    SHELL_CMD_ARG(trigger_owt, NULL, CS40L5X_TRIGGER, cmd_trigger_owt, 2, 0),
    SHELL_CMD_ARG(get_num_owt_effects, NULL, CS40L5X_GET_NUM_OWT_EFFECTS,
              cmd_get_num_owt_effects, 1, 0),
    SHELL_CMD_ARG(dump_regs, NULL, CS40L5X_DUMP_REGS, cmd_dump_regs, 3, 0),
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
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

int main(void)
{
    int ret;

    if (!gpio_is_ready_dt(&button)) {
        printk("Error: button device %s is not ready\n", button.port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
               button.pin);
        return 0;
    }

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
        if (gpio_pin_get_dt(&button)) {
            ret = haptics_start_output(cs40l5x);
            while (gpio_pin_get_dt(&button))
                ;
        }
        k_msleep(10);
    }
    return 0;
}
