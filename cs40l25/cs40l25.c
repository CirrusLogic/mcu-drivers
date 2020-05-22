/**
 * @file cs40l25.c
 *
 * @brief The CS40L25 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019, 2020 All Rights Reserved, http://www.cirrus.com/
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

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
//#define TO_FIX_IN_PORTING
//#define I2S_CONFIG_SHORTCUT

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
#define CS40L25_INT2_MASK_DEFAULT   (0x7FFD7E3F)
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
 * Beta value used to calculate value for CCM_FS_MON_0_REG
 *
 * @see Datasheet Section 4.13.9
 *
 */
#define CS40L25_FS_MON0_BETA                    (6000000)

/**
 * Value of CS40L25_CAL_STATUS that indicates Calibration success
 *
 * @see CS40L25_CAL_STATUS
 *
 */
#define CS40L25_CAL_STATUS_CALIB_SUCCESS        (0x1)

/**
 * Total number of HALO FW controls to cache before CS40L25 Power Up
 *
 * Currently, there are no HALO FW controls that are cached in the driver.
 *
 * @see cs40l25_power_up_sm
 *
 */
#define CS40L25_SYNC_CTRLS_TOTAL (0)

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

/**
 * @defgroup CS40L25_POWERSTATE_
 * @brief Valid values read from the POWERSTATE HALO FW control
 *
 * @{
 */
#define CS40L25_POWERSTATE_BLANK                (0)
#define CS40L25_POWERSTATE_ACTIVE               (1)
#define CS40L25_POWERSTATE_STANDBY              (2)
#define CS40L25_POWERSTATE_HIBERNATE            (3)
/** @} */

/**
 * HALO FW control fixed address for the FW ID
 */
#define CS40L25_FIRMWARE_ID_ADDR                (0x0280000C)

/**
 * @defgroup CS40L2X_EVENT_
 * @brief Bit masks for enables in the EVENTCONTROL HALO FW control
 *
 * @{
 */
#define CS40L2X_EVENT_DISABLED                  0x000000
#define CS40L2X_EVENT_GPIO1_ENABLED             0x000001
#define CS40L2X_EVENT_GPIO2_ENABLED             0x000002
#define CS40L2X_EVENT_GPIO3_ENABLED             0x000004
#define CS40L2X_EVENT_GPIO4_ENABLED             0x000008
#define CS40L2X_EVENT_START_ENABLED             0x000010
#define CS40L2X_EVENT_END_ENABLED               0x000020
#define CS40L2X_EVENT_READY_ENABLED             0x000040
#define CS40L2X_EVENT_HARDWARE_ENABLED          0x800000
/** @} */

/**
 * @defgroup CS40L2X_EVENT_CTRL_
 * @brief Possible values for the
 *
 * @{
 */
#define CS40L2X_EVENT_CTRL_GPIO1_FALL           0
#define CS40L2X_EVENT_CTRL_GPIO1_RISE           1
#define CS40L2X_EVENT_CTRL_GPIO2_FALL           2
#define CS40L2X_EVENT_CTRL_GPIO2_RISE           3
#define CS40L2X_EVENT_CTRL_GPIO3_FALL           4
#define CS40L2X_EVENT_CTRL_GPIO3_RISE           5
#define CS40L2X_EVENT_CTRL_GPIO4_FALL           6
#define CS40L2X_EVENT_CTRL_GPIO4_RISE           7
#define CS40L2X_EVENT_CTRL_TRIG_STOP            10
#define CS40L2X_EVENT_CTRL_GPIO_STOP            11
#define CS40L2X_EVENT_CTRL_READY                12
#define CS40L2X_EVENT_CTRL_HARDWARE             13
#define CS40L2X_EVENT_CTRL_TRIG_SUSP            14
#define CS40L2X_EVENT_CTRL_TRIG_RESM            15
#define CS40L2X_EVENT_CTRL_NONE                 0xFFFFFF
/** @} */

/**
 * Total number of event source registers to poll in HALO FW.
 *
 * @see cs40l2x_event_regs
 * @see cs40l2x_event_masks
 *
 */
#define CS40L25_EVENT_SOURCES                   (8)

/**
 * Total number of IRQ registers to poll.
 *
 * @see cs40l25_irq2_mask_1_to_event_flag_map
 *
 */
#define CS40L25_EVENT_HW_SOURCES                (6)

/**
 * @defgroup CS40L25_IMASKSEQ_WORD_
 * @brief Macros to formulate IRQ Masks for Wake Write Sequencer
 *
 * @{
 */
#define CS40L25_IMASKSEQ_WORD_1(B)              (((B & 0x000000FF) << 16))
#define CS40L25_IMASKSEQ_WORD_2(B)              ((B & 0xFFFFFF00) >> 8)
/** @} */

/**
 * @defgroup CS40L25_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#ifndef CS40L25_ALGORITHM_DYNAMIC_F0
#define CS40L25_DYNAMIC_F0_ENABLED              (0)
#define CS40L25_DYN_F0_TABLE                    (0)
#define CS40L25_DYNAMIC_REDC                    (0)
#endif // CS40L25_ALGORITHM_DYNAMIC_F0
/** @} */

#define CS40L25_EVENT_VALUE_TO_FLAG_MAP_LENGTH  (15)

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
 * @see cs40l25_power_up_sm
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
 * @see cs40l25_power_down_sm
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
 * Register configuration to send just after the CS40L25 is powered down in Power Down SM
 *
 * List is in the form:
 * - word1 - address of TEST_KEY_CTRL
 * - word2 - 1st unlock value
 * - word3 - address of TEST_KEY_CTRL
 * - word4 - 2nd unlock value
 * - word5 - Address of first configuration register
 * - word6 - Value of first configuration register
 * - word7 - Address of second configuration register
 * - word8 - Value of second configuration register
 * - ...
 * - wordx - address of TEST_KEY_CTRL
 * - wordx - 1st lock value
 * - wordx - address of TEST_KEY_CTRL
 * - wordx - 2nd lock value
 *
 * @see cs40l25_power_down_sm
 *
 */
static const uint32_t cs40l25_pdn_patch[] =
{
    CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_1,
    CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_UNLOCK_2,
    0x00002084, 0x002F1AA3,
    CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_1,
    CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG, CS40L25_TEST_KEY_CTRL_LOCK_2,
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
 * @see cs40l25_power_up_sm
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
 * @see cs40l25_configure_sm
 * @see cs40l25_t member config_regs
 * @see cs40l25_config_registers_t
 *
 * @warning  The list of registers MUST correspond to the union of structs in  in cs40l25_config_registers_t.
 *
 */
static const uint32_t cs40l25_config_register_addresses[CS40L25_CONFIG_REGISTERS_TOTAL] =
{
    CS40L25_INTP_AMP_CTRL_REG,
    CS40L25_MIXER_ASPTX1_INPUT_REG,
    CS40L25_MIXER_ASPTX2_INPUT_REG,
    CS40L25_MIXER_ASPTX3_INPUT_REG,
    CS40L25_MIXER_ASPTX4_INPUT_REG,
    CS40L25_MIXER_DSP1RX1_INPUT_REG,
    CS40L25_MIXER_DSP1RX2_INPUT_REG,
    CS40L25_MIXER_DSP1RX3_INPUT_REG,
    CS40L25_MIXER_DSP1RX4_INPUT_REG,
    CS40L25_MIXER_DACPCM1_INPUT_REG,
    CCM_REFCLK_INPUT_REG,
    MSM_BLOCK_ENABLES_REG,
    MSM_BLOCK_ENABLES2_REG,
    DATAIF_ASP_ENABLES1_REG,
    DATAIF_ASP_CONTROL2_REG,
    DATAIF_ASP_FRAME_CONTROL5_REG,
    DATAIF_ASP_FRAME_CONTROL1_REG,
    DATAIF_ASP_DATA_CONTROL5_REG,
    DATAIF_ASP_DATA_CONTROL1_REG,
    CCM_FS_MON_0_REG,
    DATAIF_ASP_CONTROL1_REG,
    BOOST_LBST_SLOPE_REG,
    BOOST_BST_LOOP_COEFF_REG,
    BOOST_BST_IPK_CTL_REG,
    BOOST_VBST_CTL_1_REG,
    BOOST_VBST_CTL_2_REG,
    CS40L25_WAKESRC_CTL_REG,
    CS40L25_GPIO_BUTTONDETECT,
    CS40L25_GPIO_ENABLE,
    CS40L25_GAIN_CONTROL,
    CS40L25_INDEXBUTTONPRESS,
    CS40L25_INDEXBUTTONPRESS + 4,
    CS40L25_INDEXBUTTONPRESS + 8,
    CS40L25_INDEXBUTTONPRESS + 12,
    CS40L25_INDEXBUTTONRELEASE,
    CS40L25_INDEXBUTTONRELEASE + 4,
    CS40L25_INDEXBUTTONRELEASE + 8,
    CS40L25_INDEXBUTTONRELEASE + 12,
    CS40L25_CLAB_ENABLED,
    CS40L25_PEAK_AMPLITUDE_CONTROL,
    CS40L25_EVENTCONTROL,
};

/**
 * Array of registers and values to add to the Wake Write Sequencer
 *
 * Written to the CS40L25 in the Boot State Machine.
 *
 * List is in the form:
 * - word0 - Address of first configuration register
 * - word1 - Default value for the configuration register
 * - ...
 *
 */
static const uint32_t cs40l25_wseq_regs[] =
{
    BOOST_VBST_CTL_1_REG,           0x000000000,
    BOOST_VBST_CTL_2_REG,           0x000000001,
    BOOST_BST_IPK_CTL_REG,          0x00000004A,
    BOOST_BST_LOOP_COEFF_REG,       0x000002424,
    BOOST_LBST_SLOPE_REG,           0x000007500,
    CS40L25_INTP_AMP_CTRL_REG,      0x000008000,
    CS40L25_WAKESRC_CTL_REG,        0x000000008,
    CCM_REFCLK_INPUT_REG,           0x000000010,
    DATAIF_ASP_ENABLES1_REG,        0x000000000,
    DATAIF_ASP_CONTROL1_REG,        0x000000028,
    CCM_FS_MON_0_REG,               0x000000000,
    DATAIF_ASP_CONTROL2_REG,        0x018180200,
    DATAIF_ASP_FRAME_CONTROL5_REG,  0x000000100,
    DATAIF_ASP_FRAME_CONTROL1_REG,  0x003020100,
    DATAIF_ASP_DATA_CONTROL5_REG,   0x000000018,
    DATAIF_ASP_DATA_CONTROL1_REG,   0x000000018,
    MSM_BLOCK_ENABLES2_REG,         0x010000010,
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
 * @see cs40l25_get_dsp_status_sm
 * @see cs40l25_dsp_status_t
 *
 * @warning  The list of registers MUST correspond to the union of structs in  in cs40l25_dsp_status_t.
 *
 */
static const uint32_t cs40l25_dsp_status_addresses[CS40L25_DSP_STATUS_WORDS_TOTAL] =
{
        CS40L25_HALO_STATE,
        CS40L25_HALO_HEARTBEAT
};

/**
 * DSP Memory addresses to read during Event Handler SM
 *
 * List is in the form:
 * - word0 - Address of first status register
 * - word1 - Address of second status register
 * - ...
 *
 * @see cs40l2x_event_masks
 *
 * @warning This list MUST correspond with the list in cs40l2x_event_masks
 *
 */
static const uint32_t cs40l2x_event_regs[CS40L25_EVENT_SOURCES] =
{
    CS40L25_HARDWAREEVENT, // For the Event Handler SM, HW Event handling must be first
    CS40L25_GPIO1EVENT,
    CS40L25_GPIO2EVENT,
    CS40L25_GPIO3EVENT,
    CS40L25_GPIO4EVENT,
    CS40L25_GPIOPLAYBACKEVENT,
    CS40L25_TRIGGERPLAYBACKEVENT,
    CS40L25_RXREADYEVENT,
};

/**
 * Masks to the 'EventControl' HALO FW control that enable events
 *
 * List is in the form:
 * - word0 - Mask in 'EventControl' that will enable results from HARDWAREEVENT
 * - word1 - Mask in 'EventControl' that will enable results from GPIO1EVENT
 * - ...
 *
 * @see cs40l2x_event_regs
 *
 * @warning This list MUST correspond with the list in cs40l2x_event_regs
 *
 */
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

/**
 * Map from values of the *Event controls in HALO FW to flag bitmasks to notification callback
 *
 * The Event Handler SM will poll the *Event controls in HALO FW ('GPIO1Event', 'TriggerPlaybackEvent', etc).  The
 * values of these registers are unique to a specific event, which is mapped to a bitmask to be applied to
 * cs40l25_t.event_flags.  This value is passed to the calling layer via the notification callback.
 *
 * @see CS40L25_EVENT_FLAG_
 *
 */
static const uint32_t cs40l25_event_value_to_flag_map[CS40L25_EVENT_VALUE_TO_FLAG_MAP_LENGTH] =
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

/**
 * Map of IRQ flags to flag bitmasks to notification callback
 *
 * If enabled, the Event Handler SM will poll the 'HardwareEvent' control in HALO FW.  The Event Handler SM will then
 * poll for IRQ hardware events, and based on the values apply the event bitmasks to cs40l25_t.event_flags.  This value
 * is passed to the calling layer via the notification callback.
 *
 * @see CS40L25_EVENT_FLAG_
 * @see SECTION_7_24_IRQ2
 *
 */
static const uint32_t cs40l25_irq2_mask_1_to_event_flag_map[CS40L25_EVENT_HW_SOURCES * 2] =
{
    IRQ2_IRQ2_EINT_1_AMP_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_AMP_SHORT,
    IRQ2_IRQ2_EINT_1_TEMP_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_OVERTEMP_ERROR,
    IRQ2_IRQ2_EINT_1_TEMP_WARN_RISE_EINT2_BITMASK, CS40L25_EVENT_FLAG_OVERTEMP_WARNING,
    IRQ2_IRQ2_EINT_1_BST_SHORT_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_BOOST_INDUCTOR_SHORT,
    IRQ2_IRQ2_EINT_1_BST_DCM_UVP_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_BOOST_UNDERVOLTAGE,
    IRQ2_IRQ2_EINT_1_BST_OVP_ERR_EINT2_BITMASK, CS40L25_EVENT_FLAG_BOOST_OVERVOLTAGE,
};

/**
 * Wake Write Sequencer patch to be applied during the Configure SM.
 *
 * These values are formatted to the packed specification required by the Write Sequencer.
 *
 */
static const uint32_t cs40l25_irqmaskseq_patch[] =
{
    CS40L25_IRQMASKSEQUENCE,        CS40L25_IMASKSEQ_WORD_1(CS40L25_IRQ2_MASK1_DEFAULT),
    (CS40L25_IRQMASKSEQUENCE + 4),  CS40L25_IMASKSEQ_WORD_2(CS40L25_IRQ2_MASK1_DEFAULT),
    (CS40L25_IRQMASKSEQUENCE + 8),  CS40L25_IMASKSEQ_WORD_1(CS40L25_IRQ2_MASK2_DEFAULT),
    (CS40L25_IRQMASKSEQUENCE + 12), CS40L25_IMASKSEQ_WORD_2(CS40L25_IRQ2_MASK2_DEFAULT),
    (CS40L25_IRQMASKSEQUENCE + 16), CS40L25_IMASKSEQ_WORD_1(CS40L25_IRQ2_MASK3_DEFAULT),
    (CS40L25_IRQMASKSEQUENCE + 20), CS40L25_IMASKSEQ_WORD_2(CS40L25_IRQ2_MASK3_DEFAULT),
    (CS40L25_IRQMASKSEQUENCE + 24), CS40L25_IMASKSEQ_WORD_1(CS40L25_IRQ2_MASK4_DEFAULT),
    (CS40L25_IRQMASKSEQUENCE + 28), CS40L25_IMASKSEQ_WORD_2(CS40L25_IRQ2_MASK4_DEFAULT),
    CS40L25_IRQMASKSEQUENCE_VALID, 0x1,
};

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Notify the driver when the BSP Timer expires.
 *
 * Implementation of cs40l25_private_functions_t.timer_callback
 *
 */
static void cs40l25_timer_callback(uint32_t status, void *cb_arg)
{
    cs40l25_t *d;

    d = (cs40l25_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        CS40L25_SET_FLAG(d->control_sm.flags, CS40L25_FLAGS_TIMEOUT);
    }

    return;
}

/**
 * Notify the driver when the BSP Control Port (cp) read transaction completes.
 *
 * Implementation of cs40l25_private_functions_t.cp_read_callback
 *
 */
static void cs40l25_cp_read_callback(uint32_t status, void *cb_arg)
{
    cs40l25_t *d;

    d = (cs40l25_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Check the driver mode to know which state machine called the BSP API and set respective flag
        if (d->mode == CS40L25_MODE_HANDLING_CONTROLS)
        {
            CS40L25_SET_FLAG(d->control_sm.flags, CS40L25_FLAGS_CP_RW_DONE);
        }
        else
        {
            CS40L25_SET_FLAG(d->event_sm.flags, CS40L25_FLAGS_CP_RW_DONE);
        }

        /*
         *  Copy 32-bit word read from BSP-allocated buffer to driver's cache.  Responses to Control Port reads
         *  come over the bus MS-Byte-first, so end up Big-Endian in the BSP buffer.  This requires swapping bytes
         *  to the driver's Little-Endian uint32_t cache.
         *
         *  FIXME: This is not platform independent.
         */
        ADD_BYTE_TO_WORD(d->register_buffer, d->cp_read_buffer[0], 3);
        ADD_BYTE_TO_WORD(d->register_buffer, d->cp_read_buffer[1], 2);
        ADD_BYTE_TO_WORD(d->register_buffer, d->cp_read_buffer[2], 1);
        ADD_BYTE_TO_WORD(d->register_buffer, d->cp_read_buffer[3], 0);
    }

    return;
}

/**
 * Notify the driver when the BSP Control Port (cp) write transaction completes.
 *
 * Implementation of cs40l25_private_functions_t.cp_write_callback
 *
 */
static void cs40l25_cp_write_callback(uint32_t status, void *cb_arg)
{
    cs40l25_t *d;

    d = (cs40l25_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Check the driver mode to know which state machine called the BSP API and set respective flag
        if (d->mode == CS40L25_MODE_HANDLING_CONTROLS)
        {
            CS40L25_SET_FLAG(d->control_sm.flags, CS40L25_FLAGS_CP_RW_DONE);
        }
        else
        {
            CS40L25_SET_FLAG(d->event_sm.flags, CS40L25_FLAGS_CP_RW_DONE);
        }
    }
    else
    {
        // Check the driver mode to know which state machine called the BSP API and set respective flag
        if (d->mode == CS40L25_MODE_HANDLING_CONTROLS)
        {
            CS40L25_SET_FLAG(d->control_sm.flags, CS40L25_FLAGS_CP_RW_ERROR);
        }
        else
        {
            CS40L25_SET_FLAG(d->event_sm.flags, CS40L25_FLAGS_CP_RW_ERROR);
        }
    }

    return;
}

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
        // Only if the driver is in CS40L25_MODE_HANDLING_CONTROLS, then reset Event Handler state machine
        if (d->mode == CS40L25_MODE_HANDLING_CONTROLS)
        {
            // Switch driver mode to CS40L25_MODE_HANDLING_EVENTS
            d->mode = CS40L25_MODE_HANDLING_EVENTS;
            // Reset Event Handler state machine
            d->event_sm.state = CS40L25_EVENT_SM_STATE_INIT;
            d->event_sm.flags = 0;
            d->event_sm.count = 0;
            /*
             * This is left to support the potential of having multiple types of Event Handler state machines.
             */
            d->event_sm.fp = cs40l25_private_functions_g->event_sm;
        }
    }

    return;
}

