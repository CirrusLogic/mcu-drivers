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
#include "cs35l41.h"
#include "cs35l41_fw_img.h"
#include "cs35l41_cal_fw_img.h"
#include "test_tone_tables.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs35l41_t cs35l41_driver;
static fw_img_boot_state_t boot_state;
static uint8_t transmit_buffer[32];
static uint8_t receive_buffer[256];
static uint32_t bsp_dut_volume = CS35L41_AMP_VOLUME_0DB;

#ifndef USE_MALLOC
static uint32_t fw_info_sym_table[CS35L41_SYM_CSPL_CAL_SET_STATUS * 2];
static uint32_t fw_info_alg_id_list[10];
static uint8_t fw_img_boot_state_block_data[4140];
#endif

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
    const uint8_t *fw_img_end;
    uint32_t write_size;

    if (!cal_boot)
    {
        fw_img = cs35l41_fw_img;
        fw_img_end = cs35l41_fw_img + sizeof(cs35l41_fw_img);
    }
    else
    {
        fw_img = cs35l41_cal_fw_img;
        fw_img_end = cs35l41_cal_fw_img + sizeof(cs35l41_cal_fw_img);
        cs35l41_driver.is_cal_boot = true;
    }

    // Inform the driver that any current firmware is no longer available by passing a NULL
    // fw_info pointer to cs35l41_boot
    ret = cs35l41_boot(&cs35l41_driver, NULL);
    if (ret != CS35L41_STATUS_OK)
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

    // Update the fw_img pointer and size in cs35l41_boot_state_t
    boot_state.fw_img_blocks = (uint8_t *) fw_img;
    boot_state.fw_img_blocks_size = write_size;

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
    ret = cs35l41_boot(&cs35l41_driver, &boot_state.fw_info);

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

uint32_t bsp_dut_mute(bool is_mute)
{
    cs35l41_control_request_t req = {0};
    req.id = CS35L41_CONTROL_ID_SET_VOLUME;

    if (is_mute)
    {
        req.arg = (void *)CS35L41_AMP_VOLUME_MUTE;
    }
    else
    {
        req.arg = (void *) bsp_dut_volume;
    }

    if (CS35L41_STATUS_OK == cs35l41_control(&cs35l41_driver, req))
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
