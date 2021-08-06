/**
 * @file cs35l41.c
 *
 * @brief The CS35L41 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019-2021 All Rights Reserved, http://www.cirrus.com/
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
#include "cs35l41.h"
#include "string.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * Default Interrupt Mask for IRQ1_MASK_1 register
 *
 * The interrupts that are unmasked in Interrupt Status and Mask Control (IRQ1) are:
 * - b31 - AMP_ERR_MASK1
 * - b17 - TEMP_ERR_MASK1
 * - b15 - TEMP_WARN_RISE_MASK1
 * - b8  - BST_SHORT_ERR_MASK1
 * - b7  - BST_DCM_UVP_ERR_MASK1
 * - b6  - BST_OVP_ERR_MASK1
 *
 * @see IRQ1_IRQ1_MASK_1_REG
 *
 */
#define CS35L41_INT1_MASK_DEFAULT               (0x7FFD7E3F)

/**
 * IRQ1 Status Bits for Speaker Safe Mode
 *
 * If any of the bits in the mask below are set in IRQ1_EINT_1, the amplifier will have entered Speaker Safe Mode.
 * - b31 - AMP_ERR_MASK1
 * - b17 - TEMP_ERR_MASK1
 * - b8  - BST_SHORT_ERR_MASK1
 * - b7  - BST_DCM_UVP_ERR_MASK1
 * - b6  - BST_OVP_ERR_MASK1
 *
 * @see IRQ1_EINT_1
 * @see Datasheet Section 4.16.1.1
 *
 */
#define CS35L41_INT1_SPEAKER_SAFE_MODE_IRQ_MASK (0x800201C0)

/**
 * IRQ1 Status Bits for Speaker Safe Mode Boost-related Events
 *
 * If any of the bits in the mask below are set in IRQ1_EINT_1, the amplifier will have entered Speaker Safe Mode
 * and will require additional steps to release from Speaker Safe Mode.
 * - b8 - BST_SHORT_ERR_MASK1
 * - b7 - BST_DCM_UVP_ERR_MASK1
 * - b6 - BST_OVP_ERR_MASK1
 *
 * @see IRQ1_EINT_1
 * @see Datasheet Section 4.16.1.1
 *
 */
#define CS35L41_INT1_BOOST_IRQ_MASK             (0x000001C0)

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
#define CS35L41_ERR_RLS_SPEAKER_SAFE_MODE_MASK  (0x0000007E)

/**
 * Value of CS35L41_CAL_STATUS that indicates Calibration success
 *
 * @see CS35L41_CAL_STATUS
 *
 */
#define CS35L41_CAL_STATUS_CALIB_SUCCESS        (0x1)

/**
 * Register address for the HALO FW Revision control
 *
 */
#define CS35L41_FIRMWARE_REVISION               (0x2800010)

/**
 * @defgroup CS35L41_DSP_MBOX_STATUS_
 * @brief Statuses of the HALO DSP Mailbox
 *
 * @{
 */
#define CS35L41_DSP_MBOX_STATUS_RUNNING         (0)
#define CS35L41_DSP_MBOX_STATUS_PAUSED          (1)
#define CS35L41_DSP_MBOX_STATUS_RDY_FOR_REINIT  (2)
#define CS35L41_DSP_MBOX_STATUS_HIBERNATE       (3)
/** @} */

/**
 * @defgroup CS35L41_DSP_MBOX_CMD_
 * @brief HALO DSP Mailbox commands
 *
 * @see cs35l41_t member mbox_cmd
 *
 * @{
 */
#define CS35L41_DSP_MBOX_CMD_NONE               (0)
#define CS35L41_DSP_MBOX_CMD_PAUSE              (1)
#define CS35L41_DSP_MBOX_CMD_RESUME             (2)
#define CS35L41_DSP_MBOX_CMD_REINIT             (3)
#define CS35L41_DSP_MBOX_CMD_STOP_PRE_REINIT    (4)
#define CS35L41_DSP_MBOX_CMD_HIBERNATE          (5)
#define CS35L41_DSP_MBOX_CMD_OUT_OF_HIBERNATE   (6)
#define CS35L41_DSP_MBOX_CMD_UNKNOWN            (-1)
/** @} */

/**
 * Maximum amount of times to poll for ACK to DSP Mailbox Command
 *
 */
#define CS35L41_POLL_ACKED_MBOX_CMD_MAX         (10)

/**
 * Maximum SPI clock speed during OTP Read
 *
 */
#define CS35L41_OTP_READ_MAX_SPI_CLOCK_HZ       (4000000)

#define CS35L41_OTP_MAP_BIT_OFFSET              (80)

/**
 * Entry in OTP Map of packed bitfield entries
 */
typedef struct
{
    uint32_t reg;   ///< Register address to trim
    uint8_t shift;  ///< Bitwise shift of register bitfield
    uint8_t size;   ///< Bitwise size of register bitfield
} cs35l41_otp_packed_entry_t;

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/**
 * CS35L41 RevB2 Register Patch Errata
 *
 * The array is in the form:
 * - word0 - Length of rest of patch in words (i.e. NOT including this word)
 * - word1 - address of TEST_KEY_CTRL
 * - word2 - 1st unlock value
 * - word3 - address of TEST_KEY_CTRL
 * - word4 - 2nd unlock value
 * - word5 - 1st register address to patch
 * - word6 - 1st register value
 * - word7 - 2nd register address to patch
 * - word8 - 2nd register value
 * - ...
 * - wordx - address of TEST_KEY_CTRL
 * - wordx - 1st lock value
 * - wordx - address of TEST_KEY_CTRL
 * - wordx - 2nd lock value
 *
 * @note To simplify the Reset SM, this includes the configuration for IRQ1 and INTb GPIO
 *
 */
static const uint32_t cs35l41_revb2_errata_patch[] =
{
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_1,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_2,
    0x00004100, 0x00000000,
    0x00004310, 0x00000000,
    0x00004400, 0x00000000,
    0x0000381C, 0x00000051,
    0x02BC20E0, 0x00000000,
    0x02BC2020, 0x00000000,
    IRQ1_IRQ1_MASK_1_REG, CS35L41_INT1_MASK_DEFAULT,    // Unmask IRQs
    PAD_INTF_GPIO_PAD_CONTROL_REG, 0x04000000,          // Set GPIO2 for INTb function
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_1,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_2,
};

/**
 * CS35L41 RevB2 OTP Map 1
 *
 * This mapping below maps the OTP bitfields of varying sizes to the Control Port register bitfields OTP is to trim.
 *
 * This is a list in the form cs35l41_otp_packed_entry_t, which is:
 * {
 *     {Register Address, Bitwise Shift, Bitwise Size}   //For first OTP bitfield
 *     {Register Address, Bitwise Shift, Bitwise Size}   //For second OTP bitfield
 *     ...
 * }
 *
 * Corresponds to OTPID 0x8
 *
 * @see cs35l41_otp_packed_entry_t
 *
 */
