/**
 * @file cs40l26.c
 *
 * @brief The CS40L26 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2023, 2025 All Rights Reserved, http://www.cirrus.com/
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
#include "cs40l26.h"
#include "bsp_driver_if.h"
#include "string.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L26_POLL_
 * @brief Polling constants for polling times and counts
 *
 * @{
 */
#define CS40L26_POLL_ACK_CTRL_MS        (10)    ///< Delay in ms between polling OTP_BOOT_DONE
#define CS40L26_POLL_ACK_CTRL_MAX       (10)    ///< Maximum number of times to poll OTP_BOOT_DONE
/** @} */

/**
 * Total EINT and MASK registers to handle in IRQ1
 */
#define CS40L26_IRQ1_REG_TOTAL          (4)

/**
 * Total attemps to wake part from hibernate
 */
#define CS40L26_WAKE_ATTEMPTS           (10)

/**
 * Total attemps to calibrate F0
 */
#define CS40L26_F0_CALIBRATION_ATTEMPTS (5)

/**
 * Delay between F0 calibration attempts
 */
#define CS40L26_F0_CALIBRATION_DELAY_MS (20)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/**
 * Mapping of CS40L26 IRQ Flag to Event Flag
 *
 * List is in the form:
 * - word0 - IRQ Flag
 * - word1 - Event Flag
 * - ...
 *
 * @see cs40l26_irq_to_event_id
 *
 */
static const uint32_t cs40l26_irq_eint_1_to_event_flag_map[] =
{
    IRQ1_IRQ1_EINT_1_WKSRC_STATUS1_EINT1_BITMASK,  CS40L26_EVENT_FLAG_WKSRC_GPIO,
    IRQ1_IRQ1_EINT_1_WKSRC_STATUS2_EINT1_BITMASK,  CS40L26_EVENT_FLAG_WKSRC_GPIO,
    IRQ1_IRQ1_EINT_1_WKSRC_STATUS3_EINT1_BITMASK,  CS40L26_EVENT_FLAG_WKSRC_GPIO,
    IRQ1_IRQ1_EINT_1_WKSRC_STATUS4_EINT1_BITMASK,  CS40L26_EVENT_FLAG_WKSRC_GPIO,
    IRQ1_IRQ1_EINT_1_WKSRC_STATUS5_EINT1_BITMASK,  CS40L26_EVENT_FLAG_WKSRC_CP,
    IRQ1_IRQ1_EINT_1_WKSRC_STATUS6_EINT1_BITMASK,  CS40L26_EVENT_FLAG_WKSRC_CP,
    IRQ1_IRQ1_EINT_1_BST_OVP_FLAG_RISE_BITMASK,    CS40L26_EVENT_FLAG_BST_ERROR,
    IRQ1_IRQ1_EINT_1_BST_OVP_FLAG_FALL_BITMASK,    CS40L26_EVENT_FLAG_BST_ERROR,
    IRQ1_IRQ1_EINT_1_BST_OVP_ERR_BITMASK,          CS40L26_EVENT_FLAG_BST_ERROR,
    IRQ1_IRQ1_EINT_1_BST_DCM_UVP_ERR_BITMASK,      CS40L26_EVENT_FLAG_BST_ERROR,
    IRQ1_IRQ1_EINT_1_BST_SHORT_ERR_BITMASK,        CS40L26_EVENT_FLAG_BST_ERROR,
    IRQ1_IRQ1_EINT_1_BST_IPK_FLAG_BITMASK,         CS40L26_EVENT_FLAG_BST_ERROR,
    IRQ1_IRQ1_EINT_1_TEMP_WARN_RISE_BITMASK,       CS40L26_EVENT_FLAG_TEMP_ERROR,
    IRQ1_IRQ1_EINT_1_TEMP_WARN_FALL_BITMASK,       CS40L26_EVENT_FLAG_TEMP_ERROR,
    IRQ1_IRQ1_EINT_1_TEMP_ERR_BITMASK,             CS40L26_EVENT_FLAG_TEMP_ERROR,
    IRQ1_IRQ1_EINT_1_AMP_ERR_BITMASK,              CS40L26_EVENT_FLAG_AMP_ERROR,
    IRQ1_IRQ1_EINT_1_DSP_VIRTUAL2_MBOX_WR_BITMASK, CS40L26_EVENT_FLAG_DSP_VIRTUAL2_MBOX
};

static const uint32_t cs40l26_a1_errata[] =
{
    CS40L26_PLL_REFCLK_DETECT_0, 0x00000000,
    CS40L26_TEST_KEY_CTRL, 0x00000055,
    CS40L26_TEST_KEY_CTRL, 0x000000AA,
    0x0000391C, 0x014DC080
};

static const uint32_t cs40l26_hibernate_patch[] =
{
    CS40L26_DSP1RX1_INPUT, CS40L26_DATA_SRC_ASPRX1,
    CS40L26_DSP1RX1_INPUT, CS40L26_DATA_SRC_ASPRX2,
    IRQ1_IRQ1_MASK_1_REG, 0xFFFFFFFF
};

