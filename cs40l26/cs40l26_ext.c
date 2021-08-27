/**
 * @file cs40l25_ext.c
 *
 * @brief The CS40L25 Driver Extended API module
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
#include <stddef.h>
#include "cs40l26_ext.h"
#include "bsp_driver_if.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/**
 * Total entries in Dynamic F0 table
 */
#define CS40L26_DYNAMIC_F0_TABLE_SIZE           (20)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Enable the HALO FW Dynamic F0 Algorithm
 *
 */
uint32_t cs40l26_set_dynamic_f0_enable(cs40l26_t *driver, bool enable)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, CS40L26_DYNAMIC_F0_ENABLED, enable);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, CS40L26_DYNAMIC_F0_IMONRINGPPTHRESHOLD, 0x20C5);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, CS40L26_DYNAMIC_F0_FRME_SKIP, 0x30);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, CS40L26_DYNAMIC_F0_NUM_PEAKS_TOFIND, 5);
    if (ret)
    {
        return ret;
    }

    return ret;
}

/**
 * Get the Dynamic F0
 *
 */
uint32_t cs40l26_get_dynamic_f0(cs40l26_t *driver, cs40l26_dynamic_f0_table_entry_t *f0_entry)
{
    uint32_t ret, reg_addr;
    cs40l26_dynamic_f0_table_entry_t f0_read;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    uint8_t i;
    reg_addr = CS40L26_DYNAMIC_F0_TABLE;
    for (i = 0; i < CS40L26_DYNAMIC_F0_TABLE_SIZE; i++)
    {
        ret = regmap_read(cp, reg_addr, &(f0_read.word));
        if (ret)
        {
            return ret;
        }

        if (f0_entry->index == f0_read.index)
        {
            f0_entry->f0 = f0_read.f0;
            break;
        }

        reg_addr += 4;
    }

    // Set to default of table entry to indicate index not found
    if (i >= CS40L26_DYNAMIC_F0_TABLE_SIZE)
    {
        f0_entry->word = CS40L26_DYNAMIC_F0_TABLE_ENTRY_DEFAULT;
    }

    return ret;
}
