/**
 * @file cs40l50.c
 *
 * @brief The CS40L50 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022-2025 All Rights Reserved, http://www.cirrus.com/
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
#include "string.h"

#ifdef CIRRUS_SDK
#include "bsp_driver_if.h"
#endif

#ifdef CIRRUS_ZEPHYR_SAMPLE
#include "cs40l50_bsp.h"
#endif

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

//#define UNUSED(x) (void)(x)

/**
 * Total attemps to wake part from hibernate
 */
#define CS40L50_WAKE_ATTEMPTS           (10)

#define CS40L50_COMPENSATION_ENABLE_F0_MASK     (1 << 0)
#define CS40L50_COMPENSATION_ENABLE_REDC_MASK   (1 << 1)

/**
 * Total INT and MASK registers to handle in IRQ1
 */
#define CS40L50_IRQ1_REG_TOTAL          (10)

/**
 * This ID is unique to the Blackstar BSP and maps to
 * the I2C address CS40L50_I2C_BROADCAST_ADDR_DEFAULT
 */
#define CS40L50_BROADCAST_DEVID          (7)

#define CS40L50_EVENT_GLOBAL_ERROR_STATE_MASK (CS40L50_EVENT_FLAG_AMP_ERROR | \
                                               CS40L50_EVENT_FLAG_TEMP_ERROR | \
                                               CS40L50_EVENT_FLAG_BST_ERROR | \
                                               CS40L50_EVENT_FLAG_RUNTIME_SHORT_DETECTED | \
                                               CS40L50_EVENT_FLAG_PERMANENT_SHORT_DETECTED)

#ifndef CS40L50_BAREMETAL

#define CS40L50_MBOX_COMMAND_HAPTIC_COMPLETE_MBOX           (0x01000000)
#define CS40L50_MBOX_COMMAND_HAPTIC_COMPLETE_GPIO           (0x01000001)
#define CS40L50_MBOX_COMMAND_HAPTIC_COMPLETE_I2S            (0x01000002)
#define CS40L50_MBOX_COMMAND_HAPTIC_TRIGGER_MBOX            (0x01000010)
#define CS40L50_MBOX_COMMAND_HAPTIC_TRIGGER_GPIO            (0x01000011)
#define CS40L50_MBOX_COMMAND_HAPTIC_TRIGGER_I2S             (0x01000012)
#define CS40L50_MBOX_COMMAND_INIT                           (0x02000000)
#define CS40L50_MBOX_COMMAND_AWAKE                          (0x02000002)
#define CS40L50_MBOX_COMMAND_PERMANENT_SHORT_DETECTED       (0x0C000C1C)
#define CS40L50_MBOX_COMMAND_RUNTIME_SHORT_DETECTED         (0x0C000C1D)

#endif //CS40L50_BAREMETAL

/**
 * Total entries in Dynamic F0 table
 */
#define CS40L50_DYNAMIC_F0_TABLE_SIZE               (20)
#define CS40L50_DYNAMIC_F0_TABLE_ENTRY_SIZE_BYTES   (12)

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
// See CS40L50_HAP2_Init_ExtVDDAmp.txt
static const uint32_t cs40l50_external_bst_cfg[] =
{
    0x00002018, 0x00003201,
    0x00004404, 0x01000000
};

// See CS40L50_HAP2_Errata_ExtVDDAmp.txt
static const uint32_t cs40l50_b0_errata_external[] =
{
    0x00000040, 0x00000055,
    0x00000040, 0x000000AA,
    0x00005C00, 0x00000400,
    0x00004220, 0x8000007D,
    0x00004200, 0x00000008,
    0x00004240, 0x510002B5,
    0x00006024, 0x00522303,
    0x02804348, 0x00040020,
    0x0280434C, 0x00183201,
    0x02804350, 0x00050044,
    0x02804354, 0x00040100,
    0x02804358, 0x00FD0001,
    0x0280435C, 0x0004005C,
    0x02804360, 0x00000400,
    0x02804364, 0x00000000,
    0x02804368, 0x00422080,
    0x0280436C, 0x0000007D,
    0x02804370, 0x00040042,
    0x02804374, 0x00000008,
    0x02804378, 0x00050042,
    0x0280437C, 0x00405100,
    0x02804380, 0x00040060,
    0x02804384, 0x00242303,
    0x02804388, 0x00FFFFFF,
};

