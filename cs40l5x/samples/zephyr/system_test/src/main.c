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
#include <sys/_stdint.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

#define LOG_LEVEL 4

#include "cs40l5x_bsp.h"

LOG_MODULE_REGISTER(main);

#ifdef CONFIG_HAPTICS_CS40L50
const struct device *cs40l5x = DEVICE_DT_GET(DT_NODELABEL(haptic1));
#endif

#ifdef CONFIG_HAPTICS_CS40L51
const struct device *cs40l5x = DEVICE_DT_GET(DT_NODELABEL(haptic1));
#endif

#ifdef CONFIG_HAPTICS_CS40L52
const struct device *cs40l5x = DEVICE_DT_GET(DT_NODELABEL(haptic1));
#endif

#ifdef CONFIG_HAPTICS_CS40L53
const struct device *cs40l5x = DEVICE_DT_GET(DT_NODELABEL(haptic1));
#endif

#ifdef CONFIG_HAPTICS_CS40L50_PC
const struct device *cs40l5x = DEVICE_DT_GET(DT_NODELABEL(haptic1));
#endif

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(usart2));

#if CONFIG_SHELL
#define CS40L5X_HELP           SHELL_HELP("CS40L5X haptics commands", NULL)
#define CS40L5X_SYSTEM_TEST    SHELL_HELP("Read back test registers", NULL)
#define CS40L5X_TRIGGER_HAPTIC SHELL_HELP("Trigger ROM waveform at given index", "<effect_index>")

static int cmd_system_test(const struct shell *sh, size_t argc, char **argv)
{
	printf("\n---Reading Back System Registers---\n");
	cs40l5x_system_test(cs40l5x);
	return 0;
}

static int cmd_trigger_haptic(const struct shell *sh, size_t argc, char **argv)
{
	struct cs40l5x_haptic_source_config hap_cfg = {
		.index = atoi(argv[1]),
		.bank = ROM_BANK,
	};
	cs40l5x_set_haptic_cfg(cs40l5x, &hap_cfg);
	uint32_t ret = haptics_start_output(cs40l5x);
	if (ret < 0) {
		LOG_ERR("Failed to start haptic output: %d", ret);
		return ret;
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cs40l5x_cmds, SHELL_CMD_ARG(system_test, NULL, CS40L5X_SYSTEM_TEST, cmd_system_test, 1, 0),
	SHELL_CMD_ARG(trigger_haptic, NULL, CS40L5X_TRIGGER_HAPTIC, cmd_trigger_haptic, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(cs40l5x, &cs40l5x_cmds, "CS40L5X shell commands", NULL);
#endif /* CONFIG_SHELL */

int main(void)
{
	if (!cs40l5x) {
		LOG_ERR("CS40L5X device not found");
		return -ENODEV;
	} else if (!device_is_ready(cs40l5x)) {
		LOG_ERR("CS40L5X device %s is not ready", cs40l5x->name);
		// return -EIO;
	} else {
		LOG_INF("Found CS40L5X device %s", cs40l5x->name);
	}

	while (1) {
		k_msleep(1);
	}

	return 0;
}