static const cs35l41_otp_packed_entry_t otp_map[] =
{
    /* addr         shift   size */
    {0x00002030,    0,      4}, /*TRIM_OSC_FREQ_TRIM*/
    {0x00002030,    7,      1}, /*TRIM_OSC_TRIM_DONE*/
    {0x0000208c,    24,     6}, /*TST_DIGREG_VREF_TRIM*/
    {0x00002090,    14,     4}, /*TST_REF_TRIM*/
    {0x00002090,    10,     4}, /*TST_REF_TEMPCO_TRIM*/
    {0x0000300C,    11,     4}, /*PLL_LDOA_TST_VREF_TRIM*/
    {0x0000394C,    23,     2}, /*BST_ATEST_CM_VOFF*/
    {0x00003950,    0,      7}, /*BST_ATRIM_IADC_OFFSET*/
    {0x00003950,    8,      7}, /*BST_ATRIM_IADC_GAIN1*/
    {0x00003950,    16,     8}, /*BST_ATRIM_IPKCOMP_OFFSET1*/
    {0x00003950,    24,     8}, /*BST_ATRIM_IPKCOMP_GAIN1*/
    {0x00003954,    0,      7}, /*BST_ATRIM_IADC_OFFSET2*/
    {0x00003954,    8,      7}, /*BST_ATRIM_IADC_GAIN2*/
    {0x00003954,    16,     8}, /*BST_ATRIM_IPKCOMP_OFFSET2*/
    {0x00003954,    24,     8}, /*BST_ATRIM_IPKCOMP_GAIN2*/
    {0x00003958,    0,      7}, /*BST_ATRIM_IADC_OFFSET3*/
    {0x00003958,    8,      7}, /*BST_ATRIM_IADC_GAIN3*/
    {0x00003958,    16,     8}, /*BST_ATRIM_IPKCOMP_OFFSET3*/
    {0x00003958,    24,     8}, /*BST_ATRIM_IPKCOMP_GAIN3*/
    {0x0000395C,    0,      7}, /*BST_ATRIM_IADC_OFFSET4*/
    {0x0000395C,    8,      7}, /*BST_ATRIM_IADC_GAIN4*/
    {0x0000395C,    16,     8}, /*BST_ATRIM_IPKCOMP_OFFSET4*/
    {0x0000395C,    24,     8}, /*BST_ATRIM_IPKCOMP_GAIN4*/
    {0x0000416C,    0,      8}, /*VMON_GAIN_OTP_VAL*/
    {0x00004160,    0,      7}, /*VMON_OFFSET_OTP_VAL*/
    {0x0000416C,    8,      8}, /*IMON_GAIN_OTP_VAL*/
    {0x00004160,    16,     10}, /*IMON_OFFSET_OTP_VAL*/
    {0x0000416C,    16,     12}, /*VMON_CM_GAIN_OTP_VAL*/
    {0x0000416C,    28,     1}, /*VMON_CM_GAIN_SIGN_OTP_VAL*/
    {0x00004170,    0,      6}, /*IMON_CAL_TEMPCO_OTP_VAL*/
    {0x00004170,    6,      1}, /*IMON_CAL_TEMPCO_SIGN_OTP*/
    {0x00004170,    8,      6}, /*IMON_CAL_TEMPCO2_OTP_VAL*/
    {0x00004170,    14,     1}, /*IMON_CAL_TEMPCO2_DN_UPB_OTP_VAL*/
    {0x00004170,    16,     9}, /*IMON_CAL_TEMPCO_TBASE_OTP_VAL*/
    {0x00004360,    0,      5}, /*TEMP_GAIN_OTP_VAL*/
    {0x00004360,    6,      9}, /*TEMP_OFFSET_OTP_VAL*/
    {0x00004448,    0,      8}, /*VP_SARADC_OFFSET*/
    {0x00004448,    8,      8}, /*VP_GAIN_INDEX*/
    {0x00004448,    16,     8}, /*VBST_SARADC_OFFSET*/
    {0x00004448,    24,     8}, /*VBST_GAIN_INDEX*/
    {0x0000444C,    0,      3}, /*ANA_SELINVREF*/
    {0x00006E30,    0,      5}, /*GAIN_ERR_COEFF_0*/
    {0x00006E30,    8,      5}, /*GAIN_ERR_COEFF_1*/
    {0x00006E30,    16,     5}, /*GAIN_ERR_COEFF_2*/
    {0x00006E30,    24,     5}, /*GAIN_ERR_COEFF_3*/
    {0x00006E34,    0,      5}, /*GAIN_ERR_COEFF_4*/
    {0x00006E34,    8,      5}, /*GAIN_ERR_COEFF_5*/
    {0x00006E34,    16,     5}, /*GAIN_ERR_COEFF_6*/
    {0x00006E34,    24,     5}, /*GAIN_ERR_COEFF_7*/
    {0x00006E38,    0,      5}, /*GAIN_ERR_COEFF_8*/
    {0x00006E38,    8,      5}, /*GAIN_ERR_COEFF_9*/
    {0x00006E38,    16,     5}, /*GAIN_ERR_COEFF_10*/
    {0x00006E38,    24,     5}, /*GAIN_ERR_COEFF_11*/
    {0x00006E3C,    0,      5}, /*GAIN_ERR_COEFF_12*/
    {0x00006E3C,    8,      5}, /*GAIN_ERR_COEFF_13*/
    {0x00006E3C,    16,     5}, /*GAIN_ERR_COEFF_14*/
    {0x00006E3C,    24,     5}, /*GAIN_ERR_COEFF_15*/
    {0x00006E40,    0,      5}, /*GAIN_ERR_COEFF_16*/
    {0x00006E40,    8,      5}, /*GAIN_ERR_COEFF_17*/
    {0x00006E40,    16,     5}, /*GAIN_ERR_COEFF_18*/
    {0x00006E40,    24,     5}, /*GAIN_ERR_COEFF_19*/
    {0x00006E44,    0,      5}, /*GAIN_ERR_COEFF_20*/
    {0x00006E48,    0,      10}, /*VOFF_GAIN_0*/
    {0x00006E48,    10,     10}, /*VOFF_GAIN_1*/
    {0x00006E48,    20,     10}, /*VOFF_GAIN_2*/
    {0x00006E4C,    0,      10}, /*VOFF_GAIN_3*/
    {0x00006E4C,    10,     10}, /*VOFF_GAIN_4*/
    {0x00006E4C,    20,     10}, /*VOFF_GAIN_5*/
    {0x00006E50,    0,      10}, /*VOFF_GAIN_6*/
    {0x00006E50,    10,     10}, /*VOFF_GAIN_7*/
    {0x00006E50,    20,     10}, /*VOFF_GAIN_8*/
    {0x00006E54,    0,      10}, /*VOFF_GAIN_9*/
    {0x00006E54,    10,     10}, /*VOFF_GAIN_10*/
    {0x00006E54,    20,     10}, /*VOFF_GAIN_11*/
    {0x00006E58,    0,      10}, /*VOFF_GAIN_12*/
    {0x00006E58,    10,     10}, /*VOFF_GAIN_13*/
    {0x00006E58,    20,     10}, /*VOFF_GAIN_14*/
    {0x00006E5C,    0,      10}, /*VOFF_GAIN_15*/
    {0x00006E5C,    10,     10}, /*VOFF_GAIN_16*/
    {0x00006E5C,    20,     10}, /*VOFF_GAIN_17*/
    {0x00006E60,    0,      10}, /*VOFF_GAIN_18*/
    {0x00006E60,    10,     10}, /*VOFF_GAIN_19*/
    {0x00006E60,    20,     10}, /*VOFF_GAIN_20*/
    {0x00006E64,    0,      10}, /*VOFF_INT1*/
    {0x00007418,    7,      5}, /*DS_SPK_INT1_CAP_TRIM*/
    {0x0000741C,    0,      5}, /*DS_SPK_INT2_CAP_TRIM*/
    {0x0000741C,    11,     4}, /*DS_SPK_LPF_CAP_TRIM*/
    {0x0000741C,    19,     4}, /*DS_SPK_QUAN_CAP_TRIM*/
    {0x00007434,    17,     1}, /*FORCE_CAL*/
    {0x00007434,    18,     7}, /*CAL_OVERRIDE*/
    {0x00007068,    0,      9}, /*MODIX*/
    {0x0000410C,    7,      1}, /*VIMON_DLY_NOT_COMB*/
    {0x0000400C,    0,      7}, /*VIMON_DLY*/
    {0x00000000,    0,      1}, /*extra bit*/
    {0x00017040,    0,      8}, /*X_COORDINATE*/
    {0x00017040,    8,      8}, /*Y_COORDINATE*/
    {0x00017040,    16,     8}, /*WAFER_ID*/
    {0x00017040,    24,     8}, /*DVS*/
    {0x00017044,    0,      24}, /*LOT_NUMBER*/
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
static const uint32_t cs35l41_post_boot_config[] =
{
    CS35L41_MIXER_DSP1RX5_INPUT_REG, CS35L41_INPUT_SRC_VPMON,
    CS35L41_MIXER_DSP1RX6_INPUT_REG, CS35L41_INPUT_SRC_CLASSH,
    CS35L41_MIXER_DSP1RX7_INPUT_REG, CS35L41_INPUT_SRC_TEMPMON,
    CS35L41_MIXER_DSP1RX8_INPUT_REG, CS35L41_INPUT_SRC_RSVD
};

/**
 * Register configuration to send just before the CS35L41 is powered up in Power Up SM
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
 * @see cs35l41_power_up
 *
 */
static const uint32_t cs35l41_pup_patch[] =
{
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_1,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_2,
    0x00002084, 0x002F1AA0,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_1,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_2,
};

/**
 * Register configuration to send just after the CS35L41 is powered down in Power Down SM
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
 * @see cs35l41_power_down
 *
 */
static const uint32_t cs35l41_pdn_patch[] =
{
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_1,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_2,
    0x00002084, 0x002F1AA3,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_1,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_2,
};

/**
 * Register configuration to lock HALO memory regions
 *
 * Sent just before the CS35L41 is powered up in Power Up SM.
 *
 * List is in the form:
 * - word1 - address of DSP1_MPU_LOCK_CONFIG
 * - word2 - 1st unlock value
 * - word3 - address of DSP1_MPU_LOCK_CONFIG
 * - word4 - 2nd unlock value
 * - word5 - Address of first configuration register
 * - word6 - Value of first configuration register
 * - word7 - Address of second configuration register
 * - word8 - Value of second configuration register
 * - ...
 * - wordx - address of DSP1_MPU_LOCK_CONFIG
 * - wordx - 1st lock value
 *
 * @see cs35l41_power_up
 *
 */
static const uint32_t cs35l41_mem_lock[] =
{
    XM_UNPACKED24_DSP1_MPU_LOCK_CONFIG_REG,     0x00005555,
    XM_UNPACKED24_DSP1_MPU_LOCK_CONFIG_REG,     0x0000AAAA,
    XM_UNPACKED24_DSP1_MPU_XMEM_ACCESS_0_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_YMEM_ACCESS_0_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_WINDOW_ACCESS_0_REG, 0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_XREG_ACCESS_0_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_YREG_ACCESS_0_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_XMEM_ACCESS_1_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_YMEM_ACCESS_1_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_WINDOW_ACCESS_1_REG, 0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_XREG_ACCESS_1_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_YREG_ACCESS_1_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_XMEM_ACCESS_2_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_YMEM_ACCESS_2_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_WINDOW_ACCESS_2_REG, 0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_XREG_ACCESS_2_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_YREG_ACCESS_2_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_XMEM_ACCESS_3_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_YMEM_ACCESS_3_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_WINDOW_ACCESS_3_REG, 0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_XREG_ACCESS_3_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_YREG_ACCESS_3_REG,   0xFFFFFFFF,
    XM_UNPACKED24_DSP1_MPU_LOCK_CONFIG_REG,     0x00000000
};

/**
 * Register addresses to set all HALO sample rates to the same value.
 *
 * Sent just before the CS35L41 is powered up in Power Up SM.  All register values will be set to
 * CS35L41_DSP1_SAMPLE_RATE_G1R2.
 *
 * List is in the form:
 * - word0 - Address of first configuration register
 * - word1 - Address of second configuration register
 * - ...
 *
 * @see cs35l41_power_up
 * @see CS35L41_DSP1_SAMPLE_RATE_G1R2
 *
 */
static const uint32_t cs35l41_frame_sync_regs[] =
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
 * Register/DSP Memory addresses to read during Get DSP Status
 *
 * List is in the form:
 * - word0 - Address of first status register
 * - word1 - Address of second status register
 * - ...
 *
 * @see cs35l41_get_dsp_status
 * @see cs35l41_dsp_status_t
 *
 * @warning  The list of registers MUST correspond to the union of structs in  in cs35l41_dsp_status_t.
 *
 */
static const uint32_t cs35l41_dsp_status_controls[CS35L41_DSP_STATUS_WORDS_TOTAL] =
{
        CS35L41_SYM_FIRMWARE_HALO_CSPL_HALO_STATE,
        CS35L41_SYM_FIRMWARE_HALO_CSPL_HALO_HEARTBEAT,
        CS35L41_SYM_CSPL_CSPL_STATE,
        CS35L41_SYM_CSPL_CAL_SET_STATUS,
        CS35L41_SYM_CSPL_CAL_R_SELECTED,
        CS35L41_SYM_CSPL_CAL_R,
        CS35L41_SYM_CSPL_CAL_STATUS,
        CS35L41_SYM_CSPL_CAL_CHECKSUM,
        CS35L41_SYM_CSPL_CSPL_TEMPERATURE
};

/**
 * Mapping of CS35L41 IRQ Flag to Event Flag
 *
 * List is in the form:
 * - word0 - IRQ Flag
 * - word1 - Event Flag
 * - ...
 *
 * @see cs35l41_irq_to_event_id
 *
 */
uint32_t cs35l41_irq_to_event_flag_map[] =
{
    IRQ1_IRQ1_EINT_1_AMP_ERR_EINT1_BITMASK, CS35L41_EVENT_FLAG_AMP_SHORT,
    IRQ1_IRQ1_EINT_1_TEMP_ERR_EINT1_BITMASK, CS35L41_EVENT_FLAG_OVERTEMP,
    IRQ1_IRQ1_EINT_1_BST_SHORT_ERR_EINT1_BITMASK, CS35L41_EVENT_FLAG_BOOST_INDUCTOR_SHORT,
    IRQ1_IRQ1_EINT_1_BST_DCM_UVP_ERR_EINT1_BITMASK, CS35L41_EVENT_FLAG_BOOST_UNDERVOLTAGE,
    IRQ1_IRQ1_EINT_1_BST_OVP_ERR_EINT1_BITMASK, CS35L41_EVENT_FLAG_BOOST_OVERVOLTAGE
};

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Notify the driver when the CS35L41 INTb GPIO drops low.
 *
 * This callback is registered with the BSP in the register_gpio_cb() API call.
 *
 * The primary task of this callback is to transition the driver mode from CS35L41_MODE_HANDLING_CONTROLS to
 * CS35L41_MODE_HANDLING_EVENTS, in order to signal to the main thread to process events.
 *
 * @param [in] status           BSP status for the INTb IRQ.
 * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
 *                              pointer to the driver state cs35l41_t.
 *
 * @return none
 *
 * @see bsp_driver_if_t member register_gpio_cb.
 * @see bsp_callback_t
 *
 */
static void cs35l41_irq_callback(uint32_t status, void *cb_arg)
{
    cs35l41_t *d;

    d = (cs35l41_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Switch driver mode to CS35L41_MODE_HANDLING_EVENTS
        d->mode = CS35L41_MODE_HANDLING_EVENTS;
    }

    return;
}

/**
 * Applies OTP trim bit-field to current register word value.
 *
 * During the Reset SM, trim bit-fields must be tweezed from OTP and applied to corresponding Control Port register
 * contents.
 *
 * @param [in] otp_mem          pointer byte array consisting of entire contents of OTP
 * @param [in] bit_count        current bit index into otp_mem, i.e. location of bit-field in OTP memory
 * @param [in,out] reg_val      contents of register to modify with trim bit-field
 * @param [in] shift            location of bit-field in control port register in terms of bits from bit 0
 * @param [in] size             size of bit-field in bits
 *
 * @return
 * - CS35L41_STATUS_FAIL        if any pointers are NULL or if the bit-field size is 0
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_apply_trim_word(uint8_t *otp_mem,
                                        uint32_t bit_count,
                                        uint32_t *reg_val,
                                        uint32_t shift,
                                        uint32_t size)
{
    uint32_t ret = CS35L41_STATUS_OK;

    if ((otp_mem == NULL) || (reg_val == NULL) || (size == 0))
    {
        ret = CS35L41_STATUS_FAIL;
    }
    else
    {
        // Create bit-field mask to use on OTP contents
        uint32_t bitmask = ~(0xFFFFFFFF << size);
        uint64_t otp_bits = 0;  // temporary storage of bit-field
        // Using bit_count, get index of current 32-bit word in otp_mem
        uint32_t otp_mem_word_index = bit_count >> 5; // divide by 32
        // Get position of current bit in the current word in otp_mem
        uint32_t otp_mem_msword_bit_index = bit_count - (otp_mem_word_index << 5);

        // Skip ahead to the current 32-bit word
        otp_mem += ((otp_mem_word_index) * sizeof(uint32_t));

        // Shift the first 32-bit word into register - OTP bytes come over I2C in Little-Endian 32-bit words!
        otp_bits |= *(otp_mem++);
        otp_bits <<= 8;
        otp_bits |= *(otp_mem++);
        otp_bits <<= 8;
        otp_bits |= *(otp_mem++);
        otp_bits <<= 8;
        otp_bits |= *(otp_mem++);

        // If there's bits to get in the second 32-bit word, get them
        if ((size + otp_mem_msword_bit_index) > 32)
        {
            uint64_t temp_word = 0;
            temp_word |= *(otp_mem++);
            temp_word <<= 8;
            temp_word |= *(otp_mem++);
            temp_word <<= 8;
            temp_word |= *(otp_mem++);
            temp_word <<= 8;
            temp_word |= *(otp_mem++);

            otp_bits |= temp_word << 32;
        }

        // Right-justify the bits to get from OTP
        otp_bits >>= otp_mem_msword_bit_index;
        // Get only required number of OTP bits
        otp_bits &= bitmask;

        // Mask off bits in the current register value
        bitmask <<= shift;
        *reg_val &= ~(bitmask);

        // Or the OTP bits into the current register value
        *reg_val |= (otp_bits << shift);
    }

    return ret;
}

/**
 * Check HALO MBOX Status against the MBOX Command sent.
 *
 * Only some states of HALO MBOX Status are valid for each HALO MBOX Command
 *
 * @param [in] cmd              which HALO MBOX Command was most recently sent
 * @param [in] status           what is the HALO MBOX Status read
 *
 * @return
 * - true                       Status is correct/valid
 * - false                      Status is incorrect/invalid
 *
 */
static bool cs35l41_is_mbox_status_correct(uint32_t cmd, uint32_t status)
{
    switch (cmd)
    {
        case CS35L41_DSP_MBOX_CMD_NONE:
            // For 'NONE' - all statuses are valid
        case CS35L41_DSP_MBOX_CMD_UNKNOWN:
            // For 'UNKNOWN' - all statuses are valid
            return true;
        case CS35L41_DSP_MBOX_CMD_PAUSE:
            // For 'PAUSE' - only valid if status is 'PAUSED'
            return (status == CS35L41_DSP_MBOX_STATUS_PAUSED);
        case CS35L41_DSP_MBOX_CMD_RESUME:
        case CS35L41_DSP_MBOX_CMD_REINIT:
            // For 'RESUME' and 'REINIT' - only valid if status is 'RUNNING'
            return (status == CS35L41_DSP_MBOX_STATUS_RUNNING);
        case CS35L41_DSP_MBOX_CMD_STOP_PRE_REINIT:
            // For 'STOP_PRE_REINIT' - only valid if status is 'RDY_FOR_REINIT'
            return (status == CS35L41_DSP_MBOX_STATUS_RDY_FOR_REINIT);
        default:
            return false;
    }
}

/**
 * Send a HALO Core mailbox command and check the status.
 *
 * This will send a HALO Core mailbox command to the Virtual Mailbox 1 and check the response in Virtual Mailbox 2.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] cmd              Virtual Mailbox 1 command
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Polling of a status bit times out
 *      - Incorrect/unexpected values of Virtual MBOX transactions
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_send_acked_mbox_cmd(cs35l41_t *driver, uint32_t cmd)
{
    uint32_t ret = CS35L41_STATUS_OK;
    uint32_t i;
    uint32_t temp_reg_val;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Clear HALO DSP Virtual MBOX 1 IRQ flag
    ret = regmap_write(cp, IRQ2_IRQ2_EINT_2_REG, IRQ2_IRQ2_EINT_2_DSP_VIRTUAL1_MBOX_WR_EINT2_BITMASK);
    if (ret)
    {
        return ret;
    }

    // Clear HALO DSP Virtual MBOX 2 IRQ flag
    ret = regmap_write(cp, IRQ1_IRQ1_EINT_2_REG, IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK);
    if (ret)
    {
        return ret;
    }

    // Read IRQ2 Mask register
    ret = regmap_read(cp, IRQ2_IRQ2_MASK_2_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Clear HALO DSP Virtual MBOX 1 IRQ mask
    temp_reg_val &= ~(IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK);
    ret = regmap_write(cp, IRQ2_IRQ2_MASK_2_REG, temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Send HALO DSP MBOX Command
    ret = regmap_write(cp, DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG, cmd);
    if (ret)
    {
        return ret;
    }

    for (i = 0; i < CS35L41_POLL_ACKED_MBOX_CMD_MAX; i++)
    {
        // Read IRQ1 flag register to poll for MBOX IRQ
        regmap_read(cp, IRQ1_IRQ1_EINT_2_REG, &temp_reg_val);

        if (temp_reg_val & IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK)
        {
            break;
        }

        bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, NULL, NULL);
    }

    if (i >= CS35L41_POLL_ACKED_MBOX_CMD_MAX)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Clear MBOX IRQ flag
    ret = regmap_write(cp, IRQ1_IRQ1_EINT_2_REG, IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK);
    if (ret)
    {
        return ret;
    }

    // Read IRQ2 Mask register to re-mask HALO DSP Virtual MBOX 1 IRQ
    ret = regmap_read(cp, IRQ2_IRQ2_MASK_2_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }
    // Re-mask HALO DSP Virtual MBOX 1 IRQ
    temp_reg_val |= IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK;
    ret = regmap_write(cp, IRQ2_IRQ2_MASK_2_REG, temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Read the MBOX status
    ret = regmap_read(cp, DSP_MBOX_DSP_MBOX_2_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Check that MBOX status is correct for command just sent
    if (!(cs35l41_is_mbox_status_correct(cmd, temp_reg_val)))
    {
        return CS35L41_STATUS_FAIL;
    }

    return ret;
}

/**
 * Power up from Standby
 *
 * This function performs all necessary steps to transition the CS35L41 to be ready to pass audio through the
 * amplifier DAC.  Completing this results in the driver transition to POWER_UP state.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Incorrect/unexpected values of Virtual MBOX transactions
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_power_up(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    uint32_t i;
    uint32_t temp_reg_val;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    //If the DSP is booted
    if (driver->state != CS35L41_STATE_STANDBY)
    {
        // Send HALO DSP Memory Lock sequence
        ret = regmap_write_array(cp, (uint32_t *) cs35l41_mem_lock, (sizeof(cs35l41_mem_lock)/sizeof(uint32_t)));
        if (ret)
        {
            return CS35L41_STATUS_FAIL;
        }

        // Set next HALO DSP Sample Rate register to G1R2
        for (i = 0; i < (sizeof(cs35l41_frame_sync_regs)/sizeof(uint32_t)); i++)
        {
            ret = regmap_write(cp, cs35l41_frame_sync_regs[i], CS35L41_DSP1_SAMPLE_RATE_G1R2);
            if (ret)
            {
                return ret;
            }
        }

        // Read the HALO DSP CCM control register
        ret = regmap_read(cp, XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG, &temp_reg_val);
        if (ret)
        {
            return ret;
        }

        // Enable clocks to HALO DSP core
        temp_reg_val |= XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_EN_BITMASK;
        ret = regmap_write(cp, XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG, temp_reg_val);
        if (ret)
        {
            return ret;
        }
    }

    // Send Power Up Patch
    ret = regmap_write_array(cp, (uint32_t *) cs35l41_pup_patch, (sizeof(cs35l41_pup_patch)/sizeof(uint32_t)));
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Read GLOBAL_EN register
    ret = regmap_read(cp, MSM_GLOBAL_ENABLES_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }
    temp_reg_val |= MSM_GLOBAL_ENABLES_GLOBAL_EN_BITMASK;
    //Set GLOBAL_EN
    ret = regmap_write(cp, MSM_GLOBAL_ENABLES_REG, temp_reg_val);
    if (ret)
    {
        return ret;
    }

    //Wait 1ms
    bsp_driver_if_g->set_timer(CS35L41_T_AMP_PUP_MS, NULL, NULL);

    // If DSP is NOT booted, then power up is finished
    if (driver->state == CS35L41_STATE_STANDBY)
    {
        return CS35L41_STATUS_OK;
    }


    // Clear HALO DSP Virtual MBOX 1 IRQ
    ret = regmap_write(cp, IRQ2_IRQ2_EINT_2_REG, IRQ2_IRQ2_EINT_2_DSP_VIRTUAL1_MBOX_WR_EINT2_BITMASK);
    if (ret)
    {
        return ret;
    }
    // Clear HALO DSP Virtual MBOX 2 IRQ
    ret = regmap_write(cp, IRQ1_IRQ1_EINT_2_REG, IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK);
    if (ret)
    {
        return ret;
    }

    // Read IRQ2 Mask register
    ret = regmap_read(cp, IRQ2_IRQ2_MASK_2_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }
    // Unmask IRQ for HALO DSP Virtual MBOX 1
    temp_reg_val &= ~(IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK);
    ret = regmap_write(cp, IRQ2_IRQ2_MASK_2_REG, temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Read HALO DSP MBOX Space 2 register
    ret = regmap_read(cp, DSP_MBOX_DSP_MBOX_2_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }

    uint32_t mbox_cmd = CS35L41_DSP_MBOX_CMD_NONE;

    // Based on MBOX status, select correct MBOX Command
    switch (temp_reg_val)
    {
        case CS35L41_DSP_MBOX_STATUS_RDY_FOR_REINIT:
            mbox_cmd = CS35L41_DSP_MBOX_CMD_REINIT;
            break;

        case CS35L41_DSP_MBOX_STATUS_PAUSED:
        case CS35L41_DSP_MBOX_STATUS_RUNNING:
            mbox_cmd = CS35L41_DSP_MBOX_CMD_RESUME;
            break;

        default:
            break;
    }

    // If no command found, indicate ERROR
    if (mbox_cmd == CS35L41_DSP_MBOX_CMD_NONE)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Write MBOX command
    ret = regmap_write(cp, DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG, mbox_cmd);
    if (ret)
    {
        return ret;
    }

    i = 5;
    for (i = 0; i < 5; i++)
    {
        // Wait for at least 1ms
        bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, NULL, NULL);

        // Poll MBOX IRQ flag
        ret = regmap_read(cp, IRQ1_IRQ1_EINT_2_REG, &temp_reg_val);
        if (ret)
        {
            return ret;
        }

        if (temp_reg_val & IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK)
        {
            break;
        }
    }

    regmap_read(cp, 0x00010098, &temp_reg_val); // Read IRQ1_STS_3
    if (i == 5)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Clear MBOX IRQ
    ret = regmap_write(cp, IRQ1_IRQ1_EINT_2_REG, IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK);
    if (ret)
    {
        return ret;
    }

    // Read IRQ2 Mask register to next re-mask the MBOX IRQ
    ret = regmap_read(cp, IRQ2_IRQ2_MASK_2_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }
    // Re-mask the MBOX IRQ
    temp_reg_val |= IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK;
    ret = regmap_write(cp, IRQ2_IRQ2_MASK_2_REG, temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Read the HALO DSP MBOX status
    ret = regmap_read(cp, DSP_MBOX_DSP_MBOX_2_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Check if the status is correct for the command just sent
    if (cs35l41_is_mbox_status_correct(mbox_cmd, temp_reg_val))
    {
        return CS35L41_STATUS_OK;
    }
    else
    {
        return CS35L41_STATUS_FAIL;
    }
}

/**
 * Power down to Standby
 *
 * This function performs all necessary steps to transition the CS35L41 to be in Standby power mode. Completing
 * this results in the driver transition to STANDBY or DSP_STANDBY state.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Incorrect/unexpected values of Virtual MBOX transactions
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_power_down(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    uint32_t temp_reg_val;
    uint32_t i;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    if (driver->state != CS35L41_STATE_POWER_UP)
    {
        // Clear HALO DSP Virtual MBOX 1 IRQ flag
        ret = regmap_write(cp, IRQ2_IRQ2_EINT_2_REG, IRQ2_IRQ2_EINT_2_DSP_VIRTUAL1_MBOX_WR_EINT2_BITMASK);
        if (ret)
        {
            return ret;
        }

        // Clear HALO DSP Virtual MBOX 2 IRQ flag
        ret = regmap_write(cp, IRQ1_IRQ1_EINT_2_REG, IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK);
        if (ret)
        {
            return ret;
        }

        // Read IRQ2 Mask register
        ret = regmap_read(cp, IRQ2_IRQ2_MASK_2_REG, &temp_reg_val);
        if (ret)
        {
            return ret;
        }

        // Clear HALO DSP Virtual MBOX 1 IRQ mask
        temp_reg_val &= ~(IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK);
        ret = regmap_write(cp, IRQ2_IRQ2_MASK_2_REG, temp_reg_val);
        if (ret)
        {
            return ret;
        }

        // Send HALO DSP MBOX 'Pause' Command
        ret = regmap_write(cp, DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG, CS35L41_DSP_MBOX_CMD_PAUSE);
        if (ret)
        {
            return ret;
        }

        // Wait for at least 1ms
        bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, NULL, NULL);

        for (i = 0; i < 5; i++)
        {
            // Read IRQ1 flag register to poll for MBOX IRQ
            regmap_read(cp, IRQ1_IRQ1_EINT_2_REG, &temp_reg_val);

            if (temp_reg_val & IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK)
            {
                break;
            }

            bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, NULL, NULL);
        }

        if (i == 5)
        {
            return CS35L41_STATUS_FAIL;
        }

        // Clear MBOX IRQ flag
        ret = regmap_write(cp, IRQ1_IRQ1_EINT_2_REG, IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK);
        if (ret)
        {
            return ret;
        }

        // Read IRQ2 Mask register to re-mask HALO DSP Virtual MBOX 1 IRQ
        ret = regmap_read(cp, IRQ2_IRQ2_MASK_2_REG, &temp_reg_val);
        if (ret)
        {
            return ret;
        }
        // Re-mask HALO DSP Virtual MBOX 1 IRQ
        temp_reg_val |= IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK;
        ret = regmap_write(cp, IRQ2_IRQ2_MASK_2_REG, temp_reg_val);
        if (ret)
        {
            return ret;
        }

        // Read the MBOX status
        ret = regmap_read(cp, DSP_MBOX_DSP_MBOX_2_REG, &temp_reg_val);
        if (ret)
        {
            return ret;
        }

        // Check that MBOX status is correct for 'Pause' command just sent
        if (!(cs35l41_is_mbox_status_correct(CS35L41_DSP_MBOX_CMD_PAUSE, temp_reg_val)))
        {
            return CS35L41_STATUS_FAIL;
        }
    }

    // Read GLOBAL_EN register in order to clear GLOBAL_EN
    ret = regmap_read(cp, MSM_GLOBAL_ENABLES_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Clear GLOBAL_EN
    temp_reg_val &= ~(MSM_GLOBAL_ENABLES_GLOBAL_EN_BITMASK);
    ret = regmap_write(cp, MSM_GLOBAL_ENABLES_REG, temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Read IRQ1 flag register to poll MSM_PDN_DONE bit
    i = 100;
    do
    {
        ret = regmap_read(cp, IRQ1_IRQ1_EINT_1_REG, &temp_reg_val);
        if (ret)
        {
            return ret;
        }

        if (temp_reg_val & IRQ1_IRQ1_EINT_1_MSM_PDN_DONE_EINT1_BITMASK)
        {
            break;
        }

        bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_1MS, NULL, NULL);

        i--;
    } while (i > 0);

    if (i == 0)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Clear MSM_PDN_DONE IRQ flag
    ret = regmap_write(cp, IRQ1_IRQ1_EINT_1_REG, IRQ1_IRQ1_EINT_1_MSM_PDN_DONE_EINT1_BITMASK);
    if (ret)
    {
        return ret;
    }

    // Send Power Down Patch set
    ret = regmap_write_array(cp, (uint32_t *) cs35l41_pdn_patch, (sizeof(cs35l41_pdn_patch)/sizeof(uint32_t)));
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    return CS35L41_STATUS_OK;
}

/**
 * Maps IRQ Flag to Event ID passed to BSP
 *
 * Allows for abstracting driver events relayed to BSP away from IRQ flags, to allow the possibility that multiple
 * IRQ flags correspond to a single event to relay.
 *
 * @param [in] irq_statuses     pointer to array of 32-bit words from IRQ1_IRQ1_EINT_*_REG registers
 *
 * @return                      32-bit word with CS35L41_EVENT_FLAG_* set for each event detected
 *
 * @see CS35L41_EVENT_FLAG_
 *
 */
static uint32_t cs35l41_irq_to_event_id(uint32_t *irq_statuses)
{
    uint32_t temp_event_flag = 0;

    for (uint8_t i = 0; i < (sizeof(cs35l41_irq_to_event_flag_map)/sizeof(uint32_t)); i+= 2)
    {
        if (irq_statuses[0] & cs35l41_irq_to_event_flag_map[i])
        {
            temp_event_flag |= (1 << cs35l41_irq_to_event_flag_map[i + 1]);
        }
    }

    return temp_event_flag;
}

/**
 * Handle events indicated by the IRQ pin ALERTb
 *
 * This function performs all steps to handle IRQ and other asynchronous events the driver is aware of,
 * resulting in calling of the notification callback (cs35l41_notification_callback_t).
 *
 * If there are any IRQ events that include Speaker-Safe Mode Errors or Boost-related events, then the procedure
 * outlined in the Datasheet Section 4.16.1.1 is implemented here.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAI         Control port activity fails
 * - CS35L41_STATUS_OK          otherwise
 *
 * @see CS35L41_EVENT_FLAG_
 * @see cs35l41_notification_callback_t
 *
 */
static uint32_t cs35l41_event_handler(void *driver)
{
    uint8_t i;
    uint32_t temp_reg_val;
    uint32_t ret = CS35L41_STATUS_OK;
    uint32_t irq_statuses[4];
    uint32_t irq_masks[4];

    cs35l41_t *d = driver;
    regmap_cp_config_t *cp = REGMAP_GET_CP(d);

    // Read the IRQ1 flag and mask registers
    uint32_t irq1_eint_1_flags_to_clear = 0;

    for (i = 0; i < (sizeof(irq_statuses)/sizeof(uint32_t)); i++)
    {
        uint32_t flags_to_clear = 0;

        ret = regmap_read(cp, (IRQ1_IRQ1_EINT_1_REG + (i * 4)), &(irq_statuses[i]));
        if (ret)
        {
            return ret;
        }

        ret = regmap_read(cp, (IRQ1_IRQ1_MASK_1_REG + (i * 4)), &(irq_masks[i]));
        if (ret)
        {
            return ret;
        }

        flags_to_clear = irq_statuses[i] & ~(irq_masks[i]);
        if (i == 0)
        {
            irq1_eint_1_flags_to_clear = flags_to_clear;
        }

        // If there are unmasked IRQs, then process
        if (flags_to_clear)
        {
            // Clear any IRQ1 flags from first register
            ret = regmap_write(cp, (IRQ1_IRQ1_EINT_1_REG + (i * 4)), flags_to_clear);
            if (ret)
            {
                return ret;
            }
        }
    }

    if (!irq1_eint_1_flags_to_clear)
    {
        return CS35L41_STATUS_OK;
    }

    // IF there are no Boost-related Errors but are Speaker-Safe Mode errors, proceed to TOGGLE_ERR_RLS
    if (irq_statuses[0] & CS35L41_INT1_SPEAKER_SAFE_MODE_IRQ_MASK)
    {
        // If there are Boost-related Errors, proceed to DISABLE_BOOST
        if (irq_statuses[0] & CS35L41_INT1_BOOST_IRQ_MASK)
        {
            // Read which MSM Blocks are enabled
            ret = regmap_read(cp, MSM_BLOCK_ENABLES_REG, &temp_reg_val);
            if (ret)
            {
                return ret;
            }
            // Disable Boost converter
            temp_reg_val &= ~(MSM_BLOCK_ENABLES_BST_EN_BITMASK);
            ret = regmap_write(cp, MSM_BLOCK_ENABLES_REG, temp_reg_val);
            if (ret)
            {
                return ret;
            }
        }

        // Clear the Error Release register
        ret = regmap_write(cp, MSM_ERROR_RELEASE_REG, 0);
        if (ret)
        {
            return ret;
        }
        // Set the Error Release register
        ret = regmap_write(cp, MSM_ERROR_RELEASE_REG, CS35L41_ERR_RLS_SPEAKER_SAFE_MODE_MASK);
        if (ret)
        {
            return ret;
        }
        // Clear the Error Release register
        ret = regmap_write(cp, MSM_ERROR_RELEASE_REG, 0);
        if (ret)
        {
            return ret;
        }

        // If there are Boost-related Errors, re-enable Boost
        if (irq_statuses[0] & CS35L41_INT1_BOOST_IRQ_MASK)
        {
            // Read register containing BST_EN
            ret = regmap_read(cp, MSM_BLOCK_ENABLES_REG, &temp_reg_val);
            if (ret)
            {
                return ret;
            }
            // Re-enable Boost Converter
            temp_reg_val |= MSM_BLOCK_ENABLES_BST_EN_BITMASK;
            ret = regmap_write(cp, MSM_BLOCK_ENABLES_REG, temp_reg_val);
            if (ret)
            {
                return ret;
            }
        }
    }

    // Set event flags
    d->event_flags = cs35l41_irq_to_event_id(irq_statuses);

    return CS35L41_STATUS_OK;
}

/**
 * Puts device into hibernate
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAI         Control port activity fails
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_hibernate(cs35l41_t *driver)
{
    uint32_t cs35l41_hibernate_patch[] =
    {
            IRQ1_IRQ1_MASK_1_REG, 0xFFFFFFFF,
            IRQ2_IRQ2_EINT_2_REG, (1 << 20),
            IRQ1_IRQ1_EINT_2_REG, (1 << 21),
            PWRMGT_WAKESRC_CTL, 0x0088,
            PWRMGT_WAKESRC_CTL, 0x0188,
            DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG, CS35L41_DSP_MBOX_CMD_HIBERNATE
    };
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    for (uint32_t i = 0; i < (sizeof(cs35l41_hibernate_patch)/sizeof(uint32_t)); i += 2)
    {
        uint32_t ret;
        ret = regmap_write(cp, cs35l41_hibernate_patch[i], cs35l41_hibernate_patch[i + 1]);
        if (ret)
        {
            return ret;
        }
    }

    return CS35L41_STATUS_OK;
}

/**
 * Poll PWRMGT_STS until WR_PEND_STS is cleared
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAI         Control port activity fails
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_wait_for_pwrmgt_sts(cs35l41_t *driver)
{
    uint32_t i;
    uint32_t wrpend_sts = 0x2;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    for (i = 0; (i < 10) && (wrpend_sts & PWRMGT_PWRMGT_STS_WR_PENDSTS_BITMASK); i++)
    {
        uint32_t ret;
        ret = regmap_read(cp, PWRMGT_PWRMGT_STS, &wrpend_sts);
        if (ret)
        {
            return ret;
        }
    }

    return CS35L41_STATUS_OK;
}

/**
 * Apply trims read from OTP to bitfields indicated in OTP Map
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAI         Control port activity fails
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_otp_unpack(cs35l41_t *driver)
{
    uint32_t ret;
    uint32_t temp_reg_val, i;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Unlock register file to apply OTP trims
    ret = regmap_write(cp, CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_1);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write(cp, CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_2);
    if (ret)
    {
        return ret;
    }

    // Initialize OTP unpacking state - otp_bit_count.  There are bits in OTP to skip to reach the trims
    uint16_t otp_bit_count = CS35L41_OTP_MAP_BIT_OFFSET;

    for (i = 0; i < (sizeof(otp_map)/sizeof(cs35l41_otp_packed_entry_t)); i++)
    {
        // Get trim entry
        cs35l41_otp_packed_entry_t temp_trim_entry = otp_map[i];

        // If the entry's 'reg' member is 0x0, it means skip that trim
        if (temp_trim_entry.reg != 0x00000000)
        {
            // Read the first register to be trimmed
            ret = regmap_read(cp, temp_trim_entry.reg, &temp_reg_val);
            if (ret)
            {
                return ret;
            }

            /*
             * Apply OTP trim bit-field to recently read trim register value.  OTP contents is saved in
             * cp_read_buffer + CS35L41_CP_REG_READ_LENGTH_BYTES
             */
            cs35l41_apply_trim_word(driver->otp_contents,
                                    otp_bit_count,
                                    &temp_reg_val,
                                    temp_trim_entry.shift,
                                    temp_trim_entry.size);
            // Write new trimmed register value back
            ret = regmap_write(cp, temp_trim_entry.reg, temp_reg_val);
            if (ret)
            {
                return ret;
            }
        }

        // Inrement the OTP unpacking state variable otp_bit_count
        otp_bit_count += temp_trim_entry.size;
    }

    // Lock register file
    ret = regmap_write(cp, CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_1);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write(cp, CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_2);
    if (ret)
    {
        return ret;
    }

    return CS35L41_STATUS_OK;
}

