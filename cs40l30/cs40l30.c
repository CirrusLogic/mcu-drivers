/**
 * @file cs40l30.c
 *
 * @brief The CS40L30 Driver module
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
#include <stddef.h>
#include "cs40l30.h"
#include "bsp_driver_if.h"
#include "string.h"
#include "cs40l30_syscfg_regs.h"
#ifndef CONFIG_NO_SHADOW_OTP
#include "cs40l30_shadow_otp_syscfg_regs.h"
#endif

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/**
 * Mask for IRQ1_INT_4 events that trigger entry to Actuator-Safe Mode
 *
 * The relevant fields in IRQ1_INT_4 that trigger entry into Actuator-Safe Mode are:
 * - b11 - TEMP_ERR
 * - b7 - AMP_SHORT_ERR
 * - b5 - BST_SHORT_ERR
 * - b4 - BST_DCM_UVP_ERR
 * - b3 - BST_OVP_ERR
 */
#define CS40L30_IRQ1_INT_4_ASM_EVENT_MASK (0x000008B8)

/**
 * Mask for IRQ1_INT_4 events that trigger entry to Actuator-Safe Mode and require disabling of Boost Converter
 *
 * The relevant fields in IRQ1_INT_4 that trigger entry into Actuator-Safe Mode and require disabling of Boost
 * Converter:
 * - b5 - BST_SHORT_ERR
 * - b4 - BST_DCM_UVP_ERR
 * - b3 - BST_OVP_ERR
 */
#define CS40L30_IRQ1_INT_4_ASM_BOOST_DISABLE_EVENT_MASK (0x00000038)

/**
 * Mask for IRQ1_INT_4 events that indicate Boost Overvoltage event
 * - b3 - BST_OVP_ERR
 * - b0 - BST_OVP_WARN_RISE
 */
#define CS40L30_IRQ1_INT_4_BOOST_OVP_EVENTS_MASK (0x00000009)

/**
 * Mask for IRQ1_INT_4 events that indicate Temperature event
 * - b11 - TEMP_ERR
 * - b8 - TEMP_WARN_RISE
 */
#define CS40L30_IRQ1_INT_4_TEMP_EVENTS_MASK (0x00000900)

/**
 * Mask for IRQ1_INT_4 events that indicate Power Supply event
 * - b14 - VBBR_THRESH
 * - b12 - VPBR_THRESH
 */
#define CS40L30_IRQ1_INT_4_POWER_SUPPLY_EVENTS_MASK (0x00005000)

/**
 * Toggle Mask for CS40L30_MSM_ERROR_RELEASE_REG to Release from Actuator-Safe Mode
 *
 * The relevant fields in CS40L30_MSM_ERROR_RELEASE_REG that require release sequence are:
 * - b6 - TEMP_ERR
 * - b5 - TEMP_WARN
 * - b4 - BST_UVP
 * - b3 - BST_OVP
 * - b2 - BST_SHORT
 * - b1 - AMP_SHORT
 *
 * @see MSM_ERROR_RELEASE_REG
 * @see Datasheet Section 4.18.6
 *
 */
#define CS40L30_ERROR_RELEASE_ASM_MASK  (0x0000007E)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

#ifdef CS40L30_USEFUL_UNUSED
/**
 * Find if an algorithm is in the algorithm list and return true if it is.
 * Returns false if not.
 */
static bool cs40l30_find_algid(cs40l30_t *driver, uint32_t algid_id)
{
    if (driver->fw_info)
    {
        for (uint32_t i = 0; i < driver->fw_info->header.alg_id_list_size; i++)
        {
            if (driver->fw_info->alg_id_list[i] == algid_id)
                return true;
        }
    }

    return false;
}
#endif

uint32_t cs40l30_write_reg_helper(cs40l30_t *driver, uint32_t addr, uint32_t val)
{
    uint32_t ret = CS40L30_STATUS_FAIL;
    uint32_t bsp_status = BSP_STATUS_OK;
    uint8_t write_buffer[8];

    /*
     * Copy Little-Endian contents of 'addr' and 'val' to the Big-Endian format required for Control Port transactions
     * using a uint8_t cp_write_buffer.
     *
     * FIXME: This is not platform independent.
     */
    write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);
    write_buffer[4] = GET_BYTE_FROM_WORD(val, 3);
    write_buffer[5] = GET_BYTE_FROM_WORD(val, 2);
    write_buffer[6] = GET_BYTE_FROM_WORD(val, 1);
    write_buffer[7] = GET_BYTE_FROM_WORD(val, 0);

    // Currently only I2C transactions are supported
    if (driver->config.bsp_config.bus_type == CS40L30_BUS_TYPE_I2C)
    {
        bsp_status = bsp_driver_if_g->i2c_write(driver->config.bsp_config.bsp_dev_id,
                                                write_buffer,
                                                8,
                                                NULL,
                                                NULL);
    }
    if (BSP_STATUS_OK == bsp_status)
    {
        ret = CS40L30_STATUS_OK;
    }

    return ret;
}

