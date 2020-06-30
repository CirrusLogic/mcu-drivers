/**
 * @file system_test_hw_0_bsp.c
 *
 * @brief Implementation of the BSP for the system_test_hw_0 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
 *
 */
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "hw_0_bsp.h"
#include "stm32f4xx_hal.h"
#include "cs40l25.h"
#include "cs40l25_syscfg_regs.h"
#include "cs40l25_fw_img.h"
#include "cs40l25_cal_fw_img.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs40l25_t cs40l25_driver;
static fw_img_boot_state_t boot_state;
static uint8_t transmit_buffer[32];
static uint8_t receive_buffer[256];

#ifdef CS40L25_ALGORITHM_DYNAMIC_F0
static cs40l25_dynamic_f0_table_entry_t dynamic_f0;
static uint32_t dynamic_redc;
#endif

#ifndef USE_MALLOC
static uint32_t fw_info_sym_table[CS40L25_SYM_Q_ESTIMATION_Q_EST * 2];
static uint32_t fw_info_alg_id_list[10];
static uint8_t fw_img_boot_state_block_data[4140];
#endif

static cs40l25_bsp_config_t bsp_config =
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

        haptic_config.dsp_config_ctrls.dsp_gpio1_button_detect_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpio2_button_detect_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpio3_button_detect_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpio4_button_detect_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpio_enable = true;
        haptic_config.dsp_config_ctrls.dsp_gpi_gain_control = 0;
        haptic_config.dsp_config_ctrls.dsp_ctrl_gain_control = 0;
        haptic_config.dsp_config_ctrls.dsp_gpio1_index_button_press = 1;
        haptic_config.dsp_config_ctrls.dsp_gpio2_index_button_press = 1;
        haptic_config.dsp_config_ctrls.dsp_gpio3_index_button_press = 1;
        haptic_config.dsp_config_ctrls.dsp_gpio4_index_button_press = 1;
        haptic_config.dsp_config_ctrls.dsp_gpio1_index_button_release = 2;
        haptic_config.dsp_config_ctrls.dsp_gpio2_index_button_release = 2;
        haptic_config.dsp_config_ctrls.dsp_gpio3_index_button_release = 2;
        haptic_config.dsp_config_ctrls.dsp_gpio4_index_button_release = 2;

        haptic_config.dsp_config_ctrls.clab_enable = true;
        haptic_config.dsp_config_ctrls.peak_amplitude = 0x400000;

        haptic_config.event_control.hardware = 1;
        haptic_config.event_control.playback_end_suspend = 1;

        haptic_status = cs40l25_configure(&cs40l25_driver, &haptic_config);
    }

    if (haptic_status != CS40L25_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }

#ifndef CONFIG_TEST_OPEN_LOOP
    // Enable 32kHz clock routing to CS40L25B
    uint8_t temp_buffer[4] = {0x00, 0x1F, 0x80, 0x03};
    bsp_i2c_write(BSP_LN2_DEV_ID, temp_buffer, 4, NULL, NULL);
#endif

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

    return BSP_STATUS_OK;
}