/**
 * Adds register address/value pair to the Write Sequencer table
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] address          Register address to add
 * @param [in] value            Register value to add
 *
 * @return
 * - CS40L25_STATUS_FAIL        if the fixed-size Write Sequencer Table is full
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_wseq_table_add(cs40l25_t *driver, uint32_t address, uint32_t value)
{
    uint32_t ret = CS40L25_STATUS_OK;

    cs40l25_wseq_entry_t *table = &driver->wseq_table;
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

/**
 * Updated a register address/value pair in the Write Sequencer table
 *
 * If value does not currently exist, it will be added to the table.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] address          Register address to update
 * @param [in] value            New value to apply to the register
 *
 * @return
 * - CS40L25_STATUS_FAIL        if the fixed-size Write Sequencer Table is full
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_wseq_table_update(cs40l25_t *driver, uint32_t address, uint32_t value)
{
    uint32_t ret = CS40L25_STATUS_OK;

    cs40l25_wseq_entry_t *table = &driver->wseq_table;
    if (address < 0xFFFF)
    {
        uint32_t num_entries = driver->wseq_num_entries;
        bool address_found = false;
        for (int i = 0; i < num_entries; i++) {
            uint32_t temp_address = (table[i].address_ms << 8) | (table[i].address_ls);
            uint32_t temp_value = (table[i].val_3 << 24) | (table[i].val_2 << 16) |
                                  (table[i].val_1 << 8) | (table[i].val_0);
            if ((temp_address == address) &&
                (temp_value != value) &&
                (address != CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG))
            {
                table[i].val_3 = (value & 0xFF000000) >> 24;
                table[i].val_2 = (value & 0x00FF0000) >> 16;
                table[i].val_1 = (value & 0x0000FF00) >> 8;
                table[i].val_0 = value & 0x000000FF;
                table[i].changed = 1;

                address_found = true;
            }
        }

        if (!address_found)
        {
            ret = cs40l25_wseq_table_add(driver, address, value);
        }
    }

    return ret;
}

/**
 * Add an array of register address/value pairs the Write Sequencer table
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] entries          Pointer to the array of address/value pairs
 * @param [in] num_entries      Number of address/value pairs in 'entries'
 *
 * @return
 * - CS40L25_STATUS_FAIL        if the fixed-size Write Sequencer Table is full
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_wseq_add_block(cs40l25_t *driver, uint32_t *entries, uint32_t num_entries)
{
    uint32_t ret;
    for (int i = 0; i < num_entries; i++) {
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
static uint32_t cs40l25_read_reg(cs40l25_t *driver, uint32_t addr, uint32_t *val, bool is_blocking)
{
    uint32_t ret = CS40L25_STATUS_FAIL;

    /*
     * Switch from Little-Endian contents of uint32_t 'addr' to Big-Endian format required for Control Port transaction.
     * Since register address is first written, cp_write_buffer[] is filled with register address.
     *
     * FIXME: This is not platform independent.
     */
    driver->cp_write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    driver->cp_write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    driver->cp_write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    driver->cp_write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    // Currently only I2C transactions are supported
    if (driver->bus_type == CS40L25_BUS_TYPE_I2C)
    {
        uint32_t bsp_status;

        if (is_blocking)
        {
            bsp_status = bsp_driver_if_g->i2c_read_repeated_start(driver->bsp_dev_id,
                                                                  driver->cp_write_buffer,
                                                                  4,
                                                                  driver->cp_read_buffer,
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
                ADD_BYTE_TO_WORD(*val, driver->cp_read_buffer[0], 3);
                ADD_BYTE_TO_WORD(*val, driver->cp_read_buffer[1], 2);
                ADD_BYTE_TO_WORD(*val, driver->cp_read_buffer[2], 1);
                ADD_BYTE_TO_WORD(*val, driver->cp_read_buffer[3], 0);

                ret = CS40L25_STATUS_OK;
            }
        }
        else
        {
            bsp_status = bsp_driver_if_g->i2c_read_repeated_start(driver->bsp_dev_id,
                                                                  driver->cp_write_buffer,
                                                                  4,
                                                                  driver->cp_read_buffer,
                                                                  4,
                                                                  cs40l25_private_functions_g->cp_read_callback,
                                                                  driver);
            if (BSP_STATUS_OK == bsp_status)
            {
                ret = CS40L25_STATUS_OK;
            }
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
static uint32_t cs40l25_write_reg(cs40l25_t *driver, uint32_t addr, uint32_t val, bool is_blocking)
{
    uint32_t ret = CS40L25_STATUS_FAIL;
    uint32_t bsp_status = BSP_STATUS_FAIL;

    //Update corresponding entry in wseq_table if it exists
    cs40l25_wseq_table_update(driver, addr, val);

    /*
     * Copy Little-Endian contents of 'addr' and 'val' to the Big-Endian format required for Control Port transactions
     * using a uint8_t cp_write_buffer.
     *
     * FIXME: This is not platform independent.
     */
    driver->cp_write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    driver->cp_write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    driver->cp_write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    driver->cp_write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);
    driver->cp_write_buffer[4] = GET_BYTE_FROM_WORD(val, 3);
    driver->cp_write_buffer[5] = GET_BYTE_FROM_WORD(val, 2);
    driver->cp_write_buffer[6] = GET_BYTE_FROM_WORD(val, 1);
    driver->cp_write_buffer[7] = GET_BYTE_FROM_WORD(val, 0);

    // Currently only I2C transactions are supported
    if (driver->bus_type == CS40L25_BUS_TYPE_I2C)
    {
        if (is_blocking)
        {
            bsp_status = bsp_driver_if_g->i2c_write(driver->bsp_dev_id,
                                                    driver->cp_write_buffer,
                                                    8,
                                                    NULL,
                                                    NULL);

        }
        else
        {
            bsp_status = bsp_driver_if_g->i2c_write(driver->bsp_dev_id,
                                                    driver->cp_write_buffer,
                                                    8,
                                                    cs40l25_private_functions_g->cp_write_callback,
                                                    driver);
        }
    }

    if (BSP_STATUS_OK == bsp_status)
    {
        ret = CS40L25_STATUS_OK;
    }

    return ret;
}

/**
 * Reset State Machine
 *
 * Implementation of cs40l25_private_functions_t.reset_sm
 *
 */
static uint32_t cs40l25_reset_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *r = &(driver->control_sm);
    uint32_t bsp_status = BSP_STATUS_OK;

    switch(r->state)
    {
        case CS40L25_RESET_SM_STATE_INIT:
            // Drive RESET low for at least T_RLPW (1ms)
            bsp_status = bsp_driver_if_g->set_gpio(driver->bsp_reset_gpio_id, BSP_GPIO_LOW);
            if (bsp_status == BSP_STATUS_OK)
            {
                CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_TIMEOUT);
                bsp_status = bsp_driver_if_g->set_timer(CS40L25_T_RLPW_MS, cs40l25_timer_callback, driver);
                if (bsp_status == BSP_STATUS_OK)
                {
                    r->state = CS40L25_RESET_SM_STATE_WAIT_T_RLPW;
                }
            }
            break;

        case CS40L25_RESET_SM_STATE_WAIT_T_RLPW:
            if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_TIMEOUT))
            {
                r->state = CS40L25_RESET_SM_STATE_WAIT_T_IRS;
                // Drive RESET high and wait for at least T_IRS (1ms)
                bsp_status = bsp_driver_if_g->set_gpio(driver->bsp_reset_gpio_id, BSP_GPIO_HIGH);
                if (bsp_status == BSP_STATUS_OK)
                {
                    CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_TIMEOUT);
                    bsp_status = bsp_driver_if_g->set_timer(CS40L25_T_IRS_MS, cs40l25_timer_callback, driver);
                    if (bsp_status == BSP_STATUS_OK)
                    {
                        r->state = CS40L25_RESET_SM_STATE_WAIT_T_IRS;
                    }
                }
            }
            break;

        case CS40L25_RESET_SM_STATE_WAIT_T_IRS:
            if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_CP_RW_DONE);
                r->count = 0;
                // Start polling OTP_BOOT_DONE bit every 10ms
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            IRQ1_IRQ1_EINT_4_REG,
                                                            &(driver->register_buffer),
                                                            false);

                if (ret == CS40L25_STATUS_OK)
                {
                    CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_TIMEOUT);
                    bsp_status = bsp_driver_if_g->set_timer(CS40L25_POLL_OTP_BOOT_DONE_MS,
                                                            cs40l25_timer_callback,
                                                            driver);
                    if (bsp_status == BSP_STATUS_OK)
                    {
                        r->state = CS40L25_RESET_SM_STATE_WAIT_OTP_BOOT_DONE;
                    }
                }
            }
            break;

        case CS40L25_RESET_SM_STATE_WAIT_OTP_BOOT_DONE:
            if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_TIMEOUT))
            {
                if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_CP_RW_DONE))
                {
                    r->count++;
                    // If OTP_BOOT_DONE is set
                    if (driver->register_buffer & IRQ1_IRQ1_EINT_4_BOOT_DONE_BITMASK)
                    {
                        CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_CP_RW_DONE);
                        // Read OTP_BOOT_ERR
                        ret = cs40l25_private_functions_g->read_reg(driver,
                                                                    IRQ1_IRQ1_EINT_3_REG,
                                                                    &(driver->register_buffer),
                                                                    false);

                        if (ret == CS40L25_STATUS_OK)
                        {
                            r->count = 0;
                            r->state = CS40L25_RESET_SM_STATE_OTP_ERR_STATUS;
                        }
                    }
                    // If polling period expired, indicate ERROR
                    else if (r->count >= CS40L25_POLL_OTP_BOOT_DONE_MAX)
                    {
                        ret = CS40L25_STATUS_FAIL;
                        r->state = CS40L25_RESET_SM_STATE_ERROR;
                    }
                    // If time left to poll, read OTP_BOOT_DONE again
                    else
                    {
                        CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_CP_RW_DONE);
                        ret = cs40l25_private_functions_g->read_reg(driver,
                                                                    IRQ1_IRQ1_EINT_4_REG,
                                                                    &(driver->register_buffer),
                                                                    false);

                        if (ret == CS40L25_STATUS_OK)
                        {
                            CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_TIMEOUT);
                            bsp_status = bsp_driver_if_g->set_timer(CS40L25_POLL_OTP_BOOT_DONE_MS,
                                                                    cs40l25_timer_callback,
                                                                    driver);
                        }
                    }
                }
                // If after 10ms I2C Read Callback hasn't been called from BSP, assume an error
                else
                {
                    ret = CS40L25_STATUS_FAIL;
                    r->state = CS40L25_RESET_SM_STATE_ERROR;
                }
            }
            break;

        case CS40L25_RESET_SM_STATE_OTP_ERR_STATUS:
            if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                if (driver->register_buffer & IRQ1_IRQ1_EINT_3_OTP_BOOT_ERR_BITMASK)
                {
                    ret = CS40L25_STATUS_FAIL;
                    r->state = CS40L25_RESET_SM_STATE_ERROR;
                }
                else
                {
                    CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_CP_RW_DONE);
                    // Read DEVID
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                CS40L25_SW_RESET_DEVID_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    if (ret == CS40L25_STATUS_OK)
                    {
                        r->state = CS40L25_RESET_SM_STATE_READ_ID;
                    }
                }
            }
            else if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_CP_RW_ERROR))
            {
                ret = CS40L25_STATUS_FAIL;
                r->state = CS40L25_RESET_SM_STATE_ERROR;
            }
            break;

        case CS40L25_RESET_SM_STATE_READ_ID:
            if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                r->count++;
                if (r->count == 1)
                {
                    driver->devid = driver->register_buffer;

                    CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_CP_RW_DONE);
                    // Read REVID
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                CS40L25_SW_RESET_REVID_REG,
                                                                &(driver->register_buffer),
                                                                false);
                }
                else
                {
                    driver->revid = driver->register_buffer;

                    CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_CP_RW_DONE);
                    r->count = 0;
                    // Start polling BHM_AMP_STATUS_BOOT_DONE bit every 10ms
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                DSP_BHM_AMP_STATUS_REG,
                                                                &(driver->register_buffer),
                                                                false);

                    if (ret == CS40L25_STATUS_OK)
                    {
                        CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_TIMEOUT);
                        bsp_status = bsp_driver_if_g->set_timer(CS40L25_POLL_OTP_BOOT_DONE_MS,
                                                                cs40l25_timer_callback,
                                                                driver);
                        if (bsp_status == BSP_STATUS_OK)
                        {
                            r->state = CS40L25_RESET_SM_STATE_WAIT_BHM_BOOT_DONE;
                        }
                    }
                }
            }
            else if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_CP_RW_ERROR))
            {
                ret = CS40L25_STATUS_FAIL;
                r->state = CS40L25_RESET_SM_STATE_ERROR;
            }
            break;

        case CS40L25_RESET_SM_STATE_WAIT_BHM_BOOT_DONE:
            if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_TIMEOUT))
            {
                if (CS40L25_IS_FLAG_SET(r->flags, CS40L25_FLAGS_CP_RW_DONE))
                {
                    r->count++;
                    // If BHM BOOT_DONE is set
                    if (driver->register_buffer & DSP_BHM_AMP_STATUS_BOOT_DONE_BITMASK)
                    {
                        CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_CP_RW_DONE);
                        r->count = 0;
                        r->state = CS40L25_RESET_SM_STATE_DONE;
                    }
                    // If polling period expired, indicate ERROR
                    else if (r->count >= CS40L25_POLL_OTP_BOOT_DONE_MAX)
                    {
                        ret = CS40L25_STATUS_FAIL;
                        r->state = CS40L25_RESET_SM_STATE_ERROR;
                    }
                    // If time left to poll, read OTP_BOOT_DONE again
                    else
                    {
                        CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_CP_RW_DONE);
                        ret = cs40l25_private_functions_g->read_reg(driver,
                                                                    DSP_BHM_AMP_STATUS_REG,
                                                                    &(driver->register_buffer),
                                                                    false);

                        if (ret == CS40L25_STATUS_OK)
                        {
                            CS40L25_CLEAR_FLAG(r->flags, CS40L25_FLAGS_TIMEOUT);
                            bsp_status = bsp_driver_if_g->set_timer(CS40L25_POLL_OTP_BOOT_DONE_MS,
                                                                    cs40l25_timer_callback,
                                                                    driver);
                        }
                    }
                }
                // If after 10ms I2C Read Callback hasn't been called from BSP, assume an error
                else
                {
                    ret = CS40L25_STATUS_FAIL;
                    r->state = CS40L25_RESET_SM_STATE_ERROR;
                }
            }
            break;

        // For both DONE and ERROR, do nothing
        case CS40L25_RESET_SM_STATE_DONE:
        case CS40L25_RESET_SM_STATE_ERROR:
            break;

        default:
            ret = CS40L25_STATUS_FAIL;
            r->state = CS40L25_RESET_SM_STATE_ERROR;
            break;
    }

    if ((ret != CS40L25_STATUS_OK) || (bsp_status != BSP_STATUS_OK))
    {
        ret = CS40L25_STATUS_FAIL;
        r->state = CS40L25_RESET_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Boot State Machine
 *
 * Implementation of cs40l25_private_functions_t.boot_sm
 *
 */
static uint32_t cs40l25_boot_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *b = &(driver->control_sm);
    cs40l25_boot_config_t *cfg = driver->boot_config;

    if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        b->state = CS40L25_BOOT_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(b->state)
    {
        case CS40L25_BOOT_SM_STATE_INIT:
            // Validate the boot configuration
            ret = cs40l25_private_functions_g->validate_boot_config(cfg,
                                                                    CS40L25_IS_FLAG_SET(b->flags, \
                                                                            CS40L25_FLAGS_REQUEST_FW_BOOT),
                                                                    CS40L25_IS_FLAG_SET(b->flags, \
                                                                            CS40L25_FLAGS_REQUEST_COEFF_BOOT),
                                                                    CS40L25_IS_FLAG_SET(b->flags, \
                                                                            CS40L25_FLAGS_REQUEST_CAL_BOOT));
            // If there is a valid boot configuration
            if (ret == CS40L25_STATUS_BOOT_REQUEST)
            {
                b->count = 0;
                CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_CP_RW_DONE);
                halo_boot_block_t *temp_block;

                if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_REQUEST_CAL_BOOT))
                {
                    // Get first calibration FW block
                    temp_block = cfg->cal_blocks;
                    b->state = CS40L25_BOOT_SM_STATE_LOAD_CAL;
                }
                // If there are FW blocks to boot
                else if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_REQUEST_FW_BOOT))
                {
                    //Ignore first word of errata_patch
                    int errata_entries = (sizeof(cs40l25_revb0_errata_patch) - sizeof(uint32_t)) / (2 * sizeof(uint32_t));
                    int wseq_entries = sizeof(cs40l25_wseq_regs) / (2 * sizeof(uint32_t));

                    driver->wseq_num_entries = 0;
                    cs40l25_wseq_add_block(driver, cs40l25_revb0_errata_patch + 1, errata_entries);
                    cs40l25_wseq_add_block(driver, cs40l25_wseq_regs, wseq_entries);

                    // Get first FW block
                    temp_block = cfg->fw_blocks;
                    b->state = CS40L25_BOOT_SM_STATE_LOAD_FW;
                }
                // Otherwise, it must be COEFF-only boot
                else
                {
                    CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_REQUEST_COEFF_BOOT);
                    // Get first COEFF block
                    temp_block = cfg->coeff_files->data;
                    b->state = CS40L25_BOOT_SM_STATE_LOAD_COEFF;
                }
                // Write first block (either FW or COEFF) to HALO DSP memory
                ret = cs40l25_private_functions_g->cp_bulk_write(driver,
                                                                 temp_block->address,
                                                                 (uint8_t *) temp_block->bytes,
                                                                 temp_block->block_size);
            }
            break;

        case CS40L25_BOOT_SM_STATE_LOAD_CAL:
            if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_CP_RW_DONE);
                b->count++;

                // If there are remaining FW blocks
                if (b->count < cfg->total_cal_blocks)
                {
                    // Get next FW block
                    halo_boot_block_t *temp_block = cfg->cal_blocks;
                    temp_block += b->count;
                    // Write next Cal block to HALO DSP memory
                    ret = cs40l25_private_functions_g->cp_bulk_write(driver,
                                                                     temp_block->address,
                                                                     (uint8_t *) temp_block->bytes,
                                                                     temp_block->block_size);
                }
                else
                {
                    b->count = 0;
                    // Write first post-boot configuration
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_post_boot_config[0],
                                                                 cs40l25_post_boot_config[1],
                                                                 false);
                    b->state = CS40L25_BOOT_SM_STATE_POST_BOOT_CONFIG;
                }

            }
            break;

        case CS40L25_BOOT_SM_STATE_LOAD_FW:
            if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_CP_RW_DONE);
                b->count++;

                // If there are remaining FW blocks
                if (b->count < cfg->total_fw_blocks)
                {
                    // Get next FW block
                    halo_boot_block_t *temp_block = cfg->fw_blocks;
                    temp_block += b->count;
                    // Write next FW block to HALO DSP memory
                    ret = cs40l25_private_functions_g->cp_bulk_write(driver,
                                                                     temp_block->address,
                                                                     (uint8_t *) temp_block->bytes,
                                                                     temp_block->block_size);
                }
                else
                {
                    b->count = 0;
                    // If there is also a request to boot COEFF blocks
                    if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_REQUEST_COEFF_BOOT))
                    {
                        CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_REQUEST_COEFF_BOOT);
                        // Get first COEFF block
                        halo_boot_block_t *temp_block = cfg->coeff_files->data;
                        // Write first COEFF block to HALO DSP memory
                        ret = cs40l25_private_functions_g->cp_bulk_write(driver,
                                                                         temp_block->address,
                                                                         (uint8_t *) temp_block->bytes,
                                                                         temp_block->block_size);
                        b->state = CS40L25_BOOT_SM_STATE_LOAD_COEFF;
                    }
                    else
                    {
                        // Write first post-boot configuration
                        ret = cs40l25_private_functions_g->write_reg(driver,
                                                                     cs40l25_post_boot_config[0],
                                                                     cs40l25_post_boot_config[1],
                                                                     false);
                        b->state = CS40L25_BOOT_SM_STATE_POST_BOOT_CONFIG;
                    }
                }

            }
            break;

        case CS40L25_BOOT_SM_STATE_LOAD_COEFF:
            if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_CP_RW_DONE);
                b->count++;
                uint32_t file_no;
                uint32_t block_ctr;

                // If there are remaining COEFF blocks
                if (b->count < cfg->total_coeff_blocks)
                {
                    file_no = 0;
                    block_ctr = b->count;
                    while (cfg->coeff_files[file_no].total_blocks <= block_ctr) {
                        block_ctr -= cfg->coeff_files[file_no].total_blocks;
                        file_no++;
                    }

                    if (file_no < CS40L25_MAX_COEFF_FILES) {
                        // Get next COEFF block
                        halo_boot_block_t *temp_block = cfg->coeff_files[file_no].data;
                        temp_block += block_ctr;

                        // Write next COEFF block to HALO DSP memory
                        ret = cs40l25_private_functions_g->cp_bulk_write(driver,
                                                                         temp_block->address,
                                                                         (uint8_t *) temp_block->bytes,
                                                                         temp_block->block_size);
                    }
                    else
                    {
                        ret = CS40L25_STATUS_FAIL;
                    }
                }
                else
                {
                    b->count = 0;
                    // Write first post-boot configuration
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_post_boot_config[0],
                                                                 cs40l25_post_boot_config[1],
                                                                 false);
                    b->state = CS40L25_BOOT_SM_STATE_POST_BOOT_CONFIG;
                }

            }
            break;

        case CS40L25_BOOT_SM_STATE_POST_BOOT_CONFIG:
            if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                b->count++;
                // If there are remaining post-boot configuration words
                if (b->count < (sizeof(cs40l25_post_boot_config)/(sizeof(uint32_t) * 2)))
                {
                    CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_CP_RW_DONE);
                    // Write next post-boot configuration
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_post_boot_config[b->count * 2],
                                                                 cs40l25_post_boot_config[(b->count * 2) + 1],
                                                                 false);
                }
                else
                {
                    if (driver->cal_data.is_valid_f0 && CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_REQUEST_FW_BOOT))
                    {
                        CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_CP_RW_DONE);
                        ret = cs40l25_private_functions_g->write_reg(driver,
                                                                     CS40L25_F0_STORED,
                                                                     driver->cal_data.f0,
                                                                     false);
                        b->state = CS40L25_BOOT_SM_STATE_WRITE_F0;
                    }
                    else if (driver->cal_data.is_valid_qest && CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_REQUEST_FW_BOOT))
                    {
                        CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_CP_RW_DONE);
                        ret = cs40l25_private_functions_g->write_reg(driver,
                                                                     CS40L25_Q_STORED,
                                                                     driver->cal_data.qest,
                                                                     false);
                        b->state = CS40L25_BOOT_SM_STATE_WRITE_Q;
                    }
                    else
                    {
                        b->state = CS40L25_BOOT_SM_STATE_DONE;
                    }
                }
            }
            break;

        case CS40L25_BOOT_SM_STATE_WRITE_F0:
            if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             CS40L25_REDC_STORED,
                                                             driver->cal_data.redc,
                                                             false);

                b->state = CS40L25_BOOT_SM_STATE_WRITE_REDC;
            }
            break;

        case CS40L25_BOOT_SM_STATE_WRITE_REDC:
            if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                if (driver->cal_data.is_valid_qest && driver->state == CS40L25_STATE_DSP_STANDBY)
                {
                    CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_Q_STORED,
                                                                 driver->cal_data.qest,
                                                                 false);
                    b->state = CS40L25_BOOT_SM_STATE_WRITE_Q;
                }
                else
                {
                    b->state = CS40L25_BOOT_SM_STATE_DONE;
                }
            }
            break;

        case CS40L25_BOOT_SM_STATE_WRITE_Q:
            if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                b->state = CS40L25_BOOT_SM_STATE_DONE;
            }
            break;

        case CS40L25_BOOT_SM_STATE_DONE:
            break;

        case CS40L25_BOOT_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    return ret;
}

