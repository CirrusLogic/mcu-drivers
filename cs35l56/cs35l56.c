/**
 * @file cs35l56.c
 *
 * @brief The CS36L56 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2026 All Rights Reserved, http://www.cirrus.com/
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
#include "cs35l56.h"
#include "string.h"

#ifdef CIRRUS_SDK
#include "bsp_driver_if.h"
#endif

#ifdef CIRRUS_ZEPHYR_SAMPLE
#include "cs35l56_bsp.h"
#endif

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * Total INT and MASK registers to handle in IRQ1
 */
#define CS35L56_IRQ1_REG_TOTAL          (10)

#define CS35L56_EVENT_GLOBAL_ERROR_STATE_MASK (CS35L56_EVENT_FLAG_AMP_ERROR | \
                                               CS35L56_EVENT_FLAG_TEMP_ERROR | \
                                               CS35L56_EVENT_FLAG_BST_ERROR | \
                                               CS35L56_EVENT_FLAG_RUNTIME_SHORT_DETECTED | \
                                               CS35L56_EVENT_FLAG_PERMANENT_SHORT_DETECTED)
#define CS35L56_EVENT_INIT_WAKE (CS35L56_EVENT_FLAG_INIT_COMPLETE | CS35L56_EVENT_FLAG_AWAKE)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

const struct cs35l56_register_encoding cs35l56_pll_refclk[CS35L56_NUM_VALD_PLL_REFCLKS] = {
    {128000, 0x0C},   {256000, 0x0F},   {384000, 0x11},   {512000, 0x12},   {768000, 0x15},
    {1024000, 0x17},  {1411200, 0x19},  {1500000, 0x1A},  {1536000, 0x1B},  {2000000, 0x1C},
    {2048000, 0x1D},  {2400000, 0x1E},  {2822400, 0x1F},  {3000000, 0x20},  {3072000, 0x21},
    {4000000, 0x23},  {4096000, 0x24},  {4800000, 0x25},  {5644800, 0x26},  {6000000, 0x27},
    {6144000, 0x28},  {6250000, 0x29},  {6400000, 0x2A},  {7526400, 0x2D},  {8000000, 0x2E},
    {8192000, 0x2F},  {9600000, 0x30},  {11289600, 0x31}, {12000000, 0x32}, {12288000, 0x33},
    {13500000, 0x37}, {19200000, 0x38}, {22579200, 0x39}, {24576000, 0x3B}};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/**
 * Mapping of CS35L56 IRQ Flag to Event Flag
 *
 * List is in the form:
 * - word0 - IRQ Flag
 * - word1 - Event Flag
 * - ...
 *
 * @see cs35l56_irq_to_event_id
 *
 */
static const uint32_t cs35l56_irq_to_event_flag_map[] =
{
    CS35L56_IRQ1_INT_1, IRQ1_INT_1_AMP_SHORT_ERR_INT1_BITMASK,  CS35L56_EVENT_FLAG_AMP_ERROR,
    CS35L56_IRQ1_INT_4, IRQ1_INT_4_OTP_BOOT_DONE_INT1_BITMASK, CS35L56_EVENT_FLAG_OTP_BOOT_DONE,
    CS35L56_IRQ1_INT_8, IRQ1_INT_8_TEMP_ERR_INT1_BITMASK,  CS35L56_EVENT_FLAG_TEMP_ERROR,
    CS35L56_IRQ1_INT_9, IRQ1_INT_9_BST_ILIMIT_ERR_INT1_BITMASK, CS35L56_EVENT_FLAG_BST_ERROR,
    CS35L56_IRQ1_INT_9, IRQ1_INT_9_BST_SHORT_ERR_INT1_BITMASK, CS35L56_EVENT_FLAG_BST_ERROR,
    CS35L56_IRQ1_INT_9, IRQ1_INT_9_BST_UVP_ERR_INT1_BITMASK, CS35L56_EVENT_FLAG_BST_ERROR,
    CS35L56_IRQ1_INT_10, IRQ1_INT_10_UVLO_VDDBATT_ERR_INT1_BITMASK, CS35L56_EVENT_FLAG_BST_ERROR,
};

