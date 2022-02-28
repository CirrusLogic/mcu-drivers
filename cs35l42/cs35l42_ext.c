/**
 * @file cs35l42_ext.c
 *
 * @brief The CS35L42 Driver Extended API module
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
#include "cs35l42_ext.h"

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
uint32_t cs35l42_set_dig_gain(cs35l42_t *driver, uint32_t *gain)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    *gain <<= CS35L42_AMP_VOL_PCM_SHIFT;
    *gain &= CS35L42_AMP_VOL_PCM_MASK;

    ret = regmap_update_reg(cp,
                            CS35L42_AMP_CTRL,
                            CS35L42_AMP_VOL_PCM_MASK,
                            *gain);

    return ret;
}
