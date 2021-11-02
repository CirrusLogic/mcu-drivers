/**
 * @file bsp_cs47l15.c
 *
 * @brief Implementation of the BSP for the cs47l15 platform.
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
#include <stdlib.h>
#include "platform_bsp.h"
#include "cs47l15.h"
#include "cs47l15_ext.h"
#include "cs47l15_syscfg_regs.h"
#include "cs47l15_fw_img.h"
#include "mp3_test_01_441.h"
#include "mp3_test_01_48.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

void cs47l15_notification_callback(uint32_t event_flags, void *arg);

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs47l15_t cs47l15_driver;
static fw_img_boot_state_t boot_state;

static void * lin_buf_ptr;
static uint8_t * mp3_data;
static uint32_t mp3_data_len;
static uint32_t bytes_written_total;
static bool start_decoding_flag = false;

dsp_buffer_t buffer;

static cs47l15_bsp_config_t bsp_config =
{
    .bsp_reset_gpio_id = BSP_GPIO_ID_DUT_CDC_RESET,
    .bsp_dcvdd_supply_id = BSP_SUPPLY_ID_LN2_DCVDD,
    .bsp_int_gpio_id = BSP_GPIO_ID_DUT_CDC_INT,
    .notification_cb = &cs47l15_notification_callback,
    .notification_cb_arg = NULL,
    .cp_config.dev_id = BSP_DUT_DEV_ID,
    .cp_config.bus_type = REGMAP_BUS_TYPE_SPI_3000,
    .cp_config.spi_pad_len = 2,
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
bool bsp_write_process_done = false;
bool dsp_decoder_interrupt_flag = false;

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_initialize(void)
{
    uint32_t ret = BSP_STATUS_OK;

    cs47l15_config_t codec_config;

    memset(&codec_config, 0, sizeof(cs47l15_config_t));

    // Initialize chip drivers
    ret = cs47l15_initialize(&cs47l15_driver);
    if (ret == CS47L15_STATUS_OK)
    {
        codec_config.bsp_config = bsp_config;

        codec_config.syscfg_regs = cs47l15_syscfg_regs;
        codec_config.syscfg_regs_total = CS47L15_SYSCFG_REGS_TOTAL;

        ret = cs47l15_configure(&cs47l15_driver, &codec_config);
    }

    if (ret != CS47L15_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    // Enable MCLK1 24.576MHz clock routing to CS47L15
    uint32_t temp_buffer = __builtin_bswap32(0x001E8007);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Enable MCLK2 32kHz clock routing to CS47L15
    temp_buffer = __builtin_bswap32(0x001F8003);
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

    bsp_set_timer(2000, NULL, NULL);

    return ret;
}

uint32_t bsp_dut_reset()
{
    uint32_t ret;

    ret = cs47l15_reset(&cs47l15_driver);

    if (ret != CS47L15_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    // Configure FLLs
    // refclk
    ret = cs47l15_fll_config(&cs47l15_driver,
                            CS47L15_FLL1_REFCLK,
                            CS47L15_FLL_SRC_MCLK2,
                            32768,
                            98304000);
    if (ret != CS47L15_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }
    // syncclk
    ret = cs47l15_fll_config(&cs47l15_driver,
                            CS47L15_FLL1_SYNCCLK,
                            CS47L15_FLL_SRC_MCLK1,
                            24576000,
                            98304000);
    if (ret != CS47L15_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_boot()
{
    uint32_t ret;
    const uint8_t *fw_img;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    fw_img = cs47l15_fw_img;
    fw_img_end = cs47l15_fw_img + FW_IMG_SIZE(cs47l15_fw_img);

    // Inform the driver that any current firmware is no longer available by passing a NULL
    // fw_info pointer to cs47l15_boot
    ret = cs47l15_boot(&cs47l15_driver, 1, NULL);
    if (ret != CS47L15_STATUS_OK)
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
            ret = cs47l15_write_block(&cs47l15_driver, boot_state.block.block_addr,
                                      boot_state.block_data, boot_state.block.block_size);
            if (ret == CS47L15_STATUS_FAIL)
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
    return cs47l15_boot(&cs47l15_driver, 1, &boot_state.fw_info);
}

uint32_t bsp_dut_use_case(uint32_t use_case)
{
    uint32_t ret = BSP_STATUS_FAIL;
    uint32_t play_stop_address;
    uint32_t buf_symbol;
    uint32_t space_avail;
    uint32_t scratch;
    uint32_t count = 0;

    switch(use_case) {
        case BSP_USE_CASE_TG_HP_EN:
            ret = cs47l15_fll_enable(&cs47l15_driver, CS47L15_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l15_fll_wait_for_lock(&cs47l15_driver, CS47L15_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SYSTEM_CLOCK_1, CS47L15_SYSCLK_ENA_MASK, CS47L15_SYSCLK_ENA);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1LMIX_INPUT_1_SOURCE, 0x4);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1RMIX_INPUT_1_SOURCE, 0x4);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_TONE_GENERATOR_1, CS47L15_TONE1_ENA);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUTPUT_ENABLES_1, CS47L15_HP1L_ENA | CS47L15_HP1R_ENA);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1L, 0x260);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1R, 0x260);
            break;

        case BSP_USE_CASE_TG_HP_DIS:
            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1R, 0x360);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1L, 0x360);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUTPUT_ENABLES_1, 0);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_TONE_GENERATOR_1, 0x0);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1RMIX_INPUT_1_SOURCE, 0x0);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1LMIX_INPUT_1_SOURCE, 0x0);
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SYSTEM_CLOCK_1, CS47L15_SYSCLK_ENA_MASK, 0);
            ret = cs47l15_fll_disable(&cs47l15_driver, CS47L15_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            break;

        case BSP_USE_CASE_MP3_441K_INIT:
            // Set up clocking
            ret = cs47l15_fll_config(&cs47l15_driver,
                                    CS47L15_FLL1_REFCLK,
                                    CS47L15_FLL_SRC_MCLK2,
                                    32768,
                                    90316800);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l15_fll_enable(&cs47l15_driver, CS47L15_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l15_fll_wait_for_lock(&cs47l15_driver, CS47L15_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SAMPLE_RATE_1, CS47L15_SAMPLE_RATE_1_MASK, 0x0B);
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SYSTEM_CLOCK_1, CS47L15_SYSCLK_SRC_MASK, 0x04);
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SYSTEM_CLOCK_1, CS47L15_SYSCLK_FRAC_MASK, 0x8000);
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SYSTEM_CLOCK_1, CS47L15_SYSCLK_ENA_MASK, CS47L15_SYSCLK_ENA);

            // Boot and load firmware
            ret = cs47l15_power(&cs47l15_driver, 1, CS47L15_POWER_MEM_ENA);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = bsp_dut_boot();
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l15_power(&cs47l15_driver, 1, CS47L15_POWER_UP);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }

            // Set up audio output channels
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1LMIX_INPUT_1_SOURCE, 0x68); // DSP1 channel 1
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1RMIX_INPUT_1_SOURCE, 0x69); // DSP1 channel 2
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUTPUT_ENABLES_1, CS47L15_HP1L_ENA | CS47L15_HP1R_ENA);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1L, 0x290);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1R, 0x290);

            // Init data and dsp buffer
            lin_buf_ptr =  (uint8_t *)bsp_malloc(BSP_DUT_BUFFER_SIZE);
            buf_symbol = cs47l15_find_symbol(&cs47l15_driver, 0, CS47L15_SYM_MP3_DEC_RING_BUFF_ADDRESS);
            ret = cs47l15_dsp_buf_init(&cs47l15_driver, &buffer, lin_buf_ptr, BSP_DUT_BUFFER_SIZE, buf_symbol, 1);
            mp3_data = (uint8_t*)&mp3_test_01_mp3_441[0];
            mp3_data_len = mp3_test_01_mp3_441_len;
            bytes_written_total = 0;

            ret = cs47l15_find_symbol(&cs47l15_driver, 0, CS47L15_SYM_MP3_DEC_PLAY_CONTROL);
            if (ret == 0)
            {
                return BSP_STATUS_FAIL;
            }
            else
            {
                play_stop_address = ret;
                cs47l15_write_reg(&cs47l15_driver, play_stop_address, 0x1); //start playing
            }

            bsp_write_process_done = false;
            start_decoding_flag = true;
            break;

        case BSP_USE_CASE_MP3_48K_INIT:
            // Set up clocking
            ret = cs47l15_fll_config(&cs47l15_driver,
                                    CS47L15_FLL1_REFCLK,
                                    CS47L15_FLL_SRC_MCLK2,
                                    32768,
                                    98304000);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l15_fll_enable(&cs47l15_driver, CS47L15_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l15_fll_wait_for_lock(&cs47l15_driver, CS47L15_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SAMPLE_RATE_1, CS47L15_SAMPLE_RATE_1_MASK, 0x03);
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SYSTEM_CLOCK_1, CS47L15_SYSCLK_SRC_MASK, 0x04);
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SYSTEM_CLOCK_1, CS47L15_SYSCLK_FRAC_MASK, 0x0);
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SYSTEM_CLOCK_1, CS47L15_SYSCLK_ENA_MASK, CS47L15_SYSCLK_ENA);

            // Boot and load firmware
            ret = cs47l15_power(&cs47l15_driver, 1, CS47L15_POWER_MEM_ENA);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = bsp_dut_boot();
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l15_power(&cs47l15_driver, 1, CS47L15_POWER_UP);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }

            // Set up audio output channels
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1LMIX_INPUT_1_SOURCE, 0x68); // DSP1 channel 1
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1RMIX_INPUT_1_SOURCE, 0x69); // DSP1 channel 2
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUTPUT_ENABLES_1, CS47L15_HP1L_ENA | CS47L15_HP1R_ENA);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1L, 0x290);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1R, 0x290);

            // Init data and dsp buffer
            lin_buf_ptr =  (uint8_t *)bsp_malloc(BSP_DUT_BUFFER_SIZE);
            buf_symbol = cs47l15_find_symbol(&cs47l15_driver, 0, CS47L15_SYM_MP3_DEC_RING_BUFF_ADDRESS);
            ret = cs47l15_dsp_buf_init(&cs47l15_driver, &buffer, lin_buf_ptr, BSP_DUT_BUFFER_SIZE, buf_symbol, 1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            mp3_data = (uint8_t*)&mp3_test_01_mp3_48[0];
            mp3_data_len = mp3_test_01_mp3_48_len;
            bytes_written_total = 0;

            ret = cs47l15_find_symbol(&cs47l15_driver, 0, CS47L15_SYM_MP3_DEC_PLAY_CONTROL);
            if (ret == 0)
            {
                return BSP_STATUS_FAIL;
            }
            else
            {
                play_stop_address = ret;
                cs47l15_write_reg(&cs47l15_driver, play_stop_address, 0x1); //start playing
            }

            bsp_write_process_done = false;
            start_decoding_flag = true;
            break;

        case BSP_USE_CASE_MP3_PROCESS:
            // Write data to be played to buffer
            if (dsp_decoder_interrupt_flag)
            {
                ret = cs47l15_dsp_buf_avail(&cs47l15_driver, &buffer, &space_avail);
                if (ret)
                {
                    return BSP_STATUS_FAIL;
                }
                if (space_avail)
                {
                    if (bytes_written_total + space_avail > mp3_data_len)
                    {
                        space_avail = mp3_data_len - bytes_written_total;
                    }
                    ret = cs47l15_dsp_buf_write(&cs47l15_driver, &buffer, mp3_data, space_avail);
                    if (ret) // error
                    {
                        dsp_decoder_interrupt_flag = false;
                        bsp_write_process_done = true;
                        return BSP_STATUS_FAIL;
                    }
                    mp3_data += space_avail;
                    bytes_written_total += space_avail;
                }
                dsp_decoder_interrupt_flag = false;
            }
            if (bytes_written_total >= mp3_data_len)
            {
                cs47l15_dsp_buf_eof(&cs47l15_driver, &buffer);
                bsp_write_process_done = true;
            }
            break;

        case BSP_USE_CASE_MP3_DONE:

            cs47l15_read_reg(&cs47l15_driver, CS47L15_DSP1_SCRATCH_1, &scratch);
            while ((scratch & CS47L15_DSP_SCRATCH_1_MASK) != CS47L15_DSP_DEC_ALGORITHM_STOPPED)
            {
                bsp_set_timer(5, NULL, NULL);
                cs47l15_read_reg(&cs47l15_driver, CS47L15_DSP1_SCRATCH_1, &scratch);
                if (count==10)
                {
                    break;
                }
                count++;
            }

            start_decoding_flag = false;

            ret = cs47l15_find_symbol(&cs47l15_driver, 0, CS47L15_SYM_MP3_DEC_PLAY_CONTROL);
            if (ret == 0)
            {
                cs47l15_fll_disable(&cs47l15_driver, CS47L15_FLL1_REFCLK);
                cs47l15_power(&cs47l15_driver, 1, CS47L15_POWER_DOWN);
                return BSP_STATUS_FAIL;
            }
            else
            {
                play_stop_address = ret;
                cs47l15_write_reg(&cs47l15_driver, play_stop_address, 0x0); //stop playing
            }

            bsp_free(lin_buf_ptr);

            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1R, 0x360);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_DAC_DIGITAL_VOLUME_1L, 0x360);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUTPUT_ENABLES_1, 0);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1RMIX_INPUT_1_SOURCE, 0x0);
            cs47l15_write_reg(&cs47l15_driver, CS47L15_OUT1LMIX_INPUT_1_SOURCE, 0x0);
            cs47l15_update_reg(&cs47l15_driver, CS47L15_SYSTEM_CLOCK_1, CS47L15_SYSCLK_ENA_MASK, 0);
            ret = cs47l15_fll_disable(&cs47l15_driver, CS47L15_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }

            ret = cs47l15_power(&cs47l15_driver, 1, CS47L15_POWER_DOWN);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            break;

        default:
            break;
    }

    return ret;
}

uint32_t bsp_dut_process(void)
{
    uint32_t ret;

    ret = cs47l15_process(&cs47l15_driver);

    if (ret != CS47L15_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

void cs47l15_notification_callback(uint32_t event_flags, void *arg)
{
    if (event_flags & CS47L15_EVENT_FLAG_DSP_IRQ1)
    {
        if (start_decoding_flag)
        {
            dsp_decoder_interrupt_flag = true;
        }
    }
    return;
}