/**
 * Power Up State Machine
 *
 * Implementation of cs40l25_private_functions_t.power_up_sm
 *
 */
static uint32_t cs40l25_power_up_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_POWER_UP_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS40L25_POWER_UP_SM_STATE_INIT:
            sm->count = 0;
            CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

            sm->count = 0;
            // Get errata based on DEVID/REVID
            ret = cs40l25_private_functions_g->get_errata(driver->devid, driver->revid, &(driver->errata));

            if (ret == CS40L25_STATUS_OK)
            {
                sm->state = CS40L25_POWER_UP_SM_STATE_ERRATA;
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                sm->count = 0;
                const uint32_t *errata_write = driver->errata;
                if (*errata_write > 0)
                {
                    // Skip first word which is errata length
                    errata_write++;
                    // Start sending errata
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 *(errata_write),
                                                                 *(errata_write + 1),
                                                                 false);
                    sm->state = CS40L25_POWER_UP_SM_STATE_ERRATA;
                }
                else
                {
                    // Set first HALO DSP Sample Rate registers to G1R2
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_frame_sync_regs[0],
                                                                 CS40L25_DSP1_SAMPLE_RATE_G1R2,
                                                                 false);
                    sm->state = CS40L25_POWER_UP_SM_STATE_SET_FRAME_SYNC;
                }
            }
            break;

        case CS40L25_POWER_UP_SM_STATE_ERRATA:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                const uint32_t *errata_write = driver->errata;

                if ((sm->count * 2) < *errata_write)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                    // Calculate position in errata erray - Skip first word which is errata length
                    errata_write++;
                    errata_write += (sm->count * 2);
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 *(errata_write),
                                                                 *(errata_write + 1),
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Set first HALO DSP Sample Rate registers to G1R2
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_frame_sync_regs[0],
                                                                 CS40L25_DSP1_SAMPLE_RATE_G1R2,
                                                                 false);
                    sm->state = CS40L25_POWER_UP_SM_STATE_SET_FRAME_SYNC;
                }
            }
            break;

        case CS40L25_POWER_UP_SM_STATE_SET_FRAME_SYNC:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                sm->count++;
                // If there are more Sample Rate registers to write
                if (sm->count < (sizeof(cs40l25_frame_sync_regs)/sizeof(uint32_t)))
                {
                    // Set next HALO DSP Sample Rate register to G1R2
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_frame_sync_regs[sm->count],
                                                                 CS40L25_DSP1_SAMPLE_RATE_G1R2,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Send first words of Power Up Patch
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_pup_patch[0],
                                                                 cs40l25_pup_patch[1],
                                                                 false);
                    sm->state = CS40L25_POWER_UP_SM_STATE_PUP_PATCH;
                }
            }
            break;

        case CS40L25_POWER_UP_SM_STATE_PUP_PATCH:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                sm->count++;
                // If there are remaining Power Up Patch words
                if (sm->count < (sizeof(cs40l25_pup_patch)/(sizeof(uint32_t) * 2)))
                {
                    // Send next words of Power Up Patch
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_pup_patch[sm->count * 2],
                                                                 cs40l25_pup_patch[(sm->count * 2) + 1],
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Read the HALO DSP CCM control register
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS40L25_POWER_UP_SM_STATE_CLOCKS_TO_DSP;
                }
            }
            break;

        case CS40L25_POWER_UP_SM_STATE_CLOCKS_TO_DSP:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    uint32_t temp_reg = driver->register_buffer;
                    sm->count++;
                    // Enable clocks to HALO DSP core
                    temp_reg |= XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_EN_BITMASK | XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_RESET_BITMASK;
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                DSP_REG(HALO_STATE),
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS40L25_POWER_UP_SM_STATE_WAIT_HALO_STATE;
                }
            }
            break;

        case CS40L25_POWER_UP_SM_STATE_WAIT_HALO_STATE_T:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            DSP_REG(HALO_STATE),
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS40L25_POWER_UP_SM_STATE_WAIT_HALO_STATE;
            }
            break;

        case CS40L25_POWER_UP_SM_STATE_WAIT_HALO_STATE:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                if (driver->register_buffer == 0xCB)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                XM_UNPACKED24_DSP1_SCRATCH_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS40L25_POWER_UP_SM_STATE_WAIT_HALO_SCRATCH;
                }
                else if (sm->count >= CS40L25_POLL_OTP_BOOT_DONE_MAX)
                {
                    sm->state = CS40L25_POWER_UP_SM_STATE_ERROR;
                    ret = CS40L25_STATUS_FAIL;
                }
                else
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                    // After enabling core, wait for at least T_AMP_PUP (1ms)
                    ret = bsp_driver_if_g->set_timer(CS40L25_T_BST_PUP_MS, cs40l25_timer_callback, driver);
                    sm->state = CS40L25_POWER_UP_SM_STATE_WAIT_HALO_STATE_T;
                }
            }
            break;

        case CS40L25_POWER_UP_SM_STATE_WAIT_HALO_SCRATCH:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                if (driver->register_buffer)
                {
                    sm->state = CS40L25_POWER_UP_SM_STATE_ERROR;
                    ret = CS40L25_STATUS_FAIL;
                }
                else
                {
                    sm->state = CS40L25_POWER_UP_SM_STATE_DONE;
                }
            }
            break;

        case CS40L25_POWER_UP_SM_STATE_DONE:
            break;

        case CS40L25_POWER_UP_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_POWER_UP_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Power Down State Machine
 *
 * Implementation of cs40l25_private_functions_t.power_down_sm
 *
 */
static uint32_t cs40l25_power_down_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_POWER_DOWN_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS40L25_POWER_DOWN_SM_STATE_INIT:
            sm->count = 0;
            CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

            // If DSP is NOT booted
            if (driver->state == CS40L25_STATE_POWER_UP)
            {
                // Request BHM shuts down
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             DSP_BHM_AMP_SHUTDOWNREQUEST_REG,
                                                             DSP_BHM_AMP_SHUTDOWNREQUEST_BITMASK,
                                                             false);
                sm->count = 0;
                // Wait for at least 1ms
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, cs40l25_timer_callback, driver);
                sm->state = CS40L25_POWER_DOWN_SM_STATE_BHM_SD_WAIT;
            }
#ifdef INCLUDE_CAL
            else if (driver->state == CS40L25_STATE_CAL_POWER_UP)
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                // Force fw into standby
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             CS40L25_CAL_SHUTDOWNREQUEST,
                                                             1,
                                                             false);

                sm->state = CS40L25_POWER_DOWN_SM_STATE_CAL_START;
            }
#endif // INCLUDE_CAL
            else
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                // Force fw into standby
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                                             CS40L25_POWERCONTROL_FRC_STDBY,
                                                             false);

                sm->state = CS40L25_POWER_DOWN_SM_STATE_MBOX_START;
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_BHM_SD_WAIT:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    // Read SHUTDOWNREQUEST to see if the reg has been cleared
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                DSP_BHM_AMP_SHUTDOWNREQUEST_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_BHM_SD_READ;
                }
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_BHM_SD_READ:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                // If OTP_BOOT_DONE is set
                if (driver->register_buffer == 0)
                {
                    // Read BHM_STATEMACHINE
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                DSP_BHM_STATEMACHINE_REG,
                                                                &(driver->register_buffer),
                                                                false);

                    if (ret == CS40L25_STATUS_OK)
                    {
                        sm->count = 0;
                        sm->state = CS40L25_POWER_DOWN_SM_STATE_BHM_SM_READ;
                    }
                }
                // If polling period expired, indicate ERROR
                else if (sm->count >= CS40L25_POLL_OTP_BOOT_DONE_MAX)
                {
                    ret = CS40L25_STATUS_FAIL;
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_ERROR;
                }
                // If time left to poll, read OTP_BOOT_DONE again
                else
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                    ret = bsp_driver_if_g->set_timer(CS40L25_POLL_OTP_BOOT_DONE_MS,
                                                     cs40l25_timer_callback,
                                                     driver);
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_BHM_SD_WAIT;
                }
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_BHM_SM_READ:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                // If STATEMACHINE != shutdown
                if (driver->register_buffer != DSP_BHM_STATEMACHINE_SHUTDOWN)
                {
                    ret = CS40L25_STATUS_FAIL;
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_ERROR;
                }
                else
                {
                    // Read BHM_AMP_STATUS
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                DSP_BHM_AMP_STATUS_REG,
                                                                &(driver->register_buffer),
                                                                false);

                    if (ret == CS40L25_STATUS_OK)
                    {
                        sm->count = 0;
                        sm->state = CS40L25_POWER_DOWN_SM_STATE_BHM_AS_READ;
                    }
                }
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_BHM_AS_READ:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                // If any errors:
                if (driver->register_buffer & (DSP_BHM_AMP_STATUS_OTP_ERROR_BITMASK |
                                               DSP_BHM_AMP_STATUS_AMP_ERROR_BITMASK |
                                               DSP_BHM_AMP_STATUS_TEMP_RISE_WARN_BITMASK |
                                               DSP_BHM_AMP_STATUS_TEMP_ERROR_BITMASK))
                {
                    ret = CS40L25_STATUS_FAIL;
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_ERROR;
                }
                else
                {
                    // start basic mode revert
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_bhm_revert_patch[0],
                                                                 cs40l25_bhm_revert_patch[1],
                                                                 false);
                    if (ret == CS40L25_STATUS_OK)
                    {
                        sm->count = 0;
                        sm->state = CS40L25_POWER_DOWN_SM_STATE_BHM_REVERT_PATCH;
                    }
                }
            }
            break;
        case CS40L25_POWER_DOWN_SM_STATE_BHM_REVERT_PATCH:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count += 2;
                if (sm->count < (sizeof(cs40l25_bhm_revert_patch)/sizeof(uint32_t)))
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    // Send next words of BHM revert patch set
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_bhm_revert_patch[sm->count],
                                                                 cs40l25_bhm_revert_patch[sm->count + 1],
                                                                 false);
                }
                else
                {
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_DONE;
                }
            }
            break;

#ifdef INCLUDE_CAL
        case CS40L25_POWER_DOWN_SM_STATE_CAL_START:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count = 0;
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                ret = bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS,
                                                 cs40l25_timer_callback,
                                                 driver);
                sm->state = CS40L25_POWER_DOWN_SM_STATE_CAL_TIMER;
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_CAL_TIMER:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                // Read mbox reg to see if it has been reset
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            CS40L25_CAL_SHUTDOWNREQUEST,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS40L25_POWER_DOWN_SM_STATE_CAL_READ;
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_CAL_READ:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                // If ack ctrl has been reset
                if (driver->register_buffer == 0)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    // Read so we can update bits
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_CORE_CTRL;
                }
                // If polling period expired, indicate ERROR
                else if (sm->count >= CS40L25_POLL_ACK_CTRL_MAX)
                {
                    ret = CS40L25_STATUS_FAIL;
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_ERROR;
                }
                // If time left to poll, read the ack ctrl again
                else
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                    ret = bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS,
                                                     cs40l25_timer_callback,
                                                     driver);
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_CAL_TIMER;
                }
            }
            break;
#endif // INCLUDE_CAL

        case CS40L25_POWER_DOWN_SM_STATE_MBOX_START:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count = 0;
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                ret = bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS,
                                                 cs40l25_timer_callback,
                                                 driver);
                sm->state = CS40L25_POWER_DOWN_SM_STATE_MBOX_TIMER;
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_MBOX_TIMER:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                // Read mbox reg to see if it has been reset
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS40L25_POWER_DOWN_SM_STATE_MBOX_READ;
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_MBOX_READ:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                // If ack ctrl has been reset
                if (driver->register_buffer == CS40L25_POWERCONTROL_NONE)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    // Read so we can update bits
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_CORE_CTRL;
                }
                // If polling period expired, indicate ERROR
                else if (sm->count >= CS40L25_POLL_ACK_CTRL_MAX)
                {
                    ret = CS40L25_STATUS_FAIL;
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_ERROR;
                }
                // If time left to poll, read the ack ctrl again
                else
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                    ret = bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS,
                                                     cs40l25_timer_callback,
                                                     driver);
                    sm->state = CS40L25_POWER_DOWN_SM_STATE_MBOX_TIMER;
                }
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_CORE_CTRL:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                uint32_t temp_reg = driver->register_buffer;
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                // Disable HALO DSP core
                temp_reg &= ~XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_EN_BITMASK;
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                             temp_reg,
                                                             false);
                sm->state = CS40L25_POWER_DOWN_SM_STATE_COMPLETE;
            }
            break;

        case CS40L25_POWER_DOWN_SM_STATE_COMPLETE:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->state = CS40L25_POWER_DOWN_SM_STATE_DONE;
            }
            break;


        case CS40L25_POWER_DOWN_SM_STATE_DONE:
            break;

        case CS40L25_POWER_DOWN_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_POWER_DOWN_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Configure State Machine
 *
 * Implementation of cs40l25_private_functions_t.configure_sm
 *
 */
