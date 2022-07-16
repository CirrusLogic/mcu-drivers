/**
 * @file bsp_cs40l26.c
 *
 * @brief Implementation of the BSP for the cs40l26 platform.
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
#include <string.h>
#include "platform_bsp.h"
#include "cs40l26.h"
#include "cs40l26_ext.h"
#include "cs40l26_syscfg_regs.h"
#include "cs40l26_fw_img.h"
#include "cs40l26_cal_fw_img.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs40l26_t cs40l26_driver;
static fw_img_boot_state_t boot_state;
static uint32_t current_halo_heartbeat = 0;
static cs40l26_dynamic_f0_table_entry_t dynamic_f0;

static cs40l26_bsp_config_t bsp_config =
{
    .cp_config.dev_id = BSP_DUT_DEV_ID,
    .reset_gpio_id = BSP_GPIO_ID_DUT_CDC_RESET,
    .int_gpio_id = BSP_GPIO_ID_DUT_CDC_INT,
    .cp_config.bus_type = REGMAP_BUS_TYPE_I2C,
    .cp_config.receive_max = 0, // No calls to regmap_read_block for the cs40l26 driver
    .notification_cb = &bsp_notification_callback,
    .notification_cb_arg = NULL
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_initialize(void)
{
    uint32_t ret = BSP_STATUS_OK;
    uint32_t haptic_status;
    cs40l26_config_t haptic_config;

    memset(&haptic_config, 0, sizeof(cs40l26_config_t));

    // Initialize chip drivers
    haptic_status = cs40l26_initialize(&cs40l26_driver);
    if (haptic_status == CS40L26_STATUS_OK)
    {
        haptic_config.bsp_config = bsp_config;

        haptic_config.syscfg_regs = cs40l26_syscfg_regs;
        haptic_config.syscfg_regs_total = CS40L26_SYSCFG_REGS_TOTAL;

        haptic_status = cs40l26_configure(&cs40l26_driver, &haptic_config);
    }

    if (haptic_status != CS40L26_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    uint32_t temp_buffer;
    // CODEC_AIF1_CTRL(0DH):    E00D  CDC_AIF1_ENA=Enabled, CDC_AIF1LRCLK_DIR=FPGA Output, CDC_AIF1BCLK_DIR=FPGA Output, CDC_AIF1_HS_ENA=Disabled, CDC_AIF1_HS_STS=Disabled, CDC_AIF1_SRC=USB_AIF_CH_1_8_SRC
    temp_buffer = __builtin_bswap32(0x000DE00D);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // USB_AIF_CTRL1(19H):      9004  USB_AIF_CH_1_8_ENA=Enabled, USB_AIF_CH_1_8_HS_ENA=Enabled, USB_AIF_CH_1_8_HS_STS=Disabled, USB_AIF_CH_1_8_SRC=CDC_AIF1_SRC
    temp_buffer = __builtin_bswap32(0x00199004);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // USB_AIF_CTRL2(1AH):      9010  USB_AIF_CH_9_16_ENA=Enabled, USB_AIF_CH_9_16_HS_ENA=Enabled, USB_AIF_CH_9_16_HS_STS=Disabled, USB_AIF_CH_9_16_SRC=SOUNDCARD_AIF_SRC
    temp_buffer = __builtin_bswap32(0x001A9010);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    return ret;
}

uint32_t bsp_dut_reset()
{
    uint32_t ret;

    ret = cs40l26_reset(&cs40l26_driver);

    if (ret != CS40L26_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    current_halo_heartbeat = 0;

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_boot(bool cal_boot)
{
    uint32_t ret;
    const uint8_t *fw_img;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    if (cal_boot)
    {
        fw_img = cs40l26_cal_fw_img;
        fw_img_end = cs40l26_cal_fw_img + FW_IMG_SIZE(cs40l26_cal_fw_img);
    }
    else
    {
        fw_img = cs40l26_fw_img;
        fw_img_end = cs40l26_fw_img + FW_IMG_SIZE(cs40l26_fw_img);
    }

    // Inform the driver that any current firmware is no longer available by passing a NULL
    // fw_info pointer to cs40l26_boot
    ret = cs40l26_boot(&cs40l26_driver, NULL);
    if (ret != CS40L26_STATUS_OK)
    {
        return ret;
    }

    // Free anything malloc'ed in previous boots
    if (boot_state.fw_info.sym_table)
        bsp_free(boot_state.fw_info.sym_table);
    if (boot_state.fw_info.alg_id_list)
        bsp_free(boot_state.fw_info.alg_id_list);
    if (boot_state.block_data)
        bsp_free(boot_state.block_data);

    // Ensure your fw_img_boot_state_t struct is initialised to zero.
    memset(&boot_state, 0, sizeof(fw_img_boot_state_t));

    // Emulate a system where only 1k fw_img blocks can be processed at a time
    write_size = 1024;

    // Initialise pointer to the currently available fw_img data
    boot_state.fw_img_blocks = (uint8_t *) fw_img;
    boot_state.fw_img_blocks_size = write_size;

    // Read in the fw_img header
    ret = fw_img_read_header(&boot_state);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    // malloc enough memory to hold the symbol table, using sym_table_size in the previously
    // read in fw_img header
    boot_state.fw_info.sym_table = (fw_img_v1_sym_table_t *)bsp_malloc(boot_state.fw_info.header.sym_table_size *
                                                                   sizeof(fw_img_v1_sym_table_t));
    if (boot_state.fw_info.sym_table == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    if (cal_boot)
    {
        if (boot_state.fw_info.header.fw_version < CS40L26_CAL_MIN_FW_VERSION && boot_state.fw_info.header.fw_version != CS40L26_WT_ONLY)
        {
            return BSP_STATUS_FAIL;
        }
    }
    else
    {
        if (boot_state.fw_info.header.fw_version < CS40L26_MIN_FW_VERSION && boot_state.fw_info.header.fw_version != CS40L26_WT_ONLY)
        {
            return BSP_STATUS_FAIL;
        }
    }

    // malloc enough memory to hold the alg_id list, using the alg_id_list_size in the fw_img header
    boot_state.fw_info.alg_id_list = (uint32_t *) bsp_malloc(boot_state.fw_info.header.alg_id_list_size * sizeof(uint32_t));
    if (boot_state.fw_info.alg_id_list == NULL)
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
        boot_state.block_data_size = 4140;
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
          ret = regmap_write_block((&cs40l26_driver.config.bsp_config.cp_config),
                                     boot_state.block.block_addr,
                                     boot_state.block_data,
                                     boot_state.block.block_size);
            if (ret == CS40L26_STATUS_FAIL)
            {
                return BSP_STATUS_FAIL;
            }
            // There is still more data in this fw_img block, so don't provide new data
            continue;
        }
        if (ret == FW_IMG_STATUS_FAIL)
        {
            return BSP_STATUS_FAIL;
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

    // fw_img processing is complete, so inform the driver and pass it the fw_info block
    ret = cs40l26_boot(&cs40l26_driver, &boot_state.fw_info);

    current_halo_heartbeat = 0;

    return ret;
}

uint32_t bsp_dut_calibrate(void)
{
    uint32_t ret;

    ret = cs40l26_calibrate(&cs40l26_driver);

    if (ret == CS40L26_STATUS_OK)
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
    uint32_t ret;

    ret = cs40l26_power(&cs40l26_driver, CS40L26_POWER_STATE_ALLOW_HIBERNATE);

    if (ret == CS40L26_STATUS_OK)
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
    uint32_t ret;

    ret = cs40l26_power(&cs40l26_driver, CS40L26_POWER_STATE_PREVENT_HIBERNATE);

    if (ret == CS40L26_STATUS_OK)
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_enable_haptic_processing(bool enable)
{
    uint32_t ret = BSP_STATUS_OK;

    // Enable Dynamic F0
    ret = cs40l26_set_dynamic_f0_enable(&cs40l26_driver, enable);

    if (ret != CS40L26_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return ret;
}

uint32_t bsp_dut_trigger_haptic(uint8_t waveform, cs40l26_wavetable_bank_t bank)
{
    uint32_t ret = BSP_STATUS_OK;

    ret = cs40l26_trigger(&cs40l26_driver, waveform, bank);

    return ret;
}

uint32_t bsp_dut_buzzgen_set(uint16_t freq, uint16_t level,
                             uint16_t duration, uint8_t buzzgen_num)
{
    uint32_t ret = BSP_STATUS_OK;

    ret = cs40l26_buzzgen_set(&cs40l26_driver, freq, level, duration, buzzgen_num);

    return ret;
}

uint32_t bsp_dut_trigger_rth_pwle(bool is_simple, rth_pwle_section_t **pwle_data, uint8_t num_sections, uint8_t repeat)
{
    uint32_t ret = BSP_STATUS_OK;
    if (is_simple)
    {
        ret = cs40l26_trigger_pwle(&cs40l26_driver, pwle_data);
    }
    else
    {
        ret = cs40l26_trigger_pwle_advanced(&cs40l26_driver, pwle_data, repeat, num_sections);
    }

    return ret;
}

uint32_t bsp_dut_trigger_rth_pcm(uint8_t *pcm_data, uint32_t num_sections, uint16_t buffer, uint16_t f0, uint16_t redc)
{
    uint32_t ret = BSP_STATUS_OK;

    ret = cs40l26_trigger_pcm(&cs40l26_driver, pcm_data, num_sections, buffer, f0, redc);

    return ret;
}

uint32_t bsp_dut_dynamic_calibrate(uint8_t index)
{
    uint32_t ret = BSP_STATUS_OK;

    // Read Dynamic F0 from WT Index 0
    dynamic_f0.index = index;
    ret = cs40l26_get_dynamic_f0(&cs40l26_driver, &dynamic_f0);
    if (ret != CS40L26_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return ret;
}

uint32_t bsp_dut_process(void)
{
    uint32_t ret;

    ret = cs40l26_process(&cs40l26_driver);

    if (ret != CS40L26_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}
