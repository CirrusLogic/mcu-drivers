/**
 * @file cs40l25.c
 *
 * @brief The CS40L25 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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
#include "cs40l25.h"
#include "bsp_driver_if.h"
#include "string.h"
#include "cs40l25_syscfg_regs.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
//#define TO_FIX_IN_PORTING

/**
 * Default Interrupt Mask for IRQ2_MASK_1 register
 *
 * The interrupts that are unmasked in Interrupt Status and Mask Control (IRQ2) are:
 * - b31 - AMP_ERR_MASK1
 * - b17 - TEMP_ERR_MASK1
 * - b15 - TEMP_WARN_RISE_MASK1
 * - b8  - BST_SHORT_ERR_MASK1
 * - b7  - BST_DCM_UVP_ERR_MASK1
 * - b6  - BST_OVP_ERR_MASK1
 *
 * @see IRQ2_IRQ2_MASK_1_REG
 *
 */
#define CS40L25_INT2_MASK_DEFAULT               (0x7FFD7E3F)
#define CS40L25_IRQ2_MASK1_DEFAULT  (CS40L25_INT2_MASK_DEFAULT)
#define CS40L25_IRQ2_MASK2_DEFAULT  (0xFFFFFFFF)
#define CS40L25_IRQ2_MASK3_DEFAULT  (0xFFFF87FF)
#define CS40L25_IRQ2_MASK4_DEFAULT  (0xFEFFFFFF)

/**
 * Event Flag Mask for IRQ2 Status Bits for Speaker Safe Mode Boost-related Events
 *
 * If any of the bits in the mask below are set in IRQ2_EINT_1, the amplifier will have entered Speaker Safe Mode
 * and will require additional steps to release from Speaker Safe Mode.
 * - b8 - BST_SHORT_ERR_MASK1
 * - b7 - BST_DCM_UVP_ERR_MASK1
 * - b6 - BST_OVP_ERR_MASK1
 *
 * These bits correspond to the following flags in Event Flags:
 * - BST_SHORT_ERR_MASK1 - CS40L25_EVENT_FLAG_BOOST_INDUCTOR_SHORT
 * - BST_DCM_UVP_ERR_MASK1 - CS40L25_EVENT_FLAG_BOOST_UNDERVOLTAGE
 * - BST_OVP_ERR_MASK1 - CS40L25_EVENT_FLAG_BOOST_OVERVOLTAGE
 *
 * @see IRQ2_EINT_1
 * @see Datasheet Section 4.16.1.1
 *
 */
#define CS40L25_EVENT_FLAGS_BOOST_CYCLE         (CS40L25_EVENT_FLAG_BOOST_INDUCTOR_SHORT | \
                                                 CS40L25_EVENT_FLAG_BOOST_UNDERVOLTAGE | \
                                                 CS40L25_EVENT_FLAG_BOOST_OVERVOLTAGE)

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
 * @see MSM_ERROR_RELEASE_REG
 * @see Datasheet Section 4.16.1.1
 *
 */
#define CS40L25_ERR_RLS_SPEAKER_SAFE_MODE_MASK  (0x0000007E)

/**
 * Value of CS40L25_CAL_STATUS that indicates Calibration success
 *
 * @see CS40L25_CAL_STATUS
 *
 */
#define CS40L25_CAL_STATUS_CALIB_SUCCESS        (0x1)

/**
 * @defgroup CS40L25_POWERCONTROL_
 * @brief Valid values to write to DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG (i.e. POWERCONTROL)
 *
 * @{
 */
#define CS40L25_POWERCONTROL_NONE               (0)
#define CS40L25_POWERCONTROL_HIBERNATE          (1)
#define CS40L25_POWERCONTROL_WAKEUP             (2)
#define CS40L25_POWERCONTROL_FRC_STDBY          (3)
/** @} */

#define CS40L25_POWERSTATE_BLANK                (0)
#define CS40L25_POWERSTATE_ACTIVE               (1)
#define CS40L25_POWERSTATE_STANDBY              (2)
#define CS40L25_POWERSTATE_HIBERNATE            (3)

#define CS40L25_FIRMWARE_ID_ADDR                (0x0280000C)

#define CS40L2X_EVENT_DISABLED      0x000000
#define CS40L2X_EVENT_GPIO1_ENABLED 0x000001
#define CS40L2X_EVENT_GPIO2_ENABLED 0x000002
#define CS40L2X_EVENT_GPIO3_ENABLED 0x000004
#define CS40L2X_EVENT_GPIO4_ENABLED 0x000008
#define CS40L2X_EVENT_START_ENABLED 0x000010
#define CS40L2X_EVENT_END_ENABLED   0x000020
#define CS40L2X_EVENT_READY_ENABLED 0x000040
#define CS40L2X_EVENT_HARDWARE_ENABLED  0x800000

#define CS40L2X_EVENT_CTRL_GPIO1_FALL   0
#define CS40L2X_EVENT_CTRL_GPIO1_RISE   1
#define CS40L2X_EVENT_CTRL_GPIO2_FALL   2
#define CS40L2X_EVENT_CTRL_GPIO2_RISE   3
#define CS40L2X_EVENT_CTRL_GPIO3_FALL   4
#define CS40L2X_EVENT_CTRL_GPIO3_RISE   5
#define CS40L2X_EVENT_CTRL_GPIO4_FALL   6
#define CS40L2X_EVENT_CTRL_GPIO4_RISE   7
#define CS40L2X_EVENT_CTRL_TRIG_STOP    10
#define CS40L2X_EVENT_CTRL_GPIO_STOP    11
#define CS40L2X_EVENT_CTRL_READY    12
#define CS40L2X_EVENT_CTRL_HARDWARE 13
#define CS40L2X_EVENT_CTRL_TRIG_SUSP    14
#define CS40L2X_EVENT_CTRL_TRIG_RESM    15
#define CS40L2X_EVENT_CTRL_NONE     0xFFFFFF

#define CS40L25_EVENT_SOURCES       (8)
#define CS40L25_EVENT_HW_SOURCES    (6)

#define CS40L25_IMASKSEQ_WORD_1(B) (((B & 0x000000FF) << 16))
#define CS40L25_IMASKSEQ_WORD_2(B) ((B & 0xFFFFFF00) >> 8)

#define CS40L25_CAL_Q_POLL_COUNT                (30)

#define CS40L25_FWID_CAL                                (0x1400C6)

#define CS40L25_FIRMWARE_REVISION 0x2800010

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/**
 * CS40L25 RevB0 Register Patch Errata
 *
 * The array is in the form:
 * - word0 - Length of rest of patch in words (i.e. NOT including this word)
 * - word1 - 1st register address to patch
 * - word2 - 1st register value
 * - word3 - 2nd register address to patch
 * - word4 - 2nd register value
 * - ...
 *
 * @note To simplify the Reset SM, this includes the configuration for IRQ1 and INTb GPIO
 *
 */
static const uint32_t cs40l25_revb0_errata_patch[] =
{
    0x00000018, //
    0x00003008, 0x000C1837,
    0x00003014, 0x03008E0E,
    CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_1,
    CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_2,
    0x0000391C, 0x004DC080,
    0x00004170, 0x002F0065,
    0x00004360, 0x00002B4F,
    0x00004100, 0x00000000,
    0x00004310, 0x00000000,
    CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_1,
    CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_2,
    0x00004400, 0x00000000,
};

/**
 * Register configuration after HALO FW is loaded in Boot SM
 *
 * List is in the form:
 * - word0 - Address of first configuration register
 * - word1 - Value of first configuration register
 * - word2 - Address of second configuration register
 * - word3 - Value of second configuration register
 * - ...
 *
 */
static const uint32_t cs40l25_post_boot_config[] =
{
    CS40L25_MIXER_DSP1RX4_INPUT_REG, CS40L25_INPUT_SRC_VPMON,
};

/**
 * Register configuration to send just before the CS40L25 is powered up in Power Up SM
 *
 * List is in the form:
 * - word1 - Address of first configuration register
 * - word2 - Value of first configuration register
 * - word3 - Address of second configuration register
 * - word4 - Value of second configuration register
 * - ...
 *
 * @see cs40l25_power_up
 *
 */
static const uint32_t cs40l25_pup_patch[] =
{
    0x02BC2020, 0x00000000,
    0x02BC20E0, 0x00000000,
    0x00002900, 0x00000002,
};

/**
 * Register configuration to send during BHM disable
 *
 * List is in the form:
 * - word1 - Address of first configuration register
 * - word2 - Value of first configuration register
 * - word3 - Address of second configuration register
 * - word4 - Value of second configuration register
 * - ...
 *
 * @see cs40l25_power_down
 *
 */
static const uint32_t cs40l25_bhm_revert_patch[] =
{
    0x00002014, 0x00000000,
    0x00002018, 0x00003321,
    0x00002418, 0x00000007,
    0x00002420, 0x00000007,
    0x00006000, 0x00008000,
    0x00010910, 0xFFFFFFFF,
    0x00010914, 0xFFFFFFFF,
};

/**
 * Register addresses to set all HALO sample rates to the same value.
 *
 * Sent just before the CS40L25 is powered up in Power Up SM.  All register values will be set to
 * CS40L25_DSP1_SAMPLE_RATE_G1R2.
 *
 * List is in the form:
 * - word0 - Address of first configuration register
 * - word1 - Address of second configuration register
 * - ...
 *
 * @see cs40l25_power_up
 * @see CS40L25_DSP1_SAMPLE_RATE_G1R2
 *
 */
static const uint32_t cs40l25_frame_sync_regs[] =
{
    XM_UNPACKED24_DSP1_SAMPLE_RATE_RX1_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_RX2_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_RX3_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_RX4_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_RX5_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_RX6_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_RX7_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_RX8_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_TX1_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_TX2_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_TX3_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_TX4_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_TX5_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_TX6_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_TX7_REG,
    XM_UNPACKED24_DSP1_SAMPLE_RATE_TX8_REG
};

