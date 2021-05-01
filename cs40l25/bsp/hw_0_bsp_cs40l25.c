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
#include <string.h>
#include "hw_0_bsp.h"
#include "cs40l25.h"
#include "cs40l25_ext.h"
#include "cs40l25_syscfg_regs.h"
#include "cs40l25_fw_img.h"
#include "cs40l25_cal_fw_img.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
#define CS40L25_EVENT_TIMEOUT_DURATION_MS   (50)
#define CS40L25_RELEASE_MAX_DURATION_MS     (15)
#define CS40L25_EVENT_TIMEOUT_BUFFER_MS     (5)
#define CS40L25_GPI_RELEASE_TO_VAMP_DISABLE_MS  (CS40L25_EVENT_TIMEOUT_DURATION_MS + \
                                                 CS40L25_RELEASE_MAX_DURATION_MS + \
                                                 CS40L25_EVENT_TIMEOUT_BUFFER_MS)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs40l25_t cs40l25_driver;
static fw_img_boot_state_t boot_state;
static uint32_t current_halo_heartbeat = 0;
#ifdef CS40L25_ALGORITHM_DYNAMIC_F0
static cs40l25_dynamic_f0_table_entry_t dynamic_f0;
static uint32_t dynamic_redc;
#endif

static cs40l25_bsp_config_t bsp_config =
{
    .bsp_dev_id = BSP_DUT_DEV_ID,
    .bsp_reset_gpio_id = BSP_GPIO_ID_DUT_CDC_RESET,
    .bsp_int_gpio_id = BSP_GPIO_ID_DUT_CDC_INT,
    .bus_type = BSP_BUS_TYPE_I2C,
    .notification_cb = &bsp_notification_callback,
    .notification_cb_arg = NULL
};

static cs40l25_haptic_config_t cs40l25_haptic_configs[] =
{
    {
        .cp_gain_control = 0,
        .gpio_enable = false,
        .gpio_gain_control = 0,
        .gpio_trigger_config =
        {
            { .enable = false, .button_press_index = 3, .button_release_index = 4},
            { .enable = false, .button_press_index = 0, .button_release_index = 0},
            { .enable = false, .button_press_index = 0, .button_release_index = 0},
            { .enable = false, .button_press_index = 0, .button_release_index = 0}
        }
    },
    {
        .cp_gain_control = 0,
        .gpio_enable = true,
        .gpio_gain_control = 0,
        .gpio_trigger_config =
        {
            { .enable = true, .button_press_index = 1, .button_release_index = 2},
            { .enable = true, .button_press_index = 0, .button_release_index = 0},
            { .enable = true, .button_press_index = 0, .button_release_index = 0},
            { .enable = true, .button_press_index = 0, .button_release_index = 0}
        }
    }
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
    cs40l25_config_t haptic_config;

    memset(&haptic_config, 0, sizeof(cs40l25_config_t));

    // Initialize chip drivers
    haptic_status = cs40l25_initialize(&cs40l25_driver);
    if (haptic_status == CS40L25_STATUS_OK)
    {
        haptic_config.bsp_config = bsp_config;

        haptic_config.syscfg_regs = cs40l25_syscfg_regs;
        haptic_config.syscfg_regs_total = CS40L25_SYSCFG_REGS_TOTAL;

        haptic_config.event_control.hardware = 1;
        haptic_config.event_control.playback_end_suspend = 1;

#ifdef CONFIG_EXT_BOOST
        // Enable External Boost Mode with 3ms delay of GPI Trigger to VAMP ready
        haptic_config.ext_boost.gpi_playback_delay = 99;    // 3 ms * (1 s / 1000 ms) * 32768 units / s) = 99 units
        haptic_config.ext_boost.use_ext_boost = true;
#endif

        haptic_status = cs40l25_configure(&cs40l25_driver, &haptic_config);
    }

    if (haptic_status != CS40L25_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

    uint32_t temp_buffer;
#ifndef CONFIG_TEST_OPEN_LOOP
    // Enable 32kHz clock routing to CS40L25
#ifndef CONFIG_L25B
    temp_buffer = __builtin_bswap32(0x001F8003);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
#else
    // CDC_AIF2BCLK source set to Channel 3
    temp_buffer = __builtin_bswap32(0x004C0003);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Channel 3 source set to PMIC_32K
    temp_buffer = __builtin_bswap32(0x00BB0022);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
#endif // CONFIG_L25B
#endif // CONFIG_TEST_OPEN_LOOP

#ifndef CONFIG_L25B
    // Configure Codec AIF2 source to be GF AIF1
    temp_buffer = __builtin_bswap32(0x000EE00B);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Configure GF AIF1 source to Codec AIF2
    temp_buffer = __builtin_bswap32(0x00168005);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
#else
    // Configure Codec AIF2 source to be GF AIF2
    temp_buffer = __builtin_bswap32(0x000EE00C);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Configure GF AIF2 source to Codec AIF2
    temp_buffer = __builtin_bswap32(0x00178005);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
#endif

    // CDC_GPIO5 (VAMP_EN) source set to Channel 1
    temp_buffer = __builtin_bswap32(0x003B0001);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Channel 1 source set to GF_GPIO7 (PC_5)
    temp_buffer = __builtin_bswap32(0x00B90017);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    // CDC_GPIO1 (GPIO1) source set to Channel 2
    temp_buffer = __builtin_bswap32(0x00370002);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);
    // Channel 2 source set to GF_GPIO2 (PC_2)
    temp_buffer = __builtin_bswap32(0x00BA0015);
    bsp_i2c_write(BSP_LN2_DEV_ID, (uint8_t *)&temp_buffer, 4, NULL, NULL);

    return ret;
}