static const uint32_t cs40l26_wseq_reg_list[] =
{
    CS40L26_DSP1RX1_INPUT,
    CS40L26_REFCLK_INPUT_REG,
    CS40L26_ASP_ENABLES1,
    CS40L26_ASP_CONTROL1,
    CS40L26_ASP_CONTROL2,
    CS40L26_GPIO_PAD_CONTROL,
};

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Notify the driver when the CS40L26 INTb GPIO drops low.
 *
 * This callback is registered with the BSP in the register_gpio_cb() API call.
 *
 * The primary task of this callback is to transition the driver mode from CS40L26_MODE_HANDLING_CONTROLS to
 * CS40L26_MODE_HANDLING_EVENTS, in order to signal to the main thread to process events.
 *
 * @param [in] status           BSP status for the INTb IRQ.
 * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
 *                              pointer to the driver state cs40l26_t.
 *
 * @return none
 *
 * @see bsp_driver_if_t member register_gpio_cb.
 * @see bsp_callback_t
 *
 */
static void cs40l26_irq_callback(uint32_t status, void *cb_arg)
{
    cs40l26_t *d;

    d = (cs40l26_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Switch driver mode to CS40L26_MODE_HANDLING_EVENTS
        d->mode = CS40L26_MODE_HANDLING_EVENTS;
    }

    return;
}

/**
 * Get DSP Power Management state
 *
 * @param [in] driver           Pointer to the driver state
 * @param [out] state           current Power Management state
 *
 * @return
 * - CS40L26_STATUS_FAIL        if DSP state is unknown, if control port read fails
 * - CS40L26_STATUS_OK          otherwise
 *
 */
static uint32_t cs40l26_dsp_state_get(cs40l26_t *driver, uint8_t *state)
{
    uint32_t dsp_state = CS40L26_DSP_STATE_UNKNOWN;
    uint32_t ret = CS40L26_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (driver->fw_info == NULL)
    {
        ret = regmap_read(cp, CS40L26_A1_PM_CUR_STATE_STATIC_REG, &dsp_state);
    }
    else
    {
        ret = regmap_read_fw_control(cp,
                                     driver->fw_info,
                                     CS40L26_SYM_PM_PM_CUR_STATE, &dsp_state);
    }

    if (ret)
    {
        return ret;
    }

    switch (dsp_state)
    {
        case CS40L26_DSP_STATE_HIBERNATE:
            /* intentionally fall through */
        case CS40L26_DSP_STATE_SHUTDOWN:
            /* intentionally fall through */
        case CS40L26_DSP_STATE_STANDBY:
            /* intentionally fall through */
        case CS40L26_DSP_STATE_ACTIVE:
            *state = CS40L26_DSP_STATE_MASK & dsp_state;
            break;

        default:
            return CS40L26_STATUS_FAIL;
    }

    return CS40L26_STATUS_OK;
}

/**
 * Request change of state for Power Management
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] state            New state for Power Management
 *
 * @return
 * - CS40L26_STATUS_FAIL        if control port write fails
 * - CS40L26_STATUS_OK          otherwise
 *
 */
static uint32_t cs40l26_pm_state_transition(cs40l26_t *driver, uint8_t state)
{
    uint32_t cmd, ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    cmd = CS40L26_DSP_MBOX_PM_CMD_BASE + state;

    switch (state)
    {
        case CS40L26_PM_STATE_WAKEUP:
            /* intentionally fall through */
        case CS40L26_PM_STATE_PREVENT_HIBERNATE:
            ret = regmap_write_acked_reg(cp,
                                         CS40L26_DSP_VIRTUAL1_MBOX_1,
                                         cmd,
                                         CS40L26_DSP_MBOX_RESET,
                                         CS40L26_POLL_ACK_CTRL_MAX,
                                         CS40L26_POLL_ACK_CTRL_MS);
            break;

        case CS40L26_PM_STATE_ALLOW_HIBERNATE:
            /* intentionally fall through */
        case CS40L26_PM_STATE_SHUTDOWN:
            ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, cmd);
            break;

        default:
            return CS40L26_STATUS_FAIL;
    }

    if (ret)
    {
        return ret;
    }

    return CS40L26_STATUS_OK;
}

static uint32_t cs40l26_error_release(cs40l26_t *driver, uint32_t err_rls)
{
    uint32_t ret, err_sts, err_cfg;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_read(cp, CS40L26_ERROR_RELEASE, &err_sts);
    if (ret)
    {
        return ret;
    }

    err_cfg = err_sts & ~err_rls;

    ret = regmap_write(cp, CS40L26_ERROR_RELEASE, err_cfg);
    if (ret)
    {
        return ret;
    }

    err_cfg |= err_rls;

    ret = regmap_write(cp, CS40L26_ERROR_RELEASE, err_cfg);
    if (ret)
    {
        return ret;
    }

    err_cfg &= ~err_rls;

    ret = regmap_write(cp, CS40L26_ERROR_RELEASE, err_cfg);

    return ret;
}

static uint32_t cs40l26_unmask_interrupts(cs40l26_t *driver)
{
    uint32_t ret = CS40L26_STATUS_OK;
    uint32_t map_size = (sizeof(cs40l26_irq_eint_1_to_event_flag_map) / sizeof(uint32_t));
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    for (uint8_t i = 0; i < map_size; i+= 2)
    {
        ret = regmap_update_reg(cp,
                                IRQ1_IRQ1_MASK_REG,
                                cs40l26_irq_eint_1_to_event_flag_map[i],
                                0);
        if (ret)
        {
            return ret;
        }
    }
    return ret;
}

