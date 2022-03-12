/**
 * @file bsp_cs35l41.c
 *
 * @brief Implementation of the BSP for the cs35l41 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2022 All Rights Reserved, http://www.cirrus.com/
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
#include "cs35l41.h"
#include "cs35l41_ext.h"
#include "cs35l41_fw_img.h"
#include "cs35l41_tune_fw_img.h"
#include "cs35l41_tune_48_fw_img.h"
#include "cs35l41_tune_44p1_fw_img.h"
#include "cs35l41_cal_fw_img.h"
#include "test_tone_tables.h"
#include "cs35l41_fs_switch_syscfg.h"
#include "bridge.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs35l41_t cs35l41_driver;
static fw_img_info_t fw_img_info;
static uint32_t bsp_dut_dig_gain = CS35L42_AMP_VOL_PCM_0DB;

static cs35l41_bsp_config_t bsp_config =
{
#ifdef USE_CS35L41_SPI
    .cp_config.dev_id = BSP_DUT_DEV_ID_SPI2,
#else
    .cp_config.dev_id = BSP_DUT_DEV_ID,
#endif
    .reset_gpio_id = BSP_GPIO_ID_DUT_DSP_RESET,
    .int_gpio_id = BSP_GPIO_ID_DUT_DSP_INT,
#ifdef USE_CS35L41_SPI
    .cp_config.bus_type = REGMAP_BUS_TYPE_SPI,
#else
    .cp_config.bus_type = REGMAP_BUS_TYPE_I2C,
#endif
    .cp_config.receive_max = CS35L41_OTP_SIZE_BYTES,
    .cp_config.spi_pad_len = 2,
    .notification_cb = &bsp_notification_callback,
    .notification_cb_arg = NULL
};

// Below values work for the Left Amp on SPI2 of the Cirrus development card
static bridge_device_t device_list[] =
{
    {
        .bus_i2c_cs_address = 1,
        .device_id_str = "35A40",
        .dev_name_str = "CS35L41-Left",
#ifdef USE_CS35L41_SPI
        .b.dev_id = BSP_DUT_DEV_ID_SPI2,
        .b.bus_type = REGMAP_BUS_TYPE_SPI,
#else
        .b.dev_id = BSP_DUT_DEV_ID,
        .b.bus_type = REGMAP_BUS_TYPE_I2C,
#endif
        .b.receive_max = BRIDGE_BLOCK_BUFFER_LENGTH_BYTES,
        .b.spi_pad_len = 2
     },
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_write_fw_img(const uint8_t *fw_img, fw_img_info_t *fw_img_info)
{
    uint32_t ret;
    fw_img_boot_state_t boot_state;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    if (fw_img == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    // Ensure your fw_img_boot_state_t struct is initialised to zero.
    memset(&boot_state, 0, sizeof(fw_img_boot_state_t));

    // Get pointer to end of this fw_img
    fw_img_end = fw_img + FW_IMG_SIZE(fw_img);

    // Emulate a system where only 1k fw_img blocks can be processed at a time
    write_size = 1024;

    // Update the fw_img pointer and size in cs35l41_boot_state_t to start transferring data blocks
    boot_state.fw_img_blocks = (uint8_t *) fw_img;
    boot_state.fw_img_blocks_size = write_size;

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

    // Finally malloc enough memory to hold the largest data block in the fw_img being processed.
    // This may have been configured during fw_img creation.
    // If your control interface has specific memory requirements (dma-able, etc), then this memory
    // should adhere to them.
    // From fw_img_v2 forward, the max_block_size is stored in the fw_img header itself
    if (boot_state.fw_info.preheader.img_format_rev == 1)
    {
        boot_state.block_data_size = CS35L41_CONTROL_PORT_MAX_PAYLOAD_BYTES;
    }
    else
    {
        boot_state.block_data_size = boot_state.fw_info.header.max_block_size;
    }
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
            ret = regmap_write_block(&(cs35l41_driver.config.bsp_config.cp_config),
                                       boot_state.block.block_addr,
                                       boot_state.block_data,
                                       boot_state.block.block_size);
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

    uint32_t temp_buffer;

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

    if (cs35l41_driver.config.bsp_config.cp_config.bus_type == BSP_BUS_TYPE_SPI)
    {
        // Toggle CIF Master mode to SPI
        temp_buffer = __builtin_bswap32(0x00F00000);
        bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
        // Redirect MCU SPI to SPI2 on LN2, SPI_SS_GF_SPI1_SS2_DST = DSP_GPIO5
        temp_buffer = __builtin_bswap32(0x0240100D);
        bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    }

    bridge_initialize(&device_list[0], (sizeof(device_list)/sizeof(bridge_device_t)));

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
    memset(&fw_img_info, 0, sizeof(fw_img_info_t));

    // Read in fw_img header to get sizes of Symbol ID and Algo List tables
    fw_img_boot_state_t temp_boot_state = {0};

    // Initialise pointer to the currently available fw_img data and set fw_img_blocks_size
    // to the size of fw_img_v1_header_t
    temp_boot_state.fw_img_blocks = (uint8_t *) fw_img;
    temp_boot_state.fw_img_blocks_size = 1024;

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
    if ((gain_db < CS35L42_AMP_VOL_PCM_MIN_DB) || (gain_db > CS35L42_AMP_VOL_PCM_MAX_DB))
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
        temp_gain = CS35L42_AMP_VOL_PCM_MUTE;
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

    status.is_calibration_applied = false;
    status.is_hb_inc = false;
    status.is_temp_changed = false;

    ret = cs35l41_get_dsp_status(&cs35l41_driver, &status);

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

uint32_t bsp_dut_change_fs(uint32_t fs_hz)
{
    uint32_t ret;
    const uint8_t *tune_img;
    const uint32_t *cfg;
    uint16_t cfg_length;

    // Validate Fs
    if (fs_hz == 48000)
    {
        tune_img = cs35l41_tune_48_fw_img;
        cfg = cs35l41_fs_48kHz_syscfg;
        cfg_length = CS35L41_FS_48KHZ_SYSCFG_REGS_TOTAL;
    }
    else if (fs_hz == 44100)
    {
        tune_img = cs35l41_tune_44p1_fw_img;
        cfg = cs35l41_fs_44p1kHz_syscfg;
        cfg_length = CS35L41_FS_44P1KHZ_SYSCFG_REGS_TOTAL;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }

    ret = cs35l41_start_tuning_switch(&cs35l41_driver);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    ret = cs35l41_send_syscfg(&cs35l41_driver, cfg, cfg_length);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    // Load new Fs tuning
    bsp_dut_write_fw_img(tune_img, NULL);

    ret = cs35l41_finish_tuning_switch(&cs35l41_driver);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_process(void)
{
    bridge_process();

    if (CS35L41_STATUS_OK == cs35l41_process(&cs35l41_driver))
    {
        return BSP_STATUS_FAIL;
    }
    else
    {
        return BSP_STATUS_OK;
    }
}
