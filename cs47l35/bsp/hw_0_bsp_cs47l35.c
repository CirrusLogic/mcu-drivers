/**
 * @file system_test_hw_0_bsp.c
 *
 * @brief Implementation of the BSP for the system_test_hw_0 platform.
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
#include "hw_0_bsp.h"
#include "cs47l35.h"
#include "cs47l35_ext.h"
#include "cs47l35_syscfg_regs.h"
#include "cs47l35_dsp2_fw_img.h"
#include "cs47l35_dsp3_fw_img.h"
#include "opus_test_01_16.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
void cs47l35_notification_callback(uint32_t event_flags, void *arg);

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs47l35_t cs47l35_driver;
static fw_img_boot_state_t boot_state_dsp2;
static fw_img_boot_state_t boot_state_dsp3;

static void * lin_buf_ptr_dec;
static void * lin_buf_ptr_enc;
static uint8_t * opus_data_ping;
static uint8_t * opus_data_pong;
static uint8_t * opus_data_enc;
static uint8_t * opus_data_dec;
static uint32_t data_avail;
static uint32_t opus_data_len;
static uint32_t bytes_written_total;
static uint32_t bytes_read_total;
static bool start_decoding_flag = false;
static bool start_encoding_flag = false;

static uint32_t vad_symbol;

uint32_t dec_count = 0;

dsp_buffer_t buffer_dec;
dsp_buffer_t buffer_enc;

static cs47l35_bsp_config_t bsp_config =
{
    .bsp_reset_gpio_id = BSP_GPIO_ID_DUT_CDC_RESET,
    .bsp_dcvdd_supply_id = BSP_SUPPLY_ID_LN2_DCVDD,
    .bsp_int_gpio_id = BSP_GPIO_ID_DUT_CDC_INT,
    .notification_cb = &cs47l35_notification_callback,
    .notification_cb_arg = NULL,
    .cp_config.dev_id = BSP_DUT_DEV_ID,
    .cp_config.bus_type = REGMAP_BUS_TYPE_SPI_3000,
    .cp_config.spi_pad_len = 2,
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
bool bsp_write_process_done = false;
bool bsp_read_process_done = false;
bool dsp_decoder_interrupt_flag = false;
bool dsp_encoder_interrupt_flag = false;

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_initialize(void)
{
    uint32_t ret = BSP_STATUS_OK;

    cs47l35_config_t codec_config;

    memset(&codec_config, 0, sizeof(cs47l35_config_t));

    // Initialize chip drivers
    ret = cs47l35_initialize(&cs47l35_driver);
    if (ret == CS47L35_STATUS_OK)
    {
        codec_config.bsp_config = bsp_config;

        codec_config.syscfg_regs = cs47l35_syscfg_regs;
        codec_config.syscfg_regs_total = sizeof(cs47l35_syscfg_regs)/sizeof(uint32_t);

        ret = cs47l35_configure(&cs47l35_driver, &codec_config);
    }

    if (ret != CS47L35_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    // Enable MCLK1 24.576MHz clock routing to CS47L35
    uint32_t temp_buffer = __builtin_bswap32(0x001E8007);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Enable MCLK2 32kHz clock routing to CS47L35
    temp_buffer = __builtin_bswap32(0x001F8003);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    // Bypass LN2 FPGA
    temp_buffer = __builtin_bswap32(0x00EE0000);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Enable MICVDD at 1v8
    temp_buffer = __builtin_bswap32(0x011B001D);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    temp_buffer = __builtin_bswap32(0x01198000);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Route MICBIAS2 to P2
    temp_buffer = __builtin_bswap32(0x00E400D2);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    temp_buffer = __builtin_bswap32(0x00E38000);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    bsp_set_timer(2000, NULL, NULL);

    return ret;
}

uint32_t bsp_dut_reset()
{
    uint32_t ret;

    ret = cs47l35_reset(&cs47l35_driver);

    if (ret != CS47L35_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    // Configure FLLs
    // refclk
    ret = cs47l35_fll_config(&cs47l35_driver,
                            CS47L35_FLL1_REFCLK,
                            CS47L35_FLL_SRC_MCLK2,
                            32768,
                            98304000);
    if (ret != CS47L35_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_boot(uint32_t core_no, const uint8_t *fw_img_ptr, fw_img_boot_state_t *boot_state)
{
    uint32_t ret;
    const uint8_t *fw_img;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    fw_img = fw_img_ptr;
    fw_img_end = fw_img_ptr + FW_IMG_SIZE(fw_img_ptr);

    // Inform the driver that any current firmware is no longer available by passing a NULL
    // fw_info pointer to cs47l35_boot
    ret = cs47l35_boot(&cs47l35_driver, core_no, NULL);
    if (ret != CS47L35_STATUS_OK)
    {
        return ret;
    }

    // Free anything malloc'ed in previous boots
    if (boot_state->fw_info.sym_table)
        bsp_free(boot_state->fw_info.sym_table);
    if (boot_state->fw_info.alg_id_list)
        bsp_free(boot_state->fw_info.alg_id_list);
    if (boot_state->block_data)
        bsp_free(boot_state->block_data);

    // Ensure your fw_img_boot_state_t struct is initialised to zero.
    memset(boot_state, 0, sizeof(fw_img_boot_state_t));

    // Emulate a system where only 1k fw_img blocks can be processed at a time
    write_size = 1024;

    // Initialise pointer to the currently available fw_img data
    boot_state->fw_img_blocks = (uint8_t *) fw_img;
    boot_state->fw_img_blocks_size = write_size;

    // Read in the fw_img header
    ret = fw_img_read_header(boot_state);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    // malloc enough memory to hold the symbol table, using sym_table_size in the previously
    // read in fw_img header
    boot_state->fw_info.sym_table = (fw_img_v1_sym_table_t *)bsp_malloc(boot_state->fw_info.header.sym_table_size *
                                                                   sizeof(fw_img_v1_sym_table_t));
    if (boot_state->fw_info.sym_table == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    // malloc enough memory to hold the alg_id list, using the alg_id_list_size in the fw_img header
    boot_state->fw_info.alg_id_list = (uint32_t *) bsp_malloc(boot_state->fw_info.header.alg_id_list_size * sizeof(uint32_t));
    if (boot_state->fw_info.alg_id_list == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    // Finally malloc enough memory to hold the largest data block in the fw_img being processed.
    // This may have been configured during fw_img creation.
    // If your control interface has specific memory requirements (dma-able, etc), then this memory
    // should adhere to them.
    // From fw_img_v2 forward, the max_block_size is stored in the fw_img header itself
    boot_state->block_data_size = boot_state->fw_info.header.max_block_size;
    boot_state->block_data = (uint8_t *) bsp_malloc(boot_state->block_data_size);
    if (boot_state->block_data == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    while (fw_img < fw_img_end)
    {
        // Start processing the rest of the fw_img
        ret = fw_img_process(boot_state);
        if (ret == FW_IMG_STATUS_DATA_READY)
        {
            // Data is ready to be sent to the device, so pass it to the driver
            ret = cs47l35_write_block(&cs47l35_driver, boot_state->block.block_addr,
                                      boot_state->block_data, boot_state->block.block_size);
            if (ret == CS47L35_STATUS_FAIL)
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

            boot_state->fw_img_blocks = (uint8_t *) fw_img;
            boot_state->fw_img_blocks_size = write_size;
        }
    }

    bsp_free(boot_state->block_data);

    // fw_img processing is complete, so inform the driver and pass it the fw_info block
    return cs47l35_boot(&cs47l35_driver, core_no, &boot_state->fw_info);
}

void bsp_enable_mic()
{
    cs47l35_write_reg(&cs47l35_driver, 0x448, 0x0283); // EDRE_Enable * fix clicking noise *
    cs47l35_write_reg(&cs47l35_driver, CS47L35_LDO2_CONTROL_1, 0x0304);
    cs47l35_write_reg(&cs47l35_driver, CS47L35_IN1L_CONTROL, 0x8480);
    cs47l35_write_reg(&cs47l35_driver, CS47L35_MIC_BIAS_CTRL_1, 0x0067);
    cs47l35_write_reg(&cs47l35_driver, CS47L35_MIC_BIAS_CTRL_5, 0x0033);
    cs47l35_write_reg(&cs47l35_driver, CS47L35_MIC_CHARGE_PUMP_1, 0x7);
    cs47l35_write_reg(&cs47l35_driver, CS47L35_INPUT_ENABLES, 0x2);
    cs47l35_write_reg(&cs47l35_driver, CS47L35_ADC_DIGITAL_VOLUME_1L, 0x280);
}

void bsp_disable_mic()
{
    cs47l35_write_reg(&cs47l35_driver, CS47L35_ADC_DIGITAL_VOLUME_1L, 0x380);
    cs47l35_write_reg(&cs47l35_driver, CS47L35_INPUT_ENABLES, 0x0);
    cs47l35_write_reg(&cs47l35_driver, CS47L35_MIC_BIAS_CTRL_1, 0x81a4);
    cs47l35_write_reg(&cs47l35_driver, CS47L35_MIC_BIAS_CTRL_5, 0x222);
}

uint32_t bsp_dut_use_case(uint32_t use_case)
{
    uint32_t ret = BSP_STATUS_OK;
    uint32_t buf_symbol;
    uint32_t space_avail;
    uint32_t scratch;
    uint32_t vad;
    uint32_t count = 0;

    switch(use_case) {
        case BSP_USE_CASE_TG_HP_EN:
            ret = cs47l35_fll_enable(&cs47l35_driver, CS47L35_FLL1);
            if (ret != CS47L35_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l35_fll_wait_for_lock(&cs47l35_driver, CS47L35_FLL1);
            if (ret != CS47L35_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_ENA_MASK, CS47L35_SYSCLK_ENA);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1LMIX_INPUT_1_SOURCE, 0x10);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1RMIX_INPUT_1_SOURCE, 0x10);
            bsp_enable_mic();
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUTPUT_ENABLES_1, CS47L35_HP1L_ENA | CS47L35_HP1R_ENA);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1L, 0x260);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1R, 0x260);
            break;

        case BSP_USE_CASE_TG_HP_DIS:
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1R, 0x360);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1L, 0x360);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUTPUT_ENABLES_1, 0);
            bsp_disable_mic();
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1RMIX_INPUT_1_SOURCE, 0x0);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1LMIX_INPUT_1_SOURCE, 0x0);
            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_ENA_MASK, 0);
            ret = cs47l35_fll_disable(&cs47l35_driver, CS47L35_FLL1);
            if (ret != CS47L35_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            break;

        case BSP_USE_CASE_OPUS_RECORD_16K_INIT:
            // Set up clocking
            ret = cs47l35_fll_config(&cs47l35_driver,
                                    CS47L35_FLL1_REFCLK,
                                    CS47L35_FLL_SRC_MCLK2,
                                    32768,
                                    98304000);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l35_fll_enable(&cs47l35_driver, CS47L35_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l35_fll_wait_for_lock(&cs47l35_driver, CS47L35_FLL1);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }

            cs47l35_update_reg(&cs47l35_driver, CS47L35_SAMPLE_RATE_1, CS47L35_SAMPLE_RATE_1_MASK, 0x12);
            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_SRC_MASK, 0x04);
            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_FRAC_MASK, 0x0);
            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_ENA_MASK, CS47L35_SYSCLK_ENA);

            cs47l35_update_reg(&cs47l35_driver, CS47L35_DSP_CLOCK_1, CS47L35_DSP_CLK_FREQ_RANGE_MASK | CS47L35_DSP_CLK_SRC_MASK, 0x404);
            cs47l35_update_reg(&cs47l35_driver, CS47L35_DSP_CLOCK_1, CS47L35_DSP_CLK_SRC_MASK, 0x4); //FLL1

            // Set up audio input channels
            bsp_enable_mic();
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DSP3RMIX_INPUT_1_SOURCE, 0x10);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DSP2LMIX_INPUT_1_SOURCE, 0x79);

            // Set up audio output channels
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1LMIX_INPUT_1_SOURCE, 0x70); // DSP2 channel 1
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1RMIX_INPUT_1_SOURCE, 0x70); // DSP2 channel 1

            // Boot and load firmware
            ret = cs47l35_power(&cs47l35_driver, 2, CS47L35_POWER_MEM_ENA);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            bsp_dut_boot(2, cs47l35_dsp2_fw_img, &boot_state_dsp2);

            ret = cs47l35_power(&cs47l35_driver, 3, CS47L35_POWER_MEM_ENA);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            bsp_dut_boot(3, cs47l35_dsp3_fw_img, &boot_state_dsp3);

            ret = cs47l35_power(&cs47l35_driver, 2, CS47L35_POWER_UP);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l35_power(&cs47l35_driver, 3, CS47L35_POWER_UP);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }

            // Enable output
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUTPUT_ENABLES_1, CS47L35_HP1L_ENA | CS47L35_HP1R_ENA);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1L, 0x290);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1R, 0x290);

            // VAD setup
            vad_symbol = cs47l35_find_symbol(&cs47l35_driver, 3, CS47L35_DSP3_SYM_SOUNDCLEAR_RT_SCVAD_TDVADOUTPUT);
            buf_symbol = cs47l35_find_symbol(&cs47l35_driver, 3, CS47L35_DSP3_SYM_SOUNDCLEAR_RT_SCVAD_TDVADMINSPEECHTHRESHSQRTS1);
            cs47l35_write_reg(&cs47l35_driver, buf_symbol, 0x819);
            buf_symbol = cs47l35_find_symbol(&cs47l35_driver, 3, CS47L35_DSP3_SYM_SOUNDCLEAR_RT_WRITEREGID);
            cs47l35_write_reg(&cs47l35_driver, buf_symbol, 0x5);

            // Init data and dsp buffer
            lin_buf_ptr_dec =  (uint8_t *)bsp_malloc(BSP_DUT_BUFFER_SIZE);
            buf_symbol = cs47l35_find_symbol(&cs47l35_driver, 2, CS47L35_DSP2_SYM_SILK_DECODER_RING_BUFF_ADDRESS);
            ret = cs47l35_dsp_buf_init(&cs47l35_driver, &buffer_dec, lin_buf_ptr_dec, BSP_DUT_BUFFER_SIZE, buf_symbol, 2);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }
            bytes_written_total = 0;
            bsp_write_process_done = false;
            start_decoding_flag = true;

            // Init data and dsp buffer
            lin_buf_ptr_enc =  (uint8_t *)bsp_malloc(BSP_DUT_BUFFER_SIZE);
            buf_symbol = cs47l35_find_symbol(&cs47l35_driver, 2, CS47L35_DSP2_SYM_SILK_ENCODER_RING_BUFF_ADDRESS);
            ret = cs47l35_dsp_buf_init(&cs47l35_driver, &buffer_enc, lin_buf_ptr_enc, BSP_DUT_BUFFER_SIZE, buf_symbol, 2);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }

            opus_data_ping = (uint8_t *)bsp_malloc(BSP_DUT_RECORDING_SIZE);
            opus_data_pong = (uint8_t *)bsp_malloc(BSP_DUT_RECORDING_SIZE);
            opus_data_enc = opus_data_ping;
            opus_data_dec = opus_data_ping;
            opus_data_len = 0x8000;
            bytes_read_total = 0;

            bsp_read_process_done = false;
            start_encoding_flag = true;

            data_avail = 0;
            break;

        case BSP_USE_CASE_OPUS_RECORD:
            // Read and play recorded data
            cs47l35_read_reg(&cs47l35_driver, vad_symbol, &vad);
            bsp_driver_if_g->set_gpio(BSP_GPIO_ID_INTP_LED1, vad & 1); // deglitched speech
            bsp_driver_if_g->set_gpio(BSP_GPIO_ID_INTP_LED2, (vad >> 1) & 1); // raw speech

            if (dsp_encoder_interrupt_flag || (bytes_read_total /* encoding has started */ && dsp_decoder_interrupt_flag && !data_avail))
            {
                ret = cs47l35_dsp_buf_data_avail(&cs47l35_driver, &buffer_enc, &data_avail);
                if (ret)
                {
                    dsp_decoder_interrupt_flag = false;
                    dsp_encoder_interrupt_flag = false;
                    bsp_write_process_done = true;
                    bsp_read_process_done = true;
                    return BSP_STATUS_FAIL;
                }
                if (data_avail)
                {
                    ret = cs47l35_dsp_buf_read(&cs47l35_driver, &buffer_enc, opus_data_enc, data_avail);
                    if (ret) // error
                    {
                        dsp_decoder_interrupt_flag = false;
                        dsp_encoder_interrupt_flag = false;
                        bsp_write_process_done = true;
                        bsp_read_process_done = true;
                        return BSP_STATUS_FAIL;
                    }
                    if (opus_data_enc == opus_data_ping)
                        opus_data_enc = opus_data_pong;
                    if (opus_data_enc == opus_data_pong)
                        opus_data_enc = opus_data_ping;
                    bytes_read_total += data_avail;
                    dsp_encoder_interrupt_flag = false;
                }
            }
            if ((bytes_read_total >= opus_data_len) && (bsp_read_process_done == false))
            {
                cs47l35_dsp_buf_eof(&cs47l35_driver, &buffer_enc);
                bsp_read_process_done = true;
            }

            if (dsp_decoder_interrupt_flag && data_avail)
            {
                if (!bytes_written_total)
                {
                    /* Give the encoder a head start */
                    bsp_set_timer(200, NULL, NULL);
                }
                ret = cs47l35_dsp_buf_space_avail(&cs47l35_driver, &buffer_dec, &space_avail);
                if (ret)
                {
                    dsp_decoder_interrupt_flag = false;
                    dsp_encoder_interrupt_flag = false;
                    bsp_write_process_done = true;
                    bsp_read_process_done = true;
                    return BSP_STATUS_FAIL;
                }
                if (space_avail > data_avail)
                {
                    space_avail = data_avail;
                }
                if (space_avail)
                {
                    if (bytes_written_total + space_avail > opus_data_len)
                    {
                        space_avail = opus_data_len - bytes_written_total;
                    }

                    ret = cs47l35_dsp_buf_write(&cs47l35_driver, &buffer_dec, opus_data_dec, space_avail);
                    if (ret) // error
                    {
                        dsp_decoder_interrupt_flag = false;
                        dsp_encoder_interrupt_flag = false;
                        bsp_write_process_done = true;
                        bsp_read_process_done = true;
                        return BSP_STATUS_FAIL;
                    }
                    if (opus_data_dec == opus_data_ping)
                        opus_data_dec = opus_data_pong;
                    if (opus_data_dec == opus_data_pong)
                        opus_data_dec = opus_data_ping;
                    bytes_written_total += space_avail;
                    data_avail = 0;
                    dsp_decoder_interrupt_flag = false;
                }
            }
            if ((bytes_written_total >= opus_data_len) && (bsp_write_process_done == false))
            {
                cs47l35_dsp_buf_eof(&cs47l35_driver, &buffer_dec);
                bsp_write_process_done = true;
            }

            break;

        case BSP_USE_CASE_OPUS_RECORD_DONE:
            bsp_driver_if_g->set_gpio(BSP_GPIO_ID_INTP_LED1, BSP_GPIO_LOW);
            bsp_driver_if_g->set_gpio(BSP_GPIO_ID_INTP_LED2, BSP_GPIO_LOW);
            cs47l35_read_reg(&cs47l35_driver, CS47L35_DSP2_SCRATCH_1, &scratch);
            while ((scratch & CS47L35_DSP_SCRATCH_1_MASK) != CS47L35_DSP_DEC_ALGORITHM_STOPPED)
            {
                bsp_set_timer(5, NULL, NULL);
                cs47l35_read_reg(&cs47l35_driver, CS47L35_DSP2_SCRATCH_1, &scratch);
                if (count==10)
                {
                    break;
                }
                count++;
            }

            start_encoding_flag = false;
            start_decoding_flag = false;
            bsp_free(lin_buf_ptr_dec);
            bsp_free(lin_buf_ptr_enc);
            bsp_free(opus_data_ping);
            bsp_free(opus_data_pong);

            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1R, 0x360);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1L, 0x360);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUTPUT_ENABLES_1, 0);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1RMIX_INPUT_1_SOURCE, 0x0);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1LMIX_INPUT_1_SOURCE, 0x0);
            bsp_disable_mic();

            ret = cs47l35_power(&cs47l35_driver, 3, CS47L35_POWER_DOWN);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }

            ret = cs47l35_power(&cs47l35_driver, 2, CS47L35_POWER_DOWN);
            if (ret)
            {
                return BSP_STATUS_FAIL;
            }

            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_ENA_MASK, 0);
            ret = cs47l35_fll_disable(&cs47l35_driver, CS47L35_FLL1);
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

    ret = cs47l35_process(&cs47l35_driver);

    if (ret != CS47L35_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

void cs47l35_notification_callback(uint32_t event_flags, void *arg)
{
    if (event_flags & CS47L35_EVENT_FLAG_DSP_DECODER)
    {
        if (start_decoding_flag)
        {
            dsp_decoder_interrupt_flag = true;
        }
    }
    if (event_flags & CS47L35_EVENT_FLAG_DSP_ENCODER)
    {
        if (start_encoding_flag)
        {
            dsp_encoder_interrupt_flag = true;
        }
    }

    return;
}