static uint32_t cs40l25_configure_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);
    uint32_t total_regs = driver->state == CS40L25_STATE_DSP_STANDBY ? CS40L25_CONFIG_REGISTERS_TOTAL : CS40L25_CONFIG_REGISTERS_CODEC;

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_CONFIGURE_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS40L25_CONFIGURE_SM_STATE_INIT:
            sm->count = 0;
            CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
            // Unlock the register file
            ret = cs40l25_private_functions_g->write_reg(driver,
                                                         CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                         CS40L25_TEST_KEY_CTRL_UNLOCK_1,
                                                         false);
            sm->state = CS40L25_CONFIGURE_SM_STATE_UNLOCK_REGS;
            break;

        case CS40L25_CONFIGURE_SM_STATE_UNLOCK_REGS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                sm->count++;
                if (sm->count == 1)
                {
                    // Unlock the register file
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS40L25_TEST_KEY_CTRL_UNLOCK_2,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Read the first of the Configuration Registers
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                cs40l25_config_register_addresses[0],
                                                                &(driver->config_regs.words[0]),
                                                                false);
                    sm->state = CS40L25_CONFIGURE_SM_STATE_READ_REGS;
                }
            }
            break;

        case CS40L25_CONFIGURE_SM_STATE_READ_REGS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                sm->count++;
                // If there are remaining Configuration Registers to read
                if (sm->count < total_regs)
                {
                    // Read the next of the Configuration Registers
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                cs40l25_config_register_addresses[sm->count],
                                                                &(driver->config_regs.words[sm->count]),
                                                                false);
                }
                else
                {
                    // Apply audio_config to config_regs
                    ret = cs40l25_private_functions_g->apply_configs(driver);

                    if (ret == CS40L25_STATUS_OK)
                    {
                        // Write new value to first of the Configuration Registers
                        sm->count = 0;
                        ret = cs40l25_private_functions_g->write_reg(driver,
                                                                     cs40l25_config_register_addresses[0],
                                                                     driver->config_regs.words[0],
                                                                     false);
                        sm->state = CS40L25_CONFIGURE_SM_STATE_WRITE_REGS;
                    }
                }
            }
            break;

        case CS40L25_CONFIGURE_SM_STATE_WRITE_REGS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                sm->count++;
                // If there are remaining Configuration Registers to read
                if (sm->count < total_regs)
                {
                    // Write new value to next of the Configuration Registers
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_config_register_addresses[sm->count],
                                                                 driver->config_regs.words[sm->count],
                                                                 false);
                }
                else
                {
                    // Write IRQMASKSEQUENCE
                    sm->count = 0;
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_irqmaskseq_patch[sm->count],
                                                                 cs40l25_irqmaskseq_patch[sm->count + 1],
                                                                 false);
                    sm->state = CS40L25_CONFIGURE_SM_STATE_WRITE_IRQ;
                }
            }
            break;

        case CS40L25_CONFIGURE_SM_STATE_WRITE_IRQ:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                sm->count += 2;
                if (sm->count < (sizeof(cs40l25_irqmaskseq_patch) / sizeof(uint32_t)))
                {
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 cs40l25_irqmaskseq_patch[sm->count],
                                                                 cs40l25_irqmaskseq_patch[sm->count + 1],
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Re-lock the register file
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS40L25_TEST_KEY_CTRL_LOCK_1,
                                                                 false);
                    sm->state = CS40L25_CONFIGURE_SM_STATE_LOCK_REGS;
                }
            }
            break;

        case CS40L25_CONFIGURE_SM_STATE_LOCK_REGS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                sm->count++;

                if (sm->count == 1)
                {
                    // Re-lock the register file
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS40L25_TEST_KEY_CTRL_LOCK_2,
                                                                 false);
                }
                else
                {
                    sm->state = CS40L25_CONFIGURE_SM_STATE_DONE;
                }
            }
            break;

        case CS40L25_CONFIGURE_SM_STATE_DONE:
            break;

        case CS40L25_CONFIGURE_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_CONFIGURE_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Field Access State Machine
 *
 * Implementation of cs40l25_private_functions_t.field_access_sm
 *
 */
static uint32_t cs40l25_field_access_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_FIELD_ACCESS_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS40L25_FIELD_ACCESS_SM_STATE_INIT:
            CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

            // Read the value from the field address
            ret = cs40l25_private_functions_g->read_reg(driver,
                                                        driver->field_accessor.address,
                                                        &(driver->register_buffer),
                                                        false);
            sm->state = CS40L25_FIELD_ACCESS_SM_STATE_READ_MEM;
            break;

        case CS40L25_FIELD_ACCESS_SM_STATE_READ_MEM:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                // Create bit-wise mask of the bit-field
                uint32_t temp_mask = (~(0xFFFFFFFF << driver->field_accessor.size) << driver->field_accessor.shift);
                uint32_t reg_val = driver->register_buffer;
                // If this is only a GET request
                if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_IS_GET_REQUEST))
                {
                    uint32_t *reg_ptr = (uint32_t *) driver->current_request.arg;
                    // Mask off bit-field and shift down to LS-Bit
                    reg_val &= temp_mask;
                    reg_val >>= driver->field_accessor.shift;
                    *reg_ptr = reg_val;

                    sm->state = CS40L25_FIELD_ACCESS_SM_STATE_DONE;
                }
                else
                {
                    uint32_t field_val = (uint32_t) driver->current_request.arg;
                    // Shift new value to bit-field bit position
                    field_val <<= driver->field_accessor.shift;
                    field_val &= temp_mask;
                    // Mask off bit-field bit locations in memory's value
                    reg_val &= ~temp_mask;
                    // Add new value
                    reg_val |= field_val;

                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    // Write new register/memory value
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 driver->field_accessor.address,
                                                                 reg_val,
                                                                 false);

                    if (driver->field_accessor.ack_ctrl)
                        sm->state = CS40L25_FIELD_ACCESS_SM_STATE_ACK_START;
                    else
                        sm->state = CS40L25_FIELD_ACCESS_SM_STATE_WRITE_MEM;
                }
            }
            break;

        case CS40L25_FIELD_ACCESS_SM_STATE_WRITE_MEM:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->state = CS40L25_FIELD_ACCESS_SM_STATE_DONE;
            }
            break;

        case CS40L25_FIELD_ACCESS_SM_STATE_ACK_START:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count = 0;
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                ret = bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS,
                                                 cs40l25_timer_callback,
                                                 driver);
                sm->state = CS40L25_FIELD_ACCESS_SM_STATE_ACK_TIMER;
            }
            break;

        case CS40L25_FIELD_ACCESS_SM_STATE_ACK_TIMER:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                // Read the value from the field address
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            driver->field_accessor.address,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS40L25_FIELD_ACCESS_SM_STATE_ACK_READ;
            }
            break;

        case CS40L25_FIELD_ACCESS_SM_STATE_ACK_READ:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                // If ack ctrl has been reset
                if (driver->register_buffer == driver->field_accessor.ack_reset)
                {
                    sm->state = CS40L25_FIELD_ACCESS_SM_STATE_DONE;
                }
                // If polling period expired, indicate ERROR
                else if (sm->count >= CS40L25_POLL_ACK_CTRL_MAX)
                {
                    ret = CS40L25_STATUS_FAIL;
                    sm->state = CS40L25_FIELD_ACCESS_SM_STATE_ERROR;
                }
                // If time left to poll, read the ack ctrl again
                else
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                    ret = bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS,
                                                     cs40l25_timer_callback,
                                                     driver);
                    sm->state = CS40L25_FIELD_ACCESS_SM_STATE_ACK_TIMER;
                }
            }
            break;

        case CS40L25_FIELD_ACCESS_SM_STATE_DONE:
            break;

        case CS40L25_FIELD_ACCESS_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_FIELD_ACCESS_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Calibration State Machine
 *
 * Implementation of cs40l25_private_functions_t.calibration_sm
 *
 */
static uint32_t cs40l25_calibration_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);
    uint32_t calib_type = (uint32_t) driver->current_request.arg;

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_CALIBRATION_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS40L25_CALIBRATION_SM_STATE_INIT:
#ifdef INCLUDE_CAL
            if (calib_type & CS40L25_CALIB_ALL)
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            CS40L25_INTP_AMP_CTRL_REG,
                                                            &(driver->register_buffer),
                                                            false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_GET_VOL;
            }
            else
            {
                sm->state = CS40L25_CALIBRATION_SM_STATE_DONE;
            }
#else
            sm->state = CS40L25_CALIBRATION_SM_STATE_DONE;
#endif // !INCLUDE_CAL

            break;

#ifdef INCLUDE_CAL
        case CS40L25_CALIBRATION_SM_STATE_GET_VOL:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                uint32_t temp_mask = (~(0xFFFFFFFF << CS40L25_INTP_AMP_CTRL_AMP_VOL_PCM_BITWIDTH) << CS40L25_INTP_AMP_CTRL_AMP_VOL_PCM_BITOFFSET);

                driver->calib_pcm_vol = driver->register_buffer;

                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             CS40L25_INTP_AMP_CTRL_REG,
                                                             driver->register_buffer & ~temp_mask,
                                                             false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_SET_VOL;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_SET_VOL:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                if (calib_type & CS40L25_CALIB_F0)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_CAL_MAXBACKEMF,
                                                                 0,
                                                                 false);

                    sm->state = CS40L25_CALIBRATION_SM_STATE_SET_MAXBEMF;
                }
                else if (calib_type & CS40L25_CALIB_QEST)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_CAL_F0_TRACKING_ENABLE,
                                                                 2,
                                                                 false);

                    sm->state = CS40L25_CALIBRATION_SM_STATE_SET_F0_TRACK_2;
                }
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_SET_MAXBEMF:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             CS40L25_CAL_CLOSED_LOOP,
                                                             0,
                                                             false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_CLEAR_CLOSED_LOOP;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_CLEAR_CLOSED_LOOP:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             CS40L25_CAL_F0_TRACKING_ENABLE,
                                                             1,
                                                             false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_SET_F0_TRACK_1;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_SET_F0_TRACK_1:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                ret = bsp_driver_if_g->set_timer(500, cs40l25_timer_callback, driver);

                sm->state = CS40L25_CALIBRATION_SM_STATE_WAIT_500MS;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_WAIT_500MS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             CS40L25_CAL_CLOSED_LOOP,
                                                             1,
                                                             false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_SET_CLOSED_LOOP;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_SET_CLOSED_LOOP:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2S, cs40l25_timer_callback, driver);

                sm->state = CS40L25_CALIBRATION_SM_STATE_WAIT_2S;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_WAIT_2S:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             CS40L25_CAL_F0_TRACKING_ENABLE,
                                                             0,
                                                             false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_CLEAR_F0_TRACK;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_CLEAR_F0_TRACK:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            CS40L25_CAL_F0,
                                                            &(driver->register_buffer),
                                                            false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_READ_F0;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_READ_F0:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                driver->cal_data.f0 = driver->register_buffer;

                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            CS40L25_CAL_REDC,
                                                            &(driver->register_buffer),
                                                            false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_READ_REDC;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_READ_REDC:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                driver->cal_data.redc = driver->register_buffer;

                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            CS40L25_CAL_MAXBACKEMF,
                                                            &(driver->register_buffer),
                                                            false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_READ_MAXBEMF;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_READ_MAXBEMF:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                driver->cal_data.backemf = driver->register_buffer;
                driver->cal_data.is_valid_f0 = true;

                if (calib_type & CS40L25_CALIB_QEST) {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_CAL_F0_TRACKING_ENABLE,
                                                                 2,
                                                                 false);

                    sm->state = CS40L25_CALIBRATION_SM_STATE_SET_F0_TRACK_2;
                }
                else
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_INTP_AMP_CTRL_REG,
                                                                 driver->calib_pcm_vol,
                                                                 false);

                    sm->state = CS40L25_CALIBRATION_SM_STATE_RESTORE_VOL;
                }
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_SET_F0_TRACK_2:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count = 0;
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                ret = bsp_driver_if_g->set_timer(100, cs40l25_timer_callback, driver);

                sm->state = CS40L25_CALIBRATION_SM_STATE_READ_F0_TRACK_T;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_READ_F0_TRACK_T:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            CS40L25_CAL_F0_TRACKING_ENABLE,
                                                            &(driver->register_buffer),
                                                            false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_READ_F0_TRACK;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_READ_F0_TRACK:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                if (driver->register_buffer == 0)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                CS40L25_CAL_Q_EST,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS40L25_CALIBRATION_SM_STATE_READ_QEST;
                }
                else if (sm->count >= 30)
                {
                    sm->state = CS40L25_POWER_UP_SM_STATE_ERROR;
                    ret = CS40L25_STATUS_FAIL;
                }
                else
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                    ret = bsp_driver_if_g->set_timer(100, cs40l25_timer_callback, driver);

                    sm->state = CS40L25_CALIBRATION_SM_STATE_READ_F0_TRACK_T;
                }
            }
            break;

        case CS40L25_CALIBRATION_SM_STATE_READ_QEST:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                driver->cal_data.qest = driver->register_buffer;
                driver->cal_data.is_valid_qest = true;

                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             CS40L25_INTP_AMP_CTRL_REG,
                                                             driver->calib_pcm_vol,
                                                             false);

                sm->state = CS40L25_CALIBRATION_SM_STATE_RESTORE_VOL;
            }

            break;

        case CS40L25_CALIBRATION_SM_STATE_RESTORE_VOL:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->state = CS40L25_CALIBRATION_SM_STATE_DONE;
            }

            break;
#endif // INCLUDE_CAL

        case CS40L25_CALIBRATION_SM_STATE_DONE:
            break;

        case CS40L25_CALIBRATION_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_CALIBRATION_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Dynamic ReDC State Machine
 *
 * Implementation of cs40l25_private_functions_t.dynamic_redc_sm
 *
 */
static uint32_t cs40l25_dynamic_redc_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_DYNAMIC_REDC_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS40L25_DYNAMIC_REDC_SM_STATE_INIT:
            CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
            sm->count = 0;

            // The driver will set the dynamic_redc control to -1 (0xFFFFFF)
            ret = cs40l25_private_functions_g->write_reg(driver,
                                                         CS40L25_DYNAMIC_REDC,
                                                         0xFFFFFFFF,
                                                         false);
            sm->state = CS40L25_DYNAMIC_REDC_SM_STATE_SET_REDC_WAKE;
            break;

        case CS40L25_DYNAMIC_REDC_SM_STATE_SET_REDC_WAKE:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    sm->count++;

                    // The PowerControl (MBOX_4) register must be set to WAKEUP (2)
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                                                 CS40L25_POWERCONTROL_WAKEUP,
                                                                 false);
                }
                else
                {
                    sm->count = 0;

                    // The dynamic_redc register contents will remain at -1 until the calculation is complete
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                CS40L25_DYNAMIC_REDC,
                                                                &(driver->register_buffer),
                                                                false);

                    sm->state = CS40L25_DYNAMIC_REDC_SM_STATE_GET_REDC;
                }
            }
            break;

        case CS40L25_DYNAMIC_REDC_SM_STATE_GET_REDC:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                if (driver->register_buffer == 0xFFFFFF)
                {
                    if (sm->count < 30)
                    {
                        sm->count++;

                        CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                        ret = bsp_driver_if_g->set_timer(CS40L25_POLL_ACK_CTRL_MS,
                                                         cs40l25_timer_callback,
                                                         driver);
                        sm->state = CS40L25_DYNAMIC_REDC_SM_STATE_WAIT_10MS;
                    }
                    else
                    {
                        sm->state = CS40L25_DYNAMIC_REDC_SM_STATE_ERROR;
                    }
                }
                else
                {
                    uint32_t *reg_ptr = (uint32_t *) driver->current_request.arg;
                    *reg_ptr = driver->register_buffer;
                    sm->state = CS40L25_DYNAMIC_REDC_SM_STATE_DONE;
                }
            }
            break;

        case CS40L25_DYNAMIC_REDC_SM_STATE_WAIT_10MS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            CS40L25_DYNAMIC_REDC,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS40L25_DYNAMIC_REDC_SM_STATE_GET_REDC;
            }
            break;

        case CS40L25_DYNAMIC_REDC_SM_STATE_DONE:
            break;

        case CS40L25_DYNAMIC_REDC_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_DYNAMIC_REDC_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Dynamic F0 State Machine
 *
 * Implementation of cs40l25_private_functions_t.dynamic_f0_sm
 *
 */
static uint32_t cs40l25_dynamic_f0_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_DYNAMIC_F0_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS40L25_DYNAMIC_F0_SM_STATE_INIT:
            CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
            sm->count = 0;

            ret = cs40l25_private_functions_g->read_reg(driver,
                                                        CS40L25_DYN_F0_TABLE,
                                                        &(driver->register_buffer),
                                                        false);
            sm->state = CS40L25_DYNAMIC_F0_SM_STATE_READ_F0;
            break;

        case CS40L25_DYNAMIC_F0_SM_STATE_READ_F0:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                cs40l25_dynamic_f0_table_entry_t *f0_req;
                cs40l25_dynamic_f0_table_entry_t f0_read;
                f0_req = (cs40l25_dynamic_f0_table_entry_t*) (driver->current_request.arg);
                f0_read.word = driver->register_buffer;

                if (f0_req->index == f0_read.index)
                {
                    f0_req->f0 = f0_read.f0;
                    sm->state = CS40L25_DYNAMIC_F0_SM_STATE_DONE;
                }
                else if (sm->count < 20)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    sm->count++;

                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                (CS40L25_DYN_F0_TABLE + (sm->count * 4)),
                                                                &(driver->register_buffer),
                                                                false);
                }
                else
                {
                    // Set to default of table entry to indicate index not found
                    f0_req->word = 0x007FE000;
                    sm->state = CS40L25_DYNAMIC_F0_SM_STATE_DONE;
                }
            }
            break;

        case CS40L25_DYNAMIC_F0_SM_STATE_DONE:
            break;

        case CS40L25_DYNAMIC_F0_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_DYNAMIC_F0_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Get DSP Status State Machine
 *
 * Implementation of cs40l25_private_functions_t.get_dsp_status_sm
 *
 */
