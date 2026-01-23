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
//#include "drivers/haptics/cs40l50.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <sys/_stdint.h>

#define LOG_LEVEL 4

#include "cs40l50_bsp.h"

LOG_MODULE_REGISTER(main);

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE    DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
                                  {0});
static struct gpio_callback button_cb_data;
const struct gpio_dt_spec reset = GPIO_DT_SPEC_GET(DT_NODELABEL(haptic1), reset_gpios);

int main(void)
{
        int ret;
        //Initialize button

        if (!gpio_is_ready_dt(&button)) {
                printk("Error: button device %s is not ready\n",
                button.port->name);
                return 0;
        }

        ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
        if (ret != 0) {
                printk("Error %d: failed to configure %s pin %d\n",
                ret, button.port->name, button.pin);
                return 0;
        }

        char in_char;
        const struct device *dev1 = DEVICE_DT_GET(DT_NODELABEL(haptic1));
        struct cs40l50_haptic_source_config hap_cfg = {
                .index = CS40L50_HAPTIC_ROM_CLICK_14_VCM,
                .bank = ROM_BANK,
        };
        cs40l50_set_haptic_cfg(dev1, &hap_cfg);

        if (!dev1) {
                LOG_ERR("CS40L50 device not found");
                return -ENODEV;
        } else if (!device_is_ready(dev1)) {
                LOG_ERR("CS40L50 device %s is not ready", dev1->name);
                return -EIO;
        } else {
                LOG_INF("Found CS40L50 device %s", dev1->name);
        }

        while (1) {
                if (gpio_pin_get_dt(&button)) {
                        // Set error flag to omit VDD amp low, VDD amp high, ReDC low, and ReDC high errors with sample_mask
                        // Set sample_mask to 0 to show all diagnostic errors
                        uint32_t sample_mask = CS40L50_DIAG_VDD_AMP_HI | CS40L50_DIAG_VDD_B_HI | CS40L50_DIAG_ReDC_HI | CS40L50_DIAG_LE_HI;
                        bool diagnostics_err = false;
                        uint32_t diagnostics_data;

#ifdef TEST_DIAGNOSTICS_EXTREMES
                        //Test low or high extremes of diagnostics test values
                        bool low_extreme_test = true;
                        haptics_cs40l50_test_diagnostics(dev1, low_extreme_test);
#endif //TEST_DIAGNOSTICS_EXTREMES
                        ret = haptics_cs40l50_diagnostics(dev1, &diagnostics_data, sample_mask, &diagnostics_err);
                        if (ret)
                        {
                                LOG_ERR("Error running diagnostics");
                        }

                        if (diagnostics_err)
                        {
                                LOG_INF("Masked Diagnostic Flag TRUE");
                        }
                        else
                        {
                                LOG_INF("Masked Diagnostic Flag FALSE");
                        }

                        while (gpio_pin_get_dt(&button));
                }
                k_msleep(10);
        }

        return 0;
}