/**
 * Apply errata configuration
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAI         Control port activity fails
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_write_errata(cs35l41_t *driver)
{
    uint32_t ret;

    ret = regmap_write_array(REGMAP_GET_CP(driver),
                             (uint32_t *) cs35l41_revb2_errata_patch,
                             (sizeof(cs35l41_revb2_errata_patch)/sizeof(uint32_t)));
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }
    else
    {
        return CS35L41_STATUS_OK;
    }
}

/**
 * Apply configuration specifically required after loading HALO FW/COEFF files
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAI         Control port activity fails
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_write_post_boot_config(cs35l41_t *driver)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Write first post-boot configuration
    ret = regmap_write_array(cp,
                             (uint32_t *) cs35l41_post_boot_config,
                             (sizeof(cs35l41_post_boot_config)/sizeof(uint32_t)));
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Write configuration data
    // Unlock the register file
    ret = regmap_write(cp, CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_1);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write(cp, CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_2);
    if (ret)
    {
        return ret;
    }

    ret = regmap_write_array(cp, (uint32_t*) driver->config.syscfg_regs, driver->config.syscfg_regs_total);
    if (ret)
    {
        return ret;
    }

    // Lock the register file
    ret = regmap_write(cp, CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_1);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write(cp, CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_LOCK_2);
    if (ret)
    {
        return ret;
    }

    return CS35L41_STATUS_OK;
}

/**
 * Restore HW regsiters to pre-hibernation state
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAI         Control port activity fails
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_restore(cs35l41_t *driver)
{
    uint32_t ret;
    uint32_t mtl_revid, chipid_match;

    mtl_revid = driver->revid & 0x0F;
    chipid_match = (mtl_revid % 2) ? CS35L41R_DEVID : CS35L41_DEVID;
    if (driver->devid != chipid_match)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Send errata
    ret = cs35l41_write_errata(driver);
    if (ret)
    {
        return ret;
    }

    // OTP unpack
    ret = cs35l41_otp_unpack(driver);
    if (ret)
    {
        return ret;
    }

    // Write all post-boot configs
    ret = cs35l41_write_post_boot_config(driver);
    if (ret)
    {
        return ret;
    }

    return ret;
}

/**
 * Wakes device from hibernate
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAI         Control port activity fails
 * - CS35L41_STATUS_OK          otherwise
 *
 */
