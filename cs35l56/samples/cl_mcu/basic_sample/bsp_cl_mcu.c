/**
 * @file bsp_cl_mcu.c
 *
 * @brief Implementation of the BSP for the cs35l56 on the CL-MCU platform.
 *
 * Wraps MSPPHooks I2C controller functions to satisfy the bsp_driver_if_t
 * interface consumed by regmap.c and cs35l56.c.
 *
 * @copyright
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

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <string.h>
#include "main.h"
#ifndef CIRRUS_SDK
#define CIRRUS_SDK
#endif
#include "cs35l56.h"
#include "regmap.h"
#include "bsp_driver_if.h"
#include "bsp_dut.h"
#include "mspp_hooks_i2ccon.h"
#include "main.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define BSP_I2C_TIMEOUT_MS (100) ///< Default blocking I2C timeout in milliseconds

#define BSP_DUT_DEV_ID_0 (0x60) ///< 7-bit I2C address of DUT

#define BSP_GPIO_ID_DUT0_RST (0) ///< GPIO ID for DUT reset (GPIO0_DUT_RST0)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/* CL_I2C0 handle is owned by main.c */
extern MSPPHooks_I2CCon_Handle CL_I2C0;

static cs35l56_t cs35l56_driver;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS - bsp_driver_if_t implementation
 **********************************************************************************************************************/