/**
 * Maps IRQ Flag to Event ID passed to BSP
 *
 * Allows for abstracting driver events relayed to BSP away from IRQ flags, to allow the possibility that multiple
 * IRQ flags correspond to a single event to relay.
 *
 * @param [in] irq_statuses     pointer to array of 32-bit words from IRQ1_IRQ1_EINT_*_REG registers
 *
 * @return                      32-bit word with CS40L26_EVENT_FLAG_* set for each event detected
 *
 * @see CS40L26_EVENT_FLAG_
 *
 */
static uint32_t cs40l26_irq_to_event_id(uint8_t irq_index, uint32_t irq_statuses)
{
    uint32_t temp_event_flag = 0;
    const uint32_t *map = NULL;
    uint8_t map_size = 0;

    if (irq_index == 0)
    {
        map = cs40l26_irq_eint_1_to_event_flag_map;
        map_size = sizeof(cs40l26_irq_eint_1_to_event_flag_map)/sizeof(uint32_t);
    }

    for (uint8_t i = 0; i < map_size; i+= 2)
    {
        if (irq_statuses & map[i])
        {
            temp_event_flag |= map[i + 1];
        }
    }

    return temp_event_flag;
}

/**
 * Handle events indicated by the IRQ pin ALERTb
 *
 * This function performs all steps to handle IRQ and other asynchronous events the driver is aware of,
 * resulting in calling of the notification callback (cs40l26_notification_callback_t).
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS40L26_STATUS_FAIL        Control port activity fails
 * - CS40L26_STATUS_OK          otherwise
 *
 * @see CS40L26_EVENT_FLAG_
 * @see cs40l26_notification_callback_t
 *
 */
static uint32_t cs40l26_event_handler(cs40l26_t *driver)
{
    uint32_t ret = CS40L26_STATUS_OK;
    uint32_t irq_statuses[CS40L26_IRQ1_REG_TOTAL];
    uint32_t irq_masks[CS40L26_IRQ1_REG_TOTAL];
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);
    uint32_t data;

    ret = regmap_read(cp, IRQ1_IRQ1_STATUS_REG, &data);
    if (ret || (data == 0))
    {
        return ret;
    }

    // Read IRQ1_STATUS
    ret = regmap_read(cp, IRQ1_IRQ1_STS1_REG, &(irq_statuses[0]));
    if (ret)
    {
        return ret;
    }
    // If event handler was called without any IRQ set, then return
    if (irq_statuses[0] == 0)
    {
        return CS40L26_STATUS_OK;
    }

    for (uint8_t i = 0; i < CS40L26_IRQ1_REG_TOTAL; i++)
    {
        // Read IRQ1_EINT_1_*
        ret = regmap_read(cp, (IRQ1_IRQ1_EINT_1_REG + (i * 4)), &(irq_statuses[i]));
        if (ret)
        {
            return ret;
        }

        // Read IRQ1_MASK_1_*
        ret = regmap_read(cp, (IRQ1_IRQ1_MASK_1_REG + (i * 4)), &(irq_masks[i]));
        if (ret)
        {
            return ret;
        }

        irq_statuses[i] &= ~(irq_masks[i]);

        // If there are unmasked IRQs, then process
        if (irq_statuses[i])
        {
            // Clear any IRQ1 flags from first register
            ret = regmap_write(cp, (IRQ1_IRQ1_EINT_1_REG + (i * 4)), irq_statuses[i]);
            if (ret)
            {
                return ret;
            }
        }
        // Set event flags
        if (i == 0)
        {
            driver->event_flags = cs40l26_irq_to_event_id(i, irq_statuses[i]);
        }
    }

    if (irq_statuses[0] & CS40L26_INT1_ACTUATOR_SAFE_MODE_IRQ_MASK)
    {
        // Handle BST flags
        if (irq_statuses[0] & CS40L26_INT1_BOOST_IRQ_MASK)
        {
            ret = regmap_write(cp, CS40L26_GLOBAL_ENABLES_REG, 0);
            if (ret)
            {
                return ret;
            }
        }
        ret = cs40l26_error_release(driver, CS40L26_BST_ERR_RLS);
        if (ret)
        {
            return ret;
        }

        if (irq_statuses[0] & CS40L26_INT1_BOOST_IRQ_MASK)
        {
            ret = regmap_write(cp, CS40L26_GLOBAL_ENABLES_REG, 1);
            if (ret)
            {
                return ret;
            }
        }
    }

    return CS40L26_STATUS_OK;
}