static uint32_t cs35l56_mbox_command_to_event_id_map[] =
{
    CS35L56_MBOX_COMMAND_INIT, CS35L56_EVENT_FLAG_INIT_COMPLETE,
    CS35L56_MBOX_COMMAND_AWAKE, CS35L56_EVENT_FLAG_AWAKE,
    CS35L56_MBOX_COMMAND_PERMANENT_SHORT_DETECTED, CS35L56_EVENT_FLAG_PERMANENT_SHORT_DETECTED,
    CS35L56_MBOX_COMMAND_RUNTIME_SHORT_DETECTED, CS35L56_EVENT_FLAG_RUNTIME_SHORT_DETECTED
};

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Notify the driver when the CS35L56 INTb GPIO drops low.
 *
 * This callback is registered with the BSP in the register_gpio_cb() API call.
 *
 * The primary task of this callback is to transition the driver mode from CS35L56_MODE_HANDLING_CONTROLS to
 * CS35L56_MODE_HANDLING_EVENTS, in order to signal to the main thread to process events.
 *
 * @param [in] status           BSP status for the INTb IRQ.
 * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
 *                              pointer to the driver state cs35l56_t.
 *
 * @return none
 *
 * @see bsp_driver_if_t member register_gpio_cb.
 * @see bsp_callback_t
 *
 */

static void cs35l56_irq_callback(uint32_t status, void *cb_arg)
{
    cs35l56_t *d;

    d = (cs35l56_t *) cb_arg;

    if (status)
    {
        // Switch driver mode to CS35L56_MODE_HANDLING_EVENTS
        d->mode = CS35L56_MODE_HANDLING_EVENTS;
    }
    return;
}

/**
 * Maps IRQ Flag to Event ID passed to BSP
 *
 * Allows for abstracting driver events relayed to BSP away from IRQ flags, to allow the possibility that multiple
 * IRQ flags correspond to a single event to relay.
 *
 * @param [in] irq_statuses     pointer to array of 32-bit words from IRQ1_IRQ1_EINT_*_REG registers
 *
 * @return                      32-bit word with CS35L56_EVENT_FLAG_* set for each event detected
 *
 * @see CS35L56_EVENT_FLAG_
 *
 */

static uint32_t cs35l56_irq_to_event_id(uint32_t irq_reg, uint32_t irq_statuses)
{
    uint32_t temp_event_flag = 0;

    for (uint8_t i = 0; i < (sizeof(cs35l56_irq_to_event_flag_map)/sizeof(uint32_t)); i += 3)
    {
        if ((cs35l56_irq_to_event_flag_map[i] == irq_reg) &&
            (cs35l56_irq_to_event_flag_map[(i) + 1] & irq_statuses))
        {
            temp_event_flag |= cs35l56_irq_to_event_flag_map[(i) + 2];
        }
    }

    return temp_event_flag;
}

static uint32_t cs35l56_mbox_read_next_command(regmap_cp_config_t *cp, uint32_t *command)
{
    uint32_t ret = CS35L56_STATUS_OK;
    uint32_t q_base, q_rd_ptr, q_wr_ptr, q_length;

    // Read MBOX queue parameters
    ret = regmap_read(cp, CS35L56_MAILBOX_QUEUE_BASE, &q_base);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, (CS35L56_MAILBOX_QUEUE_BASE + CS35L56_MAILBOX_QUEUE_LEN_OFFSET), &q_length);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, (CS35L56_MAILBOX_QUEUE_BASE + CS35L56_MAILBOX_QUEUE_WT_OFFSET), &q_wr_ptr);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, (CS35L56_MAILBOX_QUEUE_BASE + CS35L56_MAILBOX_QUEUE_RD_OFFSET), &q_rd_ptr);
    if (ret)
    {
        return ret;
    }

    // If MBOX queue empty, exit
    if (q_wr_ptr == q_rd_ptr)
    {
        return CS35L56_STATUS_FAIL;
    }

    // Read next command
    ret = regmap_read(cp, q_rd_ptr, command);
    if (ret)
    {
        return ret;
    }

    // Calculate next q_rd_ptr, wrap to q_base if past last queue element
    q_rd_ptr += 4;
    if (q_rd_ptr > (q_base + ((q_length - 1) * 4)))
    {
        q_rd_ptr = q_base;
    }

    // Update new RD address
    ret = regmap_write(cp, (CS35L56_MAILBOX_QUEUE_BASE + CS35L56_MAILBOX_QUEUE_RD_OFFSET), q_rd_ptr);
    if (ret)
    {
        return ret;
    }

    return CS35L56_STATUS_OK;
}

static uint32_t cs35l56_mbox_command_to_event_id(uint32_t command)
{
    for (uint8_t i = 0; i < (sizeof(cs35l56_mbox_command_to_event_id_map)/sizeof(uint32_t)); i += 2)
    {
        if (cs35l56_mbox_command_to_event_id_map[i] == command)
        {
            return cs35l56_mbox_command_to_event_id_map[i + 1];
        }
    }

    return 0;
}

