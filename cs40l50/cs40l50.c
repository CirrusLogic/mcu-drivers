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

/**
 * Total attemps to wake part from hibernate
 */
#define CS40L50_WAKE_ATTEMPTS           (10)



/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static const uint32_t cs40l50_internal_bst_cfg[] =
{
    0x00002018, 0x00003321,
    0x00003800, 0x000000FA,
    0x0000380C, 0xC8710200,
    0x00003810, 0x00000001
};

static const uint32_t cs40l50_external_bst_cfg[] =
{
    0x00002018, 0x00003301,
    0x00004404, 0x01000000
};

static const uint32_t cs40l50_a1_errata_internal[] =
{
    0x00000040, 0x00000055,
    0x00000040, 0x000000AA,
    0x00003808, 0x40000001,
    0x000038EC, 0x00000032,
    0x00000040, 0x00000000,
    0x0000201C, 0x00000010,
    0x00003800, 0x0000026E,
    0x0280279C, 0x00000006,
    0x0280285C, 0x00000000,
    0x0280404C, 0x00040020,
    0x02804050, 0x001C0010,
    0x02804054, 0x00040038,
    0x02804058, 0x000002FA,
    0x0280405C, 0x00FFFFFF
};

static const uint32_t cs40l50_a1_errata_external[] =
{
    0x00002034, 0x02000000,
    0x0280279C, 0x00000006,
    0x0280285C, 0x00000000,
    0x0280404C, 0x00050020,
    0x02804050, 0x00340200,
    0x02804054, 0x00040020,
    0x02804058, 0x00183201,
    0x0280405C, 0x00050044,
    0x02804060, 0x00040100,
    0x02804064, 0x00FFFFFF
};

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Get DSP Power Management state
 *
 * @param [in] driver           Pointer to the driver state
 * @param [out] state           current Power Management state
 *
 * @return
 * - CS40L50_STATUS_FAIL        if DSP state is unknown, if control port read fails
 * - CS40L50_STATUS_OK          otherwise
 *
 */
static uint32_t cs40l50_dsp_state_get(cs40l50_t *driver, uint8_t *state)
{
    uint32_t dsp_state = CS40L50_DSP_STATE_UNKNOWN;
    uint32_t ret = CS40L50_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (driver->fw_info == NULL)
    {
        ret = regmap_read(cp, FIRMWARE_CS40L50_HALO_STATE, &dsp_state);
    }
    else
    {
        return CS40L50_STATUS_FAIL;
    }

    if (ret)
    {
        return ret;
    }

    switch (dsp_state)
    {
        case CS40L50_DSP_STATE_HIBERNATE:
            /* intentionally fall through */
        case CS40L50_DSP_STATE_SHUTDOWN:
            /* intentionally fall through */
        case CS40L50_DSP_STATE_STANDBY:
            /* intentionally fall through */
        case CS40L50_DSP_STATE_ACTIVE:
            *state = CS40L50_DSP_STATE_MASK & dsp_state;
            break;

        default:
            return CS40L50_STATUS_FAIL;
    }

    return CS40L50_STATUS_OK;
}

/**
 * Request change of state for Power Management
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] state            New state for Power Management
 *
 * @return
 * - CS40L50_STATUS_FAIL        if control port write fails
 * - CS40L50_STATUS_OK          otherwise
 *
 */

static uint32_t cs40l50_pm_state_transition(cs40l50_t *driver, uint8_t state)
{
    uint32_t cmd, ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    cmd = CS40L50_DSP_MBOX_PM_CMD_BASE + state;

    switch (state)
    {
        case CS40L50_PM_STATE_WAKEUP:
            /* intentionally fall through */
        case CS40L50_PM_STATE_PREVENT_HIBERNATE:
            ret = regmap_write_acked_reg(cp,
                                         CS40L50_DSP_VIRTUAL1_MBOX_1,
                                         cmd,
                                         CS40L50_DSP_MBOX_RESET,
                                         CS40L50_POLL_ACK_CTRL_MAX,
                                         CS40L50_POLL_ACK_CTRL_MS);
            break;
        case CS40L50_PM_STATE_HIBERNATE:
            /* intentionally fall through */
        case CS40L50_PM_STATE_ALLOW_HIBERNATE:
            /* intentionally fall through */
        case CS40L50_PM_STATE_SHUTDOWN:
            ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, cmd);
            break;

        default:
            return CS40L50_STATUS_FAIL;
    }

    return ret;
}

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

uint32_t cs40l50_allow_hibernate(cs40l50_t *driver)
{
    uint32_t ret = CS40L50_STATUS_FAIL;

    ret = cs40l50_pm_state_transition(driver, CS40L50_PM_STATE_ALLOW_HIBERNATE);
    if (ret)
    {
        return ret;
    }

    return ret;
}

