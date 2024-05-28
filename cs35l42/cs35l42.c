/**
 * @file cs35l42.c
 *
 * @brief The CS35L42 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022, 2024 All Rights Reserved, http://www.cirrus.com/
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
#include "cs35l42.h"
#include "cs35l42_sym.h"
#include "string.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * @defgroup CS35L42_DSP_MBOX_CMD_
 * @brief HALO DSP Mailbox commands
 *
 * @{
 */
#define CS35L42_DSP_MBOX_CMD_NONE               0x00000000
#define CS35L42_DSP_MBOX_CMD_AUDIO_PLAY         0x0B000001
#define CS35L42_DSP_MBOX_CMD_AUDIO_PAUSE        0x0B000002
#define CS35L42_DSP_MBOX_CMD_AUDIO_REINIT       0x0B000003

#define CS35L42_DSP_MBOX_CMD_HIBERNATE          0x02000001
#define CS35L42_DSP_MBOX_CMD_WAKEUP             0x02000002
#define CS35L42_DSP_MBOX_CMD_PREVENT_HIBERNATE  0x02000003
#define CS35L42_DSP_MBOX_CMD_ALLOW_HIBERNATE    0x02000004
#define CS35L42_DSP_MBOX_CMD_SHUTDOWN           0x02000005
/** @} */

/**
 * @defgroup CS35L42_DSP_MBOX_STATUS_
 * @brief Statuses of the HALO DSP Mailbox
 *
 * @{
 */
#define CS35L42_DSP_MBOX_STATUS_AWAKE           0x02000002
/** @} */

/**
 * @defgroup CS35L42_DSP_PM_CUR_STATUS_
 * @brief Statuses of the HALO firmware
 *
 * @{
 */
#define CS35L42_DSP_PM_CUR_STATUS_HIBERNATE     (0)
#define CS35L42_DSP_PM_CUR_STATUS_SHUTDOWN      (1)
#define CS35L42_DSP_PM_CUR_STATUS_STANDBY       (2)
#define CS35L42_DSP_PM_CUR_STATUS_ACTIVE        (3)
/** @} */

/**
 * Value of CS35L42_CAL_STATUS that indicates Calibration success
 *
 * @see CS35L42_CAL_STATUS
 *
 */
#define CS35L42_CAL_STATUS_CALIB_ERROR              (0x0)
#define CS35L42_CAL_STATUS_CALIB_SUCCESS            (0x1)
#define CS35L42_CAL_STATUS_CALIB_WAITING_FOR_DATA   (0x2)
#define CS35L42_CAL_STATUS_CALIB_OUT_OF_RANGE       (0x3)

/**
 * IRQ1 Status Bits for Speaker Safe Mode
 *
 * If any of the bits in the mask below are set in IRQ1_EINT_1, the amplifier will have entered Speaker Safe Mode.
 * - b27 - AMP_ERR_MASK1
 * - b26 - TEMP_ERR_MASK1
 * - b22 - BST_SHORT_ERR_MASK1
 * - b21 - BST_DCM_UVP_ERR_MASK1
 * - b20 - BST_OVP_ERR_MASK1
 *
 * @see IRQ1_EINT_1
 * @see Datasheet Section 4.14.1.1
 *
 */
#define CS35L42_INT1_SPEAKER_SAFE_MODE_IRQ_MASK (0x0C700000)

/**
 * IRQ1 Status Bits for Speaker Safe Mode Boost-related Events
 *
 * If any of the bits in the mask below are set in IRQ1_EINT_1, the amplifier will have entered Speaker Safe Mode
 * and will require additional steps to release from Speaker Safe Mode.
 * - b22 - BST_SHORT_ERR_MASK1
 * - b21 - BST_DCM_UVP_ERR_MASK1
 * - b20 - BST_OVP_ERR_MASK1
 *
 * @see IRQ1_EINT_1
 * @see Datasheet Section 4.14.1.1
 *
 */
#define CS35L42_INT1_BOOST_IRQ_MASK             (0x00700000)

