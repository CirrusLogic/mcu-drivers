/**
 * @file bsp_cs40l26_export.c
 *
 * @brief Implementation of the export BSP for the cs40l26 platform.
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

#include <string.h>
#include <stdlib.h>
#include "platform_bsp.h"
#include "cs40l26.h"
#include "bsp_cs40l26_export.h"
#include "cs40l26_firmware.h"

extern cs40l26_t cs40l26_driver;

//Map symbol headers from fw_img_v2 to symbols generated in export format
static const fw_img_v1_sym_table_t cs40l26_sym_table[] =
{
    { CS40L26_SYM_PM_POWER_ON_SEQUENCE,               PM_POWER_ON_SEQUENCE },
    { CS40L26_SYM_PM_PM_TIMER_TIMEOUT_TICKS,          PM_PM_TIMER_TIMEOUT_TICKS },
    { CS40L26_SYM_PM_PM_CUR_STATE,                    PM_PM_CUR_STATE },
    { CS40L26_SYM_MAILBOX_QUEUE_RD,                   MAILBOX_QUEUE_RD },
    { CS40L26_SYM_MAILBOX_QUEUE_WT,                   MAILBOX_QUEUE_WT },
    { CS40L26_SYM_A2H_A2HEN,                          A2H_A2HEN },
    { CS40L26_SYM_VIBEGEN_COMPENSATION_ENABLE,        VIBEGEN_COMPENSATION_ENABLE },
    { CS40L26_SYM_VIBEGEN_OWT_SIZE_XM,                VIBEGEN_OWT_SIZE_XM },
    { CS40L26_SYM_VIBEGEN_OWT_NEXT_XM,                VIBEGEN_OWT_NEXT_XM },
    { CS40L26_SYM_BUZZGEN_BUZZ_EFFECTS1_BUZZ_FREQ,    BUZZGEN_BUZZ_EFFECTS1_BUZZ_FREQ },
    { CS40L26_SYM_DYNAMIC_F0_DYNAMIC_F0_ENABLED,      DYNAMIC_F0_DYNAMIC_F0_ENABLED },
    { CS40L26_SYM_DYNAMIC_F0_IMONRINGPPTHRESHOLD,     DYNAMIC_F0_IMONRINGPPTHRESHOLD },
    { CS40L26_SYM_DYNAMIC_F0_FRME_SKIP,               DYNAMIC_F0_FRME_SKIP },
    { CS40L26_SYM_DYNAMIC_F0_NUM_PEAKS_TOFIND,        DYNAMIC_F0_NUM_PEAKS_TOFIND },
    { CS40L26_SYM_FW_RAM_EXT_GPI_PMIC_MUTE_ENABLE,    FW_RAM_EXT_GPI_PMIC_MUTE_ENABLE },
};

static fw_img_info_t cs40l26_fw_info =
{
    .preheader = { 0, 2 },
    .header = { 0, sizeof(cs40l26_sym_table) / sizeof(fw_img_v1_sym_table_t), 0, 0, 1, 0, 0, 0 },
    .sym_table = (fw_img_v1_sym_table_t *) cs40l26_sym_table,
    .alg_id_list = NULL,
};

static uint32_t bsp_write_fw_blocks(halo_boot_block_t *blocks, int num_blocks)
{
    int i;
    int ret;
    halo_boot_block_t block;
    uint32_t bytes;
    uint32_t address;
    uint8_t *buffer;

    for (i = 0; i < num_blocks; i++)
    {
        block = blocks[i];
        bytes = block.block_size;
        address = block.address;
        buffer = (uint8_t *)block.bytes;
        ret = regmap_write_block((&cs40l26_driver.config.bsp_config.cp_config), address, buffer, bytes);
        if (ret != 0)
        {
            return ret;
        }
    }

    return 0;
}

uint32_t bsp_dut_boot_export(bool cal_boot)
{
    uint32_t ret;
    static fw_img_boot_state_t boot_state;

    if (cal_boot) // Cal fw loading currently not supported for export format
    {
        return BSP_STATUS_OK;
    }

    else
    {
        uint32_t num_blocks;
        halo_boot_block_t *blocks;
        memset(&boot_state, 0, sizeof(fw_img_boot_state_t));

        cs40l26_driver.is_cal_boot = false;

        // Inform the driver that any current firmware is no longer available by passing a NULL
        // fw_info pointer to cs40l26_boot
        ret = cs40l26_boot(&cs40l26_driver, NULL);
        if (ret != CS40L26_STATUS_OK)
        {
            return ret;
        }

        /* NOTE: Example tunings and wavetable are not designed for general-purpose applications and should
         * be used for verification purposes only. Please contact your Cirrus Logic representative for
         * application specific waveforms and tunings
         */
        num_blocks = cs40l26_total_fw_blocks;
        blocks = (halo_boot_block_t *)cs40l26_fw_blocks;
        ret = bsp_write_fw_blocks(blocks, num_blocks);
        if (ret != 0)
        {
            return ret;
        }

        // Write wavetable coefficients
        num_blocks = cs40l26_wt_total_coeff_blocks;
        blocks = (halo_boot_block_t *)cs40l26_wt_coeff_blocks;
        ret = bsp_write_fw_blocks(blocks, num_blocks);
        if (ret != 0)
        {
            return ret;
        }

        // Write audio2haptics coefficients
        num_blocks = cs40l26_A2H_total_coeff_blocks;
        blocks = (halo_boot_block_t *)cs40l26_A2H_coeff_blocks;
        ret = bsp_write_fw_blocks(blocks, num_blocks);
        if (ret != 0)
        {
            return ret;
        }

        // Write DVL coefficients
        num_blocks = cs40l26_DVL_total_coeff_blocks;
        blocks = (halo_boot_block_t *)cs40l26_DVL_coeff_blocks;
        ret = bsp_write_fw_blocks(blocks, num_blocks);
        if (ret != 0)
        {
            return ret;
        }
    }

    // Manually provide fw info symbols to driver boot functions since export format doesn't generate this
    return cs40l26_boot(&cs40l26_driver, &cs40l26_fw_info);
}

// WT loading is handled in bsp_dut_boot for export format.
uint32_t bsp_dut_load_wavetable_export(void)
{
    return BSP_STATUS_OK;
}

// Only allow calibrate with fw_img_v2 firmware.
uint32_t bsp_dut_calibrate_export(void)
{
    return BSP_STATUS_OK;
}