static uint32_t cs40l26_wseq_write_to_dsp(cs40l26_t *driver)
{
    uint32_t ret;
    uint32_t final_offset;
    uint32_t words[3];
    uint32_t base_reg = fw_img_find_symbol(driver->fw_info, CS40L26_SYM_PM_POWER_ON_SEQUENCE);
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (!base_reg)
    {
        return CS40L26_STATUS_FAIL;
    }

    for (uint32_t i = 0; i < driver->wseq_num_entries; i++)
    {
        switch (driver->wseq_table[i].operation)
        {
            case CS40L26_POWER_SEQ_OP_WRITE_REG_FULL:
                words[0] = (driver->wseq_table[i].address & 0xFFFF0000) >> 16;
                words[1] = ((driver->wseq_table[i].address & 0xFFFF) << 8) |
                           ((driver->wseq_table[i].value & 0xFF000000) >> 24);
                words[2] = (driver->wseq_table[i].value & 0xFFFFFF);

                break;
            case CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8:
                words[0] = (CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8 << 16) |
                           ((driver->wseq_table[i].address & 0xFF) << 8) |
                           ((driver->wseq_table[i].value & 0xFF000000) >> 24);
                words[1] = (driver->wseq_table[i].value & 0xFFFFFF);
                break;
            case CS40L26_POWER_SEQ_OP_WRITE_REG_L16:
                words[0] = (CS40L26_POWER_SEQ_OP_WRITE_REG_L16 << 16) |
                           ((driver->wseq_table[i].address & 0xFFFF00) >> 8);
                words[1] = ((driver->wseq_table[i].address & 0xFF) << 16) |
                            (driver->wseq_table[i].value & 0xFFFF);
                break;
            case CS40L26_POWER_SEQ_OP_WRITE_REG_H16:
                words[0] = (CS40L26_POWER_SEQ_OP_WRITE_REG_H16 << 16) |
                           ((driver->wseq_table[i].address & 0xFFFF00) >> 8);
                words[1] = ((driver->wseq_table[i].address & 0xFF) << 16) |
                            (driver->wseq_table[i].value & 0xFFFF);
                break;
            default:
                break;
        }
        for (uint32_t j = 0; j < driver->wseq_table[i].size; j++)
        {
            ret = regmap_write(cp, base_reg + (4 * (driver->wseq_table[i].offset + j)), words[j]);
            if (ret)
            {
                return ret;
            }
        }
    }
    final_offset = driver->wseq_table[driver->wseq_num_entries].offset + driver->wseq_table[driver->wseq_num_entries].size;
    regmap_write(cp, base_reg + (4 * final_offset), CS40L26_POWER_SEQ_OP_END << 16);
    if (ret)
    {
        return ret;
    }

    driver->wseq_written = true;

    return CS40L26_STATUS_OK;
}

/**
 * Update an existing entry in the wseq_table or add new entry to the table
 * if not already present
 */
static uint32_t cs40l26_wseq_table_update(cs40l26_t *driver, uint32_t address, uint32_t value, uint32_t operation, bool read)
{
    uint32_t ret = CS40L26_STATUS_OK;
    cs40l26_wseq_entry_t *table = driver->wseq_table;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (address < 0xFFFFFFFF)
    {
        uint32_t num_entries = driver->wseq_num_entries;
        bool found = false;

        for (uint32_t i = 0; i < num_entries; i++)
        {
            if (table[i].operation == operation)
            {
                //If the address is in the table already,
                //And the value has been updated:
                //Update the value in the table to match the new value and write the new value to the dsp
                if (table[i].address == address)
                {
                    if (read)
                    {
                        uint32_t full_address = address;
                        if (operation == CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8)
                        {
                            uint32_t prev_address;
                            // Search back for full address, to fetch upper 3 bytes of address
                            for (int j = i; j >= 0; j--)
                            {
                                if (table[j].operation != CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8)
                                {
                                    prev_address = table[j].address;
                                    full_address = (prev_address & 0xFFFFFF00) | table[i].address;
                                    break;
                                }
                            }
                        }
                        regmap_read(cp, full_address, &value);
                    }
                    if (table[i].value != value)
                    {
                        table[i].address = address;
                        table[i].value = value;
                    }
                    found = true;
                }
            }
        }
        //If the address isn't in the table alrady,
        //Attempt to append it to the table. Only fail if some registers have already been written to the dsp.
        //Otherwise, writing to dsp is delayed until next write.
        if (!found)
        {
            if (read)
            {
                uint32_t full_address = address;
                if (operation == CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8)
                {
                    uint32_t prev_address;
                    // Search back for full address, to fetch upper 3 bytes of address
                    for (int j = num_entries; j >= 0; j--)
                    {
                        if (table[j].operation != CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8)
                        {
                            prev_address = table[j].address;
                            full_address = (prev_address & 0xFFFFFF00) | table[num_entries].address;
                            break;
                        }
                    }
                }
                regmap_read(cp, full_address, &value);
            }

            if (num_entries < CS40L26_POWER_SEQ_LENGTH)
            {
                table[num_entries].address = address;
                table[num_entries].value = value;
                switch (operation)
                {
                    case CS40L26_POWER_SEQ_OP_WRITE_REG_FULL:
                        table[num_entries].operation = CS40L26_POWER_SEQ_OP_WRITE_REG_FULL;
                        table[num_entries].size = CS40L26_POWER_SEQ_OP_WRITE_REG_FULL_WORDS;
                        break;
                    case CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8:
                        table[num_entries].operation = CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8;
                        table[num_entries].size = CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8_WORDS;
                        break;
                    case CS40L26_POWER_SEQ_OP_WRITE_REG_L16:
                        table[num_entries].operation = CS40L26_POWER_SEQ_OP_WRITE_REG_L16;
                        table[num_entries].size = CS40L26_POWER_SEQ_OP_WRITE_REG_L16_WORDS;
                        break;
                    case CS40L26_POWER_SEQ_OP_WRITE_REG_H16:
                        table[num_entries].operation = CS40L26_POWER_SEQ_OP_WRITE_REG_H16;
                        table[num_entries].size = CS40L26_POWER_SEQ_OP_WRITE_REG_H16_WORDS;
                        break;
                    default:
                        break;
                }
                if (num_entries > 0)
                {
                    table[num_entries].offset = table[num_entries - 1].offset + table[num_entries - 1].size;
                }
                else
                {
                    table[num_entries].offset = 0;
                }

                driver->wseq_num_entries += 1;
            }
            else
            {
                ret = CS40L26_STATUS_FAIL;
            }
        }
    }

    return ret;
}