/**
 * Toggle Mask for MSM_ERROR_RELEASE_REG to Release from Speaker Safe Mode
 *
 * The relevant fields in MSM_ERROR_RELEASE_REG that require release sequence are:
 * - b6 - TEMP_ERR
 * - b5 - TEMP_WARN
 * - b4 - BST_UVP
 * - b3 - BST_OVP
 * - b2 - BST_SHORT
 * - b1 - AMP_SHORT
 *
 * @see ERROR_RELEASE
 * @see Datasheet Section 4.14.1.1
 *
 */
#define CS35L42_ERR_RLS_SPEAKER_SAFE_MODE_MASK  (0x0000007E)
/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/**
 * CS35L42 non-default DSP io
 *
 */
static const uint32_t cs35l42_dsp_io[] =
{
    CS35L42_DSP1RX1_INPUT, 0x00000008, /* DSP input 1 is ASPRX1 */
    CS35L42_DSP1RX2_INPUT, 0x00000009, /* DSP input 2 is ASPRX2 */
    CS35L42_DSP1RX5_INPUT, 0x00000019, /* DSP input 5 is imon */
    CS35L42_DSP1RX6_INPUT, 0x00000018, /* DSP input 6 is vmon */
    CS35L42_DACPCM1_INPUT, 0x00000036, /* DSP 48kHz output */
};

/**
 * Register addresses for write sequencer.
 *
 * Used just before hibernation.
 *
 * List is in the form:
 * - word0 - Address of first configuration register
 * - word1 - Address of second configuration register
 * - ...
 *
 * @see cs35l42_power_up
 *
 */
static const uint32_t cs35l42_hibernate_update_regs[CS35L42_POWER_SEQ_LENGTH] = {
    CS35L42_ASPTX1_INPUT,
    CS35L42_ASPTX2_INPUT,
    CS35L42_ASPTX3_INPUT,
    CS35L42_ASPTX4_INPUT,
    CS35L42_DSP1RX1_INPUT,
    CS35L42_DSP1RX2_INPUT,
    CS35L42_DACPCM1_INPUT,
    CS35L42_AMP_CTRL,
    CS35L42_AMP_GAIN,
    CS35L42_GLOBAL_SAMPLE_RATE,
    CS35L42_ASP_CONTROL2,
    CS35L42_ASP_DATA_CONTROL1,
    CS35L42_ASP_DATA_CONTROL5,
    CS35L42_GPIO_PAD_CONTROL,
    CS35L42_VBST_CTL_1,
    CS35L42_VBST_CTL_2,
    CS35L42_NG_CONFIG,
    CS35L42_REFCLK_INPUT,
    CS35L42_ASP_ENABLES1,
    CS35L42_ASP_CONTROL3,
};

/**
 * Mapping of CS35L42 IRQ Flag to Event Flag
 *
 * List is in the form:
 * - word0 - IRQ Flag
 * - word1 - Event Flag
 * - ...
 *
 * @see cs35l42_irq_to_event_id
 *
 */
uint32_t cs35l42_irq_to_event_flag_map[] =
{
    CS35L42_AMP_ERR_EINT1_MASK,                 CS35L42_EVENT_FLAG_AMP_ERR,
    CS35L42_TEMP_ERR_EINT1_MASK,                CS35L42_EVENT_FLAG_TEMP_ERR,
    CS35L42_BST_SHORT_ERR_EINT1_MASK,           CS35L42_EVENT_FLAG_BST_SHORT_ERR,
    CS35L42_BST_DCM_UVP_ERR_EINT1_MASK,         CS35L42_EVENT_FLAG_BST_DCM_UVP_ERR,
    CS35L42_BST_OVP_ERR_EINT1_MASK,             CS35L42_EVENT_FLAG_BST_OVP_ERR,
    CS35L42_DSP_VIRTUAL2_MBOX_WR_EINT1_MASK,    CS35L42_EVENT_FLAG_DSP_VIRTUAL2_MBOX_WR,
    CS35L42_WKSRC_STATUS6_EINT1_MASK,           CS35L42_EVENT_FLAG_WKSRC_STATUS6,
    CS35L42_WKSRC_STATUS_ANY_EINT1_MASK,        CS35L42_EVENT_FLAG_WKSRC_STATUS_ANY
};

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
    uint32_t flags = 0;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Unmask selected interrupts
    for (uint32_t i = 0; i < (sizeof(cs35l42_irq_to_event_flag_map)/sizeof(uint32_t)); i+=2)
    {
        flags |= cs35l42_irq_to_event_flag_map[i];
    }

    ret = regmap_update_reg(cp, CS35L42_IRQ1_MASK_1, flags, 0);

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
 * @param [in] driver            Pointer to the driver state
 *
 * @return
 * - CS35L42_STATUS_FAIL         Control port activity fails
 * - CS35L42_STATUS_OK           otherwise
 *
 * @see CS35L42_EVENT_FLAG_
 * @see cs35l42_notification_callback_t
 *
 */