static uint32_t cs35l56_process_mbox_queue(regmap_cp_config_t *cp)
{
    uint32_t event_flags = 0;
    uint32_t command = 0;
    uint32_t ret = CS35L56_STATUS_FAIL;

    do
    {
        ret = cs35l56_mbox_read_next_command(cp, &command);
        if (ret == CS35L56_STATUS_OK)
        {
            event_flags |= cs35l56_mbox_command_to_event_id(command);
        }
    } while (ret == CS35L56_STATUS_OK);

    return event_flags;
}

/**
 * Handle events indicated by the IRQ pin ALERTb
 *
 * This function performs all steps to handle IRQ and other asynchronous events the driver is aware of,
 * resulting in calling of the notification callback (cs35l56_notification_callback_t).
 *
 * Can assume event_flags is 0 before entering.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L56_STATUS_FAIL        Control port activity fails
 * - CS35L56_STATUS_OK          otherwise
 *
 * @see CS35L56_EVENT_FLAG_
 * @see cs35l56_notification_callback_t
 *
 */
static uint32_t cs35l56_event_handler(cs35l56_t *driver)
{
    uint32_t ret = CS35L56_STATUS_OK;
    uint32_t irq_statuses[CS35L56_IRQ1_REG_TOTAL];
    uint32_t irq_masks[CS35L56_IRQ1_REG_TOTAL];
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);
    bool process_mbox_queue = false;
    // Read IRQ1_STATUS
    ret = regmap_read(cp, CS35L56_IRQ1_IRQ1_STATUS, &(irq_statuses[0]));
    if (ret)
    {
        return ret;
    }

    // If event handler was called without any IRQ set, then return
    else if (irq_statuses[0] == 0)
    {
        return CS35L56_STATUS_OK;
    }

    for (uint8_t i = 0; i < CS35L56_IRQ1_REG_TOTAL; i++)
    {
        uint32_t irq_flag_reg = CS35L56_IRQ1_INT_1 + (i * 4);

        // Read IRQ1_INT_1_*
        ret = regmap_read(cp, irq_flag_reg, &(irq_statuses[i]));
        if (ret)
        {
            return ret;
        }

        // Read IRQ1_MASK_1_*
        ret = regmap_read(cp, (CS35L56_IRQ1_IRQ1_MASK_1 + (i * 4)), &(irq_masks[i]));
        if (ret)
        {
            return ret;
        }

        irq_statuses[i] &= ~(irq_masks[i]);

        // If there are unmasked IRQs, then process
        if (irq_statuses[i])
        {
            // Clear any IRQ1 flags from first register
            ret = regmap_write(cp, irq_flag_reg, irq_statuses[i]);
            if (ret)
            {
                return ret;
            }

            driver->event_flags |= cs35l56_irq_to_event_id(irq_flag_reg, irq_statuses[i]);

            // If MBOX IRQ, then set flag to process MBOX queue
            if ((irq_flag_reg == CS35L56_IRQ1_INT_2) &&
                (irq_statuses[i] & CS35L56_IRQ1_MASK_2_DSP_VIRTUAL2_MBOX_WR_MASK1))
            {
                process_mbox_queue = true;
            }
        }
    }

    // Process MBOX Queue if source of IRQ
    if (process_mbox_queue)
    {
        driver->event_flags |= cs35l56_process_mbox_queue(cp);
    }

    // Handle any events that result in Global Error State OR FW Runtime/Permanent Short Detection
    if (driver->event_flags & CS35L56_EVENT_GLOBAL_ERROR_STATE_MASK)
    {
        ret = regmap_write(cp, CS35L56_MSM_ERROR_RELEASE, CS35L56_MSM_ERROR_RELEASE_GLOBAL_ERR_RELEASE_BITMASK);
        if (ret)
        {
            return ret;
        }

        ret = regmap_write(cp, CS35L56_MSM_ERROR_RELEASE, 0);
        if (ret)
        {
            return ret;
        }
        cs35l56_reset(driver);
    }

    if (driver->event_flags & CS35L56_EVENT_FLAG_OTP_BOOT_DONE)
    {
        // Wait 10ms and check mailbox again for init/wake
        bsp_driver_if_g->set_timer(10, NULL, NULL);
        driver->event_flags |= cs35l56_process_mbox_queue(cp);
        if (driver->event_flags & CS35L56_EVENT_INIT_WAKE)
        {
            ret = regmap_write(cp, CS35L56_IRQ1_INT_4, IRQ1_INT_4_OTP_BOOT_DONE_INT1_BITMASK);
            if (ret)
            {
                return ret;
            }
        } else {
            //timer expired without init/wake
            cs35l56_reset(driver);
        }
    }

    return CS35L56_STATUS_OK;
}