static uint32_t cl_mcu_set_gpio(uint32_t gpio_id, uint8_t gpio_state)
{
	GPIO_PinState pin_state = (gpio_state == BSP_GPIO_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;

	switch (gpio_id) {
	case BSP_GPIO_ID_DUT0_RST:
		HAL_GPIO_WritePin(GPIO0_DUT_RST0_GPIO_Port, GPIO0_DUT_RST0_Pin, pin_state);
		return BSP_STATUS_OK;
	default:
		return BSP_STATUS_FAIL;
	}
}

static uint32_t cl_mcu_register_gpio_cb(uint32_t gpio_id, bsp_callback_t cb, void *cb_arg)
{
	(void)gpio_id;
	(void)cb;
	(void)cb_arg;
	return BSP_STATUS_OK;
}

static uint32_t cl_mcu_i2c_reset(uint32_t bsp_dev_id, bool *was_i2c_busy)
{
	(void)bsp_dev_id;
	if (was_i2c_busy != NULL) {
		*was_i2c_busy = false;
	}
	return BSP_STATUS_OK;
}

static uint32_t cl_mcu_set_timer(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg)
{
	HAL_Delay(duration_ms);
	if (cb != NULL) {
		cb(BSP_STATUS_OK, cb_arg);
	}
	return BSP_STATUS_OK;
}

static uint32_t cl_mcu_i2c_read_repeated_start(uint32_t bsp_dev_id, uint8_t *write_buffer,
					       uint32_t write_length, uint8_t *read_buffer,
					       uint32_t read_length, bsp_callback_t cb,
					       void *cb_arg)
{
	(void)cb;
	(void)cb_arg;

	MSPPHooks_I2CCon_Status sts = MSPPHooks_I2CCon_RegRead(
		&CL_I2C0, (uint16_t)bsp_dev_id, write_buffer, write_length, read_buffer,
		read_length, BSP_I2C_TIMEOUT_MS, true, NULL, NULL);

	return (sts == MSPP_HOOKS_I2CCON_STS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

static uint32_t cl_mcu_i2c_write(uint32_t bsp_dev_id, uint8_t *write_buffer, uint32_t write_length,
				 bsp_callback_t cb, void *cb_arg)
{
	(void)cb;
	(void)cb_arg;

	MSPPHooks_I2CCon_Status sts = MSPPHooks_I2CCon_TXBytes(
		&CL_I2C0, (uint16_t)bsp_dev_id, write_buffer, write_length, BSP_I2C_TIMEOUT_MS,
		MSPP_HOOKS_I2CCON_XFBEGIN_END, NULL, NULL);

	return (sts == MSPP_HOOKS_I2CCON_STS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

static uint32_t cl_mcu_i2c_db_write(uint32_t bsp_dev_id, uint8_t *write_buffer_0,
				    uint32_t write_length_0, uint8_t *write_buffer_1,
				    uint32_t write_length_1, bsp_callback_t cb, void *cb_arg)
{
	(void)cb;
	(void)cb_arg;

	MSPPHooks_I2CCon_Status sts = MSPPHooks_I2CCon_RegWrite(
		&CL_I2C0, (uint16_t)bsp_dev_id, write_buffer_0, write_length_0, write_buffer_1,
		write_length_1, BSP_I2C_TIMEOUT_MS, false, NULL, NULL);

	return (sts == MSPP_HOOKS_I2CCON_STS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

static uint32_t cl_mcu_enable_irq(void)
{
	__enable_irq();
	return BSP_STATUS_OK;
}

static uint32_t cl_mcu_disable_irq(void)
{
	__disable_irq();
	return BSP_STATUS_OK;
}

static bsp_driver_if_t cl_mcu_bsp_driver_if = {
	.set_gpio = cl_mcu_set_gpio,
	.set_supply = NULL,
	.register_gpio_cb = cl_mcu_register_gpio_cb,
	.set_timer = cl_mcu_set_timer,
	.i2c_reset = cl_mcu_i2c_reset,
	.i2c_read_repeated_start = cl_mcu_i2c_read_repeated_start,
	.i2c_write = cl_mcu_i2c_write,
	.i2c_db_write = cl_mcu_i2c_db_write,
	.spi_read = NULL,
	.spi_write = NULL,
	.enable_irq = cl_mcu_enable_irq,
	.disable_irq = cl_mcu_disable_irq,
	.spi_throttle_speed = NULL,
	.spi_restore_speed = NULL,
};

bsp_driver_if_t *bsp_driver_if_g = &cl_mcu_bsp_driver_if;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS - helpers
 **********************************************************************************************************************/

static void bsp_notification_callback(uint32_t event_flags, void *arg)
{
	(void)event_flags;
	(void)arg;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

uint32_t bsp_dut_initialize(void)
{
	uint32_t audio_status;
	cs35l56_config_t audio_config;

	memset(&audio_config, 0, sizeof(cs35l56_config_t));

	audio_status = cs35l56_initialize(&cs35l56_driver);
	if (audio_status == CS35L56_STATUS_OK) {
		audio_config.bsp_config.reset_gpio_id = BSP_GPIO_ID_DUT0_RST;
		audio_config.bsp_config.int_gpio_id = 0;
		audio_config.bsp_config.notification_cb = &bsp_notification_callback;
		audio_config.bsp_config.notification_cb_arg = NULL;
		audio_config.bsp_config.cp_config.dev_id = BSP_DUT_DEV_ID_0;
		audio_config.bsp_config.cp_config.bus_type = REGMAP_BUS_TYPE_I2C;
		audio_config.bsp_config.cp_config.receive_max = 0;
		audio_config.is_ext_bst = true;

		audio_status = cs35l56_configure(&cs35l56_driver, &audio_config);
	}

	return (audio_status == CS35L56_STATUS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

uint32_t bsp_dut_reset(void)
{
	uint32_t ret;

	ret = cs35l56_reset(&cs35l56_driver);
	if (ret != CS35L56_STATUS_OK) {
		return BSP_STATUS_FAIL;
	}

	ret = cs35l56_timeout_ticks_set(&cs35l56_driver, 500);

	return (ret == CS35L56_STATUS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

uint32_t bsp_dut_process(void)
{
	uint32_t ret = cs35l56_process(&cs35l56_driver);

	return (ret == CS35L56_STATUS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

uint32_t bsp_dut_boot(void)
{
	uint32_t ret;

	ret = cs35l56_boot(&cs35l56_driver, NULL);
	if (ret != CS35L56_STATUS_OK) {
		return BSP_STATUS_FAIL;
	}

	ret = cs35l56_set_asp_enable(&cs35l56_driver, true,
				     AUDIO_PCM_RATE_48K * AUDIO_PCM_WIDTH_32_BITS * 2);
	if (ret != CS35L56_STATUS_OK) {
		return BSP_STATUS_FAIL;
	}

	return bsp_dut_timeout_ticks_set(1000);
}

uint32_t bsp_dut_calibrate(void)
{
	uint32_t ret = cs35l56_calibrate(&cs35l56_driver);

	return (ret == CS35L56_STATUS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

uint32_t bsp_dut_timeout_ticks_set(uint32_t ms)
{
	uint32_t ret = cs35l56_timeout_ticks_set(&cs35l56_driver, ms);

	return (ret == CS35L56_STATUS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

uint32_t bsp_dut_hibernate(void)
{
	uint32_t ret = cs35l56_power(&cs35l56_driver, CS35L56_POWER_HIBERNATE);

	return (ret == CS35L56_STATUS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

uint32_t bsp_dut_wake(void)
{
	uint32_t ret = cs35l56_power(&cs35l56_driver, CS35L56_POWER_WAKE);

	return (ret == CS35L56_STATUS_OK) ? BSP_STATUS_OK : BSP_STATUS_FAIL;
}

uint32_t bsp_start_streaming(void)
{
	return regmap_write(REGMAP_GET_CP(&cs35l56_driver), CS35L56_DSP_VIRTUAL1_MBOX_1,
			    CS35L56_DSP_MBOX_CMD_PLAY);
}

uint32_t bsp_pause_streaming(void)
{
	return regmap_write(REGMAP_GET_CP(&cs35l56_driver), CS35L56_DSP_VIRTUAL1_MBOX_1,
			    CS35L56_DSP_MBOX_CMD_PAUSE);
}