#ifndef CS40L50_BAREMETAL
cs40l50_pwle_t pwle_default =
{
    .word1.wf_length = WF_LENGTH_DEFAULT,
    .word2.pwls_ms4  = PWLS_MS4,
    .word2.wait_time = WAIT_TIME_DEFAULT,
    .word2.repeat    = REPEAT_DEFAULT,
    .word3.level_ms4 = LEVEL_MS4,
    .word3.time      = TIME_DEFAULT,
    .word3.pwls_ls4  = PWLS_LS4,
    .word4.ext_freq  = EXT_FREQ_DEFAULT,
    .word4.amp_reg   = AMP_REG_DEFAULT,
    .word4.braking   = BRAKING_DEFAULT,
    .word4.chirp     = CHIRP_DEFAULT,
    .word4.freq      = FREQ_DEFAULT,
    .word4.level_ls8 = LEVEL_LS8,
    .word5.level_ms4 = 0,
    .word5.time      = TIME_DEFAULT,
    .word6.level_ls8 = 0,
    .word6.freq      = FREQ_DEFAULT,
    .word6.ext_freq  = EXT_FREQ_DEFAULT,
    .word6.amp_reg   = AMP_REG_DEFAULT,
    .word6.braking   = BRAKING_DEFAULT,
    .word6.chirp     = CHIRP_DEFAULT
};

cs40l50_pwle_short_section_t pwle_short_default =
{
    .word1.time      = TIME_DEFAULT,
    .word1.level_ms8 = LEVEL_MS8_DEFAULT,
    .word2.level_ls4 = LEVEL_LS4_DEFAULT,
    .word2.freq      = FREQ_DEFAULT,
    .word2.chirp     = CHIRP_DEFAULT,
    .word2.braking   = BRAKING_DEFAULT,
    .word2.amp_reg   = AMP_REG_DEFAULT,
    .word2.ext_freq  = EXT_FREQ_DEFAULT
};

/**
 * Mapping of CS40L50 IRQ Flag to Event Flag
 *
 * List is in the form:
 * - word0 - IRQ Flag
 * - word1 - Event Flag
 * - ...
 *
 * @see cs40l50_irq_to_event_id
 *
 */
static const uint32_t cs40l50_irq_to_event_flag_map[] =
{
    CS40L50_IRQ1_INT_1, IRQ1_INT_1_AMP_SHORT_ERR_INT1_BITMASK,  CS40L50_EVENT_FLAG_AMP_ERROR,
    CS40L50_IRQ1_INT_8, IRQ1_INT_8_TEMP_ERR_INT1_BITMASK,  CS40L50_EVENT_FLAG_TEMP_ERROR,
    CS40L50_IRQ1_INT_9, IRQ1_INT_9_BST_ILIMIT_ERR_INT1_BITMASK, CS40L50_EVENT_FLAG_BST_ERROR,
    CS40L50_IRQ1_INT_9, IRQ1_INT_9_BST_SHORT_ERR_INT1_BITMASK, CS40L50_EVENT_FLAG_BST_ERROR,
    CS40L50_IRQ1_INT_9, IRQ1_INT_9_BST_UVP_ERR_INT1_BITMASK, CS40L50_EVENT_FLAG_BST_ERROR,
    CS40L50_IRQ1_INT_10, IRQ1_INT_10_UVLO_VDDBATT_ERR_INT1_BITMASK, CS40L50_EVENT_FLAG_BST_ERROR,
};

static uint32_t cs40l50_mbox_command_to_event_id_map[] =
{
    CS40L50_MBOX_COMMAND_HAPTIC_COMPLETE_MBOX, CS40L50_EVENT_FLAG_HAPTIC_COMPLETE_MBOX,
    CS40L50_MBOX_COMMAND_HAPTIC_COMPLETE_GPIO, CS40L50_EVENT_FLAG_HAPTIC_COMPLETE_GPIO,
    CS40L50_MBOX_COMMAND_HAPTIC_COMPLETE_I2S, CS40L50_EVENT_FLAG_HAPTIC_COMPLETE_I2S,
    CS40L50_MBOX_COMMAND_HAPTIC_TRIGGER_MBOX, CS40L50_EVENT_FLAG_HAPTIC_TRIGGER_MBOX,
    CS40L50_MBOX_COMMAND_HAPTIC_TRIGGER_GPIO, CS40L50_EVENT_FLAG_HAPTIC_TRIGGER_GPIO,
    CS40L50_MBOX_COMMAND_HAPTIC_TRIGGER_I2S, CS40L50_EVENT_FLAG_HAPTIC_TRIGGER_I2S,
    CS40L50_MBOX_COMMAND_INIT, CS40L50_EVENT_FLAG_INIT_COMPLETE,
    CS40L50_MBOX_COMMAND_AWAKE, CS40L50_EVENT_FLAG_AWAKE,
    CS40L50_MBOX_COMMAND_PERMANENT_SHORT_DETECTED, CS40L50_EVENT_FLAG_PERMANENT_SHORT_DETECTED,
    CS40L50_MBOX_COMMAND_RUNTIME_SHORT_DETECTED, CS40L50_EVENT_FLAG_RUNTIME_SHORT_DETECTED
};

