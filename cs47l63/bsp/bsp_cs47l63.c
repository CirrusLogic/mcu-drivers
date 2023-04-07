/**
 * @file bsp_cs47l63.c
 *
 * @brief Implementation of the BSP for the cs47l63 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021, 2023 All Rights Reserved, http://www.cirrus.com/
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "platform_bsp.h"
#include "debug.h"
#include "decompr.h"
#include "cs47l63.h"
#include "cs47l63_ext.h"
#include "cs47l63_sym.h"
#include "cs47l63_syscfg_regs.h"
#include "cs47l63_fw_img.h"
#include "bridge.h"
#include "dspbuf.h"
#include "scc.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

#define CS47L63_SRC_MUTE            (0x0)
#define CS47L63_SRC_IN1L            (0x10)
#define CS47L63_SRC_IN2L            (0x12)
#define CS47L63_SRC_TONE_GENERATOR1 (0x4)

#define CS47L63_DSP1_CHANNEL1       (0x100)
#define CS47L63_DSP1_CHANNEL2       (0x101)

#define SCC_FEATURE_SCC             (1<<1)

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
void cs47l63_notification_callback(uint32_t event_flags, void* arg);

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs47l63_t cs47l63_driver;
static fw_img_boot_state_t boot_state;

static uint8_t *decompressed_data;
static uint32_t decompressed_data_len;
static uint8_t *i2s_data;
static uint32_t i2s_data_len;
static data_ringbuf_t i2s_data_buf;
static uint32_t bytes_read_total;
static volatile bool bsp_decompressed_data_playing;

static cs47l63_bsp_config_t bsp_config =
{
    .bsp_reset_gpio_id = BSP_GPIO_ID_DUT_CDC_RESET,
    .bsp_dcvdd_supply_id = BSP_SUPPLY_ID_LN2_DCVDD,
    .bsp_int_gpio_id = BSP_GPIO_ID_DUT_CDC_INT,
    .notification_cb = &cs47l63_notification_callback,
    .notification_cb_arg = NULL,
    .cp_config.dev_id = BSP_DUT_DEV_ID,
    .cp_config.bus_type = REGMAP_BUS_TYPE_SPI,
    .cp_config.receive_max = BSP_DUT_BUFFER_SIZE,
    .cp_config.spi_pad_len = 4
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

static scc_config_t scc_config =
{
    .dsp_core = 1,
    .enc_format = COMPR_ENC_FORMAT_PACKED16,
    .cp_config = REGMAP_GET_CP(&cs47l63_driver),
    .fw_info = NULL,
    .state_symbol = CS47L63_SYM_SCC_SCC_STATE,
    .status_symbol = CS47L63_SYM_SCC_SCC_STATUS,
    .error_symbol = CS47L63_SYM_SCC_SCC_ERROR,
    .control_symbol = CS47L63_SYM_SCC_SCC_CONTROL,
    .manageackctrl_symbol = CS47L63_SYM_SCC_SCCMANAGEACKCTRL,
    .enc_format_symbol = CS47L63_SYM_SCC_BUFFER_FORMAT,
    .host_buffer_raw_symbol = CS47L63_SYM_SCC_HOST_BUFFER_RAW,
};

static scc_t scc;

static dspbuf_config_t dspbuf_config =
{
    .cp = REGMAP_GET_CP(&cs47l63_driver),
    .bufs_config =
    {
        {.base_id = buf1_base, .size_id = buf1_size,      .mem_base = CS47L63_DSP1_XMEM_UNPACKED24_0},
        {.base_id = buf2_base, .size_id = buf1_buf2_size, .mem_base = CS47L63_DSP1_XMEM_UNPACKED24_0},
        {.base_id = buf3_base, .size_id = total_buf_size, .mem_base = CS47L63_DSP1_YMEM_UNPACKED24_0},
    },
    .rb_struct_mem_start_address = CS47L63_DSP1_XMEM_UNPACKED24_0,
    .compr_buf_ptr = NULL,
    .compr_buf_size = 0,
    .buf_symbol = 0,
    .enc_format = COMPR_ENC_FORMAT_PACKED16,
    .bytes_per_reg = CS47L63_DSP_UNPACKED24_BYTES_PER_REG,
};

static dspbuf_t dspbuf;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
bool bsp_process_irq = false;
volatile bool bsp_process_i2s = false;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

// Write audio data to I2S - silence or decompressed audio (if started streaming)
static void bsp_dut_update_i2s_data(void)
{
    // Remove the read data
    data_ringbuf_bytes_read(&i2s_data_buf, BSP_DUT_I2S_HALF_SIZE);

    // If not yet playing then fake adding more silence
    if (!bsp_decompressed_data_playing)
    {
        uint8_t *next_write_ptr;
        uint32_t next_write_length;

        // Get the next write block
        data_ringbuf_next_write_block(&i2s_data_buf, &next_write_ptr, &next_write_length);
        // Fill with silence
        memset(next_write_ptr, 0, next_write_length);
        data_ringbuf_bytes_written(&i2s_data_buf, next_write_length);
    }
    else
    {
        // Playing so add more decompressed data to i2s data buffer
        // CS47L63 only provides a mono stream whereas the I2S is expecting stereo, so duplicate the stream as it
        // is copied in
        uint32_t data_to_write = data_ringbuf_free_space(&i2s_data_buf);
        uint32_t decompr_data_avail = data_ringbuf_data_length(&dspbuf.decompr_data_buf);


        // Whilst there is space for data and data to be read, copy it mono->stereo
        while ((data_to_write > 0) && (decompr_data_avail > 0))
        {
            uint8_t *next_read_ptr;
            uint32_t next_read_len = 0;
            uint8_t stereo_buf[256];
            uint32_t stereo_bytes_written = 0;
            data_ringbuf_next_read_block(&dspbuf.decompr_data_buf, &next_read_ptr, &next_read_len);
            while ((next_read_len > 0) && (data_to_write > 0) && ((stereo_bytes_written + 3) < sizeof(stereo_buf)))
            {
                stereo_buf[stereo_bytes_written] = *next_read_ptr;
                stereo_buf[stereo_bytes_written + 2] = *next_read_ptr;
                next_read_ptr++;
                next_read_len--;
                decompr_data_avail--;
                stereo_buf[stereo_bytes_written + 1] = *next_read_ptr;
                stereo_buf[stereo_bytes_written + 3] = *next_read_ptr;
                next_read_ptr++;
                next_read_len--;
                decompr_data_avail--;
                stereo_bytes_written += 4;
                data_to_write -= 4;
            }
            data_ringbuf_write(&i2s_data_buf, stereo_buf, stereo_bytes_written);
            data_ringbuf_bytes_read(&dspbuf.decompr_data_buf, stereo_bytes_written / 2);
        }
    }
}

void cs47l63_i2s_callback(uint32_t status, void *arg)
{
    if (bsp_decompressed_data_playing)
    {
        bsp_process_i2s = true;
    }
    bsp_dut_update_i2s_data();
}

static uint32_t bsp_scc_init(scc_t *scc)
{
    uint32_t ret;
    ret = regmap_write_fw_control(scc->config.cp_config,
                                  scc->config.fw_info,
                                  CS47L63_SYM_FIRMWARE_INIT_FEATURES,
                                  SCC_FEATURE_SCC);
    if (ret != SCC_STATUS_OK)
    {
        return SCC_STATUS_FAIL;
    }
    else
    {
        return SCC_STATUS_OK;
    }
}
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
    // Route MICBIAS1 to P2
    temp_buffer = __builtin_bswap32(0x00E40008);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    temp_buffer = __builtin_bswap32(0x00E50104);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    temp_buffer = __builtin_bswap32(0x00E38000);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    // Set CDC AIF1 src to GF AIF1
    temp_buffer = __builtin_bswap32(0x000DE00B);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Set GF AIF1 src to CDC AIF1
    temp_buffer = __builtin_bswap32(0x00169004);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    cs47l63_wait(2000);

    bridge_initialize(&device_list[0], (sizeof(device_list)/sizeof(bridge_device_t)));

    temp_buffer = __builtin_bswap32(0x00310001);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    // Set audio frequency to 16000
    bsp_audio_set_fs(BSP_AUDIO_FS_16000_HZ);

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
        free(boot_state.fw_info.sym_table);
    if (boot_state.fw_info.alg_id_list)
        free(boot_state.fw_info.alg_id_list);
    if (boot_state.block_data)
        free(boot_state.block_data);

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
    boot_state.fw_info.sym_table = (fw_img_v1_sym_table_t *)malloc(boot_state.fw_info.header.sym_table_size *
                                                                   sizeof(fw_img_v1_sym_table_t));
    if (boot_state.fw_info.sym_table == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    // malloc enough memory to hold the alg_id list, using the alg_id_list_size in the fw_img header
    boot_state.fw_info.alg_id_list = (uint32_t *) malloc(boot_state.fw_info.header.alg_id_list_size * sizeof(uint32_t));
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
    boot_state.block_data = (uint8_t *) malloc(boot_state.block_data_size);
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

    free(boot_state.block_data);
    boot_state.block_data = NULL;

    return ret;
}

static uint32_t bsp_setup_clocking(void)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(&cs47l63_driver);

    regmap_update_reg(cp,
                      CS47L63_DSP_CLOCK1,
                      CS47L63_DSP_CLK_FREQ_MASK,
                      (0x24DD << CS47L63_DSP_CLK_FREQ_SHIFT));
    ret = cs47l63_fll_config(&cs47l63_driver,
                             CS47L63_FLL1,
                             CS47L63_FLL_SRC_ASP1_BCLK,
                             512000,
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
        debug_printf("Failed to lock FLL1\n\r");
        return BSP_STATUS_FAIL;
    }
    regmap_write(cp, CS47L63_SAMPLE_RATE1, 0x12);
    regmap_update_reg(cp, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN_MASK, CS47L63_SYSCLK_EN);
    return ret;
}

// Initialize buffers, firmware and routing necesssary for compressed audio record/playback
static uint32_t bsp_dut_scc_record(compr_enc_format_t enc_format, uint32_t dsp_core)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(&cs47l63_driver);
    cs47l63_dsp_t dsp_info = cs47l63_driver.dsp_info[dsp_core - 1];

    bytes_read_total = 0;
    bsp_process_irq = false;
    bsp_process_i2s = false;
    bsp_decompressed_data_playing = false;

    // Play silence to ensure there is a clock
    if (i2s_data == NULL)
    {
        i2s_data_len = BSP_DUT_I2S_SIZE;
        i2s_data = (uint8_t *)malloc(i2s_data_len);
        if (i2s_data == NULL)
        {
            debug_printf("Failed to allocate I2S buffer\n\r");
            i2s_data_len = 0;
            return BSP_STATUS_FAIL;
        }
    }
    memset(i2s_data, 0, i2s_data_len);
    data_ringbuf_init(&i2s_data_buf, i2s_data, i2s_data_len);
    // Fake the buffer being full so the DMA callbacks can keep track of which part to fill with data
    data_ringbuf_bytes_written(&i2s_data_buf, i2s_data_len);

    if (decompressed_data == NULL)
    {
        decompressed_data_len = BSP_DUT_RECORDING_SIZE;
        decompressed_data = (uint8_t *)malloc(decompressed_data_len);
        if (decompressed_data == NULL)
        {
            decompressed_data_len = 0;
            free(i2s_data);
            i2s_data = NULL;
            i2s_data_len = 0;
            debug_printf("Failed to allocate decompressed data buffer\n\r");
            return BSP_STATUS_FAIL;
        }
    }
    data_ringbuf_init(&dspbuf.decompr_data_buf, decompressed_data, decompressed_data_len);
    ret =  bsp_audio_play_stream(BSP_I2S_PORT_PRIMARY,
                                 i2s_data,
                                 i2s_data_len,
                                 cs47l63_i2s_callback,
                                 NULL,
                                 cs47l63_i2s_callback,
                                 NULL);
    if (ret != BSP_STATUS_OK)
    {
        debug_printf("Failed to start play over I2S\n\r");
        return BSP_STATUS_FAIL;
    }

    ret = bsp_setup_clocking();
    if (ret != BSP_STATUS_OK)
    {
        debug_printf("Failed to setup clocks\n\r");
        return BSP_STATUS_FAIL;
    }
    regmap_update_reg(cp, CS47L63_SAMPLE_RATE1, CS47L63_SAMPLE_RATE_1_MASK, 0x12);
    regmap_update_reg(cp, CS47L63_SAMPLE_RATE2, CS47L63_SAMPLE_RATE_2_MASK, 0x12);

    // Setup sample rates on DSP RXn
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_RX1, 0x1);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_RX2, 0x1);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_RX3, 0x1);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_RX4, 0x1);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_RX5, 0x1);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_RX6, 0x1);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_RX7, 0x1);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_RX8, 0x1);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_TX1, 0x0);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_TX2, 0x0);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_TX3, 0x0);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_TX4, 0x0);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_TX5, 0x0);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_TX6, 0x0);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_TX7, 0x0);
    regmap_write(cp, dsp_info.base_addr + CS47L63_DSP_OFF_SAMPLE_RATE_TX8, 0x0);
    regmap_write(cp, CS47L63_DSP1RX2_INPUT1, 0x800012); // IN2L

    // Setup MICBIAS
    regmap_write(cp, CS47L63_MICBIAS_CTRL1, 0x81a5);
    regmap_write(cp, CS47L63_MICBIAS_CTRL5, 0x227);

    // Set up audio input channels
    regmap_update_reg(cp, CS47L63_INPUT2_CONTROL1, 0x50021, 0x50021);
    regmap_write(cp, CS47L63_IN2L_CONTROL1, 0x804);
    regmap_write(cp, CS47L63_INPUT_CONTROL, 0x8); // IN2L_EN
    regmap_write(cp, CS47L63_IN2L_CONTROL2, 0xB00080);
    regmap_write(cp, CS47L63_INPUT_CONTROL3, 0x20000000); // IN_VU

    // Boot and load firmware
    ret = cs47l63_power(&cs47l63_driver, 1 , CS47L63_POWER_MEM_ENA);
    if (ret != CS47L63_STATUS_OK)
    {
        return CS47L63_STATUS_FAIL;
    }
    ret = bsp_dut_boot();
    if (ret != CS47L63_STATUS_OK)
    {
        return CS47L63_STATUS_FAIL;
    }

    scc_config.dsp_core = dsp_core;
    scc_config.enc_format = enc_format;
    scc_config.fw_info = cs47l63_driver.dsp_info[scc_config.dsp_core -1].fw_info; //TODO: Should be a getter
    scc_config.cp_config = cp;
    scc_init(&scc, &scc_config, &bsp_scc_init);

    // Set up audio input channels
    regmap_update_reg(cp, CS47L63_DSP1RX2_INPUT1, CS47L63_DSP1RX2_SRC1_MASK, CS47L63_SRC_IN2L);

    // Setup output
    regmap_write(cp, CS47L63_OUTPUT_CONTROL_1, 0x800);
    regmap_update_reg(cp, CS47L63_OUT1L_INPUT1, CS47L63_OUT1L_SRC1_MASK, 0x20);
    regmap_write(cp, CS47L63_OUT1L_VOLUME_1, CS47L63_OUT_VU | 0x64);
    regmap_write(cp, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN_MASK);

    regmap_write(cp, CS47L63_ASP1_ENABLES1, 0x10000);

    ret = cs47l63_power(&cs47l63_driver, 1, CS47L63_POWER_UP);
    if (ret != CS47L63_STATUS_OK)
    {
        return CS47L63_STATUS_FAIL;
    }

    // Init data and dsp buffer
    dspbuf_config.compr_buf_ptr = malloc(BSP_DUT_BUFFER_SIZE);
    dspbuf_config.compr_buf_size = BSP_DUT_BUFFER_SIZE;
    dspbuf_config.buf_symbol = scc_get_host_buffer(&scc);
    dspbuf_config.enc_format = enc_format;
    ret = dspbuf_init(&dspbuf, &dspbuf_config);
    if (ret != DSPBUF_STATUS_OK)
    {
        debug_printf("Failed to init dsp buf %lu\n\r", ret);
        return BSP_STATUS_FAIL;
    }
    bytes_read_total = 0;
    ret = scc_host_command(&scc, SCC_HOST_CMD_START_VTE1);
    if (ret != SCC_STATUS_OK)
    {
        debug_printf("Failed to send host command\n\r");
        return BSP_STATUS_FAIL;
    }
    ret = scc_update_status(&scc);
    if (ret != SCC_STATUS_OK)
    {
        debug_printf("Failed to update status\n\r");
        return BSP_STATUS_FAIL;
    }
    ret = dspbuf_update_status(&dspbuf);
    if (ret != DSPBUF_STATUS_OK)
    {
        debug_printf("Failed to update status\n\r");
        return BSP_STATUS_FAIL;
    }

    return ret;
}

uint32_t bsp_dut_process_compressed_data(void)
{
    uint32_t ret;
    bool can_read_more_data;
    bool can_decrypt_more_data;

    do
    {
        uint32_t compress_space_avail;
        uint32_t decompress_space_avail;
        uint32_t data_read = 0;
        uint32_t bytes_decompressed;

        compress_space_avail = data_ringbuf_free_space(&dspbuf.compr_data_buf);

        // Check if we can read more data
        if ((compress_space_avail > 0) && (dspbuf_get_data_avail(&dspbuf) > 0))
        {
            uint32_t data_len = (compress_space_avail > dspbuf_get_data_avail(&dspbuf)) ?
                            dspbuf_get_data_avail(&dspbuf) : compress_space_avail;
            ret = dspbuf_read(&dspbuf, &dspbuf.compr_data_buf, data_len, &data_read);
            if (ret != DSPBUF_STATUS_OK)
            {
                debug_printf("Failed to read %lu bytes\n\r", data_len);
                return BSP_STATUS_FAIL;
            }
        }
        ret = decompr_data(&dspbuf.decompr, &dspbuf.decompr_data_buf, &dspbuf.compr_data_buf, &bytes_decompressed);
        if (ret != CS47L63_STATUS_OK)
        {
            debug_printf("Failed to decompress\n\r");
            return BSP_STATUS_FAIL;
        }

        // See how much more data can be dealt with
        compress_space_avail = data_ringbuf_free_space(&dspbuf.compr_data_buf);
        decompress_space_avail = data_ringbuf_free_space(&dspbuf.decompr_data_buf);
        bytes_read_total += data_read;
        can_read_more_data = (dspbuf_get_data_avail(&dspbuf) > 0) && (compress_space_avail > 4);
        can_decrypt_more_data = (decompress_space_avail > 8)
                             && (data_ringbuf_data_length(&dspbuf.compr_data_buf) > 0)
                             && (bytes_decompressed > 0);
    } while (can_read_more_data && can_decrypt_more_data);

    if (!bsp_decompressed_data_playing
     && (data_ringbuf_data_length(&dspbuf.decompr_data_buf) >= (BSP_DUT_I2S_SIZE * 2)))
    {
        bsp_decompressed_data_playing = true;
    }

    // If all data from the original interrupt has been processed, and it has not yet been acknowledged, then
    // acknowledge the interrupt
    if ((dspbuf_get_data_avail(&dspbuf) == 0) && bsp_process_irq)
    {
         // If the data has been dealt with, ack the interrupt
         ret = dspbuf_reenable_irq(&dspbuf);
         if (ret != DSPBUF_STATUS_OK)
         {
             return BSP_STATUS_FAIL;
         }
         bsp_process_irq = false;
     }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_use_case(uint32_t use_case)
{
    uint32_t ret = BSP_STATUS_OK;
    uint32_t dsp_buf_error;
    uint32_t scc_state;
    uint32_t scc_status;
    uint32_t scc_error;
    regmap_cp_config_t *cp = REGMAP_GET_CP(&cs47l63_driver);

    switch (use_case) {
        case BSP_USE_CASE_SCC_RECORD_PACKED16:
            bsp_dut_scc_record(COMPR_ENC_FORMAT_PACKED16, 1);
            debug_printf("PACKED16 format\n\r");
            break;
        case BSP_USE_CASE_SCC_RECORD_MSBC:
            bsp_dut_scc_record(COMPR_ENC_FORMAT_MSBC, 1);
            debug_printf("MSBC format\n\r");
            break;
        case BSP_USE_CASE_SCC_MANUAL_TRIGGER:
            ret = scc_update_status(&scc);
            if (ret != SCC_STATUS_OK)
            {
                debug_printf("MANUAL_TRIGGER: failed to update scc status\n\r");
                return BSP_STATUS_FAIL;
            }
            ret = dspbuf_update_status(&dspbuf);
            if (ret != DSPBUF_STATUS_OK)
            {
                debug_printf("MANUAL_TRIGGER: failed to update dsp_buf status\n\r");
                return BSP_STATUS_FAIL;
            }
            ret = scc_host_command(&scc, SCC_HOST_CMD_START_VTE_STREAM1);
            if (ret != SCC_STATUS_OK)
            {
                debug_printf("MANUAL_TRIGGER: failed to issue START_VTE_STREAM1 command\n\r");
                return BSP_STATUS_FAIL;
            }
            ret = scc_update_status(&scc);
            if (ret != SCC_STATUS_OK)
            {
                debug_printf("MANUAL_TRIGGER: failed to update scc status\n\r");
                return BSP_STATUS_FAIL;
            }
            break;
        case BSP_USE_CASE_SCC_TRIGGERED:
            dsp_buf_error = dspbuf_get_error(&dspbuf);
            scc_state = scc_get_state(&scc);
            scc_status = scc_get_status(&scc);
            scc_error = scc_get_error(&scc);
            if ((dsp_buf_error != 0) || (scc_error != 0))
            {
                debug_printf("TRIGGERED: dsp_buf or scc error\n\r");
                return BSP_STATUS_FAIL;
            }

            if ((scc_state == SCC_STATE_STREAM)
            && ((scc_status & SCC_STATUS_VTE1_TRIGGERED) == SCC_STATUS_VTE1_TRIGGERED)
            && ((scc_status & SCC_STATUS_VTE1_MOST_RECENT_TRIGGER) == SCC_STATUS_VTE1_MOST_RECENT_TRIGGER))
            {
                debug_printf("SCC VTE1 has TRIGGERED!\n\r");

                // Acknowledge the trigger
                ret = scc_host_command(&scc, SCC_HOST_CMD_ACK_VTE1_TRIG);
                if (ret != SCC_STATUS_OK)
                {
                    debug_printf("TRIGGERED: failed to issue ACK_VTE1_TRIG command\n\r");
                    return BSP_STATUS_FAIL;
                }
                ret = scc_update_status(&scc);
                if (ret != SCC_STATUS_OK)
                {
                    debug_printf("TRIGGERED: failed to update scc status\n\r");
                    return BSP_STATUS_FAIL;
                }
                ret = dspbuf_update_status(&dspbuf);
                if (ret != DSPBUF_STATUS_OK)
                {
                    debug_printf("TRIGGERED: failed to update dsp_buf status\n\r");
                    return BSP_STATUS_FAIL;
                }
            }
            else
            {
                debug_printf("SCC VTE1 has not TRIGGERED\n\r");
                return BSP_STATUS_FAIL;
            }
            // Deliberate drop through to read any data and ack the interrupt
        case BSP_USE_CASE_SCC_PROCESS_IRQ:
            if (dspbuf_get_data_avail(&dspbuf) == 0)
            {
                ret = dspbuf_data_avail(&dspbuf);
                if (ret != DSPBUF_STATUS_OK)
                {
                    debug_printf("Failed to get data_avail\n\r");
                    return BSP_STATUS_FAIL;
                }
            }
            // Deliberate drop-through
        case BSP_USE_CASE_SCC_PROCESS_I2S:
            dsp_buf_error = dspbuf_get_error(&dspbuf);
            scc_error = scc_get_error(&scc);
            if ((dsp_buf_error != 0) || (scc_error != 0))
            {
                debug_printf("PROCESS_I2S: dsp_buf or scc error\n\r");
                return BSP_STATUS_FAIL;
            }

            ret = bsp_dut_process_compressed_data();
            if (ret != BSP_STATUS_OK)
            {
                debug_printf("SCC PROCESS: Failed to process data\n\r");
                return BSP_STATUS_FAIL;
            }
            break;
        case BSP_USE_CASE_SCC_STOP_RECORDING:
            // SCC
            ret = scc_host_command(&scc, SCC_HOST_CMD_STOP_VTE_STREAM1);
            if (ret != SCC_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = scc_host_command(&scc, SCC_HOST_CMD_STOP_VTE1);
            if (ret != SCC_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = scc_update_status(&scc);
            if (ret != SCC_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = dspbuf_update_status(&dspbuf);
            if (ret != DSPBUF_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }

            dsp_buf_error = dspbuf_get_error(&dspbuf);

            bsp_audio_stop(BSP_I2S_PORT_PRIMARY);

            // Allow some time for the last interrupt to fire
            cs47l63_wait(200);

            decompr_deinit(&dspbuf.decompr);

            // Buffers free
            free(dspbuf_config.compr_buf_ptr);
            dspbuf_config.compr_buf_ptr = NULL;
            free(decompressed_data);
            decompressed_data = NULL;
            decompressed_data_len = 0;
            data_ringbuf_init(&dspbuf.decompr_data_buf, decompressed_data, decompressed_data_len);
            free(i2s_data);
            i2s_data = NULL;
            i2s_data_len = 0;
            data_ringbuf_init(&i2s_data_buf, i2s_data, i2s_data_len);

            // Reset flags
            bsp_process_irq = false;
            bsp_process_i2s = false;
            bsp_decompressed_data_playing = false;

            // Power down
            ret = cs47l63_power(&cs47l63_driver, 1, CS47L63_POWER_DOWN);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }

            // Disable inputs
            regmap_write(cp, CS47L63_IN2L_CONTROL1, 0x804);
            regmap_write(cp, CS47L63_INPUT_CONTROL, 0x0); // IN2L_EN=0
            regmap_write(cp, CS47L63_IN2L_CONTROL2, 0x10000000); // IN2L_MUTE
            regmap_write(cp, CS47L63_INPUT_CONTROL3, 0x20000000); // IN_VU
            regmap_update_reg(cp, CS47L63_DSP1RX2_INPUT1, CS47L63_DSP1RX2_SRC1_MASK, CS47L63_SRC_MUTE);

            // MICBIAS
            regmap_write(cp, CS47L63_MICBIAS_CTRL1, 0x81a4);
            regmap_write(cp, CS47L63_MICBIAS_CTRL5, 0x226);

            // Disable DSP memory
            ret = cs47l63_power(&cs47l63_driver, 1 , CS47L63_POWER_MEM_DIS);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }

            // Disable clock
            regmap_update_reg(cp, CS47L63_SYSTEM_CLOCK1, CS47L63_SYSCLK_EN_MASK, 0);

            // Disable FLLs
            ret = cs47l63_fll_disable(&cs47l63_driver, CS47L63_FLL1);
            if (ret != CS47L63_STATUS_OK)
            {
                return BSP_STATUS_FAIL;
            }
            ret = cs47l63_fll_disable(&cs47l63_driver, CS47L63_FLL2);
            if (ret != CS47L63_STATUS_OK)
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

    ret = cs47l63_process(&cs47l63_driver);

    if (ret != CS47L63_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    bridge_process();

    return BSP_STATUS_OK;
}

void cs47l63_notification_callback(uint32_t event_flags, void* arg)
{
    if (event_flags & CS47L63_EVENT_FLAG_DSP1_IRQ0)
    {
        // Update the statuses of the dsp buffer and scc
        dspbuf_data_avail(&dspbuf);
        dspbuf_update_status(&dspbuf);
        scc_update_status(&scc);
        bsp_process_irq = true;
    }
}

uint32_t bsp_dut_get_driver_handle(void **driver)
{
    if (driver == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    *driver = (void **) &cs47l63_driver;

    return BSP_STATUS_OK;
}

