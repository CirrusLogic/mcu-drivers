/**
 * @file cs40l30.c
 *
 * @brief The CS40L30 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021 All Rights Reserved, http://www.cirrus.com/
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
#include "cs40l30_ext.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

uint32_t cs40l30_trigger(cs40l30_t *driver, uint32_t index)
{
    return cs40l30_write_acked_reg(driver, CS40L30_DSP_VIRTUAL1_MBOX_1_REG, index, 0x0);
}

#ifdef CS40L30_ALGORITHM_BUZZGEN
uint32_t  cs40l30_buzzgen_config(cs40l30_t *driver, uint8_t id, uint8_t freq, uint8_t level, uint32_t duration)
{
    uint32_t ret, freq_sym, level_sym, duration_sym;

    switch (id)
    {
        case 1:
            freq_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS1_BUZZ_FREQ;
            level_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS1_BUZZ_LEVEL;
            duration_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS1_BUZZ_DURATION;
            break;
        case 2:
            freq_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS2_BUZZ_FREQ;
            level_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS2_BUZZ_LEVEL;
            duration_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS2_BUZZ_DURATION;
            break;
        case 3:
            freq_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS3_BUZZ_FREQ;
            level_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS3_BUZZ_LEVEL;
            duration_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS3_BUZZ_DURATION;
            break;
        case 4:
            freq_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS4_BUZZ_FREQ;
            level_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS4_BUZZ_LEVEL;
            duration_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS4_BUZZ_DURATION;
            break;
        case 5:
            freq_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS5_BUZZ_FREQ;
            level_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS5_BUZZ_LEVEL;
            duration_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS5_BUZZ_DURATION;
            break;
        case 6:
            freq_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS6_BUZZ_FREQ;
            level_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS6_BUZZ_LEVEL;
            duration_sym = CS40L30_SYM_BUZZGEN_BUZZ_EFFECTS6_BUZZ_DURATION;
            break;
        default:
            return CS40L30_STATUS_FAIL;
    }

    if (freq < CS40L30_BUZZ_FREQ_MIN)
        freq = CS40L30_BUZZ_FREQ_MIN;
    if (duration > CS40L30_BUZZ_DURATION_MAX)
        duration = CS40L30_BUZZ_DURATION_MAX;

    ret = cs40l30_find_symbol(driver, freq_sym);
    if (!ret)
    {
        return CS40L30_STATUS_FAIL;
    }

    ret = cs40l30_write_reg(driver, ret, freq);
    if (ret)
    {
        return ret;
    }

    ret = cs40l30_find_symbol(driver, level_sym);
    if (!ret)
    {
        return CS40L30_STATUS_FAIL;
    }

    ret = cs40l30_write_reg(driver, ret, level);
    if (ret)
    {
        return ret;
    }

    ret = cs40l30_find_symbol(driver, duration_sym);
    if (!ret)
    {
        return CS40L30_STATUS_FAIL;
    }

    ret = cs40l30_write_reg(driver, ret, duration);
    if (ret)
    {
        return ret;
    }

    return CS40L30_STATUS_OK;
}
#endif //CS40L30_ALGORITHM_BUZZGEN
