/**
 * @file system_test_hw_0_bsp.c
 *
 * @brief Implementation of the BSP for the system_test_hw_0 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
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
#include "hw_0_bsp.h"
#include "cs35l41.h"
#include "cs35l41_ext.h"
#include "cs35l41_fw_img.h"
#include "cs35l41_tune_fw_img.h"
#include "cs35l41_cal_fw_img.h"
#include "test_tone_tables.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs35l41_t cs35l41_driver;
static fw_img_v1_info_t fw_img_info;
static uint8_t transmit_buffer[32];
static uint8_t receive_buffer[256];
static uint32_t bsp_dut_dig_gain = CS35L41_AMP_VOLUME_0DB;

static cs35l41_bsp_config_t bsp_config =
{
    .bsp_dev_id = BSP_DUT_DEV_ID,
    .bsp_reset_gpio_id = BSP_GPIO_ID_DUT_RESET,
    .bsp_int_gpio_id = BSP_GPIO_ID_DUT_INT,
    .bus_type = BSP_BUS_TYPE_I2C,
    .cp_write_buffer = transmit_buffer,
    .cp_read_buffer = receive_buffer,
    .notification_cb = &bsp_notification_callback,
    .notification_cb_arg = NULL
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_write_fw_img(const uint8_t *fw_img, fw_img_v1_info_t *fw_img_info)
{
    uint32_t ret;
    fw_img_boot_state_t boot_state;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    // Ensure your fw_img_boot_state_t struct is initialised to zero.
    memset(&boot_state, 0, sizeof(fw_img_boot_state_t));

    // Get pointer to end of this fw_img
    fw_img_end = fw_img + FW_IMG_SIZE(fw_img);

    // Emulate a system where only 1k fw_img blocks can be processed at a time
    write_size = 1024;

    // Initialise pointer to the currently available fw_img data and set fw_img_blocks_size
    // to the size of fw_img_v1_header_t
    boot_state.fw_img_blocks = (uint8_t *) fw_img;
    boot_state.fw_img_blocks_size = sizeof(fw_img_v1_header_t);

    // Get pointers to buffers for Symbol and Algorithm list
    if (fw_img_info != NULL)
    {
        boot_state.fw_info = *fw_img_info;
    }

    // Read in the fw_img header
    ret = fw_img_read_header(&boot_state);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    // Increment fw_img pointer to skip header
    fw_img += sizeof(fw_img_v1_header_t);

    // Update the fw_img pointer and size in cs35l41_boot_state_t to start transferring data blocks
    boot_state.fw_img_blocks = (uint8_t *) fw_img;
    boot_state.fw_img_blocks_size = write_size;

    // Finally malloc enough memory to hold the largest data block in the fw_img being processed.
    // This may have been configured during fw_img creation.
    // If your control interface has specific memory requirements (dma-able, etc), then this memory
    // should adhere to them.
    boot_state.block_data_size = CS35L41_CONTROL_PORT_MAX_PAYLOAD_BYTES;
    boot_state.block_data = (uint8_t *) bsp_malloc(boot_state.block_data_size);
    if (boot_state.block_data == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    while (fw_img < fw_img_end)
    {
        // Start processing the rest of the fw_img
        ret = fw_img_process(&boot_state);
        if (ret == FW_IMG_STATUS_DATA_READY)
        {
            // Data is ready to be sent to the device, so pass it to the driver
            ret = cs35l41_write_block(&cs35l41_driver, boot_state.block.block_addr,
                                      boot_state.block_data, boot_state.block.block_size);
            if (ret == CS35L41_STATUS_FAIL)
            {
                ret = BSP_STATUS_FAIL;
                break;
            }
            // There is still more data in this fw_img block, so don't provide new data
            continue;
        }
        if (ret == FW_IMG_STATUS_FAIL)
        {
            ret = BSP_STATUS_FAIL;
            break;
        }

        // This fw_img block has been processed, so fetch the next block.
        // In this example, we just increment the pointer.
        fw_img += write_size;

        if (ret == FW_IMG_STATUS_NODATA)
        {
            if (fw_img_end - fw_img < write_size)
            {
                write_size = fw_img_end - fw_img;
            }

            boot_state.fw_img_blocks = (uint8_t *) fw_img;
            boot_state.fw_img_blocks_size = write_size;
        }
    }

    if ((fw_img_info != NULL) && (ret != BSP_STATUS_FAIL))
    {
        *fw_img_info = boot_state.fw_info;
    }

    if (boot_state.block_data)
        bsp_free(boot_state.block_data);

    return ret;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_initialize(void)
{
    uint32_t ret = BSP_STATUS_OK;
    cs35l41_config_t amp_config;

    memset(&amp_config, 0, sizeof(cs35l41_config_t));

    // Initialize chip drivers
    ret = cs35l41_initialize(&cs35l41_driver);
    if (ret == CS35L41_STATUS_OK)
    {
        amp_config.bsp_config = bsp_config;

        amp_config.syscfg_regs = cs35l41_syscfg_regs;
        amp_config.syscfg_regs_total = CS35L41_SYSCFG_REGS_TOTAL;

        amp_config.cal_data.is_valid = false;

        ret = cs35l41_configure(&cs35l41_driver, &amp_config);
    }

    if (ret != CS35L41_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    return ret;
}

uint32_t bsp_dut_reset(void)
{
    uint32_t ret;

    ret = cs35l41_reset(&cs35l41_driver);

    if (ret != CS35L41_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_boot(bool cal_boot)
{
    uint32_t ret;
    const uint8_t *fw_img;
    const uint8_t *tune_img;

    fw_img = cs35l41_fw_img;

    if (!cal_boot)
    {
        tune_img = cs35l41_tune_fw_img;
    }
    else
    {
        tune_img = cs35l41_cal_fw_img;
    }
    cs35l41_driver.is_cal_boot = cal_boot;

    // Inform the driver that any current firmware is no longer available by passing a NULL
    // fw_info pointer to cs35l41_boot
    ret = cs35l41_boot(&cs35l41_driver, NULL);
    if (ret != CS35L41_STATUS_OK)
    {
        return ret;
    }

    // Free anything malloc'ed in previous boots
    if (fw_img_info.sym_table)
        bsp_free(fw_img_info.sym_table);
    if (fw_img_info.alg_id_list)
        bsp_free(fw_img_info.alg_id_list);

    // Ensure your fw_img_boot_state_t struct is initialised to zero.
    memset(&fw_img_info, 0, sizeof(fw_img_v1_info_t));

    // Read in fw_img header to get sizes of Symbol ID and Algo List tables
    fw_img_boot_state_t temp_boot_state = {0};

    // Initialise pointer to the currently available fw_img data and set fw_img_blocks_size
    // to the size of fw_img_v1_header_t
    temp_boot_state.fw_img_blocks = (uint8_t *) fw_img;
    temp_boot_state.fw_img_blocks_size = sizeof(fw_img_v1_header_t);

    // Read in the fw_img header
    ret = fw_img_read_header(&temp_boot_state);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }
    fw_img_info.header = temp_boot_state.fw_info.header;

    // malloc enough memory to hold the symbol table, using sym_table_size in the previously
    // read in fw_img header
    fw_img_info.sym_table = (fw_img_v1_sym_table_t *)bsp_malloc(fw_img_info.header.sym_table_size *
                                                                   sizeof(fw_img_v1_sym_table_t));
    if (fw_img_info.sym_table == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    // malloc enough memory to hold the alg_id list, using the alg_id_list_size in the fw_img header
    fw_img_info.alg_id_list = (uint32_t *) bsp_malloc(fw_img_info.header.alg_id_list_size * sizeof(uint32_t));
    if (fw_img_info.alg_id_list == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    bsp_dut_write_fw_img(fw_img, &fw_img_info);
    bsp_dut_write_fw_img(tune_img, NULL);

    // fw_img processing is complete, so inform the driver and pass it the fw_info block
    ret = cs35l41_boot(&cs35l41_driver, &fw_img_info);

    return ret;
}

uint32_t bsp_dut_calibrate(void)
{
    if (CS35L41_STATUS_OK == cs35l41_calibrate(&cs35l41_driver, 23))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_power_up(void)
{
    if (CS35L41_STATUS_OK == cs35l41_power(&cs35l41_driver, CS35L41_POWER_UP))
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
    if (CS35L41_STATUS_OK == cs35l41_power(&cs35l41_driver, CS35L41_POWER_DOWN))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_get_id(uint8_t *id)
{
    uint32_t id_gpi_level;

    if (id == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    if (CS35L41_STATUS_OK != cs35l41_get_gpio(&cs35l41_driver, GPIO1_ID, &id_gpi_level))
    {
        return BSP_STATUS_FAIL;
    }
    else
    {
        *id = (uint8_t) id_gpi_level;
        return BSP_STATUS_OK;
    }
}

uint32_t bsp_dut_set_dig_gain(float gain_db)
{
    int16_t gain_int;

    // Convert dB to digital value - check range
    if ((gain_db < -102) || (gain_db > 12))
    {
        return BSP_STATUS_FAIL;
    }
    gain_db *= 8;
    gain_int = (int16_t) gain_db;

    // Save volume level
    bsp_dut_dig_gain = (uint32_t) gain_int;

    if (CS35L41_STATUS_OK == cs35l41_set_dig_gain(&cs35l41_driver, &bsp_dut_dig_gain))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_FAIL;
}

uint32_t bsp_dut_mute(bool is_mute)
{
    uint32_t temp_gain;

    if (is_mute)
    {
        temp_gain = CS35L41_AMP_VOLUME_MUTE;
    }
    else
    {
        temp_gain = bsp_dut_dig_gain;
    }

    if (CS35L41_STATUS_OK == cs35l41_set_dig_gain(&cs35l41_driver, &temp_gain))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_is_processing(bool *is_processing)
{
    uint32_t ret;
    cs35l41_dsp_status_t status;
    cs35l41_control_request_t req = {0};

    status.is_calibration_applied = false;
    status.is_hb_inc = false;
    status.is_temp_changed = false;

    req.id = CS35L41_CONTROL_ID_GET_DSP_STATUS;
    req.arg = &status;

    ret = cs35l41_control(&cs35l41_driver, req);

    *is_processing = status.is_hb_inc;

    if (CS35L41_STATUS_OK == ret)
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
    if (CS35L41_STATUS_OK == cs35l41_power(&cs35l41_driver, CS35L41_POWER_HIBERNATE))
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_wake(void)
{
    if (CS35L41_STATUS_OK == cs35l41_power(&cs35l41_driver, CS35L41_POWER_WAKE))
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
    if (CS35L41_STATUS_OK == cs35l41_process(&cs35l41_driver))
    {
        return BSP_STATUS_FAIL;
    }
    else
    {
        return BSP_STATUS_OK;
    }
}