static uint32_t cs40l30_cp_bulk_write_block(cs40l30_t *driver, uint32_t addr, uint8_t *bytes, uint32_t length)
{
    uint32_t ret = CS40L30_STATUS_OK;
    uint32_t bsp_status;
    uint8_t write_buffer[4];

    /*
     * Switch from Little-Endian contents of uint32_t 'addr' to Big-Endian format required for Control Port
     * transaction.
     *
     * FIXME: This is not platform independent.
     */
    write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    bsp_status = bsp_driver_if_g->i2c_db_write(driver->config.bsp_config.bsp_dev_id,
                                               write_buffer,
                                               4,
                                               bytes,
                                               length,
                                               NULL,
                                               NULL);

    if (bsp_status == BSP_STATUS_FAIL)
    {
        ret = CS40L30_STATUS_FAIL;
    }

    return ret;
}

uint32_t cs40l30_wseq_write_terminator(cs40l30_t *driver)
{
    int pos = driver->wseq_num_entries;
    if (pos > CS40L30_WSEQ_MAX_ENTRIES)
    {
        return CS40L30_STATUS_FAIL;
    }

    return cs40l30_write_reg_helper(driver, CS40L30_DSP1_POWERONSEQUENCE_REG + (8 * pos), 0x00FFFFFF);
}

uint32_t cs40l30_wseq_write_reg(cs40l30_t *driver, uint32_t entry_pos)
{
    if (entry_pos >= CS40L30_WSEQ_MAX_ENTRIES)
    {
        return CS40L30_STATUS_FAIL;
    }

    return cs40l30_cp_bulk_write_block(driver, CS40L30_DSP1_POWERONSEQUENCE_REG + (8 * entry_pos), driver->wseq_table[entry_pos].words, 8);
}

/**
  *  Append an entry to the table by converting the address and value provided to
 *  a wseq_entry_t
 */
uint32_t cs40l30_wseq_table_add(cs40l30_t *driver, uint32_t address, uint32_t value)
{
    uint32_t ret = CS40L30_STATUS_OK;

    cs40l30_wseq_entry_t *table = (cs40l30_wseq_entry_t *) &driver->wseq_table;
    uint32_t num_entries = driver->wseq_num_entries;

    if (num_entries < CS40L30_WSEQ_MAX_ENTRIES)
    {
        table[num_entries].address_ms = (address & 0xFF00) >> 8;
        table[num_entries].address_ls = address & 0x00FF;
        table[num_entries].val_3 = (value & 0xFF000000) >> 24;
        table[num_entries].val_2 = (value & 0x00FF0000) >> 16;
        table[num_entries].val_1 = (value & 0x0000FF00) >> 8;
        table[num_entries].val_0 = value & 0x000000FF;

        table[num_entries].reserved_0 = 0;
        table[num_entries].reserved_1 = 0;

        driver->wseq_num_entries += 1;
        ret = cs40l30_wseq_write_reg(driver, num_entries);
    }
    else
    {
        ret = CS40L30_STATUS_FAIL;
    }

    return ret;
}

/**
 * Update an existing entry in the wseq_table or add new entry to the table
 * if not already present
 */