static uint32_t cs40l25_get_dsp_status_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);
    // Get pointer to status passed in to Control Request
    cs40l25_dsp_status_t *status = (cs40l25_dsp_status_t *) driver->current_request.arg;

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_GET_DSP_STATUS_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS40L25_GET_DSP_STATUS_SM_STATE_INIT:
            sm->count = 0;
            CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

            // Read the first DSP Status field address
            ret = cs40l25_private_functions_g->read_reg(driver,
                                                        cs40l25_dsp_status_addresses[0],
                                                        &(driver->register_buffer),
                                                        false);

            sm->state = CS40L25_GET_DSP_STATUS_SM_STATE_READ_STATUSES_1;
            break;

        case CS40L25_GET_DSP_STATUS_SM_STATE_READ_STATUSES_1:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                status->data.words[sm->count] = driver->register_buffer;
                sm->count++;
                // If there are remaining DSP Status fields to read
                if (sm->count < CS40L25_DSP_STATUS_WORDS_TOTAL)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    // Read the next DSP Status field address
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                cs40l25_dsp_status_addresses[sm->count],
                                                                &(driver->register_buffer),
                                                                false);
                }
                else
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);

                    // Wait at least 10ms
                    ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_10MS, cs40l25_timer_callback, driver);

                    sm->state = CS40L25_GET_DSP_STATUS_SM_STATE_WAIT;
                }
            }
            break;

        case CS40L25_GET_DSP_STATUS_SM_STATE_WAIT:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                sm->count = 0;
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                // Read the first DSP Status field address
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            cs40l25_dsp_status_addresses[0],
                                                            &(driver->register_buffer),
                                                            false);

                sm->state = CS40L25_GET_DSP_STATUS_SM_STATE_READ_STATUSES_2;
            }
            break;

        case CS40L25_GET_DSP_STATUS_SM_STATE_READ_STATUSES_2:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                // If the current field is HALO_HEARTBEAT, and there is a change in subsequent values
                if ((sm->count == 1) && (driver->register_buffer != status->data.words[sm->count]))
                {
                    status->is_hb_inc = true;
                }

                // If the current field is CSPL_TEMPERATURE, and there is a change in subsequent values
                if ((sm->count == 8) && (driver->register_buffer != status->data.words[sm->count]))
                {
                    status->is_temp_changed = true;
                }

                status->data.words[sm->count] = driver->register_buffer;

                sm->count++;

                // If there are remaining DSP Statuses to read
                if (sm->count < CS40L25_DSP_STATUS_WORDS_TOTAL)
                {
                    // Read the next DSP Status field address
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                cs40l25_dsp_status_addresses[sm->count],
                                                                &(driver->register_buffer),
                                                                false);
                }
                else
                {
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

                    sm->state = CS40L25_GET_DSP_STATUS_SM_STATE_DONE;
                }
            }
            break;

        case CS40L25_GET_DSP_STATUS_SM_STATE_DONE:
            break;

        case CS40L25_GET_DSP_STATUS_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_GET_DSP_STATUS_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Hibernate State Machine
 *
 * Implementation of cs40l25_private_functions_t.hibernate_sm
 *
 */
static uint32_t cs40l25_hibernate_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_HIBERNATE_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS40L25_HIBERNATE_SM_STATE_INIT:
            // Only if in AoH Standby
            if (driver->state == CS40L25_STATE_DSP_POWER_UP)
            {
                sm->count = 0;
                // Find first changed wseq entry and write it, if no changed entries
                // transition directly to sending hibernate command
                int found_entry = 0;
                while (sm->count < driver->wseq_num_entries)
                {
                    if (driver->wseq_table[sm->count].changed == 1)
                    {
                        //Write 16bit address and 32bit value to poweronsequence
                        cs40l25_private_functions_g->cp_bulk_write(driver,
                                                                   CS40L25_POWERONSEQUENCE + (8 * sm->count),
                                                                   (uint8_t *) driver->wseq_table[sm->count].words,
                                                                   8);

                        driver->wseq_table[sm->count].changed = 0;
                        sm->count++;
                        sm->state = CS40L25_HIBERNATE_SM_STATE_WSEQ;
                        found_entry = 1;
                        break;
                    }
                    sm->count++;
                }
                if (found_entry == 0)
                {
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_POWERONSEQUENCE + (8 * sm->count),
                                                                 0x00FFFFFF,
                                                                 false);
                    sm->state = CS40L25_HIBERNATE_SM_STATE_WRITE_TERMINATOR;
                }

            }
            else
            {
                sm->state = CS40L25_HIBERNATE_SM_STATE_ERROR;
                ret = CS40L25_STATUS_FAIL;
            }
            break;
        case CS40L25_HIBERNATE_SM_STATE_WSEQ:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                int found_entry = 0;
                while (sm->count < driver->wseq_num_entries)
                {
                    if (driver->wseq_table[sm->count].changed == 1)
                    {
                        //Write 32bit address and 32bit value to poweronsequence
                        cs40l25_private_functions_g->cp_bulk_write(driver,
                                                                   CS40L25_POWERONSEQUENCE + (8 * sm->count),
                                                                   (uint8_t *) driver->wseq_table[sm->count].words,
                                                                   8);

                        driver->wseq_table[sm->count].changed = 0;
                        sm->count++;
                        found_entry = 1;
                        break;
                    }
                    sm->count++;
                }
                if (found_entry == 0)
                {
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 CS40L25_POWERONSEQUENCE + (8 * sm->count),
                                                                 0x00FFFFFF,
                                                                 false);
                    sm->state = CS40L25_HIBERNATE_SM_STATE_WRITE_TERMINATOR;
                }
            }
            break;

        case CS40L25_HIBERNATE_SM_STATE_WRITE_TERMINATOR:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                                             CS40L25_POWERCONTROL_HIBERNATE,
                                                             false);
                sm->state = CS40L25_HIBERNATE_SM_STATE_SEND_CMD;
            }
            break;

        case CS40L25_HIBERNATE_SM_STATE_SEND_CMD:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->state = CS40L25_HIBERNATE_SM_STATE_DONE;
            }
            break;

        case CS40L25_HIBERNATE_SM_STATE_DONE:
            break;

        case CS40L25_HIBERNATE_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_HIBERNATE_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Wake State Machine
 *
 * Implementation of cs40l25_private_functions_t.wake_sm
 *
 */
static uint32_t cs40l25_wake_sm(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_sm_t *sm = &(driver->control_sm);

    if ((CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR)) &&
        ((sm->state != CS40L25_WAKE_SM_STATE_SEND_WAKE) && (sm->state != CS40L25_WAKE_SM_STATE_SEND_HIBERNATE)))
    {
        sm->state = CS40L25_WAKE_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch (sm->state)
    {
        case CS40L25_WAKE_SM_STATE_INIT:
            // Only if in AoH Hibernate
            if (driver->state == CS40L25_STATE_HIBERNATE)
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                // Request Wake
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                                             CS40L25_POWERCONTROL_WAKEUP,
                                                             false);
                sm->count = 0;
                sm->state = CS40L25_WAKE_SM_STATE_SEND_WAKE;
            }
            else
            {
                sm->state = CS40L25_HIBERNATE_SM_STATE_ERROR;
                ret = CS40L25_STATUS_FAIL;
            }

            break;

        case CS40L25_WAKE_SM_STATE_SEND_WAKE:
            // Check for successful control port write
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                // Set the inner counter to 0
                sm->count &= 0xF0;

                // Wait for at least 5ms
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_5MS, cs40l25_timer_callback, driver);
                sm->state = CS40L25_WAKE_SM_STATE_WAKE_OK_WAIT_5MS;
            }
            // Check for control port write error, indicating possible wake from control port
            else if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_ERROR);

                sm->count++;
                // If less than 10 tries to wake, wait 1ms and then try again
                if ((sm->count & 0x0F) < 10)
                {
                    // Wait for at least 1ms
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                    ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_1MS, cs40l25_timer_callback, driver);
                    sm->state = CS40L25_WAKE_SM_STATE_WAKE_FAILED_WAIT_1MS;
                }
                else
                {
                    sm->state = CS40L25_WAKE_SM_STATE_ERROR;
                }
            }
            break;

        case CS40L25_WAKE_SM_STATE_WAKE_FAILED_WAIT_1MS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                // Request Wake
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                                             CS40L25_POWERCONTROL_WAKEUP,
                                                             false);
                sm->state = CS40L25_WAKE_SM_STATE_SEND_WAKE;
            }
            break;

        case CS40L25_WAKE_SM_STATE_WAKE_OK_WAIT_5MS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                // Read FW ID
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            CS40L25_FIRMWARE_ID_ADDR,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS40L25_WAKE_SM_STATE_READ_FW_ID;
            }
            break;

        case CS40L25_WAKE_SM_STATE_READ_FW_ID:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                // Increment the inner loop counter to read FW ID
                sm->count++;

                // Check if FW ID is correct
                if (driver->register_buffer == CS40L25_FIRMWARE_ID)
                {
                    sm->count = 0;
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                    // Read POWERSTATE
                    ret = cs40l25_private_functions_g->read_reg(driver,
                                                                CS40L25_POWERSTATE,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS40L25_WAKE_SM_STATE_READ_POWER_STATE;
                }
                // If less than 10 tries to read FW ID, wait 5ms and then try again
                else if ((sm->count & 0x0F) < 10)
                {
                    // Wait for at least 5ms
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                    ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_5MS, cs40l25_timer_callback, driver);
                    sm->state = CS40L25_WAKE_SM_STATE_WAKE_OK_WAIT_5MS;
                }
                // If FW ID was incorrect 10 times, force back into hibernate
                else
                {
                    // Increment outer loop counter - force hibernate attempts
                    sm->count += (1 << 4);

                    // If less than 10 force hibernate attemps, force once more
                    if (sm->count < (10 << 4))
                    {
                        CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                        // Request Hibernate manually (not via HALO MBOX)
                        ret = cs40l25_private_functions_g->write_reg(driver,
                                                                     CS40L25_PWRMGT_CTL_REG,
                                                                     CS40L25_PWRMGT_CTL_MEM_RDY_TRIG_HIBER,
                                                                     false);
                        sm->state = CS40L25_WAKE_SM_STATE_SEND_HIBERNATE;
                    }
                    else
                    {
                        sm->state = CS40L25_WAKE_SM_STATE_ERROR;
                    }
                }
            }
            break;

        case CS40L25_WAKE_SM_STATE_SEND_HIBERNATE:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                // Wait for at least 1ms
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_1MS, cs40l25_timer_callback, driver);
                sm->state = CS40L25_WAKE_SM_STATE_HIBERNATE_WAIT_1MS;
            }
            break;

        case CS40L25_WAKE_SM_STATE_HIBERNATE_WAIT_1MS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                // Request Wake
                ret = cs40l25_private_functions_g->write_reg(driver,
                                                             DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                                             CS40L25_POWERCONTROL_WAKEUP,
                                                             false);
                // Clear lower nibble of 'count' used for inner loop to send Wake command
                sm->count &= 0xF0;
                sm->state = CS40L25_WAKE_SM_STATE_SEND_WAKE;
            }
            break;

        case CS40L25_WAKE_SM_STATE_READ_POWER_STATE:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                if ((driver->register_buffer == CS40L25_POWERSTATE_ACTIVE) ||
                    (driver->register_buffer == CS40L25_POWERSTATE_STANDBY))
                {
                    sm->state = CS40L25_WAKE_SM_STATE_DONE;
                }
                else if ((driver->register_buffer == CS40L25_POWERSTATE_HIBERNATE) &&
                         (sm->count < 10))
                {
                    // Wait for at least 5ms
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_TIMEOUT);
                    ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_5MS, cs40l25_timer_callback, driver);
                    sm->state = CS40L25_WAKE_SM_STATE_STILL_HIBERNATE_WAIT_5MS;
                }
                else
                {
                    sm->state = CS40L25_WAKE_SM_STATE_ERROR;
                }
            }
            break;

        case CS40L25_WAKE_SM_STATE_STILL_HIBERNATE_WAIT_5MS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_TIMEOUT))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                // Read POWERSTATE
                ret = cs40l25_private_functions_g->read_reg(driver,
                                                            CS40L25_POWERSTATE,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS40L25_WAKE_SM_STATE_READ_POWER_STATE;
            }
            break;

        case CS40L25_WAKE_SM_STATE_DONE:
            break;

        case CS40L25_WAKE_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_WAKE_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Event Handler State Machine
 *
 * Implementation of cs40l25_private_functions_t.event_sm
 *
 */