#endif //CS40L50_BAREMETAL


#ifdef CIRRUS_SDK
static regmap_cp_config_t broadcast_cp =
{
    .dev_id = CS40L50_BROADCAST_DEVID,
    .bus_type = REGMAP_BUS_TYPE_I2C,
    .receive_max = 0,
};
#endif


#ifdef CIRRUS_ZEPHYR_SAMPLE
static void *broadcast_cp;
#endif

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
        ret = regmap_read(cp, 0x28021E0, &dsp_state);
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
#ifndef CS40L50_BAREMETAL
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
#endif //CS40L50_BAREMETAL

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

    cs40l50_set_broadcast_enable(driver, driver->config.broadcast);

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
 * @return                      32-bit word with CS40L50_EVENT_FLAG_* set for each event detected
 *
 * @see CS40L50_EVENT_FLAG_
 *
 */
#ifndef CS40L50_BAREMETAL
static uint32_t cs40l50_irq_to_event_id(uint32_t irq_reg, uint32_t irq_statuses)
{
    uint32_t temp_event_flag = 0;

    for (uint8_t i = 0; i < (sizeof(cs40l50_irq_to_event_flag_map)/sizeof(uint32_t)); i += 3)
    {
        if ((cs40l50_irq_to_event_flag_map[i % 3] == irq_reg) &&
            (cs40l50_irq_to_event_flag_map[(i % 3) + 1] & irq_statuses))
        {
            temp_event_flag |= cs40l50_irq_to_event_flag_map[(i % 3) + 2];
        }
    }

    return temp_event_flag;
}

static uint32_t cs40l50_mbox_read_next_command(regmap_cp_config_t *cp, uint32_t *command)
{
    uint32_t ret = CS40L50_STATUS_OK;
    uint32_t q_base, q_rd_ptr, q_wr_ptr, q_length;

    // Read MBOX queue parameters
    ret = regmap_read(cp, CS40L50_MAILBOX_QUEUE_BASE, &q_base);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, (CS40L50_MAILBOX_QUEUE_BASE + CS40L50_MAILBOX_QUEUE_LEN_OFFSET), &q_length);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, (CS40L50_MAILBOX_QUEUE_BASE + CS40L50_MAILBOX_QUEUE_WT_OFFSET), &q_wr_ptr);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, (CS40L50_MAILBOX_QUEUE_BASE + CS40L50_MAILBOX_QUEUE_RD_OFFSET), &q_rd_ptr);
    if (ret)
    {
        return ret;
    }

    // If MBOX queue empty, exit
    if (q_wr_ptr == q_rd_ptr)
    {
        return CS40L50_STATUS_FAIL;
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
    ret = regmap_write(cp, (CS40L50_MAILBOX_QUEUE_BASE + CS40L50_MAILBOX_QUEUE_RD_OFFSET), q_rd_ptr);
    if (ret)
    {
        return ret;
    }

    return CS40L50_STATUS_OK;
}

static uint32_t cs40l50_mbox_command_to_event_id(uint32_t command)
{
    for (uint8_t i = 0; i < (sizeof(cs40l50_mbox_command_to_event_id_map)/sizeof(uint32_t)); i += 2)
    {
        if (cs40l50_mbox_command_to_event_id_map[i] == command)
        {
            return cs40l50_mbox_command_to_event_id_map[i + 1];
        }
    }

    return 0;
}

static uint32_t cs40l50_process_mbox_queue(regmap_cp_config_t *cp)
{
    uint32_t event_flags = 0;
    uint32_t command = 0;
    uint32_t ret = CS40L50_STATUS_FAIL;

    do
    {
        ret = cs40l50_mbox_read_next_command(cp, &command);
        if (ret == CS40L50_STATUS_OK)
        {
            event_flags |= cs40l50_mbox_command_to_event_id(command);
        }
    } while (ret == CS40L50_STATUS_OK);

    return event_flags;
}
#endif //CS40L50_BAREMETAL