static uint32_t cs40l26_wseq_read_from_dsp(cs40l26_t *driver)
{
    uint32_t temp_entry[3];
    uint32_t address;
    uint32_t value;
    uint32_t base_reg = fw_img_find_symbol(driver->fw_info, CS40L26_SYM_PM_POWER_ON_SEQUENCE);
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (!base_reg)
    {
        return CS40L26_STATUS_FAIL;
    }

    for (uint32_t i = 0; i < CS40L26_POWER_SEQ_MAX_WORDS; i++)
    {
        uint32_t operation;
        regmap_read(cp, base_reg + (4 * i), &(temp_entry[0]));

        operation = ((temp_entry[0] & 0xFF0000) >> 16);
        if (operation == CS40L26_POWER_SEQ_OP_END)
        {
            break;
        }
        else
        {
            switch (operation)
            {
                case CS40L26_POWER_SEQ_OP_WRITE_REG_FULL:
                {
                    regmap_read(cp, base_reg + (4 * ++i), &(temp_entry[1]));
                    regmap_read(cp, base_reg + (4 * ++i), &(temp_entry[2]));
                    address = ((temp_entry[0] & 0xFFFF) << 16) |
                           ((temp_entry[1] & 0xFFFF00) >> 8);
                    value = ((temp_entry[1] & 0xFF) << 24) |
                             (temp_entry[2] & 0xFFFFFF);
                    break;
                }
                case CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8:
                {
                    regmap_read(cp, base_reg + (4 * ++i), &(temp_entry[1]));
                    address = (temp_entry[0] & 0xFF00) >> 8;
                    value = ((temp_entry[0] & 0xFF) << 24) |
                             (temp_entry[1] & 0xFFFFFF);
                    break;
                }
                case CS40L26_POWER_SEQ_OP_WRITE_REG_L16:
                {
                    regmap_read(cp, base_reg + (4 * ++i), &(temp_entry[1]));
                    address = ((temp_entry[0] & 0xFFFF) << 8) |
                           ((temp_entry[1] & 0xFF0000) >> 16);
                    value = (temp_entry[1] & 0xFFFF);
                    break;
                }
                default:
                {
                    return CS40L26_STATUS_FAIL;
                }
            }
            cs40l26_wseq_table_update(driver, address, value, operation, true);
        }
    }

    return CS40L26_STATUS_OK;
}

static uint32_t cs40l26_allow_hibernate(cs40l26_t *driver)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write_fw_control(cp, driver->fw_info, CS40L26_SYM_PM_PM_TIMER_TIMEOUT_TICKS, 0);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write_array(cp,(uint32_t *) cs40l26_hibernate_patch, 6);
    if (ret)
    {
        return ret;
    }
    ret = cs40l26_wseq_read_from_dsp(driver);
    if (ret)
    {
        return ret;
    }
    for(int i = 0; i < (sizeof(cs40l26_wseq_reg_list)/sizeof(uint32_t)); i++)
    {
        ret = cs40l26_wseq_table_update(driver, cs40l26_wseq_reg_list[i], 0,
                                        CS40L26_POWER_SEQ_OP_WRITE_REG_FULL, true);
        if (ret)
        {
            return ret;
        }
    }
    ret = cs40l26_wseq_write_to_dsp(driver);
    if (ret)
    {
        return ret;
    }

    ret = cs40l26_pm_state_transition(driver, CS40L26_PM_STATE_ALLOW_HIBERNATE);

    return ret;
}