static uint32_t cs35l41_wake(cs35l41_t *driver)
{
    uint32_t timeout = 10, ret;
    uint32_t status = CS35L41_DSP_MBOX_STATUS_HIBERNATE;
    int8_t retries = 5;
    uint32_t mbox_cmd_drv_shift = 1 << 20;
    uint32_t mbox_cmd_fw_shift = 1 << 21;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    do {
        do {
            ret = regmap_write(cp,
                               DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG,
                               CS35L41_DSP_MBOX_CMD_OUT_OF_HIBERNATE);

            bsp_driver_if_g->set_timer(4, NULL, NULL);

            if (ret != CS35L41_STATUS_FAIL)
            {
                ret = regmap_read(cp, DSP_MBOX_DSP_MBOX_2_REG,  &status);
                if (ret)
                {
                  return ret;
                }
            }

        } while (status != CS35L41_DSP_MBOX_STATUS_PAUSED && --timeout > 0);

        if (timeout != 0)
        {
            break;
        }

        ret = cs35l41_wait_for_pwrmgt_sts(driver);
        if (ret)
        {
            return ret;
        }
        ret = regmap_write(cp, PWRMGT_WAKESRC_CTL, 0x0088);
        if (ret)
        {
            return ret;
        }
        ret = cs35l41_wait_for_pwrmgt_sts(driver);
        if (ret)
        {
            return ret;
        }
        ret = regmap_write(cp, PWRMGT_WAKESRC_CTL, 0x0188);
        if (ret)
        {
            return ret;
        }
        ret = cs35l41_wait_for_pwrmgt_sts(driver);
        if (ret)
        {
            return ret;
        }
        ret = regmap_write(cp, PWRMGT_PWRMGT_CTL, 0x3);
        if (ret)
        {
            return ret;
        }

        timeout = 10;

    } while (--retries > 0);

    ret = regmap_write(cp, IRQ2_IRQ2_EINT_2_REG, mbox_cmd_drv_shift);
    if (ret)
    {
        return ret;
    }
    ret = regmap_write(cp, IRQ1_IRQ1_EINT_2_REG, mbox_cmd_fw_shift);
    if (ret)
    {
        return ret;
    }

    retries = 5;

    do {
        ret = cs35l41_restore(driver);
        if (ret)
        {
            return ret;
        }
        bsp_driver_if_g->set_timer(4, NULL, NULL);
    } while (ret != 0 && --retries > 0);

    if (retries < 0)
    {
        //Failed to wake
        ret = CS35L41_STATUS_FAIL;
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
uint32_t cs35l41_initialize(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

    if (NULL != driver)
    {
        /*
         * The memset() call sets all members to 0, including the following semantics:
         * - 'state' is set to UNCONFIGURED
         */
        memset(driver, 0, sizeof(cs35l41_t));

        ret = CS35L41_STATUS_OK;
    }

    return ret;
}

/**
 * Configures driver state/handle
 *
 */
uint32_t cs35l41_configure(cs35l41_t *driver, cs35l41_config_t *config)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

    if ((NULL != driver) && \
        (NULL != config))
    {
        driver->config = *config;

        // Advance driver to CONFIGURED state
        driver->state = CS35L41_STATE_CONFIGURED;

        ret = bsp_driver_if_g->register_gpio_cb(driver->config.bsp_config.int_gpio_id,
                                                cs35l41_irq_callback,
                                                driver);

        if (ret == BSP_STATUS_OK)
        {
            ret = CS35L41_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Processes driver states and modes
 *
 */
uint32_t cs35l41_process(cs35l41_t *driver)
{
    // check for driver state
    if ((driver->state != CS35L41_STATE_UNCONFIGURED) && (driver->state != CS35L41_STATE_ERROR))
    {
        // check for driver mode
        if (driver->mode == CS35L41_MODE_HANDLING_EVENTS)
        {
            // run through event handler
            if (CS35L41_STATUS_OK == cs35l41_event_handler(driver))
            {
                driver->mode = CS35L41_MODE_HANDLING_CONTROLS;
            }
            else
            {
                driver->state = CS35L41_STATE_ERROR;
            }
        }

        if (driver->state == CS35L41_STATE_ERROR)
        {
            driver->event_flags |= CS35L41_EVENT_FLAG_STATE_ERROR;
        }

        if (driver->event_flags)
        {
            cs35l41_bsp_config_t *b = &(driver->config.bsp_config);
            if (b->notification_cb != NULL)
            {
                b->notification_cb(driver->event_flags, b->notification_cb_arg);
            }

            driver->event_flags = 0;
        }
    }

    return CS35L41_STATUS_OK;
}

/**
 * Reset the CS35L41 and prepare for HALO FW booting
 *
 */
uint32_t cs35l41_reset(cs35l41_t *driver)
{
    uint32_t ret;
    uint8_t i;
    uint32_t temp_reg_val;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Drive RESET low for at least T_RLPW (1ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.reset_gpio_id, BSP_GPIO_LOW);
    bsp_driver_if_g->set_timer(CS35L41_T_RLPW_MS, NULL, NULL);
    // Drive RESET high and wait for at least T_IRS (1ms)
    bsp_driver_if_g->set_gpio(driver->config.bsp_config.reset_gpio_id, BSP_GPIO_HIGH);
    bsp_driver_if_g->set_timer(CS35L41_T_IRS_MS, NULL, NULL);

    // Start polling OTP_BOOT_DONE bit every 10ms
    for (i = 0; i < CS35L41_POLL_OTP_BOOT_DONE_MAX; i++)
    {
        ret = regmap_read(cp, CS35L41_OTP_CTRL_OTP_CTRL8_REG, &temp_reg_val);
        if (ret)
        {
            return ret;
        }

        // If OTP_BOOT_DONE is set
        if (temp_reg_val & OTP_CTRL_OTP_CTRL8_OTP_BOOT_DONE_STS_BITMASK)
        {
            break;
        }

        bsp_driver_if_g->set_timer(CS35L41_POLL_OTP_BOOT_DONE_MS, NULL, NULL);
    }

    if (i >= CS35L41_POLL_OTP_BOOT_DONE_MAX)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Read DEVID
    ret = regmap_read(cp, CS35L41_SW_RESET_DEVID_REG, &(driver->devid));
    if (ret)
    {
        return ret;
    }
    // Read REVID
    ret = regmap_read(cp, CS35L41_SW_RESET_REVID_REG, &(driver->revid));
    if (ret)
    {
        return ret;
    }

    // Only Support CS35L41 B2
    if ((driver->devid != CS35L41_DEVID) || (driver->revid != CS35L41_REVID_B2))
    {
        return CS35L41_STATUS_FAIL;
    }

    // Send errata
    ret = cs35l41_write_errata(driver);
    if (ret)
    {
        return ret;
    }

    // Read OTPID
    ret = regmap_read(cp, CS35L41_SW_RESET_OTPID_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }
    temp_reg_val &= CS35L41_SW_RESET_OTPID_OTPID_BITMASK;

    // If invalid OTPID, indicate ERROR
    if ((temp_reg_val != 0x01) && (temp_reg_val != 0x08))
    {
        return CS35L41_STATUS_FAIL;
    }

    if(driver->config.bsp_config.cp_config.bus_type == REGMAP_BUS_TYPE_SPI)
    {
        bsp_driver_if_g->spi_throttle_speed(CS35L41_OTP_READ_MAX_SPI_CLOCK_HZ);
    }

    // Read entire OTP trim contents
    ret = regmap_read_block(cp, CS35L41_OTP_IF_OTP_MEM0_REG, driver->otp_contents, CS35L41_OTP_SIZE_BYTES);
    if (ret)
    {
        return ret;
    }

    if(driver->config.bsp_config.cp_config.bus_type == REGMAP_BUS_TYPE_SPI)
    {
        bsp_driver_if_g->spi_restore_speed();
    }

    // OTP Unpack
    ret = cs35l41_otp_unpack(driver);
    if (ret)
    {
        return ret;
    }

    // Stop clocks to HALO DSP Core
    ret = regmap_write(cp, XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG, 0);
    if (ret)
    {
        return ret;
    }

    if ((driver->state == CS35L41_STATE_CONFIGURED) ||
        (driver->state == CS35L41_STATE_DSP_STANDBY))
    {
        driver->state = CS35L41_STATE_STANDBY;
    }

    return CS35L41_STATUS_OK;
}

/**
 * Finish booting the CS35L41
 *
 */
uint32_t cs35l41_boot(cs35l41_t *driver, fw_img_info_t *fw_info)
{
    uint32_t ret = CS35L41_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    driver->fw_info = fw_info;

    // Initializing fw_info is okay, but do not proceed
    if (driver->fw_info == NULL)
    {
        return CS35L41_STATUS_OK;
    }

    // Write all post-boot configs
    ret = cs35l41_write_post_boot_config(driver);
    if (ret)
    {
        return ret;
    }

    // If calibration data is valid
    if ((!driver->is_cal_boot) && (driver->config.cal_data.is_valid))
    {
        // Write calibrated load impedance
        ret = regmap_write_fw_control(cp, driver->fw_info, CS35L41_SYM_CSPL_CAL_R, driver->config.cal_data.r);
        if (ret)
        {
            return CS35L41_STATUS_FAIL;
        }

        // Write CAL_STATUS
        ret = regmap_write_fw_control(cp,
                                      driver->fw_info,
                                      CS35L41_SYM_CSPL_CAL_STATUS,
                                      CS35L41_CAL_STATUS_CALIB_SUCCESS);
        if (ret)
        {
            return CS35L41_STATUS_FAIL;
        }

        // Write CAL_CHECKSUM
        ret = regmap_write_fw_control(cp,
                                      driver->fw_info,
                                      CS35L41_SYM_CSPL_CAL_CHECKSUM,
                                      (driver->config.cal_data.r + CS35L41_CAL_STATUS_CALIB_SUCCESS));
        if (ret)
        {
            return CS35L41_STATUS_FAIL;
        }
    }

    driver->state = CS35L41_STATE_DSP_STANDBY;

    return CS35L41_STATUS_OK;
}

/**
 * Change the power state
 *
 */
uint32_t cs35l41_power(cs35l41_t *driver, uint32_t power_state)
{
    uint32_t ret;
    uint32_t (*fp)(cs35l41_t *driver) = NULL;
    uint32_t next_state = CS35L41_STATE_UNCONFIGURED;

    switch (power_state)
    {
        case CS35L41_POWER_UP:
            if ((driver->state == CS35L41_STATE_STANDBY) ||
                (driver->state == CS35L41_STATE_DSP_STANDBY))
            {
                fp = &cs35l41_power_up;

                if (driver->state == CS35L41_STATE_STANDBY)
                {
                    next_state = CS35L41_STATE_POWER_UP;
                }
                else
                {
                    next_state = CS35L41_STATE_DSP_POWER_UP;
                }
            }
            break;

        case CS35L41_POWER_DOWN:
            if ((driver->state == CS35L41_STATE_POWER_UP) ||
                (driver->state == CS35L41_STATE_DSP_POWER_UP))
            {
                fp = &cs35l41_power_down;

                if (driver->state == CS35L41_STATE_STANDBY)
                {
                    next_state = CS35L41_STATE_STANDBY;
                }
                else
                {
                    next_state = CS35L41_STATE_DSP_STANDBY;
                }
            }
            break;

        case CS35L41_POWER_HIBERNATE:
            if (driver->state == CS35L41_STATE_DSP_STANDBY)
            {
                fp = &cs35l41_hibernate;
                next_state = CS35L41_STATE_HIBERNATE;
            }
            break;

        case CS35L41_POWER_WAKE:
            if (driver->state == CS35L41_STATE_HIBERNATE)
            {
                fp = &cs35l41_wake;
                next_state = CS35L41_STATE_DSP_STANDBY;
            }
            break;
    }

    if (fp == NULL)
    {
        return CS35L41_STATUS_FAIL;
    }

    ret = fp(driver);

    if (ret == CS35L41_STATUS_OK)
    {
        driver->state = next_state;
    }

    return ret;
}

/**
 * Calibrate the HALO DSP Protection Algorithm
 *
 */
uint32_t cs35l41_calibrate(cs35l41_t *driver, uint32_t ambient_temp_deg_c)
{
    uint32_t temp_reg_val;
    uint32_t ret = CS35L41_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Set the Ambient Temp (deg C)
    ret = regmap_write_fw_control(cp, driver->fw_info, CS35L41_SYM_CSPL_CAL_AMBIENT, ambient_temp_deg_c);
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Wait for at least 2 seconds while DSP FW performs calibration
    bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2S, NULL, NULL);

    // Read the Calibration Load Impedance "R"
    ret = regmap_read_fw_control(cp, driver->fw_info, CS35L41_SYM_CSPL_CAL_R, &temp_reg_val);
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }
    driver->config.cal_data.r = temp_reg_val;

    // Read the Calibration Status
    ret = regmap_read_fw_control(cp, driver->fw_info, CS35L41_SYM_CSPL_CAL_STATUS, &temp_reg_val);
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    if (temp_reg_val != CS35L41_CAL_STATUS_CALIB_SUCCESS)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Read the Calibration Checksum
    ret = regmap_read_fw_control(cp, driver->fw_info, CS35L41_SYM_CSPL_CAL_CHECKSUM, &temp_reg_val);
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Verify the Calibration Checksum
    if (temp_reg_val == (driver->config.cal_data.r + CS35L41_CAL_STATUS_CALIB_SUCCESS))
    {
        driver->config.cal_data.is_valid = true;
    }
    else
    {
        return CS35L41_STATUS_FAIL;
    }

    return CS35L41_STATUS_OK;
}

/**
 * Send a set of HW configuration registers
 *
 */
uint32_t cs35l41_send_syscfg(cs35l41_t *driver, const uint32_t *cfg, uint16_t cfg_length)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write_array(cp, (uint32_t*) cfg, cfg_length);
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    return CS35L41_STATUS_OK;
}

/**
 * Start the process for updating the tuning for the HALO FW
 *
 */
uint32_t cs35l41_start_tuning_switch(cs35l41_t *driver)
{
    uint32_t ret;
    uint8_t i;
    uint32_t temp_reg_val;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    /*
     * The Host (i.e. the AP or the Codec driving the amp) sends a PAUSE request to the Prince FW and Pauses the
     * current playback.
     */
    ret = cs35l41_send_acked_mbox_cmd(driver, CS35L41_DSP_MBOX_CMD_PAUSE);
    if (ret)
    {
        return ret;
    }

    // The Host ensures both PLL_FORCE_EN and GLOBAL_EN are set to 0
    // The Host checks the Power Down Done flag on Prince (MSM_PDN_DONE) to ensure that the PLL has stopped.
    // Read GLOBAL_EN register in order to clear GLOBAL_EN
    ret = regmap_read(cp, MSM_GLOBAL_ENABLES_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Clear GLOBAL_EN
    temp_reg_val &= ~(MSM_GLOBAL_ENABLES_GLOBAL_EN_BITMASK);
    ret = regmap_write(cp, MSM_GLOBAL_ENABLES_REG, temp_reg_val);
    if (ret)
    {
        return ret;
    }

    // Read IRQ1 flag register to poll MSM_PDN_DONE bit
    i = 100;
    do
    {
        ret = regmap_read(cp, IRQ1_IRQ1_EINT_1_REG, &temp_reg_val);
        if (ret)
        {
            return ret;
        }

        if (temp_reg_val & IRQ1_IRQ1_EINT_1_MSM_PDN_DONE_EINT1_BITMASK)
        {
            break;
        }

        bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_1MS, NULL, NULL);

        i--;
    } while (i > 0);

    if (i == 0)
    {
        return CS35L41_STATUS_FAIL;
    }

    // Clear MSM_PDN_DONE IRQ flag
    ret = regmap_write(cp, IRQ1_IRQ1_EINT_1_REG, IRQ1_IRQ1_EINT_1_MSM_PDN_DONE_EINT1_BITMASK);
    if (ret)
    {
        return ret;
    }

    bsp_driver_if_g->set_timer(10, NULL, NULL);

    /*
     * The Host sends a CSPL_STOP_PRE_REINIT.   This puts the FW into a state ready to accept a new
     * tuning/configuration but leaves the DSP running.
     * Poll for RDY_FOR_REINIT from MBOX2
     */
    ret = cs35l41_send_acked_mbox_cmd(driver, CS35L41_DSP_MBOX_CMD_STOP_PRE_REINIT);
    if (ret)
    {
        return ret;
    }

    return CS35L41_STATUS_OK;
}

/**
 * Finish the process for updating the tuning for the HALO FW
 *
 */
uint32_t cs35l41_finish_tuning_switch(cs35l41_t *driver)
{
    uint32_t ret;
    uint32_t temp_reg_val;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    /*
     * The Host sends a REINIT request.   This causes the FW to read the new configuration and initialize the new CSPL
     * audio chain. This will compare the GLOBAL_FS with the sample rate from the tuning.
     * Poll for RDY_FOR_REINIT from MBOX2
     */
    ret = cs35l41_send_acked_mbox_cmd(driver, CS35L41_DSP_MBOX_CMD_REINIT);
    if (ret)
    {
        return ret;
    }

    // The Host sets the GLOBAL_EN to 1. It is not expected that the PLL_FORCE_EN should be used
    // Read GLOBAL_EN register
    ret = regmap_read(cp, MSM_GLOBAL_ENABLES_REG, &temp_reg_val);
    if (ret)
    {
        return ret;
    }
    temp_reg_val |= MSM_GLOBAL_ENABLES_GLOBAL_EN_BITMASK;
    //Set GLOBAL_EN
    ret = regmap_write(cp, MSM_GLOBAL_ENABLES_REG, temp_reg_val);
    if (ret)
    {
        return ret;
    }

    //Wait 1ms
    bsp_driver_if_g->set_timer(CS35L41_T_AMP_PUP_MS, NULL, NULL);

    // The Host sends a RESUME command and the FW starts to process and output the new audio.
    ret = cs35l41_send_acked_mbox_cmd(driver, CS35L41_DSP_MBOX_CMD_RESUME);
    if (ret)
    {
        return ret;
    }

    return CS35L41_STATUS_OK;
}

/**
 * Get DSP Status
 *
 */
uint32_t cs35l41_get_dsp_status(cs35l41_t *driver, cs35l41_dsp_status_t *status)
{
    uint8_t i;
    uint32_t temp_reg_val;
    uint32_t ret = CS35L41_STATUS_OK;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    // Read the DSP Status field addresses
    for (i = 0; i < CS35L41_DSP_STATUS_WORDS_TOTAL; i++)
    {
        ret = regmap_read_fw_control(cp, driver->fw_info, cs35l41_dsp_status_controls[i], &(status->data.words[i]));
        if (ret)
        {
            return CS35L41_STATUS_FAIL;
        }
    }

    // Wait at least 10ms
    bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_10MS, NULL, NULL);

    for (i = 0; i < CS35L41_DSP_STATUS_WORDS_TOTAL; i++)
    {
        ret = regmap_read_fw_control(cp, driver->fw_info, cs35l41_dsp_status_controls[i], &temp_reg_val);
        if (ret)
        {
            return CS35L41_STATUS_FAIL;
        }

        // If the current field is HALO_HEARTBEAT, and there is a change in subsequent values
        if ((i == 1) && (temp_reg_val != status->data.words[i]))
        {
            status->is_hb_inc = true;
        }

        // If the current field is CSPL_TEMPERATURE, and there is a change in subsequent values
        if ((i == 8) && (temp_reg_val != status->data.words[i]))
        {
            status->is_temp_changed = true;
        }

        status->data.words[i] = temp_reg_val;
    }

    // Assess if Calibration is applied
    if ((status->data.cal_set_status == 2) &&
        (status->data.cal_r_selected == status->data.cal_r) &&
        (status->data.cal_r == driver->config.cal_data.r) &&
        (status->data.cspl_state == 0) &&
        (status->data.halo_state == 2))
    {
        status->is_calibration_applied = true;
    }

    return CS35L41_STATUS_OK;
}

