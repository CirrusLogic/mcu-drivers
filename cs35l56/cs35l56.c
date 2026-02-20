/**
 * @file cs35l56.c
 *
 * @brief The CS36L56 Driver module
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
#include <stddef.h>
#include "cs35l56.h"

#ifdef CIRRUS_SDK
#include "bsp_driver_if.h"
#endif

#ifdef CIRRUS_ZEPHYR_SAMPLE
#include "cs35l56_bsp.h"
#endif

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

const struct cs35l56_register_encoding cs35l56_pll_refclk[CS35L56_NUM_VALD_PLL_REFCLKS] = {
    {128000, 0x0C},   {256000, 0x0F},   {384000, 0x11},   {512000, 0x12},   {768000, 0x15},
    {1024000, 0x17},  {1411200, 0x19},  {1500000, 0x1A},  {1536000, 0x1B},  {2000000, 0x1C},
    {2048000, 0x1D},  {2400000, 0x1E},  {2822400, 0x1F},  {3000000, 0x20},  {3072000, 0x21},
    {4000000, 0x23},  {4096000, 0x24},  {4800000, 0x25},  {5644800, 0x26},  {6000000, 0x27},
    {6144000, 0x28},  {6250000, 0x29},  {6400000, 0x2A},  {7526400, 0x2D},  {8000000, 0x2E},
    {8192000, 0x2F},  {9600000, 0x30},  {11289600, 0x31}, {12000000, 0x32}, {12288000, 0x33},
    {13500000, 0x37}, {19200000, 0x38}, {22579200, 0x39}, {24576000, 0x3B}};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/*
 * Reads the contents of a single register/memory address
 *
 */
uint32_t cs35l56_read_reg(cs35l56_t *driver, uint32_t addr, uint32_t *val)
{
    uint32_t ret;
    const regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_read(cp, addr, val);
    if (ret) {
        return CS35L56_STATUS_FAIL;
    }

    return CS35L56_STATUS_OK;
}

/*
 * Writes the contents of a single register/memory address
 *
 */
uint32_t cs35l56_write_reg(cs35l56_t *driver, uint32_t addr, uint32_t val)
{
    uint32_t ret;
    const regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, addr, val);
    if (ret) {
        return CS35L56_STATUS_FAIL;
    }

    return CS35L56_STATUS_OK;
}

/**
 * Initialize driver state/handle
 *
 */
uint32_t cs35l56_initialize(cs35l56_t *driver)
{
    uint32_t ret = CS35L56_STATUS_FAIL;

    if (NULL != driver) {
        /*
         * The memset() call sets all members to 0, including the following semantics:
         * - 'state' is set to UNCONFIGURED
         */
        memset(driver, 0, sizeof(cs35l56_t));

        ret = CS35L56_STATUS_OK;
    }

    return ret;
}

/**
 * Reset the CS35L56
 *
 */
uint32_t cs35l56_reset(cs35l56_t *driver)
{
    uint32_t ret;
    const regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Drive RESET low for at least T_RLPW (1ms)
    bsp_driver_if_g->set_gpio((uint32_t)driver->config.bsp_config.reset_gpio_id, 0);
    bsp_driver_if_g->set_timer(2, NULL, NULL);
    // Drive RESET high and wait for at least T_IRS (2.2ms)
    bsp_driver_if_g->set_gpio((uint32_t)driver->config.bsp_config.reset_gpio_id, 1);
    bsp_driver_if_g->set_timer(5, NULL, NULL);

    // Read DEVID
    ret = regmap_read(cp, CS35L56_SW_RESET_DEVID_REG, &(driver->devid));
    if (ret) {
        return ret;
    }

    // Read REVID
    ret = regmap_read(cp, CS35L56_SW_RESET_REVID_REG, &(driver->revid));
    if (ret) {
        return ret;
    }

    // Only allow driver to handle REVID B2
    if (driver->revid != CS35L56_REVID_B2) {
        return CS35L56_STATUS_FAIL;
    }

    // Wait for (OTP + ROM) boot complete
    ret = regmap_poll_reg(cp, FIRMWARE_CS35L56_HALO_STATE, CS35L56_DSP_STATE_RUNNING, 10,
                  CS35L56_BOOT_TIMEMOUT_MS);
    if (ret) {
        return ret;
    }

    return ret;
}