/**
 * Register addresses to modify during Configure SM
 *
 * Sent after the CS40L25 has been reset and, if firmware is available, has been booted.
 *
 * List is in the form:
 * - word0 - Address of first configuration register
 * - word1 - Address of second configuration register
 * - ...
 *
 * @see cs40l25_configure
 * @see cs40l25_t member config_regs
 * @see cs40l25_config_registers_t
 *
 * @warning  The list of registers MUST correspond to the union of structs in  in cs40l25_config_registers_t.
 *
 */
static const uint32_t cs40l25_config_register_symbols[CS40L25_CONFIG_REGISTERS_TOTAL] =
{
    CS40L25_SYM_GENERAL_GPIO_BUTTONDETECT,
    CS40L25_SYM_GENERAL_GPIO_ENABLE,
    CS40L25_SYM_GENERAL_GAIN_CONTROL,
    CS40L25_SYM_GENERAL_INDEXBUTTONPRESS,
    CS40L25_SYM_GENERAL_INDEXBUTTONPRESS,
    CS40L25_SYM_GENERAL_INDEXBUTTONPRESS,
    CS40L25_SYM_GENERAL_INDEXBUTTONPRESS,
    CS40L25_SYM_GENERAL_INDEXBUTTONRELEASE,
    CS40L25_SYM_GENERAL_INDEXBUTTONRELEASE,
    CS40L25_SYM_GENERAL_INDEXBUTTONRELEASE,
    CS40L25_SYM_GENERAL_INDEXBUTTONRELEASE,
    CS40L25_SYM_CLAB_CLAB_ENABLED,
    CS40L25_SYM_CLAB_PEAK_AMPLITUDE_CONTROL,
    CS40L25_SYM_GENERAL_EVENTCONTROL,
};

static const uint32_t cs40l25_wseq_regs[] =
{
    BOOST_VBST_CTL_1_REG,           0x00000000,
    BOOST_VBST_CTL_2_REG,           0x00000001,
    BOOST_BST_IPK_CTL_REG,          0x0000004A,
    BOOST_BST_LOOP_COEFF_REG,       0x00002424,
    BOOST_LBST_SLOPE_REG,           0x00007500,
    CS40L25_INTP_AMP_CTRL_REG,      0x00008000,
    CS40L25_WAKESRC_CTL_REG,        0x00000008,
    CS40L25_GPIO_PAD_CONTROL_REG,   0x03010000,
    CCM_REFCLK_INPUT_REG,           0x00000010,
    0x00003018,                     0x00000000,
    0x00002D20,                     0x00000000,
    DATAIF_ASP_ENABLES1_REG,        0x00000000,
    DATAIF_ASP_CONTROL1_REG,        0x00000028,
    CCM_FS_MON_0_REG,               0x00000000,
    DATAIF_ASP_CONTROL2_REG,        0x18180200,
    DATAIF_ASP_FRAME_CONTROL5_REG,  0x00000100,
    DATAIF_ASP_FRAME_CONTROL1_REG,  0x03020100,
    DATAIF_ASP_DATA_CONTROL5_REG,   0x00000018,
    DATAIF_ASP_DATA_CONTROL1_REG,   0x00000018,
    MSM_BLOCK_ENABLES_REG,          0x00003321,
    MSM_BLOCK_ENABLES2_REG,         0x10000010,
    CS40L25_MIXER_DACPCM1_INPUT_REG, CS40L25_INPUT_SRC_DSP1TX1,
    CS40L25_MIXER_DSP1RX1_INPUT_REG, CS40L25_INPUT_SRC_ASPRX1,
    CS40L25_MIXER_DSP1RX2_INPUT_REG, CS40L25_INPUT_SRC_VMON,
    CS40L25_MIXER_DSP1RX3_INPUT_REG, CS40L25_INPUT_SRC_VMON,
    CS40L25_MIXER_DSP1RX4_INPUT_REG, CS40L25_INPUT_SRC_VMON
};

/**
 * Register/DSP Memory addresses to read during Get DSP Status SM
 *
 * List is in the form:
 * - word0 - Address of first status register
 * - word1 - Address of second status register
 * - ...
 *
 * @see cs40l25_get_dsp_status
 * @see cs40l25_dsp_status_t
 *
 * @warning  The list of registers MUST correspond to the union of structs in  in cs40l25_dsp_status_t.
 *
 */
static const uint32_t cs40l25_dsp_status_symbols[CS40L25_DSP_STATUS_WORDS_TOTAL] =
{
        CS40L25_SYM_GENERAL_HALO_STATE,
        CS40L25_SYM_GENERAL_HALO_HEARTBEAT
};

static const uint32_t cs40l2x_event_controls[CS40L25_EVENT_SOURCES] =
{
    CS40L25_SYM_GENERAL_HARDWAREEVENT, // For the Event Handler SM, HW Event handling must be first
    CS40L25_SYM_GENERAL_GPIO1EVENT,
    CS40L25_SYM_GENERAL_GPIO2EVENT,
    CS40L25_SYM_GENERAL_GPIO3EVENT,
    CS40L25_SYM_GENERAL_GPIO4EVENT,
    CS40L25_SYM_GENERAL_GPIOPLAYBACKEVENT,
    CS40L25_SYM_GENERAL_TRIGGERPLAYBACKEVENT,
    CS40L25_SYM_GENERAL_RXREADYEVENT,
};

static const unsigned int cs40l2x_event_masks[CS40L25_EVENT_SOURCES] =
{
    CS40L2X_EVENT_HARDWARE_ENABLED, // For the Event Handler SM, HW Event handling must be first
    CS40L2X_EVENT_GPIO1_ENABLED,
    CS40L2X_EVENT_GPIO2_ENABLED,
    CS40L2X_EVENT_GPIO3_ENABLED,
    CS40L2X_EVENT_GPIO4_ENABLED,
    CS40L2X_EVENT_START_ENABLED | CS40L2X_EVENT_END_ENABLED,
    CS40L2X_EVENT_START_ENABLED | CS40L2X_EVENT_END_ENABLED,
    CS40L2X_EVENT_READY_ENABLED,
};

static const uint32_t cs40l25_event_value_to_flag_map[] =
{
    CS40L25_EVENT_FLAG_GPIO_1_RELEASE,
    CS40L25_EVENT_FLAG_GPIO_1_PRESS,
    CS40L25_EVENT_FLAG_GPIO_2_RELEASE,
    CS40L25_EVENT_FLAG_GPIO_2_PRESS,
    CS40L25_EVENT_FLAG_GPIO_3_RELEASE,
    CS40L25_EVENT_FLAG_GPIO_3_PRESS,
    CS40L25_EVENT_FLAG_GPIO_4_RELEASE,
    CS40L25_EVENT_FLAG_GPIO_4_PRESS,
    0,
    0,
    CS40L25_EVENT_FLAG_CP_PLAYBACK_DONE,
    CS40L25_EVENT_FLAG_GPIO_PLAYBACK_DONE,
    CS40L25_EVENT_FLAG_READY_FOR_DATA,
    0,
    CS40L25_EVENT_FLAG_CP_PLAYBACK_SUSPEND,
    CS40L25_EVENT_FLAG_CP_PLAYBACK_RESUME
};

static const uint32_t cs40l25_irq2_mask_1_to_event_flag_map[CS40L25_EVENT_HW_SOURCES * 2] =
{
    IRQ2_IRQ2_EINT_1_AMP_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_AMP_SHORT,
    IRQ2_IRQ2_EINT_1_TEMP_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_OVERTEMP_ERROR,
    IRQ2_IRQ2_EINT_1_TEMP_WARN_RISE_EINT2_BITMASK, CS40L25_EVENT_FLAG_OVERTEMP_WARNING,
    IRQ2_IRQ2_EINT_1_BST_SHORT_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_BOOST_INDUCTOR_SHORT,
    IRQ2_IRQ2_EINT_1_BST_DCM_UVP_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_BOOST_UNDERVOLTAGE,
    IRQ2_IRQ2_EINT_1_BST_OVP_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_BOOST_OVERVOLTAGE,
};

static const uint32_t cs40l25_irqmaskseq_patch[] =
{
    CS40L25_IMASKSEQ_WORD_1(CS40L25_IRQ2_MASK1_DEFAULT),
    CS40L25_IMASKSEQ_WORD_2(CS40L25_IRQ2_MASK1_DEFAULT),
    CS40L25_IMASKSEQ_WORD_1(CS40L25_IRQ2_MASK2_DEFAULT),
    CS40L25_IMASKSEQ_WORD_2(CS40L25_IRQ2_MASK2_DEFAULT),
    CS40L25_IMASKSEQ_WORD_1(CS40L25_IRQ2_MASK3_DEFAULT),
    CS40L25_IMASKSEQ_WORD_2(CS40L25_IRQ2_MASK3_DEFAULT),
    CS40L25_IMASKSEQ_WORD_1(CS40L25_IRQ2_MASK4_DEFAULT),
    CS40L25_IMASKSEQ_WORD_2(CS40L25_IRQ2_MASK4_DEFAULT),
};

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
#ifdef CS40L25_USEFUL_UNUSED
/**
 * Find if an algorithm is in the algorithm list and return true if it is.
 * Returns false if not.
 */
static bool cs40l25_find_algid(cs40l25_t *driver, uint32_t algid_id)
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

/**
 * Find if a symbol is in the symbol table and return its address if it is.
 * Returns 0 if not found.
 */
static uint32_t cs40l25_find_symbol(cs40l25_t *driver, uint32_t symbol_id)
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

static uint32_t cs40l25_cp_bulk_write_block(cs40l25_t *driver, uint32_t addr, uint8_t *bytes, uint32_t length);

/**
 * Notify the driver when the CS40L25 INTb GPIO drops low.
 *
 * Implementation of cs40l25_private_functions_t.irq_callback
 *
 */
static void cs40l25_irq_callback(uint32_t status, void *cb_arg)
{
    cs40l25_t *d;

    d = (cs40l25_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Switch driver mode to CS40L25_MODE_HANDLING_EVENTS
        d->mode = CS40L25_MODE_HANDLING_EVENTS;
        d->event_counter++;
    }

    return;
}