uint32_t bsp_dut_reset()
{
    uint32_t ret;

    ret = cs40l25_reset(&cs40l25_driver);

    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    current_halo_heartbeat = 0;

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_update_haptic_config(uint8_t config_index);
uint32_t bsp_dut_enable_haptic_processing(bool enable);

uint32_t bsp_dut_boot(bool cal_boot)
{
    uint32_t ret;
    const uint8_t *fw_img;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    if (!cal_boot)
    {
        fw_img = cs40l25_fw_img;
        fw_img_end = cs40l25_fw_img + FW_IMG_SIZE(cs40l25_fw_img);
    }
    else
    {
        fw_img = cs40l25_cal_fw_img;
        fw_img_end = cs40l25_cal_fw_img + FW_IMG_SIZE(cs40l25_cal_fw_img);
    }

    // Inform the driver that any current firmware is no longer available by passing a NULL
    // fw_info pointer to cs40l25_boot
    ret = cs40l25_boot(&cs40l25_driver, NULL);
    if (ret != CS40L25_STATUS_OK)
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
            ret = cs40l25_write_block(&cs40l25_driver, boot_state.block.block_addr,
                                      boot_state.block_data, boot_state.block.block_size);
            if (ret == CS40L25_STATUS_FAIL)
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
    ret = cs40l25_boot(&cs40l25_driver, &boot_state.fw_info);

    current_halo_heartbeat = 0;

    return ret;
}

uint32_t bsp_dut_calibrate(void)
{
    uint32_t ret;

    ret = cs40l25_calibrate(&cs40l25_driver, CS40L25_CALIB_ALL);

    if (ret == CS40L25_STATUS_OK)
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
    uint32_t ret;

    ret = cs40l25_power(&cs40l25_driver, CS40L25_POWER_UP);

    if (ret == CS40L25_STATUS_OK)
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
    uint32_t ret;

    ret = cs40l25_power(&cs40l25_driver, CS40L25_POWER_DOWN);

    if (ret == CS40L25_STATUS_OK)
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

    ret = cs40l25_power(&cs40l25_driver, CS40L25_POWER_HIBERNATE);

    if (ret == CS40L25_STATUS_OK)
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

    ret = cs40l25_power(&cs40l25_driver, CS40L25_POWER_WAKE);

    if (ret == CS40L25_STATUS_OK)
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_start_i2s(void)
{
    uint32_t ret;
#ifdef CS40L25_ALGORITHM_DVL
    cs40l25_control_request_t req;
    cs40l25_field_accessor_t fa = {0};
#endif

    ret = cs40l25_start_i2s(&cs40l25_driver);
    if (ret == CS40L25_STATUS_FAIL)
    {
        return BSP_STATUS_FAIL;
    }

#ifdef CS40L25_ALGORITHM_DVL
    // Example: Wait 3 seconds and then disable DVL
    bsp_set_timer(3000, NULL, NULL);

    req.id = CS40L25_CONTROL_ID_SET_SYM;
    req.arg = (void *) &fa;

    fa.id = CS40L25_SYM_DVL_EN;
    fa.value = 0;
    fa.size = 32;

    ret = cs40l25_control(&cs40l25_driver, req);
    if (ret == CS40L25_STATUS_FAIL)
    {
        return BSP_STATUS_FAIL;
    }
#endif

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_stop_i2s(void)
{
    uint32_t ret;

    ret = cs40l25_stop_i2s(&cs40l25_driver);

    if (ret == CS40L25_STATUS_OK)
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_has_processed(bool *has_processed)
{
    uint32_t temp_hb;
    uint32_t ret;

    if (has_processed == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    ret = cs40l25_get_halo_heartbeat(&cs40l25_driver, &temp_hb);
    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    if ((temp_hb == current_halo_heartbeat) || (temp_hb == 0))
    {
        *has_processed = false;
    }
    else
    {
        *has_processed = true;
    }

    current_halo_heartbeat = temp_hb;

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_update_haptic_config(uint8_t config_index)
{
    uint32_t ret;

    if (config_index > sizeof(cs40l25_haptic_configs)/sizeof(cs40l25_haptic_config_t))
    {
        return BSP_STATUS_FAIL;
    }

    ret = cs40l25_update_haptic_config(&cs40l25_driver, &(cs40l25_haptic_configs[config_index]));

    if (ret == CS40L25_STATUS_OK)
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

#ifdef CS40L25_ALGORITHM_CLAB
    ret = cs40l25_set_clab_enable(&cs40l25_driver, enable);
    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    ret = cs40l25_set_clab_peak_amplitude(&cs40l25_driver, 0x400000);
    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }
#endif

#ifdef CS40L25_ALGORITHM_DYNAMIC_F0
    // Enable Dynamic F0
    ret = cs40l25_set_dynamic_f0_enable(&cs40l25_driver, enable);

    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }
#endif

    return ret;
}

uint32_t bsp_dut_trigger_haptic(uint8_t waveform, uint32_t duration_ms)
{
    uint32_t ret;

    if (waveform == BSP_DUT_TRIGGER_HAPTIC_POWER_ON)
    {
        ret = cs40l25_trigger_bhm(&cs40l25_driver);
        bsp_set_timer(500, NULL, NULL);
    }
    else
    {
        ret = cs40l25_trigger(&cs40l25_driver, waveform, duration_ms);
    }

    if (ret == CS40L25_STATUS_OK)
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
}

uint32_t bsp_dut_dynamic_calibrate(void)
{
#ifdef CS40L25_ALGORITHM_DYNAMIC_F0
    uint32_t ret;

    // Read Dynamic F0 from WT Index 0
    dynamic_f0.index = 0;
    ret = cs40l25_get_dynamic_f0(&cs40l25_driver, &dynamic_f0);
    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    // Get Dynamic ReDC
    ret = cs40l25_get_dynamic_redc(&cs40l25_driver, &dynamic_redc);
    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }
#endif

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_process(void)
{
    uint32_t ret;

    ret = cs40l25_process(&cs40l25_driver);

    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_discharge_vamp(void)
{
    uint32_t ret;

    ret = cs40l25_enable_vamp_discharge(&cs40l25_driver, true);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    bsp_set_timer(50, NULL, NULL);

    ret = cs40l25_enable_vamp_discharge(&cs40l25_driver, false);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    return ret;
}

uint32_t bsp_dut_enable_vamp(bool is_enabled)
{
    // Set VAMP_EN (GF_GPIO7) according to is_enabled
    if (is_enabled)
    {
        bsp_driver_if_g->set_gpio(BSP_GPIO_ID_GF_GPIO7, BSP_GPIO_HIGH);
    }
    else
    {
        bsp_driver_if_g->set_gpio(BSP_GPIO_ID_GF_GPIO7, BSP_GPIO_LOW);
        bsp_driver_if_g->set_timer(5, NULL, NULL);
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_trigger_gpio1(uint32_t duration_ms)
{
    // Drive GPIO1 HIGH
    bsp_driver_if_g->set_gpio(BSP_GPIO_ID_GF_GPIO2, BSP_GPIO_HIGH);

    // Wait
    bsp_driver_if_g->set_timer(duration_ms, NULL, NULL);

    // Drive GPIO1 LOW
    bsp_driver_if_g->set_gpio(BSP_GPIO_ID_GF_GPIO2, BSP_GPIO_LOW);

    bsp_driver_if_g->set_timer(CS40L25_GPI_RELEASE_TO_VAMP_DISABLE_MS, NULL, NULL);

    return BSP_STATUS_OK;
}