uint32_t bsp_dut_boot(bool cal_boot)
{
    uint32_t ret;
    const uint8_t *fw_img;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    if (!cal_boot)
    {
        fw_img = cs40l25_fw_img;
        fw_img_end = cs40l25_fw_img + sizeof(cs40l25_fw_img);
    }
    else
    {
        fw_img = cs40l25_cal_fw_img;
        fw_img_end = cs40l25_cal_fw_img + sizeof(cs40l25_cal_fw_img);
    }

    // Inform the driver that any current firmware is no longer available by passing a NULL
    // fw_info pointer to cs40l25_boot
    ret = cs40l25_boot(&cs40l25_driver, NULL);
    if (ret != CS40L25_STATUS_OK)
    {
        return ret;
    }

#ifdef USE_MALLOC
    // Free anything malloc'ed in previous boots
    if (boot_state.fw_info.sym_table)
        free(boot_state.fw_info.sym_table);
    if (boot_state.fw_info.alg_id_list)
        free(boot_state.fw_info.alg_id_list);
    if (boot_state.block_data)
        free(boot_state.block_data);
#endif

    // Ensure your fw_img_boot_state_t struct is initialised to zero.
    memset(&boot_state, 0, sizeof(fw_img_boot_state_t));

    // Initialise pointer to the currently available fw_img data and set fw_img_blocks_size
    // to the size of fw_img_v1_header_t
    boot_state.fw_img_blocks = (uint8_t *) fw_img;
    boot_state.fw_img_blocks_size = sizeof(fw_img_v1_header_t);

    // Read in the fw_img header
    ret = fw_img_read_header(&boot_state);
    if (ret)
    {
        return BSP_STATUS_FAIL;
    }

    // Increment fw_img pointer to skip header
    fw_img += sizeof(fw_img_v1_header_t);

    // malloc enough memory to hold the symbol table, using sym_table_size in the previously
    // read in fw_img header
#ifdef USE_MALLOC
    boot_state.fw_info.sym_table = (fw_img_v1_sym_table_t *)malloc(boot_state.fw_info.header.sym_table_size *
                                                                   sizeof(fw_img_v1_sym_table_t));
    if (boot_state.fw_info.sym_table == NULL)
    {
        return BSP_STATUS_FAIL;
    }
#else
    boot_state.fw_info.sym_table = (fw_img_v1_sym_table_t *) fw_info_sym_table;
#endif

    // malloc enough memory to hold the alg_id list, using the alg_id_list_size in the fw_img header
#ifdef USE_MALLOC
    boot_state.fw_info.alg_id_list = (uint32_t *) malloc(boot_state.fw_info.header.alg_id_list_size * sizeof(uint32_t));
    if (boot_state.fw_info.alg_id_list == NULL)
    {
        return BSP_STATUS_FAIL;
    }
#else
    boot_state.fw_info.alg_id_list = (uint32_t *) fw_info_alg_id_list;
#endif

    // Finally malloc enough memory to hold the largest data block in the fw_img being processed.
    // This may have been configured during fw_img creation.
    // If your control interface has specific memory requirements (dma-able, etc), then this memory
    // should adhere to them.
    boot_state.block_data_size = 4140;
#ifdef USE_MALLOC
    boot_state.block_data = (uint8_t *) malloc(boot_state.block_data_size);
#else
    boot_state.block_data = (uint8_t *) fw_img_boot_state_block_data;
#endif
    if (boot_state.block_data == NULL)
    {
        return BSP_STATUS_FAIL;
    }

    // Emulate a system where only 1k fw_img blocks can be processed at a time
    write_size = 1024;

    // Update the fw_img pointer and size in cs40l25_boot_state_t
    boot_state.fw_img_blocks = (uint8_t *) fw_img;
    boot_state.fw_img_blocks_size = write_size;

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

    ret = cs40l25_start_i2s(&cs40l25_driver);

    if (ret == CS40L25_STATUS_OK)
    {
        return BSP_STATUS_OK;
    }
    else
    {
        return BSP_STATUS_FAIL;
    }
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

uint32_t bsp_dut_control(uint32_t id, void *arg)
{
    uint32_t ret;
    cs40l25_control_request_t req;

    req.id = id;
    req.arg = arg;

    if (req.id != BSP_HAPTIC_CONTROL_GET_HALO_HEARTBEAT)
    {
        req.id |= CS40L25_CONTROL_ID_FA_SET_MASK;
    }

    ret = cs40l25_control(&cs40l25_driver, req);

    if (ret != CS40L25_STATUS_OK)
    {
        return BSP_STATUS_FAIL;
    }

    return BSP_STATUS_OK;
}

uint32_t bsp_haptic_dynamic_calibrate(void)
{
#ifdef CS40L25_ALGORITHM_DYNAMIC_F0
    uint32_t ret;
    cs40l25_control_request_t req;

    // Enable Dynamic F0
    req.id = CS40L25_CONTROL_ID_ENABLE_DYNAMIC_F0;
    req.arg = (void *) 1;

    ret = cs40l25_control(&cs40l25_driver, req);

    // Read Dynamic F0 from WT Index 0
    req.id = CS40L25_CONTROL_ID_GET_DYNAMIC_F0;
    dynamic_f0.index = 0;
    req.arg = &dynamic_f0;

    ret = cs40l25_control(&cs40l25_driver, req);

    // Get Dynamic ReDC
    req.id = CS40L25_CONTROL_ID_GET_DYNAMIC_REDC;
    req.arg = &dynamic_redc;

    ret = cs40l25_control(&cs40l25_driver, req);

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