uint32_t cs40l25_wseq_table_add(cs40l25_t *driver, uint32_t address, uint32_t value)
{
    uint32_t ret = CS40L25_STATUS_OK;

    cs40l25_wseq_entry_t *table = (cs40l25_wseq_entry_t *) &driver->wseq_table;
    uint32_t num_entries = driver->wseq_num_entries;

    if (num_entries < CS40L25_WSEQ_MAX_ENTRIES)
    {
        table[num_entries].address_ms = (address & 0xFF00) >> 8;
        table[num_entries].address_ls = address & 0x00FF;
        table[num_entries].val_3 = (value & 0xFF000000) >> 24;
        table[num_entries].val_2 = (value & 0x00FF0000) >> 16;
        table[num_entries].val_1 = (value & 0x0000FF00) >> 8;
        table[num_entries].val_0 = value & 0x000000FF;
        table[num_entries].changed = 1;
        // Make sure reserved* members are 0
        table[num_entries].reserved_0 = 0;
        table[num_entries].reserved_1 = 0;

        driver->wseq_num_entries += 1;
    }
    else
    {
        ret = CS40L25_STATUS_FAIL;
    }

    return ret;
}

uint32_t cs40l25_wseq_table_update(cs40l25_t *driver, uint32_t address, uint32_t value)
{
    uint32_t ret = CS40L25_STATUS_OK;
    if (!driver->wseq_initialized)
    {
        return ret;
    }
    cs40l25_wseq_entry_t *table = (cs40l25_wseq_entry_t *) &driver->wseq_table;
    if (address < 0xFFFF)
    {
        uint32_t num_entries = driver->wseq_num_entries;
        bool address_found = false;
        for (uint32_t i = 0; i < num_entries; i++) {
            uint32_t temp_address = (table[i].address_ms << 8) | (table[i].address_ls);
            uint32_t temp_value = (table[i].val_3 << 24) | (table[i].val_2 << 16) |
                                  (table[i].val_1 << 8) | (table[i].val_0);
            if (temp_address == address)
            {
                if ((temp_value != value) &&
                    (address != CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG))
                {
                    table[i].val_3 = (value & 0xFF000000) >> 24;
                    table[i].val_2 = (value & 0x00FF0000) >> 16;
                    table[i].val_1 = (value & 0x0000FF00) >> 8;
                    table[i].val_0 = value & 0x000000FF;
                    table[i].changed = 1;
                }

                address_found = true;
            }
        }

        if (!address_found && address != CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG)
        {
            //Add new address to end of table if there is space
            ret = cs40l25_wseq_table_add(driver, address, value);
            num_entries = driver->wseq_num_entries;
            if (ret == CS40L25_STATUS_OK)
            {
                //Shift the locking entries ( the last two entries ) back to the end
                //after appending the new entry. Set all entries to changed = 1, as
                //they now have a new position in POWERONSEQUENCE and must be written
                //over the bus.
                cs40l25_wseq_entry_t temp;
                temp = table[num_entries - 1];
                table[num_entries - 1] = table[num_entries - 2];
                table[num_entries - 1].changed = 1;
                table[num_entries - 2] = table[num_entries - 3];
                table[num_entries - 2].changed = 1;
                table[num_entries - 3] = temp;
            }
        }
    }

    return ret;
}

uint32_t cs40l25_wseq_add_block(cs40l25_t *driver, uint32_t *entries, uint32_t num_entries)
{
    for (uint32_t i = 0; i < num_entries; i++) {
        uint32_t ret;
        if (entries[2 * i] == CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG)
        {
            continue;
        }
        ret = cs40l25_wseq_table_add(driver, entries[2 * i], entries[2 * i + 1]);
        if (ret != CS40L25_STATUS_OK) {
            return ret;
        }
    }
    return CS40L25_STATUS_OK;
}

/**
 * Reads the contents of a single register/memory address
 *
 * Implementation of cs40l25_private_functions_t.read_reg
 *
 */
static uint32_t cs40l25_read_reg(cs40l25_t *driver, uint32_t addr, uint32_t *val)
{
    uint32_t ret = CS40L25_STATUS_FAIL;
    cs40l25_bsp_config_t *b = &(driver->config.bsp_config);

    /*
     * Switch from Little-Endian contents of uint32_t 'addr' to Big-Endian format required for Control Port transaction.
     * Since register address is first written, cp_write_buffer[] is filled with register address.
     *
     * FIXME: This is not platform independent.
     */
    b->cp_write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    b->cp_write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    b->cp_write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    b->cp_write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    // Currently only I2C transactions are supported
    if (b->bus_type == CS40L25_BUS_TYPE_I2C)
    {
        uint32_t bsp_status;

        bsp_status = bsp_driver_if_g->i2c_read_repeated_start(b->bsp_dev_id,
                                                              b->cp_write_buffer,
                                                              4,
                                                              b->cp_read_buffer,
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
            ADD_BYTE_TO_WORD(*val, b->cp_read_buffer[0], 3);
            ADD_BYTE_TO_WORD(*val, b->cp_read_buffer[1], 2);
            ADD_BYTE_TO_WORD(*val, b->cp_read_buffer[2], 1);
            ADD_BYTE_TO_WORD(*val, b->cp_read_buffer[3], 0);

            ret = CS40L25_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Writes the contents of a single register/memory address
 *
 * Implementation of cs40l25_private_functions_t.write_reg
 *
 */
static uint32_t cs40l25_write_reg(cs40l25_t *driver, uint32_t addr, uint32_t val)
{
    uint32_t ret = CS40L25_STATUS_FAIL;
    uint32_t bsp_status = BSP_STATUS_OK;
    cs40l25_bsp_config_t *b = &(driver->config.bsp_config);

    //Update corresponding entry in wseq_table if it exists
    cs40l25_wseq_table_update(driver, addr, val);

    /*
     * Copy Little-Endian contents of 'addr' and 'val' to the Big-Endian format required for Control Port transactions
     * using a uint8_t cp_write_buffer.
     *
     * FIXME: This is not platform independent.
     */
    b->cp_write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    b->cp_write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    b->cp_write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    b->cp_write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);
    b->cp_write_buffer[4] = GET_BYTE_FROM_WORD(val, 3);
    b->cp_write_buffer[5] = GET_BYTE_FROM_WORD(val, 2);
    b->cp_write_buffer[6] = GET_BYTE_FROM_WORD(val, 1);
    b->cp_write_buffer[7] = GET_BYTE_FROM_WORD(val, 0);

    // Currently only I2C transactions are supported
    if (b->bus_type == CS40L25_BUS_TYPE_I2C)
    {
        bsp_status = bsp_driver_if_g->i2c_write(b->bsp_dev_id,
                                                b->cp_write_buffer,
                                                8,
                                                NULL,
                                                NULL);
    }

    if (BSP_STATUS_OK == bsp_status)
    {
        ret = CS40L25_STATUS_OK;
    }

    return ret;
}

static uint32_t cs40l25_write_acked_reg(cs40l25_t *driver, uint32_t addr, uint32_t val, uint32_t acked_val)
{
    int count;
    uint32_t temp_val;
    cs40l25_write_reg(driver, addr, val);

    for (count = 0 ; count < CS40L25_POLL_ACK_CTRL_MAX; count++)
    {
        bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS, NULL, NULL);

        cs40l25_read_reg(driver, addr, &temp_val);
        if (temp_val == acked_val)
        {
            return CS40L25_STATUS_OK;
        }
    }
    return CS40L25_STATUS_FAIL;
}

static uint32_t cs40l25_power_up(cs40l25_t *driver)
{
    uint32_t count = 0;
    uint32_t temp_reg_val = 0;
    uint32_t reg_address;

    const uint32_t *errata_write = cs40l25_revb0_errata_patch;
    uint32_t errata_length = *errata_write;
    // Skip first word which is errata length
    errata_write++;
    // Start sending errata
    for (count = 0; count < errata_length; count += 2)
    {
        cs40l25_write_reg(driver, *(errata_write), *(errata_write + 1));
        errata_write += 2;
    }

    // Set HALO DSP Sample Rate registers to G1R2
    for (count = 0; count < (sizeof(cs40l25_frame_sync_regs)/sizeof(uint32_t)); count++)
    {
        cs40l25_write_reg(driver, cs40l25_frame_sync_regs[count], CS40L25_DSP1_SAMPLE_RATE_G1R2);
    }

    // Send words of Power Up Patch
    for (count = 0; count < (sizeof(cs40l25_pup_patch)/sizeof(uint32_t)); count += 2)
    {
        cs40l25_write_reg(driver, cs40l25_pup_patch[count], cs40l25_pup_patch[count + 1]);
    }

    // Read the HALO DSP CCM control register
    cs40l25_read_reg(driver, XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG, &temp_reg_val);
    // Enable clocks to HALO DSP core
    temp_reg_val |= XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_EN_BITMASK | XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_RESET_BITMASK;
    cs40l25_write_reg(driver, XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG, temp_reg_val);

    reg_address = cs40l25_find_symbol(driver, CS40L25_SYM_GENERAL_HALO_STATE);
    if (!reg_address)
    {
        return CS40L25_STATUS_FAIL;
    }

    for (count = 0; count < CS40L25_POLL_OTP_BOOT_DONE_MAX; count++)
    {
        cs40l25_read_reg(driver, reg_address, &temp_reg_val);
        if (temp_reg_val == 0xCB)
        {
            break;
        }
        bsp_driver_if_g->set_timer(CS40L25_T_BST_PUP_MS, NULL, NULL);
    }
    if (count >= CS40L25_POLL_OTP_BOOT_DONE_MAX)
    {
        return CS40L25_STATUS_FAIL;
    }

    cs40l25_read_reg(driver, XM_UNPACKED24_DSP1_SCRATCH_REG, &temp_reg_val);

    if (temp_reg_val)
    {
        return CS40L25_STATUS_FAIL;
    }

    return CS40L25_STATUS_OK;
}

static uint32_t cs40l25_power_down(cs40l25_t *driver)
{
    uint32_t temp_reg_val = 0;
    uint32_t reg_address;
    uint32_t ret;

    // Force fw into standby
    if (driver->state == CS40L25_STATE_CAL_POWER_UP)
    {
        reg_address = cs40l25_find_symbol(driver, CS40L25_SYM_GENERAL_SHUTDOWNREQUEST);
        if (!reg_address)
        {
            return CS40L25_STATUS_FAIL;
        }

        ret = cs40l25_write_acked_reg(driver, reg_address, 1, 0);

        if (ret != CS40L25_STATUS_OK)
        {
            return CS40L25_STATUS_FAIL;
        }
    }
    else
    {
        ret = cs40l25_write_acked_reg(driver,
                                      DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                      CS40L25_POWERCONTROL_FRC_STDBY,
                                      CS40L25_POWERCONTROL_NONE);

        if (ret != CS40L25_STATUS_OK)
        {
            return CS40L25_STATUS_FAIL;
        }
    }

    // Read so we can update bits
    cs40l25_read_reg(driver, XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG, &temp_reg_val);
    // Disable HALO DSP core
    temp_reg_val &= ~XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_EN_BITMASK;
    cs40l25_write_reg(driver, XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG, temp_reg_val);

    return CS40L25_STATUS_OK;
}

static uint32_t cs40l25_exit_bhm(cs40l25_t *driver)
{
    uint32_t count = 0;
    uint32_t temp_reg_val = 0;

#ifndef CS40L25_IS_OPEN_LOOP
    // Request BHM shuts down
    cs40l25_write_reg(driver, DSP_BHM_AMP_SHUTDOWNREQUEST_REG, DSP_BHM_AMP_SHUTDOWNREQUEST_BITMASK);
    // Wait for at least 1ms
    bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, NULL, NULL);

    // If OTP_BOOT_DONE is set
    for (count = 0; count < CS40L25_POLL_OTP_BOOT_DONE_MAX; count++)
    {
        // Read SHUTDOWNREQUEST to see if the reg has been cleared
        cs40l25_read_reg(driver, DSP_BHM_AMP_SHUTDOWNREQUEST_REG, &temp_reg_val);
        if (temp_reg_val == 0)
        {
            break;
        }
        // If time left to poll, read OTP_BOOT_DONE again
        bsp_driver_if_g->set_timer(CS40L25_POLL_OTP_BOOT_DONE_MS, NULL, NULL);
    }
    // If polling period expired, indicate ERROR
    if (count >= CS40L25_POLL_OTP_BOOT_DONE_MAX)
    {
        return CS40L25_STATUS_FAIL;
    }

    // Read BHM_STATEMACHINE
    cs40l25_read_reg(driver, DSP_BHM_STATEMACHINE_REG, &temp_reg_val);

    // If STATEMACHINE != shutdown
    if (temp_reg_val != DSP_BHM_STATEMACHINE_SHUTDOWN)
    {
        return CS40L25_STATUS_FAIL;
    }

    // Read BHM_AMP_STATUS
    cs40l25_read_reg(driver, DSP_BHM_AMP_STATUS_REG, &temp_reg_val);

    // If any errors:
    if (temp_reg_val & (DSP_BHM_AMP_STATUS_OTP_ERROR_BITMASK |
                        DSP_BHM_AMP_STATUS_AMP_ERROR_BITMASK |
                        DSP_BHM_AMP_STATUS_TEMP_RISE_WARN_BITMASK |
                        DSP_BHM_AMP_STATUS_TEMP_ERROR_BITMASK))
    {
        return CS40L25_STATUS_FAIL;
    }
#endif

    // start basic mode revert
    for (count = 0; count < (sizeof(cs40l25_bhm_revert_patch)/sizeof(uint32_t)); count += 2)
    {
        // Send next words of BHM revert patch set
        cs40l25_write_reg(driver, cs40l25_bhm_revert_patch[count], cs40l25_bhm_revert_patch[count + 1]);
    }

    return CS40L25_STATUS_OK;
}