static uint32_t cs35l42_event_handler(void *driver)
{
    uint32_t ret = CS35L42_STATUS_OK;
    uint32_t irq_statuses;
    uint32_t irq_masks;
    uint32_t flags_to_clear = 0;

    cs35l42_t *d = driver;
    regmap_cp_config_t *cp = REGMAP_GET_CP(d);

    ret = regmap_read(cp, CS35L42_IRQ1_EINT_1, &irq_statuses);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, CS35L42_IRQ1_MASK_1, &irq_masks);
    if (ret)
    {
        return ret;
    }

    flags_to_clear = irq_statuses & ~(irq_masks);

    // If there are unmasked IRQs, then process
    if (flags_to_clear)
    {
        // Clear any IRQ1 flags from first register
        ret = regmap_write(cp, CS35L42_IRQ1_EINT_1, flags_to_clear);
        if (ret)
        {
            return ret;
        }
    }

    // IF there are no Boost-related Errors but are Speaker-Safe Mode errors, proceed to TOGGLE_ERR_RLS
    if (irq_statuses & CS35L42_INT1_SPEAKER_SAFE_MODE_IRQ_MASK)
    {
        // If there are Boost-related Errors, proceed to DISABLE_BOOST
        if (irq_statuses & CS35L42_INT1_BOOST_IRQ_MASK)
        {
            // Disable Boost converter
            ret = regmap_update_reg(cp, CS35L42_BLOCK_ENABLES, CS35L42_BST_EN_MASK, 0);
            if (ret)
            {
                return ret;
            }
        }

        // Clear the Error Release register
        ret = regmap_write(cp, CS35L42_ERROR_RELEASE, 0);
        if (ret)
        {
            return ret;
        }
        // Set the Error Release register
        ret = regmap_write(cp, CS35L42_ERROR_RELEASE, CS35L42_ERR_RLS_SPEAKER_SAFE_MODE_MASK);
        if (ret)
        {
            return ret;
        }
        // Clear the Error Release register
        ret = regmap_write(cp, CS35L42_ERROR_RELEASE, 0);
        if (ret)
        {
            return ret;
        }

        // If there are Boost-related Errors, re-enable Boost
        if (irq_statuses & CS35L42_INT1_BOOST_IRQ_MASK)
        {
            // Re-enable Boost Converter
            ret = regmap_update_reg(cp, CS35L42_BLOCK_ENABLES, CS35L42_BST_EN_MASK, 0x2 << CS35L42_BST_EN_SHIFT);
            if (ret)
            {
                return ret;
            }
        }
    }

    // Set event flags
    for (uint8_t i = 0; i < (sizeof(cs35l42_irq_to_event_flag_map)/sizeof(uint32_t)); i+= 2)
    {
        if (irq_statuses & cs35l42_irq_to_event_flag_map[i])
        {
            d->event_flags |= (1 << cs35l42_irq_to_event_flag_map[i + 1]);
        }
    }

    return CS35L42_STATUS_OK;
}

