/**
 * @file bsp_cs35l42.c
 *
 * @brief Implementation of the BSP for the cs35l42 platform.
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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "platform_bsp.h"
#include "cs35l42.h"
#include "cs35l42_ext.h"
#include "test_tone_tables.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs35l42_t cs35l42_driver;
static uint32_t bsp_dut_dig_gain = CS35L42_AMP_VOL_PCM_0DB;

static cs35l42_bsp_config_t bsp_config =
{
    .cp_config.dev_id = BSP_DUT_DEV_ID,
    .reset_gpio_id = BSP_GPIO_ID_DUT_DSP_RESET,
    .int_gpio_id = BSP_GPIO_ID_DUT_DSP_INT,
    .cp_config.bus_type = REGMAP_BUS_TYPE_I2C,
    .cp_config.receive_max = CS35L42_OTP_SIZE_BYTES,
    .cp_config.spi_pad_len = 2,
    .notification_cb = &bsp_notification_callback,
    .notification_cb_arg = NULL
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
    cs35l42_config_t amp_config;

    memset(&amp_config, 0, sizeof(cs35l42_config_t));

    // Initialize chip drivers
    ret = cs35l42_initialize(&cs35l42_driver);
    if (ret == CS35L42_STATUS_OK)
    {
        amp_config.bsp_config = bsp_config;

        amp_config.syscfg_regs = cs35l42_syscfg_regs;
        amp_config.syscfg_regs_total = CS35L42_SYSCFG_REGS_TOTAL;

        ret = cs35l42_configure(&cs35l42_driver, &amp_config);
    }

    if (ret != CS35L42_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    // Set MCLK2 to 12.288Mhz and Enable
    uint32_t temp_buffer = __builtin_bswap32(0x001F8005);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Configure Codec AIF1 source to be GF AIF1
    temp_buffer = __builtin_bswap32(0x000DE00B);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Configure GF AIF1 source to Codec AIF1
    temp_buffer = __builtin_bswap32(0x00168004);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    // DSP_GPIO3 (AMP_L_RST) source set to Channel 1
    temp_buffer = __builtin_bswap32(0x00410001);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Channel 1 source set to GF_GPIO1 (PC_1)
    temp_buffer = __builtin_bswap32(0x00B90018);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    return ret;
}

uint32_t bsp_dut_reset(void)
{
    uint32_t ret;

    ret = cs35l42_reset(&cs35l42_driver);

    if (ret != CS35L42_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_boot(bool cal_boot)
{
    uint32_t ret = BSP_STATUS_OK;
    return ret;
}

uint32_t bsp_dut_power_up(void)
{
    if (CS35L42_STATUS_OK == cs35l42_power(&cs35l42_driver, CS35L42_POWER_UP))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_power_down(void)
{
    if (CS35L42_STATUS_OK == cs35l42_power(&cs35l42_driver, CS35L42_POWER_DOWN))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_hibernate(void)
{
    uint32_t ret = BSP_STATUS_OK;
    return ret;
}

uint32_t bsp_dut_set_dig_gain(float gain_db)
{
    int16_t gain_int;

    // Convert dB to digital value - check range
    if ((gain_db < CS35L42_AMP_VOL_PCM_MIN_DB) || (gain_db > CS35L42_AMP_VOL_PCM_MAX_DB))
    {
        return BSP_STATUS_FAIL;
    }
    gain_db *= 8;
    gain_int = (int16_t) gain_db;

    // Save volume level
    bsp_dut_dig_gain = (uint32_t) gain_int;

    if (CS35L42_STATUS_OK == cs35l42_set_dig_gain(&cs35l42_driver, &bsp_dut_dig_gain))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_FAIL;
}

uint32_t bsp_dut_process(void)
{
    if (CS35L42_STATUS_OK == cs35l42_process(&cs35l42_driver))
    {
        return BSP_STATUS_FAIL;
    }
    else
    {
        return BSP_STATUS_OK;
    }
}