static uint32_t cs40l25_event_sm(void *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    cs40l25_t *d = driver;
    cs40l25_sm_t *sm = &(d->event_sm);

    if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS40L25_EVENT_SM_STATE_ERROR;
        ret = CS40L25_STATUS_FAIL;
    }

    switch (sm->state)
    {
        case CS40L25_EVENT_SM_STATE_INIT:
        {
            /*
             * Since upon entering the Event Handler SM, the BSP Control Port may be in the middle of a transaction,
             * request the BSP to reset the Control Port and abort the current transaction.
             */

            bool was_i2c_busy = false;
            bsp_driver_if_g->i2c_reset(d->bsp_dev_id, &was_i2c_busy);

            // If an I2C transaction was interrupted, then current Control Request must be restarted
            if (was_i2c_busy)
            {
                CS40L25_SET_FLAG(d->control_sm.flags, CS40L25_FLAGS_REQUEST_RESTART);
            }

            d->event_flags = 0;
            CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
            sm->count = 0;
            // Read the first IRQ1 flag register
            ret = cs40l25_private_functions_g->read_reg(d,
                                                        XM_UNPACKED24_DSP1_SCRATCH_REG,
                                                        &d->register_buffer,
                                                        false);
            sm->state = CS40L25_EVENT_SM_STATE_READ_SCRATCH;
            break;
        }

        case CS40L25_EVENT_SM_STATE_READ_SCRATCH:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                // If SCRATCH is nonzero OR If no events are unmasked
                uint32_t temp_event_control = d->config_regs.event_control.reg.halo_word;
                if ((d->register_buffer) || (!temp_event_control))
                {
                    // Call BSP Notification Callback
                    if (d->notification_cb != NULL)
                    {
                        uint32_t temp_event_flags = 0;
                        if (d->register_buffer)
                        {
                            temp_event_flags = CS40L25_EVENT_FLAG_DSP_ERROR;
                        }

                        d->notification_cb(temp_event_flags, d->notification_cb_arg);
                    }
                    sm->state = CS40L25_EVENT_SM_STATE_DONE;
                }
                else
                {
                    sm->count = 0;

                    // Read unmasked event register
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    ret = cs40l25_private_functions_g->read_reg(d,
                                                                cs40l2x_event_regs[sm->count],
                                                                &d->register_buffer,
                                                                false);
                    sm->state = CS40L25_EVENT_SM_STATE_READ_EVENT;
                }
            }
            break;

        case CS40L25_EVENT_SM_STATE_READ_EVENT:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                // If Event is not trigger source or we don't care about this source, read next Event register
                uint32_t temp_event_control = d->config_regs.event_control.reg.halo_word;
                if ((d->register_buffer == CS40L2X_EVENT_CTRL_NONE) ||
                    ((temp_event_control & cs40l2x_event_masks[sm->count]) == 0))
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                    sm->count++;
                    // If there are any more event sources to poll
                    if (sm->count < CS40L25_EVENT_SOURCES)
                    {
                        // Read unmasked event register
                        ret = cs40l25_private_functions_g->read_reg(d,
                                                                    cs40l2x_event_regs[sm->count],
                                                                    &d->register_buffer,
                                                                    false);
                    }
                    else
                    {
                        // Write WAKE to POWERCONTROL register
                        /*
                         * polling for acknowledgment as with other mailbox registers
                         * is unnecessary in this case and adds latency, so only send
                         * the wake-up command to complete the notification sequence
                         */
                        ret = cs40l25_private_functions_g->write_reg(driver,
                                                                     DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                                                     CS40L25_POWERCONTROL_WAKEUP,
                                                                     false);
                        sm->state = CS40L25_EVENT_SM_STATE_WRITE_WAKE;
                    }
                }
                // If event is HW Event, then process separately
                else if (d->register_buffer == CS40L2X_EVENT_CTRL_HARDWARE)
                {
                    // Read the first IRQ2 flag register
                    ret = cs40l25_private_functions_g->read_reg(d,
                                                                IRQ2_IRQ2_EINT_1_REG,
                                                                &d->register_buffer,
                                                                false);

                    // Since the CS40L25_HARDWAREEVENT is first in the list, it will be presumed that sm->count = 0 here
                    sm->state = CS40L25_EVENT_SM_STATE_READ_IRQ_STATUS;
                }
                // Else set event flag and clear event source
                else if (d->register_buffer <= CS40L2X_EVENT_CTRL_TRIG_RESM)
                {
                    // Set correct bit in flags to send to the BSP Notification Callback
                    d->event_flags |= cs40l25_event_value_to_flag_map[d->register_buffer];

                    // Write EVENT_CTRL_NONE to the triggered event register
                    ret = cs40l25_private_functions_g->write_reg(d,
                                                                 cs40l2x_event_regs[sm->count],
                                                                 CS40L2X_EVENT_CTRL_NONE,
                                                                 false);
                    sm->state = CS40L25_EVENT_SM_STATE_CLEAR_EVENT;
                }
                else
                {
                    sm->state = CS40L25_EVENT_SM_STATE_ERROR;
                }
            }
            break;

        case CS40L25_EVENT_SM_STATE_CLEAR_EVENT:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                // If there are any more event sources to poll
                if (sm->count < CS40L25_EVENT_SOURCES)
                {
                    // Read unmasked event register
                    ret = cs40l25_private_functions_g->read_reg(d,
                                                                cs40l2x_event_regs[sm->count],
                                                                &d->register_buffer,
                                                                false);
                    sm->state = CS40L25_EVENT_SM_STATE_READ_EVENT;
                }
                else
                {
                    // Write WAKE to POWERCONTROL register
                    /*
                     * polling for acknowledgment as with other mailbox registers
                     * is unnecessary in this case and adds latency, so only send
                     * the wake-up command to complete the notification sequence
                     */
                    ret = cs40l25_private_functions_g->write_reg(driver,
                                                                 DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_4_REG,
                                                                 CS40L25_POWERCONTROL_WAKEUP,
                                                                 false);
                    sm->state = CS40L25_EVENT_SM_STATE_WRITE_WAKE;
                }
            }
            break;

        case CS40L25_EVENT_SM_STATE_READ_IRQ_STATUS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                // Set flags in event_flags
                for (uint8_t i = 0; i < (CS40L25_EVENT_HW_SOURCES * 2); i += 2)
                {
                    if (d->register_buffer & cs40l25_irq2_mask_1_to_event_flag_map[i])
                    {
                        d->event_flags |= cs40l25_irq2_mask_1_to_event_flag_map[i + 1];
                    }
                }

                // Clear any IRQ2 flags from first register
                d->register_buffer &= ~CS40L25_INT2_MASK_DEFAULT;
                ret = cs40l25_private_functions_g->write_reg(d,
                                                             IRQ2_IRQ2_EINT_1_REG,
                                                             d->register_buffer,
                                                             false);

                sm->state = CS40L25_EVENT_SM_STATE_CLEAR_IRQ_FLAGS;
            }
            break;

        case CS40L25_EVENT_SM_STATE_CLEAR_IRQ_FLAGS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                sm->count = 0;

                // If there are Boost-related Errors, proceed to DISABLE_BOOST
                if (d->event_flags & CS40L25_EVENT_FLAGS_BOOST_CYCLE)
                {
                    // Read which MSM Blocks are enabled
                    ret = cs40l25_private_functions_g->read_reg(d,
                                                                MSM_BLOCK_ENABLES_REG,
                                                                &d->register_buffer,
                                                                false);
                    sm->state = CS40L25_EVENT_SM_STATE_DISABLE_BOOST;
                }
                // IF there are no Boost-related Errors, proceed to TOGGLE_ERR_RLS
                else
                {
                    // Clear the Error Release register
                    ret = cs40l25_private_functions_g->write_reg(d,
                                                                 MSM_ERROR_RELEASE_REG,
                                                                 0,
                                                                 false);
                    sm->state = CS40L25_EVENT_SM_STATE_TOGGLE_ERR_RLS;
                }
            }
            break;

        case CS40L25_EVENT_SM_STATE_DISABLE_BOOST:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    sm->count++;
                    // Disable Boost converter
                    d->register_buffer &= ~(MSM_BLOCK_ENABLES_BST_EN_BITMASK);
                    ret = cs40l25_private_functions_g->write_reg(d,
                                                                 MSM_BLOCK_ENABLES_REG,
                                                                 d->register_buffer,
                                                                 false);
                }
                else
                {
                    sm->count = 0;

                    // Clear the Error Release register
                    ret = cs40l25_private_functions_g->write_reg(d,
                                                                 MSM_ERROR_RELEASE_REG,
                                                                 0,
                                                                 false);

                    sm->state = CS40L25_EVENT_SM_STATE_TOGGLE_ERR_RLS;
                }
            }
            break;

        case CS40L25_EVENT_SM_STATE_TOGGLE_ERR_RLS:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);

                if (sm->count == 0)
                {
                    sm->count++;
                    // Set the Error Release register
                    ret = cs40l25_private_functions_g->write_reg(d,
                                                                 MSM_ERROR_RELEASE_REG,
                                                                 CS40L25_ERR_RLS_SPEAKER_SAFE_MODE_MASK,
                                                                 false);
                }
                else if (sm->count == 1)
                {
                    sm->count++;
                    // Clear the Error Release register
                    ret = cs40l25_private_functions_g->write_reg(d,
                                                                 MSM_ERROR_RELEASE_REG,
                                                                 0,
                                                                 false);
                }
                else
                {
                    sm->count = 0;

                    // If there are Boost-related Errors, re-enable Boost
                    if (d->event_flags & CS40L25_EVENT_FLAGS_BOOST_CYCLE)
                    {
                        // Read register containing BST_EN
                        ret = cs40l25_private_functions_g->read_reg(d,
                                                                    MSM_BLOCK_ENABLES_REG,
                                                                    &d->register_buffer,
                                                                    false);
                        sm->state = CS40L25_EVENT_SM_STATE_ENABLE_BOOST;
                    }
                    else
                    {
                        // Write EVENT_CTRL_NONE to the triggered event register
                        sm->count = 0;
                        ret = cs40l25_private_functions_g->write_reg(d,
                                                                     cs40l2x_event_regs[sm->count],
                                                                     CS40L2X_EVENT_CTRL_NONE,
                                                                     false);
                        sm->state = CS40L25_EVENT_SM_STATE_CLEAR_EVENT;
                    }
                }
            }
            break;

        case CS40L25_EVENT_SM_STATE_ENABLE_BOOST:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                if (sm->count == 0)
                {
                    CS40L25_CLEAR_FLAG(sm->flags, CS40L25_FLAGS_CP_RW_DONE);
                    sm->count++;
                    // Re-enable Boost Converter
                    d->register_buffer |= MSM_BLOCK_ENABLES_BST_EN_BITMASK;
                    ret = cs40l25_private_functions_g->write_reg(d,
                                                                 MSM_BLOCK_ENABLES_REG,
                                                                 d->register_buffer,
                                                                 false);
                }
                else
                {
                    // Write EVENT_CTRL_NONE to the triggered event register
                    sm->count = 0;
                    ret = cs40l25_private_functions_g->write_reg(d,
                                                                 cs40l2x_event_regs[sm->count],
                                                                 CS40L2X_EVENT_CTRL_NONE,
                                                                 false);
                    sm->state = CS40L25_EVENT_SM_STATE_CLEAR_EVENT;
                }
            }
            break;

        case CS40L25_EVENT_SM_STATE_WRITE_WAKE:
            if (CS40L25_IS_FLAG_SET(sm->flags, CS40L25_FLAGS_CP_RW_DONE))
            {
                // Call BSP Notification Callback
                if (d->notification_cb != NULL)
                {
                    d->notification_cb(d->event_flags, d->notification_cb_arg);
                }
                sm->state = CS40L25_EVENT_SM_STATE_DONE;
            }
            break;

        case CS40L25_EVENT_SM_STATE_DONE:
            break;

        case CS40L25_EVENT_SM_STATE_ERROR:
        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    if (ret == CS40L25_STATUS_FAIL)
    {
        sm->state = CS40L25_EVENT_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Gets pointer to correct errata based on DEVID/REVID
 *
 * Implementation of cs40l25_private_functions_t.get_errata
 *
 */
static uint32_t cs40l25_get_errata(uint32_t devid, uint32_t revid, const uint32_t **errata)
{
    uint32_t ret = CS40L25_STATUS_FAIL;

    // CS40L25 and CS40L25B Rev B1 are supported
    if (((devid == CS40L25_DEVID) || (devid == CS40L25B_DEVID)) && (revid == CS40L25_REVID_B1))
    {
        ret = CS40L25_STATUS_OK;

        *errata = cs40l25_revb0_errata_patch;
    }

    return ret;
}

/**
 * Reads contents from a consecutive number of memory addresses
 *
 * Implementation of cs40l25_private_functions_t.cp_bulk_read
 *
 */
static uint32_t cs40l25_cp_bulk_read(cs40l25_t *driver, uint32_t addr, uint32_t length)
{
    uint32_t ret = CS40L25_STATUS_FAIL;

    // Check that 'length' does not exceed the size of the BSP buffer
    if (length <= CS40L25_CP_BULK_READ_LENGTH_BYTES)
    {
        uint32_t bsp_status;

        /*
         * Switch from Little-Endian contents of uint32_t 'addr' to Big-Endian format required for Control Port
         * transaction.  Since register address is first written, cp_write_buffer[] is filled with register address.
         *
         * FIXME: This is not platform independent.
         */
        driver->cp_write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
        driver->cp_write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
        driver->cp_write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
        driver->cp_write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

        /*
         * Start reading contents into the BSP buffer starting at byte offset 4 - bytes 0-3 are reserved for calls to
         * cs40l25_read_reg.
         */
        bsp_status = bsp_driver_if_g->i2c_read_repeated_start(driver->bsp_dev_id,
                                                              driver->cp_write_buffer,
                                                              4,
                                                              (driver->cp_read_buffer + \
                                                                       CS40L25_CP_REG_READ_LENGTH_BYTES),
                                                              (length * 4),
                                                              cs40l25_private_functions_g->cp_read_callback,
                                                              driver);
        if (bsp_status == BSP_STATUS_OK)
        {
            ret = CS40L25_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Validates the boot configuration provided by the BSP.
 *
 * Implementation of cs40l25_private_functions_t.validate_boot_config
 *
 */
static uint32_t cs40l25_validate_boot_config(cs40l25_boot_config_t *config, bool is_fw_boot, bool is_coeff_boot, bool is_cal_boot)
{
    uint32_t ret = CS40L25_STATUS_FAIL;

    // Only check config if at least one of FW, COEFF or CAL boot are set
    if ((!is_fw_boot) && (!is_coeff_boot) && (!is_cal_boot))
    {
        ret = CS40L25_STATUS_OK;
    }
    // Check that 'config' is not NULL
    else if (config != NULL)
    {
        ret = CS40L25_STATUS_BOOT_REQUEST;

        // Calibration and fw boot are mutually exclusive
        if (is_cal_boot && is_fw_boot)
        {
            ret = CS40L25_STATUS_FAIL;
        }
        // If booting FW
        if (is_fw_boot)
        {
            // Check that pointer to list of FW blocks is not null, nor is size of list 0
            if ((config->fw_blocks != NULL) && \
                (config->total_fw_blocks > 0))
            {
                halo_boot_block_t *temp_block_ptr = config->fw_blocks;
                // Check that number of required FW block pointers are NOT 0
                for (int i = 0; i < config->total_fw_blocks; i++)
                {
                    if ((temp_block_ptr++)->bytes == NULL)
                    {
                        ret = CS40L25_STATUS_FAIL;
                        break;
                    }
                }
            }
            else
            {
                ret = CS40L25_STATUS_FAIL;
            }
        }

        // If booting COEFF file
        if (is_coeff_boot)
        {
            // Check that pointer to list of COEFF blocks is not null, nor is size of list 0
            if ((config->coeff_files != NULL) && \
                (config->total_coeff_blocks > 0))
            {
                uint32_t coeff_blocks_verified = 0;
                uint32_t file_no = 0;
                while (coeff_blocks_verified < config->total_coeff_blocks) {
                    halo_boot_block_t *temp_block_ptr = config->coeff_files[file_no].data;
                    // Check that number of required COEFF block pointers are NOT 0
                    for (int i = 0; i < config->coeff_files[file_no].total_blocks; i++)
                    {
                        if ((temp_block_ptr++)->bytes == NULL)
                        {
                            ret = CS40L25_STATUS_FAIL;
                            break;
                        }
                    }
                    file_no++;
                    coeff_blocks_verified += config->coeff_files[file_no].total_blocks;
                }
            }
            else
            {
                ret = CS40L25_STATUS_FAIL;
            }
        }

        // If booting calibration FW
        if (is_cal_boot)
        {
            // Check that pointer to list of cal FW blocks is not null, nor is size of list 0
            if ((config->cal_blocks != NULL) && \
                (config->total_cal_blocks > 0))
            {
                halo_boot_block_t *temp_block_ptr = config->cal_blocks;
                // Check that number of required FW block pointers are NOT 0
                for (int i = 0; i < config->total_cal_blocks; i++)
                {
                    if ((temp_block_ptr++)->bytes == NULL)
                    {
                        ret = CS40L25_STATUS_FAIL;
                        break;
                    }
                }
            }
            else
            {
                ret = CS40L25_STATUS_FAIL;
            }
        }
    }

    return ret;
}

/**
 * Writes from byte array to consecutive number of Control Port memory addresses
 *
 * Implementation of cs40l25_private_functions_t.cp_bulk_write
 *
 */
static uint32_t cs40l25_cp_bulk_write(cs40l25_t *driver, uint32_t addr, uint8_t *bytes, uint32_t length)
{
    uint32_t ret = CS40L25_STATUS_OK;
    uint32_t bsp_status;

    /*
     * Switch from Little-Endian contents of uint32_t 'addr' to Big-Endian format required for Control Port
     * transaction.
     *
     * FIXME: This is not platform independent.
     */
    driver->cp_write_buffer[0] = GET_BYTE_FROM_WORD(addr, 3);
    driver->cp_write_buffer[1] = GET_BYTE_FROM_WORD(addr, 2);
    driver->cp_write_buffer[2] = GET_BYTE_FROM_WORD(addr, 1);
    driver->cp_write_buffer[3] = GET_BYTE_FROM_WORD(addr, 0);

    bsp_status = bsp_driver_if_g->i2c_db_write(driver->bsp_dev_id,
                                               driver->cp_write_buffer,
                                               4,
                                               bytes,
                                               length,
                                               cs40l25_private_functions_g->cp_write_callback,
                                               driver);

    if (bsp_status == BSP_STATUS_FAIL)
    {
        ret = CS40L25_STATUS_FAIL;
    }

    return ret;
}

/**
 * Implements 'copy' method for Control Request Queue contents
 *
 * Implementation of cs40l25_private_functions_t.control_q_copy
 *
 */
static bool cs40l25_control_q_copy(void *from, void *to)
{
    bool ret = false;

    // Check for any NULL pointers
    if ((from != NULL) && (to != NULL))
    {
        cs40l25_control_request_t *from_r, *to_r;
        from_r = (cs40l25_control_request_t *) from;
        to_r = (cs40l25_control_request_t *) to;

        // Copy contents
        to_r->arg = from_r->arg;
        to_r->cb = from_r->cb;
        to_r->cb_arg = from_r->cb_arg;
        to_r->id = from_r->id;

        ret = true;
    }

    return ret;
}

/**
 * Check that the currently processed Control Request is valid for the current state of the driver.
 *
 * Implementation of cs40l25_private_functions_t.is_control_valid
 *
 */
static uint32_t cs40l25_is_control_valid(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_FAIL;

    // Request is considered invalid if there is no Control Request being processed
    if (driver->control_sm.fp == NULL)
    {
        return ret;
    }

    uint32_t state = driver->state;
    switch (driver->current_request.id)
    {
        case CS40L25_CONTROL_ID_RESET:
            // RESET Control Request is only invalid for UNCONFIGURED and ERROR states, otherwise valid
            if ((state == CS40L25_STATE_CONFIGURED) ||
                (state == CS40L25_STATE_DSP_STANDBY) ||
                (state == CS40L25_STATE_CAL_STANDBY) ||
                (state == CS40L25_STATE_HIBERNATE) ||
                (state == CS40L25_STATE_STANDBY))
            {
                ret = CS40L25_STATUS_OK;
            }
            break;

        case CS40L25_CONTROL_ID_BOOT:
            // BOOT Control Request is only valid for STANDBY state
            if ((state == CS40L25_STATE_STANDBY) ||
                (state == CS40L25_STATE_DSP_STANDBY) ||
                (state == CS40L25_STATE_CAL_STANDBY))
            {
                ret = CS40L25_STATUS_OK;
            }
            break;

        case CS40L25_CONTROL_ID_CONFIGURE:
            // CONFIGURE Control Requests are only valid for STANDBY and DSP_STANDBY states
            if ((state == CS40L25_STATE_STANDBY) ||
                (state == CS40L25_STATE_DSP_STANDBY) ||
                (state == CS40L25_STATE_CAL_STANDBY))
            {
                ret = CS40L25_STATUS_OK;
            }
            break;

        case CS40L25_CONTROL_ID_POWER_UP:
            // POWER_UP Control Requests are only valid for STANDBY and DSP_STANDBY states
            if ((state == CS40L25_STATE_STANDBY) ||
                (state == CS40L25_STATE_DSP_STANDBY) ||
                (state == CS40L25_STATE_CAL_STANDBY))
            {
                ret = CS40L25_STATUS_OK;
            }
            break;

        case CS40L25_CONTROL_ID_POWER_DOWN:
            // POWER_DOWN Control Requests are valid for all *POWER_UP states
            if ((state == CS40L25_STATE_POWER_UP) ||
                (state == CS40L25_STATE_DSP_POWER_UP) ||
                (state == CS40L25_STATE_CAL_POWER_UP))
            {
                ret = CS40L25_STATUS_OK;
            }
            break;

        case CS40L25_CONTROL_ID_POWER_HIBERNATE:
            // HIBERNATE Control Request is only valid for DSP_POWER_UP state
            if (state == CS40L25_STATE_DSP_POWER_UP)
            {
                ret = CS40L25_STATUS_OK;
            }
            break;

        case CS40L25_CONTROL_ID_POWER_WAKE:
            // WAKE Control Request is only valid for HIBERNATE state
            if (state == CS40L25_STATE_HIBERNATE)
            {
                ret = CS40L25_STATUS_OK;
            }
            break;

        case CS40L25_CONTROL_ID_CALIBRATION:
            // CALIBRATION Control Requests are valid for DSP_POWER_UP and CAL_POWER_UP states
            if (state == CS40L25_STATE_DSP_POWER_UP ||
                state == CS40L25_STATE_CAL_POWER_UP)
            {
                ret = CS40L25_STATUS_OK;
            }
            break;

        case CS40L25_CONTROL_ID_GET_VOLUME:
        case CS40L25_CONTROL_ID_SET_VOLUME:
            // GET_VOLUME and SET_VOLUME Control Requests are always valid
        case CS40L25_CONTROL_ID_GET_HALO_HEARTBEAT:
        case CS40L25_CONTROL_ID_SET_BHM_BUZZ_TRIGGER:
        case CS40L25_CONTROL_ID_SET_GPIO_ENABLE:
        case CS40L25_CONTROL_ID_SET_GPIO1_BUTTON_DETECT:
        case CS40L25_CONTROL_ID_SET_GPIO2_BUTTON_DETECT:
        case CS40L25_CONTROL_ID_SET_GPIO3_BUTTON_DETECT:
        case CS40L25_CONTROL_ID_SET_GPIO4_BUTTON_DETECT:
        case CS40L25_CONTROL_ID_SET_CLAB_ENABLED:
        case CS40L25_CONTROL_ID_SET_GPI_GAIN_CONTROL:
        case CS40L25_CONTROL_ID_SET_CTRL_PORT_GAIN_CONTROL:
        case CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_PRESS:
        case CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_PRESS:
        case CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_PRESS:
        case CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_PRESS:
        case CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_RELEASE:
        case CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_RELEASE:
        case CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_RELEASE:
        case CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_RELEASE:
        case CS40L25_CONTROL_ID_SET_TRIGGER_INDEX:
        case CS40L25_CONTROL_ID_SET_TRIGGER_MS:
        case CS40L25_CONTROL_ID_SET_TIMEOUT_MS:
        case CS40L25_CONTROL_ID_GET_DSP_STATUS:
        case CS40L25_CONTROL_ID_GET_FW_REVISION:
            // GET_HALO_HEARTBEAT and GET_DSP_STATUS Control Requests are always valid
            ret = CS40L25_STATUS_OK;
            break;


        case CS40L25_CONTROL_ID_GET_DYNAMIC_REDC:
        case CS40L25_CONTROL_ID_GET_DYNAMIC_F0:
        case CS40L25_CONTROL_ID_ENABLE_DYNAMIC_F0:
            // If Dynamic F0 algorithm is loaded, then OK
#ifdef CS40L25_ALGORITHM_DYNAMIC_F0
            ret = CS40L25_STATUS_OK;
#endif
            break;

        default:
            break;
    }

    return ret;
}

/**
 * Load new Control Request to be processed
 *
 * Implementation of cs40l25_private_functions_t.load_control
 *
 */
static uint32_t cs40l25_load_control(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_FAIL;

    // Only proceed if successful removal of Control Request from Control Request Queue
    if (F_QUEUE_STATUS_OK == f_queue_if_g->remove(&(driver->control_q), &(driver->current_request)))
    {
        /*
         * Reset all Control State Machines by:
         * - clearing flags
         * - assigning state machine function pointer
         * - setting initial state to CS40L25_SM_STATE_INIT
         */
        driver->control_sm.flags = 0;
        switch (driver->current_request.id)
        {
            case CS40L25_CONTROL_ID_RESET:
                driver->control_sm.fp = cs40l25_private_functions_g->reset_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_BOOT:
                driver->control_sm.fp = cs40l25_private_functions_g->boot_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                // For BOOT Control Request, pass through request argument to state machine flags
                driver->control_sm.flags = (uint32_t) driver->current_request.arg;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_POWER_UP:
                driver->control_sm.fp = cs40l25_private_functions_g->power_up_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_POWER_DOWN:
                driver->control_sm.fp = cs40l25_private_functions_g->power_down_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_POWER_HIBERNATE:
                driver->control_sm.fp = cs40l25_private_functions_g->hibernate_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_POWER_WAKE:
                driver->control_sm.fp = cs40l25_private_functions_g->wake_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_CONFIGURE:
                driver->control_sm.fp = cs40l25_private_functions_g->configure_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_GET_VOLUME:
                // For a GET request, set the GET_REQUESt flag
                CS40L25_SET_FLAG(driver->control_sm.flags, CS40L25_FLAGS_IS_GET_REQUEST);
            case CS40L25_CONTROL_ID_SET_VOLUME:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                // For the GET_/SET_VOLUME Control Requests, setup field_accessor with bit-field information
                driver->field_accessor.address = CS40L25_INTP_AMP_CTRL_REG;
                driver->field_accessor.shift = CS40L25_INTP_AMP_CTRL_AMP_VOL_PCM_BITOFFSET;
                driver->field_accessor.size = CS40L25_INTP_AMP_CTRL_AMP_VOL_PCM_BITWIDTH;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_GET_HALO_HEARTBEAT:
                switch(driver->state) {
                    case CS40L25_STATE_POWER_UP:
                    case CS40L25_STATE_STANDBY:
                        CS40L25_SET_FLAG(driver->control_sm.flags, CS40L25_FLAGS_IS_GET_REQUEST);
                        driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                        driver->control_sm.state = CS40L25_SM_STATE_INIT;
                        driver->field_accessor.address = DSP_BHM_HALO_HEARTBEAT_REG;
                        driver->field_accessor.shift = 0;
                        driver->field_accessor.size = 32;
                        ret = CS40L25_STATUS_OK;
                        break;

                    case CS40L25_STATE_DSP_POWER_UP:
                    case CS40L25_STATE_CAL_POWER_UP:
                        // For a GET request, set the GET_REQUEST flag
                        CS40L25_SET_FLAG(driver->control_sm.flags, CS40L25_FLAGS_IS_GET_REQUEST);
                        driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                        driver->control_sm.state = CS40L25_SM_STATE_INIT;
                        // Setup field_accessor with bit-field information
                        driver->field_accessor.address = DSP_REG(HALO_HEARTBEAT);
                        driver->field_accessor.shift = 0;
                        driver->field_accessor.size = 32;
                        ret = CS40L25_STATUS_OK;
                        break;

                    default:
                        ret = CS40L25_STATUS_INVALID;
                        break;
                }
                break;
            case CS40L25_CONTROL_ID_SET_BHM_BUZZ_TRIGGER:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = DSP_BHM_BUZZ_TRIGGER_REG;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = true;
                driver->field_accessor.ack_reset = 0x0;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_SET_GPIO_ENABLE:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_GPIO_ENABLE;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO1_BUTTON_DETECT:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_GPIO_BUTTONDETECT;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 1;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO2_BUTTON_DETECT:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_GPIO_BUTTONDETECT;
                driver->field_accessor.shift = 1;
                driver->field_accessor.size = 1;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO3_BUTTON_DETECT:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_GPIO_BUTTONDETECT;
                driver->field_accessor.shift = 2;
                driver->field_accessor.size = 1;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO4_BUTTON_DETECT:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_GPIO_BUTTONDETECT;
                driver->field_accessor.shift = 3;
                driver->field_accessor.size = 1;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_CLAB_ENABLED:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_CLAB_ENABLED;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 1;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPI_GAIN_CONTROL:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_GAIN_CONTROL;
                driver->field_accessor.shift = 14;
                driver->field_accessor.size = 10;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_CTRL_PORT_GAIN_CONTROL:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_GAIN_CONTROL;
                driver->field_accessor.shift = 4;
                driver->field_accessor.size = 10;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
           case CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_PRESS:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_INDEXBUTTONPRESS;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_PRESS:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_INDEXBUTTONPRESS + 4;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_PRESS:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_INDEXBUTTONPRESS + 8;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_PRESS:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_INDEXBUTTONPRESS + 12;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_RELEASE:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_INDEXBUTTONRELEASE;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_RELEASE:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_INDEXBUTTONRELEASE + 4;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_RELEASE:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_INDEXBUTTONRELEASE + 8;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_RELEASE:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_INDEXBUTTONRELEASE + 12;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_TRIGGER_INDEX:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = true;
                driver->field_accessor.ack_reset = 0xFFFFFFFF;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_TRIGGER_MS:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_2_REG;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = true;
                driver->field_accessor.ack_reset = 0xFFFFFFFF;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_SET_TIMEOUT_MS:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_TIMEOUT_MS;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;
            case CS40L25_CONTROL_ID_CALIBRATION:
                driver->control_sm.fp = cs40l25_private_functions_g->calibration_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_GET_DSP_STATUS:
                driver->control_sm.fp = cs40l25_private_functions_g->get_dsp_status_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_GET_FW_REVISION:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                CS40L25_SET_FLAG(driver->control_sm.flags, CS40L25_FLAGS_IS_GET_REQUEST);
                driver->field_accessor.address = CS40L25_FIRMWARE_REVISION;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_GET_DYNAMIC_REDC:
                driver->control_sm.fp = cs40l25_private_functions_g->dynamic_redc_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_GET_DYNAMIC_F0:
                driver->control_sm.fp = cs40l25_private_functions_g->dynamic_f0_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                ret = CS40L25_STATUS_OK;
                break;

            case CS40L25_CONTROL_ID_ENABLE_DYNAMIC_F0:
                driver->control_sm.fp = cs40l25_private_functions_g->field_access_sm;
                driver->control_sm.state = CS40L25_SM_STATE_INIT;
                driver->field_accessor.address = CS40L25_DYNAMIC_F0_ENABLED;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 1;
                driver->field_accessor.ack_ctrl = false;
                ret = CS40L25_STATUS_OK;
                break;

            default:
                break;
        }
    }

    return ret;
}

/**
 * Apply all driver one-time configurations to corresponding Control Port register/memory addresses
 *
 * Implementation of cs40l25_private_functions_t.apply_configs
 *
 */
static uint32_t cs40l25_apply_configs(cs40l25_t *driver)
{
    uint32_t ret = CS40L25_STATUS_OK;
    uint8_t i;
    bool code_found;
    cs40l25_config_registers_t *regs = &(driver->config_regs);

    /*
     * apply audio hw configurations
     */
    cs40l25_audio_hw_config_t *hw = &(driver->audio_config.hw);

    regs->dataif_asp_control2.asp_bclk_mstr = hw->is_master_mode;
    regs->dataif_asp_control2.asp_fsync_mstr = regs->dataif_asp_control2.asp_bclk_mstr;
    regs->dataif_asp_control2.asp_fsync_inv = hw->fsync_inv;
    regs->dataif_asp_control2.asp_bclk_inv = hw->bclk_inv;

    regs->msm_block_enables2.amp_dre_en = hw->amp_dre_en;

    regs->intp_amp_ctrl.amp_ramp_pcm = hw->amp_ramp_pcm;
    regs->intp_amp_ctrl.amp_hpf_pcm_en = 1;

    /*
     * apply audio clocking configurations
     */
    cs40l25_clock_config_t *clk = &(driver->audio_config.clock);

    // apply audio clocking - refclk source
    regs->ccm_refclk_input.pll_refclk_sel = clk->refclk_sel;

    // apply audio clocking - refclk frequency
    code_found = false;
    for (i = 0; i < (sizeof(cs40l25_pll_sysclk)/sizeof(struct cs40l25_register_encoding)); i++)
    {
        if (clk->refclk_freq == cs40l25_pll_sysclk[i].value)
        {
            code_found = true;
            regs->ccm_refclk_input.pll_refclk_freq = cs40l25_pll_sysclk[i].code;
            break;
        }
    }
    if (!code_found)
    {
        ret = CS40L25_STATUS_FAIL;
    }

    // apply audio clocking - sclk frequency
    code_found = false;
    for (i = 0; i < (sizeof(cs40l25_sclk_encoding)/sizeof(struct cs40l25_register_encoding)); i++)
    {
        if (clk->sclk == cs40l25_sclk_encoding[i].value)
        {
            code_found = true;
            regs->dataif_asp_control1.asp_bclk_freq = cs40l25_sclk_encoding[i].code;
            break;
        }
    }
    if (!code_found)
    {
        ret = CS40L25_STATUS_FAIL;
    }

    // The procedure below is taken from the datasheet, Section 4.13.9
    if (clk->sclk > CS40L25_FS_MON0_BETA)
    {
        regs->ccm_fs_mon0 = 0x00024010;
    }
    else
    {
        uint32_t x = 12 * CS40L25_FS_MON0_BETA / clk->sclk + 4;
        uint32_t y = 20 * CS40L25_FS_MON0_BETA / clk->sclk + 4;
        regs->ccm_fs_mon0 = x + (y * 4096);
    }

    regs->ccm_refclk_input.pll_refclk_en = 1;

    /*
     * apply audio port configurations
     */
    cs40l25_asp_config_t *asp = &(driver->audio_config.asp);
    if (asp->is_i2s)
    {
        regs->dataif_asp_control2.asp_fmt = CS40L25_ASP_CONTROL2_ASP_FMT_I2S;
    }
    else
    {
        regs->dataif_asp_control2.asp_fmt = CS40L25_ASP_CONTROL2_ASP_FMT_DSPA;
    }

    regs->dataif_asp_frame_control5.asp_rx1_slot = asp->rx1_slot;
    regs->dataif_asp_frame_control5.asp_rx2_slot = asp->rx2_slot;
    regs->dataif_asp_frame_control1.asp_tx1_slot = asp->tx1_slot;
    regs->dataif_asp_frame_control1.asp_tx2_slot = asp->tx2_slot;
    regs->dataif_asp_frame_control1.asp_tx3_slot = asp->tx3_slot;
    regs->dataif_asp_frame_control1.asp_tx4_slot = asp->tx4_slot;

    regs->dataif_asp_data_control5.asp_rx_wl = asp->rx_wl;
    regs->dataif_asp_control2.asp_rx_width = asp->rx_width;

    regs->dataif_asp_data_control1.asp_tx_wl = asp->tx_wl;
    regs->dataif_asp_control2.asp_tx_width = asp->tx_width;

    /*
     * apply audio routing configurations
     */
    cs40l25_routing_config_t *routing = &(driver->audio_config.routing);
    regs->dacpcm1_input.src = routing->dac_src;
    regs->asptx1_input.src = routing->asp_tx1_src;
    regs->asptx2_input.src = routing->asp_tx2_src;
    regs->asptx3_input.src = routing->asp_tx3_src;
    regs->asptx4_input.src = routing->asp_tx4_src;
    regs->dsp1rx1_input.src = routing->dsp_rx1_src;
    regs->dsp1rx2_input.src = routing->dsp_rx2_src;
    regs->dsp1rx3_input.src = routing->dsp_rx3_src;
    regs->dsp1rx4_input.src = routing->dsp_rx4_src;

    /*
     * apply asp block enable configurations
     */
    regs->dataif_asp_enables1.asp_rx1_en = 0;
    if (cs40l25_private_functions_g->is_mixer_source_used(driver, CS40L25_INPUT_SRC_ASPRX1))
    {
        regs->dataif_asp_enables1.asp_rx1_en = 1;
    }

    regs->dataif_asp_enables1.asp_rx2_en = 0;
    if (cs40l25_private_functions_g->is_mixer_source_used(driver, CS40L25_INPUT_SRC_ASPRX2))
    {
        regs->dataif_asp_enables1.asp_rx2_en = 1;
    }

    if (routing->asp_tx1_src != CS40L25_INPUT_SRC_DISABLE)
    {
        regs->dataif_asp_enables1.asp_tx1_en = 1;
    }
    if (routing->asp_tx2_src != CS40L25_INPUT_SRC_DISABLE)
    {
        regs->dataif_asp_enables1.asp_tx2_en = 1;
    }
    if (routing->asp_tx3_src != CS40L25_INPUT_SRC_DISABLE)
    {
        regs->dataif_asp_enables1.asp_tx3_en = 1;
    }
    if (routing->asp_tx4_src != CS40L25_INPUT_SRC_DISABLE)
    {
        regs->dataif_asp_enables1.asp_tx4_en = 1;
    }

    /*
     * apply startup volume
     */
    regs->intp_amp_ctrl.amp_vol_pcm = driver->audio_config.volume;

    /*
     * apply boost configurations
     */
    cs40l25_amp_config_t *amp = &(driver->amp_config);

    uint8_t lbst_code, cbst_code, ipk_code;

    // Get code for Boost Inductor
    switch (amp->boost_inductor_value_nh)
    {
        case 1000:  /* 1.0 uH */
            lbst_code = 0;
            break;

        case 1200:  /* 1.2 uH */
            lbst_code = 1;
            break;

        case 1500:  /* 1.5 uH */
            lbst_code = 2;
            break;

        case 2200:  /* 2.2 uH */
            lbst_code = 3;
            break;

        default:
            ret = CS40L25_STATUS_FAIL;
            break;
    }

    // Get code for Boost Capacitor
    switch (amp->boost_capacitor_value_uf)
    {
        case 0 ... 19:
            cbst_code = 0;
            break;

        case 20 ... 50:
            cbst_code = 1;
            break;

        case 51 ... 100:
            cbst_code = 2;
            break;

        case 101 ... 200:
            cbst_code = 3;
            break;

        default:    /* 201 uF and greater */
            cbst_code = 4;
            break;
    }

    // Get Boost Loop Coefficient and LBST Slope based on codes above
    regs->boost_bst_loop_coeff.bst_k1 = cs40l25_bst_k1_table[lbst_code][cbst_code];
    regs->boost_bst_loop_coeff.bst_k2 = cs40l25_bst_k2_table[lbst_code][cbst_code];
    regs->boost_lbst_slope.bst_lbst_val = lbst_code;
    regs->boost_lbst_slope.bst_slope = cs40l25_bst_slope_table[lbst_code];

    // Bounds check the Peak Current configuration
    if ((amp->boost_ipeak_ma < 1600) || (amp->boost_ipeak_ma > 4500))
    {
        ret = CS40L25_STATUS_FAIL;
    }
    else
    {
        // Encoding corresponds to values in Datasheet Section 7.11.3
        ipk_code = ((amp->boost_ipeak_ma - 1600) / 50) + 0x10;
    }
    regs->boost_bst_ipk_ctl.bst_ipk = ipk_code;

    regs->boost_vbst_ctl_1.bst_ctl = amp->bst_ctl;

    // Only if Class H is enabled, then apply Class H configurations
    if (amp->classh_enable)
    {
        regs->boost_vbst_ctl_2.bst_ctl_sel = amp->bst_ctl_sel;
        regs->boost_vbst_ctl_2.bst_ctl_lim_en = (amp->bst_ctl_lim_en ? 1 : 0);
    }

    /*
     * apply block enable configurations
     */
    // Always enable the Amplifier section
    regs->msm_block_enables.amp_en = 1;

    // If DSP is booted, then turn on some blocks by default
    if (driver->state == CS40L25_STATE_DSP_STANDBY ||
        driver->state == CS40L25_STATE_CAL_STANDBY)
    {
        // The DSP needs VMON/IMON data for CSPL
        regs->msm_block_enables.vmon_en = 1;
        regs->msm_block_enables.imon_en = 1;
        // The DSP is using VPMON, CLASSH, and TEMPMON (see cs40l25_post_boot_config[])
        regs->msm_block_enables.vpmon_en = 1;
        regs->msm_block_enables2.classh_en = 1;
        regs->msm_block_enables.tempmon_en = 0;
    }
    // Otherwise, see if the blocks are being used somewhere in order to enable
    else
    {
        regs->msm_block_enables2.classh_en = 0;
        if (amp->classh_enable)
        {
            regs->msm_block_enables2.classh_en = 1;
        }

        regs->msm_block_enables.tempmon_en = 0;
        if (cs40l25_private_functions_g->is_mixer_source_used(driver, CS40L25_INPUT_SRC_TEMPMON))
        {
            regs->msm_block_enables.tempmon_en = 1;
        }

        regs->msm_block_enables.vpmon_en = 0;
        if (cs40l25_private_functions_g->is_mixer_source_used(driver, CS40L25_INPUT_SRC_VPMON))
        {
            regs->msm_block_enables.vpmon_en = 1;
        }
    }

    regs->msm_block_enables.vbstmon_en = 1;

    regs->wakesrc_ctl.wksrc_en = (amp->wksrc_gpio1_en ? 0x1 : 0) |
                                 (amp->wksrc_gpio2_en ? 0x2 : 0) |
                                 (amp->wksrc_gpio4_en ? 0x4 : 0) |
                                 (amp->wksrc_sda_en ? 0x8 : 0);

    regs->wakesrc_ctl.wksrc_pol = (amp->wksrc_gpio1_falling_edge ? 0x1 : 0) |
                                  (amp->wksrc_gpio2_falling_edge ? 0x2 : 0) |
                                  (amp->wksrc_gpio4_falling_edge ? 0x4 : 0) |
                                  (amp->wksrc_sda_falling_edge ? 0x8 : 0);

    // Always configure as Boost converter enabled.
    regs->msm_block_enables.bst_en = 0x2;

    cs40l25_dsp_config_controls_t dsp_ctrls = driver->dsp_config_ctrls;
    regs->dsp_gpio_button_detect.gpio1_enable = (dsp_ctrls.dsp_gpio1_button_detect_enable ? 1 : 0);
    regs->dsp_gpio_button_detect.gpio2_enable = (dsp_ctrls.dsp_gpio2_button_detect_enable ? 1 : 0);
    regs->dsp_gpio_button_detect.gpio3_enable = (dsp_ctrls.dsp_gpio3_button_detect_enable ? 1 : 0);
    regs->dsp_gpio_button_detect.gpio4_enable = (dsp_ctrls.dsp_gpio4_button_detect_enable ? 1 : 0);
    regs->dsp_gpio_enable.halo_word = (dsp_ctrls.dsp_gpio_enable ? 1 : 0);
    regs->dsp_gain_control.gpi_gain = dsp_ctrls.dsp_gpi_gain_control;
    regs->dsp_gain_control.control_gain = dsp_ctrls.dsp_ctrl_gain_control;
    regs->dsp_gpio1_index_button_press.halo_word = dsp_ctrls.dsp_gpio1_index_button_press;
    regs->dsp_gpio2_index_button_press.halo_word = dsp_ctrls.dsp_gpio2_index_button_press;
    regs->dsp_gpio3_index_button_press.halo_word = dsp_ctrls.dsp_gpio3_index_button_press;
    regs->dsp_gpio4_index_button_press.halo_word = dsp_ctrls.dsp_gpio4_index_button_press;
    regs->dsp_gpio1_index_button_release.halo_word = dsp_ctrls.dsp_gpio1_index_button_release;
    regs->dsp_gpio2_index_button_release.halo_word = dsp_ctrls.dsp_gpio2_index_button_release;
    regs->dsp_gpio3_index_button_release.halo_word = dsp_ctrls.dsp_gpio3_index_button_release;
    regs->dsp_gpio4_index_button_release.halo_word = dsp_ctrls.dsp_gpio4_index_button_release;

    regs->clab_enabled.halo_word = (dsp_ctrls.clab_enable ? 1 : 0);
    regs->peak_amplitude_control.halo_word =  dsp_ctrls.peak_amplitude;

    return ret;
}

/**
 * Checks all hardware mixer source selections for a specific source.
 *
 * Implementation of cs40l25_private_functions_t.is_mixer_source_used
 *
 */
static bool cs40l25_is_mixer_source_used(cs40l25_t *driver, uint8_t source)
{
    cs40l25_routing_config_t *routing = &(driver->audio_config.routing);

    if ((routing->dac_src == source) || \
        (routing->asp_tx1_src == source) || \
        (routing->asp_tx2_src == source) || \
        (routing->asp_tx3_src == source) || \
        (routing->asp_tx4_src == source) || \
        (routing->dsp_rx1_src == source) || \
        (routing->dsp_rx2_src == source) || \
        (routing->dsp_rx3_src == source) || \
        (routing->dsp_rx4_src == source))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * Function pointer table for Private API implementation
 *
 * @attention Although not const, this should never be changed run-time in an end-product.  It is implemented this
 * way to facilitate unit testing.
 *
 */
static cs40l25_private_functions_t cs40l25_private_functions_s =
{
    .timer_callback = &cs40l25_timer_callback,
    .cp_read_callback = &cs40l25_cp_read_callback,
    .cp_write_callback = &cs40l25_cp_write_callback,
    .irq_callback = &cs40l25_irq_callback,
    .read_reg = &cs40l25_read_reg,
    .write_reg = &cs40l25_write_reg,
    .reset_sm = &cs40l25_reset_sm,
    .boot_sm = &cs40l25_boot_sm,
    .power_up_sm = &cs40l25_power_up_sm,
    .power_down_sm = &cs40l25_power_down_sm,
    .configure_sm = &cs40l25_configure_sm,
    .field_access_sm = &cs40l25_field_access_sm,
    .calibration_sm = &cs40l25_calibration_sm,
    .get_dsp_status_sm = &cs40l25_get_dsp_status_sm,
    .hibernate_sm = &cs40l25_hibernate_sm,
    .wake_sm = &cs40l25_wake_sm,
    .dynamic_redc_sm = &cs40l25_dynamic_redc_sm,
    .dynamic_f0_sm = &cs40l25_dynamic_f0_sm,
    .event_sm = &cs40l25_event_sm,
    .get_errata = &cs40l25_get_errata,
    .cp_bulk_read = &cs40l25_cp_bulk_read,
    .cp_bulk_write = &cs40l25_cp_bulk_write,
    .validate_boot_config = &cs40l25_validate_boot_config,
    .control_q_copy = &cs40l25_control_q_copy,
    .is_control_valid = &cs40l25_is_control_valid,
    .load_control = &cs40l25_load_control,
    .apply_configs = &cs40l25_apply_configs,
    .is_mixer_source_used = &cs40l25_is_mixer_source_used
};

/**
 * Pointer to Private API implementation
 */
cs40l25_private_functions_t *cs40l25_private_functions_g = &cs40l25_private_functions_s;

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
        // Initialize the Control Request Queue
        ret = f_queue_if_g->initialize(&(driver->control_q),
                                       CS40L25_CONTROL_REQUESTS_SIZE,
                                       driver->control_requests,
                                       sizeof(cs40l25_control_request_t),
                                       cs40l25_private_functions_g->control_q_copy);

        if (ret == F_QUEUE_STATUS_OK)
        {
            ret = CS40L25_STATUS_OK;
        }
        else
        {
            ret = CS40L25_STATUS_FAIL;
        }
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
        (NULL != config->cp_write_buffer) && \
        (NULL != config->cp_read_buffer))
    {
        driver->bsp_dev_id = config->bsp_dev_id;
        driver->bsp_reset_gpio_id = config->bsp_reset_gpio_id;
        driver->bsp_int_gpio_id = config->bsp_int_gpio_id;
        driver->bus_type = config->bus_type;
        driver->cp_write_buffer = config->cp_write_buffer;
        driver->cp_read_buffer = config->cp_read_buffer;
        driver->notification_cb = config->notification_cb;
        driver->notification_cb_arg = config->notification_cb_arg;
        // Advance driver to CONFIGURED state
        driver->state = CS40L25_STATE_CONFIGURED;

        driver->audio_config = config->audio_config;
        driver->amp_config = config->amp_config;

        driver->dsp_config_ctrls = config->dsp_config_ctrls;
        /*
         * Copy the Calibration data.  If it is not valid (is_valid = false), then it will not be sent to the device
         * during boot()
         */
        driver->cal_data = config->cal_data;

        ret = bsp_driver_if_g->register_gpio_cb(driver->bsp_int_gpio_id,
                                                cs40l25_private_functions_g->irq_callback,
                                                driver);

        driver->config_regs.event_control = config->event_control;

        if (ret == BSP_STATUS_OK)
        {
            ret = CS40L25_STATUS_OK;
        }
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
    uint32_t ret;
    uint32_t status = CS40L25_STATUS_OK;
    uint32_t sm_ret = CS40L25_STATUS_OK;

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
                // run through event sm
                sm_ret = cs40l25_private_functions_g->event_sm(driver);

                if (sm_ret == CS40L25_STATUS_OK)
                {
                    // check current status of Event SM
                    if (driver->event_sm.state == CS40L25_SM_STATE_DONE)
                    {
                        driver->mode = CS40L25_MODE_HANDLING_CONTROLS;

                        // If a control port transaction was interrupted, restart the current Control Request
                        if (CS40L25_IS_FLAG_SET(driver->control_sm.flags, CS40L25_FLAGS_REQUEST_RESTART))
                        {
                            driver->event_sm.state = CS40L25_EVENT_SM_STATE_INIT;
                            // Need to reset current Control SM here
                            driver->control_sm.state = CS40L25_SM_STATE_INIT;
                            driver->control_sm.flags = 0;
                        }
                    }
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

        // Instead of 'else' here, re-check driver mode in case Event Handler SM previously transitioned to DONE
        if (driver->mode == CS40L25_MODE_HANDLING_CONTROLS)
        {
            bool is_new_request_loaded;

            do
            {
                // Is currently loaded control valid?
                status = cs40l25_private_functions_g->is_control_valid(driver);

                // If invalid, unload it
                if (status == CS40L25_STATUS_INVALID)
                {
                    // Unload control
                    driver->control_sm.fp = NULL;
                    // Call request callback with status
                    cs40l25_control_request_t r = driver->current_request;
                    if (r.cb != NULL)
                    {
                        r.cb(r.id, CS40L25_STATUS_INVALID, r.cb_arg);
                    }
                }
                // Handle currently loaded request
                else if (status == CS40L25_STATUS_OK)
                {
                    // Step through Control SM
                    sm_ret = driver->control_sm.fp(driver);

                    /*
                     *  If Control SM is now in state DONE, update driver state based on which Control Request was
                     *  processed
                     */
                    if (driver->control_sm.state == CS40L25_SM_STATE_DONE)
                    {
                        switch (driver->current_request.id)
                        {
                            case CS40L25_CONTROL_ID_RESET:
                                if ((driver->state == CS40L25_STATE_CONFIGURED) ||
                                    (driver->state == CS40L25_STATE_DSP_STANDBY) ||
                                    (driver->state == CS40L25_STATE_CAL_STANDBY) ||
                                    (driver->state == CS40L25_STATE_HIBERNATE))
                                {
                                    driver->state = CS40L25_STATE_POWER_UP;
                                }
                                break;

                            case CS40L25_CONTROL_ID_BOOT:
                                if (driver->state == CS40L25_STATE_STANDBY ||
                                    driver->state == CS40L25_STATE_DSP_STANDBY ||
                                    driver->state == CS40L25_STATE_CAL_STANDBY)
                                {
                                    cs40l25_sm_t *b = &(driver->control_sm);
                                    if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_REQUEST_CAL_BOOT))
                                    {
                                        CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_REQUEST_CAL_BOOT);
                                        driver->state = CS40L25_STATE_CAL_STANDBY;
                                    }
                                    else if (CS40L25_IS_FLAG_SET(b->flags, CS40L25_FLAGS_REQUEST_FW_BOOT))
                                    {
                                        CS40L25_CLEAR_FLAG(b->flags, CS40L25_FLAGS_REQUEST_FW_BOOT);
                                        driver->state = CS40L25_STATE_DSP_STANDBY;
                                    }
                                }
                                break;

                            case CS40L25_CONTROL_ID_POWER_UP:
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
                                break;

                            case CS40L25_CONTROL_ID_POWER_DOWN:
                                if (driver->state == CS40L25_STATE_POWER_UP)
                                {
                                    driver->state = CS40L25_STATE_STANDBY;
                                }
                                else if (driver->state == CS40L25_STATE_DSP_POWER_UP)
                                {
                                    driver->state = CS40L25_STATE_DSP_STANDBY;
                                }
                                else if (driver->state == CS40L25_STATE_CAL_POWER_UP)
                                {
                                    driver->state = CS40L25_STATE_CAL_STANDBY;
                                }
                                break;

                            case CS40L25_CONTROL_ID_POWER_HIBERNATE:
                                if (driver->state == CS40L25_STATE_DSP_POWER_UP)
                                {
                                    driver->state = CS40L25_STATE_HIBERNATE;
                                }
                                break;

                            case CS40L25_CONTROL_ID_POWER_WAKE:
                                if (driver->state == CS40L25_STATE_HIBERNATE)
                                {
                                    driver->state = CS40L25_STATE_DSP_POWER_UP;
                                }
                                break;

                            case CS40L25_CONTROL_ID_CONFIGURE:
                            default:
                                break;
                        }
                    }

                    // If current control SM finished or error, unload it
                    if ((driver->control_sm.state == CS40L25_SM_STATE_DONE) || (sm_ret == CS40L25_STATUS_FAIL))
                    {
                        driver->control_sm.fp = NULL;
                        // Call request callback with status
                        cs40l25_control_request_t r = driver->current_request;
                        if (r.cb != NULL)
                        {
                            r.cb(r.id, sm_ret, r.cb_arg);
                        }

                        if (sm_ret == CS40L25_STATUS_FAIL)
                        {
                            driver->state = CS40L25_STATE_ERROR;
                        }
                    }
                }

                // If previous SM finished without error, try to load a new request from the Control Request Queue
                is_new_request_loaded = false;
                if ((sm_ret != CS40L25_STATUS_FAIL) && (driver->control_sm.fp == NULL))
                {
                    if (CS40L25_STATUS_OK == cs40l25_private_functions_g->load_control(driver))
                    {
                        is_new_request_loaded = true;
                    }
                }
            }
            /*
             * If the last Control SM finished OK and there is a new Control Request loaded, keep processing.  Since
             * each state machine is designed as non-run to completion (i.e. the SM function exits if there is a
             * wait state), then this loop should not take much time to complete.
             */
            while ((sm_ret == CS40L25_STATUS_OK) && is_new_request_loaded);
        }

        if (driver->state == CS40L25_STATE_ERROR)
        {
            driver->event_flags |= CS40L25_EVENT_FLAG_SM_ERROR;
            if (driver->mode == CS40L25_MODE_HANDLING_CONTROLS)
                debug_printf("Error handling control id 0x%x - SM state:0x%x\n", driver->current_request.id, driver->control_sm.state);
            else
                debug_printf("Error handling event, SM state:0x%x\n", driver->event_sm.state);
            if (driver->notification_cb != NULL)
            {
                driver->notification_cb(driver->event_flags, driver->notification_cb_arg);
            }
        }
    }

    ret = sm_ret;

    return ret;
}

/**
 * Submit a Control Request to the driver
 *
 * Implementation of cs40l25_functions_t.control
 *
 */
uint32_t cs40l25_control(cs40l25_t *driver, cs40l25_control_request_t req)
{
    uint32_t ret = CS40L25_STATUS_FAIL;

    // Check for valid Control Request ID
    if ((req.id > CS40L25_CONTROL_ID_NONE) && (req.id <= CS40L25_CONTROL_ID_MAX))
    {
        // Insert new request into Control Request Queue
        ret = f_queue_if_g->insert(&(driver->control_q), &req);
        if (ret == F_QUEUE_STATUS_OK)
        {
            ret = CS40L25_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Reset the CS40L25
 *
 * Implementation of cs40l25_functions_t.reset
 *
 */
uint32_t cs40l25_reset(cs40l25_t *driver, cs40l25_control_callback_t cb, void *cb_arg)
{
    uint32_t ret = CS40L25_STATUS_FAIL;
    cs40l25_control_request_t r;

    // Submit request for RESET Control
    r.id = CS40L25_CONTROL_ID_RESET;
    r.cb = cb;
    r.cb_arg = cb_arg;

    return cs40l25_functions_g->control(driver, r);
}

/**
 * Boot the CS40L25
 *
 * Implementation of cs40l25_functions_t.boot
 *
 */
uint32_t cs40l25_boot(cs40l25_t *driver, bool cal_boot, cs40l25_control_callback_t cb, void *cb_arg)
{
    uint32_t ret = CS40L25_STATUS_FAIL;
    cs40l25_control_request_t r;

    r.cb = cb;
    r.cb_arg = cb_arg;

    // Check that RESET Control submitted and that there are blocks of FW to load
    if (cal_boot && driver->boot_config->cal_blocks != NULL)
    {
        uint32_t temp_flags = 0;
        r.id = CS40L25_CONTROL_ID_BOOT;
        CS40L25_SET_FLAG(temp_flags, CS40L25_FLAGS_REQUEST_CAL_BOOT);
        // Pass in flags for CAL boot to Control SM
        r.arg = (void *) temp_flags;
        // Submit request for BOOT Control
        ret = cs40l25_functions_g->control(driver, r);
    }
    else if (!cal_boot && driver->boot_config->fw_blocks != NULL)
    {
        uint32_t temp_flags = 0;
        r.id = CS40L25_CONTROL_ID_BOOT;
        CS40L25_SET_FLAG(temp_flags, CS40L25_FLAGS_REQUEST_FW_BOOT);
        // Check that there are blocks of COEFF to load
        if (driver->boot_config->coeff_files != NULL &&
                driver->boot_config->total_coeff_blocks != 0)
        {
            CS40L25_SET_FLAG(temp_flags, CS40L25_FLAGS_REQUEST_COEFF_BOOT);
        }
        // Pass in flags for FW/COEFF boot to Control SM
        r.arg = (void *) temp_flags;
        // Submit request for BOOT Control
        ret = cs40l25_functions_g->control(driver, r);
    }

#ifndef I2S_CONFIG_SHORTCUT
    // If everything is okay, submit request for CONFIGURE Control
    if (ret == CS40L25_STATUS_OK)
    {
        r.id = CS40L25_CONTROL_ID_CONFIGURE;
        ret = cs40l25_functions_g->control(driver, r);
    }
#endif

    return ret;
}

/**
 * Change the power state
 *
 * Implementation of cs40l25_functions_t.power
 *
 */
uint32_t cs40l25_power(cs40l25_t *driver, uint32_t power_state, cs40l25_control_callback_t cb, void *cb_arg)
{
    uint32_t ret = CS40L25_STATUS_FAIL;
    cs40l25_control_request_t r;

    // Submit the correct request based on power_state
    switch (power_state)
    {
        case CS40L25_POWER_UP:
            r.id = CS40L25_CONTROL_ID_POWER_UP;
            break;

        case CS40L25_POWER_DOWN:
            r.id = CS40L25_CONTROL_ID_POWER_DOWN;
            break;

        case CS40L25_POWER_HIBERNATE:
            r.id = CS40L25_CONTROL_ID_POWER_HIBERNATE;
            break;

        case CS40L25_POWER_WAKE:
            r.id = CS40L25_CONTROL_ID_POWER_WAKE;
            break;

        default:
            r.id = CS40L25_CONTROL_ID_NONE;
            break;
    }

    if (r.id != CS40L25_CONTROL_ID_NONE)
    {
        r.cb = cb;
        r.cb_arg = cb_arg;
        ret = cs40l25_functions_g->control(driver, r);
    }

    return ret;
}

/**
 * Calibrate the HALO DSP Protection Algorithm
 *
 * Implementation of cs40l25_functions_t.calibrate
 *
 */
uint32_t cs40l25_calibrate(cs40l25_t *driver,
                           uint32_t calib_type,
                           cs40l25_control_callback_t cb,
                           void *cb_arg)
{
    uint32_t ret = CS40L25_STATUS_FAIL;
    cs40l25_control_request_t r;

    // Submit Control Request for CALIBRATION
    r.id = CS40L25_CONTROL_ID_CALIBRATION;
    r.cb = cb;
    r.cb_arg = cb_arg;
    // Pass in calibration type to control sm
    r.arg = (void *) calib_type;
    ret = cs40l25_functions_g->control(driver, r);

    return ret;
}

/**
 * Function pointer table for Public API implementation
 *
 * @attention Although not const, this should never be changed run-time in an end-product.  It is implemented this
 * way to facilitate unit testing.
 *
 */
static cs40l25_functions_t cs40l25_functions_s =
{
    .initialize = &cs40l25_initialize,
    .configure = &cs40l25_configure,
    .process = &cs40l25_process,
    .control = &cs40l25_control,
    .reset = &cs40l25_reset,
    .boot = &cs40l25_boot,
    .power = &cs40l25_power,
    .calibrate = &cs40l25_calibrate,
};

/**
 * Pointer to Public API implementation
 */
cs40l25_functions_t *cs40l25_functions_g = &cs40l25_functions_s;

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