/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/*
 * Reads the contents of a single register/memory address
 *
 */
uint32_t cs35l56_read_reg(cs35l56_t *driver, uint32_t addr, uint32_t *val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_read(cp, addr, val);
    if (ret) {
        return CS35L56_STATUS_FAIL;
    }

    return CS35L56_STATUS_OK;
}

/*
 * Writes the contents of a single register/memory address
 *
 */
uint32_t cs35l56_write_reg(cs35l56_t *driver, uint32_t addr, uint32_t val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, addr, val);
    if (ret) {
        return CS35L56_STATUS_FAIL;
    }

    return CS35L56_STATUS_OK;
}

/**
 * Initialize driver state/handle
 *
 */
uint32_t cs35l56_initialize(cs35l56_t *driver)
{
    uint32_t ret = CS35L56_STATUS_FAIL;

    if (NULL != driver) {
        /*
         * The memset() call sets all members to 0, including the following semantics:
         * - 'state' is set to UNCONFIGURED
         */
        memset(driver, 0, sizeof(cs35l56_t));

        ret = CS35L56_STATUS_OK;
    }

    return ret;
}

/**
 * Configures driver state/handle
 *
 */
uint32_t cs35l56_configure(cs35l56_t *driver, cs35l56_config_t *config)
{
    uint32_t ret = CS35L56_STATUS_FAIL;

    if ((NULL != driver) && \
        (NULL != config))
    {
        driver->config = *config;
        ret = bsp_driver_if_g->register_gpio_cb((uint32_t)driver->config.bsp_config.int_gpio_id,
                                                &cs35l56_irq_callback,
                                                driver);
        if (ret)
        {
            ret = CS35L56_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Processes driver events and notifications
 *
 */
uint32_t cs35l56_process(cs35l56_t *driver)
{
    // check for driver mode
    if (driver->mode == CS35L56_MODE_HANDLING_EVENTS)
    {
        driver->mode = CS35L56_MODE_HANDLING_CONTROLS;
        // run through event handler
        if (CS35L56_STATUS_OK != cs35l56_event_handler(driver))
        {
            driver->event_flags |= CS35L56_EVENT_FLAG_STATE_ERROR;
        }
    }
    if (driver->event_flags)
    {

#ifndef CS35L56_BAREMETAL
        if (driver->config.bsp_config.notification_cb != NULL)
        {
            driver->config.bsp_config.notification_cb(driver->event_flags,
                                                      driver->config.bsp_config.notification_cb_arg);
        }

#endif //CS35L56_BAREMETAL
        driver->event_flags = 0;
    }
    return CS35L56_STATUS_OK;
}

/**
 * Reset the CS35L56
 *
 */
uint32_t cs35l56_reset(cs35l56_t *driver)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Drive RESET low for at least T_RLPW (1ms)
    bsp_driver_if_g->set_gpio((uint32_t)driver->config.bsp_config.reset_gpio_id, 0);
    bsp_driver_if_g->set_timer(2, NULL, NULL);
    // Drive RESET high and wait for at least T_IRS (2.2ms)
    bsp_driver_if_g->set_gpio((uint32_t)driver->config.bsp_config.reset_gpio_id, 1);
    bsp_driver_if_g->set_timer(5, NULL, NULL);

    // Read DEVID
    ret = regmap_read(cp, CS35L56_SW_RESET_DEVID_REG, &(driver->devid));
    if (ret) {
        return ret;
    }

    // Read REVID
    ret = regmap_read(cp, CS35L56_SW_RESET_REVID_REG, &(driver->revid));
    if (ret) {
        return ret;
    }

    // Only allow driver to handle REVID B2
    if (driver->revid != CS35L56_REVID_B2) {
        return CS35L56_STATUS_FAIL;
    }

    // Wait for (OTP + ROM) boot complete
    ret = regmap_poll_reg(cp, CS35L56_HALO_STATE, CS35L56_DSP_STATE_RUNNING, 10,
                  CS35L56_BOOT_TIMEMOUT_MS);
    if (ret) {
        return ret;
    }

    return ret;
}

/**
 * Finish booting the CS35L56
 *
 */
uint32_t cs35l56_boot(cs35l56_t *driver, fw_img_info_t *fw_info)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);
    driver->fw_info = fw_info;

    ret = regmap_write(cp, CS35L56_DSP_VIRTUAL1_MBOX_1, CS35L56_DSP_MBOX_CMD_SHUTDOWN);
    if (ret) {
        return ret;
    }

    bsp_driver_if_g->set_timer(10, NULL, NULL);

    ret = regmap_poll_reg(cp, CS35L56_PM_STATE_SHUTDOWN, CS35L56_PM_STATE, 10,
                  CS35L56_BOOT_TIMEMOUT_MS);
    if (ret) {
        return ret;
    }

    bsp_driver_if_g->set_timer(10, NULL, NULL);

    return CS35L56_STATUS_OK;
}