static uint32_t cs40l25_get_dsp_status(cs40l25_t *driver)
{
    uint32_t count = 0;

    // Get pointer to status passed in to Control Request
    cs40l25_dsp_status_t *status = (cs40l25_dsp_status_t *) driver->current_request.arg;

    uint32_t reg_address;
    for (count = 0; count < CS40L25_DSP_STATUS_WORDS_TOTAL; count++)
    {
        reg_address = cs40l25_find_symbol(driver, cs40l25_dsp_status_symbols[count]);
        if (!reg_address)
        {
            return CS40L25_STATUS_FAIL;
        }
        // Read the DSP Status field address
        cs40l25_read_reg(driver, reg_address, &(status->data.words[count]));
    }

    // Wait at least 10ms
    bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_10MS, NULL, NULL);

    for (count = 0; count < CS40L25_DSP_STATUS_WORDS_TOTAL; count++)
    {
        uint32_t temp_reg_val = 0;

        reg_address = cs40l25_find_symbol(driver, cs40l25_dsp_status_symbols[count]);
        if (!reg_address)
        {
            return CS40L25_STATUS_FAIL;
        }
        // Read the DSP Status field address
        cs40l25_read_reg(driver, reg_address, &temp_reg_val);

        // If the current field is HALO_HEARTBEAT, and there is a change in subsequent values
        if ((count == 1) && (temp_reg_val != status->data.words[count]))
        {
            status->is_hb_inc = true;
        }

        status->data.words[count] = temp_reg_val;
    }

    // Assess if Calibration is applied
    if ((status->data.cal_set_status == 2) &&
        (status->data.cal_r_selected == status->data.cal_r) &&
#ifdef TO_FIX_IN_PORTING
        (status->data.cal_r == driver->cal_data.r) &&
#endif
        (status->data.cspl_state == 0) &&
        (status->data.halo_state == 2))
    {
        status->is_calibration_applied = true;
    }

    return CS40L25_STATUS_OK;
}

static uint32_t cs40l25_hibernate(cs40l25_t *driver)
{
    uint32_t count = 0;
    uint32_t reg_address;

    reg_address = cs40l25_find_symbol(driver, CS40L25_SYM_GENERAL_POWERONSEQUENCE);
    if (!reg_address)
    {
        return CS40L25_STATUS_FAIL;
    }

    while (count < driver->wseq_num_entries)
    {
        if (driver->wseq_table[count].changed == 1)
        {
            //Write 16bit address and 32bit value to poweronsequence
            cs40l25_cp_bulk_write_block(driver,
                                        reg_address + (8 * count),
                                        (uint8_t *) driver->wseq_table[count].words,
                                        8);

            driver->wseq_table[count].changed = 0;
            count++;
            continue;
        }
        count++;
    }

    cs40l25_write_reg(driver, reg_address + (8 * count), 0x00FFFFFF);

    cs40l25_write_reg(driver, DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG, CS40L25_POWERCONTROL_HIBERNATE);

    return CS40L25_STATUS_OK;
}

static uint32_t cs40l25_wake(cs40l25_t *driver)
{
    uint32_t i, j;
    uint32_t temp_reg_val = 0;

    // Outer loop for wake-hibernate attempts
    for (i = 0; i < 10; i++)
    {
        // Inner loop for send WAKE command attempts
        for (j = 0; j < 10; j++)
        {
            uint32_t ret;
            // Request Wake
            ret = cs40l25_write_reg(driver,
                                    DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                    CS40L25_POWERCONTROL_WAKEUP);

            // Check for control port write error, indicating possible wake from control port
            // If I2C command failed, then wait 1ms and try again
            if (ret == CS40L25_STATUS_FAIL)
            {
                bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_1MS, NULL, NULL);
            }
            else
            {
                break;
            }
        }

        if (j >= 10)
        {
            return CS40L25_STATUS_FAIL;
        }

        // Increment the inner loop counter to read FW ID
        for (j = 0; j < 10; j++)
        {
            // Wait for at least 5ms
            bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_5MS, NULL, NULL);

            // Read FW ID
            cs40l25_read_reg(driver, CS40L25_FIRMWARE_ID_ADDR, &temp_reg_val);

            // Check if FW ID is correct
            if (temp_reg_val == driver->fw_info->header.fw_id)
            {
                break;
            }
        }

        // If FW ID was incorrect 10 times, force back into hibernate
        if (j >= 10)
        {
            // Request Hibernate manually (not via HALO MBOX)
            cs40l25_write_reg(driver, CS40L25_PWRMGT_CTL_REG, CS40L25_PWRMGT_CTL_MEM_RDY_TRIG_HIBER);
            // Wait for at least 1ms
            bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_1MS, NULL, NULL);
        }
        else
        {
            for (j = 0; j < 10; j++)
            {
                // Read POWERSTATE
                uint32_t reg_address;

                reg_address = cs40l25_find_symbol(driver, CS40L25_SYM_GENERAL_POWERSTATE);
                if (!reg_address)
                {
                    return CS40L25_STATUS_FAIL;
                }

                cs40l25_read_reg(driver, reg_address, &temp_reg_val);
                if ((temp_reg_val == CS40L25_POWERSTATE_ACTIVE) || (temp_reg_val == CS40L25_POWERSTATE_STANDBY))
                {
                    break;
                }
                else if (temp_reg_val == CS40L25_POWERSTATE_HIBERNATE)
                {
                    // Wait for at least 5ms
                    bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_5MS, NULL, NULL);
                }
            }
            if (j >= 10)
            {
                return CS40L25_STATUS_FAIL;
            }
            else
            {
                break;
            }
        }
    }

    return CS40L25_STATUS_OK;
}

/**
 * Event Handler State Machine
 *
 * Implementation of cs40l25_private_functions_t.event
 *
 */
