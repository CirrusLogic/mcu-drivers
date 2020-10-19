/**
 * @file cs35l41_ext.c
 *
 * @brief The CS35L41 Driver Extended API module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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
    cs35l41_control_request_t req = {0};
    cs35l41_field_accessor_t fa = {0};

    *gain &= (CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITMASK >> CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITOFFSET);

    // Set up Control Request
    req.id = CS35L41_CONTROL_ID_SET_REG;
    req.arg = (void *) &fa;

    fa.address = CS35L41_INTP_AMP_CTRL_REG;
    fa.value = *gain;
    fa.size = CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITWIDTH;
    fa.shift = CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITOFFSET;

    return cs35l41_control(driver, req);
}

/**
 * Get CS35L41 GPIO Level
 *
 */
uint32_t cs35l41_get_gpio(cs35l41_t *driver, cs35l41_gpio_id_t gpio_id, uint32_t *level)
{
    cs35l41_control_request_t req = {0};
    cs35l41_field_accessor_t fa = {0};

    // Check for null pointer
    if (level == NULL)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Set up Control Request
    req.id = CS35L41_CONTROL_ID_GET_REG;
    req.arg = (void *) &fa;

    fa.address = GPIO_STATUS1_REG;
    fa.value = (uint32_t) level;
    fa.size = 1;
    fa.shift = (uint32_t) gpio_id; //GPIO1 is bit 0 of GPIO_STATUS1_REG

    return cs35l41_control(driver, req);
}
