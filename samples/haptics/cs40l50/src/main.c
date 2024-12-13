/*
 * Copyright (c) 2024 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/i2c.h>
//#include "drivers/haptics/cs40l50.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <sys/_stdint.h>

#define LOG_LEVEL 4

#include "cs40l50_bsp.h"

LOG_MODULE_REGISTER(main);

int main(void)
{
        int ret;
        char in_char;
        const struct device *dev1 = DEVICE_DT_GET(DT_NODELABEL(haptic1));
        const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(usart2));
        struct cs40l50_haptic_source_config hap_cfg = {
                .index = 23,
                .bank = 0, //ROM
        };

        if (!dev1) {
                LOG_ERR("CS40L50 device not found");
                return -ENODEV;
        } else if (!device_is_ready(dev1)) {
                LOG_ERR("CS40L50 device %s is not ready", dev1->name);
                //return -EIO;
        } else {
                LOG_INF("Found CS40L50 device %s", dev1->name);
        }

        while (1) {

                ret = uart_poll_in(uart, &in_char);

                if (ret == 0) {
                        ret = haptics_start_output(dev1);
                        if (ret < 0) {
                                LOG_ERR("Failed to start output: %d", ret);
                                return ret;
                        }
                }
                k_msleep(1);
        }

        return 0;
}