static uint32_t cs35l42_wseq_write_to_dsp(cs35l42_t *driver)
{
    uint32_t ret;
    uint32_t final_offset;
    uint32_t words[3];
    uint32_t base_reg = fw_img_find_symbol(driver->fw_info, CS35L42_SYM_PM_POWER_ON_SEQUENCE);
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (!base_reg)
    {
        return CS35L42_STATUS_FAIL;
    }

    for (uint32_t i = 0; i < driver->wseq_num_entries; i++)
    {
        switch (driver->wseq_table[i].operation)
        {
            case CS35L42_POWER_SEQ_OP_WRITE_REG_FULL:
                words[0] = (driver->wseq_table[i].address & 0xFFFF0000) >> 16;
                words[1] = ((driver->wseq_table[i].address & 0xFFFF) << 8) |
                           ((driver->wseq_table[i].value & 0xFF000000) >> 24);
                words[2] = (driver->wseq_table[i].value & 0xFFFFFF);

                break;
            case CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8:
                words[0] = (CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8 << 16) |
                           ((driver->wseq_table[i].address & 0xFF) << 8) |
                           ((driver->wseq_table[i].value & 0xFF000000) >> 24);
                words[1] = (driver->wseq_table[i].value & 0xFFFFFF);
                break;
            case CS35L42_POWER_SEQ_OP_WRITE_REG_L16:
                words[0] = (CS35L42_POWER_SEQ_OP_WRITE_REG_L16 << 16) |
                           ((driver->wseq_table[i].address & 0xFFFF00) >> 8);
                words[1] = ((driver->wseq_table[i].address & 0xFF) << 16) |
                            (driver->wseq_table[i].value & 0xFFFF);
                break;
            case CS35L42_POWER_SEQ_OP_WRITE_REG_H16:
                words[0] = (CS35L42_POWER_SEQ_OP_WRITE_REG_H16 << 16) |
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
    regmap_write(cp, base_reg + (4 * final_offset), CS35L42_POWER_SEQ_OP_END << 24);
    if (ret)
    {
        return ret;
    }

    driver->wseq_written = true;

    return CS35L42_STATUS_OK;
}

/**
 * Update an existing entry in the wseq_table or add new entry to the table
 * if not already present
 */
static uint32_t cs35l42_wseq_table_update(cs35l42_t *driver, uint32_t address, uint32_t value, uint32_t operation, bool read)
{
    uint32_t ret = CS35L42_STATUS_OK;
    cs35l42_wseq_entry_t *table = driver->wseq_table;
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
                        uint32_t prev_address;
                        uint32_t full_address = address;
                        if (operation == CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8)
                        {
                            // Search back for full address, to fetch upper 3 bytes of address
                            for (int j = i; j >= 0; j--)
                            {
                                if (table[j].operation != CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8)
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
                uint32_t prev_address;
                uint32_t full_address = address;
                if (operation == CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8)
                {
                    // Search back for full address, to fetch upper 3 bytes of address
                    for (int j = num_entries; j >= 0; j--)
                    {
                        if (table[j].operation != CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8)
                        {
                            prev_address = table[j].address;
                            full_address = (prev_address & 0xFFFFFF00) | table[num_entries].address;
                            break;
                        }
                    }
                }
                regmap_read(cp, full_address, &value);
            }

            if (num_entries < CS35L42_POWER_SEQ_LENGTH)
            {
                table[num_entries].address = address;
                table[num_entries].value = value;
                switch (operation)
                {
                    case CS35L42_POWER_SEQ_OP_WRITE_REG_FULL:
                        table[num_entries].operation = CS35L42_POWER_SEQ_OP_WRITE_REG_FULL;
                        table[num_entries].size = CS35L42_POWER_SEQ_OP_WRITE_REG_FULL_WORDS;
                        break;
                    case CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8:
                        table[num_entries].operation = CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8;
                        table[num_entries].size = CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8_WORDS;
                        break;
                    case CS35L42_POWER_SEQ_OP_WRITE_REG_L16:
                        table[num_entries].operation = CS35L42_POWER_SEQ_OP_WRITE_REG_L16;
                        table[num_entries].size = CS35L42_POWER_SEQ_OP_WRITE_REG_L16_WORDS;
                        break;
                    case CS35L42_POWER_SEQ_OP_WRITE_REG_H16:
                        table[num_entries].operation = CS35L42_POWER_SEQ_OP_WRITE_REG_H16;
                        table[num_entries].size = CS35L42_POWER_SEQ_OP_WRITE_REG_H16_WORDS;
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
                ret = CS35L42_STATUS_FAIL;
            }
        }
    }

    return ret;
}

static uint32_t cs35l42_wseq_read_from_dsp(cs35l42_t *driver)
{
    uint32_t temp_entry[3];
    uint32_t address;
    uint32_t value;
    uint32_t operation;
    uint32_t base_reg = fw_img_find_symbol(driver->fw_info, CS35L42_SYM_PM_POWER_ON_SEQUENCE);
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (!base_reg)
    {
        return CS35L42_STATUS_FAIL;
    }

    for (uint32_t i = 0; i < CS35L42_POWER_SEQ_MAX_WORDS; i++)
    {
        regmap_read(cp, base_reg + (4 * i), &(temp_entry[0]));

        operation = ((temp_entry[0] & 0xFF0000) >> 16);
        if (operation == CS35L42_POWER_SEQ_OP_END)
        {
            break;
        }
        else
        {
            switch (operation)
            {
                case CS35L42_POWER_SEQ_OP_WRITE_REG_FULL:
                {
                    regmap_read(cp, base_reg + (4 * ++i), &(temp_entry[1]));
                    regmap_read(cp, base_reg + (4 * ++i), &(temp_entry[2]));
                    address = ((temp_entry[0] & 0xFFFF) << 16) |
                           ((temp_entry[1] & 0xFFFF00) >> 8);
                    value = ((temp_entry[1] & 0xFF) << 24) |
                             (temp_entry[2] & 0xFFFFFF);
                    break;
                }
                case CS35L42_POWER_SEQ_OP_WRITE_REG_ADDR8:
                {
                    regmap_read(cp, base_reg + (4 * ++i), &(temp_entry[1]));
                    address = (temp_entry[0] & 0xFF00) >> 8;
                    value = ((temp_entry[0] & 0xFF) << 24) |
                             (temp_entry[1] & 0xFFFFFF);
                    break;
                }
                case CS35L42_POWER_SEQ_OP_WRITE_REG_L16:
                {
                    regmap_read(cp, base_reg + (4 * ++i), &(temp_entry[1]));
                    address = ((temp_entry[0] & 0xFFFF) << 8) |
                           ((temp_entry[1] & 0xFF0000) >> 16);
                    value = (temp_entry[1] & 0xFFFF);
                    break;
                }
                default:
                {
                    return CS35L42_STATUS_FAIL;
                }
            }
            cs35l42_wseq_table_update(driver, address, value, operation, true);
        }
    }

    return CS35L42_STATUS_OK;
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

    //If the DSP is booted
    if (driver->state != CS35L42_STATE_STANDBY)
    {
        // Set MEM_RDY
        ret = regmap_write(cp, CS35L42_PWRMGT_CTL, CS35L42_MEM_RDY_MASK);
        if (ret)
        {
            return ret;
        }
        // Enable clocks to HALO DSP core
        ret = regmap_update_reg(cp, CS35L42_DSP1_CCM_CORE_CONTROL, CS35L42_DSP1_CCM_CORE_EN_MASK, 1 << CS35L42_DSP1_CCM_CORE_EN_SHIFT);
        if (ret)
        {
            return ret;
        }
    }

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
            return CS35L42_STATUS_FAIL;
        }
    } while ((temp_reg_val & CS35L42_MSM_PUP_DONE_EINT1_MASK) == 0);

    // Clear MSM_PUP_DONE IRQ flag
    ret = regmap_write(cp, CS35L42_IRQ1_EINT_1, 1 << CS35L42_MSM_PUP_DONE_EINT1_SHIFT);
    if (ret)
    {
        return ret;
    }

    // If DSP is NOT booted, then power up is finished
    if (driver->state == CS35L42_STATE_STANDBY)
    {
        return CS35L42_STATUS_OK;
    }

    // If calibration data is valid
    if (driver->config.cal_data.is_valid)
    {
        ret = regmap_write_fw_control(cp,
                                      driver->fw_info,
                                      CS35L42_SYM_PROTECT_LITE_RE_CALIB_SELECTOR_CMPST_0_RECALIBSELECTOR_0_SEL_RE_CAL,
                                      driver->config.cal_data.r);
        if (ret)
        {
            return ret;
        }
        ret = regmap_write(cp, CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_AUDIO_REINIT);
        if (ret)
        {
            return ret;
        }
    }

    // Start playback
    ret = regmap_write(cp, CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_AUDIO_PLAY);
    if (ret)
    {
        return ret;
    }

    if (driver->config.cal_data.is_valid)
    {
        bsp_driver_if_g->set_timer(50, NULL, NULL); // allow CAL_R value to be acted upon once the audio is in PLAY mode
        // Verify calibration
        ret = regmap_read_fw_control(cp,
                                     driver->fw_info,
                                     CS35L42_SYM_PROTECT_LITE_VAR_ARRAY_INITIAL_CALI_IMPEDANCE,
                                     &temp_reg_val);
        if (ret)
        {
            return ret;
        }

        if (temp_reg_val != driver->config.cal_data.r)
        {
            return CS35L42_STATUS_FAIL;
        }
    }

    // Check for correct state
    ret = regmap_read_fw_control(cp, driver->fw_info, CS35L42_SYM_PM_PM_CUR_STATE, &temp_reg_val);
    if (ret)
    {
        return ret;
    }
    if (temp_reg_val == CS35L42_DSP_PM_CUR_STATUS_ACTIVE)
    {
        return CS35L42_STATUS_OK;
    }
    else
    {
        return CS35L42_STATUS_FAIL;
    }
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
 * - CS35L42_STATUS_FAIL if:
 *      - Control port activity fails
 * - CS35L42_STATUS_OK          otherwise
 *
 */
 static uint32_t cs35l42_power_down(cs35l42_t *driver)
{
    uint32_t ret;
    uint32_t temp_reg_val;
    uint32_t iter_timeout = 0;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    //If the DSP is booted
    if (driver->state != CS35L42_STATE_POWER_UP)
    {
        // Send HALO DSP MBOX 'Pause' Command
        ret = regmap_write(cp, CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_AUDIO_PAUSE);
        if (ret)
        {
            return ret;
        }
        // Wait for audio ramp down
        do
        {
            bsp_driver_if_g->set_timer(10, NULL, NULL);
            // Read PM_CUR_STATE
            ret = regmap_read_fw_control(cp, driver->fw_info, CS35L42_SYM_PM_PM_CUR_STATE, &temp_reg_val);
            if (ret)
            {
                return ret;
            }
            iter_timeout++;
            if (iter_timeout > 30) // ~244mS
            {
                return CS35L42_STATUS_FAIL;
            }
        } while (temp_reg_val != CS35L42_DSP_PM_CUR_STATUS_STANDBY);
    }

    //Clear GLOBAL_EN
    ret = regmap_update_reg(cp, CS35L42_GLOBAL_ENABLES, CS35L42_GLOBAL_EN_MASK, 0);
    if (ret)
    {
        return ret;
    }

    // Wait for MSM_PDN_DONE_EINT1 in IRQ1_EINT_1 (Sticky Interrupt Status) to be set
    iter_timeout = 0;
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
    ret = regmap_write(cp, CS35L42_IRQ1_EINT_1, 1 << CS35L42_MSM_PDN_DONE_EINT1_SHIFT);
    if (ret)
    {
        return ret;
    }

    //Clear BLOCK_ENABLES
    ret = regmap_write(cp, CS35L42_BLOCK_ENABLES, 0);
    return ret;
}

/**
 * Apply configuration specifically required after loading HALO FW/COEFF files
 *
 * @param [in] driver            Pointer to the driver state
 *
 * @return
 * - CS35L42_STATUS_FAIL         Control port activity fails
 * - CS35L42_STATUS_OK           otherwise
 *
 */
static uint32_t cs35l42_write_post_boot_config(cs35l42_t *driver)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Write first post-boot configuration
    ret = regmap_write_array(cp, (uint32_t *) cs35l42_dsp_io,
                             (sizeof(cs35l42_dsp_io)/sizeof(uint32_t)));
    if (ret)
    {
        return CS35L42_STATUS_FAIL;
    }

    // Set power down timer to minimum
    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PM_PM_TIMER_TIMEOUT_TICKS, 1);
    if (ret)
    {
        return ret;
    }

    return ret;
}