static uint32_t cs40l25_event_handler(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    uint32_t temp_reg_val;
    uint8_t count;
    uint32_t temp_event_control = driver->config_regs.event_control.reg.halo_word;

    /*
     * Since upon entering the Event Handler SM, the BSP Control Port may be in the middle of a transaction,
     * request the BSP to reset the Control Port and abort the current transaction.
     */
    bsp_driver_if_g->i2c_reset(driver->config.bsp_config.bsp_dev_id, NULL);
    driver->event_flags = 0;

    // Read the first IRQ1 flag register
    ret = cs40l25_read_reg(driver, XM_UNPACKED24_DSP1_SCRATCH_REG, &temp_reg_val);

    // If SCRATCH is nonzero OR CP transaction error
    if (temp_reg_val || (ret != CS40L25_STATUS_OK))
    {
        driver->event_flags = CS40L25_EVENT_FLAG_DSP_ERROR;

        return CS40L25_STATUS_OK;
    }

    // Read unmasked event registers
    uint32_t reg_address;

    for (count = 0; count < (sizeof(cs40l2x_event_controls)/sizeof(uint32_t)); count++)
    {
        reg_address = cs40l25_find_symbol(driver, cs40l2x_event_controls[count]);
        if (!reg_address)
        {
            return CS40L25_STATUS_FAIL;
        }

        cs40l25_read_reg(driver, reg_address, &temp_reg_val);

        if ((temp_reg_val == CS40L2X_EVENT_CTRL_NONE) || ((temp_event_control & cs40l2x_event_masks[count]) == 0))
        {
            continue;
        }

        // If event is HW Event, then process separately
        if (temp_reg_val == CS40L2X_EVENT_CTRL_HARDWARE)
        {
            // Read the first IRQ2 flag register
            cs40l25_read_reg(driver, IRQ2_IRQ2_EINT_1_REG, &temp_reg_val);
            // Set flags in event_flags
            for (uint8_t i = 0; i < (CS40L25_EVENT_HW_SOURCES * 2); i += 2)
            {
                if (temp_reg_val & cs40l25_irq2_mask_1_to_event_flag_map[i])
                {
                    driver->event_flags |= cs40l25_irq2_mask_1_to_event_flag_map[i + 1];
                }
            }

            // Clear any IRQ2 flags from first register
            temp_reg_val &= ~CS40L25_INT2_MASK_DEFAULT;
            cs40l25_write_reg(driver, IRQ2_IRQ2_EINT_1_REG, temp_reg_val);

            // If there are Boost-related Errors, proceed to DISABLE_BOOST
            if (driver->event_flags & CS40L25_EVENT_FLAGS_BOOST_CYCLE)
            {
                // Read which MSM Blocks are enabled
                cs40l25_read_reg(driver, MSM_BLOCK_ENABLES_REG, &temp_reg_val);
                // Disable Boost converter
                temp_reg_val &= ~(MSM_BLOCK_ENABLES_BST_EN_BITMASK);
                cs40l25_write_reg(driver, MSM_BLOCK_ENABLES_REG, temp_reg_val);
            }
            // IF there are no Boost-related Errors, proceed to TOGGLE_ERR_RLS
            // Clear the Error Release register
            cs40l25_write_reg(driver, MSM_ERROR_RELEASE_REG, 0);
            // Set the Error Release register
            cs40l25_write_reg(driver, MSM_ERROR_RELEASE_REG, CS40L25_ERR_RLS_SPEAKER_SAFE_MODE_MASK);
            // Clear the Error Release register
            cs40l25_write_reg(driver, MSM_ERROR_RELEASE_REG, 0);

            // If there are Boost-related Errors, re-enable Boost
            if (driver->event_flags & CS40L25_EVENT_FLAGS_BOOST_CYCLE)
            {
                // Read register containing BST_EN
                cs40l25_read_reg(driver, MSM_BLOCK_ENABLES_REG, &temp_reg_val);
                // Re-enable Boost Converter
                temp_reg_val |= MSM_BLOCK_ENABLES_BST_EN_BITMASK;
                cs40l25_write_reg(driver, MSM_BLOCK_ENABLES_REG, temp_reg_val);
            }

        }
        // Else set event flag and clear event source
        else if (temp_reg_val <= CS40L2X_EVENT_CTRL_TRIG_RESM)
        {
            // Set correct bit in flags to send to the BSP Notification Callback
            driver->event_flags |= cs40l25_event_value_to_flag_map[temp_reg_val];
        }
        else
        {
            return CS40L25_STATUS_FAIL;
        }

        // Write EVENT_CTRL_NONE to the triggered event register
        ret = cs40l25_write_reg(driver, reg_address, CS40L2X_EVENT_CTRL_NONE);
    }

    // Write WAKE to POWERCONTROL register
    /*
     * polling for acknowledgment as with other mailbox registers
     * is unnecessary in this case and adds latency, so only send
     * the wake-up command to complete the notification sequence
     */
    cs40l25_write_reg(driver, DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG, CS40L25_POWERCONTROL_WAKEUP);

    return CS40L25_STATUS_OK;
}

static uint32_t cs40l25_cp_bulk_write_block(cs40l25_t *driver, uint32_t addr, uint8_t *bytes, uint32_t length)
{
    uint32_t ret = CS40L25_STATUS_OK;
    uint32_t bsp_status;
    cs40l25_bsp_config_t *b = &(driver->config.bsp_config);

    /*
     * Switch from Little-Endian contents of uint32_t 'addr' to Big-Endian format required for Control Port
     * transaction.
     *
     * FIXME: This is not platform independent.
     */
    b->cp_write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    b->cp_write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    b->cp_write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    b->cp_write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    bsp_status = bsp_driver_if_g->i2c_db_write(b->bsp_dev_id,
                                               b->cp_write_buffer,
                                               4,
                                               bytes,
                                               length,
                                               NULL,
                                               NULL);

    if (bsp_status == BSP_STATUS_FAIL)
    {
        ret = CS40L25_STATUS_FAIL;
    }

    return ret;
}

static uint32_t cs40l25_field_access_get(cs40l25_t *driver)
{
    uint32_t temp_reg_val = 0;
    uint32_t reg_address;

    if (driver->field_accessor.symbol)
    {
        reg_address = cs40l25_find_symbol(driver, driver->field_accessor.id);
        if (!reg_address)
        {
            return CS40L25_STATUS_FAIL;
        }

        reg_address += (driver->field_accessor.address * 4);
    }
    else
    {
        reg_address = driver->field_accessor.address;
    }

    // Read the value from the field address
    cs40l25_read_reg(driver, reg_address, &temp_reg_val);

    // Create bit-wise mask of the bit-field
    uint32_t temp_mask = (~(0xFFFFFFFF << driver->field_accessor.size) << driver->field_accessor.shift);
    uint32_t reg_val = temp_reg_val;

    uint32_t *reg_ptr = (uint32_t *) driver->current_request.arg;

    // Mask off bit-field and shift down to LS-Bit
    reg_val &= temp_mask;
    reg_val >>= driver->field_accessor.shift;
    *reg_ptr = reg_val;

    return CS40L25_STATUS_OK;
}
static uint32_t cs40l25_field_access_set(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    uint32_t temp_reg_val = 0;
    uint32_t reg_address;

    if (driver->field_accessor.symbol)
    {
        reg_address = cs40l25_find_symbol(driver, driver->field_accessor.id);
        if (!reg_address)
        {
            return CS40L25_STATUS_FAIL;
        }

        reg_address += (driver->field_accessor.address * 4);
    }
    else
    {
        reg_address = driver->field_accessor.address;
    }

    // Read the value from the field address
    cs40l25_read_reg(driver, reg_address, &temp_reg_val);
    // Create bit-wise mask of the bit-field
    uint32_t temp_mask = (~(0xFFFFFFFF << driver->field_accessor.size) << driver->field_accessor.shift);
    uint32_t reg_val = temp_reg_val;

    uint32_t field_val = (uint32_t) driver->current_request.arg;
    // Shift new value to bit-field bit position
    field_val <<= driver->field_accessor.shift;
    field_val &= temp_mask;
    // Mask off bit-field bit locations in memory's value
    reg_val &= ~temp_mask;
    // Add new value
    reg_val |= field_val;

    // Write new register/memory value
    cs40l25_write_reg(driver, reg_address, reg_val);

    if (driver->field_accessor.ack_ctrl)
    {
        for (uint32_t count = 0; count < CS40L25_POLL_ACK_CTRL_MAX; count++)
        {
            bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS, NULL, NULL);

            // Read the value from the field address
            cs40l25_read_reg(driver, reg_address, &temp_reg_val);
            if (temp_reg_val == driver->field_accessor.ack_reset)
            {
                break;
            }
        }

        // Fail if register never reset to ACK value
        if (temp_reg_val != driver->field_accessor.ack_reset)
        {
            ret = CS40L25_STATUS_FAIL;
        }
    }

    return ret;
}