uint32_t cs40l30_wseq_table_update(cs40l30_t *driver, uint32_t address, uint32_t value)
{
    uint32_t ret = CS40L30_STATUS_OK;

    cs40l30_wseq_entry_t *table = (cs40l30_wseq_entry_t *) &driver->wseq_table;

    if (address < 0xFFFF)
    {
        uint32_t num_entries = driver->wseq_num_entries;
        bool address_found = false;

        for (uint32_t i = 0; i < num_entries; i++) {
            //Convert entry in table back to separate uint32 address and value for comparison
            uint32_t temp_address = (table[i].address_ms << 8) | (table[i].address_ls);
            uint32_t temp_value = (table[i].val_3 << 24) | (table[i].val_2 << 16) |
                                  (table[i].val_1 << 8) | (table[i].val_0);

            //If the address is in the table already,
            //And the value has been updated:
            //Update the value in the table to match the new value and write the new value to the dsp
            if (temp_address == address)
            {
                if (temp_value != value)
                {
                    table[i].val_3 = (value & 0xFF000000) << 24;
                    table[i].val_2 = (value & 0x00FF0000) << 16;
                    table[i].val_1 = (value & 0x0000FF00) << 8;
                    table[i].val_0 = value & 0x000000FF;

                    ret = cs40l30_wseq_write_reg(driver, i);
                    if (ret)
                        return ret;
                }

                address_found = true;
            }
        }
        //If the address isn't in the table alrady, attempt to append it to the table.
        if (!address_found)
        {
            ret = cs40l30_wseq_table_add(driver, address, value);
            if (ret == CS40L30_STATUS_OK)
            {
                ret = cs40l30_wseq_write_terminator(driver);
            }
        }
    }

    return ret;
}

/**
 * Notify the driver when the CS40L30 INTb GPIO drops low.
 *
 */
static void cs40l30_irq_callback(uint32_t status, void *cb_arg)
{
    cs40l30_t *d;

    d = (cs40l30_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Switch driver mode to CS40L25_MODE_HANDLING_EVENTS
        d->mode = CS40L30_MODE_HANDLING_EVENTS;
    }

    return;
}

static uint32_t cs40l30_handle_hw_events(cs40l30_t *driver)
{
    uint32_t temp_reg_val, ret;

    // Check for and clear
    ret = cs40l30_read_reg(driver, CS40L30_IRQ1_INT_2_REG, &temp_reg_val);
    if (ret)
        return ret;

    if (temp_reg_val & CS40L30_IRQ1_INT_2_FIRST_WAKE_LVL_INT1_BITMASK)
    {
        ret = cs40l30_write_reg_helper(driver, CS40L30_ALWAYS_ON_AO_CTRL_REG, CS40L30_AO_CTRL_FIRST_WAKE_CLR_BITMASK);
        if (ret)
            return ret;
    }

    // Check for Boost, Power Supply, Temperature, and Short events
    cs40l30_irq1_int_4_t irq1_int_4;
    ret = cs40l30_read_reg(driver, CS40L30_IRQ1_INT_4_REG, &(irq1_int_4.word));
    if (ret)
        return ret;

    ret = cs40l30_read_reg(driver, CS40L30_IRQ1_MASK_4_REG, &temp_reg_val);
    if (ret)
        return ret;

    irq1_int_4.word &= ~temp_reg_val;
    if (irq1_int_4.word)
    {
        // Clear IRQ1_INT_4 flags
        ret = cs40l30_write_reg_helper(driver, CS40L30_IRQ1_INT_4_REG, irq1_int_4.word);
        if (ret)
            return ret;

        // Encode events into event_flags
        if (irq1_int_4.word & CS40L30_IRQ1_INT_4_BOOST_OVP_EVENTS_MASK)
        {
            driver->event_flags.boost_overvoltage = 1;
        }
        driver->event_flags.boost_undervoltage = irq1_int_4.bst_dcm_uvp_err;
        driver->event_flags.boost_short = irq1_int_4.bst_short_err;
        driver->event_flags.boost_peak_current = irq1_int_4.bst_ipk;
        driver->event_flags.amp_short = irq1_int_4.amp_short_err;

        if (irq1_int_4.word & CS40L30_IRQ1_INT_4_TEMP_EVENTS_MASK)
        {
            driver->event_flags.overtemp = 1;
        }

        if (irq1_int_4.word & CS40L30_IRQ1_INT_4_POWER_SUPPLY_EVENTS_MASK)
        {
            driver->event_flags.brownout = 1;
        }

        // Check for any events that trigger Actuator-Safe Mode
        if (irq1_int_4.word & CS40L30_IRQ1_INT_4_ASM_EVENT_MASK)
        {
            cs40l30_msm_block_enables_t block_enables;

            // Check for any events that require disabling boost converter
            if (irq1_int_4.word & CS40L30_IRQ1_INT_4_ASM_BOOST_DISABLE_EVENT_MASK)
            {
                // Disable Boost - save bst_en value
                ret = cs40l30_read_reg(driver, CS40L30_MSM_BLOCK_ENABLES_REG, &(block_enables.word));
                if (ret)
                    return ret;

                temp_reg_val = block_enables.word;
                block_enables.bst_en = 0;
                ret = cs40l30_write_reg_helper(driver, CS40L30_MSM_BLOCK_ENABLES_REG, block_enables.word);
                if (ret)
                    return ret;
            }

            // Write '1' to ERR_RLS bits
            ret = cs40l30_write_reg_helper(driver, CS40L30_MSM_ERROR_RELEASE_REG, CS40L30_ERROR_RELEASE_ASM_MASK);
            if (ret)
                return ret;

            // Write '0' to ERR_RLS bits
            ret = cs40l30_write_reg_helper(driver, CS40L30_MSM_ERROR_RELEASE_REG, 0);
            if (ret)
                return ret;

            // Re-enable boost converter if previously disabled
            if (irq1_int_4.word & CS40L30_IRQ1_INT_4_ASM_BOOST_DISABLE_EVENT_MASK)
            {
                // Re-enable Boost
                block_enables.word = temp_reg_val;
                ret = cs40l30_write_reg_helper(driver, CS40L30_MSM_BLOCK_ENABLES_REG, block_enables.word);
                if (ret)
                    return ret;
            }
        }
    }

    return CS40L30_STATUS_OK;
}