uint32_t cs40l50_prevent_hibernate(cs40l50_t *driver)
{
    uint32_t ret = CS40L50_STATUS_FAIL;
    uint8_t dsp_state = CS40L50_STATE_HIBERNATE;

    for (uint8_t i = 0; i < CS40L50_WAKE_ATTEMPTS; i++)
    {
        ret = cs40l50_pm_state_transition(driver, CS40L50_PM_STATE_PREVENT_HIBERNATE);
        if (!ret)
        {
            break;
        }
    }

    if (ret)
    {
        return ret;
    }

    ret = cs40l50_dsp_state_get(driver, &dsp_state);

    if (dsp_state != CS40L50_STATE_STANDBY &&
        dsp_state != CS40L50_STATE_ACTIVE)
    {
        return CS40L50_STATUS_FAIL;
    }
    return ret;
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
    // Drive RESET high and wait for at least T_IRS (2.2ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.reset_gpio_id, BSP_GPIO_HIGH);
    bsp_driver_if_g->set_timer(3, NULL, NULL);

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

    // Write system errata
    if (driver->revid == CS40L50_REVID_A1)
    {
        if (driver->config.is_ext_bst)
        {
            ret = regmap_write_array(cp, (uint32_t *) cs40l50_external_bst_cfg,
                                     sizeof(cs40l50_external_bst_cfg)/sizeof(uint32_t));
            if (ret)
            {
              return ret;
            }
            ret = regmap_write_array(cp, (uint32_t *) cs40l50_a1_errata_external,
                                     sizeof(cs40l50_a1_errata_external)/sizeof(uint32_t));
            if (ret)
            {
                return ret;
            }
        }
        else
        {
            ret = regmap_write_array(cp, (uint32_t *) cs40l50_internal_bst_cfg,
                                   sizeof(cs40l50_internal_bst_cfg)/sizeof(uint32_t));
            if (ret)
            {
                return ret;
            }
            ret = regmap_write_array(cp, (uint32_t *) cs40l50_a1_errata_internal,
                                     sizeof(cs40l50_a1_errata_internal)/sizeof(uint32_t));
            if (ret)
            {
                return ret;
            }
        }
    }
    return ret;
}

/**
 * Finish booting the CS40L50
 *
 */
uint32_t cs40l50_boot(cs40l50_t *driver, fw_img_info_t *fw_info)
{
    return CS40L50_STATUS_OK;
}

/**
 * Sets the hibernation timeout
 *
 */
uint32_t cs40l50_timeout_ticks_set(cs40l50_t *driver, uint32_t ms)
{
    uint32_t ret = CS40L50_STATUS_FAIL;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);
    uint32_t lower_val, ticks;


    if (ms > CS40L50_PM_TIMEOUT_MS_MAX)
    {
      ticks = CS40L50_PM_TIMEOUT_MS_MAX * CS40L50_PM_TICKS_MS_DIV;
    }
    else
    {
      ticks = ms * CS40L50_PM_TICKS_MS_DIV;
    }

    lower_val = ticks & CS40L50_PM_TIMEOUT_TICKS_LOWER_MASK;

    ret = regmap_write(cp, CS40L50_PM_TIMER_TIMEOUT_TICKS_3_L, lower_val);
    if (ret)
    {
      return ret;
    }

    return ret;
}

/**
 * Change the power state
 *
 */
uint32_t cs40l50_power(cs40l50_t *driver, uint32_t power_state)
{
    uint32_t ret = CS40L50_STATUS_OK;
    uint32_t new_state = driver->power_state;

    switch (power_state)
    {
        case CS40L50_POWER_HIBERNATE:
            if (driver->power_state == CS40L50_POWER_STATE_WAKE)
            {
                ret = cs40l50_allow_hibernate(driver);
                if (ret)
                {
                    return ret;
                }
                new_state = CS40L50_POWER_STATE_HIBERNATE;
            }
            break;
        case CS40L50_POWER_WAKE:
            if (driver->power_state == CS40L50_POWER_STATE_HIBERNATE)
            {
                ret = cs40l50_prevent_hibernate(driver);
                if (ret)
                {
                    return ret;
                }
                new_state = CS40L50_POWER_STATE_WAKE;
            }
            break;
        case CS40L50_POWER_DOWN:
        case CS40L50_POWER_UP:
        default:
            ret = CS40L50_STATUS_FAIL;
            break;
    }

    if (ret == CS40L50_STATUS_OK)
    {
        driver->power_state = new_state;
    }

    return ret;
}

/**
 * Calibrate the HALO Core DSP Protection Algorithm
 *
 */
uint32_t cs40l50_calibrate(cs40l50_t *driver)
{
    uint32_t redc, mbox_rd, data;
    uint32_t ret = CS40L50_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_read(cp, CS40L50_DSP_MBOX_QUEUE_WT, &data);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, CS40L50_DSP_MBOX_QUEUE_RD, data);
    if (ret)
    {
        return ret;
    }

    mbox_rd = data;

    ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, CS40L50_DSP_MBOX_REDC_EST);
    if (ret)
    {
        return ret;
    }

    ret = regmap_poll_reg(cp, mbox_rd, CS40L50_DSP_MBOX_REDC_EST_START, 10, 1);
    if (ret)
    {
        return ret;
    }

    mbox_rd += 4;

    ret = regmap_write(cp, CS40L50_DSP_MBOX_QUEUE_RD, mbox_rd);
    if (ret)
    {
        return ret;
    }

    ret = regmap_poll_reg(cp, mbox_rd, CS40L50_DSP_MBOX_REDC_EST_DONE, 30, 1);
    if (ret)
    {
        return ret;
    }

    mbox_rd += 4;

    ret = regmap_write(cp, CS40L50_DSP_MBOX_QUEUE_RD, mbox_rd);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, CS40L50_RE_EST_STATUS_REG, &redc);
    if (ret)
    {
        return ret;
    }

    driver->config.cal_data.redc = redc;

    return CS40L50_STATUS_OK;
}

uint32_t cs40l50_set_redc(cs40l50_t *driver, uint32_t redc)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, CS40L50_REDC_OTP_STORED, redc);
    return ret;
}

uint32_t cs40l50_set_f0(cs40l50_t *driver, uint32_t f0)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, CS40L50_F0_OTP_STORED, f0);
    return ret;
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
        case RAM_BANK:
            wf_index = CS40L50_CMD_INDEX_RAM_WAVE | index;
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
