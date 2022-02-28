/**
 * @file cs35l42.c
 *
 * @brief The CS35L42 Driver module
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
#include <stdio.h>
#include "cs35l42.h"
#include "string.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialization sequence as in section 4.1.5 of the datasheet
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return none
 *
 * @see cs35l42_reset
 *
 */
static uint32_t cs35l42_initialization_patch(cs35l42_t *driver)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Device Initialization Sequence
    ret = regmap_write(cp, CS35L42_TST_DAC_MSM_CONFIG, 0x11330000);

    return ret;
}

/**
 * Unmask selected IRQs.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return none
 *
 * @see cs35l42_reset
 *
 */
static uint32_t cs35l42_unmask_irqs(cs35l42_t *driver)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Unmask selected interrupts
    ret = regmap_update_reg(cp, CS35L42_IRQ1_MASK_1, CS35L42_AMP_ERR_MASK1_MASK | \
                                                     CS35L42_BST_SHORT_ERR_MASK1_MASK | \
                                                     CS35L42_BST_DCM_UVP_ERR_MASK1_MASK | \
                                                     CS35L42_BST_OVP_ERR_MASK1_MASK | \
                                                     CS35L42_MSM_PUP_DONE_MASK1 | \
                                                     CS35L42_MSM_PDN_DONE_MASK1 | \
                                                     CS35L42_DSP_VIRTUAL2_MBOX_WR_MASK1_MASK | \
                                                     CS35L42_DC_WATCHDOG_IRQ_RISE_MASK1_MASK | \
                                                     CS35L42_WKSRC_STATUS6_MASK1_MASK | \
                                                     CS35L42_WKSRC_STATUS_ANY_MASK1_MASK, 0);

    return ret;
}

/**
 * Notify the driver when the CS35L42 INTb GPIO drops low.
 *
 * This callback is registered with the BSP in the register_gpio_cb() API call.
 *
 * The primary task of this callback is to transition the driver mode from CS35L42_MODE_HANDLING_CONTROLS to
 * CS35L42_MODE_HANDLING_EVENTS, in order to signal to the main thread to process events.
 *
 * @param [in] status           BSP status for the INTb IRQ.
 * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
 *                              pointer to the driver state cs35l42_t.
 *
 * @return none
 *
 * @see bsp_driver_if_t member register_gpio_cb.
 * @see bsp_callback_t
 *
 */
static void cs35l42_irq_callback(uint32_t status, void *cb_arg)
{
    cs35l42_t *d;

    d = (cs35l42_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Switch driver mode to CS35L42_MODE_HANDLING_EVENTS
        d->mode = CS35L42_MODE_HANDLING_EVENTS;
    }

    return;
}

/**
 * Handle events indicated by the IRQ pin ALERTb
 *
 * This function performs all steps to handle IRQ and other asynchronous events the driver is aware of,
 * resulting in calling of the notification callback (cs35l42_notification_callback_t).
 *
 * If there are any IRQ events that include Speaker-Safe Mode Errors or Boost-related events, then the procedure
 * outlined in the Datasheet Section 4.16.1.1 is implemented here.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L42_STATUS_FAI         Control port activity fails
 * - CS35L42_STATUS_OK          otherwise
 *
 * @see CS35L42_EVENT_FLAG_
 * @see cs35l42_notification_callback_t
 *
 */
static uint32_t cs35l42_event_handler(void *driver)
{
    uint32_t ret = CS35L42_STATUS_OK;
    return ret;
}

