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
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include "cs35l56_bsp.h"

#define LOG_LEVEL 4

#define AUDIO_AMP_LEFT  0
#define AUDIO_AMP_RIGHT 1

LOG_MODULE_REGISTER(main);

const struct device *amps[] = {
    [AUDIO_AMP_LEFT] = DEVICE_DT_GET(DT_NODELABEL(cs35l56_l)),
    [AUDIO_AMP_RIGHT] = DEVICE_DT_GET(DT_NODELABEL(cs35l56_r)),
};

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

static int init_button(void)
{
    int ret;
    if (!gpio_is_ready_dt(&button)) {
        printk("Error: button device %s is not ready\n", button.port->name);
        return ret;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
               button.pin);
        return ret;
    }
    return 0;
}

static void set_codec_output_state(bool playback)
{
    printk("Audio playback %s\n", playback ? "starting" : "paused");
    if (playback) {
        for (int i = 0; i < ARRAY_SIZE(amps); i++) {
            if (amps[i] == NULL) {
                continue;
            }
            audio_codec_start_output(amps[i]);
        }
    } else {
        for (int i = 0; i < ARRAY_SIZE(amps); i++) {
            if (amps[i] == NULL) {
                continue;
            }
            audio_codec_stop_output(amps[i]);
        }
    }
    return;
}

int main(void)
{
    init_button();

    if (amps[AUDIO_AMP_LEFT] == NULL || amps[AUDIO_AMP_RIGHT] == NULL) {
        LOG_ERR("CS35L56 stereo device not found");
        return -ENODEV;
    }
    for (int i = 0; i < ARRAY_SIZE(amps); i++) {
        if (!device_is_ready(amps[i])) {
            LOG_ERR("CS35L56 device %s is not ready", amps[i]->name);
            return -EIO;
        } else {
            LOG_INF("Found CS35L56 device %s", amps[i]->name);
        }
    }

    bool playback = false;
    while (1) {
        // Playback is initially paused, toggle playback on button press
        if (gpio_pin_get_dt(&button)) {
            playback = !playback;
            set_codec_output_state(playback);
            k_msleep(100);
            while (gpio_pin_get_dt(&button))
                ;
        }
        k_msleep(1);
    }

    return 0;
}