static uint32_t cs40l26_prevent_hibernate(cs40l26_t *driver)
{
    for(int i = 0; i < CS40L26_WAKE_ATTEMPTS; i++)
    {
        uint32_t ret;
        ret = cs40l26_pm_state_transition(driver, CS40L26_PM_STATE_PREVENT_HIBERNATE);
        if (!ret)
        {
            return ret;
        }
    }
    return cs40l26_unmask_interrupts(driver);
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize driver state/handle
 *
 */
uint32_t cs40l26_initialize(cs40l26_t *driver)
{
    uint32_t ret = CS40L26_STATUS_FAIL;

    if (NULL != driver)
    {
        memset(driver, 0, sizeof(cs40l26_t));

        ret = CS40L26_STATUS_OK;
    }
    return ret;
}

/**
 * Configures driver state/handle
 *
 */
uint32_t cs40l26_configure(cs40l26_t *driver, cs40l26_config_t *config)
{
    uint32_t ret = CS40L26_STATUS_FAIL;

    if ((NULL != driver) && \
        (NULL != config))
    {
        driver->config = *config;

        ret = bsp_driver_if_g->register_gpio_cb(driver->config.bsp_config.int_gpio_id,
                                              &cs40l26_irq_callback,
                                              driver);

        if (ret == BSP_STATUS_OK)
        {
            ret = CS40L26_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Processes driver events and notifications
 *
 */
uint32_t cs40l26_process(cs40l26_t *driver)
{
    // check for driver mode
    if (driver->mode == CS40L26_MODE_HANDLING_EVENTS)
    {
        // run through event handler
        if (CS40L26_STATUS_OK != cs40l26_event_handler(driver))
        {
            driver->event_flags |= CS40L26_EVENT_FLAG_STATE_ERROR;
        }

        driver->mode = CS40L26_MODE_HANDLING_CONTROLS;
    }

    if (driver->event_flags)
    {
        if (driver->config.bsp_config.notification_cb != NULL)
        {
            driver->config.bsp_config.notification_cb(driver->event_flags,
                                                      driver->config.bsp_config.notification_cb_arg);
        }

        driver->event_flags = 0;
    }

    return CS40L26_STATUS_OK;
}

/**
 * Reset the CS40L26
 *
 */
uint32_t cs40l26_reset(cs40l26_t *driver)
{
    uint8_t dsp_state;
    uint32_t halo_state;
    int ret, i;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Drive RESET low for at least T_RLPW (1ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.reset_gpio_id, BSP_GPIO_LOW);
    bsp_driver_if_g->set_timer(CS40L26_T_RLPW_MS, NULL, NULL);

    // Drive RESET high and wait for at least T_IRS (1ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.reset_gpio_id, BSP_GPIO_HIGH);
    bsp_driver_if_g->set_timer(CS40L26_T_IRS_MS, NULL, NULL);

    ret = regmap_read(cp, CS40L26_DEVID, &(driver->devid));
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, CS40L26_REVID, &(driver->revid));
    if (ret)
    {
        return ret;
    }

    for (i = 0; i < 10; i++)
    {
        ret = regmap_read(cp, CS40L26_A1_DSP_HALO_STATE_REG, &halo_state);
        if (ret)
        {
            return ret;
        }

        if (halo_state == CS40L26_DSP_HALO_STATE_RUN)
        {
            break;
        }
        bsp_driver_if_g->set_timer(CS40L26_1_MS, NULL, NULL);
    }

    if(i == 10)
    {
        return CS40L26_STATUS_FAIL;
    }
    ret = cs40l26_pm_state_transition(driver, CS40L26_PM_STATE_PREVENT_HIBERNATE);
    if (ret)
    {
        return ret;
    }

    ret = cs40l26_unmask_interrupts(driver);
    if (ret)
    {
        return ret;
    }

    ret = cs40l26_dsp_state_get(driver, &dsp_state);
    if (ret)
    {
        return ret;
    }

    if (dsp_state != CS40L26_STATE_SHUTDOWN &&
        dsp_state != CS40L26_STATE_STANDBY)
    {
        return CS40L26_STATUS_FAIL;
    }

    return CS40L26_STATUS_OK;
}

/**
 * Finish booting the CS40L26
 *
 */
uint32_t cs40l26_boot(cs40l26_t *driver, fw_img_info_t *fw_info)
{
    uint8_t dsp_state;
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    driver->fw_info = fw_info;
    if (driver->fw_info == NULL)
    {
        ret = regmap_write(cp, CS40L26_DSP1_CCM_CORE_CONTROL, CS40L26_DSP_CCM_CORE_KILL);
        if (ret)
        {
           return ret;
        }

        ret = regmap_write(cp, CS40L26_CALL_RAM_INIT, 1);
        if (ret)
        {
            return ret;
        }

        ret = regmap_update_reg(cp, CS40L26_PWRMGT_CTL, CS40L26_MEM_RDY_MASK, 1 << CS40L26_MEM_RDY_SHIFT);
        if (ret)
        {
            return ret;
        }

        if (driver->config.cal_data.is_valid_f0)
        {
            ret = regmap_write(cp, CS40L26_F0_ESTIMATION_REDC_REG,
                               driver->config.cal_data.redc);
            if (ret)
            {
                return ret;
            }
            ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1,
                               CS40L26_DSP_MBOX_F0_EST);
            if (ret)
            {
                return ret;
            }
        }
        return CS40L26_STATUS_OK;
    }

    ret = regmap_write(cp, CS40L26_DSP1_CCM_CORE_CONTROL, CS40L26_DSP_CCM_CORE_RESET);
    if (ret)
    {
        return ret;
    }

    regmap_write_array(cp, driver->config.syscfg_regs, driver->config.syscfg_regs_total);

    if (driver->revid == CS40L26_REVID_A1)
    {
        ret = regmap_write_array(cp, (uint32_t *) cs40l26_a1_errata, sizeof(cs40l26_a1_errata)/sizeof(uint32_t));
        if (ret)
        {
            return ret;
        }
    }

    ret = cs40l26_dsp_state_get(driver, &dsp_state);
    if (ret)
    {
        return ret;
    }
    if (dsp_state != CS40L26_DSP_HALO_STATE_RUN)
    {
        return CS40L26_STATUS_FAIL;
    }
    return CS40L26_STATUS_OK;
}

/**
 * Change the power state
 *
 */

uint32_t cs40l26_power(cs40l26_t *driver, uint32_t power_state)
{
    uint32_t ret = CS40L26_STATUS_OK;
    uint32_t new_state = driver->power_state;

    switch (power_state)
    {
        case CS40L26_POWER_STATE_ALLOW_HIBERNATE:
            if (power_state != driver->power_state)
            {
                ret = cs40l26_allow_hibernate(driver);
                if (ret)
                {
                    return ret;
                }
                new_state = CS40L26_POWER_STATE_ALLOW_HIBERNATE;
            }
            break;
        case CS40L26_POWER_STATE_PREVENT_HIBERNATE:
            if (power_state != driver->power_state)
            {
                ret = cs40l26_prevent_hibernate(driver);
                if (ret)
                {
                    return ret;
                }
                new_state = CS40L26_POWER_STATE_PREVENT_HIBERNATE;
            }
            break;
        default:
            ret = CS40L26_STATUS_FAIL;
            break;
    }

    if (ret == CS40L26_STATUS_OK)
    {
        driver->power_state = new_state;
    }

    return ret;
}

/**
 * Calibrate the HALO Core haptics processing algorithm
 *
 */
uint32_t cs40l26_calibrate(cs40l26_t *driver)
{
    uint32_t redc, f0;
    uint32_t ret = CS40L26_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, CS40L26_DSP_MBOX_REDC_EST);
    if (ret)
    {
        return ret;
    }

    bsp_driver_if_g->set_timer(20 , NULL, NULL);

    ret = regmap_read(cp, CS40L26_REDC_ESTIMATION_REG, &redc);
    if (ret)
    {
        return ret;
    }

    driver->config.cal_data.redc = redc;
    redc = 0xFF8000 & redc;

    ret = regmap_write(cp, CS40L26_F0_ESTIMATION_REDC_REG, redc);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, CS40L26_DSP_MBOX_F0_EST);
    if (ret)
    {
        return ret;
    }

    for (uint8_t i = 0; i < CS40L26_F0_CALIBRATION_ATTEMPTS; i++)
    {
        bsp_driver_if_g->set_timer(CS40L26_F0_CALIBRATION_DELAY_MS , NULL, NULL);
        ret = regmap_read(cp, CS40L26_F0_ESTIMATION_F0_REG, &f0);
        if (ret)
        {
            return ret;
        }

        if (f0)
        {
            break;
        }
    }

    if (f0 == 0)
    {
        return CS40L26_STATUS_FAIL;
    }

    driver->config.cal_data.f0 = f0;
    driver->config.cal_data.is_valid_f0 = true;

    return CS40L26_STATUS_OK;
}