const cs40l25_field_accessor_t fa_list[] =
{
    {
        .address = CS40L25_INTP_AMP_CTRL_REG,
        .shift = CS40L25_INTP_AMP_CTRL_AMP_VOL_PCM_BITOFFSET,
        .size = CS40L25_INTP_AMP_CTRL_AMP_VOL_PCM_BITWIDTH,
        .ack_ctrl = false,
    },
    {
        .address = DSP_BHM_HALO_HEARTBEAT_REG,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_HALO_HEARTBEAT,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .address = DSP_BHM_BUZZ_TRIGGER_REG,
        .shift = 0,
        .size = 32,
        .ack_ctrl = true,
        .ack_reset = 0x0,
    },
    {
        .address = DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG,
        .shift = 0,
        .size = 32,
        .ack_ctrl = true,
        .ack_reset = 0xFFFFFFFF,
    },
    {
        .address = DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_2_REG,
        .shift = 0,
        .size = 32,
        .ack_ctrl = true,
        .ack_reset = 0xFFFFFFFF,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_VIBEGEN_TIMEOUT_MS,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_GPIO_ENABLE,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_GPIO_BUTTONDETECT,
        .shift = 0,
        .size = 1,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_GPIO_BUTTONDETECT,
        .shift = 1,
        .size = 1,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_GPIO_BUTTONDETECT,
        .shift = 2,
        .size = 1,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_GPIO_BUTTONDETECT,
        .shift = 3,
        .size = 1,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_CLAB_CLAB_ENABLED,
        .shift = 0,
        .size = 1,
        .ack_ctrl = false,
    },
    {
            .symbol = true,
        .id = CS40L25_SYM_GENERAL_GAIN_CONTROL,
        .shift = 14,
        .size = 10,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_GAIN_CONTROL,
        .shift = 4,
        .size = 10,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_INDEXBUTTONPRESS,
        .address = 0,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
            .symbol = true,
        .id = CS40L25_SYM_GENERAL_INDEXBUTTONPRESS,
        .address = 4,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_INDEXBUTTONPRESS,
        .address = 8,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_INDEXBUTTONPRESS,
        .address = 12,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
            .symbol = true,
        .id = CS40L25_SYM_GENERAL_INDEXBUTTONRELEASE,
        .address = 0,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_INDEXBUTTONRELEASE,
        .address = 4,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_INDEXBUTTONRELEASE,
        .address = 8,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .symbol = true,
        .id = CS40L25_SYM_GENERAL_INDEXBUTTONRELEASE,
        .address = 12,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
    {
        .address = CS40L25_FIRMWARE_REVISION,
        .shift = 0,
        .size = 32,
        .ack_ctrl = false,
    },
};

/**
 * Load new Control Request to be processed
 *
 * Implementation of cs40l25_private_functions_t.load_control
 *
 */
static uint32_t cs40l25_load_control(cs40l25_t *driver)
{
    uint32_t id = driver->current_request.id;
    cs40l25_sm_fp_t fp = NULL;

    fp = NULL;

    if (id == CS40L25_CONTROL_ID_GET_DSP_STATUS)
    {
       fp = (cs40l25_sm_fp_t) &cs40l25_get_dsp_status;
    }
    else
    {
        if (id == CS40L25_CONTROL_ID_GET_HALO_HEARTBEAT)
        {
            if (driver->state == CS40L25_STATE_POWER_UP)
            {
                id = CS40L25_CONTROL_ID_GET_BHM_HALO_HEARTBEAT;
            }
            else if ((driver->state == CS40L25_STATE_CAL_POWER_UP) || (driver->state == CS40L25_STATE_DSP_POWER_UP))
            {
                id = CS40L25_CONTROL_ID_GET_RAM_HALO_HEARTBEAT;
            }
        }

        if (CS40L25_CONTROL_ID_GET_CONTROL(id) <= CS40L25_CONTROL_ID_FA_MAX)
        {
            driver->field_accessor = fa_list[CS40L25_CONTROL_ID_GET_CONTROL(id)];
            if (CS40L25_CONTROL_ID_GET_HANDLER(id) == CS40L25_CONTROL_ID_HANDLER_FA_GET)
            {
                fp = (cs40l25_sm_fp_t) &cs40l25_field_access_get;
            }
            else
            {
                fp = (cs40l25_sm_fp_t) &cs40l25_field_access_set;
            }
        }
    }

    driver->control_sm.fp = fp;
    if (fp == NULL)
    {
        return CS40L25_STATUS_FAIL;
    }
    else
    {
        return CS40L25_STATUS_OK;
    }
}

static bool cs40l25_is_mixer_source_used(cs40l25_t *driver, uint8_t source)
{
    const syscfg_reg_t *regs = driver->config.syscfg_regs;

    if ((regs[CS40L25_MIXER_DACPCM1_INPUT_SYSCFG_REGS_INDEX].value == source) || \
        (regs[CS40L25_MIXER_DSP1RX1_INPUT_SYSCFG_REGS_INDEX].value == source) || \
        (regs[CS40L25_MIXER_DSP1RX2_INPUT_SYSCFG_REGS_INDEX].value == source) || \
        (regs[CS40L25_MIXER_DSP1RX3_INPUT_SYSCFG_REGS_INDEX].value == source) || \
        (regs[CS40L25_MIXER_DSP1RX4_INPUT_SYSCFG_REGS_INDEX].value == source))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize driver state/handle
 *
 * Implementation of cs40l25_functions_t.initialize
 *
 */
uint32_t cs40l25_initialize(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_FAIL;

    if (NULL != driver)
    {
        /*
         * The memset() call sets all members to 0, including the following semantics:
         * - 'state' is set to UNCONFIGURED
         */
        memset(driver, 0, sizeof(cs40l25_t));

        ret = CS40L25_STATUS_OK;
    }

    return ret;
}

/**
 * Configures driver state/handle
 *
 * Implementation of cs40l25_functions_t.configure
 *
 */
uint32_t cs40l25_configure(cs40l25_t *driver, cs40l25_config_t *config)
{
    uint32_t ret = CS40L25_STATUS_FAIL;

    if ((NULL != driver) && \
        (NULL != config) && \
        (NULL != config->bsp_config.cp_write_buffer) && \
        (NULL != config->bsp_config.cp_read_buffer))
    {
        driver->config = *config;

        driver->config_regs.event_control = config->event_control;

        ret = bsp_driver_if_g->register_gpio_cb(driver->config.bsp_config.bsp_int_gpio_id,
                                                &cs40l25_irq_callback,
                                                driver);

        cs40l25_dsp_config_controls_t *dsp_ctrls = &(config->dsp_config_ctrls);
        driver->config_regs.dsp_gpio_button_detect.gpio1_enable = (dsp_ctrls->dsp_gpio1_button_detect_enable ? 1 : 0);
        driver->config_regs.dsp_gpio_button_detect.gpio2_enable = (dsp_ctrls->dsp_gpio2_button_detect_enable ? 1 : 0);
        driver->config_regs.dsp_gpio_button_detect.gpio3_enable = (dsp_ctrls->dsp_gpio3_button_detect_enable ? 1 : 0);
        driver->config_regs.dsp_gpio_button_detect.gpio4_enable = (dsp_ctrls->dsp_gpio4_button_detect_enable ? 1 : 0);
        driver->config_regs.dsp_gpio_enable.halo_word = (dsp_ctrls->dsp_gpio_enable ? 1 : 0);
        driver->config_regs.dsp_gain_control.gpi_gain = dsp_ctrls->dsp_gpi_gain_control;
        driver->config_regs.dsp_gain_control.control_gain = dsp_ctrls->dsp_ctrl_gain_control;
        driver->config_regs.dsp_gpio1_index_button_press.halo_word = dsp_ctrls->dsp_gpio1_index_button_press;
        driver->config_regs.dsp_gpio2_index_button_press.halo_word = dsp_ctrls->dsp_gpio2_index_button_press;
        driver->config_regs.dsp_gpio3_index_button_press.halo_word = dsp_ctrls->dsp_gpio3_index_button_press;
        driver->config_regs.dsp_gpio4_index_button_press.halo_word = dsp_ctrls->dsp_gpio4_index_button_press;
        driver->config_regs.dsp_gpio1_index_button_release.halo_word = dsp_ctrls->dsp_gpio1_index_button_release;
        driver->config_regs.dsp_gpio2_index_button_release.halo_word = dsp_ctrls->dsp_gpio2_index_button_release;
        driver->config_regs.dsp_gpio3_index_button_release.halo_word = dsp_ctrls->dsp_gpio3_index_button_release;
        driver->config_regs.dsp_gpio4_index_button_release.halo_word = dsp_ctrls->dsp_gpio4_index_button_release;
        driver->config_regs.clab_enabled.halo_word = (dsp_ctrls->clab_enable ? 1 : 0);
        driver->config_regs.peak_amplitude_control.halo_word =  dsp_ctrls->peak_amplitude;

        if (ret == BSP_STATUS_OK)
        {
            ret = CS40L25_STATUS_OK;
        }

        // Advance driver to CONFIGURED state
        driver->state = CS40L25_STATE_CONFIGURED;
    }

    return ret;
}

/**
 * Processes driver state machines
 *
 * Implementation of cs40l25_functions_t.process
 *
 */
uint32_t cs40l25_process(cs40l25_t *driver)
{
    // check for driver state
    if ((driver->state != CS40L25_STATE_UNCONFIGURED) && (driver->state != CS40L25_STATE_ERROR))
    {
        // check for driver mode
        if (driver->mode == CS40L25_MODE_HANDLING_EVENTS)
        {
            // Check for valid state to process events
            if ((driver->state == CS40L25_STATE_DSP_STANDBY) ||
                (driver->state == CS40L25_STATE_DSP_POWER_UP) ||
                (driver->state == CS40L25_STATE_HIBERNATE))
            {
                // run through event handler
                if (CS40L25_STATUS_OK == cs40l25_event_handler(driver))
                {
                    driver->mode = CS40L25_MODE_HANDLING_CONTROLS;
                }
                else
                {
                    driver->state = CS40L25_STATE_ERROR;
                }
            }
            // If in invalid state for handling events (i.e. BHM, Calibration), simply switch back to Handling Controls
            else
            {
                driver->mode = CS40L25_MODE_HANDLING_CONTROLS;
            }
        }

        if (driver->state == CS40L25_STATE_ERROR)
        {
            driver->event_flags |= CS40L25_EVENT_FLAG_STATE_ERROR;
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
    }

    return CS40L25_STATUS_OK;
}

/**
 * Submit a Control Request to the driver
 *
 * Implementation of cs40l25_functions_t.control
 *
 */
uint32_t cs40l25_control(cs40l25_t *driver, cs40l25_control_request_t req)
{
    uint32_t ret;

    // FIXME:  Check that control is valid

    // Load control
    driver->current_request = req;
    cs40l25_load_control(driver);

    // Execute control request
    ret = driver->control_sm.fp(driver);

    return ret;
}

/**
 * Reset the CS40L25
 *
 * Implementation of cs40l25_functions_t.reset
 *
 */
uint32_t cs40l25_reset(cs40l25_t *driver)
{
    uint32_t count;
    uint32_t temp_reg_val;

    if ((driver->state == CS40L25_STATE_UNCONFIGURED) || (driver->state == CS40L25_STATE_ERROR))
    {
        return CS40L25_STATUS_FAIL;
    }

    // Drive RESET low for at least T_RLPW (1ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.bsp_reset_gpio_id, BSP_GPIO_LOW);
    bsp_driver_if_g->set_timer(CS40L25_T_RLPW_MS, NULL, NULL);

    // Drive RESET high and wait for at least T_IRS (1ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.bsp_reset_gpio_id, BSP_GPIO_HIGH);
    bsp_driver_if_g->set_timer(CS40L25_T_IRS_MS, NULL, NULL);

    // Start polling OTP_BOOT_DONE bit every 10ms
    count = 0;
    do
    {
        cs40l25_read_reg(driver, IRQ1_IRQ1_EINT_4_REG,  &temp_reg_val);
        if (temp_reg_val & IRQ1_IRQ1_EINT_4_BOOT_DONE_BITMASK)
            break;
        else if (count < CS40L25_POLL_OTP_BOOT_DONE_MAX)
        {
            bsp_driver_if_g->set_timer(CS40L25_POLL_OTP_BOOT_DONE_MS, NULL, NULL);
            count++;
        }
        else
        {
            return CS40L25_STATUS_FAIL;
        }
    }
    while (!(temp_reg_val & IRQ1_IRQ1_EINT_4_BOOT_DONE_BITMASK));

    // Read OTP_BOOT_ERR
    cs40l25_read_reg(driver, IRQ1_IRQ1_EINT_3_REG, &temp_reg_val);
    if (temp_reg_val & IRQ1_IRQ1_EINT_3_OTP_BOOT_ERR_BITMASK)
    {
        return CS40L25_STATUS_FAIL;
    }

    // Read DEVID
    cs40l25_read_reg(driver, CS40L25_SW_RESET_DEVID_REG, &temp_reg_val);
    driver->devid = temp_reg_val;
    // Read REVID
    cs40l25_read_reg(driver, CS40L25_SW_RESET_REVID_REG, &temp_reg_val);
    driver->revid = temp_reg_val;

    // Start polling BHM_AMP_STATUS_BOOT_DONE bit every 10ms
    count = 0;
    do
    {
        cs40l25_read_reg(driver, DSP_BHM_AMP_STATUS_REG, &temp_reg_val);
        if (temp_reg_val & DSP_BHM_AMP_STATUS_BOOT_DONE_BITMASK)
            break;
        else if (count < CS40L25_POLL_OTP_BOOT_DONE_MAX)
        {
            bsp_driver_if_g->set_timer(CS40L25_POLL_OTP_BOOT_DONE_MS, NULL, NULL);
            count++;
        }
        else
        {
            return CS40L25_STATUS_FAIL;
        }
    }
    while (!(temp_reg_val & DSP_BHM_AMP_STATUS_BOOT_DONE_BITMASK));

    driver->state = CS40L25_STATE_POWER_UP;

    return CS40L25_STATUS_OK;
}

/**
 * Write fw data to the CS40L25
 *
 */
uint32_t cs40l25_write_block(cs40l25_t *driver, uint32_t addr, uint8_t *data, uint32_t size)
{
    if (addr == 0 || data == NULL || size == 0 || size % 4 != 0)
    {
        return CS40L25_STATUS_FAIL;
    }

    return cs40l25_cp_bulk_write_block(driver, addr, data, size);
}

/**
 * Boot the CS40L25
 *
 * Implementation of cs40l25_functions_t.boot
 *
 */
uint32_t cs40l25_boot(cs40l25_t *driver, fw_img_v1_info_t *fw_info)
{
    uint32_t count = 0;
    bool is_cal_boot = false;
    uint32_t reg_address;

    driver->fw_info = fw_info;

    if (driver->fw_info == NULL)
    {
        return CS40L25_STATUS_OK;
    }

    if (driver->fw_info->header.fw_id == CS40L25_FWID_CAL)
    {
        is_cal_boot = true;
    }

    if (!is_cal_boot)
    {
        //Ignore first word of errata_patch
        int errata_entries = (sizeof(cs40l25_revb0_errata_patch) - sizeof(uint32_t)) / (2 * sizeof(uint32_t));
        int wseq_entries = sizeof(cs40l25_wseq_regs) / (2 * sizeof(uint32_t));

        driver->wseq_num_entries = 0;
        cs40l25_wseq_table_add(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_1);
        cs40l25_wseq_table_add(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_2);
        cs40l25_wseq_add_block(driver, (uint32_t *) (cs40l25_revb0_errata_patch + 1), errata_entries);
        cs40l25_wseq_add_block(driver, (uint32_t *) cs40l25_wseq_regs, wseq_entries);
        cs40l25_wseq_table_add(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_1);
        cs40l25_wseq_table_add(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_2);
        driver->wseq_initialized = true;
    }

    // Write next post-boot configuration
    for (count = 0; count < (sizeof(cs40l25_post_boot_config)/(sizeof(uint32_t) * 2)); count++)
    {
        cs40l25_write_reg(driver,
                          cs40l25_post_boot_config[count * 2],
                          cs40l25_post_boot_config[(count * 2) + 1]);
    }

    // Apply Calibration data
    if (!is_cal_boot)
    {
        uint32_t reg_address;

        if (driver->config.cal_data.is_valid_f0)
        {
            reg_address = cs40l25_find_symbol(driver, CS40L25_SYM_GENERAL_F0_STORED);
            if (!reg_address)
            {
                return CS40L25_STATUS_FAIL;
            }
            cs40l25_write_reg(driver, reg_address, driver->config.cal_data.f0);

            reg_address = cs40l25_find_symbol(driver, CS40L25_SYM_GENERAL_REDC_STORED);
            if (!reg_address)
            {
                return CS40L25_STATUS_FAIL;
            }
            cs40l25_write_reg(driver, reg_address, driver->config.cal_data.redc);
        }

        if (driver->config.cal_data.is_valid_qest)
        {
            reg_address = cs40l25_find_symbol(driver, CS40L25_SYM_GENERAL_Q_STORED);
            if (!reg_address)
            {
                return CS40L25_STATUS_FAIL;
            }
            cs40l25_write_reg(driver, reg_address, driver->config.cal_data.qest);
        }

        driver->state = CS40L25_STATE_DSP_STANDBY;
    }
    else
    {
        driver->state = CS40L25_STATE_CAL_STANDBY;
    }

    // Write configuration data
    // Unlock the register file
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_1);
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_2);

    for (count = 0; count < driver->config.syscfg_regs_total; count++)
    {
        uint32_t temp_reg_val;

        cs40l25_read_reg(driver, driver->config.syscfg_regs[count].address, &temp_reg_val);
        temp_reg_val &= ~(driver->config.syscfg_regs[count].mask);
        temp_reg_val |= driver->config.syscfg_regs[count].value;
        cs40l25_write_reg(driver, driver->config.syscfg_regs[count].address, temp_reg_val);
    }

    // Write HALO configuration data
    if (driver->state == CS40L25_STATE_DSP_STANDBY)
    {
        uint32_t previous_reg_address;
        uint8_t address_offset;

        previous_reg_address = 0;
        reg_address = 0;
        address_offset = 0;
        for (count = 0; count < CS40L25_CONFIG_REGISTERS_TOTAL; count++)
        {
            reg_address = cs40l25_find_symbol(driver, cs40l25_config_register_symbols[count]);
            if (!reg_address)
            {
                return CS40L25_STATUS_FAIL;
            }

            if (reg_address == previous_reg_address)
            {
                address_offset += 4;
            }
            else
            {
                previous_reg_address = reg_address;
                address_offset = 0;
            }

            cs40l25_write_reg(driver, (reg_address + address_offset), driver->config_regs.words[count]);
        }
    }

    // Apply IRQMASKSEQ Patch set
    reg_address = cs40l25_find_symbol(driver, CS40L25_SYM_GENERAL_IRQMASKSEQUENCE);
    if (!reg_address)
    {
        return CS40L25_STATUS_FAIL;
    }

    for (count = 0; count < (sizeof(cs40l25_irqmaskseq_patch) / sizeof(uint32_t)); count++)
    {
        cs40l25_write_reg(driver, (reg_address + (count * 4)), cs40l25_irqmaskseq_patch[count]);
    }

    reg_address = cs40l25_find_symbol(driver, CS40L25_SYM_GENERAL_IRQMASKSEQUENCE_VALID);
    if (!reg_address)
    {
        return CS40L25_STATUS_FAIL;
    }
    cs40l25_write_reg(driver, reg_address, 0x1);

    // Lock the register file
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_1);
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_2);

    return CS40L25_STATUS_OK;
}

