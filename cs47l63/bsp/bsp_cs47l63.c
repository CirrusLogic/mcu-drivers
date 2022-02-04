/**
 * @file bsp_cs47l63.c
 *
 * @brief Implementation of the BSP for the cs47l63 platform.
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
#include <string.h>
#include "platform_bsp.h"
#include "cs47l63.h"
#include "cs47l63_ext.h"
#include "cs47l63_syscfg_regs.h"
#include "cs47l63_fw_img.h"
#include "bridge.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

#define CS47L63_SRC_TONE_GENERATOR1 (0x4)

#define CS47L63_DSP1_CHANNEL1 (0x100)
#define CS47L63_DSP1_CHANNEL2 (0x101)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs47l63_t cs47l63_driver;
static fw_img_boot_state_t boot_state;

static cs47l63_bsp_config_t bsp_config =
{
    .bsp_reset_gpio_id = BSP_GPIO_ID_DUT_CDC_RESET,
    .bsp_dcvdd_supply_id = BSP_SUPPLY_ID_LN2_DCVDD,
    .bsp_int_gpio_id = BSP_GPIO_ID_DUT_CDC_INT,
    .notification_cb = &bsp_notification_callback,
    .notification_cb_arg = NULL,
    .cp_config.dev_id = BSP_DUT_DEV_ID,
    .cp_config.bus_type = REGMAP_BUS_TYPE_SPI,
    .cp_config.spi_pad_len = 4,
};

#ifdef CONFIG_USE_VREGMAP
static bridge_device_t device_list[] =
{
    {0},    // Placeholder for vregmap entry, initialized in bridge_initialize()
    {
        .bus_i2c_cs_address = 1,
        .device_id_str = "CS47A63",
        .dev_name_str = "CS47A63-2",
        .b.dev_id = BSP_DUT_DEV_ID,
        .b.bus_type = REGMAP_BUS_TYPE_SPI,
        .b.receive_max = BRIDGE_BLOCK_BUFFER_LENGTH_BYTES,
        .b.spi_pad_len = 4
    },
};
#else
static bridge_device_t device_list[] =
{
    {
        .bus_i2c_cs_address = 1,
        .device_id_str = "CS47A63",
        .dev_name_str = "CS47A63-1",
        .b.dev_id = BSP_DUT_DEV_ID,
        .b.bus_type = REGMAP_BUS_TYPE_SPI,
        .b.receive_max = BRIDGE_BLOCK_BUFFER_LENGTH_BYTES,
        .b.spi_pad_len = 4
    },
};
#endif

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

    cs47l63_config_t codec_config;

    memset(&codec_config, 0, sizeof(cs47l63_config_t));

    // Initialize chip drivers
    ret = cs47l63_initialize(&cs47l63_driver);
    if (ret == CS47L63_STATUS_OK)
    {
        codec_config.bsp_config = bsp_config;

        codec_config.syscfg_regs = cs47l63_syscfg_regs;
        codec_config.syscfg_regs_total = CS47L63_SYSCFG_REGS_TOTAL;

        ret = cs47l63_configure(&cs47l63_driver, &codec_config);
    }

    if (ret != CS47L63_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    // Enable 32kHz clock routing to CS47L63
    uint32_t temp_buffer = __builtin_bswap32(0x001F8003);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Enable MICVDD at 1v8
    temp_buffer = __builtin_bswap32(0x011B001D);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    temp_buffer = __builtin_bswap32(0x01198000);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Route MICBIAS2 to P2
    temp_buffer = __builtin_bswap32(0x00E40010);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    temp_buffer = __builtin_bswap32(0x00E50100);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    temp_buffer = __builtin_bswap32(0x00E38000);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    cs47l63_wait(2000);

    bridge_initialize(&device_list[0], (sizeof(device_list)/sizeof(bridge_device_t)));

    temp_buffer = __builtin_bswap32(0x00310001);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    return ret;
}

uint32_t bsp_dut_reset(void)
{
    uint32_t ret;

    ret = cs47l63_reset(&cs47l63_driver);

    if (ret != CS47L63_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_boot(void)
{
    uint32_t ret;
    const uint8_t *fw_img;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    fw_img = cs47l63_fw_img;
    fw_img_end = cs47l63_fw_img + FW_IMG_SIZE(cs47l63_fw_img);

    // Inform the driver that any current firmware is no longer available by passing a NULL
    // fw_info pointer to cs47l63_boot
    ret = cs47l63_boot(&cs47l63_driver, 1, NULL);
    if (ret != CS47L63_STATUS_OK)
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
    boot_state.block_data_size = boot_state.fw_info.header.max_block_size;
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
            ret = cs47l63_write_block(&cs47l63_driver, boot_state.block.block_addr,
                                      boot_state.block_data, boot_state.block.block_size);
            if (ret == CS47L63_STATUS_FAIL)
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
    ret = cs47l63_boot(&cs47l63_driver, 1, &boot_state.fw_info);

    bsp_free(boot_state.block_data);
    boot_state.block_data = NULL;

    return ret;
}

uint32_t bsp_dut_use_case(uint32_t use_case)
{
    uint32_t ret = BSP_STATUS_OK;

    switch (use_case) {
        case BSP_USE_CASE_TG_HP_EN:
            cs47l63_update_reg(&cs47l63_driver,
                               CS47L63_DSP_CLOCK1,
                               CS47L63_DSP_CLK_FREQ_MASK,
                               (0x24DD << CS47L63_DSP_CLK_FREQ_SHIFT));
            ret = cs47l63_fll_config(&cs47l63_driver,
                                     CS47L63_FLL1,
                                     CS47L63_FLL_SRC_MCLK2,
                                     32768,
                                     49152000);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l63_fll_enable(&cs47l63_driver, CS47L63_FLL1);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l63_fll_wait_for_lock(&cs47l63_driver, CS47L63_FLL1);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            cs47l63_update_reg(&cs47l63_driver, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN_MASK, CS47L63_SYSCLK_EN);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_OUT1L_INPUT1, CS47L63_OUT1L_SRC1_MASK, CS47L63_SRC_TONE_GENERATOR1);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_TONE_GENERATOR1, CS47L63_TONE1_EN_MASK, CS47L63_TONE1_EN);
            cs47l63_write_reg(&cs47l63_driver, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN_MASK);
            cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1, CS47L63_OUT_VU | 0x60);
            break;
        case BSP_USE_CASE_TG_HP_DIS:
            cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1, CS47L63_OUT_VU | CS47L63_OUT1L_MUTE | 0x60);
            cs47l63_write_reg(&cs47l63_driver, CS47L63_OUTPUT_ENABLE_1, 0);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_TONE_GENERATOR1, CS47L63_TONE1_EN_MASK, 0);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_OUT1L_INPUT1, CS47L63_OUT1L_SRC1_MASK, 0);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN_MASK, 0);
            ret = cs47l63_fll_disable(&cs47l63_driver, CS47L63_FLL1);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            break;
        case BSP_USE_CASE_DSP_PRELOAD_PT_EN:
            ret = cs47l63_power(&cs47l63_driver, 1 , CS47L63_POWER_MEM_ENA);
            bsp_dut_boot();
            break;
        case BSP_USE_CASE_DSP_PRELOAD_PT_DIS:
            ret = cs47l63_power(&cs47l63_driver, 1 , CS47L63_POWER_MEM_DIS);
            break;
        case BSP_USE_CASE_TG_DSP_HP_EN:
            cs47l63_update_reg(&cs47l63_driver,
                               CS47L63_DSP_CLOCK1,
                               CS47L63_DSP_CLK_FREQ_MASK,
                               (0x24DD << CS47L63_DSP_CLK_FREQ_SHIFT));
            ret = cs47l63_fll_config(&cs47l63_driver,
                                     CS47L63_FLL1,
                                     CS47L63_FLL_SRC_INT_OSC,
                                     12288000,
                                     49152000);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l63_fll_enable(&cs47l63_driver, CS47L63_FLL1);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l63_fll_wait_for_lock(&cs47l63_driver, CS47L63_FLL1);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            cs47l63_update_reg(&cs47l63_driver, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN_MASK, CS47L63_SYSCLK_EN);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_DSP1RX1_INPUT1, CS47L63_DSP1RX1_SRC1_MASK, CS47L63_SRC_TONE_GENERATOR1);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_DSP1RX2_INPUT1, CS47L63_DSP1RX2_SRC1_MASK, CS47L63_SRC_TONE_GENERATOR1);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_OUT1L_INPUT1, CS47L63_OUT1L_SRC1_MASK, CS47L63_DSP1_CHANNEL1);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_OUT1L_INPUT2, CS47L63_OUT1L_SRC1_MASK, CS47L63_DSP1_CHANNEL2);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_TONE_GENERATOR1, CS47L63_TONE1_EN_MASK, CS47L63_TONE1_EN);
            cs47l63_write_reg(&cs47l63_driver, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN_MASK);
            cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1, CS47L63_OUT_VU | 0x60);
            ret = cs47l63_power(&cs47l63_driver, 1, CS47L63_POWER_UP);
            break;
        case BSP_USE_CASE_TG_DSP_HP_DIS:
            cs47l63_write_reg(&cs47l63_driver, CS47L63_OUT1L_VOLUME_1, CS47L63_OUT_VU | CS47L63_OUT1L_MUTE | 0x60);
            cs47l63_write_reg(&cs47l63_driver, CS47L63_OUTPUT_ENABLE_1, 0);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_TONE_GENERATOR1, CS47L63_TONE1_EN_MASK, 0);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_OUT1L_INPUT1, CS47L63_OUT1L_SRC1_MASK, 0);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_OUT1L_INPUT2, CS47L63_OUT1L_SRC1_MASK, 0);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_DSP1RX1_INPUT1, CS47L63_DSP1RX1_SRC1_MASK, 0);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_DSP1RX2_INPUT1, CS47L63_DSP1RX2_SRC1_MASK, 0);
            cs47l63_update_reg(&cs47l63_driver, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN_MASK, 0);
            ret = cs47l63_fll_disable(&cs47l63_driver, CS47L63_FLL1);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l63_power(&cs47l63_driver, 1, CS47L63_POWER_DOWN);
            break;
        case BSP_USE_CASE_MIC_DSP_HP_EN:
            ret = cs47l63_power(&cs47l63_driver, 1, CS47L63_POWER_UP);
            break;
        case BSP_USE_CASE_MIC_DSP_HP_DIS:
            ret = cs47l63_power(&cs47l63_driver, 1, CS47L63_POWER_DOWN);
            break;
        default:
            break;
    }

    return ret;
}

uint32_t bsp_dut_process(void)
{
    uint32_t ret;

    ret = cs47l63_process(&cs47l63_driver);

    if (ret != CS47L63_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    bridge_process();

    return BSP_STATUS_OK;
}