/**
 * Handle events indicated by the IRQ pin ALERTb
 *
 * This function performs all steps to handle IRQ and other asynchronous events the driver is aware of,
 * resulting in calling of the notification callback (cs40l50_notification_callback_t).
 *
 * Can assume event_flags is 0 before entering.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS40L50_STATUS_FAIL        Control port activity fails
 * - CS40L50_STATUS_OK          otherwise
 *
 * @see CS40L50_EVENT_FLAG_
 * @see cs40l50_notification_callback_t
 *
 */
#ifndef CS40L50_BAREMETAL
static uint32_t cs40l50_event_handler(cs40l50_t *driver)
{
    uint32_t ret = CS40L50_STATUS_OK;
    return ret;
    uint32_t irq_statuses[CS40L50_IRQ1_REG_TOTAL];
    uint32_t irq_masks[CS40L50_IRQ1_REG_TOTAL];
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);
    bool process_mbox_queue = false;

    // Read IRQ1_STATUS
    ret = regmap_read(cp, CS40L50_IRQ1_IRQ1_STATUS, &(irq_statuses[0]));
    if (ret)
    {
        return ret;
    }

    // If event handler was called without any IRQ set, then return
    else if (irq_statuses[0] == 0)
    {
        return CS40L50_STATUS_OK;
    }

    for (uint8_t i = 0; i < CS40L50_IRQ1_REG_TOTAL; i++)
    {
        uint32_t irq_flag_reg = CS40L50_IRQ1_INT_1 + (i * 4);

        // Read IRQ1_INT_1_*
        ret = regmap_read(cp, irq_flag_reg, &(irq_statuses[i]));
        if (ret)
        {
            return ret;
        }

        // Read IRQ1_MASK_1_*
        ret = regmap_read(cp, (CS40L50_IRQ1_IRQ1_MASK_1 + (i * 4)), &(irq_masks[i]));
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

            driver->event_flags |= cs40l50_irq_to_event_id(irq_flag_reg, irq_statuses[i]);

            // If MBOX IRQ, then set flag to process MBOX queue
            if ((irq_flag_reg == CS40L50_IRQ1_INT_2) &&
                (irq_statuses[i] & CS40L50_IRQ1_MASK_2_DSP_VIRTUAL2_MBOX_WR_MASK1))
            {
                process_mbox_queue = true;
            }
        }
    }

    // Process MBOX Queue if source of IRQ
    if (process_mbox_queue)
    {
        driver->event_flags |= cs40l50_process_mbox_queue(cp);
    }

    // Handle any events that result in Global Error State OR FW Runtime/Permanent Short Detection
    if (driver->event_flags & CS40L50_EVENT_GLOBAL_ERROR_STATE_MASK)
    {
        ret = regmap_write(cp, CS40L50_MSM_ERROR_RELEASE, CS40L50_MSM_ERROR_RELEASE_GLOBAL_ERR_RELEASE_BITMASK);
        if (ret)
        {
            return ret;
        }

        ret = regmap_write(cp, CS40L50_MSM_ERROR_RELEASE, 0);
        if (ret)
        {
            return ret;
        }
    }

    return CS40L50_STATUS_OK;
}

#endif //CS40L50_BAREMETAL
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
#ifndef CS40L50_BAREMETAL
        ret = bsp_driver_if_g->register_gpio_cb(driver->config.bsp_config.int_gpio_id,
                                                &cs40l50_irq_callback,
                                                driver);
