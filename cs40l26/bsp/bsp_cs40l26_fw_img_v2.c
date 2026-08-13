/**
 * @file bsp_cs40l26_fw_img_v2.c
 *
 * @brief Implementation of the fw_img_v2 BSP for the cs40l26 platform.
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
#include "bsp_cs40l26_fw_img_v2.h"
#include "cs40l26_fw_img.h"
#include "cs40l26_waveform.h"
#include "cs40l26_cal_fw_img.h"

extern cs40l26_t cs40l26_driver;
extern uint32_t current_halo_heartbeat;

static fw_img_boot_state_t boot_state;
static fw_img_boot_state_t wt_boot_state;

uint32_t bsp_dut_boot_fw_img_v2(bool cal_boot)
{
    uint32_t ret;
    const uint8_t *fw_img;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    /* NOTE: Example tunings and wavetable are not designed for general-purpose applications and should
     * be used for verification purposes only. Please contact your Cirrus Logic representative for
     * application specific waveforms and tunings
     */
    if (cal_boot)
    {
        cs40l26_driver.is_cal_boot = true;
        fw_img = cs40l26_cal_fw_img;
        fw_img_end = cs40l26_cal_fw_img + FW_IMG_SIZE(cs40l26_cal_fw_img);
    }
    else
    {
        cs40l26_driver.is_cal_boot = false;
        fw_img = cs40l26_fw_img;
        fw_img_end = cs40l26_fw_img + FW_IMG_SIZE(cs40l26_fw_img);
    }

    // Inform the driver that any current firmware is no longer available by passing a NULL fw_info pointer.
    ret = cs40l26_boot(&cs40l26_driver, NULL);
    if (ret != CS40L26_STATUS_OK)
    {
        return ret;
    }

    // Free anything malloc'ed in previous boots.
    if (boot_state.fw_info.sym_table)
    {
        free(boot_state.fw_info.sym_table);
    }
    if (boot_state.fw_info.alg_id_list)
    {
        free(boot_state.fw_info.alg_id_list);
    }
    if (boot_state.block_data)
    {
        free(boot_state.block_data);
    }

    // Ensure boot state is initialized to zero.
    memset(&boot_state, 0, sizeof(fw_img_boot_state_t));

    // Emulate a system where only 1k fw_img blocks can be processed at a time.
    write_size = 1024;

    // Initialize pointer to the currently available fw_img data.
    boot_state.fw_img_blocks = (uint8_t *)fw_img;
    boot_state.fw_img_blocks_size = write_size;

    // Read in the fw_img header.
    ret = fw_img_read_header(&boot_state);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    // Allocate memory for the symbol table.
    boot_state.fw_info.sym_table = (fw_img_v1_sym_table_t *)malloc(boot_state.fw_info.header.sym_table_size *
                                                                    sizeof(fw_img_v1_sym_table_t));
    if (boot_state.fw_info.sym_table == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    if (cal_boot)
    {
        if (boot_state.fw_info.header.fw_version < CS40L26_CAL_MIN_FW_VERSION &&
            boot_state.fw_info.header.fw_version != CS40L26_WT_ONLY)
        {
            return BSP_STATUS_FAIL;
        }
    }
    else
    {
        if (boot_state.fw_info.header.fw_version < CS40L26_MIN_FW_VERSION &&
            boot_state.fw_info.header.fw_version != CS40L26_WT_ONLY)
        {
            return BSP_STATUS_FAIL;
        }
    }

    // Allocate memory for the alg_id list.
    boot_state.fw_info.alg_id_list = (uint32_t *)malloc(boot_state.fw_info.header.alg_id_list_size * sizeof(uint32_t));
    if (boot_state.fw_info.alg_id_list == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    // Allocate memory for the largest data block in the fw_img being processed.
    if (boot_state.fw_info.preheader.img_format_rev == 1)
    {
        boot_state.block_data_size = 4140;
    }
    else
    {
        boot_state.block_data_size = boot_state.fw_info.header.max_block_size;
    }
    boot_state.block_data = (uint8_t *)malloc(boot_state.block_data_size);
    if (boot_state.block_data == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    while (fw_img < fw_img_end)
    {
        // Start processing the rest of the fw_img.
        ret = fw_img_process(&boot_state);
        if (ret == FW_IMG_STATUS_DATA_READY)
        {
            // Data is ready to be sent to the device.
            ret = regmap_write_block((&cs40l26_driver.config.bsp_config.cp_config),
                                     boot_state.block.block_addr,
                                     boot_state.block_data,
                                     boot_state.block.block_size);
            if (ret == CS40L26_STATUS_FAIL)
            {
                return BSP_STATUS_FAIL;
            }
            // There is still more data in this fw_img block, so do not provide new data.
            continue;
        }
        if (ret == FW_IMG_STATUS_FAIL)
        {
            return BSP_STATUS_FAIL;
        }

        // This fw_img block has been processed, so fetch the next block.
        fw_img += write_size;

        if (ret == FW_IMG_STATUS_NODATA)
        {
            if (fw_img_end - fw_img < write_size)
            {
                write_size = fw_img_end - fw_img;
            }

            boot_state.fw_img_blocks = (uint8_t *)fw_img;
            boot_state.fw_img_blocks_size = write_size;
        }
    }

    // fw_img processing is complete, so inform the driver and pass fw_info.
    ret = cs40l26_boot(&cs40l26_driver, &boot_state.fw_info);

    current_halo_heartbeat = 0;

    return ret;
}

uint32_t bsp_dut_load_wavetable_fw_img_v2(void)
{
    uint32_t ret;
    const uint8_t *waveform;
    const uint8_t *waveform_end;
    uint32_t write_size;

    waveform = cs40l26_waveform;
    waveform_end = cs40l26_waveform + FW_IMG_SIZE(cs40l26_waveform);

    // Free anything malloc'ed in previous boots.
    if (wt_boot_state.fw_info.sym_table)
    {
        free(wt_boot_state.fw_info.sym_table);
    }
    if (wt_boot_state.fw_info.alg_id_list)
    {
        free(wt_boot_state.fw_info.alg_id_list);
    }
    if (wt_boot_state.block_data)
    {
        free(wt_boot_state.block_data);
    }

    // Ensure waveform state is initialized to zero.
    memset(&wt_boot_state, 0, sizeof(fw_img_boot_state_t));

    // Emulate a system where only 1k fw_img blocks can be processed at a time.
    write_size = 1024;

    // Initialize pointer to the currently available fw_img data.
    wt_boot_state.fw_img_blocks = (uint8_t *)waveform;
    wt_boot_state.fw_img_blocks_size = write_size;

    // Read in the fw_img header.
    ret = fw_img_read_header(&wt_boot_state);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    // Allocate memory for the symbol table.
    wt_boot_state.fw_info.sym_table = (fw_img_v1_sym_table_t *)malloc(wt_boot_state.fw_info.header.sym_table_size *
                                                                       sizeof(fw_img_v1_sym_table_t));
    if (wt_boot_state.fw_info.sym_table == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    // Allocate memory for the alg_id list.
    wt_boot_state.fw_info.alg_id_list = (uint32_t *)malloc(wt_boot_state.fw_info.header.alg_id_list_size * sizeof(uint32_t));
    if (wt_boot_state.fw_info.alg_id_list == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    // Allocate memory for the largest data block in the fw_img being processed.
    if (wt_boot_state.fw_info.preheader.img_format_rev == 1)
    {
        wt_boot_state.block_data_size = 4140;
    }
    else
    {
        wt_boot_state.block_data_size = wt_boot_state.fw_info.header.max_block_size;
    }
    wt_boot_state.block_data = (uint8_t *)malloc(wt_boot_state.block_data_size);
    if (wt_boot_state.block_data == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    while (waveform < waveform_end)
    {
        // Start processing the rest of the fw_img.
        ret = fw_img_process(&wt_boot_state);
        if (ret == FW_IMG_STATUS_DATA_READY)
        {
            // Data is ready to be sent to the device.
            ret = regmap_write_block((&cs40l26_driver.config.bsp_config.cp_config),
                                     wt_boot_state.block.block_addr,
                                     wt_boot_state.block_data,
                                     wt_boot_state.block.block_size);
            if (ret == CS40L26_STATUS_FAIL)
            {
                return BSP_STATUS_FAIL;
            }
            // There is still more data in this fw_img block, so do not provide new data.
            continue;
        }
        if (ret == FW_IMG_STATUS_FAIL)
        {
            return BSP_STATUS_FAIL;
        }

        // This fw_img block has been processed, so fetch the next block.
        waveform += write_size;

        if (ret == FW_IMG_STATUS_NODATA)
        {
            if (waveform_end - waveform < write_size)
            {
                write_size = waveform_end - waveform;
            }

            wt_boot_state.fw_img_blocks = (uint8_t *)waveform;
            wt_boot_state.fw_img_blocks_size = write_size;
        }
    }

    ret = cs40l26_load_waveform(&cs40l26_driver);

    return ret;
}

uint32_t bsp_dut_calibrate_fw_img_v2(void)
{
    uint32_t ret;

    ret = cs40l26_calibrate(&cs40l26_driver);

    if (ret == CS40L26_STATUS_OK)
    {
        return BSP_STATUS_OK;
    }

    return BSP_STATUS_FAIL;
}