/**
 * Finish booting the CS35L56
 *
 */
uint32_t cs35l56_boot(cs35l56_t *driver, fw_img_info_t *fw_info)
{
    uint32_t ret;
    const regmap_cp_config_t *cp = REGMAP_GET_CP(driver);
    driver->fw_info = fw_info;

    ret = regmap_write(cp, CS35L56_DSP_VIRTUAL1_MBOX_1, CS35L56_DSP_MBOX_CMD_SHUTDOWN);
    if (ret) {
        return ret;
    }

    bsp_driver_if_g->set_timer(10, NULL, NULL);

    ret = regmap_poll_reg(cp, CS35L56_PM_STATE_SHUTDOWN, CS35L56_PM_STATE, 10,
                  CS35L56_BOOT_TIMEMOUT_MS);
    if (ret) {
        return ret;
    }

    bsp_driver_if_g->set_timer(10, NULL, NULL);

    return CS35L56_STATUS_OK;
}

/**
 * Enable/Disable Audio Streaming Port
 *
 */
uint32_t cs35l56_set_asp_enable(cs35l56_t *driver, bool enable, uint32_t freq)
{
    uint32_t ret;
    uint8_t pll_refclk_val;
    const regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (!enable) {
        // Disable I2C Config
        ret = regmap_update_reg(cp, CS35L56_BLOCK_ENABLES2,
                    CS35L56_BLOCK_ENABLES2_ASP_EN_MASK, 0);
        if (ret) {
            return ret;
        }
        ret = regmap_write(cp, CS35L56_ASP1_ENABLES, 0);
        if (ret) {
            return ret;
        }
    } else {
        // Check for valid freq
        int i;
        for (i = 0; i < CS35L56_NUM_VALD_PLL_REFCLKS; i++) {
            if (freq == cs35l56_pll_refclk[i].value) {
                pll_refclk_val = cs35l56_pll_refclk[i].code;
                break;
            }
        }
        if (i == CS35L56_NUM_VALD_PLL_REFCLKS) {
            return CS35L56_STATUS_FAIL;
        }
        // Configure for I2S at given frequency
        ret = regmap_write(cp, CS35L56_ASP1_CTRL_1, pll_refclk_val);
        if (ret) {
            return ret;
        }
        ret = regmap_write(cp, CS35L56_ASP1_CTRL_2,
                   (0x40 << CS35L56_ASP1_CTRL_RX_WIDTH_OFFSET) |
                       CS35L56_ASP1_CTRL_2_ASP1_FMT_BCLK_MASK);
        if (ret) {
            return ret;
        }
        ret = regmap_write(cp, CS35L56_BLOCK_ENABLES2, CS35L56_BLOCK_ENABLES2_ASP_EN_MASK);
        if (ret) {
            return ret;
        }
        ret = regmap_write(cp, CS35L56_ASP1_ENABLES,
                   (0x3 << CS35L56_ASP1_ENABLES_RX_SHIFT));
        if (ret) {
            return ret;
        }

        // PLL_OPEN_LOOP must clear before changing REFCLK FREQ
        ret = regmap_update_reg(cp, FIRMWARE_CS35L56_PLL_REFCLK_FREQ,
                    CS35L56_FIRMWARE_PLL_OPEN_LOOP_MASK, 0);
        if (ret) {
            return ret;
        }
        ret = regmap_update_reg(
            cp, FIRMWARE_CS35L56_PLL_REFCLK_FREQ, 0,
            (pll_refclk_val << CS35L56_FIRMWARE_PLL_REFCLK_FREQ_OFFSET) |
                CS35L56_FIRMWARE_PLL_REFCLK_EN_MASK);
        if (ret) {
            return ret;
        }
        ret = regmap_update_reg(cp, FIRMWARE_CS35L56_PLL_REFCLK_FREQ,
                    CS35L56_FIRMWARE_PLL_OPEN_LOOP_MASK, 0);
        if (ret) {
            return ret;
        }

        // Handle Mixer + Mailbox Command for DSP
        ret = regmap_write(cp, CS35L56_DACPCM1_INPUT, CS35L56_DACPCM1_INPUT_DSP1_CH5);
        if (ret) {
            return ret;
        }
    }
    return CS35L56_STATUS_OK;
}