/*
 * Reads the contents of a single register/memory address
 *
 */
uint32_t cs35l41_read_reg(cs35l41_t *driver, uint32_t addr, uint32_t *val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_read(cp, addr, val);
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    return CS35L41_STATUS_OK;
}

/*
 * Writes the contents of a single register/memory address
 *
 */
uint32_t cs35l41_write_reg(cs35l41_t *driver, uint32_t addr, uint32_t val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write(cp, addr, val);
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    return CS35L41_STATUS_OK;
}

/*
 * Reads, updates and writes (if there's a change) the contents of a single register/memory address
 *
 */
uint32_t cs35l41_update_reg(cs35l41_t *driver, uint32_t addr, uint32_t mask, uint32_t val)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_update_reg(cp, addr, mask, val);
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    return CS35L41_STATUS_OK;
}

/*
 * Write block of data to the CS35L41 register file
 *
 * This call is used to load the HALO FW/COEFF files to HALO RAM.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Starting address of loading destination
 * @param [in] data             Pointer to array of bytes to be written
 * @param [in] size             Size of array of bytes to be written
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Any pointers are NULL
 *      - size is not multiple of 4
 *      - Control port activity fails
 * - otherwise, returns CS35L41_STATUS_OK
 *
 */
uint32_t cs35l41_write_block(cs35l41_t *driver, uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t ret;
    regmap_cp_config_t *cp = REGMAP_GET_CP(driver);

    ret = regmap_write_block(cp,
                             addr,
                             data,
                             size);
    if (ret)
    {
        return CS35L41_STATUS_FAIL;
    }

    return CS35L41_STATUS_OK;
}


/*!
 * \mainpage Introduction
 *
 * This document outlines the driver source code included in the MCU Driver Software Package for the CS35L41 Boosted
 * Amplifier.  This guide is primarily intended for those involved in end-system implementation, integration, and
 * testing, who will use the CS35L41 MCU Driver Software Package to integrate the CS35L41 driver source code into the
 * end-system's host MCU software.  After reviewing this guide, the reader will be able to begin software integration
 * of the CS35L41 MCU driver and then have the ability to initialize, reset, boot, configure, and service events from
 * the CS35L41.  This guide should be used along with the CS35L41 Datasheet.
 *
 *  In order to obtain any additional materials, and for any questions regarding this guide, the MCU Driver
 *  Software Package, or CS40L25 system integration, please contact your Cirrus Logic Representative.
 */

