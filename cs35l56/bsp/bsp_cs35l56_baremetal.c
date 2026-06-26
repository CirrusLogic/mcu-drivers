/**
 * @file bsp_cs35l56.c
 *
 * @brief Implementation of the BSP for the cs35l56 platform.
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
#include <stdlib.h>
#include "platform_bsp.h"
#include "cs35l56.h"

#include "cs35l56_firmware.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs35l56_t cs35l56_driver;

static cs35l56_bsp_config_t bsp_config = {
    .reset_gpio_id = BSP_GPIO_ID_DUT_CDC_RESET,
    .int_gpio_id = BSP_GPIO_ID_DUT_CDC_INT,
    .notification_cb = &bsp_notification_callback,
    .notification_cb_arg = NULL,
    .cp_config.dev_id = BSP_DUT_DEV_ID,
    .cp_config.bus_type = REGMAP_BUS_TYPE_I2C,
    .cp_config.receive_max = 0, // No calls to regmap_read_block for the cs35l56 driver
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_initialize(void)
{
    uint32_t ret = BSP_STATUS_OK;
    uint32_t audio_status;
    cs35l56_config_t audio_config;

    memset(&audio_config, 0, sizeof(cs35l56_config_t));

    // Initialize chip drivers
    audio_status = cs35l56_initialize(&cs35l56_driver);
    if (audio_status == CS35L56_STATUS_OK) {
        audio_config.bsp_config = bsp_config;

        // audio_config.syscfg_regs = cs35l56_syscfg_regs;
        // audio_config.syscfg_regs_total = CS35L56_SYSCFG_REGS_TOTAL;

        audio_config.is_ext_bst = true;

        audio_status = cs35l56_configure(&cs35l56_driver, &audio_config);
    }

    if (audio_status != CS35L56_STATUS_OK) {
        ret = BSP_STATUS_FAIL;
    }
    return ret;
}

uint32_t bsp_dut_reset(void)
{
    uint32_t ret;

    ret = cs35l56_reset(&cs35l56_driver);

    if (ret != CS35L56_STATUS_OK) {
        return BSP_STATUS_FAIL;
    }

    ret = cs35l56_timeout_ticks_set(&cs35l56_driver, 500);

    if (ret != CS35L56_STATUS_OK) {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_process()
{
    uint32_t ret;
    ret = cs35l56_process(&cs35l56_driver);

    if (ret != CS35L56_STATUS_OK) {
        return BSP_STATUS_FAIL;
    }
    return BSP_STATUS_OK;
}

uint32_t bsp_dut_boot(void)
{
    uint32_t ret = BSP_STATUS_OK;
    int i;

    cs35l56_boot(&cs35l56_driver, NULL);

    for (i = 0; i < cs35l56_total_fw_blocks; i++) {
        ret = regmap_write_block(
            (&cs35l56_driver.config.bsp_config.cp_config), cs35l56_fw_blocks[i].address,
            (uint8_t *)cs35l56_fw_blocks[i].bytes, cs35l56_fw_blocks[i].block_size);
    }

    for (i = 0; i < CS35L56_LT_total_coeff_blocks; i++) {
        ret = regmap_write_block((&cs35l56_driver.config.bsp_config.cp_config),
                     CS35L56_LT_coeff_blocks[i].address,
                     (uint8_t *)CS35L56_LT_coeff_blocks[i].bytes,
                     CS35L56_LT_coeff_blocks[i].block_size);
    }

    if (ret == CS35L56_STATUS_FAIL) {
        return BSP_STATUS_FAIL;
    }

    regmap_write((&cs35l56_driver.config.bsp_config.cp_config), CS35L56_DSP1_CCM_CORE_CONTROL,
             0x00000281);

    // Wait for (OTP + ROM) boot complete
    ret = regmap_poll_reg((&cs35l56_driver.config.bsp_config.cp_config),
                  FIRMWARE_CS35L56_HALO_STATE, CS35L56_HALO_STATE_RUNNING, 10,
                  CS35L56_BOOT_TIMEMOUT_MS);
    if (ret == CS35L56_STATUS_FAIL) {
        return BSP_STATUS_FAIL;
    }

    ret = cs35l56_set_asp_enable(&cs35l56_driver, true,
                     AUDIO_PCM_RATE_48K * AUDIO_PCM_WIDTH_32_BITS * 2);
    if (ret) {
        return ret;
    }
    ret = bsp_dut_timeout_ticks_set(1000);

    return ret;
}

uint32_t bsp_dut_calibrate(void)
{
    uint32_t ret;

    ret = cs35l56_calibrate(&cs35l56_driver);

    if (ret == CS35L56_STATUS_OK) {
        return BSP_STATUS_OK;
    } else {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_timeout_ticks_set(uint32_t ms)
{
    uint32_t ret;

    ret = cs35l56_timeout_ticks_set(&cs35l56_driver, ms);
    if (ret != CS35L56_STATUS_OK) {
        return BSP_STATUS_FAIL;
    }
    return BSP_STATUS_OK;
}

uint32_t bsp_start_streaming(void)
{
    uint32_t ret = regmap_write((&cs35l56_driver.config.bsp_config.cp_config),
                    CS35L56_DSP_VIRTUAL1_MBOX_1, CS35L56_DSP_MBOX_CMD_PLAY);
    return ret;
}

uint32_t bsp_pause_streaming(void)
{
    uint32_t ret = regmap_write((&cs35l56_driver.config.bsp_config.cp_config),
                    CS35L56_DSP_VIRTUAL1_MBOX_1, CS35L56_DSP_MBOX_CMD_PAUSE);
    return ret;
}