#endif //CS40L50_BAREMETAL
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
    // check for driver mode
    if (driver->mode == CS40L50_MODE_HANDLING_EVENTS)
    {
        // run through event handler
#ifndef CS40L50_BAREMETAL
        if (CS40L50_STATUS_OK != cs40l50_event_handler(driver))
        {
            driver->event_flags |= CS40L50_EVENT_FLAG_STATE_ERROR;
        }
#endif //CS40L50_BAREMETAL
        driver->mode = CS40L50_MODE_HANDLING_CONTROLS;
    }

    if (driver->event_flags)
    {

#ifndef CS40L50_BAREMETAL
        if (driver->config.bsp_config.notification_cb != NULL)
        {
            driver->config.bsp_config.notification_cb(driver->event_flags,
                                                      driver->config.bsp_config.notification_cb_arg);
        }

#endif //CS40L50_BAREMETAL
        driver->event_flags = 0;
    }

    return CS40L50_STATUS_OK;
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

    // Only allow driver to handle REVID B0
    if (driver->revid != CS40L50_REVID_B0)
    {
        return CS40L50_STATUS_FAIL;
    }

    // Wait for (OTP + ROM) boot complete
    ret = regmap_poll_reg(cp, 0x28021E0, 2, 10, 10);
    if (ret)
    {
        return ret;
    }

    // Write system errata
    if (driver->config.is_ext_bst)
    {
        ret = regmap_write_array(cp, (uint32_t *) cs40l50_external_bst_cfg,
                                 sizeof(cs40l50_external_bst_cfg)/sizeof(uint32_t));
        if (ret)
        {
          return ret;
        }
        ret = regmap_write_array(cp, (uint32_t *) cs40l50_b0_errata_external,
                                 sizeof(cs40l50_b0_errata_external)/sizeof(uint32_t));
        if (ret)
        {
            return ret;
        }
    }

    /**
     * Enable/Disable MBOX IRQs if specified.
     * For Rev B0, enabled by default.
     */
    if (driver->config.enable_mbox_irq)
    {
        ret = regmap_update_reg(cp,
                                CS40L50_IRQ1_MASK_2,
                                CS40L50_IRQ1_MASK_2_DSP_VIRTUAL2_MBOX_WR_MASK1,
                                0);
    }
    else
    {
        ret = regmap_update_reg(cp,
                                CS40L50_IRQ1_MASK_2,
                                CS40L50_IRQ1_MASK_2_DSP_VIRTUAL2_MBOX_WR_MASK1,
                                CS40L50_IRQ1_MASK_2_DSP_VIRTUAL2_MBOX_WR_MASK1);
    }

    return ret;
}

/**
 * Finish booting the CS40L50
 *
 */
uint32_t cs40l50_boot(cs40l50_t *driver, fw_img_info_t *fw_info)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);
    driver->fw_info = fw_info;

    // If fw_info is NULL, prepare part to exit ROM FW mode, otherwise RAM FW is downloaded
    if (driver->fw_info == NULL)
    {
        // Wake up part via mailbox interaction and prevent hibernate
        cs40l50_pm_state_transition(driver, CS40L50_PM_STATE_PREVENT_HIBERNATE);

        // Turn off DSP clock
        ret = regmap_write(cp, CS40L50_DSP1_CCM_CORE_CONTROL, 0x00000080);
        if (ret)
        {
            return ret;
        }

        // Set the RAM init flag in XRAM FW register FIRMWARE_CS40L50_CALL_RAM_INIT
        ret = regmap_write(cp, CS40L50_FIRMWARE_CALL_RAM_INIT, 0x00000001);
        if (ret)
        {
            return ret;
        }

        // Set the MEM_RDY HW flag (probably already set if part has ever hibernated but this is to be safe)
        ret = regmap_write(cp, CS40L50_PWRMGT_CTL, 0x00000002);
        if (ret)
        {
            return ret;
        }
    }
    else
    {
        // Start DSP
        ret = regmap_write(cp, CS40L50_DSP1_CCM_CORE_CONTROL, 0x00000281);
        if (ret)
        {
            return ret;
        }

        bsp_driver_if_g->set_timer(10, NULL, NULL);
    }

    driver->power_state = CS40L50_POWER_STATE_WAKE;

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
    uint32_t redc, f0, mbox_rd_ptr_value, data;
    uint32_t mbox_rd_ptr_addr;
    uint32_t ret = CS40L50_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    mbox_rd_ptr_addr = CS40L50_MAILBOX_QUEUE_BASE + CS40L50_MAILBOX_QUEUE_RD_OFFSET;

    ret = regmap_read(cp, (CS40L50_MAILBOX_QUEUE_BASE + CS40L50_MAILBOX_QUEUE_WT_OFFSET), &data);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, mbox_rd_ptr_addr, data);
    if (ret)
    {
        return ret;
    }

    mbox_rd_ptr_value = data;

    ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, CS40L50_DSP_MBOX_REDC_EST);
    if (ret)
    {
        return ret;
    }

    ret = regmap_poll_reg(cp, mbox_rd_ptr_value, CS40L50_DSP_MBOX_REDC_EST_START, 10, 1);
    if (ret)
    {
        return ret;
    }

    mbox_rd_ptr_value += 4;

    ret = regmap_write(cp, mbox_rd_ptr_addr, mbox_rd_ptr_value);
    if (ret)
    {
        return ret;
    }

    ret = regmap_poll_reg(cp, mbox_rd_ptr_value, CS40L50_DSP_MBOX_REDC_EST_DONE, 30, 1);
    if (ret)
    {
        return ret;
    }

    mbox_rd_ptr_value += 4;

    ret = regmap_write(cp, mbox_rd_ptr_addr, mbox_rd_ptr_value);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, CS40L50_SVC_RE_EST_STATUS, &redc);
    if (ret)
    {
        return ret;
    }

    driver->config.cal_data.redc = redc;


    ret = regmap_write(cp, CS40L50_F0_ESTIMATION_REDC, redc);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, CS40L50_DSP_MBOX_F0_EST);
    if (ret)
    {
        return ret;
    }
    ret = regmap_poll_reg(cp, mbox_rd_ptr_value, CS40L50_DSP_MBOX_F0_EST_START, 10, 1);
    if (ret)
    {
        return ret;
    }

    mbox_rd_ptr_value += 4;

    ret = regmap_write(cp, mbox_rd_ptr_addr, mbox_rd_ptr_value);
    if (ret)
    {
        return ret;
    }
    ret = regmap_poll_reg(cp, mbox_rd_ptr_value, CS40L50_DSP_MBOX_F0_EST_DONE, 43, 35);
    if (ret)
    {
        return ret;
    }

    mbox_rd_ptr_value += 4;

    ret = regmap_write(cp, mbox_rd_ptr_addr, mbox_rd_ptr_value);
    if (ret)
    {
        return ret;
    }

    ret = regmap_read(cp, CS40L50_F0_ESTIMATION_F0_EST, &f0);
    if (ret)
    {
        return ret;
    }

    driver->config.cal_data.f0 = f0;
    driver->config.cal_data.is_valid = true;

    return CS40L50_STATUS_OK;
}

