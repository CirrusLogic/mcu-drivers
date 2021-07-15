/**
 * @file cs35l41_ext.c
 *
 * @brief The CS35L41 Driver Extended API module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020-2021 All Rights Reserved, http://www.cirrus.com/
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
#include "cs35l41_ext.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Set HW Digital Gain
 *
 */
uint32_t cs35l41_set_dig_gain(cs35l41_t *driver, uint32_t *gain)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    *gain <<= CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITOFFSET;
    *gain &= CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITMASK;

    ret = regmap_update_reg(cp,
                            CS35L41_INTP_AMP_CTRL_REG,
                            CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITMASK,
                            *gain);

    return ret;
}

/**
 * Configure CS35L41 GPIO Direction
 *
 */
uint32_t cs35l41_config_gpio(cs35l41_t *driver, cs35l41_gpio_id_t gpio_id, bool is_output)
{
    uint32_t gpio_ctrl1_addr = 0;
    cs35l41_gpio_ctrl1_t ctrl;
    uint32_t ret = CS35L41_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Get GPIOx_CTRL1 register
    switch (gpio_id)
    {
        case GPIO1_ID:
            gpio_ctrl1_addr = GPIO_GPIO1_CTRL1_REG;
            break;

        case GPIO2_ID:
            gpio_ctrl1_addr = GPIO_GPIO2_CTRL1_REG;
            break;

        case GPIO3_ID:
            gpio_ctrl1_addr = GPIO_GPIO3_CTRL1_REG;
            break;

        case GPIO4_ID:
            gpio_ctrl1_addr = GPIO_GPIO4_CTRL1_REG;
            break;

        default:
            break;
    }

    ret = regmap_read(cp, gpio_ctrl1_addr, &(ctrl.word));
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    if (is_output)
    {
        ctrl.gp_dir = 0;
    }
    else
    {
        ctrl.gp_dir = 1;
    }

    ret = regmap_write(cp, gpio_ctrl1_addr, ctrl.word);

    return ret;
}

/**
 * Set CS35L41 GPIO Level
 *
 */
uint32_t cs35l41_set_gpio(cs35l41_t *driver, cs35l41_gpio_id_t gpio_id, bool is_high)
{
    uint32_t gpio_ctrl1_addr = 0;
    cs35l41_gpio_ctrl1_t ctrl;
    uint32_t ret = CS35L41_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Get GPIOx_CTRL1 register
    switch (gpio_id)
    {
        case GPIO1_ID:
            gpio_ctrl1_addr = GPIO_GPIO1_CTRL1_REG;
            break;

        case GPIO2_ID:
            gpio_ctrl1_addr = GPIO_GPIO2_CTRL1_REG;
            break;

        case GPIO3_ID:
            gpio_ctrl1_addr = GPIO_GPIO3_CTRL1_REG;
            break;

        case GPIO4_ID:
            gpio_ctrl1_addr = GPIO_GPIO4_CTRL1_REG;
            break;

        default:
            break;
    }

    ret = regmap_read(cp, gpio_ctrl1_addr, &(ctrl.word));
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    if (is_high)
    {
        ctrl.gp_lvl = 0;
    }
    else
    {
        ctrl.gp_lvl = 1;
    }

    ret = regmap_write(cp, gpio_ctrl1_addr, ctrl.word);

    return ret;
}

/**
 * Get CS35L41 GPIO Level
 *
 */
uint32_t cs35l41_get_gpio(cs35l41_t *driver, cs35l41_gpio_id_t gpio_id, uint32_t *level)
{
    uint32_t val;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Check for null pointer
    if (level == NULL)
    {
        return CS35L41_STATUS_FAIL;
    }

    if (regmap_read(cp, GPIO_STATUS1_REG, &(val)))
    {
        return CS35L41_STATUS_FAIL;
    }

    *level = 0;
    if (val & (1 << gpio_id))
    {
        *level = 1;
    }

    return CS35L41_STATUS_OK;
}
