/**
 * @file bsp_cs40l50.c
 *
 * @brief Implementation of the BSP for the cs40l50 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022 All Rights Reserved, http://www.cirrus.com/
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
#include "platform_bsp.h"
#include "cs40l50.h"
#include "cs40l50_ext.h"
#include "cs40l50_syscfg_regs.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs40l50_t cs40l50_driver;
//static fw_img_boot_state_t boot_state;

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

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_boot(void)
{
    return BSP_STATUS_OK;
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

uint32_t bsp_dut_process(void)
{
    uint32_t ret;

    ret = cs40l50_process(&cs40l50_driver);

    if (ret != CS40L50_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_set_click_compensation(bool f0_enable, bool redc_enable)
{
    uint32_t ret;

    ret = cs40l50_set_click_compensation_enable(&cs40l50_driver, f0_enable, redc_enable);

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

uint32_t bsp_dut_trigger_rth_pwle(bool is_simple, rth_pwle_section_t **pwle_data, uint8_t num_sections, uint8_t repeat)
{
    uint32_t ret = BSP_STATUS_OK;
    if (is_simple)
    {
        ret = cs40l50_trigger_pwle(&cs40l50_driver, pwle_data);
    }
    else
    {
        ret = cs40l50_trigger_pwle_advanced(&cs40l50_driver, pwle_data, repeat, num_sections);
    }

    return ret;
}

uint32_t bsp_dut_trigger_rth_pcm(uint8_t *pcm_data, uint32_t num_sections, uint16_t buffer, uint16_t f0, uint16_t redc)
{
    uint32_t ret = BSP_STATUS_OK;

    ret = cs40l50_trigger_pcm(&cs40l50_driver, pcm_data, num_sections, buffer, f0, redc);

    return ret;
}
