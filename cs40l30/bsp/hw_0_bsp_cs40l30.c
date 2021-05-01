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
#include "hw_0_bsp_dut.h"
#include "hw_0_bsp.h"
#include "cs40l30.h"
#include "cs40l30_ext.h"
#include "cs40l30_syscfg_regs.h"
#include "cs40l30_fw_img.h"
#include "cs40l30_cal_fw_img.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static cs40l30_t cs40l30_driver;
static fw_img_boot_state_t boot_state;

cs40l30_fsense_input_desc_t fsense_input_desc[] =
{
    {
        .code = BSP_FSENSE_BUTTON_VOLUMEUP_ID,
        .type = 0,
        .btn_id = CS40L30_VIRT_BTN1,
    },
    {
        .code = BSP_FSENSE_BUTTON_VOLUMEDOWN_ID,
        .type = 0,
        .btn_id = CS40L30_VIRT_BTN2,
    },
    {
        .code = BSP_FSENSE_BUTTON_CAMERA_ID,
        .type = 0,
        .btn_id = CS40L30_VIRT_BTN3,
    },
    {
        .code = BSP_FSENSE_BUTTON_VOICECOMMAND_ID,
        .type = 0,
        .btn_id = CS40L30_VIRT_BTN4,
    },
};

cs40l30_bsp_config_t bsp_config =
{
    .bsp_dev_id = BSP_DUT_DEV_ID,
    .bsp_reset_gpio_id = BSP_GPIO_ID_DUT_CDC_RESET,
    .bsp_int_gpio_id = BSP_GPIO_ID_DUT_CDC_INT,
    .bus_type = BSP_BUS_TYPE_I2C,
    .notification_cb = &bsp_notification_callback,
    .notification_cb_arg = NULL,
    .fsense_desc = fsense_input_desc,
    .fsense_input_count = 4
};

uint32_t bsp_haptic_trigger_list[] =
{
    CS40L30_MBOX_HAPTIC_TRIGGER_CTRL_STOP_PLAYBACK,
    CS40L30_MBOX_HAPTIC_TRIGGER_ROM_BANK_0(1),
    CS40L30_MBOX_HAPTIC_TRIGGER_RAM_WAVEFORM(1),
    CS40L30_MBOX_HAPTIC_TRIGGER_OTP_BUZZ(1),
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
    uint32_t ret;
    cs40l30_config_t cs40l30_config;

    // Initialize chip drivers
    ret = cs40l30_initialize(&cs40l30_driver);
    if (ret == CS40L30_STATUS_OK)
    {
        memset(&cs40l30_config, 0, sizeof(cs40l30_config_t));

        cs40l30_config.bsp_config = bsp_config;

        cs40l30_config.syscfg_regs = (const uint32_t *) cs40l30_syscfg_regs;
        cs40l30_config.syscfg_regs_total = CS40L30_SYSCFG_REGS_TOTAL;
        ret = cs40l30_configure(&cs40l30_driver, &cs40l30_config);
    }

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_reset()
{
    uint32_t ret;

    ret = cs40l30_reset(&cs40l30_driver);

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_boot(bool cal_boot)
{
    uint32_t ret;
    const uint8_t *fw_img;
    const uint8_t *fw_img_end;
    uint32_t write_size;

    if (!cal_boot)
    {
        fw_img = cs40l30_fw_img;
        fw_img_end = cs40l30_fw_img + FW_IMG_SIZE(cs40l30_fw_img);
    }
    else
    {
        fw_img = cs40l30_cal_fw_img;
        fw_img_end = cs40l30_cal_fw_img + FW_IMG_SIZE(cs40l30_cal_fw_img);
    }

    // Inform the driver that any current firmware is no longer available by passing a NULL
    // fw_info pointer to cs40l30_boot
    ret = cs40l30_boot(&cs40l30_driver, NULL);
    if (ret)
    {
        return BSP_STATUS_FAIL;
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
    boot_state.fw_info.sym_table = (fw_img_v1_sym_table_t *) bsp_malloc(boot_state.fw_info.header.sym_table_size *
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
            ret = cs40l30_write_block(&cs40l30_driver, boot_state.block.block_addr,
                                      boot_state.block_data, boot_state.block.block_size);
            if (ret == CS40L30_STATUS_FAIL)
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
    ret = cs40l30_boot(&cs40l30_driver, &boot_state.fw_info);

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_calibrate(void)
{
    uint32_t ret;

    ret = cs40l30_calibrate(&cs40l30_driver, CS40L30_CALIB_ALL);

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_power_up(void)
{
    uint32_t ret;

    ret = cs40l30_power(&cs40l30_driver, CS40L30_POWER_UP);

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_power_down(void)
{
    uint32_t ret;

    ret = cs40l30_power(&cs40l30_driver, CS40L30_POWER_DOWN);

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_prevent_hibernate(void)
{
    uint32_t ret;

    ret = cs40l30_power(&cs40l30_driver, CS40L30_POWER_PREVENT_HIBERNATE);

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_allow_hibernate(void)
{
    uint32_t ret;

    ret = cs40l30_power(&cs40l30_driver, CS40L30_POWER_ALLOW_HIBERNATE);

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_process(void)
{
    uint32_t ret;

    ret = cs40l30_process(&cs40l30_driver);

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_haptic_trigger(uint32_t trigger)
{
    uint32_t ret;

    ret = cs40l30_trigger(&cs40l30_driver, bsp_haptic_trigger_list[trigger]);

    if (ret != CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_FAIL;
    }
    else
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}

uint32_t bsp_dut_update_haptic_config(uint8_t config_index)
{
    uint32_t ret = BSP_STATUS_FAIL;

    if (config_index == 1)
    {
        ret = cs40l30_buzzgen_config(&cs40l30_driver, 1, 150, 200, 1000);
    }

    if (ret == CS40L30_STATUS_OK)
    {
        ret = BSP_STATUS_OK;
    }

    return ret;
}