/**
 * Enables dynamic f0 and sets the specified threshold
 *
 */
uint32_t cs40l50_set_dynamic_f0(cs40l50_t *driver, bool enable)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, CS40L50_DYNAMIC_F0_ENABLED, (uint32_t) enable);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write(cp, CS40L50_DYNAMIC_F0_THRESHOLD, driver->config.dynamic_f0_threshold);

    return ret;
}

/**
 * Get Dynamic F0 entry
 *
 */
uint32_t cs40l50_get_dynamic_f0(cs40l50_t *driver, cs40l50_df0_table_entry_t *f0_entry)
{
    uint32_t ret, reg_addr;
    cs40l50_df0_table_entry_t f0_read;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);
    uint8_t i;

    reg_addr = CS40L50_DYNAMIC_F0_TABLE;

    for (i = 0; i < CS40L50_DYNAMIC_F0_TABLE_SIZE; i++)
    {
        ret = regmap_read(cp, reg_addr, &(f0_read.table1.word));
        if (ret)
        {
            return ret;
        }

        if (f0_entry->table1.index == f0_read.table1.index)
        {
            f0_entry->table1.word = f0_read.table1.word;
            break;
        }

        reg_addr += 4;
    }

    // Set to default of table entry to indicate index not found; otherwise read Table2 and Table3 contents
    if (i >= CS40L50_DYNAMIC_F0_TABLE_SIZE)
    {
        f0_entry->table1.word = CS40L50_DYNAMIC_F0_TABLE_ENTRY_DEFAULT;
    }
    else
    {
        // Skip to same index but in Table2 section of Dynamic F0 Tables
        reg_addr += CS40L50_DYNAMIC_F0_TABLE_SIZE * CS40L50_DYNAMIC_F0_TABLE_ENTRY_SIZE_BYTES;

        ret = regmap_read(cp, reg_addr, &(f0_entry->table2.word));
        if (ret)
        {
            return ret;
        }

        // Skip to same index but in Table3 section of Dynamic F0 Tables
        reg_addr += CS40L50_DYNAMIC_F0_TABLE_SIZE * CS40L50_DYNAMIC_F0_TABLE_ENTRY_SIZE_BYTES;

        ret = regmap_read(cp, reg_addr, &(f0_entry->table3));
        if (ret)
        {
            return ret;
        }
    }

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

    if (driver->config.broadcast)
        cp = &broadcast_cp;

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

uint32_t cs40l50_configure_gpio_trigger(cs40l50_t *driver, cs40l50_gpio_bank_t gpio, bool rth,
                                        uint8_t attenuation, bool ram, uint8_t plybck_index)
{
    uint32_t ret, data;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    data = plybck_index;
    data |= ((uint8_t) ram) << 8;
    data |= attenuation << 9;
    data |= ((uint8_t) rth) << 16;

    ret = regmap_write(cp, (CS40L50_GPIO_HANDLERS_BASE + (gpio * CS40L50_GPIO_HANDLERS_ENTRY_LENGTH_BYTES)), data);
    return ret;
}