/**
 * Start I2S Streaming mode
 *
 */
uint32_t cs40l26_start_i2s(cs40l26_t *driver)
{
    uint32_t ret, mbox_rd, data;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write_acked_reg(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, CS40L26_DSP_MBOX_CMD_STOP_PLAYBACK,
                                 CS40L26_DSP_MBOX_RESET, 5, 1);
    if (ret)
    {
        return ret;
    }

    ret = regmap_update_reg(cp, CS40L26_REFCLK_INPUT_REG,
                            CS40L26_REFCLK_PLL_LOOP_MASK,
                            1 << CS40L26_REFCLK_PLL_LOOP_SHIFT);
    if (ret)
    {
        return ret;
    }

    ret = regmap_update_reg(cp, CS40L26_REFCLK_INPUT_REG,
                            CS40L26_PLL_REFCLK_FREQ_MASK, driver->config.bclk_freq);
    if (ret)
    {
        return ret;
    }

    ret = regmap_update_reg(cp, CS40L26_REFCLK_INPUT_REG,
                            CS40L26_PLL_REFCLK_SEL_MASK, 0);
    if (ret)
    {
        return ret;
    }

    ret = regmap_update_reg(cp, CS40L26_REFCLK_INPUT_REG,
                            CS40L26_REFCLK_PLL_LOOP_MASK, 0);
    if (ret)
    {
        return ret;
    }

    cs40l26_dataif_asp_enables1_t asp_reg_val;
    asp_reg_val.word = 0;
    asp_reg_val.asp_rx1_en = 1;
    ret = regmap_write(cp, CS40L26_ASP_ENABLES1, asp_reg_val.word);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write_fw_control(cp, driver->fw_info, CS40L26_SYM_A2H_A2HEN, 1);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write_acked_reg(cp, CS40L26_DSP_VIRTUAL1_MBOX_1,
                                 CS40L26_DSP_MBOX_CMD_START_I2S, CS40L26_DSP_MBOX_RESET, 5, 1);
    if (ret)
    {
        return ret;
    }

    //Polling 10ms for MBOX_HAPTIC_TRIGGER_I2S message
    for (int i = 0; i < 10; i++)
    {
        ret = regmap_read_fw_control(cp, driver->fw_info, CS40L26_SYM_MAILBOX_QUEUE_RD, &mbox_rd);
        if (ret)
        {
            return ret;
        }
        ret = regmap_read(cp, mbox_rd, &data);
        if (ret)
        {
            return ret;
        }
        if (data == CS40L26_DSP_MBOX_HAPTIC_TRIGGER_I2S)
        {
            break;
        }
        bsp_driver_if_g->set_timer(1 , NULL, NULL);
    }

    return ret;
}