static uint32_t cs40l30_handle_dsp_notifications(cs40l30_t *driver)
{
    cs40l30_dsp_event_notifier_t irq_flags, irq_masks;

    cs40l30_fsense_input_desc_t *desc = driver->config.bsp_config.fsense_desc;
    uint32_t input_count = driver->config.bsp_config.fsense_input_count;
    uint32_t ret;

    // Check virtual buttons press and release
    ret = cs40l30_read_reg(driver, CS40L30_IRQ1_INT_9_REG, &(irq_flags.words[0]));
    if (ret)
        return ret;

    ret = cs40l30_read_reg(driver, CS40L30_IRQ1_INT_10_REG, &(irq_flags.words[1]));
    if (ret)
        return ret;

    ret = cs40l30_read_reg(driver, CS40L30_IRQ1_MASK_9_REG, &(irq_masks.words[0]));
    if (ret)
        return ret;

    ret = cs40l30_read_reg(driver, CS40L30_IRQ1_MASK_10_REG, &(irq_masks.words[1]));
    if (ret)
        return ret;

    // Handle button events - Check each button descriptor entry
    for (uint32_t i = 0; i < input_count; i++)
    {
        uint32_t events;

        // Determine if any events correspond to the button descriptor entry at index 'i'
        events = (irq_flags.words[0] >> ((desc->btn_id - 1) * CS40L30_INT9_VIRT_BTN_SHIFT)) & CS40L30_INT9_BTN_BITS;

        // Set event_flags based on a press or release event
        uint32_t shift = (desc->btn_id - 1) * 4;
        if (events & CS40L30_VIRT_PRESS_MASK)
        {
            driver->event_flags.words[0] |= CS40L30_PRESS << shift;
        }
        else if (events & CS40L30_VIRT_RELEASE_MASK)
        {
            driver->event_flags.words[0] |= CS40L30_RELEASE << shift;
        }

        desc++;
    }

    // Handle haptic events
    driver->event_flags.lra_start = (irq_masks.lra_start & irq_flags.lra_start) ? 1 : 0;
    driver->event_flags.lra_end = (irq_masks.lra_end & irq_flags.lra_end) ? 1 : 0;

    // Clear IRQ9 & IRQ10
    ret = cs40l30_write_reg_helper(driver, CS40L30_IRQ1_INT_9_REG, irq_flags.words[0]);
    if (ret)
        return ret;

    return cs40l30_write_reg_helper(driver, CS40L30_IRQ1_INT_10_REG, irq_flags.words[1]);
}

static uint32_t cs40l30_handle_dsp_notifications_cal(cs40l30_t *driver)
{
    // TODO:  will implement as part of [SQA-1736]
    return CS40L30_STATUS_OK;
}

