/**
 * @file cs40l50.c
 *
 * @brief The CS40L50 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022 All Rights Reserved, http://www.cirrus.com/
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
#include <stddef.h>
#include "cs40l50.h"
#include "bsp_driver_if.h"
#include "string.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

#define UNUSED(x) (void)(x)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Notify the driver when the CS40L50 INTb GPIO drops low.
 *
 * This callback is registered with the BSP in the register_gpio_cb() API call.
 *
 * The primary task of this callback is to transition the driver mode from CS40L50_MODE_HANDLING_CONTROLS to
 * CS40L50_MODE_HANDLING_EVENTS, in order to signal to the main thread to process events.
 *
 * @param [in] status           BSP status for the INTb IRQ.
 * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
 *                              pointer to the driver state cs40l50_t.
 *
 * @return none
 *
 * @see bsp_driver_if_t member register_gpio_cb.
 * @see bsp_callback_t
 *
 */
static void cs40l50_irq_callback(uint32_t status, void *cb_arg)
{
    cs40l50_t *d;

    d = (cs40l50_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Switch driver mode to CS40L50_MODE_HANDLING_EVENTS
        d->mode = CS40L50_MODE_HANDLING_EVENTS;
    }

    return;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize driver state/handle
 *
 */
uint32_t cs40l50_initialize(cs40l50_t *driver)
{
    uint32_t ret = CS40L50_STATUS_FAIL;

    if (NULL != driver)
    {
        /*
         * The memset() call sets all members to 0, including the following semantics:
         * - 'state' is set to UNCONFIGURED
         */
        memset(driver, 0, sizeof(cs40l50_t));

        ret = CS40L50_STATUS_OK;
    }

    return ret;
}

/**
 * Configures driver state/handle
 *
 */
uint32_t cs40l50_configure(cs40l50_t *driver, cs40l50_config_t *config)
{
    uint32_t ret = CS40L50_STATUS_FAIL;

    if ((NULL != driver) && \
        (NULL != config))
    {
        driver->config = *config;

        ret = bsp_driver_if_g->register_gpio_cb(driver->config.bsp_config.int_gpio_id,
                                                &cs40l50_irq_callback,
                                                driver);

        if (ret == BSP_STATUS_OK)
        {
            ret = CS40L50_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Processes driver events and notifications
 *
 */
uint32_t cs40l50_process(cs40l50_t *driver)
{
    UNUSED(driver);

    return CS40L50_STATUS_FAIL;
}

/**
 * Reset the CS40L50
 *
 */
uint32_t cs40l50_reset(cs40l50_t *driver)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Drive RESET low for at least T_RLPW (1ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.reset_gpio_id, BSP_GPIO_LOW);
    bsp_driver_if_g->set_timer(2, NULL, NULL);
    // Drive RESET high and wait for at least T_IRS (0.75ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.reset_gpio_id, BSP_GPIO_HIGH);
    bsp_driver_if_g->set_timer(2, NULL, NULL);

    // Read DEVID
    ret = regmap_read(cp, CS40L50_SW_RESET_DEVID_REG, &(driver->devid));
    if (ret)
    {
        return ret;
    }

    // Read REVID
    ret = regmap_read(cp, CS40L50_SW_RESET_REVID_REG, &(driver->revid));
    if (ret)
    {
        return ret;
    }

    // Wait for (OTP + ROM) boot complete
    ret = regmap_poll_reg(cp, FIRMWARE_CS40L50_HALO_STATE, 2, 10, 10);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, CS40L50_DSP_MBOX_CMD_PREVENT_HIBER);
    if (ret)
    {
        return ret;
    }

    // Write system configuration
    ret = regmap_write_array(cp, driver->config.syscfg_regs, driver->config.syscfg_regs_total);
    if (ret)
    {
        return ret;
    }


    return ret;
}

/**
 * Finish booting the CS40L50
 *
 */
uint32_t cs40l50_boot(cs40l50_t *driver, fw_img_info_t *fw_info)
{
    UNUSED(driver);
    UNUSED(fw_info);

    return CS40L50_STATUS_FAIL;
}

/**
 * Change the power state
 *
 */
uint32_t cs40l50_power(cs40l50_t *driver, uint32_t power_state)
{
    UNUSED(driver);
    UNUSED(power_state);

    return CS40L50_STATUS_FAIL;
}

/**
 * Calibrate the HALO Core DSP Protection Algorithm
 *
 */
uint32_t cs40l50_calibrate(cs40l50_t *driver)
{
    uint32_t redc;
    uint32_t ret = CS40L50_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, CS40L50_DSP_MBOX_REDC_EST);
    if (ret)
    {
        return ret;
    }

    bsp_driver_if_g->set_timer(20 , NULL, NULL);

    ret = regmap_read(cp, CS40L50_REDC_ESTIMATION_REG, &redc);
    if (ret)
    {
        return ret;
    }

    driver->config.cal_data.redc = redc;

    return CS40L50_STATUS_OK;
}

/**
 * Trigger haptic effect
 *
 */
uint32_t cs40l50_trigger(cs40l50_t *driver, uint32_t index, cs40l50_wavetable_bank_t bank)
{
    uint32_t ret, wf_index;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    switch (bank)
    {
        case ROM_BANK:
            wf_index = CS40L50_CMD_INDEX_ROM_WAVE | index;
            break;
        default:
            return CS40L50_STATUS_FAIL;
    }

    ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, wf_index);
    if (ret)
    {
        return ret;
    }

    return ret;
}

/*
 * Reads the contents of a single register/memory address
 *
 */
uint32_t cs40l50_read_reg(cs40l50_t *driver, uint32_t addr, uint32_t *val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_read(cp, addr, val);
    if (ret)
    {
        return CS40L50_STATUS_FAIL;
    }

    return CS40L50_STATUS_OK;
}

/*
 * Writes the contents of a single register/memory address
 *
 */
uint32_t cs40l50_write_reg(cs40l50_t *driver, uint32_t addr, uint32_t val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, addr, val);
    if (ret)
    {
        return CS40L50_STATUS_FAIL;
    }

    return CS40L50_STATUS_OK;
}

/*!
 * \mainpage Introduction
 *
 * This document outlines the driver source code included in the MCU Driver Software Package for the CS40L50 Boosted
 * Haptics Driver.  This guide is primarily intended for those involved in end-system implementation, integration, and
 * testing, who will use the CS40L50 MCU Driver Software Package to integrate the CS40L50 driver source code into the
 * end-system's host MCU software.  After reviewing this guide, the reader will be able to begin software integration
 * of the CS40L50 MCU driver and then have the ability to initialize, reset, boot, configure, and service events from
 * the CS40L50.  This guide should be used along with the CS40L50 Datasheet.
 *
 *  In order to obtain any additional materials, and for any questions regarding this guide, the MCU Driver
 *  Software Package, or CS40L50 system integration, please contact your Cirrus Logic Representative.
 */