/**
 * Sets the hibernation timeout
 *
 */
uint32_t cs35l56_timeout_ticks_set(cs35l56_t *driver, uint32_t ms)
{
    uint32_t ret = CS35L56_STATUS_FAIL;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);
    uint32_t lower_val, ticks;


    if (ms > CS35L56_PM_TIMEOUT_MS_MAX)
    {
      ticks = CS35L56_PM_TIMEOUT_MS_MAX * CS35L56_PM_TICKS_MS_DIV;
    }
    else
    {
      ticks = ms * CS35L56_PM_TICKS_MS_DIV;
    }

    lower_val = ticks & CS35L56_PM_TIMEOUT_TICKS_LOWER_MASK;

    ret = regmap_write(cp, CS35L56_PM_TIMER_TIMEOUT_TICKS_3_L, lower_val);
    return ret;
}

/**
 * Enable/Disable Audio Streaming Port
 *
 */
uint32_t cs35l56_set_asp_enable(cs35l56_t *driver, bool enable, uint32_t freq)
{
    uint32_t ret;
    uint8_t pll_refclk_val;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (!enable) {
        // Disable I2C Config
        ret = regmap_update_reg(cp, CS35L56_BLOCK_ENABLES2,
                    CS35L56_BLOCK_ENABLES2_ASP_EN_MASK, 0);
        if (ret) {
            return ret;
        }
        ret = regmap_write(cp, CS35L56_ASP1_ENABLES, 0);
        if (ret) {
            return ret;
        }
    } else {
        // Check for valid freq
        int i;
        for (i = 0; i < CS35L56_NUM_VALD_PLL_REFCLKS; i++) {
            if (freq == cs35l56_pll_refclk[i].value) {
                pll_refclk_val = cs35l56_pll_refclk[i].code;
                break;
            }
        }
        if (i == CS35L56_NUM_VALD_PLL_REFCLKS) {
            return CS35L56_STATUS_FAIL;
        }
        // Configure for I2S at given frequency
        ret = regmap_write(cp, CS35L56_ASP1_CTRL_1, pll_refclk_val);
        if (ret) {
            return ret;
        }
        ret = regmap_write(cp, CS35L56_ASP1_CTRL_2,
                   (0x40 << CS35L56_ASP1_CTRL_RX_WIDTH_OFFSET) |
                       CS35L56_ASP1_CTRL_2_ASP1_FMT_BCLK_MASK);
        if (ret) {
            return ret;
        }
        ret = regmap_write(cp, CS35L56_BLOCK_ENABLES2, CS35L56_BLOCK_ENABLES2_ASP_EN_MASK);
        if (ret) {
            return ret;
        }
        ret = regmap_write(cp, CS35L56_ASP1_ENABLES,
                   (0x3 << CS35L56_ASP1_ENABLES_RX_SHIFT));
        if (ret) {
            return ret;
        }

        // PLL_OPEN_LOOP must clear before changing REFCLK FREQ
        ret = regmap_update_reg(cp, CS35L56_FIRMWARE_PLL_REFCLK_FREQ,
                    CS35L56_FIRMWARE_PLL_OPEN_LOOP_MASK, 0);
        if (ret) {
            return ret;
        }
        ret = regmap_update_reg(
            cp, CS35L56_FIRMWARE_PLL_REFCLK_FREQ, 0,
            (pll_refclk_val << CS35L56_FIRMWARE_PLL_REFCLK_FREQ_OFFSET) |
                CS35L56_FIRMWARE_PLL_REFCLK_EN_MASK);
        if (ret) {
            return ret;
        }
        ret = regmap_update_reg(cp, CS35L56_FIRMWARE_PLL_REFCLK_FREQ,
                    CS35L56_FIRMWARE_PLL_OPEN_LOOP_MASK, 0);
        if (ret) {
            return ret;
        }

        // Handle Mixer + Mailbox Command for DSP
        ret = regmap_write(cp, CS35L56_DACPCM1_INPUT, CS35L56_DACPCM1_INPUT_DSP1_CH5);
        if (ret) {
            return ret;
        }
    }
    return CS35L56_STATUS_OK;
}