/**
 * Power up from Standby
 *
 * This function performs all necessary steps to transition the CS35L42 to be ready to pass audio through the
 * amplifier DAC.  Completing this results in the driver transition to POWER_UP state.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L42_STATUS_FAIL if:
 *      - Control port activity fails
 * - CS35L42_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l42_power_up(cs35l42_t *driver)
{
    uint32_t ret;
    uint32_t temp_reg_val;
    uint32_t iter_timeout = 0;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    //Set GLOBAL_EN
    ret = regmap_update_reg(cp, CS35L42_GLOBAL_ENABLES, CS35L42_GLOBAL_EN_MASK, 1);
    if (ret)
    {
        return ret;
    }

    // Wait for MSM_PUP_DONE_EINT1 in IRQ1_EINT_1 (Sticky Interrupt Status) to be set
    do
    {
        // T_AMP_PUP (1ms)
        bsp_driver_if_g->set_timer(1, NULL, NULL);

        // Read CS35L42_IRQ1_EINT_1
        ret = regmap_read(cp, CS35L42_IRQ1_EINT_1, &temp_reg_val);
        if (ret)
        {
            return ret;
        }
        iter_timeout++;
        if (iter_timeout > 20)
        {
            printf("power up timeout\n\r");
            return CS35L42_STATUS_FAIL;
        }
    } while ((temp_reg_val & CS35L42_MSM_PUP_DONE_EINT1_MASK) == 0);

    // Clear MSM_PUP_DONE IRQ flag
    ret = regmap_update_reg(cp, CS35L42_IRQ1_EINT_1, CS35L42_MSM_PUP_DONE_EINT1_MASK, 1 << CS35L42_MSM_PUP_DONE_EINT1_SHIFT);
    if (ret)
    {
        return ret;
    }

    return CS35L42_STATUS_OK;
}

/**
 * Power down to Standby
 *
 * This function performs all necessary steps to transition the CS35L42 to be in Standby power mode. Completing
 * this results in the driver transition to STANDBY state.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Control port activity fails
 * - CS35L41_STATUS_OK          otherwise
 *
 */
 static uint32_t cs35l42_power_down(cs35l42_t *driver)
{
    uint32_t ret;
    uint32_t temp_reg_val;
    uint32_t iter_timeout = 0;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    //Clear GLOBAL_EN
    ret = regmap_update_reg(cp, CS35L42_GLOBAL_ENABLES, CS35L42_GLOBAL_EN_MASK, 0);
    if (ret)
    {
        return ret;
    }

    // Wait for MSM_PDN_DONE_EINT1 in IRQ1_EINT_1 (Sticky Interrupt Status) to be set
    do
    {
        // T_AMP_PDN (1ms)
        bsp_driver_if_g->set_timer(1, NULL, NULL);

        // Read CS35L42_IRQ1_EINT_1
        ret = regmap_read(cp, CS35L42_IRQ1_EINT_1, &temp_reg_val);
        if (ret)
        {
            return ret;
        }
        iter_timeout++;
        if (iter_timeout > 20)
        {
            return CS35L42_STATUS_FAIL;
        }
    } while ((temp_reg_val & CS35L42_MSM_PDN_DONE_EINT1_MASK) == 0);

    // Clear MSM_PDN_DONE IRQ flag
    ret = regmap_update_reg(cp, CS35L42_IRQ1_EINT_1, CS35L42_MSM_PDN_DONE_EINT1_MASK, 1 << CS35L42_MSM_PDN_DONE_EINT1_SHIFT);
    if (ret)
    {
        return ret;
    }

    return CS35L42_STATUS_OK;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize driver state/handle
 *
 */
uint32_t cs35l42_initialize(cs35l42_t *driver)
{
    uint32_t ret = CS35L42_STATUS_FAIL;

    if (NULL != driver)
    {
        /*
         * The memset() call sets all members to 0, including the following semantics:
         * - 'state' is set to UNCONFIGURED
         */
        memset(driver, 0, sizeof(cs35l42_t));

        ret = CS35L42_STATUS_OK;
    }

    return ret;
}

/**
 * Configures driver state/handle
 *
 */
uint32_t cs35l42_configure(cs35l42_t *driver, cs35l42_config_t *config)
{
    uint32_t ret = CS35L42_STATUS_FAIL;

    if ((NULL != driver) && \
        (NULL != config))
    {
        driver->config = *config;

        // Advance driver to CONFIGURED state
        driver->state = CS35L42_STATE_CONFIGURED;

        ret = bsp_driver_if_g->register_gpio_cb(driver->config.bsp_config.int_gpio_id,
                                                cs35l42_irq_callback,
                                                driver);

        if (ret == BSP_STATUS_OK)
        {
            ret = CS35L42_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Processes driver states and modes
 *
 */
uint32_t cs35l42_process(cs35l42_t *driver)
{
    // check for driver state
    if ((driver->state != CS35L42_STATE_UNCONFIGURED) && (driver->state != CS35L42_STATE_ERROR))
    {
        // check for driver mode
        if (driver->mode == CS35L42_MODE_HANDLING_EVENTS)
        {
            // run through event handler
            if (CS35L42_STATUS_OK == cs35l42_event_handler(driver))
            {
                driver->mode = CS35L42_MODE_HANDLING_CONTROLS;
            }
            else
            {
                driver->state = CS35L42_STATE_ERROR;
            }
        }

        if (driver->state == CS35L42_STATE_ERROR)
        {
            driver->event_flags |= CS35L42_EVENT_FLAG_STATE_ERROR;
        }

        if (driver->event_flags)
        {
            cs35l42_bsp_config_t *b = &(driver->config.bsp_config);
            if (b->notification_cb != NULL)
            {
                b->notification_cb(driver->event_flags, b->notification_cb_arg);
            }

            driver->event_flags = 0;
        }
    }

    return CS35L42_STATUS_OK;
}

/**
 * Reset the CS35L42 and prepare for HALO FW booting
 *
 */
uint32_t cs35l42_reset(cs35l42_t *driver)
{
    uint32_t ret;
    uint32_t temp_reg_val;
    uint32_t iter_timeout = 0;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Drive RESET low for at least T_RLPW (1ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.reset_gpio_id, BSP_GPIO_LOW);
    bsp_driver_if_g->set_timer(2, NULL, NULL);
    // Drive RESET high and wait for at least T_IRS (0.75ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.reset_gpio_id, BSP_GPIO_HIGH);
    bsp_driver_if_g->set_timer(2, NULL, NULL);

    // Wait for boot sequence to finish
    do
    {
        // Delay to allow boot before checking OTP_BOOT_DONE_STS
        bsp_driver_if_g->set_timer(10, NULL, NULL);

        // Read OTP_CTRL8
        ret = regmap_read(cp, CS35L42_OTP_CTRL8, &temp_reg_val);
        if (ret)
        {
            return ret;
        }
        iter_timeout++;
        if (iter_timeout > 20)
        {
            return CS35L42_STATUS_FAIL;
        }
    } while ((temp_reg_val & CS35L42_OTP_BOOT_DONE_STS_MASK) == 0);

    // Read DEVID
    ret = regmap_read(cp, CS35L42_DEVID, &(driver->devid));
    if (ret)
    {
        return ret;
    }
    // Read REVID
    ret = regmap_read(cp, CS35L42_REVID, &(driver->revid));
    if (ret)
    {
        return ret;
    }

    // Apply patch
    ret = cs35l42_initialization_patch(driver);
    if (ret)
    {
        return ret;
    }

    // Write configuration data
    ret = regmap_write_array(cp, (uint32_t *) driver->config.syscfg_regs, driver->config.syscfg_regs_total);
    if (ret)
    {
        return CS35L42_STATUS_FAIL;
    }

    // Unmask interrupts
    ret = cs35l42_unmask_irqs(driver);
    if (ret)
    {
        return ret;
    }

    ret = regmap_update_reg(cp, CS35L42_ALIVE_DCIN_WD, CS35L42_DCIN_WD_EN_MASK, CS35L42_DCIN_WD_EN_MASK);
    if (ret)
    {
        return ret;
    }
    ret = regmap_update_reg(cp, CS35L42_ALIVE_DCIN_WD, CS35L42_DCIN_WD_THLD_MASK, 1 << CS35L42_DCIN_WD_THLD_SHIFT);
    if (ret)
    {
        return ret;
    }

    // Pause DSP: set DSP1_CCM_CORE_CONTROL = 0x280
    ret = regmap_write(cp, CS35L42_DSP1_CCM_CORE_CONTROL, 0x280);
    if (ret)
    {
        return ret;
    }

    driver->state = CS35L42_STATE_STANDBY;

    return CS35L42_STATUS_OK;
}

/**
 * Finish booting the CS35L42
 *
 */
uint32_t cs35l42_boot(cs35l42_t *driver, fw_img_info_t *fw_info)
{
    uint32_t ret = CS35L42_STATUS_OK;
    return ret;
}

/**
 * Change the power state
 *
 */
uint32_t cs35l42_power(cs35l42_t *driver, uint32_t power_state)
{
    uint32_t ret;
    uint32_t (*fp)(cs35l42_t *driver) = NULL;
    uint32_t next_state = CS35L42_STATE_UNCONFIGURED;

    switch (power_state)
    {
        case CS35L42_POWER_UP:
            if (driver->state == CS35L42_STATE_STANDBY)
            {
                fp = &cs35l42_power_up;

                next_state = CS35L42_STATE_POWER_UP;
            }
            break;

        case CS35L42_POWER_DOWN:
            if ((driver->state == CS35L42_STATE_POWER_UP) ||
                (driver->state == CS35L42_STATE_DSP_POWER_UP))
            {
                fp = &cs35l42_power_down;

                next_state = CS35L42_STATE_STANDBY;
            }
            break;
    }

    if (fp == NULL)
    {
        return CS35L42_STATUS_FAIL;
    }

    ret = fp(driver);

    if (ret == CS35L42_STATUS_OK)
    {
        driver->state = next_state;
    }

    return ret;
}

/**
 * Calibrate the HALO DSP Protection Algorithm
 *
 */
uint32_t cs35l42_calibrate(cs35l42_t *driver, uint32_t ambient_temp_deg_c)
{
    return CS35L42_STATUS_OK;
}

/*
 * Reads the contents of a single register/memory address
 *
 */
uint32_t cs35l42_read_reg(cs35l42_t *driver, uint32_t addr, uint32_t *val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_read(cp, addr, val);
    if (ret)
    {
        return CS35L42_STATUS_FAIL;
    }

    return CS35L42_STATUS_OK;
}

/*
 * Writes the contents of a single register/memory address
 *
 */
uint32_t cs35l42_write_reg(cs35l42_t *driver, uint32_t addr, uint32_t val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, addr, val);
    if (ret)
    {
        return CS35L42_STATUS_FAIL;
    }

    return CS35L42_STATUS_OK;
}

/*
 * Reads, updates and writes (if there's a change) the contents of a single register/memory address
 *
 */
uint32_t cs35l42_update_reg(cs35l42_t *driver, uint32_t addr, uint32_t mask, uint32_t val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_update_reg(cp, addr, mask, val);
    if (ret)
    {
        return CS35L42_STATUS_FAIL;
    }

    return CS35L42_STATUS_OK;
}

/*
 * Write block of data to the CS35L42 register file
 *
 * This call is used to load the HALO FW/COEFF files to HALO RAM.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Starting address of loading destination
 * @param [in] data             Pointer to array of bytes to be written
 * @param [in] size             Size of array of bytes to be written
 *
 * @return
 * - CS35L42_STATUS_FAIL if:
 *      - Any pointers are NULL
 *      - size is not multiple of 4
 *      - Control port activity fails
 * - otherwise, returns CS35L42_STATUS_OK
 *
 */
uint32_t cs35l42_write_block(cs35l42_t *driver, uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write_block(cp,
                             addr,
                             data,
                             size);
    if (ret)
    {
        return CS35L42_STATUS_FAIL;
    }

    return CS35L42_STATUS_OK;
}


/*!
 * \mainpage Introduction
 *
 * This document outlines the driver source code included in the MCU Driver Software Package for the CS35L42 Boosted
 * Amplifier.  This guide is primarily intended for those involved in end-system implementation, integration, and
 * testing, who will use the CS35L42 MCU Driver Software Package to integrate the CS35L42 driver source code into the
 * end-system's host MCU software.  After reviewing this guide, the reader will be able to begin software integration
 * of the CS35L42 MCU driver and then have the ability to initialize, reset, boot, configure, and service events from
 * the CS35L42.  This guide should be used along with the CS35L42 Datasheet.
 *
 *  In order to obtain any additional materials, and for any questions regarding this guide, the MCU Driver
 *  Software Package, or CS40L25 system integration, please contact your Cirrus Logic Representative.
 */

