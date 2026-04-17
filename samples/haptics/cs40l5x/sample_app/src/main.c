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
//#include "drivers/haptics/cs40l5x.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <sys/_stdint.h>

#define LOG_LEVEL 4

#include "cs40l5x_bsp.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
        int ret;
        char in_char;
        const struct device *cs40l5x = DEVICE_DT_GET(DT_NODELABEL(haptic1));
        const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(usart2));
        struct cs40l5x_haptic_source_config hap_cfg = {
                .index = CS40L5X_HAPTIC_ROM_ZIPPER,
                .bank = ROM_BANK,
        };
        cs40l5x_set_haptic_cfg(cs40l5x, &hap_cfg);

        if (!cs40l5x) {
                LOG_ERR("CS40L5X device not found");
                return -ENODEV;
        } else if (!device_is_ready(cs40l5x)) {
                LOG_ERR("CS40L5X device %s is not ready", cs40l5x->name);
                //return -EIO;
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
