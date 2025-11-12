/**
 * @file bsp_cs40l50.c
 *
 * @brief Implementation of the BSP for the cs40l50 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2025 All Rights Reserved, http://www.cirrus.com/
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
#include "cs40l50.h"
#include "cs40l50_syscfg_regs.h"

#ifndef CS40L50_BAREMETAL
#include "cs40l50_fw_img.h"
static fw_img_boot_state_t boot_state;
static uint32_t current_halo_heartbeat = 0;
#else
#include "cs40l50_firmware.h"
#endif

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs40l50_t cs40l50_driver;


static cs40l50_bsp_config_t bsp_config =
{
    .reset_gpio_id = BSP_GPIO_ID_DUT_CDC_RESET,
    .int_gpio_id = BSP_GPIO_ID_DUT_CDC_INT,
    .notification_cb = &bsp_notification_callback,
    .notification_cb_arg = NULL,
    .cp_config.dev_id = BSP_DUT_DEV_ID,
    .cp_config.bus_type = REGMAP_BUS_TYPE_I2C,
    .cp_config.receive_max = 0, // No calls to regmap_read_block for the cs40l50 driver
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
    uint32_t haptic_status;
    cs40l50_config_t haptic_config;

    memset(&haptic_config, 0, sizeof(cs40l50_config_t));

    // Initialize chip drivers
    haptic_status = cs40l50_initialize(&cs40l50_driver);
    if (haptic_status == CS40L50_STATUS_OK)
    {
        haptic_config.bsp_config = bsp_config;

        haptic_config.syscfg_regs = cs40l50_syscfg_regs;
        haptic_config.syscfg_regs_total = CS40L50_SYSCFG_REGS_TOTAL;

        haptic_config.is_ext_bst = true;

        haptic_config.dynamic_f0_threshold = 0x20C5;

        haptic_status = cs40l50_configure(&cs40l50_driver, &haptic_config);

    }

    if (haptic_status != CS40L50_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    uint32_t temp_buffer;

    // Configure Codec AIF1 source to be GF AIF1
    temp_buffer = __builtin_bswap32(0x000DE00B);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    // Configure GF AIF1 source to Codec AIF1
    temp_buffer = __builtin_bswap32(0x00168004);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    // CDC_MCLK1_ENA=Enabled, CDC_MCLK1_SRC=CLK_24.576MHz
    temp_buffer = __builtin_bswap32(0x001E8007);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    // Set CDC_GPIO1 to GND for S1/S2 functionality
    // CDC_GPIO1 source set to Channel 1
    temp_buffer = __builtin_bswap32(0x00370001);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Channel 1 source set to Logic 0
    temp_buffer = __builtin_bswap32(0x00B900FE);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    return ret;
}

uint32_t bsp_dut_reset(void)
{
    uint32_t ret;

    ret = cs40l50_reset(&cs40l50_driver);

    if (ret != CS40L50_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    ret = cs40l50_timeout_ticks_set(&cs40l50_driver, 500);

    if (ret != CS40L50_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}



uint32_t bsp_dut_boot(void)
{
    uint32_t ret;
    int i;
    for (i = 0; i < cs40l50_total_fw_blocks; i++) {
        ret = regmap_write_block((&cs40l50_driver.config.bsp_config.cp_config),
                                 cs40l50_fw_blocks[i].address,
                                 (uint8_t *)cs40l50_fw_blocks[i].bytes,
                                 cs40l50_fw_blocks[i].block_size);
        if (ret == CS40L50_STATUS_FAIL)
        {
            return BSP_STATUS_FAIL;
        }
    }

    for (i = 0; i < cs40l50_SVC_A_total_coeff_blocks; i++) {
        ret = regmap_write_block((&cs40l50_driver.config.bsp_config.cp_config),
                                cs40l50_SVC_A_coeff_blocks[i].address,
                                (uint8_t *)cs40l50_SVC_A_coeff_blocks[i].bytes,
                                cs40l50_SVC_A_coeff_blocks[i].block_size);
        if (ret == CS40L50_STATUS_FAIL)
        {
            return BSP_STATUS_FAIL;
        }
    }

    for (i = 0; i < cs40l50_WT_A_total_coeff_blocks; i++) {
        ret = regmap_write_block((&cs40l50_driver.config.bsp_config.cp_config),
                                cs40l50_WT_A_coeff_blocks[i].address,
                                (uint8_t *)cs40l50_WT_A_coeff_blocks[i].bytes,
                                cs40l50_WT_A_coeff_blocks[i].block_size);
        if (ret == CS40L50_STATUS_FAIL)
        {
            return BSP_STATUS_FAIL;
        }
    }

    regmap_write((&cs40l50_driver.config.bsp_config.cp_config), CS40L50_DSP1_CCM_CORE_CONTROL, 0x00000281);

    return ret;
}

uint32_t bsp_dut_calibrate(void)
{
    uint32_t ret;

    ret = cs40l50_calibrate(&cs40l50_driver);

    if (ret == CS40L50_STATUS_OK)
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_timeout_ticks_set(uint32_t ms)
{
    uint32_t ret;

    ret = cs40l50_timeout_ticks_set(&cs40l50_driver, ms);
    if (ret != CS40L50_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }
    return BSP_STATUS_OK;
}

uint32_t bsp_dut_trigger_haptic(uint8_t waveform, cs40l50_wavetable_bank_t bank)
{
    uint32_t ret = BSP_STATUS_OK;

    ret = cs40l50_trigger(&cs40l50_driver, waveform, bank);

    return ret;
}
