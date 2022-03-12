/**
 * @file bsp_cs47l35.c
 *
 * @brief Implementation of the BSP for the cs47l35 platform.
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
#include <stdlib.h>
#include "platform_bsp.h"
#include "cs47l35.h"
#include "cs47l35_ext.h"
#include "cs47l35_syscfg_regs.h"
#include "cs47l35_dsp2_fw_img.h"
#include "cs47l35_dsp3_fw_img.h"
#include "opus_test_01_16.h"
#include "bridge.h"

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
static uint8_t * opus_data;
static uint32_t data_avail;
static uint32_t opus_data_len;
static uint32_t bytes_written_total;
static uint32_t bytes_read_total;
static bool start_decoding_flag = false;
static bool start_encoding_flag = false;

static uint32_t vad_symbol;

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

static bridge_device_t device_list[] =
{
    {
        .bus_i2c_cs_address = 0,
        .device_id_str = "6360",
        .dev_name_str = "CS47L35-1",
        .b.dev_id = BSP_DUT_DEV_ID,
        .b.bus_type = REGMAP_BUS_TYPE_SPI,
        .b.receive_max = BRIDGE_BLOCK_BUFFER_LENGTH_BYTES,
        .b.spi_pad_len = 4
     },
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
        codec_config.syscfg_regs_total = CS47L35_SYSCFG_REGS_TOTAL;

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

    bridge_initialize(&device_list[0], (sizeof(device_list)/sizeof(bridge_device_t)));

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
    boot_state->block_data = NULL;

    // fw_img processing is complete, so inform the driver and pass it the fw_info block
    return cs47l35_boot(&cs47l35_driver, core_no, &boot_state->fw_info);
}

void bsp_enable_mic()
{
    cs47l35_write_reg(&cs47l35_driver, CS47L35_MIC_CHARGE_PUMP_1, 0x0007); // * Mic_Charge_Pump_1(200H): 0007  CP2_DISCH=1, CP2_BYPASS=1, CP2_ENA=1
    cs47l35_write_reg(&cs47l35_driver, CS47L35_MIC_BIAS_CTRL_1, 0x00e7); // * Mic_Bias_Ctrl_1(218H):   00E7  MICB1_EXT_CAP=0, MICB1_LVL=2.2V, MICB1_RATE=Fast start-up / shut-down, MICB1_DISCH=MICBIAS1 discharged when disabled, MICB1_BYPASS=1, MICB1_ENA=1
    cs47l35_write_reg(&cs47l35_driver, CS47L35_MIC_BIAS_CTRL_5, 0x0032); // * Mic_Bias_Ctrl_5(21CH):   0032  MICB1B_DISCH=MICBIAS1B discharged when disabled, MICB1B_ENA=1, MICB1A_DISCH=MICBIAS1A discharged when disabled, MICB1A_ENA=0
    cs47l35_write_reg(&cs47l35_driver, CS47L35_IN1L_CONTROL, 0x8c80); // * IN1L_Control(310H):      8C80  IN1L_HPF=1, IN1_DMIC_SUP=MICBIAS1B, IN1_MODE=Digital input, IN1L_PGA_VOL=0dB
    cs47l35_write_reg(&cs47l35_driver, CS47L35_DMIC1L_CONTROL, 0x0300); // * DMIC1L_Control(312H):    0300  IN1_OSR=768kHz, IN1L_DMIC_DLY=0 samples
    cs47l35_write_reg(&cs47l35_driver, CS47L35_HPF_CONTROL, 0x0004); // * HPF_Control(30CH):       0004  IN_HPF_CUT=40Hz
    cs47l35_write_reg(&cs47l35_driver, CS47L35_INPUT_ENABLES, 0x0002); // * Input_Enables(300H):     0002  IN2L_ENA=0, IN2R_ENA=0, IN1L_ENA=1, IN1R_ENA=0
    cs47l35_write_reg(&cs47l35_driver, CS47L35_ADC_DIGITAL_VOLUME_1L, 0x0280); // * ADC_Digital_Volume_1L(311H): 0280  IN1L_SRC=Differential (IN1ALP - IN1ALN), IN_VU=1, IN1L_MUTE=0, IN1L_VOL=0dB
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
    uint32_t ret = BSP_STATUS_FAIL;
    uint32_t buf_symbol, space_avail, scratch, vad, addr, i;

    switch(use_case) {
        case BSP_USE_CASE_TG_HP_EN:
            ret = cs47l35_fll_enable(&cs47l35_driver, CS47L35_FLL1);
            if (ret != CS47L35_STATUS_OK)
            {
                break;
            }
            ret = cs47l35_fll_wait_for_lock(&cs47l35_driver, CS47L35_FLL1);
            if (ret != CS47L35_STATUS_OK)
            {
                break;
            }
            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_ENA_MASK, CS47L35_SYSCLK_ENA);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1LMIX_INPUT_1_SOURCE, 0x10);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1RMIX_INPUT_1_SOURCE, 0x10);
            bsp_enable_mic();
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUTPUT_ENABLES_1, CS47L35_HP1L_ENA | CS47L35_HP1R_ENA);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1L, 0x260);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1R, 0x260);

            ret = BSP_STATUS_OK;
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
                break;
            }

            ret = BSP_STATUS_OK;
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
                break;
            }
            ret = cs47l35_fll_enable(&cs47l35_driver, CS47L35_FLL1);
            if (ret)
            {
                break;
            }
            ret = cs47l35_fll_wait_for_lock(&cs47l35_driver, CS47L35_FLL1);
            if (ret)
            {
                break;
            }

            cs47l35_update_reg(&cs47l35_driver, CS47L35_SAMPLE_RATE_1, CS47L35_SAMPLE_RATE_1_MASK, 0x12);
            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_SRC_MASK, 0x04);
            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_FRAC_MASK, 0x0);
            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_ENA_MASK, CS47L35_SYSCLK_ENA);

            cs47l35_update_reg(&cs47l35_driver, CS47L35_DSP_CLOCK_1, CS47L35_DSP_CLK_FREQ_RANGE_MASK | CS47L35_DSP_CLK_SRC_MASK, 0x404);
            cs47l35_update_reg(&cs47l35_driver, CS47L35_DSP_CLOCK_1, CS47L35_DSP_CLK_SRC_MASK, 0x4); //FLL1

            // Set up audio input channels
            bsp_enable_mic();

            // Route IN1L to SCVoice TX
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DSP3RMIX_INPUT_1_SOURCE, 0x10); // IN1L

            // Route SCVoice TX to Opus Encode
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DSP2LMIX_INPUT_1_SOURCE, 0x79); // DSP3 Channel 2

            // Route Opus Decode to SCVoice RX
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DSP3LMIX_INPUT_1_SOURCE, 0x70); // DSP2 Channel 1

            // Route SCVoice RX to OUT1
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1LMIX_INPUT_1_SOURCE, 0x78); // DSP3 channel 1
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1RMIX_INPUT_1_SOURCE, 0x78); // DSP3 channel 1

            // Boot and load firmware
            ret = cs47l35_power(&cs47l35_driver, 2, CS47L35_POWER_MEM_ENA);
            if (ret)
            {
                break;
            }
            bsp_dut_boot(2, cs47l35_dsp2_fw_img, &boot_state_dsp2);

            ret = cs47l35_power(&cs47l35_driver, 3, CS47L35_POWER_MEM_ENA);
            if (ret)
            {
                break;
            }
            bsp_dut_boot(3, cs47l35_dsp3_fw_img, &boot_state_dsp3);

            addr = cs47l35_find_symbol(&cs47l35_driver, 2, CS47L35_DSP2_SYM_SILK_ENCODER_BITRATE_BPS);
            if (!addr)
            {
                break;
            }
            cs47l35_write_reg(&cs47l35_driver, addr, 16000);

            addr = cs47l35_find_symbol(&cs47l35_driver, 2, CS47L35_DSP2_SYM_SILK_ENCODER_USE_VBR);
            if (!addr)
            {
                break;
            }
            cs47l35_write_reg(&cs47l35_driver, addr, 1);

            addr = cs47l35_find_symbol(&cs47l35_driver, 2, CS47L35_DSP2_SYM_SILK_ENCODER_HIGH_WATERMARK_LEVEL);
            if (!addr)
            {
                break;
            }
            // set the initial encoder buffer watermark to 34% free space to ensure a large write to the decode buffer
            cs47l35_write_reg(&cs47l35_driver, addr, 34);

            addr = cs47l35_find_symbol(&cs47l35_driver, 2, CS47L35_DSP2_SYM_SILK_DECODER_HIGH_WATERMARK_LEVEL);
            if (!addr)
            {
                break;
            }
            // set the decoder watermark to 10% full, which we shouldn't see trigger until the encoder finishes
            cs47l35_write_reg(&cs47l35_driver, addr, 10);

            dsp_decoder_interrupt_flag = false;
            dsp_encoder_interrupt_flag = false;

            ret = cs47l35_power(&cs47l35_driver, 2, CS47L35_POWER_UP);
            if (ret)
            {
                break;
            }
            ret = cs47l35_power(&cs47l35_driver, 3, CS47L35_POWER_UP);
            if (ret)
            {
                break;
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
                break;
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
                break;
            }

            opus_data = (uint8_t *)bsp_malloc(BSP_DUT_RECORDING_SIZE);
            opus_data_len = 0x8000;
            bytes_read_total = 0;

            bsp_read_process_done = false;
            start_encoding_flag = true;

            ret = BSP_STATUS_OK;
            break;

        case BSP_USE_CASE_OPUS_RECORD:
            // Read and play recorded data

            if ((dsp_encoder_interrupt_flag && !bsp_read_process_done) || (dsp_decoder_interrupt_flag && bytes_read_total))
            {
                dsp_encoder_interrupt_flag = false;
                dsp_decoder_interrupt_flag = false;

                if (!bytes_read_total)
                {
                    addr = cs47l35_find_symbol(&cs47l35_driver, 2, CS47L35_DSP2_SYM_SILK_ENCODER_HIGH_WATERMARK_LEVEL);
                    if (!addr)
                    {
                        break;
                    }
                    // if this is the first IRQ, reduce the watermark to 80% free space to avoid buffer underruns in the decoder
                    cs47l35_write_reg(&cs47l35_driver, addr, 80);
                }

                ret = cs47l35_dsp_buf_data_avail(&cs47l35_driver, &buffer_enc, &data_avail);
                if (ret)
                {
                    break;
                }

                ret = cs47l35_dsp_buf_read(&cs47l35_driver, &buffer_enc, opus_data, data_avail);
                if (ret)
                {
                    break;
                }

                for (i = 0; i < 10; i++)
                {
                    ret = cs47l35_dsp_buf_space_avail(&cs47l35_driver, &buffer_dec, &space_avail);
                    if (ret)
                    {
                        continue;
                    }

                    if (space_avail >= data_avail)
                    {
                        break;
                    }
                    bsp_set_timer(50, NULL, NULL);

                }
                if (i == 10)
                {
                    break;
                }

                if (data_avail)
                {
                    ret = cs47l35_dsp_buf_write(&cs47l35_driver, &buffer_dec, opus_data, data_avail);
                    if (ret)
                    {
                        break;
                    }
                }

                if (!bsp_read_process_done)
                {
                    bytes_read_total += data_avail;
                    if (bytes_read_total >= opus_data_len)
                    {
                        cs47l35_dsp_buf_eof(&cs47l35_driver, &buffer_enc);
                        bsp_read_process_done = true;
                    }
                }
                else
                {
                    cs47l35_dsp_buf_eof(&cs47l35_driver, &buffer_dec);
                    bsp_write_process_done = true;
                }
            }

            cs47l35_read_reg(&cs47l35_driver, vad_symbol, &vad);
            bsp_driver_if_g->set_gpio(BSP_GPIO_ID_INTP_LED1, vad & 1); // deglitched speech
            bsp_driver_if_g->set_gpio(BSP_GPIO_ID_INTP_LED2, (vad >> 1) & 1); // raw speech

            ret = BSP_STATUS_OK;
            break;

        case BSP_USE_CASE_OPUS_RECORD_DONE:
            bsp_driver_if_g->set_gpio(BSP_GPIO_ID_INTP_LED1, BSP_GPIO_LOW);
            bsp_driver_if_g->set_gpio(BSP_GPIO_ID_INTP_LED2, BSP_GPIO_LOW);
            for (i = 0; i < 30; i++)
            {
                bsp_set_timer(100, NULL, NULL);
                cs47l35_read_reg(&cs47l35_driver, CS47L35_DSP2_SCRATCH_1, &scratch);
                if (scratch & CS47L35_DSP_DEC_ALGORITHM_STOPPED)
                {
                    break;
                }
            }

            if (i == 30)
            {
                break;
            }

            start_encoding_flag = false;
            start_decoding_flag = false;
            bsp_free(lin_buf_ptr_dec);
            bsp_free(lin_buf_ptr_enc);
            bsp_free(opus_data);

            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1R, 0x360);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DAC_DIGITAL_VOLUME_1L, 0x360);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUTPUT_ENABLES_1, 0);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DSP3RMIX_INPUT_1_SOURCE, 0x0);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DSP2LMIX_INPUT_1_SOURCE, 0x0);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DSP3LMIX_INPUT_1_SOURCE, 0x0);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1LMIX_INPUT_1_SOURCE, 0x0);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_OUT1RMIX_INPUT_1_SOURCE, 0x0);
            cs47l35_write_reg(&cs47l35_driver, CS47L35_DSP3AUX2MIX_INPUT_1_SOURCE, 0x0);
            bsp_disable_mic();

            ret = cs47l35_power(&cs47l35_driver, 3, CS47L35_POWER_DOWN);
            if (ret)
            {
                break;
            }

            ret = cs47l35_power(&cs47l35_driver, 2, CS47L35_POWER_DOWN);
            if (ret)
            {
                break;
            }

            cs47l35_update_reg(&cs47l35_driver, CS47L35_SYSTEM_CLOCK_1, CS47L35_SYSCLK_ENA_MASK, 0);
            ret = cs47l35_fll_disable(&cs47l35_driver, CS47L35_FLL1);
            if (ret)
            {
                break;
            }

            ret = BSP_STATUS_OK;
            break;

        default:
            break;
    }

    if (ret == BSP_STATUS_FAIL)
    {
        bsp_write_process_done = true;
        bsp_read_process_done = true;
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

    bridge_process();

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