/**
 * Wakes device from hibernate
 *
 * @param [in] driver            Pointer to the driver state
 *
 * @return
 * - CS35L42_STATUS_FAIL         Control port activity fails
 * - CS35L42_STATUS_OK           otherwise
 *
 */
static uint32_t cs35l42_wake(cs35l42_t *driver)
{
    uint32_t ret;
    uint32_t retries = 2;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    while (retries > 0)
    {
        ret = regmap_write(cp, CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_WAKEUP);
        if (ret)
        {
            retries -= 1;
            bsp_driver_if_g->set_timer(10, NULL, NULL);
        }
        else
        {
            break;
        }
    }
    if (retries == 0)
    {
        return CS35L42_STATUS_FAIL;
    }

    ret = regmap_write(cp, CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_PREVENT_HIBERNATE);
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

    // Write all post-boot configs
    ret = cs35l42_write_post_boot_config(driver);
    if (ret)
    {
        return ret;
    }

    // Unmask interrupts
    ret = cs35l42_unmask_irqs(driver);

    return ret;
}

/**
 * Puts device into hibernate
 *
 * @param [in] driver            Pointer to the driver state
 *
 * @return
 * - CS35L42_STATUS_FAIL         Control port activity fails
 * - CS35L42_STATUS_OK           otherwise
 *
 */