static uint32_t cs40l30_event_handler(cs40l30_t *driver)
{
    uint32_t ret;
    uint32_t temp_reg_val;

    // Check IRQ1 Status
    ret = cs40l30_read_reg(driver, CS40L30_IRQ1_STATUS_REG, &temp_reg_val);
    if (ret)
        return ret;

    if ((temp_reg_val & CS40L30_IRQ1_STATUS_IRQ1_STS1_BITMASK) != CS40L30_IRQ1_STATUS_IRQ1_STS1_BITMASK)
    {
        // Arrived here for an unknown reason
        return CS40L30_STATUS_FAIL;
    }

    // Handle HW events
    ret = cs40l30_handle_hw_events(driver);
    if (ret)
        return ret;

    // Check IRQ1 Status again - if cleared, event handled by checking HW events
    ret = cs40l30_read_reg(driver, CS40L30_IRQ1_STATUS_REG, &temp_reg_val);
    if (ret)
        return ret;

    if ((temp_reg_val & CS40L30_IRQ1_STATUS_IRQ1_STS1_BITMASK) != CS40L30_IRQ1_STATUS_IRQ1_STS1_BITMASK)
    {
        return CS40L30_STATUS_OK;
    }

    // Otherwise, check for HALO-handled events.  Handle events differently if running ROM/Run-time FW or Cal FW
    if (driver->fw_info && driver->fw_info->header.fw_id == CS40L30_FWID_CAL)
    {
        return cs40l30_handle_dsp_notifications_cal(driver);
    }
    else
    {
        return cs40l30_handle_dsp_notifications(driver);
    }
}

static uint32_t cs40l30_prevent_hibernate(cs40l30_t *driver)
{
    return cs40l30_write_acked_reg(driver, CS40L30_DSP_VIRTUAL1_MBOX_1_REG, CS40L30_MBOX_POWER_MGMT_PREVENT_HIBERNATE, 0);
}

static uint32_t cs40l30_allow_hibernate(cs40l30_t *driver)
{
    return cs40l30_write_acked_reg(driver, CS40L30_DSP_VIRTUAL1_MBOX_1_REG, CS40L30_MBOX_POWER_MGMT_ALLOW_HIBERNATE, 0);
}

#ifndef CONFIG_NO_SHADOW_OTP
static uint32_t cs40l30_config_shadow_otp(cs40l30_t *driver)
{
    uint32_t ret;

    // The cs40l30_shadow_otp_syscfg_regs[] masks are all 0xFFFFFFFF, so simply writing the value is ok
    for (uint32_t i = 0; i < CS40L30_SHADOW_OTP_SYSCFG_REGS_TOTAL; i++)
    {
        ret = cs40l30_write_reg(driver,
                                cs40l30_shadow_otp_syscfg_regs[i].address,
                                cs40l30_shadow_otp_syscfg_regs[i].value);
        if (ret)
            return ret;
    }

    ret = cs40l30_write_reg(driver, CS40L30_SKIP_CINIT_REG, CS40L30_SKIP_CINIT);
    if (ret)
        return ret;

    ret = cs40l30_find_symbol(driver, CS40L30_SYM_FIRMWARE_HAPTICS_TUNING_FLAGS);
    if (!ret)
        return CS40L30_STATUS_FAIL;

    ret = cs40l30_write_reg(driver, ret, CS40L30_BOOT_RAM_OTP_SHADOW_ENABLED);
    if (ret)
        return ret;

    return cs40l30_write_reg(driver, CS40L30_ALWAYS_ON_MEM_RET_REG, CS40L30_ALWAYS_ON_MEM_RET_BITMASK);
}
#endif

static uint32_t cs40l30_power_up(cs40l30_t *driver)
{
    uint32_t val, i, reg, ret, halo_state;

#ifndef CONFIG_NO_SHADOW_OTP
    if (driver->need_shadow_otp)
    {
        ret = cs40l30_config_shadow_otp(driver);
        if (ret)
            return ret;
    }
#endif

    if (driver->state == CS40L30_STATE_POWER_UP)
    {
        ret = cs40l30_write_acked_reg(driver, CS40L30_DSP_VIRTUAL1_MBOX_1_REG, CS40L30_MBOX_POWER_MGMT_BOOT_TO_RAM, 0x0);
        if (ret)
            return ret;
    }
    else
    {
        ret = cs40l30_write_reg(driver,
                                CS40L30_DSP1_CCM_CORE_CONTROL_REG,
                                CS40L30_DSP1_CCM_CORE_CONTROL_EN_BITMASK |
                                CS40L30_DSP1_CCM_CORE_CONTROL_PM_REMAP_BITMASK |
                                CS40L30_DSP1_CCM_CORE_CONTROL_RESET_BITMASK);
        if (ret)
            return ret;
    }

    reg = cs40l30_find_symbol(driver, CS40L30_SYM_FIRMWARE_HAPTICS_HALO_STATE);
    if (!reg)
        return CS40L30_STATUS_FAIL;

    if (driver->fw_info->header.fw_id == CS40L30_FWID_CAL)
    {
        halo_state = 0x5;
    }
    else
    {
        halo_state = 0x3;
    }

    for (i = 0; i < CS40L30_PM_TIMEOUT_COUNT; i++)
    {
        ret = cs40l30_read_reg(driver, reg, &val);
        if (ret)
            return ret;

        if (val == halo_state)
        {
            break;
        }
        bsp_driver_if_g->set_timer(CS40L30_PM_TIMEOUT_WAIT, NULL, NULL);
    }
    if (i >= CS40L30_PM_TIMEOUT_COUNT)
    {
        return CS40L30_STATUS_FAIL;
    }

    return CS40L30_STATUS_OK;
}