/**
 * Enable the HALO FW Click Compensation
 *
 */
uint32_t cs40l50_set_click_compensation_enable(cs40l50_t *driver, bool f0_enable, bool redc_enable)
{
    uint32_t ret;
    uint32_t enable = 0;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (!driver->config.cal_data.is_valid)
    {
        return CS40L50_STATUS_FAIL;
    }

    if (f0_enable)
        enable = CS40L50_COMPENSATION_ENABLE_F0_MASK;

    if (redc_enable)
        enable |= CS40L50_COMPENSATION_ENABLE_REDC_MASK;

    ret = regmap_write(cp, CS40L50_VIBEGEN_COMPENSATION_ENABLE, (uint32_t) enable);

    return ret;
}

uint32_t cs40l50_set_broadcast_enable(cs40l50_t *driver, bool enable)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    driver->config.broadcast = enable;

    if (enable)
        ret = regmap_write(cp, CS40L50_I2C_BROADCAST,
                            CS40L50_I2C_BROADCAST_EN_MASK |
                            CS40L50_I2C_BROADCAST_ADDR_DEFAULT);
    else
        ret = regmap_write(cp, CS40L50_I2C_BROADCAST,
                            CS40L50_I2C_BROADCAST_ADDR_DEFAULT);

    return ret;
}
#ifndef CS40L50_BAREMETAL
uint32_t cs40l50_trigger_pwle(cs40l50_t *driver, rth_pwle_section_t **s)
{
    int i;
    uint32_t ret, addr;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    regmap_read(cp, CS40L50_VIBEGEN_OWT_BASE_XM , &addr);
    addr = addr & ~(0x800000);
    addr = CS40L50_OWT_WAVE_XM_TABLE + (addr * 4);

    if (driver->config.broadcast)
            cp = &broadcast_cp;

    ret = regmap_write(cp, addr, CS40L50_RTH_TYPE_PWLE);
    if (ret)
    {
        return ret;
    }
    addr += 0xC;

    pwle_default.word3.pwls_ls4  = 2;
    pwle_default.word3.time      = s[0]->duration;
    pwle_default.word4.level_ls8 = s[0]->level & 0xFF;
    pwle_default.word3.level_ms4 = (s[0]->level & 0xF00) >> 8;
    pwle_default.word4.freq      = s[0]->freq;
    pwle_default.word6.level_ls8 = s[0]->level & 0xFF;
    pwle_default.word5.level_ms4 = (s[0]->level & 0xF00) >> 8;
    pwle_default.word5.time      = s[1]->duration;
    pwle_default.word6.freq      = s[1]->freq;

    for (i = 0; i < 6; i++)
    {
        ret = regmap_write(cp, addr, pwle_default.words[i]);
        if (ret)
        {
            return ret;
        }
        addr += 0x4;
    }
    ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, CS40L50_TRIGGER_RTH);
    if (ret)
    {
        return ret;
    }
    return ret;
}