uint32_t cs40l26_stop_i2s(cs40l26_t *driver)
{
    uint32_t ret, mbox_rd, data;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write_acked_reg(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, CS40L26_DSP_MBOX_CMD_STOP_I2S,
                                 CS40L26_DSP_MBOX_RESET, 5, 1);
    if (ret)
    {
        return ret;
    }

    ret = regmap_update_reg(cp, CS40L26_ASP_ENABLES1, (0x3 << 16) | 3, 0);
    if (ret)
    {
        return ret;
    }
    ret = regmap_update_reg(cp, CS40L26_ASPTX1_INPUT, 0x3F, CS40L26_DATA_SRC_VMON);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write_fw_control(cp, driver->fw_info, CS40L26_SYM_A2H_A2HEN, 0);
    if (ret)
    {
        return ret;
    }
    ret = regmap_update_reg(cp, CS40L26_REFCLK_INPUT_REG,
                            CS40L26_REFCLK_PLL_LOOP_MASK, 1 << 11);
    if (ret)
    {
        return ret;
    }
    ret = regmap_update_reg(cp, CS40L26_REFCLK_INPUT_REG, CS40L26_PLL_REFCLK_FREQ_MASK |
                            CS40L26_PLL_REFCLK_SEL_MASK, 0);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write(cp, CS40L26_REFCLK_INPUT_REG, 0x815);
    if (ret)
    {
        return ret;
    }
    ret = regmap_update_reg(cp, CS40L26_REFCLK_INPUT_REG,
                            CS40L26_REFCLK_PLL_LOOP_MASK, 0);
    if (ret)
    {
        return ret;
    }
    for (int i = 0; i < 10; i++)
    {
        ret = regmap_read_fw_control(cp, driver->fw_info, CS40L26_SYM_MAILBOX_QUEUE_RD, &mbox_rd);
        if (ret)
        {
            return ret;
        }
        ret = regmap_read(cp, mbox_rd, &data);
        if (ret)
        {
            return ret;
        }
        if (data == CS40L26_DSP_MBOX_HAPTIC_COMPLETE_I2S)
        {
            return CS40L26_STATUS_OK;
        }
        bsp_driver_if_g->set_timer(1 , NULL, NULL);
    }
    return ret;
}
/**
 * Set buzzgen waveform
 *
 */
uint32_t cs40l26_buzzgen_set(cs40l26_t *driver, uint16_t freq,
                             uint16_t level, uint16_t duration, uint8_t buzzgen_num)
{
    uint32_t ret, base_reg, freq_reg, level_reg, duration_reg;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (buzzgen_num > CS40L26_CMD_MAX_INDEX_BUZZ_WAVE)
    {
        return CS40L26_STATUS_FAIL;
    }

    base_reg = fw_img_find_symbol(driver->fw_info, CS40L26_SYM_BUZZGEN_BUZZ_EFFECTS1_BUZZ_FREQ);
    if (base_reg == 0)
    {
        return CS40L26_STATUS_FAIL;
    }

    freq_reg = base_reg
               + ((buzzgen_num) * CS40L26_BUZZGEN_CONFIG_OFFSET);
    level_reg = base_reg
                + ((buzzgen_num) * CS40L26_BUZZGEN_CONFIG_OFFSET)
                + CS40L26_BUZZGEN_LEVEL_OFFSET;
    duration_reg = base_reg
                   + ((buzzgen_num) * CS40L26_BUZZGEN_CONFIG_OFFSET)
                   + CS40L26_BUZZGEN_DURATION_OFFSET;

    ret = regmap_write(cp, freq_reg, freq);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, level_reg, level);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, duration_reg, duration);

    return ret;
}

uint32_t cs40l26_load_waveform(cs40l26_t *driver)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1,
                       CS40L26_DSP_MBOX_CMD_OWT_RESET);

    return ret;
}

/**
 * Trigger haptic effect
 *
 */
uint32_t cs40l26_trigger(cs40l26_t *driver, uint32_t index, cs40l26_wavetable_bank_t bank)
{
    uint32_t ret, wf_index;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    switch (bank)
    {
        case RAM_BANK:
            if (index > CS40L26_CMD_MAX_INDEX_RAM_WAVE)
            {
                return CS40L26_STATUS_FAIL;
            }
            wf_index = CS40L26_CMD_INDEX_RAM_WAVE | index;
            break;

        case ROM_BANK:
            if (index > CS40L26_CMD_MAX_INDEX_ROM_WAVE)
            {
                return CS40L26_STATUS_FAIL;
            }
            wf_index = CS40L26_CMD_INDEX_ROM_WAVE | index;
            break;

        case BUZZ_BANK:
            if (index > CS40L26_CMD_MAX_INDEX_BUZZ_WAVE)
            {
                return CS40L26_STATUS_FAIL;
            }
            wf_index = CS40L26_CMD_INDEX_BUZZ_WAVE | index;
            break;

        case OWT_BANK:
            if (index > CS40L26_CMD_MAX_INDEX_OWT_WAVE)
            {
                return CS40L26_STATUS_FAIL;
            }
            wf_index = CS40L26_CMD_INDEX_OWT_WAVE | index;
            break;

        default:
            return CS40L26_STATUS_FAIL;
    }

    ret = regmap_write(cp, CS40L26_DSP_VIRTUAL1_MBOX_1, wf_index);

    return ret;
}