static uint32_t cs40l30_power_down(cs40l30_t *driver)
{
    uint32_t ret;

    ret = cs40l30_write_acked_reg(driver, CS40L30_DSP_VIRTUAL1_MBOX_1_REG, CS40L30_MBOX_POWER_MGMT_SHUTDOWN, 0x0);
    if (ret)
        return ret;

    ret = cs40l30_write_reg(driver,
                            CS40L30_DSP1_CCM_CORE_CONTROL_REG,
                            CS40L30_DSP1_CCM_CORE_CONTROL_PM_REMAP_BITMASK);
    if (ret)
        return ret;

    return CS40L30_STATUS_OK;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize driver state/handle
 *
 */
uint32_t cs40l30_initialize(cs40l30_t *driver)
{
    uint32_t ret = CS40L30_STATUS_FAIL;

    if (NULL != driver)
    {
        /*
         * The memset() call sets all members to 0, including the following semantics:
         * - 'state' is set to UNCONFIGURED
         */
        memset(driver, 0, sizeof(cs40l30_t));

        ret = CS40L30_STATUS_OK;
    }

    return ret;
}

/**
 * Configures driver state/handle
 *
 */
uint32_t cs40l30_configure(cs40l30_t *driver, cs40l30_config_t *config)
{
    uint32_t ret = CS40L30_STATUS_FAIL;

    if ((NULL != driver) && \
        (NULL != config))
    {
        driver->config = *config;

        // Advance driver to CONFIGURED state
        driver->state = CS40L30_STATE_CONFIGURED;

        ret = bsp_driver_if_g->register_gpio_cb(driver->config.bsp_config.bsp_int_gpio_id,
                                                &cs40l30_irq_callback,
                                                driver);

        if (ret == BSP_STATUS_OK)
        {
            ret = CS40L30_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Processes driver events and notifications
 *
 */
uint32_t cs40l30_process(cs40l30_t *driver)
{
    // check for driver state
    if ((driver->state != CS40L30_STATE_UNCONFIGURED) && (driver->state != CS40L30_STATE_ERROR))
    {
        // check for driver mode
        if (driver->mode == CS40L30_MODE_HANDLING_EVENTS)
        {
            // run through event handler
            if (CS40L30_STATUS_OK == cs40l30_event_handler(driver))
            {
                driver->mode = CS40L30_MODE_HANDLING_CONTROLS;
            }
            else
            {
                driver->state = CS40L30_STATE_ERROR;
            }
        }

        if (driver->state == CS40L30_STATE_ERROR)
        {
            driver->event_flags.driver_state_error = 1;
        }

        if (driver->event_flags.words[0])
        {
            if (driver->config.bsp_config.notification_cb != NULL)
            {
                driver->config.bsp_config.notification_cb(driver->event_flags.words[0],
                                                          driver->config.bsp_config.notification_cb_arg);
            }

            driver->event_flags.words[0] = 0;
        }
    }

    return CS40L30_STATUS_OK;
}

/**
 * Reset the CS40L30
 *
 */
uint32_t cs40l30_reset(cs40l30_t *driver)
{
    uint32_t i, val, temp_reg_val, orig_val, ret;
    bool config_otp_empty = true;
    bool calib_otp_empty = true;

    if ((driver->state == CS40L30_STATE_UNCONFIGURED) || (driver->state == CS40L30_STATE_ERROR))
    {
        return CS40L30_STATUS_FAIL;
    }

    // Drive RESET low for at least T_RLPW (1ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.bsp_reset_gpio_id, BSP_GPIO_LOW);
    bsp_driver_if_g->set_timer(CS40L30_T_RLPW_MS, NULL, NULL);

    // Drive RESET high and wait for at least T_IRS (3ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.bsp_reset_gpio_id, BSP_GPIO_HIGH);
    bsp_driver_if_g->set_timer(CS40L30_T_IRS_MS, NULL, NULL);

    // Read DEVID
    ret = cs40l30_read_reg(driver, CS40L30_SW_RESET_DEVID_REG, &val);
    if (ret)
        return ret;

    driver->devid = val;

    // Read REVID
    ret = cs40l30_read_reg(driver, CS40L30_SW_RESET_REVID_REG, &val);
    if (ret)
        return ret;

    driver->revid = val;

    for (i = 0; i < CS40L30_OTP_CONFIG_BLOCK_SIZE_WORDS; i++)
    {
        ret = cs40l30_read_reg(driver, CS40L30_OTP_CONFIG_START_REG + (i * 4), &val);
        if (ret)
            return ret;

        if (val)
        {
            config_otp_empty = false;
            break;
        }
    }

    ret = cs40l30_read_reg(driver, CS40L30_OTP_CALIB_START_REG, &val);
    if (ret)
        return ret;

    if (val)
    {
        calib_otp_empty = false;
    }

    if (config_otp_empty || calib_otp_empty)
    {
        driver->state = CS40L30_STATE_STANDBY;
#ifndef CONFIG_NO_SHADOW_OTP
        driver->need_shadow_otp = true;
#endif
    }
    else
    {
        driver->state = CS40L30_STATE_POWER_UP;
    }

    syscfg_reg_t *reg = (syscfg_reg_t *) driver->config.syscfg_regs;

    for (uint32_t i = 0; i < driver->config.syscfg_regs_total; i++)
    {
        ret = cs40l30_read_reg(driver, reg->address, &orig_val);
        if (ret)
            return ret;

        temp_reg_val = orig_val & ~(reg->mask);
        temp_reg_val |= reg->value;
        if (orig_val != temp_reg_val)
        {
            ret = cs40l30_write_reg(driver, reg->address, temp_reg_val);
            if (ret)
                return ret;
        }

        reg++;
    }

    return CS40L30_STATUS_OK;
}

/**
 * Write block of data to the CS40L30 register file
 *
 */
uint32_t cs40l30_write_block(cs40l30_t *driver, uint32_t addr, uint8_t *data, uint32_t size)
{
    if (addr == 0 || data == NULL || size == 0 || size % 4 != 0)
    {
        return CS40L30_STATUS_FAIL;
    }

    return cs40l30_cp_bulk_write_block(driver, addr, data, size);
}

/**
 * Finish booting the CS40L30
 *
 */
uint32_t cs40l30_boot(cs40l30_t *driver, fw_img_info_t *fw_info)
{
    driver->fw_info = fw_info;

    return CS40L30_STATUS_OK;
}

/**
 * Change the power state
 *
 */
uint32_t cs40l30_power(cs40l30_t *driver, uint32_t power_state)
{
    // Submit the correct request based on power_state
    uint32_t ret = CS40L30_STATUS_OK;
    switch (power_state)
    {
        case CS40L30_POWER_UP:
            // CS40L30 can transition directly from POWER_UP (ROM Mode) to DSP_POWER_UP (RAM Mode)
            if (driver->fw_info &&
                (driver->state == CS40L30_STATE_STANDBY ||
                driver->state == CS40L30_STATE_POWER_UP))
            {
                ret = cs40l30_power_up(driver);
                driver->state = CS40L30_STATE_DSP_POWER_UP;
            }
            break;

        case CS40L30_POWER_DOWN:
            if (driver->state == CS40L30_STATE_POWER_UP ||
                driver->state == CS40L30_STATE_DSP_POWER_UP)
            {
                ret = cs40l30_power_down(driver);
                driver->state = CS40L30_STATE_STANDBY;
            }
            break;

        case CS40L30_POWER_PREVENT_HIBERNATE:
            ret = cs40l30_prevent_hibernate(driver);
            break;

        case CS40L30_POWER_ALLOW_HIBERNATE:
            ret = cs40l30_allow_hibernate(driver);
            break;

        default:
            break;
    }

    return ret;
}

/**
 * Calibrate the HALO FW
 *
 */
uint32_t cs40l30_calibrate(cs40l30_t *driver,
                           uint32_t calib_type)
{
    return CS40L30_STATUS_OK;
}

/**
 * Reads the contents of a single register/memory address
 *
 */
uint32_t cs40l30_read_reg(cs40l30_t *driver, uint32_t addr, uint32_t *val)
{
    uint32_t ret = CS40L30_STATUS_FAIL;
    uint8_t write_buffer[4];
    uint8_t read_buffer[4];

    /*
     * Switch from Little-Endian contents of uint32_t 'addr' to Big-Endian format required for Control Port transaction.
     * Since register address is first written, cp_write_buffer[] is filled with register address.
     *
     * FIXME: This is not platform independent.
     */
    write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    // Currently only I2C transactions are supported
    if (driver->config.bsp_config.bus_type == CS40L30_BUS_TYPE_I2C)
    {
        uint32_t bsp_status;

        bsp_status = bsp_driver_if_g->i2c_read_repeated_start(driver->config.bsp_config.bsp_dev_id,
                                                              write_buffer,
                                                              4,
                                                              read_buffer,
                                                              4,
                                                              NULL,
                                                              NULL);
        if (BSP_STATUS_OK == bsp_status)
        {
            /*
             * Switch from Big-Endian format required for Control Port transaction to Little-Endian contents of
             * uint32_t 'val'
             *
             * FIXME: This is not platform independent.
             */
            ADD_BYTE_TO_WORD(*val, read_buffer[0], 3);
            ADD_BYTE_TO_WORD(*val, read_buffer[1], 2);
            ADD_BYTE_TO_WORD(*val, read_buffer[2], 1);
            ADD_BYTE_TO_WORD(*val, read_buffer[3], 0);

            ret = CS40L30_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Writes the contents of a single register/memory address
 *
 */
uint32_t cs40l30_write_reg(cs40l30_t *driver, uint32_t addr, uint32_t val)
{
    uint32_t ret = CS40L30_STATUS_FAIL;

    ret = cs40l30_write_reg_helper(driver, addr, val);
    if (ret == CS40L30_STATUS_OK)
    {
        ret = cs40l30_wseq_table_update(driver, addr, val);
    }

    return ret;
}

/**
 * Find if a symbol is in the symbol table and return its address if it is.
 * Returns 0 if not found.
 */
uint32_t cs40l30_find_symbol(cs40l30_t *driver, uint32_t symbol_id)
{
    if (driver->fw_info)
    {
        for (uint32_t i = 0; i < driver->fw_info->header.sym_table_size; i++)
        {
            if (driver->fw_info->sym_table[i].sym_id == symbol_id)
                return driver->fw_info->sym_table[i].sym_addr;
        }
    }

    return 0;
}

/**
 * Writes the contents of a single register/memory address that ACK's with a default value
 *
 */
uint32_t cs40l30_write_acked_reg(cs40l30_t *driver, uint32_t addr, uint32_t val, uint32_t acked_val)
{
    uint32_t count, temp_val, ret;

    ret = cs40l30_write_reg(driver, addr, val);
    if (ret)
        return ret;

    for (count = 0 ; count < CS40L30_ACK_CTRL_TIMEOUT_COUNT; count++)
    {
        bsp_driver_if_g->set_timer(CS40L30_ACK_CTRL_TIMEOUT_WAIT, NULL, NULL);

        ret = cs40l30_read_reg(driver, addr, &temp_val);
        if (ret)
            return ret;

        if (temp_val == acked_val)
        {
            return CS40L30_STATUS_OK;
        }
    }

    return CS40L30_STATUS_FAIL;
}

/*!
 * \mainpage Introduction
 *
 * This document outlines the driver source code included in the MCU Driver Software Package for the CS40L30 Boosted
 * Haptics Driver.  This guide is primarily intended for those involved in end-system implementation, integration, and
 * testing, who will use the CS40L30 MCU Driver Software Package to integrate the CS40L30 driver source code into the
 * end-system's host MCU software.  After reviewing this guide, the reader will be able to begin software integration
 * of the CS40L30 MCU driver and then have the ability to initialize, reset, boot, configure, and service events from
 * the CS40L30.  This guide should be used along with the CS40L30 Datasheet.
 *
 *  In order to obtain any additional materials, and for any questions regarding this guide, the MCU Driver
 *  Software Package, or CS40L30 system integration, please contact your Cirrus Logic Representative.
 */