uint32_t cs40l50_trigger_pwle_advanced(cs40l50_t *driver, rth_pwle_section_t **s, uint8_t repeat, uint8_t num_sections)
{
    uint32_t ret, addr;
    int i;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    regmap_read(cp, CS40L50_VIBEGEN_OWT_BASE_XM , &addr);
    addr = addr & ~(0x800000);
    addr = CS40L50_OWT_WAVE_XM_TABLE + (addr * 4);

    if (driver->config.broadcast)
            cp = &broadcast_cp;

    ret = regmap_write(cp, addr, CS40L50_RTH_TYPE_PWLE);
    if (ret)
    {
        return ret;
    }
    addr += 0xC;

    pwle_default.word2.repeat    = repeat;
    pwle_default.word2.pwls_ms4  = (num_sections & 0xF0) >> 4;
    pwle_default.word3.pwls_ls4  = (num_sections & 0xF);
    pwle_default.word3.time      = s[0]->duration;
    pwle_default.word4.level_ls8 = s[0]->level & 0xFF;
    pwle_default.word3.level_ms4 = (s[0]->level & 0xF00) >> 8;
    pwle_default.word4.freq      = s[0]->freq;
    pwle_default.word4.amp_reg   = (s[0]->half_cycles ? 1 : 0);
    pwle_default.word4.chirp     = (s[0]->chirp ? 1 : 0);
    pwle_default.word6.level_ls8 = s[1]->level & 0xFF;
    pwle_default.word5.level_ms4 = (s[1]->level & 0xF00) >> 8;
    pwle_default.word5.time      = s[1]->duration;
    pwle_default.word6.freq      = s[1]->freq;
    pwle_default.word6.amp_reg   = (s[1]->half_cycles ? 1 : 0);
    pwle_default.word6.chirp     = (s[1]->chirp ? 1 : 0);

    for (i = 0; i < 6; i++)
    {
        ret = regmap_write(cp, addr, pwle_default.words[i]);
        if (ret)
        {
            return ret;
        }
        addr += 0x4;
    }
    for (i = 2; i < num_sections; i++)
    {
        pwle_short_default.word1.time      = s[i]->duration;
        pwle_short_default.word1.level_ms8 = (s[i]->level & 0xFF0) >> 4;
        pwle_short_default.word2.level_ls4 = s[i]->level & 0x00F;
        pwle_short_default.word2.freq      = s[i]->freq;
        pwle_short_default.word2.amp_reg   = (s[i]->half_cycles ? 1 : 0);
        pwle_short_default.word2.chirp     = (s[i]->chirp ? 1 : 0);

        ret = regmap_write(cp, addr, pwle_short_default.words[0] >> 4);
        if (ret)
        {
            return ret;
        }
        addr += 0x4;
        uint32_t data = (pwle_short_default.words[0]&0xF) << 20;
        data |= (pwle_short_default.words[1]) >> 4;
        ret = regmap_write(cp, addr, data);
        if (ret)
        {
            return ret;
        }
        addr += 0x4;
        ret = regmap_write(cp, addr, (pwle_short_default.words[1]&0xF) << 20);
        if (ret)
        {
            return ret;
        }
    }

    ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, CS40L50_TRIGGER_RTH);
    if (ret)
    {
        return ret;
    }

    return ret;
}

uint32_t cs40l50_pack_pcm_data(regmap_cp_config_t *cp, int index, uint32_t *word, uint8_t data, uint32_t *addr)
{
    uint32_t ret;

    switch(index%3)
    {
    case 0:
        *word = *word | (data << 16);
        break;
    case 1:
        *word = *word | (data << 8);
        break;
    case 2:
        *word = *word | (data);
        ret = regmap_write(cp, *addr, *word);
        if (ret)
        {
            return ret;
        }
        *addr += 0x4;
        *word = 0;
        break;
    default:
        break;
    };

    return 0;
}

uint32_t cs40l50_trigger_pcm(cs40l50_t *driver, uint8_t *s, uint32_t num_sections, uint16_t buffer_size_samples, uint16_t f0, uint16_t redc)
{
    uint32_t ret, addr;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    regmap_read(cp, CS40L50_VIBEGEN_OWT_BASE_XM, &addr);
    addr = addr & ~(0x800000);
    addr = CS40L50_OWT_WAVE_XM_TABLE + (addr * 4);

    if (driver->config.broadcast)
        cp = &broadcast_cp;

    ret = regmap_write(cp, addr, CS40L50_RTH_TYPE_PCM); //write the type of waveform
    if (ret)
    {
        return ret;
    }

    addr += 0xC;
    ret = regmap_write(cp, addr, num_sections); //Writes the wavelengh that also is the number of sections
    if (ret)
    {
        return ret;
    }
    addr += 0x4;
    ret = regmap_write(cp, addr, (f0 << 12) | redc); //Writes F0 and ReDC Values
    if (ret)
    {
        return ret;
    }
    addr += 0x4;
    uint32_t word = 0;
    for (int i = 0; i < buffer_size_samples; i++)
    {
        ret = cs40l50_pack_pcm_data(cp, i, &word, s[i], &addr);
        if (ret)
        {
            return ret;
        }
    }

    ret = regmap_write(cp, CS40L50_DSP_VIRTUAL1_MBOX_1, CS40L50_TRIGGER_RTH);
    if (ret)
    {
        return ret;
    }
    if (buffer_size_samples < num_sections)
    {
        for (int i = buffer_size_samples; i < num_sections; i++)
        {
            ret = cs40l50_pack_pcm_data(cp, i, &word, s[i], &addr);
            if (ret)
            {
                return ret;
            }
        }
        if ((num_sections % 3) != 0)
        {
            ret = regmap_write(cp, addr, word);
            if (ret)
            {
                return ret;
            }
        }

    }
    return ret;
}
#endif //CS40L50_BAREMETAL
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