/**
 * Change the power state
 *
 * Implementation of cs40l25_functions_t.power
 *
 */
uint32_t cs40l25_power(cs40l25_t *driver, uint32_t power_state)
{
    // Submit the correct request based on power_state
    switch (power_state)
    {
        case CS40L25_POWER_UP:
            // POWER_UP Control Requests are only valid for STANDBY and DSP_STANDBY states
            if ((driver->state == CS40L25_STATE_STANDBY) ||
                (driver->state == CS40L25_STATE_DSP_STANDBY) ||
                (driver->state == CS40L25_STATE_CAL_STANDBY))
            {
                cs40l25_power_up(driver);
                if (driver->state == CS40L25_STATE_STANDBY)
                {
                    driver->state = CS40L25_STATE_POWER_UP;
                }
                else if (driver->state == CS40L25_STATE_DSP_STANDBY)
                {
                    driver->state = CS40L25_STATE_DSP_POWER_UP;
                }
                else if (driver->state == CS40L25_STATE_CAL_STANDBY)
                {
                    driver->state = CS40L25_STATE_CAL_POWER_UP;
                }
            }

            break;

        case CS40L25_POWER_DOWN:
            if (driver->state == CS40L25_STATE_POWER_UP)
            {
                cs40l25_exit_bhm(driver);
                driver->state = CS40L25_STATE_STANDBY;
            }
            else if (driver->state == CS40L25_STATE_DSP_POWER_UP)
            {
                cs40l25_power_down(driver);
                driver->state = CS40L25_STATE_DSP_STANDBY;
            }
            else if (driver->state == CS40L25_STATE_CAL_POWER_UP)
            {
                cs40l25_power_down(driver);
                driver->state = CS40L25_STATE_CAL_STANDBY;
            }
            break;

        case CS40L25_POWER_HIBERNATE:
            if (driver->state == CS40L25_STATE_DSP_POWER_UP)
            {
                cs40l25_hibernate(driver);
                driver->state = CS40L25_STATE_HIBERNATE;
            }
            break;

        case CS40L25_POWER_WAKE:
            if (driver->state == CS40L25_STATE_HIBERNATE)
            {
                cs40l25_wake(driver);
                driver->state = CS40L25_STATE_DSP_POWER_UP;
            }
            break;

        default:
            break;
    }

    return CS40L25_STATUS_OK;
}