static uint32_t cs35l42_hibernate(cs35l42_t *driver)
{
    uint32_t ret;
    uint32_t cs35l42_hibernate_patch[] =
    {
            CS35L42_IRQ1_MASK_1, 0xFFFFFFFF,
            CS35L42_WAKESRC_CTL, 0x0400, // wake source = I2C
            CS35L42_WAKESRC_CTL, 0x8400,
            CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_ALLOW_HIBERNATE,
            CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_HIBERNATE
    };
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Parse init contents of the POWER_ON_SEQUENCE
    ret = cs35l42_wseq_read_from_dsp(driver);
    if (ret)
    {
        return ret;
    }

    // Add driver-controlled registers to the sequence
    for (uint32_t i = 0; i < (sizeof(cs35l42_hibernate_update_regs)/sizeof(uint32_t)); i++)
    {
        if (cs35l42_hibernate_update_regs[i] == 0)
        {
            break;
        }
        ret = cs35l42_wseq_table_update(driver, cs35l42_hibernate_update_regs[i], 0, CS35L42_POWER_SEQ_OP_WRITE_REG_FULL, true);
        if (ret)
        {
            return ret;
        }
    }

    if(!driver->wseq_written)
    {
        ret = cs35l42_wseq_write_to_dsp(driver);
        if (ret)
        {
            return ret;
        }
    }

    ret = regmap_write_array(cp, (uint32_t *) cs35l42_hibernate_patch, (sizeof(cs35l42_hibernate_patch)/sizeof(uint32_t)));
    return ret;
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

    if (driver->state == CS35L42_STATE_ERROR)
    {
        return CS35L42_STATUS_FAIL;
    }
    else
    {
        return CS35L42_STATUS_OK;
    }
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

    ret = regmap_write(cp, CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_PREVENT_HIBERNATE);
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
    uint32_t ret;
    driver->fw_info = fw_info;

    // Initializing fw_info is okay, but do not proceed
    if (driver->fw_info == NULL)
    {
        return CS35L42_STATUS_OK;
    }

    // Write all post-boot configs
    ret = cs35l42_write_post_boot_config(driver);
    if (ret)
    {
        return ret;
    }

    driver->state = CS35L42_STATE_DSP_STANDBY;

    return CS35L42_STATUS_OK;
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
            if ((driver->state == CS35L42_STATE_STANDBY) ||
                (driver->state == CS35L42_STATE_DSP_STANDBY))
            {
                fp = &cs35l42_power_up;

                if (driver->state == CS35L42_STATE_STANDBY)
                {
                    next_state = CS35L42_STATE_POWER_UP;
                }
                else
                {
                    next_state = CS35L42_STATE_DSP_POWER_UP;
                }
            }
            break;

        case CS35L42_POWER_DOWN:
            if ((driver->state == CS35L42_STATE_POWER_UP) ||
                (driver->state == CS35L42_STATE_DSP_POWER_UP))
            {
                fp = &cs35l42_power_down;

                if (driver->state == CS35L42_STATE_POWER_UP)
                {
                    next_state = CS35L42_STATE_STANDBY;
                }
                else
                {
                    next_state = CS35L42_STATE_DSP_STANDBY;
                }
            }
            break;

        case CS35L42_POWER_HIBERNATE:
            if (driver->state == CS35L42_STATE_DSP_STANDBY)
            {
                fp = &cs35l42_hibernate;
                next_state = CS35L42_STATE_HIBERNATE;
            }
            break;

        case CS35L42_POWER_WAKE:
            if (driver->state == CS35L42_STATE_HIBERNATE)
            {
                fp = &cs35l42_wake;
                next_state = CS35L42_STATE_DSP_STANDBY;
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
uint32_t cs35l42_calibrate(cs35l42_t *driver, uint32_t ambient_temp_deg_c, uint32_t expected_redc)
{
    uint32_t temp_reg_val;
    uint32_t orig_threshold;
    uint32_t iter_timeout = 0;
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_AUDIO_PAUSE);
    if (ret)
    {
        return ret;
    }

    if (expected_redc != CS35L42_CAL_IGNORE_EXPECTED_REDC) // Can ignore setting reference redc if value is CS35L42_CAL_IGNORE_EXPECTED_REDC
    {
        // Set expected ReDC value
        ret = regmap_write_fw_control(cp,
                                      driver->fw_info,
                                      CS35L42_SYM_PROTECT_LITE_R_CALIB_0_R_REF,
                                      expected_redc);
        if (ret)
        {
            return ret;
        }
    }

    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_PROTECT_LITE_CTRL_PROTECT_LITE_ENABLE, 0);
    if (ret)
    {
        return ret;
    }

    // Set the Ambient Temp (deg C)
    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_CALIB_DIAG_VAR_ARRAY_CAL_AMBIENT_TEMPERATURE, ambient_temp_deg_c);
    if (ret)
    {
        return ret;
    }

    // Apply mixer settings
    ret = regmap_read_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_PROTECT_PILOT_TONE_PEART_CMPST_0_SINEGENERATORSENSE_0_THRESHOLD, &orig_threshold);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_PROTECT_PILOT_TONE_PEART_CMPST_0_SINEGENERATORSENSE_0_THRESHOLD, 0);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_R_CALIB_0_FIRST_RUN, 1);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_PROTECT_LITE_CTRL_CALIBRATION_ENABLE, 1);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_PROTECT_LITE_CTRL_PROTECT_LITE_ENABLE, 1);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_AUDIO_REINIT);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write(cp, CS35L42_DSP_VIRTUAL1_MBOX_1, CS35L42_DSP_MBOX_CMD_AUDIO_PLAY);
    if (ret)
    {
        return ret;
    }

    // Wait for calibration sequence to finish
    do
    {
        bsp_driver_if_g->set_timer(100, NULL, NULL);

        ret = regmap_read_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_VAR_ARRAY_IMPEDANCE_MEASURE_STATUS, &temp_reg_val);
        if (ret)
        {
            return ret;
        }
        iter_timeout++;
        if ((iter_timeout > 30) || (temp_reg_val == CS35L42_CAL_STATUS_CALIB_ERROR))
        {
            return CS35L42_STATUS_FAIL;
        }
    } while (temp_reg_val == CS35L42_CAL_STATUS_CALIB_WAITING_FOR_DATA);

    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_PROTECT_LITE_CTRL_PROTECT_LITE_ENABLE, 0);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_PROTECT_LITE_CTRL_CALIBRATION_ENABLE, 0);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_PROTECT_PILOT_TONE_PEART_CMPST_0_SINEGENERATORSENSE_0_THRESHOLD, orig_threshold);
    if (ret)
    {
        return ret;
    }
    // Apply most recent calibration
    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_RE_CALIB_SELECTOR_CMPST_0_RECALIBSELECTOR_0_SEL_RE_CAL, 0xFFFFFF);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_PROTECT_LITE_CTRL_PROTECT_LITE_ENABLE, 1);
    if (ret)
    {
        return ret;
    }

    // Read the Calibration Load Impedance "R"
    ret = regmap_read_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_VAR_ARRAY_MEASURED_IMPEDANCE_CALIBRATION, &temp_reg_val);
    if (ret)
    {
        return ret;
    }
    driver->config.cal_data.r = temp_reg_val;

    // Read the Calibration Checksum
    ret = regmap_read_fw_control(cp, driver->fw_info, CS35L42_SYM_PROTECT_LITE_VAR_ARRAY_CHECK_SUM_CALIBRATION, &temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Verify the Calibration Checksum
    if (temp_reg_val == (driver->config.cal_data.r + CS35L42_CAL_STATUS_CALIB_SUCCESS))
    {
        driver->config.cal_data.is_valid = true;
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