/**
 * Calibrate the HALO DSP Protection Algorithm
 *
 * Implementation of cs40l25_functions_t.calibrate
 *
 */
uint32_t cs40l25_calibrate(cs40l25_t *driver, uint32_t calib_type)
{
    uint32_t temp_reg_val = 0;

    if (!(calib_type & CS40L25_CALIB_ALL) || (driver->state != CS40L25_STATE_CAL_POWER_UP))
    {
        return CS40L25_STATUS_FAIL;
    }

    // Get all control addresses needed - just re-use the array for the addresses
    uint32_t ctrl_addresses[] =
    {
        CS40L25_SYM_F0_TRACKING_MAXBACKEMF,
        CS40L25_SYM_F0_TRACKING_CLOSED_LOOP,
        CS40L25_SYM_F0_TRACKING_F0_TRACKING_ENABLE,
        CS40L25_SYM_F0_TRACKING_F0,
        CS40L25_SYM_F0_TRACKING_REDC,
        CS40L25_SYM_Q_ESTIMATION_Q_EST
    };

    for (uint8_t i = 0; i < (sizeof(ctrl_addresses) / sizeof(uint32_t)); i++)
    {
        ctrl_addresses[i] = cs40l25_find_symbol(driver, ctrl_addresses[i]);
        if (!ctrl_addresses[i])
        {
            return CS40L25_STATUS_FAIL;
        }
    }

    driver->config.cal_data.is_valid_f0 = false;
    driver->config.cal_data.is_valid_qest = false;

    // Read current volume
    cs40l25_read_reg(driver, CS40L25_INTP_AMP_CTRL_REG, &temp_reg_val);
    uint32_t temp_mask = (~(0xFFFFFFFF << CS40L25_INTP_AMP_CTRL_AMP_VOL_PCM_BITWIDTH) << CS40L25_INTP_AMP_CTRL_AMP_VOL_PCM_BITOFFSET);

    // Save volume level
    driver->calib_pcm_vol = temp_reg_val;
    cs40l25_write_reg(driver, CS40L25_INTP_AMP_CTRL_REG, temp_reg_val & ~temp_mask);

    if (calib_type & CS40L25_CALIB_F0)
    {
        cs40l25_write_reg(driver, ctrl_addresses[0], 0); // MAXBACKEMF
        cs40l25_write_reg(driver, ctrl_addresses[1], 0); // CLOSED_LOOP
        cs40l25_write_reg(driver, ctrl_addresses[2], 1); // F0_TRACKING_ENABLE

        // Wait 500ms
        bsp_driver_if_g->set_timer(500, NULL, NULL);

        cs40l25_write_reg(driver, ctrl_addresses[1], 1); // CLOSED_LOOP

        // Wait 2s
        bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2S, NULL, NULL);

        cs40l25_write_reg(driver, ctrl_addresses[2], 0); // F0_TRACKING_ENABLE
        cs40l25_read_reg(driver, ctrl_addresses[3], &(driver->config.cal_data.f0)); // F0
        cs40l25_read_reg(driver, ctrl_addresses[4], &(driver->config.cal_data.redc)); // REDC
        cs40l25_read_reg(driver, ctrl_addresses[0], &(driver->config.cal_data.backemf)); // MAXBACKEMF
        driver->config.cal_data.is_valid_f0 = true;
    }

    if (calib_type & CS40L25_CALIB_QEST)
    {
        cs40l25_write_reg(driver, ctrl_addresses[2], 2); // F0_TRACKING_ENABLE

        uint32_t count;
        for (count = 0; count < CS40L25_CAL_Q_POLL_COUNT; count++)
        {
            bsp_driver_if_g->set_timer(100, NULL, NULL);

            cs40l25_read_reg(driver, ctrl_addresses[2], &temp_reg_val); // F0_TRACKING_ENABLE

            if (temp_reg_val == 0)
            {
                break;
            }
        }

        if (count >= CS40L25_CAL_Q_POLL_COUNT)
        {
            return CS40L25_STATUS_FAIL;
        }

        cs40l25_read_reg(driver, ctrl_addresses[5], &(driver->config.cal_data.qest)); // Q_EST
        driver->config.cal_data.is_valid_qest = true;
    }

    cs40l25_write_reg(driver, CS40L25_INTP_AMP_CTRL_REG, driver->calib_pcm_vol);

    return CS40L25_STATUS_OK;
}

/**
 * Put DSP into I2S Streaming Mode
 *
 * Implementaion of cs40l25_functions_t.start_i2s
 */
uint32_t cs40l25_start_i2s(cs40l25_t *driver)
{
    uint32_t ret;

    // Enable ASP
    cs40l25_dataif_asp_enables1_t asp_reg_val;
    cs40l25_read_reg(driver, DATAIF_ASP_ENABLES1_REG, &(asp_reg_val.word));

    if (cs40l25_is_mixer_source_used(driver, CS40L25_INPUT_SRC_ASPRX1))
    {
        asp_reg_val.asp_rx1_en = 1;
    }

    if (cs40l25_is_mixer_source_used(driver, CS40L25_INPUT_SRC_ASPRX2))
    {
        asp_reg_val.asp_rx2_en = 1;
    }

    cs40l25_write_reg(driver, DATAIF_ASP_ENABLES1_REG, asp_reg_val.word);

    // Force DSP into standby
    ret = cs40l25_write_acked_reg(driver,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_FORCE_STANDBY,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_NONE);

    if (ret != CS40L25_STATUS_OK)
    {
        return CS40L25_STATUS_FAIL;
    }

    // Change clock to bclk
    cs40l25_ccm_refclk_input_t clk_reg_val;

    cs40l25_read_reg(driver, CCM_REFCLK_INPUT_REG, &(clk_reg_val.word));

    clk_reg_val.pll_refclk_sel = CS40L25_PLL_REFLCLK_SEL_BCLK;
    clk_reg_val.pll_refclk_freq = CS40L25_SCLK_BASED_PLL_REFCLK_CODE;

#ifdef CS40L25_IS_OPEN_LOOP
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_1);
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_2);
    cs40l25_write_reg(driver, 0x2D20, 0x0);
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_1);
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_2);
    cs40l25_write_reg(driver, 0x3018, 0x0);
#endif

    cs40l25_write_reg(driver, CCM_REFCLK_INPUT_REG, clk_reg_val.word);

    //Wake the firmware
    ret = cs40l25_write_acked_reg(driver,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_WAKEUP,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_NONE);

    if (ret != CS40L25_STATUS_OK)
    {
        return CS40L25_STATUS_FAIL;
    }

    //Enable I2S
    return cs40l25_write_acked_reg(driver,
                                   DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_5_REG,
                                   DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_5_START_I2S,
                                   DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_5_NONE);
}

/**
 * Pull DSP out of I2S Streaming Mode
 *
 * Implementation of cs40l25_fuctions_t.stop_i2s
 *
 */
uint32_t cs40l25_stop_i2s(cs40l25_t *driver)
{
    uint32_t ret;

    ret = cs40l25_write_acked_reg(driver,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_FORCE_STANDBY,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_5_NONE);

    if (ret != CS40L25_STATUS_OK)
    {
        return CS40L25_STATUS_FAIL;
    }

    // Change clock back to MCLK
    cs40l25_ccm_refclk_input_t clk_reg_val, clk_reg_val_cfg;

    ret = cs40l25_read_reg(driver, CCM_REFCLK_INPUT_REG, &(clk_reg_val.word));
    if (ret != CS40L25_STATUS_OK)
    {
        return CS40L25_STATUS_FAIL;
    }

    clk_reg_val_cfg.word = driver->config.syscfg_regs[CS40L25_CCM_REFCLK_INPUT_SYSCFG_REGS_INDEX].value;
    clk_reg_val.pll_refclk_sel = clk_reg_val_cfg.pll_refclk_sel;
    clk_reg_val.pll_refclk_freq = clk_reg_val_cfg.pll_refclk_freq;

    ret = cs40l25_write_reg(driver, CCM_REFCLK_INPUT_REG, clk_reg_val.word);
    if (ret != CS40L25_STATUS_OK)
    {
        return CS40L25_STATUS_FAIL;
    }

#ifdef CS40L25_IS_OPEN_LOOP
    cs40l25_write_reg(driver, 0x3018, 0x02000000);
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_1);
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_2);
    cs40l25_write_reg(driver, 0x2D20, 0x00000030);
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_1);
    cs40l25_write_reg(driver, CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_2);
#endif

    //Wake the firmware
    ret = cs40l25_write_acked_reg(driver,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_WAKEUP,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_NONE);

    if (ret != CS40L25_STATUS_OK)
    {
        return CS40L25_STATUS_FAIL;
    }

    ret = cs40l25_write_acked_reg(driver,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_5_REG,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_5_STOP_I2S,
                                  DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_5_NONE);

    if (ret != CS40L25_STATUS_OK)
    {
        return CS40L25_STATUS_FAIL;
    }

    //Disable ASP
    cs40l25_dataif_asp_enables1_t asp_reg_val;
    cs40l25_read_reg(driver, DATAIF_ASP_ENABLES1_REG, &(asp_reg_val.word));
    asp_reg_val.asp_rx1_en = 0;
    asp_reg_val.asp_rx2_en = 0;
    cs40l25_write_reg(driver, DATAIF_ASP_ENABLES1_REG, asp_reg_val.word);

    return CS40L25_STATUS_OK;
}

/*!
 * \mainpage Introduction
 *
 * This document outlines the driver source code included in the MCU Driver Software Package for the CS40L25 Boosted
 * Haptics Driver.  This guide is primarily intended for those involved in end-system implementation, integration, and
 * testing, who will use the CS40L25 MCU Driver Software Package to integrate the CS40L25 driver source code into the
 * end-system's host MCU software.  After reviewing this guide, the reader will be able to begin software integration
 * of the CS40L25 MCU driver and then have the ability to initialize, reset, boot, configure, and service events from
 * the CS40L25.  This guide should be used along with the CS40L25 Datasheet.
 *
 *  In order to obtain any additional materials, and for any questions regarding this guide, the MCU Driver
 *  Software Package, or CS40L25 system integration, please contact your Cirrus Logic Representative.
 */
