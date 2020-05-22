/**
 * @file cs35l41.c
 *
 * @brief The CS35L41 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
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
#include "cs35l41.h"
#include "bsp_driver_if.h"
#include "string.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/
//#define I2S_CONFIG_SHORTCUT
#ifdef I2S_CONFIG_SHORTCUT
//#define USE_DIAG_SIGGEN
#endif
//#define DEBUG_POWER_DOWN_STOP_DSP

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
 * Beta value used to calculate value for CCM_FS_MON_0_REG
 *
 * @see Datasheet Section 4.13.9
 *
 */
#define CS35L41_FS_MON0_BETA                    (6000000)

#ifdef INCLUDE_FW
/**
 * Value of CS35L41_CAL_STATUS that indicates Calibration success
 *
 * @see CS35L41_CAL_STATUS
 *
 */
#define CS35L41_CAL_STATUS_CALIB_SUCCESS        (0x1)

/**
 * Total number of HALO FW controls to cache before CS35L41 Power Up
 *
 * Currently, there are no HALO FW controls that are cached in the driver.
 *
 * @see cs35l41_power_up_sm
 *
 */
#define CS35L41_SYNC_CTRLS_TOTAL (0)
#endif // INCLUDE_FW

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/**
 * CS35L41 RevB0 Register Patch Errata
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
static const uint32_t cs35l41_revb0_errata_patch[] =
{
    0x0000001A, //
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_1,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_2,
    0x00004100, 0x00000000,
    0x00004310, 0x00000000,
    0x00004400, 0x00000000,
    0x0000381C, 0x00000051,
    0x02BC20E0, 0x00000000,
    0x02BC2020, 0x00000000,
    0x00004854, 0x01010000,
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
static const cs35l41_otp_packed_entry_t otp_map_1[] =
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
 * List of possible OTP Maps for CS35L41 RevB2
 *
 * For CS35L41 RevB2, the following values of OTPID are possible:
 * - 0x1 - on used at first release of RevB2, this driver should not experience any in the field
 * - 0x8 - currently only common ID for this driver
 *
 * @see cs35l41_otp_map_t
 *
 */
static const cs35l41_otp_map_t cs35l41_otp_maps[] = {
    {
        .id = 0x01,
        .map = otp_map_1,
        .num_elements = sizeof(otp_map_1)/sizeof(cs35l41_otp_packed_entry_t),
        .bit_offset = 80,
    },
    {
        .id = 0x08,
        .map = otp_map_1,
        .num_elements = sizeof(otp_map_1)/sizeof(cs35l41_otp_packed_entry_t),
        .bit_offset = 80,
    },
};

#ifdef INCLUDE_FW
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
#endif // INCLUDE_FW

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
 * @see cs35l41_power_up_sm
 *
 */
static const uint32_t cs35l41_pup_patch[] =
{
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_1,
    CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG, CS35L41_TEST_KEY_CTRL_UNLOCK_2,
    0x00002084, 0x002F1AA0,
#ifdef I2S_CONFIG_SHORTCUT
    CCM_REFCLK_INPUT_REG, 0x00000430,
    CCM_GLOBAL_SAMPLE_RATE_REG, 0x00000003,
    DATAIF_ASP_CONTROL1_REG, 0x00000021,
    DATAIF_ASP_CONTROL2_REG, 0x20200200,
    DATAIF_ASP_ENABLES1_REG, 0x00010003,
    CCM_FS_MON_0_REG, 0x0002C01C,
    MSM_BLOCK_ENABLES_REG, 0x00003721,
#ifdef USE_DIAG_SIGGEN
    0x00003800, 0x00000000,     // BST_CTL = 0x0; VBST=VP
    0x00003804, 0x00000000,     // BST_CTL_SEL = 0b00; control port BST_CTL
    0x00006000, 0x00000000,     // clear AMP_HPF_PCM_EN to disable HPF
    0x00004C00, 0x00000004,     // DACPCM1_SRC = 4 to route SIGGEN to DAC
    0x00007400, 0x005801C0,     // Select -6dBFs sine and enable SIGGEN
#endif
#endif
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
 * @see cs35l41_power_down_sm
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

#ifdef INCLUDE_FW
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
 * @see cs35l41_power_up_sm
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
 * @see cs35l41_power_up_sm
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
#endif // INCLUDE_FW

/**
 * Register addresses to modify during Configure SM
 *
 * Sent after the CS35L41 has been reset and, if firmware is available, has been booted.
 *
 * List is in the form:
 * - word0 - Address of first configuration register
 * - word1 - Address of second configuration register
 * - ...
 *
 * @see cs35l41_configure_sm
 * @see cs35l41_t member config_regs
 * @see cs35l41_config_registers_t
 *
 * @warning  The list of registers MUST correspond to the union of structs in  in cs35l41_config_registers_t.
 *
 */
static const uint32_t cs35l41_config_register_addresses[CS35L41_CONFIG_REGISTERS_TOTAL] =
{
    CS35L41_INTP_AMP_CTRL_REG,
    CS35L41_DRE_AMP_GAIN_REG,
    CS35L41_MIXER_ASPTX1_INPUT_REG,
    CS35L41_MIXER_ASPTX2_INPUT_REG,
    CS35L41_MIXER_ASPTX3_INPUT_REG,
    CS35L41_MIXER_ASPTX4_INPUT_REG,
    CS35L41_MIXER_DSP1RX1_INPUT_REG,
    CS35L41_MIXER_DSP1RX2_INPUT_REG,
    CS35L41_MIXER_DACPCM1_INPUT_REG,
    CCM_GLOBAL_SAMPLE_RATE_REG,
    NOISE_GATE_MIXER_NGATE_CH1_CFG_REG,
    NOISE_GATE_MIXER_NGATE_CH2_CFG_REG,
    CCM_REFCLK_INPUT_REG,
    MSM_BLOCK_ENABLES_REG,
    MSM_BLOCK_ENABLES2_REG,
    DATAIF_ASP_ENABLES1_REG,
    DATAIF_ASP_CONTROL2_REG,
    DATAIF_ASP_FRAME_CONTROL5_REG,
    DATAIF_ASP_FRAME_CONTROL1_REG,
    DATAIF_ASP_CONTROL3_REG,
    DATAIF_ASP_DATA_CONTROL5_REG,
    DATAIF_ASP_DATA_CONTROL1_REG,
    CCM_FS_MON_0_REG,
    DATAIF_ASP_CONTROL1_REG,
    BOOST_LBST_SLOPE_REG,
    BOOST_BST_LOOP_COEFF_REG,
    BOOST_BST_IPK_CTL_REG,
    BOOST_VBST_CTL_1_REG,
    BOOST_VBST_CTL_2_REG,
    TEMPMON_WARN_LIMIT_THRESHOLD_REG,
    PWRMGMT_CLASSH_CONFIG_REG,
    PWRMGMT_WKFET_AMP_CONFIG_REG,
};

#ifdef INCLUDE_FW
/**
 * Register/DSP Memory addresses to read during Get DSP Status SM
 *
 * List is in the form:
 * - word0 - Address of first status register
 * - word1 - Address of second status register
 * - ...
 *
 * @see cs35l41_get_dsp_status_sm
 * @see cs35l41_dsp_status_t
 *
 * @warning  The list of registers MUST correspond to the union of structs in  in cs35l41_dsp_status_t.
 *
 */
static const uint32_t cs35l41_dsp_status_addresses[CS35L41_DSP_STATUS_WORDS_TOTAL] =
{
        CS35L41_HALO_STATE,
        CS35L41_HALO_HEARTBEAT,
        CS35L41_CSPL_STATE,
        CS35L41_CAL_SET_STATUS,
        CS35L41_CAL_R_SELECTED,
        CS35L41_CAL_R,
        CS35L41_CAL_STATUS,
        CS35L41_CAL_CHECKSUM,
        CS35L41_CSPL_TEMPERATURE
};
#endif // INCLUDE_FW

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
/**
 * Cache for contents of IRQ1_EINT_*_REG interrupt flag registers.
 *
 * Currently, the following registers are cached:
 * - IRQ1_IRQ1_EINT_1_REG
 * - IRQ1_IRQ1_EINT_2_REG
 * - IRQ1_IRQ1_EINT_3_REG
 * - IRQ1_IRQ1_EINT_4_REG
 *
 * This cache is required for cs35l41_event_sm.  It is used along with irq_masks[] to determine what unmasked
 * interrupts have occurred.  This cache is required for cs35l41_event_sm.  The cache currently is not allocated as
 * part of cs35l41_t, but it should either be allocated there or have another means to cache the contents.
 *
 * @see IRQ1_IRQ1_EINT_1_REG
 * @see IRQ1_IRQ1_EINT_2_REG
 * @see IRQ1_IRQ1_EINT_3_REG
 * @see IRQ1_IRQ1_EINT_4_REG
 * @see cs35l41_event_sm
 *
 */
static uint32_t irq_statuses[4];

/**
 * Cache for contents of IRQ1_MASK_*_REG interrupt mask registers.
 *
 * Currently, the following registers are cached:
 * - IRQ1_IRQ1_MASK_1_REG
 * - IRQ1_IRQ1_MASK_2_REG
 * - IRQ1_IRQ1_MASK_3_REG
 * - IRQ1_IRQ1_MASK_4_REG
 *
 * This cache is required for cs35l41_event_sm.  It is used along with irq_statuses[] to determine what unmasked
 * interrupts have occurred. The cache currently is not allocated as part of cs35l41_t, but it should either be
 * allocated there or have another means to cache the contents.
 *
 * @see IRQ1_IRQ1_MASK_1_REG
 * @see IRQ1_IRQ1_MASK_2_REG
 * @see IRQ1_IRQ1_MASK_3_REG
 * @see IRQ1_IRQ1_MASK_4_REG
 * @see cs35l41_event_sm
 *
 */
static uint32_t irq_masks[4];

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Notify the driver when the BSP Timer expires.
 *
 * Implementation of cs35l41_private_functions_t.timer_callback
 *
 */
static void cs35l41_timer_callback(uint32_t status, void *cb_arg)
{
    cs35l41_t *d;

    d = (cs35l41_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        CS35L41_SET_FLAG(d->control_sm.flags, CS35L41_FLAGS_TIMEOUT);
    }

    return;
}

/**
 * Notify the driver when the BSP Control Port (cp) read transaction completes.
 *
 * Implementation of cs35l41_private_functions_t.cp_read_callback
 *
 */
static void cs35l41_cp_read_callback(uint32_t status, void *cb_arg)
{
    cs35l41_t *d;

    d = (cs35l41_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Check the driver mode to know which state machine called the BSP API and set respective flag
        if (d->mode == CS35L41_MODE_HANDLING_CONTROLS)
        {
            CS35L41_SET_FLAG(d->control_sm.flags, CS35L41_FLAGS_CP_RW_DONE);
        }
        else
        {
            CS35L41_SET_FLAG(d->event_sm.flags, CS35L41_FLAGS_CP_RW_DONE);
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
 * Implementation of cs35l41_private_functions_t.cp_write_callback
 *
 */
static void cs35l41_cp_write_callback(uint32_t status, void *cb_arg)
{
    cs35l41_t *d;

    d = (cs35l41_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Check the driver mode to know which state machine called the BSP API and set respective flag
        if (d->mode == CS35L41_MODE_HANDLING_CONTROLS)
        {
            CS35L41_SET_FLAG(d->control_sm.flags, CS35L41_FLAGS_CP_RW_DONE);
        }
        else
        {
            CS35L41_SET_FLAG(d->event_sm.flags, CS35L41_FLAGS_CP_RW_DONE);
        }
    }

    return;
}

/**
 * Notify the driver when the CS35L41 INTb GPIO drops low.
 *
 * Implementation of cs35l41_private_functions_t.irq_callback
 *
 */
static void cs35l41_irq_callback(uint32_t status, void *cb_arg)
{
    cs35l41_t *d;

    d = (cs35l41_t *) cb_arg;

    if (status == BSP_STATUS_OK)
    {
        // Only if the driver is in CS35L41_MODE_HANDLING_CONTROLS, then reset Event Handler state machine
        if (d->mode == CS35L41_MODE_HANDLING_CONTROLS)
        {
            // Switch driver mode to CS35L41_MODE_HANDLING_EVENTS
            d->mode = CS35L41_MODE_HANDLING_EVENTS;
            // Reset Event Handler state machine
            d->event_sm.state = CS35L41_EVENT_SM_STATE_INIT;
            d->event_sm.flags = 0;
            d->event_sm.count = 0;
            /*
             * This is left to support the potential of having multiple types of Event Handler state machines.
             */
            d->event_sm.fp = cs35l41_private_functions_g->event_sm;
        }
    }

    return;
}

/**
 * Reads the contents of a single register/memory address
 *
 * Implementation of cs35l41_private_functions_t.read_reg
 *
 */
static uint32_t cs35l41_read_reg(cs35l41_t *driver, uint32_t addr, uint32_t *val, bool is_blocking)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

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
    if (driver->bus_type == CS35L41_BUS_TYPE_I2C)
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

                ret = CS35L41_STATUS_OK;
            }
        }
        else
        {
            bsp_status = bsp_driver_if_g->i2c_read_repeated_start(driver->bsp_dev_id,
                                                                  driver->cp_write_buffer,
                                                                  4,
                                                                  driver->cp_read_buffer,
                                                                  4,
                                                                  cs35l41_private_functions_g->cp_read_callback,
                                                                  driver);
            if (BSP_STATUS_OK == bsp_status)
            {
                ret = CS35L41_STATUS_OK;
            }
        }
    }

    return ret;
}

/**
 * Writes the contents of a single register/memory address
 *
 * Implementation of cs35l41_private_functions_t.write_reg
 *
 */
static uint32_t cs35l41_write_reg(cs35l41_t *driver, uint32_t addr, uint32_t val, bool is_blocking)
{
    uint32_t ret = CS35L41_STATUS_FAIL;
    uint32_t bsp_status = BSP_STATUS_FAIL;

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
    if (driver->bus_type == CS35L41_BUS_TYPE_I2C)
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
                                                    cs35l41_private_functions_g->cp_write_callback,
                                                    driver);
        }
    }

    if (BSP_STATUS_OK == bsp_status)
    {
        ret = CS35L41_STATUS_OK;
    }

    return ret;
}

/**
 * Reset State Machine
 *
 * Implementation of cs35l41_private_functions_t.reset_sm
 *
 */
static uint32_t cs35l41_reset_sm(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    cs35l41_sm_t *r = &(driver->control_sm);
    uint32_t bsp_status = BSP_STATUS_OK;

    switch(r->state)
    {
        case CS35L41_RESET_SM_STATE_INIT:
            // Drive RESET low for at least T_RLPW (1ms)
            bsp_status = bsp_driver_if_g->set_gpio(driver->bsp_reset_gpio_id, BSP_GPIO_LOW);
            if (bsp_status == BSP_STATUS_OK)
            {
                CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_TIMEOUT);
                bsp_status = bsp_driver_if_g->set_timer(CS35L41_T_RLPW_MS, cs35l41_timer_callback, driver);
                if (bsp_status == BSP_STATUS_OK)
                {
                    r->state = CS35L41_RESET_SM_STATE_WAIT_T_RLPW;
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_WAIT_T_RLPW:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_TIMEOUT))
            {
                r->state = CS35L41_RESET_SM_STATE_WAIT_T_IRS;
                // Drive RESET high and wait for at least T_IRS (1ms)
                bsp_status = bsp_driver_if_g->set_gpio(driver->bsp_reset_gpio_id, BSP_GPIO_HIGH);
                if (bsp_status == BSP_STATUS_OK)
                {
                    CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_TIMEOUT);
                    bsp_status = bsp_driver_if_g->set_timer(CS35L41_T_IRS_MS, cs35l41_timer_callback, driver);
                    if (bsp_status == BSP_STATUS_OK)
                    {
                        r->state = CS35L41_RESET_SM_STATE_WAIT_T_IRS;
                    }
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_WAIT_T_IRS:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_TIMEOUT))
            {
                CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                r->count = 0;
                // Start polling OTP_BOOT_DONE bit every 10ms
                ret = cs35l41_private_functions_g->read_reg(driver,
                                                            CS35L41_OTP_CTRL_OTP_CTRL8_REG,
                                                            &(driver->register_buffer),
                                                            false);

                if (ret == CS35L41_STATUS_OK)
                {
                    CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_TIMEOUT);
                    bsp_status = bsp_driver_if_g->set_timer(CS35L41_POLL_OTP_BOOT_DONE_MS,
                                                            cs35l41_timer_callback,
                                                            driver);
                    if (bsp_status == BSP_STATUS_OK)
                    {
                        r->state = CS35L41_RESET_SM_STATE_WAIT_OTP_BOOT_DONE;
                    }
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_WAIT_OTP_BOOT_DONE:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_TIMEOUT))
            {
                if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
                {
                    r->count++;
                    // If OTP_BOOT_DONE is set
                    if (driver->register_buffer & OTP_CTRL_OTP_CTRL8_OTP_BOOT_DONE_STS_BITMASK)
                    {
                        CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                        // Read DEVID
                        ret = cs35l41_private_functions_g->read_reg(driver,
                                                                    CS35L41_SW_RESET_DEVID_REG,
                                                                    &(driver->register_buffer),
                                                                    false);

                        if (ret == CS35L41_STATUS_OK)
                        {
                            r->count = 0;
                            r->state = CS35L41_RESET_SM_STATE_READ_ID;
                        }
                    }
                    // If polling period expired, indicate ERROR
                    else if (r->count >= CS35L41_POLL_OTP_BOOT_DONE_MAX)
                    {
                        ret = CS35L41_STATUS_FAIL;
                        r->state = CS35L41_RESET_SM_STATE_ERROR;
                    }
                    // If time left to poll, read OTP_BOOT_DONE again
                    else
                    {
                        CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                        ret = cs35l41_private_functions_g->read_reg(driver,
                                                                    CS35L41_OTP_CTRL_OTP_CTRL8_REG,
                                                                    &(driver->register_buffer),
                                                                    false);

                        if (ret == CS35L41_STATUS_OK)
                        {
                            CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_TIMEOUT);
                            bsp_status = bsp_driver_if_g->set_timer(CS35L41_POLL_OTP_BOOT_DONE_MS,
                                                                    cs35l41_timer_callback,
                                                                    driver);
                        }
                    }
                }
                // If after 10ms I2C Read Callback hasn't been called from BSP, assume an error
                else
                {
                    ret = CS35L41_STATUS_FAIL;
                    r->state = CS35L41_RESET_SM_STATE_ERROR;
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_READ_ID:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                r->count++;
                if (r->count == 1)
                {
                    driver->devid = driver->register_buffer;

                    CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Read REVID
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                CS35L41_SW_RESET_REVID_REG,
                                                                &(driver->register_buffer),
                                                                false);
                }
                else
                {
                    driver->revid = driver->register_buffer;
                    // Get errata based on DEVID/REVID
                    ret = cs35l41_private_functions_g->get_errata(driver->devid, driver->revid, &(driver->errata));

                    if (ret == CS35L41_STATUS_OK)
                    {
                        r->state = CS35L41_RESET_SM_STATE_WRITE_IRQ_ERRATA;
                        CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                        r->count = 0;
                        const uint32_t *errata_write = driver->errata;
                        // Skip first word which is errata length
                        errata_write++;
                        // Start sending errata
                        ret = cs35l41_private_functions_g->write_reg(driver,
                                                                     *(errata_write),
                                                                     *(errata_write + 1),
                                                                     false);
                    }
                }
            }
            else if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_ERROR))
            {
                ret = CS35L41_STATUS_FAIL;
                r->state = CS35L41_RESET_SM_STATE_ERROR;
            }
            break;

        case CS35L41_RESET_SM_STATE_WRITE_IRQ_ERRATA:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                r->count++;
                const uint32_t *errata_write = driver->errata;

                if ((r->count * 2) < *errata_write)
                {
                    CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);

                    // Calculate position in errata erray - Skip first word which is errata length
                    errata_write++;
                    errata_write += (r->count * 2);
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 *(errata_write),
                                                                 *(errata_write + 1),
                                                                 false);
                }
                else
                {
                    r->state = CS35L41_RESET_SM_STATE_READ_OTPID;

                    CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Read OTPID
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                CS35L41_SW_RESET_OTPID_REG,
                                                                &(driver->register_buffer),
                                                                false);
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_READ_OTPID:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_ERROR))
            {
                ret = CS35L41_STATUS_FAIL;
                r->state = CS35L41_RESET_SM_STATE_ERROR;
            }
            else if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                uint32_t otpid = driver->register_buffer & CS35L41_SW_RESET_OTPID_OTPID_BITMASK;
                // Find correct OTP Map based on OTPID
                for (uint8_t i = 0; i < (sizeof(cs35l41_otp_maps)/sizeof(cs35l41_otp_map_t)); i++)
                {
                    if (cs35l41_otp_maps[i].id == otpid)
                    {
                        driver->otp_map = &(cs35l41_otp_maps[i]);
                    }
                }

                // If no OTP Map found, indicate ERROR
                if (driver->otp_map == NULL)
                {
                    ret = CS35L41_STATUS_FAIL;
                    r->state = CS35L41_RESET_SM_STATE_ERROR;
                }
                else
                {
                    CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Read entire OTP trim contents
                    ret = cs35l41_private_functions_g->cp_bulk_read(driver,
                                                                    CS35L41_OTP_IF_OTP_MEM0_REG,
                                                                    CS35L41_OTP_SIZE_WORDS);
                    r->state = CS35L41_RESET_SM_STATE_READ_OTP;
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_READ_OTP:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_ERROR))
            {
                ret = CS35L41_STATUS_FAIL;
                r->state = CS35L41_RESET_SM_STATE_ERROR;
            }
            else if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                r->count = 0;
                // Unlock register file to apply OTP trims
                ret = cs35l41_private_functions_g->write_reg(driver,
                                                             CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                             CS35L41_TEST_KEY_CTRL_UNLOCK_1,
                                                             false);
                r->state = CS35L41_RESET_SM_STATE_WRITE_OTP_UNLOCK;
            }
            break;

        case CS35L41_RESET_SM_STATE_WRITE_OTP_UNLOCK:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_ERROR))
            {
                ret = CS35L41_STATUS_FAIL;
                r->state = CS35L41_RESET_SM_STATE_ERROR;
            }
            else if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                r->count++;
                if (r->count == 1)
                {
                    // Unlock register file to apply OTP trims
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS35L41_TEST_KEY_CTRL_UNLOCK_2,
                                                                 false);
                }
                else
                {
                    r->count = 0;
                    // Initialize OTP unpacking state - otp_bit_count.  There are bits in OTP to skip to reach the trims
                    driver->otp_bit_count = driver->otp_map->bit_offset;
                    // Get first trim entry
                    cs35l41_otp_packed_entry_t temp_trim_entry = driver->otp_map->map[0];
                    // Read the first register to be trimmed
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                temp_trim_entry.reg,
                                                                &(driver->register_buffer),
                                                                false);
                    r->state = CS35L41_RESET_SM_STATE_READ_TRIM_WORD;
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_READ_TRIM_WORD:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_ERROR))
            {
                ret = CS35L41_STATUS_FAIL;
                r->state = CS35L41_RESET_SM_STATE_ERROR;
            }
            else if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                // Get current trim entry
                cs35l41_otp_packed_entry_t temp_trim_entry = driver->otp_map->map[r->count];
                r->count++;
                CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);

                // If the entry's 'reg' member is 0x0, it means skip that trim
                if (temp_trim_entry.reg != 0x00000000)
                {
                    /*
                     * Apply OTP trim bit-field to recently read trim register value.  OTP contents is saved in
                     * cp_read_buffer + CS35L41_CP_REG_READ_LENGTH_BYTES
                     */
                    ret = cs35l41_private_functions_g->apply_trim_word((driver->cp_read_buffer + \
                                                                        CS35L41_CP_REG_READ_LENGTH_BYTES),
                                                                       driver->otp_bit_count,
                                                                       &(driver->register_buffer),
                                                                       temp_trim_entry.shift,
                                                                       temp_trim_entry.size);
                    if (ret == CS35L41_STATUS_OK)
                    {
                        // Write new trimmed register value back
                        ret = cs35l41_private_functions_g->write_reg(driver,
                                                                     temp_trim_entry.reg,
                                                                     driver->register_buffer,
                                                                     false);
                        // Inrement the OTP unpacking state variable otp_bit_count
                        driver->otp_bit_count += temp_trim_entry.size;
                        r->state = CS35L41_RESET_SM_STATE_WRITE_TRIM_WORD;
                    }
                }
                else if (r->count < driver->otp_map->num_elements)
                {
                    // If trim entry skipped, get next trim entry and read the register
                    driver->otp_bit_count += temp_trim_entry.size;
                    temp_trim_entry = driver->otp_map->map[r->count];
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                temp_trim_entry.reg,
                                                                &(driver->register_buffer),
                                                                false);
                }
                // If done unpacking OTP
                else
                {
                    r->count = 0;
                    // Lock register file
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS35L41_TEST_KEY_CTRL_LOCK_1,
                                                                 false);
                    r->state = CS35L41_RESET_SM_STATE_WRITE_TRIM_LOCK;
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_WRITE_TRIM_WORD:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_ERROR))
            {
                ret = CS35L41_STATUS_FAIL;
                r->state = CS35L41_RESET_SM_STATE_ERROR;
            }
            else if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                // If there are still trim entries remaining in OTP
                if (r->count < driver->otp_map->num_elements)
                {
                    // Get current trim entry
                    cs35l41_otp_packed_entry_t temp_trim_entry = driver->otp_map->map[r->count];

                    CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Read value of next register to trim
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                temp_trim_entry.reg,
                                                                &(driver->register_buffer),
                                                                false);

                    r->state = CS35L41_RESET_SM_STATE_READ_TRIM_WORD;
                }
                else
                {
                    CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                    r->count = 0;
                    // Lock register file
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS35L41_TEST_KEY_CTRL_LOCK_1,
                                                                 false);
                    r->state = CS35L41_RESET_SM_STATE_WRITE_TRIM_LOCK;
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_WRITE_TRIM_LOCK:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_ERROR))
            {
                ret = CS35L41_STATUS_FAIL;
                r->state = CS35L41_RESET_SM_STATE_ERROR;
            }
            else if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(r->flags, CS35L41_FLAGS_CP_RW_DONE);
                r->count++;
                if (r->count == 1)
                {
                    // Lock register file
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS35L41_TEST_KEY_CTRL_LOCK_2,
                                                                 false);
                }
                else
                {
                    r->count = 0;
                    // Stop clocks to HALO DSP Core
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                                 0,
                                                                 false);
                    r->state = CS35L41_RESET_SM_STATE_WRITE_CCM_CORE_CTRL;
                }
            }
            break;

        case CS35L41_RESET_SM_STATE_WRITE_CCM_CORE_CTRL:
            if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_ERROR))
            {
                ret = CS35L41_STATUS_FAIL;
                r->state = CS35L41_RESET_SM_STATE_ERROR;
            }
            else if (CS35L41_IS_FLAG_SET(r->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                r->state = CS35L41_RESET_SM_STATE_DONE;
            }
            break;

        // For both DONE and ERROR, do nothing
        case CS35L41_RESET_SM_STATE_DONE:
        case CS35L41_RESET_SM_STATE_ERROR:
            break;

        default:
            ret = CS35L41_STATUS_FAIL;
            r->state = CS35L41_RESET_SM_STATE_ERROR;
            break;
    }

    if ((ret != CS35L41_STATUS_OK) || (bsp_status != BSP_STATUS_OK))
    {
        ret = CS35L41_STATUS_FAIL;
        r->state = CS35L41_RESET_SM_STATE_ERROR;
    }

    return ret;
}

#ifdef INCLUDE_FW
/**
 * Boot State Machine
 *
 * Implementation of cs35l41_private_functions_t.boot_sm
 *
 */
static uint32_t cs35l41_boot_sm(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    cs35l41_sm_t *b = &(driver->control_sm);
    cs35l41_boot_config_t *cfg = driver->boot_config;

    if (CS35L41_IS_FLAG_SET(b->flags, CS35L41_FLAGS_CP_RW_ERROR))
    {
        b->state = CS35L41_BOOT_SM_STATE_ERROR;
        ret = CS35L41_STATUS_FAIL;
    }

    switch(b->state)
    {
        case CS35L41_BOOT_SM_STATE_INIT:
            // Validate the boot configuration
            ret = cs35l41_private_functions_g->validate_boot_config(cfg,
                                                                    CS35L41_IS_FLAG_SET(b->flags, \
                                                                            CS35L41_FLAGS_REQUEST_FW_BOOT),
                                                                    CS35L41_IS_FLAG_SET(b->flags, \
                                                                            CS35L41_FLAGS_REQUEST_COEFF_BOOT));
            // If there is a valid boot configuration
            if (ret == CS35L41_STATUS_BOOT_REQUEST)
            {
                b->count = 0;
                CS35L41_CLEAR_FLAG(b->flags, CS35L41_FLAGS_CP_RW_DONE);
                halo_boot_block_t *temp_block;
                // If there are FW blocks to boot
                if (CS35L41_IS_FLAG_SET(b->flags, CS35L41_FLAGS_REQUEST_FW_BOOT))
                {
                    CS35L41_CLEAR_FLAG(b->flags, CS35L41_FLAGS_REQUEST_FW_BOOT);
                    // Get first FW block
                    temp_block = cfg->fw_blocks;
                    b->state = CS35L41_BOOT_SM_STATE_LOAD_FW;
                }
                // Otherwise, it must be COEFF-only boot
                else
                {
                    CS35L41_CLEAR_FLAG(b->flags, CS35L41_FLAGS_REQUEST_COEFF_BOOT);
                    // Get first COEFF block
                    temp_block = cfg->coeff_blocks;
                    b->state = CS35L41_BOOT_SM_STATE_LOAD_COEFF;
                }
                // Write first block (either FW or COEFF) to HALO DSP memory
                ret = cs35l41_private_functions_g->cp_bulk_write(driver,
                                                                 temp_block->address,
                                                                 (uint8_t *) temp_block->bytes,
                                                                 temp_block->block_size);
            }
            break;

        case CS35L41_BOOT_SM_STATE_LOAD_FW:
            if (CS35L41_IS_FLAG_SET(b->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(b->flags, CS35L41_FLAGS_CP_RW_DONE);
                b->count++;

                // If there are remaining FW blocks
                if (b->count < cfg->total_fw_blocks)
                {
                    // Get next FW block
                    halo_boot_block_t *temp_block = cfg->fw_blocks;
                    temp_block += b->count;
                    // Write next FW block to HALO DSP memory
                    ret = cs35l41_private_functions_g->cp_bulk_write(driver,
                                                                     temp_block->address,
                                                                     (uint8_t *) temp_block->bytes,
                                                                     temp_block->block_size);
                }
                else
                {
                    b->count = 0;
                    // If there is also a request to boot COEFF blocks
                    if (CS35L41_IS_FLAG_SET(b->flags, CS35L41_FLAGS_REQUEST_COEFF_BOOT))
                    {
                        CS35L41_CLEAR_FLAG(b->flags, CS35L41_FLAGS_REQUEST_COEFF_BOOT);
                        // Get first COEFF block
                        halo_boot_block_t *temp_block = cfg->coeff_blocks;
                        // Write first COEFF block to HALO DSP memory
                        ret = cs35l41_private_functions_g->cp_bulk_write(driver,
                                                                         temp_block->address,
                                                                         (uint8_t *) temp_block->bytes,
                                                                         temp_block->block_size);
                        b->state = CS35L41_BOOT_SM_STATE_LOAD_COEFF;
                    }
                    else
                    {
                        // Write first post-boot configuration
                        ret = cs35l41_private_functions_g->write_reg(driver,
                                                                     cs35l41_post_boot_config[0],
                                                                     cs35l41_post_boot_config[1],
                                                                     false);
                        b->state = CS35L41_BOOT_SM_STATE_POST_BOOT_CONFIG;
                    }
                }

            }
            break;

        case CS35L41_BOOT_SM_STATE_LOAD_COEFF:
            if (CS35L41_IS_FLAG_SET(b->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(b->flags, CS35L41_FLAGS_CP_RW_DONE);
                b->count++;

                // If there are remaining COEFF blocks
                if (b->count < cfg->total_coeff_blocks)
                {
                    // Get next COEFF block
                    halo_boot_block_t *temp_block = cfg->coeff_blocks;
                    temp_block += b->count;
                    // Write next COEFF block to HALO DSP memory
                    ret = cs35l41_private_functions_g->cp_bulk_write(driver,
                                                                     temp_block->address,
                                                                     (uint8_t *) temp_block->bytes,
                                                                     temp_block->block_size);
                }
                else
                {
                    b->count = 0;
                    // Write first post-boot configuration
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 cs35l41_post_boot_config[0],
                                                                 cs35l41_post_boot_config[1],
                                                                 false);
                    b->state = CS35L41_BOOT_SM_STATE_POST_BOOT_CONFIG;
                }

            }
            break;

        case CS35L41_BOOT_SM_STATE_POST_BOOT_CONFIG:
            if (CS35L41_IS_FLAG_SET(b->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                b->count++;
                // If there are remaining post-boot configuration words
                if (b->count < (sizeof(cs35l41_post_boot_config)/(sizeof(uint32_t) * 2)))
                {
                    CS35L41_CLEAR_FLAG(b->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Write next post-boot configuration
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 cs35l41_post_boot_config[b->count * 2],
                                                                 cs35l41_post_boot_config[(b->count * 2) + 1],
                                                                 false);
                }
                else
                {
                    // If calibration data is valid
                    if (driver->cal_data.is_valid)
                    {
                        CS35L41_CLEAR_FLAG(b->flags, CS35L41_FLAGS_CP_RW_DONE);
                        b->count = 0;
                        // Write calibrated load impedance
                        ret = cs35l41_private_functions_g->write_reg(driver,
                                                                     CS35L41_CAL_R,
                                                                     driver->cal_data.r,
                                                                     false);
                        b->state = CS35L41_BOOT_SM_STATE_APPLY_CAL_DATA;
                    }
                    else
                    {
                        b->state = CS35L41_BOOT_SM_STATE_DONE;
                    }
                }
            }
            break;

        case CS35L41_BOOT_SM_STATE_APPLY_CAL_DATA:
            if (CS35L41_IS_FLAG_SET(b->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(b->flags, CS35L41_FLAGS_CP_RW_DONE);
                b->count++;
                if (b->count == 1)
                {
                    // Write CAL_STATUS
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 CS35L41_CAL_STATUS,
                                                                 CS35L41_CAL_STATUS_CALIB_SUCCESS,
                                                                 false);
                }
                else if (b->count == 2)
                {
                    // Write CAL_CHECKSUM
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 CS35L41_CAL_CHECKSUM,
                                                                 (driver->cal_data.r + \
                                                                         CS35L41_CAL_STATUS_CALIB_SUCCESS),
                                                                 false);
                }
                else
                {
                    b->state = CS35L41_BOOT_SM_STATE_DONE;
                }
            }
            break;

        case CS35L41_BOOT_SM_STATE_DONE:
            break;

        case CS35L41_BOOT_SM_STATE_ERROR:
        default:
            ret = CS35L41_STATUS_FAIL;
            break;
    }

    return ret;
}
#endif // INCLUDE_FW

/**
 * Power Up State Machine
 *
 * Implementation of cs35l41_private_functions_t.power_up_sm
 *
 */
static uint32_t cs35l41_power_up_sm(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    cs35l41_sm_t *sm = &(driver->control_sm);

    if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS35L41_POWER_UP_SM_STATE_ERROR;
        ret = CS35L41_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS35L41_POWER_UP_SM_STATE_INIT:
            sm->count = 0;
            CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);

#ifndef INCLUDE_FW
            // Send first words of Power Up Patch
            ret = cs35l41_private_functions_g->write_reg(driver,
                                                         cs35l41_pup_patch[0],
                                                         cs35l41_pup_patch[1],
                                                         false);
            sm->state = CS35L41_POWER_UP_SM_STATE_PUP_PATCH;
            break;
#else
            // If DSP is NOT booted
            if (driver->state == CS35L41_STATE_STANDBY)
            {
                // Send first words of Power Up Patch
                ret = cs35l41_private_functions_g->write_reg(driver,
                                                             cs35l41_pup_patch[0],
                                                             cs35l41_pup_patch[1],
                                                             false);
                sm->state = CS35L41_POWER_UP_SM_STATE_PUP_PATCH;
            }
            // Otherwise, assume DSP is booted
            else
            {
                sm->count = 0;
                // Send first words of HALO DSP Memory Lock sequence
                ret = cs35l41_private_functions_g->write_reg(driver,
                                                             cs35l41_mem_lock[0],
                                                             cs35l41_mem_lock[1],
                                                             false);
                sm->state = CS35L41_POWER_UP_SM_STATE_LOCK_MEM;
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_LOCK_MEM:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                sm->count++;
                // If there are remaining lock sequence words
                if (sm->count < (sizeof(cs35l41_mem_lock)/(sizeof(uint32_t) * 2)))
                {
                    // Send next words of HALO DSP Memory Lock sequence
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 cs35l41_mem_lock[sm->count * 2],
                                                                 cs35l41_mem_lock[(sm->count * 2) + 1],
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Set first HALO DSP Sample Rate registers to G1R2
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 cs35l41_frame_sync_regs[0],
                                                                 CS35L41_DSP1_SAMPLE_RATE_G1R2,
                                                                 false);
                    sm->state = CS35L41_POWER_UP_SM_STATE_SET_FRAME_SYNC;
                }
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_SET_FRAME_SYNC:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                sm->count++;
                // If there are more Sample Rate registers to write
                if (sm->count < (sizeof(cs35l41_frame_sync_regs)/sizeof(uint32_t)))
                {
                    // Set next HALO DSP Sample Rate register to G1R2
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 cs35l41_frame_sync_regs[sm->count],
                                                                 CS35L41_DSP1_SAMPLE_RATE_G1R2,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Read the HALO DSP CCM control register
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS35L41_POWER_UP_SM_STATE_CLOCKS_TO_DSP;
                }
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_CLOCKS_TO_DSP:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    uint32_t temp_reg = 0;
                    sm->count++;
                    // Enable clocks to HALO DSP core
                    temp_reg |= XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_EN_BITMASK;
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Send first words of Power Up Patch
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 cs35l41_pup_patch[0],
                                                                 cs35l41_pup_patch[1],
                                                                 false);
                    sm->state = CS35L41_POWER_UP_SM_STATE_PUP_PATCH;
                }
            }
            break;
#endif // INCLUDE_FW

        case CS35L41_POWER_UP_SM_STATE_PUP_PATCH:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                sm->count++;
                // If there are remaining Power Up Patch words
                if (sm->count < (sizeof(cs35l41_pup_patch)/(sizeof(uint32_t) * 2)))
                {
                    // Send next words of Power Up Patch
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 cs35l41_pup_patch[sm->count * 2],
                                                                 cs35l41_pup_patch[(sm->count * 2) + 1],
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Read GLOBAL_EN register
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                MSM_GLOBAL_ENABLES_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS35L41_POWER_UP_SM_STATE_SET_GLOBAL_EN;
                }
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_SET_GLOBAL_EN:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    uint32_t temp_reg = 0;
                    sm->count++;
                    // Set GLOBAL_EN
                    temp_reg |= MSM_GLOBAL_ENABLES_GLOBAL_EN_BITMASK;
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 MSM_GLOBAL_ENABLES_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_TIMEOUT);
                    // After setting GLOBAL_EN, wait for at least T_AMP_PUP (1ms)
                    ret = bsp_driver_if_g->set_timer(CS35L41_T_AMP_PUP_MS, cs35l41_timer_callback, driver);
                    sm->state = CS35L41_POWER_UP_SM_STATE_WAIT_T_AMP_PUP;
                }
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_WAIT_T_AMP_PUP:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_TIMEOUT))
            {
#ifndef INCLUDE_FW
                sm->state = CS35L41_POWER_UP_SM_STATE_DONE;
#else
                // If the DSP is NOT booted
                if (driver->state == CS35L41_STATE_STANDBY)
                {
                    sm->state = CS35L41_POWER_UP_SM_STATE_DONE;
                }
                else
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    sm->count = 0;
                    // Clear HALO DSP Virtual MBOX 1 IRQ
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ2_IRQ2_EINT_2_REG,
                                                                 IRQ2_IRQ2_EINT_2_DSP_VIRTUAL1_MBOX_WR_EINT2_BITMASK,
                                                                 false);
                    sm->state = CS35L41_POWER_UP_SM_STATE_MBOX_CLR_UNMASK_IRQ;
                }
#endif // INCLUDE_FW
            }
            break;

#ifdef INCLUDE_FW
        case CS35L41_POWER_UP_SM_STATE_MBOX_CLR_UNMASK_IRQ:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    sm->count++;
                    // Clear HALO DSP Virtual MBOX 2 IRQ
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ1_IRQ1_EINT_2_REG,
                                                                 IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK,
                                                                 false);
                }
                else if (sm->count == 1)
                {
                    sm->count++;
                    // Read IRQ2 Mask register
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                IRQ2_IRQ2_MASK_2_REG,
                                                                &(driver->register_buffer),
                                                                false);
                }
                else if (sm->count == 2)
                {
                    uint32_t temp_reg = driver->register_buffer;
                    sm->count++;

                    // Unmask IRQ for HALO DSP Virtual MBOX 1
                    temp_reg &= ~(IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK);
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ2_IRQ2_MASK_2_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    // Read HALO DSP MBOX Space 2 register
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                DSP_MBOX_DSP_MBOX_2_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS35L41_POWER_UP_SM_STATE_MBOX_READ_STATUS_1;
                }
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_MBOX_READ_STATUS_1:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                driver->mbox_cmd = CS35L41_DSP_MBOX_CMD_NONE;

                // Based on MBOX status, select correct MBOX Command
                switch (driver->register_buffer)
                {
                    case CS35L41_DSP_MBOX_STATUS_RDY_FOR_REINIT:
                        driver->mbox_cmd = CS35L41_DSP_MBOX_CMD_REINIT;
                        break;

                    case CS35L41_DSP_MBOX_STATUS_PAUSED:
                    case CS35L41_DSP_MBOX_STATUS_RUNNING:
                        driver->mbox_cmd = CS35L41_DSP_MBOX_CMD_RESUME;
                        break;

                    default:
                        break;
                }

                // If no command found, indicate ERROR
                if (driver->mbox_cmd == CS35L41_DSP_MBOX_CMD_NONE)
                {
                    sm->state = CS35L41_POWER_UP_SM_STATE_ERROR;
                    ret = CS35L41_STATUS_FAIL;
                }
                else
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Write MBOX command
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG,
                                                                 driver->mbox_cmd,
                                                                 false);
                    sm->state = CS35L41_POWER_UP_SM_STATE_MBOX_WRITE_CMD;
                }
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_MBOX_WRITE_CMD:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_TIMEOUT);
                sm->count = 0;
                // Wait for at least 1ms
                ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, cs35l41_timer_callback, driver);
                sm->state = CS35L41_POWER_UP_SM_STATE_MBOX_WAIT_1MS;

            }
            break;

        case CS35L41_POWER_UP_SM_STATE_MBOX_WAIT_1MS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_TIMEOUT))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                // Poll MBOX IRQ flag
                ret = cs35l41_private_functions_g->read_reg(driver,
                                                            IRQ1_IRQ1_EINT_2_REG,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS35L41_POWER_UP_SM_STATE_MBOX_READ_IRQ;
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_MBOX_READ_IRQ:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                // If MBOX IRQ flag is set
                if (driver->register_buffer & IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK)
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    sm->count = 0;
                    // Clear MBOX IRQ
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ1_IRQ1_EINT_2_REG,
                                                                 IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK,
                                                                 false);
                    sm->state = CS35L41_POWER_UP_SM_STATE_MBOX_MASK_CLR_IRQ;
                }
                // Repeat 1ms delay then poll IRQ 5x
                else if (sm->count < 5)
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_TIMEOUT);
                    // Wait again for at least 1ms
                    ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, cs35l41_timer_callback, driver);
                    sm->state = CS35L41_POWER_UP_SM_STATE_MBOX_WAIT_1MS;
                }
                // If polling finished without MBOX IRQ set, then indicate ERROR
                else
                {
                    ret = CS35L41_STATUS_FAIL;
                    sm->state = CS35L41_POWER_UP_SM_STATE_ERROR;
                }
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_MBOX_MASK_CLR_IRQ:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    sm->count++;
                    // Read IRQ2 Mask register to next re-mask the MBOX IRQ
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                IRQ2_IRQ2_MASK_2_REG,
                                                                &(driver->register_buffer),
                                                                false);
                }
                else if (sm->count == 1)
                {
                    uint32_t temp_reg = driver->register_buffer;
                    sm->count++;

                    // Re-mask the MBOX IRQ
                    temp_reg |= IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK;
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ2_IRQ2_MASK_2_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    // Read the HALO DSP MBOX status
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                DSP_MBOX_DSP_MBOX_2_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS35L41_POWER_UP_SM_STATE_MBOX_READ_STATUS_2;
                }
            }
            break;

        case CS35L41_POWER_UP_SM_STATE_MBOX_READ_STATUS_2:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                // Check if the status is correct for the command just sent
                if (cs35l41_private_functions_g->is_mbox_status_correct(driver->mbox_cmd, driver->register_buffer))
                {
                    sm->state = CS35L41_POWER_UP_SM_STATE_DONE;
                }
                else
                {
                    ret = CS35L41_STATUS_FAIL;
                    sm->state = CS35L41_POWER_UP_SM_STATE_ERROR;
                }
            }
            break;
#endif // INCLUDE_FW

        case CS35L41_POWER_UP_SM_STATE_DONE:
            break;

        case CS35L41_POWER_UP_SM_STATE_ERROR:
        default:
            ret = CS35L41_STATUS_FAIL;
            break;
    }

    if (ret == CS35L41_STATUS_FAIL)
    {
        sm->state = CS35L41_POWER_UP_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Power Down State Machine
 *
 * Implementation of cs35l41_private_functions_t.power_down_sm
 *
 */
static uint32_t cs35l41_power_down_sm(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    cs35l41_sm_t *sm = &(driver->control_sm);

    if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS35L41_POWER_DOWN_SM_STATE_ERROR;
        ret = CS35L41_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS35L41_POWER_DOWN_SM_STATE_INIT:
            sm->count = 0;
            CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
#ifndef INCLUDE_FW
            // Read register for read-modify-write of GLOBAL_EN
            ret = cs35l41_private_functions_g->read_reg(driver,
                                                        MSM_GLOBAL_ENABLES_REG,
                                                        &(driver->register_buffer),
                                                        false);
            sm->state = CS35L41_POWER_DOWN_SM_STATE_CLEAR_GLOBAL_EN;
#else
            // If DSP is NOT booted
            if (driver->state == CS35L41_STATE_POWER_UP)
            {
                // Read register for read-modify-write of GLOBAL_EN
                ret = cs35l41_private_functions_g->read_reg(driver,
                                                            MSM_GLOBAL_ENABLES_REG,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS35L41_POWER_DOWN_SM_STATE_CLEAR_GLOBAL_EN;
            }
            else
            {
                // Clear HALO DSP Virtual MBOX 1 IRQ flag
                ret = cs35l41_private_functions_g->write_reg(driver,
                                                             IRQ2_IRQ2_EINT_2_REG,
                                                             IRQ2_IRQ2_EINT_2_DSP_VIRTUAL1_MBOX_WR_EINT2_BITMASK,
                                                             false);
                sm->state = CS35L41_POWER_DOWN_SM_STATE_MBOX_CLR_UNMASK_IRQ;
            }
#endif // INCLUDE_FW
            break;

#ifdef INCLUDE_FW
#ifdef DEBUG_POWER_DOWN_STOP_DSP
        case CS35L41_POWER_DOWN_SM_STATE_STOP_WDT:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    uint32_t temp_reg = driver->register_buffer;

                    sm->count++;
                    // Clear WDT_EN bit to disable HALO DSP Watchdog Timer
                    temp_reg &= ~(XM_UNPACKED24_DSP1_WDT_CONTROL_DSP1_WDT_EN_BITMASK);
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 XM_UNPACKED24_DSP1_WDT_CONTROL_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Read HALO DSP CCM Core Control register
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_STOP_DSP;
                }
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_STOP_DSP:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    uint32_t temp_reg = driver->register_buffer;

                    sm->count++;
                    // Disable clocks to the HALO DSP core
                    temp_reg &= ~(XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_EN_BITMASK);
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else if (sm->count == 1)
                {
                    sm->count++;
                    // Read SOFT_RESET register
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                XM_UNPACKED24_DSP1_CORE_SOFT_RESET_REG,
                                                                &(driver->register_buffer),
                                                                false);
                }
                else if (sm->count == 2)
                {
                    uint32_t temp_reg = driver->register_buffer;

                    sm->count++;
                    // Initiate a HALO DSP core Soft Reset
                    temp_reg |= XM_UNPACKED24_DSP1_CORE_SOFT_RESET_DSP1_CORE_SOFT_RESET_BITMASK;
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 XM_UNPACKED24_DSP1_CORE_SOFT_RESET_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Read register for GLOBAL_EN bit
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                MSM_GLOBAL_ENABLES_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_CLEAR_GLOBAL_EN;
                }
            }
            break;
#endif // DEBUG_POWER_DOWN_STOP_DSP

        case CS35L41_POWER_DOWN_SM_STATE_MBOX_CLR_UNMASK_IRQ:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    sm->count++;
                    // Clear HALO DSP Virtual MBOX 2 IRQ flag
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ1_IRQ1_EINT_2_REG,
                                                                 IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK,
                                                                 false);
                }
                else if (sm->count == 1)
                {
                    sm->count++;
                    // Read IRQ2 Mask register
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                IRQ2_IRQ2_MASK_2_REG,
                                                                &(driver->register_buffer),
                                                                false);
                }
                else if (sm->count == 2)
                {
                    uint32_t temp_reg = driver->register_buffer;
                    sm->count++;

                    // Clear HALO DSP Virtual MBOX 1 IRQ mask
                    temp_reg &= ~(IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK);
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ2_IRQ2_MASK_2_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    // Send HALO DSP MBOX 'Pause' Command
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG,
                                                                 CS35L41_DSP_MBOX_CMD_PAUSE,
                                                                 false);
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_MBOX_WRITE_CMD;
                }
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_MBOX_WRITE_CMD:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_TIMEOUT);
                sm->count = 0;
                // Wait for at least 1ms
                ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, cs35l41_timer_callback, driver);
                sm->state = CS35L41_POWER_DOWN_SM_STATE_MBOX_WAIT_1MS;

            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_MBOX_WAIT_1MS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_TIMEOUT))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                // Read IRQ1 flag register to poll for MBOX IRQ
                ret = cs35l41_private_functions_g->read_reg(driver,
                                                            IRQ1_IRQ1_EINT_2_REG,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS35L41_POWER_DOWN_SM_STATE_MBOX_READ_IRQ;
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_MBOX_READ_IRQ:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                sm->count++;
                // If MBOX IRQ flag set
                if (driver->register_buffer & IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK)
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    sm->count = 0;
                    // Clear MBOX IRQ flag
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ1_IRQ1_EINT_2_REG,
                                                                 IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK,
                                                                 false);
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_MBOX_MASK_CLR_IRQ;
                }
                // If have not yet polled 5x
                else if (sm->count < 5)
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_TIMEOUT);
                    // Wait at least 1ms
                    ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2MS, cs35l41_timer_callback, driver);
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_MBOX_WAIT_1MS;
                }
                // If MBOX IRQ flag was never set, indicate ERROR
                else
                {
                    ret = CS35L41_STATUS_FAIL;
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_ERROR;
                }
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_MBOX_MASK_CLR_IRQ:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    sm->count++;
                    // Read IRQ2 Mask register to re-mask HALO DSP Virtual MBOX 1 IRQ
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                IRQ2_IRQ2_MASK_2_REG,
                                                                &(driver->register_buffer),
                                                                false);
                }
                else if (sm->count == 1)
                {
                    uint32_t temp_reg = driver->register_buffer;
                    sm->count++;
                    // Re-mask HALO DSP Virtual MBOX 1 IRQ
                    temp_reg |= IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK;
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ2_IRQ2_MASK_2_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    // Read the MBOX status
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                DSP_MBOX_DSP_MBOX_2_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_MBOX_READ_STATUS;
                }
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_MBOX_READ_STATUS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                // Check that MBOX status is correct for 'Pause' command just sent
                if (cs35l41_private_functions_g->is_mbox_status_correct(CS35L41_DSP_MBOX_CMD_PAUSE, driver->register_buffer))
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    sm->count = 0;
                    // Read GLOBAL_EN register in order to clear GLOBAL_EN
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                MSM_GLOBAL_ENABLES_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_CLEAR_GLOBAL_EN;
                }
            }
            break;
#endif // INCLUDE_FW

        case CS35L41_POWER_DOWN_SM_STATE_CLEAR_GLOBAL_EN:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    uint32_t temp_reg = driver->register_buffer;

                    sm->count++;
                    // Clear GLOBAL_EN
                    temp_reg &= ~(MSM_GLOBAL_ENABLES_GLOBAL_EN_BITMASK);
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 MSM_GLOBAL_ENABLES_REG,
                                                                 temp_reg,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Read IRQ1 flag register to poll MSM_PDN_DONE bit
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                IRQ1_IRQ1_EINT_1_REG,
                                                                &(driver->register_buffer),
                                                                false);
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_READ_PDN_IRQ;
                }
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_READ_PDN_IRQ:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                // If MSM_PDN_DONE IRQ flag is set
                if (driver->register_buffer & IRQ1_IRQ1_EINT_1_MSM_PDN_DONE_EINT1_BITMASK)
                {
                    sm->count = 0;
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Clear MSM_PDN_DONE IRQ flag
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 IRQ1_IRQ1_EINT_1_REG,
                                                                 IRQ1_IRQ1_EINT_1_MSM_PDN_DONE_EINT1_BITMASK,
                                                                 false);
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_CLEAR_PDN_IRQ;
                }
                else
                {
                    sm->count++;
                    // Poll MSM_PDN_DONE IRQ flag at least 100x
                    if (sm->count < 100)
                    {
                        CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_TIMEOUT);
                        // Wait at least 1ms until next poll
                        ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_1MS, cs35l41_timer_callback, driver);
                        sm->state = CS35L41_POWER_DOWN_SM_STATE_READ_PDN_IRQ_WAIT;
                    }
                    // If exceeded 100 reads of MSM_PDN_DONE and still clear, then indicate ERROR
                    else
                    {
                        ret = CS35L41_STATUS_FAIL;
                        sm->state = CS35L41_POWER_DOWN_SM_STATE_ERROR;
                    }
                }
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_READ_PDN_IRQ_WAIT:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_TIMEOUT))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                // Read IRQ1 flag register to poll MSM_PDN_DONE again
                ret = cs35l41_private_functions_g->read_reg(driver,
                                                            IRQ1_IRQ1_EINT_1_REG,
                                                            &(driver->register_buffer),
                                                            false);
                sm->state = CS35L41_POWER_DOWN_SM_STATE_READ_PDN_IRQ;
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_CLEAR_PDN_IRQ:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                sm->count = 0;
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                // Send first words of Power Down Patch set
                ret = cs35l41_private_functions_g->write_reg(driver,
                                                             cs35l41_pdn_patch[0],
                                                             cs35l41_pdn_patch[1],
                                                             false);
                sm->state = CS35L41_POWER_DOWN_SM_STATE_WRITE_PDN_PATCH;
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_WRITE_PDN_PATCH:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                sm->count += 2;
                if (sm->count < (sizeof(cs35l41_pdn_patch)/sizeof(uint32_t)))
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Send next words of Power Down Patch set
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 cs35l41_pdn_patch[sm->count],
                                                                 cs35l41_pdn_patch[sm->count + 1],
                                                                 false);
                }
                else
                {
                    sm->state = CS35L41_POWER_DOWN_SM_STATE_DONE;
                }
            }
            break;

        case CS35L41_POWER_DOWN_SM_STATE_DONE:
            break;

        case CS35L41_POWER_DOWN_SM_STATE_ERROR:
        default:
            ret = CS35L41_STATUS_FAIL;
            break;
    }

    if (ret == CS35L41_STATUS_FAIL)
    {
        sm->state = CS35L41_POWER_DOWN_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Configure State Machine
 *
 * Implementation of cs35l41_private_functions_t.configure_sm
 *
 */
static uint32_t cs35l41_configure_sm(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    cs35l41_sm_t *sm = &(driver->control_sm);

    if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS35L41_CONFIGURE_SM_STATE_ERROR;
        ret = CS35L41_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS35L41_CONFIGURE_SM_STATE_INIT:
            sm->count = 0;
            CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
            // Unlock the register file
            ret = cs35l41_private_functions_g->write_reg(driver,
                                                         CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                         CS35L41_TEST_KEY_CTRL_UNLOCK_1,
                                                         false);
            sm->state = CS35L41_CONFIGURE_SM_STATE_UNLOCK_REGS;
            break;

        case CS35L41_CONFIGURE_SM_STATE_UNLOCK_REGS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                sm->count++;
                if (sm->count == 1)
                {
                    // Unlock the register file
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS35L41_TEST_KEY_CTRL_UNLOCK_2,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Read the first of the Configuration Registers
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                cs35l41_config_register_addresses[0],
                                                                &(driver->config_regs.words[0]),
                                                                false);
                    sm->state = CS35L41_CONFIGURE_SM_STATE_READ_REGS;
                }
            }
            break;

        case CS35L41_CONFIGURE_SM_STATE_READ_REGS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                sm->count++;
                // If there are remaining Configuration Registers to read
                if (sm->count < CS35L41_CONFIG_REGISTERS_TOTAL)
                {
                    // Read the next of the Configuration Registers
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                cs35l41_config_register_addresses[sm->count],
                                                                &(driver->config_regs.words[sm->count]),
                                                                false);
                }
                else
                {
                    // Apply audio_config to config_regs
                    ret = cs35l41_private_functions_g->apply_configs(driver);

                    if (ret == CS35L41_STATUS_OK)
                    {
                        // Write new value to first of the Configuration Registers
                        sm->count = 0;
                        ret = cs35l41_private_functions_g->write_reg(driver,
                                                                     cs35l41_config_register_addresses[0],
                                                                     driver->config_regs.words[0],
                                                                     false);
                        sm->state = CS35L41_CONFIGURE_SM_STATE_WRITE_REGS;
                    }
                }
            }
            break;

        case CS35L41_CONFIGURE_SM_STATE_WRITE_REGS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                sm->count++;
                // If there are remaining Configuration Registers to read
                if (sm->count < CS35L41_CONFIG_REGISTERS_TOTAL)
                {
                    // Write new value to next of the Configuration Registers
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 cs35l41_config_register_addresses[sm->count],
                                                                 driver->config_regs.words[sm->count],
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // Re-lock the register file
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS35L41_TEST_KEY_CTRL_LOCK_1,
                                                                 false);
                    sm->state = CS35L41_CONFIGURE_SM_STATE_LOCK_REGS;
                }
            }
            break;

        case CS35L41_CONFIGURE_SM_STATE_LOCK_REGS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                sm->count++;

                if (sm->count == 1)
                {
                    // Re-lock the register file
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG,
                                                                 CS35L41_TEST_KEY_CTRL_LOCK_2,
                                                                 false);
                }
                else
                {
                    sm->state = CS35L41_CONFIGURE_SM_STATE_DONE;
                }
            }
            break;

        case CS35L41_CONFIGURE_SM_STATE_DONE:
            break;

        case CS35L41_CONFIGURE_SM_STATE_ERROR:
        default:
            ret = CS35L41_STATUS_FAIL;
            break;
    }

    if (ret == CS35L41_STATUS_FAIL)
    {
        sm->state = CS35L41_CONFIGURE_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Field Access State Machine
 *
 * Implementation of cs35l41_private_functions_t.field_access_sm
 *
 */
static uint32_t cs35l41_field_access_sm(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    cs35l41_sm_t *sm = &(driver->control_sm);

    if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS35L41_FIELD_ACCESS_SM_STATE_ERROR;
        ret = CS35L41_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS35L41_FIELD_ACCESS_SM_STATE_INIT:
            CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);

            // Read the value from the field address
            ret = cs35l41_private_functions_g->read_reg(driver,
                                                        driver->field_accessor.address,
                                                        &(driver->register_buffer),
                                                        false);
            sm->state = CS35L41_FIELD_ACCESS_SM_STATE_READ_MEM;
            break;

        case CS35L41_FIELD_ACCESS_SM_STATE_READ_MEM:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                // Create bit-wise mask of the bit-field
                uint32_t temp_mask = (~(0xFFFFFFFF << driver->field_accessor.size) << driver->field_accessor.shift);
                uint32_t reg_val = driver->register_buffer;
                // If this is only a GET request
                if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_IS_GET_REQUEST))
                {
                    uint32_t *reg_ptr = (uint32_t *) driver->current_request.arg;
                    // Mask off bit-field and shift down to LS-Bit
                    reg_val &= temp_mask;
                    reg_val >>= driver->field_accessor.shift;
                    *reg_ptr = reg_val;

                    sm->state = CS35L41_FIELD_ACCESS_SM_STATE_DONE;
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

                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Write new register/memory value
                    ret = cs35l41_private_functions_g->write_reg(driver,
                                                                 driver->field_accessor.address,
                                                                 reg_val,
                                                                 false);

                    sm->state = CS35L41_FIELD_ACCESS_SM_STATE_WRITE_MEM;
                }
            }
            break;

        case CS35L41_FIELD_ACCESS_SM_STATE_WRITE_MEM:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                sm->state = CS35L41_FIELD_ACCESS_SM_STATE_DONE;
            }
            break;

        case CS35L41_FIELD_ACCESS_SM_STATE_DONE:
            break;

        case CS35L41_FIELD_ACCESS_SM_STATE_ERROR:
        default:
            ret = CS35L41_STATUS_FAIL;
            break;
    }

    if (ret == CS35L41_STATUS_FAIL)
    {
        sm->state = CS35L41_FIELD_ACCESS_SM_STATE_ERROR;
    }

    return ret;
}

#ifdef INCLUDE_FW
/**
 * Calibration State Machine
 *
 * Implementation of cs35l41_private_functions_t.calibration_sm
 *
 */
static uint32_t cs35l41_calibration_sm(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    cs35l41_sm_t *sm = &(driver->control_sm);

    if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS35L41_CALIBRATION_SM_STATE_ERROR;
        ret = CS35L41_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS35L41_CALIBRATION_SM_STATE_INIT:
            CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);

            // Set the Ambient Temp (deg C)
            ret = cs35l41_private_functions_g->write_reg(driver,
                                                         CS35L41_CAL_AMBIENT,
                                                         driver->ambient_temp_deg_c,
                                                         false);

            sm->state = CS35L41_CALIBRATION_SM_STATE_SET_TEMP;
            break;

        case CS35L41_CALIBRATION_SM_STATE_SET_TEMP:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_TIMEOUT);

                // Wait for at least 2 seconds while DSP FW performs calibration
                ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_2S, cs35l41_timer_callback, driver);

                sm->state = CS35L41_CALIBRATION_SM_STATE_WAIT_2S;
            }
            break;

        case CS35L41_CALIBRATION_SM_STATE_WAIT_2S:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_TIMEOUT))
            {
                sm->count = 0;
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);

                // Read the Calibration Status
                ret = cs35l41_private_functions_g->read_reg(driver,
                                                            CS35L41_CAL_STATUS,
                                                            &(driver->register_buffer),
                                                            false);

                sm->state = CS35L41_CALIBRATION_SM_STATE_READ_DATA;
            }
            break;

        case CS35L41_CALIBRATION_SM_STATE_READ_DATA:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                sm->count++;
                if (sm->count == 1)
                {
                    if (driver->register_buffer == CS35L41_CAL_STATUS_CALIB_SUCCESS)
                    {
                        // Read the Calibration Load Impedance "R"
                        ret = cs35l41_private_functions_g->read_reg(driver,
                                                                    CS35L41_CAL_R,
                                                                    &(driver->register_buffer),
                                                                    false);
                    }
                    else
                    {
                        sm->state = CS35L41_CALIBRATION_SM_STATE_ERROR;
                    }
                }
                else if (sm->count == 2)
                {
                    driver->cal_data.r = driver->register_buffer;
                    // Read the Calibration Checksum
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                CS35L41_CAL_CHECKSUM,
                                                                &(driver->register_buffer),
                                                                false);
                }
                else
                {
                    // Verify the Calibration Checksum
                    if (driver->register_buffer == (driver->cal_data.r + CS35L41_CAL_STATUS_CALIB_SUCCESS))
                    {
                        driver->cal_data.is_valid = true;
                        sm->state = CS35L41_CALIBRATION_SM_STATE_DONE;
                    }
                    else
                    {
                        sm->state = CS35L41_CALIBRATION_SM_STATE_ERROR;
                    }
                }
            }
            break;

        case CS35L41_CALIBRATION_SM_STATE_DONE:
            break;

        case CS35L41_CALIBRATION_SM_STATE_ERROR:
        default:
            ret = CS35L41_STATUS_FAIL;
            break;
    }

    if (ret == CS35L41_STATUS_FAIL)
    {
        sm->state = CS35L41_CALIBRATION_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Get DSP Status State Machine
 *
 * Implementation of cs35l41_private_functions_t.get_dsp_status_sm
 *
 */
static uint32_t cs35l41_get_dsp_status_sm(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    cs35l41_sm_t *sm = &(driver->control_sm);
    // Get pointer to status passed in to Control Request
    cs35l41_dsp_status_t *status = (cs35l41_dsp_status_t *) driver->current_request.arg;

    if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS35L41_GET_DSP_STATUS_SM_STATE_ERROR;
        ret = CS35L41_STATUS_FAIL;
    }

    switch(sm->state)
    {
        case CS35L41_GET_DSP_STATUS_SM_STATE_INIT:
            sm->count = 0;
            CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);

            // Read the first DSP Status field address
            ret = cs35l41_private_functions_g->read_reg(driver,
                                                        cs35l41_dsp_status_addresses[0],
                                                        &(driver->register_buffer),
                                                        false);

            sm->state = CS35L41_GET_DSP_STATUS_SM_STATE_READ_STATUSES_1;
            break;

        case CS35L41_GET_DSP_STATUS_SM_STATE_READ_STATUSES_1:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                status->data.words[sm->count] = driver->register_buffer;
                sm->count++;
                // If there are remaining DSP Status fields to read
                if (sm->count < CS35L41_DSP_STATUS_WORDS_TOTAL)
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    // Read the next DSP Status field address
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                cs35l41_dsp_status_addresses[sm->count],
                                                                &(driver->register_buffer),
                                                                false);
                }
                else
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_TIMEOUT);

                    // Wait at least 10ms
                    ret = bsp_driver_if_g->set_timer(BSP_TIMER_DURATION_10MS, cs35l41_timer_callback, driver);

                    sm->state = CS35L41_GET_DSP_STATUS_SM_STATE_WAIT;
                }
            }
            break;

        case CS35L41_GET_DSP_STATUS_SM_STATE_WAIT:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_TIMEOUT))
            {
                sm->count = 0;
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);

                // Read the first DSP Status field address
                ret = cs35l41_private_functions_g->read_reg(driver,
                                                            cs35l41_dsp_status_addresses[0],
                                                            &(driver->register_buffer),
                                                            false);

                sm->state = CS35L41_GET_DSP_STATUS_SM_STATE_READ_STATUSES_2;
            }
            break;

        case CS35L41_GET_DSP_STATUS_SM_STATE_READ_STATUSES_2:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);

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
                if (sm->count < CS35L41_DSP_STATUS_WORDS_TOTAL)
                {
                    // Read the next DSP Status field address
                    ret = cs35l41_private_functions_g->read_reg(driver,
                                                                cs35l41_dsp_status_addresses[sm->count],
                                                                &(driver->register_buffer),
                                                                false);
                }
                else
                {
                    // Assess if Calibration is applied
                    if ((status->data.cal_set_status == 2) &&
                        (status->data.cal_r_selected == status->data.cal_r) &&
                        (status->data.cal_r == driver->cal_data.r) &&
                        (status->data.cspl_state == 0) &&
                        (status->data.halo_state == 2))
                    {
                        status->is_calibration_applied = true;
                    }

                    sm->state = CS35L41_GET_DSP_STATUS_SM_STATE_DONE;
                }
            }
            break;

        case CS35L41_GET_DSP_STATUS_SM_STATE_DONE:
            break;

        case CS35L41_GET_DSP_STATUS_SM_STATE_ERROR:
        default:
            ret = CS35L41_STATUS_FAIL;
            break;
    }

    if (ret == CS35L41_STATUS_FAIL)
    {
        sm->state = CS35L41_GET_DSP_STATUS_SM_STATE_ERROR;
    }

    return ret;
}
#endif // INCLUDE_FW

/**
 * Event Handler State Machine
 *
 * Implementation of cs35l41_private_functions_t.event_sm
 *
 */
static uint32_t cs35l41_event_sm(void *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    cs35l41_t *d = driver;
    cs35l41_sm_t *sm = &(d->event_sm);


    if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_ERROR))
    {
        sm->state = CS35L41_EVENT_SM_STATE_ERROR;
        ret = CS35L41_STATUS_FAIL;
    }

    switch (sm->state)
    {
        case CS35L41_EVENT_SM_STATE_INIT:
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
                CS35L41_SET_FLAG(d->control_sm.flags, CS35L41_FLAGS_REQUEST_RESTART);
            }

            CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
            sm->count = 0;
            // Read the first IRQ1 flag register
            ret = cs35l41_private_functions_g->read_reg(d,
                                                        IRQ1_IRQ1_EINT_1_REG,
                                                        &d->register_buffer,
                                                        false);
            sm->state = CS35L41_EVENT_SM_STATE_READ_IRQ_STATUS;
            break;
        }

        case CS35L41_EVENT_SM_STATE_READ_IRQ_STATUS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                irq_statuses[sm->count] = d->register_buffer;
                // If more IRQ1 flag registers remain to be read
                if (sm->count < 4)
                {
                    sm->count++;
                    // Read the next IRQ1 flag register
                    ret = cs35l41_private_functions_g->read_reg(d,
                                                                IRQ1_IRQ1_EINT_1_REG + (sm->count * 4),
                                                                &d->register_buffer,
                                                                false);
                }
                else
                {
                    sm->count = 0;
                    // Read the first IRQ1 mask register
                    ret = cs35l41_private_functions_g->read_reg(d,
                                                                IRQ1_IRQ1_MASK_1_REG,
                                                                &d->register_buffer,
                                                                false);
                    sm->state = CS35L41_EVENT_SM_STATE_READ_IRQ_MASK;
                }
            }
            break;

        case CS35L41_EVENT_SM_STATE_READ_IRQ_MASK:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                irq_masks[sm->count] = d->register_buffer;
                // If more IRQ1 mask registers remain to be read
                if (sm->count < 4)
                {
                    sm->count++;
                    // Read the next IRQ1 flag register
                    ret = cs35l41_private_functions_g->read_reg(d,
                                                                IRQ1_IRQ1_MASK_1_REG + (sm->count * 4),
                                                                &d->register_buffer,
                                                                false);
                }
                else
                {
                    uint32_t flags_to_clear = 0;

                    sm->count = 0;
                    flags_to_clear = irq_statuses[0] & ~(irq_masks[0]);

                    // If there are unmasked IRQs, then process
                    if (flags_to_clear)
                    {
                        // Clear any IRQ1 flags from first register
                        ret = cs35l41_private_functions_g->write_reg(d,
                                                                     IRQ1_IRQ1_EINT_1_REG,
                                                                     flags_to_clear,
                                                                     false);

                        sm->state = CS35L41_EVENT_SM_STATE_CLEAR_IRQ_FLAGS;
                    }
                    else
                    {
                        sm->state = CS35L41_EVENT_SM_STATE_DONE;
                    }
                }
            }
            break;

        case CS35L41_EVENT_SM_STATE_CLEAR_IRQ_FLAGS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                // If more IRQ1 flag registers remain to be cleared
                if (sm->count < 4)
                {
                    uint32_t flags_to_clear = 0;

                    sm->count++;
                    // Get the unmasked IRQ1 flags to process
                    flags_to_clear = irq_statuses[sm->count] & ~(irq_masks[sm->count]);
                    // Clear any IRQ1 flags from next register
                    ret = cs35l41_private_functions_g->write_reg(d,
                                                                 IRQ1_IRQ1_EINT_1_REG + (sm->count * 4),
                                                                 flags_to_clear,
                                                                 false);
                }
                else
                {
                    sm->count = 0;
                    // If there are Boost-related Errors, proceed to DISABLE_BOOST
                    if (irq_statuses[0] & CS35L41_INT1_BOOST_IRQ_MASK)
                    {
                        // Read which MSM Blocks are enabled
                        ret = cs35l41_private_functions_g->read_reg(d,
                                                                    MSM_BLOCK_ENABLES_REG,
                                                                    &d->register_buffer,
                                                                    false);
                        sm->state = CS35L41_EVENT_SM_STATE_DISABLE_BOOST;
                    }
                    // IF there are no Boost-related Errors but are Speaker-Safe Mode errors, proceed to TOGGLE_ERR_RLS
                    else if (irq_statuses[0] & CS35L41_INT1_SPEAKER_SAFE_MODE_IRQ_MASK)
                    {
                        // Clear the Error Release register
                        ret = cs35l41_private_functions_g->write_reg(d,
                                                                     MSM_ERROR_RELEASE_REG,
                                                                     0,
                                                                     false);
                        sm->state = CS35L41_EVENT_SM_STATE_TOGGLE_ERR_RLS;
                    }
                    else
                    {
                        // Call BSP Notification Callback
                        if (d->notification_cb != NULL)
                        {
                            uint32_t event_flags = cs35l41_private_functions_g->irq_to_event_id(irq_statuses);
                            d->notification_cb(event_flags, d->notification_cb_arg);
                        }
                        sm->state = CS35L41_EVENT_SM_STATE_DONE;
                    }
                }
            }
            break;

        case CS35L41_EVENT_SM_STATE_DISABLE_BOOST:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                if (sm->count == 0)
                {
                    sm->count++;
                    // Disable Boost converter
                    d->register_buffer &= ~(MSM_BLOCK_ENABLES_BST_EN_BITMASK);
                    ret = cs35l41_private_functions_g->write_reg(d,
                                                                 MSM_BLOCK_ENABLES_REG,
                                                                 d->register_buffer,
                                                                 false);
                }
                else
                {
                    sm->count = 0;

                    // Clear the Error Release register
                    ret = cs35l41_private_functions_g->write_reg(d,
                                                                 MSM_ERROR_RELEASE_REG,
                                                                 0,
                                                                 false);

                    sm->state = CS35L41_EVENT_SM_STATE_TOGGLE_ERR_RLS;
                }
            }
            break;

        case CS35L41_EVENT_SM_STATE_TOGGLE_ERR_RLS:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);

                if (sm->count == 0)
                {
                    sm->count++;
                    // Set the Error Release register
                    ret = cs35l41_private_functions_g->write_reg(d,
                                                                 MSM_ERROR_RELEASE_REG,
                                                                 CS35L41_ERR_RLS_SPEAKER_SAFE_MODE_MASK,
                                                                 false);
                }
                else if (sm->count == 1)
                {
                    sm->count++;
                    // Clear the Error Release register
                    ret = cs35l41_private_functions_g->write_reg(d,
                                                                 MSM_ERROR_RELEASE_REG,
                                                                 0,
                                                                 false);
                }
                else
                {
                    sm->count = 0;

                    // If there are Boost-related Errors, re-enable Boost
                    if (irq_statuses[0] & CS35L41_INT1_BOOST_IRQ_MASK)
                    {
                        // Read register containing BST_EN
                        ret = cs35l41_private_functions_g->read_reg(d,
                                                                    MSM_BLOCK_ENABLES_REG,
                                                                    &d->register_buffer,
                                                                    false);
                        sm->state = CS35L41_EVENT_SM_STATE_ENABLE_BOOST;
                    }
                    else
                    {
                        // Call BSP Notification Callback
                        if (d->notification_cb != NULL)
                        {
                            uint32_t event_flags = cs35l41_private_functions_g->irq_to_event_id(irq_statuses);
                            d->notification_cb(event_flags, d->notification_cb_arg);
                        }
                        sm->state = CS35L41_EVENT_SM_STATE_DONE;
                    }
                }
            }
            break;

        case CS35L41_EVENT_SM_STATE_ENABLE_BOOST:
            if (CS35L41_IS_FLAG_SET(sm->flags, CS35L41_FLAGS_CP_RW_DONE))
            {
                if (sm->count == 0)
                {
                    CS35L41_CLEAR_FLAG(sm->flags, CS35L41_FLAGS_CP_RW_DONE);
                    sm->count++;
                    // Re-enable Boost Converter
                    d->register_buffer |= MSM_BLOCK_ENABLES_BST_EN_BITMASK;
                    ret = cs35l41_private_functions_g->write_reg(d,
                                                                 MSM_BLOCK_ENABLES_REG,
                                                                 d->register_buffer,
                                                                 false);
                }
                else
                {
                    // Call BSP Notification Callback
                    if (d->notification_cb != NULL)
                    {
                        uint32_t event_flags = cs35l41_private_functions_g->irq_to_event_id(irq_statuses);
                        d->notification_cb(event_flags, d->notification_cb_arg);
                    }
                    sm->state = CS35L41_EVENT_SM_STATE_DONE;
                }
            }
            break;

        case CS35L41_EVENT_SM_STATE_DONE:
            break;

        case CS35L41_EVENT_SM_STATE_ERROR:
        default:
            ret = CS35L41_STATUS_FAIL;
            break;
    }

    if (ret == CS35L41_STATUS_FAIL)
    {
        sm->state = CS35L41_EVENT_SM_STATE_ERROR;
    }

    return ret;
}

/**
 * Gets pointer to correct errata based on DEVID/REVID
 *
 * Implementation of cs35l41_private_functions_t.get_errata
 *
 */
static uint32_t cs35l41_get_errata(uint32_t devid, uint32_t revid, const uint32_t **errata)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

    // Only CS35L41 Rev B2 is supported
    if ((devid == CS35L41_DEVID) && (revid == CS35L41_REVID_B2))
    {
        ret = CS35L41_STATUS_OK;

        *errata = cs35l41_revb0_errata_patch;
    }

    return ret;
}

/**
 * Reads contents from a consecutive number of memory addresses
 *
 * Implementation of cs35l41_private_functions_t.cp_bulk_read
 *
 */
static uint32_t cs35l41_cp_bulk_read(cs35l41_t *driver, uint32_t addr, uint32_t length)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

    // Check that 'length' does not exceed the size of the BSP buffer
    if (length <= CS35L41_CP_BULK_READ_LENGTH_BYTES)
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
         * cs35l41_read_reg.
         */
        bsp_status = bsp_driver_if_g->i2c_read_repeated_start(driver->bsp_dev_id,
                                                              driver->cp_write_buffer,
                                                              4,
                                                              (driver->cp_read_buffer + \
                                                                       CS35L41_CP_REG_READ_LENGTH_BYTES),
                                                              (length * 4),
                                                              cs35l41_private_functions_g->cp_read_callback,
                                                              driver);
        if (bsp_status == BSP_STATUS_OK)
        {
            ret = CS35L41_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Applies OTP trim bit-field to current register word value.
 *
 * Implementation of cs35l41_private_functions_t.apply_trim_word
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

#ifdef INCLUDE_FW
/**
 * Validates the boot configuration provided by the BSP.
 *
 * Implementation of cs35l41_private_functions_t.validate_boot_config
 *
 */
static uint32_t cs35l41_validate_boot_config(cs35l41_boot_config_t *config, bool is_fw_boot, bool is_coeff_boot)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

    // Only check config if either FW or COEFF boot or both
    if ((!is_fw_boot) && (!is_coeff_boot))
    {
        ret = CS35L41_STATUS_OK;
    }
    // Check that 'config' is not NULL
    else if (config != NULL)
    {
        ret = CS35L41_STATUS_BOOT_REQUEST;
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
                        ret = CS35L41_STATUS_FAIL;
                        break;
                    }
                }
            }
            else
            {
                ret = CS35L41_STATUS_FAIL;
            }
        }

        // If booting COEFF file
        if (is_coeff_boot)
        {
            // Check that pointer to list of COEFF blocks is not null, nor is size of list 0
            if ((config->coeff_blocks != NULL) && \
                (config->total_coeff_blocks > 0))
            {
                halo_boot_block_t *temp_block_ptr = config->coeff_blocks;
                // Check that number of required COEFF block pointers are NOT 0
                for (int i = 0; i < config->total_coeff_blocks; i++)
                {
                    if ((temp_block_ptr++)->bytes == NULL)
                    {
                        ret = CS35L41_STATUS_FAIL;
                        break;
                    }
                }
            }
            else
            {
                ret = CS35L41_STATUS_FAIL;
            }
        }
    }

    return ret;
}
#endif // INCLUDE_FW

/**
 * Writes from byte array to consecutive number of Control Port memory addresses
 *
 * Implementation of cs35l41_private_functions_t.cp_bulk_write
 *
 */
static uint32_t cs35l41_cp_bulk_write(cs35l41_t *driver, uint32_t addr, uint8_t *bytes, uint32_t length)
{
    uint32_t ret = CS35L41_STATUS_OK;
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
                                               cs35l41_private_functions_g->cp_write_callback,
                                               driver);

    if (bsp_status == BSP_STATUS_FAIL)
    {
        ret = CS35L41_STATUS_FAIL;
    }

    return ret;
}

/**
 * Implements 'copy' method for Control Request Queue contents
 *
 * Implementation of cs35l41_private_functions_t.control_q_copy
 *
 */
static bool cs35l41_control_q_copy(void *from, void *to)
{
    bool ret = false;

    // Check for any NULL pointers
    if ((from != NULL) && (to != NULL))
    {
        cs35l41_control_request_t *from_r, *to_r;
        from_r = (cs35l41_control_request_t *) from;
        to_r = (cs35l41_control_request_t *) to;

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
 * Implementation of cs35l41_private_functions_t.is_control_valid
 *
 */
static uint32_t cs35l41_is_control_valid(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

    // Request is considered invalid if there is no Control Request being processed
    if (driver->control_sm.fp == NULL)
    {
        return ret;
    }

    uint32_t state = driver->state;
    switch (driver->current_request.id)
    {
        case CS35L41_CONTROL_ID_RESET:
            // RESET Control Request is only invalid for UNCONFIGURED and ERROR states, otherwise valid
            if ((state != CS35L41_STATE_UNCONFIGURED) && (state != CS35L41_STATE_ERROR))
            {
                ret = CS35L41_STATUS_OK;
            }
            break;

#ifdef INCLUDE_FW
        case CS35L41_CONTROL_ID_BOOT:
            // BOOT Control Request is only valid for STANDBY state
            if (state == CS35L41_STATE_STANDBY)
            {
                ret = CS35L41_STATUS_OK;
            }
            break;
#endif // INCLUDE_FW

        case CS35L41_CONTROL_ID_CONFIGURE:
        case CS35L41_CONTROL_ID_POWER_UP:
#ifdef INCLUDE_FW
            // CONFIGURE and POWER_UP Control Requests are only valid for STANDBY and DSP_STANDBY states
            if ((state == CS35L41_STATE_STANDBY) || (state == CS35L41_STATE_DSP_STANDBY))
#else
            // CONFIGURE and POWER_UP Control Requests are only valid for STANDBY state
            if (state == CS35L41_STATE_STANDBY)
#endif // INCLUDE_FW
            {
                ret = CS35L41_STATUS_OK;
            }
            break;

        case CS35L41_CONTROL_ID_POWER_DOWN:
            // POWER_DOWN Control Request is only valid for POWER_UP state
#ifdef INCLUDE_FW
        case CS35L41_CONTROL_ID_CALIBRATION:
            // POWER_DOWN and CALIBRATION Control Requests are only valid for POWER_UP and DSP_POWER_UP states
            if ((state == CS35L41_STATE_POWER_UP) || (state == CS35L41_STATE_DSP_POWER_UP))
#else
            if (state == CS35L41_STATE_POWER_UP)
#endif // INCLUDE_FW
            {
                ret = CS35L41_STATUS_OK;
            }
            break;

        case CS35L41_CONTROL_ID_GET_VOLUME:
        case CS35L41_CONTROL_ID_SET_VOLUME:
            // GET_VOLUME and SET_VOLUME Control Requests are always valid
#ifdef INCLUDE_FW
        case CS35L41_CONTROL_ID_GET_HALO_HEARTBEAT:
        case CS35L41_CONTROL_ID_GET_DSP_STATUS:
            // GET_HALO_HEARTBEAT and GET_DSP_STATUS Control Requests are always valid
#endif // INCLUDE_FW
            ret = CS35L41_STATUS_OK;
            break;

        default:
            break;
    }

    return ret;
}

/**
 * Load new Control Request to be processed
 *
 * Implementation of cs35l41_private_functions_t.load_control
 *
 */
static uint32_t cs35l41_load_control(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

    // Only proceed if successful removal of Control Request from Control Request Queue
    if (F_QUEUE_STATUS_OK == f_queue_if_g->remove(&(driver->control_q), &(driver->current_request)))
    {
        /*
         * Reset all Control State Machines by:
         * - clearing flags
         * - assigning state machine function pointer
         * - setting initial state to CS35L41_SM_STATE_INIT
         */
        driver->control_sm.flags = 0;
        switch (driver->current_request.id)
        {
            case CS35L41_CONTROL_ID_RESET:
                driver->control_sm.fp = cs35l41_private_functions_g->reset_sm;
                driver->control_sm.state = CS35L41_SM_STATE_INIT;
                ret = CS35L41_STATUS_OK;
                break;

#ifdef INCLUDE_FW
            case CS35L41_CONTROL_ID_BOOT:
                driver->control_sm.fp = cs35l41_private_functions_g->boot_sm;
                driver->control_sm.state = CS35L41_SM_STATE_INIT;
                // For BOOT Control Request, pass through request argument to state machine flags
                driver->control_sm.flags = (uint32_t) driver->current_request.arg;
                ret = CS35L41_STATUS_OK;
                break;
#endif // INCLUDE_FW

            case CS35L41_CONTROL_ID_POWER_UP:
                driver->control_sm.fp = cs35l41_private_functions_g->power_up_sm;
                driver->control_sm.state = CS35L41_SM_STATE_INIT;
                ret = CS35L41_STATUS_OK;
                break;

            case CS35L41_CONTROL_ID_POWER_DOWN:
                driver->control_sm.fp = cs35l41_private_functions_g->power_down_sm;
                driver->control_sm.state = CS35L41_SM_STATE_INIT;
                ret = CS35L41_STATUS_OK;
                break;

            case CS35L41_CONTROL_ID_CONFIGURE:
                driver->control_sm.fp = cs35l41_private_functions_g->configure_sm;
                driver->control_sm.state = CS35L41_SM_STATE_INIT;
                ret = CS35L41_STATUS_OK;
                break;

            case CS35L41_CONTROL_ID_GET_VOLUME:
                // For a GET request, set the GET_REQUESt flag
                CS35L41_SET_FLAG(driver->control_sm.flags, CS35L41_FLAGS_IS_GET_REQUEST);
            case CS35L41_CONTROL_ID_SET_VOLUME:
                driver->control_sm.fp = cs35l41_private_functions_g->field_access_sm;
                driver->control_sm.state = CS35L41_SM_STATE_INIT;
                // For the GET_/SET_VOLUME Control Requests, setup field_accessor with bit-field information
                driver->field_accessor.address = CS35L41_INTP_AMP_CTRL_REG;
                driver->field_accessor.shift = CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITOFFSET;
                driver->field_accessor.size = CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITWIDTH;
                ret = CS35L41_STATUS_OK;
                break;

#ifdef INCLUDE_FW
            case CS35L41_CONTROL_ID_GET_HALO_HEARTBEAT:
                // For a GET request, set the GET_REQUESt flag
                CS35L41_SET_FLAG(driver->control_sm.flags, CS35L41_FLAGS_IS_GET_REQUEST);
                driver->control_sm.fp = cs35l41_private_functions_g->field_access_sm;
                driver->control_sm.state = CS35L41_SM_STATE_INIT;
                // Setup field_accessor with bit-field information
                driver->field_accessor.address = CS35L41_HALO_HEARTBEAT;
                driver->field_accessor.shift = 0;
                driver->field_accessor.size = 32;
                ret = CS35L41_STATUS_OK;
                break;

            case CS35L41_CONTROL_ID_CALIBRATION:
                driver->control_sm.fp = cs35l41_private_functions_g->calibration_sm;
                // Pass through Ambient Temperature (in degrees C) to Calibration state machine
                driver->ambient_temp_deg_c = (uint32_t) driver->current_request.arg;
                driver->control_sm.state = CS35L41_SM_STATE_INIT;
                ret = CS35L41_STATUS_OK;
                break;

            case CS35L41_CONTROL_ID_GET_DSP_STATUS:
                driver->control_sm.fp = cs35l41_private_functions_g->get_dsp_status_sm;
                driver->control_sm.state = CS35L41_SM_STATE_INIT;
                ret = CS35L41_STATUS_OK;
                break;
#endif // INCLUDE_FW

            default:
                break;
        }
    }

    return ret;
}

#ifdef INCLUDE_FW
/**
 * Check HALO MBOX Status against the MBOX Command sent.
 *
 * Implementation of cs35l41_private_functions_t.is_mbox_status_correct
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
#endif // INCLUDE_FW

/**
 * Maps IRQ Flag to Event ID passed to BSP
 *
 * Implementation of cs35l41_private_functions_t.irq_to_event_id
 *
 */
static uint32_t cs35l41_irq_to_event_id(uint32_t *irq_statuses)
{
    uint32_t temp_event_flag = 0;

    if (irq_statuses[0] & IRQ1_IRQ1_EINT_1_AMP_ERR_EINT1_BITMASK)
    {
        CS35L41_SET_FLAG(temp_event_flag, CS35L41_EVENT_FLAG_AMP_SHORT);
    }
    if (irq_statuses[0] & IRQ1_IRQ1_EINT_1_TEMP_ERR_EINT1_BITMASK)
    {
        CS35L41_SET_FLAG(temp_event_flag, CS35L41_EVENT_FLAG_OVERTEMP);
    }
    if (irq_statuses[0] & IRQ1_IRQ1_EINT_1_BST_SHORT_ERR_EINT1_BITMASK)
    {
        CS35L41_SET_FLAG(temp_event_flag, CS35L41_EVENT_FLAG_BOOST_INDUCTOR_SHORT);
    }
    if (irq_statuses[0] & IRQ1_IRQ1_EINT_1_BST_DCM_UVP_ERR_EINT1_BITMASK)
    {
        CS35L41_SET_FLAG(temp_event_flag, CS35L41_EVENT_FLAG_BOOST_UNDERVOLTAGE);
    }
    if (irq_statuses[0] & IRQ1_IRQ1_EINT_1_BST_OVP_ERR_EINT1_BITMASK)
    {
        CS35L41_SET_FLAG(temp_event_flag, CS35L41_EVENT_FLAG_BOOST_OVERVOLTAGE);
    }

    return temp_event_flag;
}

/**
 * Apply all driver one-time configurations to corresponding Control Port register/memory addresses
 *
 * Implementation of cs35l41_private_functions_t.apply_configs
 *
 */
static uint32_t cs35l41_apply_configs(cs35l41_t *driver)
{
    uint32_t ret = CS35L41_STATUS_OK;
    uint8_t i;
    bool code_found;
    cs35l41_config_registers_t *regs = &(driver->config_regs);

    /*
     * apply audio hw configurations
     */
    cs35l41_audio_hw_config_t *hw = &(driver->audio_config.hw);
    regs->dataif_asp_control3.asp_dout_hiz_ctrl = hw->dout_hiz_ctrl;

    regs->dataif_asp_control2.asp_bclk_mstr = hw->is_master_mode;
    regs->dataif_asp_control2.asp_fsync_mstr = regs->dataif_asp_control2.asp_bclk_mstr;
    regs->dataif_asp_control2.asp_fsync_inv = hw->fsync_inv;
    regs->dataif_asp_control2.asp_bclk_inv = hw->bclk_inv;

    regs->msm_block_enables2.amp_dre_en = hw->amp_dre_en;

    regs->noise_gate_mixer_ngate_ch1_cfg.aux_ngate_ch1_en = hw->ng_enable;
    regs->noise_gate_mixer_ngate_ch2_cfg.aux_ngate_ch2_en = hw->ng_enable;
    regs->noise_gate_mixer_ngate_ch1_cfg.aux_ngate_ch1_hold = hw->ng_delay;
    regs->noise_gate_mixer_ngate_ch2_cfg.aux_ngate_ch2_hold = hw->ng_delay;
    regs->noise_gate_mixer_ngate_ch1_cfg.aux_ngate_ch1_thr = hw->ng_thld;
    regs->noise_gate_mixer_ngate_ch2_cfg.aux_ngate_ch2_thr = hw->ng_thld;

    regs->dre_amp_gain.amp_gain_pcm = hw->amp_gain_pcm;
    regs->intp_amp_ctrl.amp_ramp_pcm = hw->amp_ramp_pcm;

    /*
     * apply audio clocking configurations
     */
    cs35l41_clock_config_t *clk = &(driver->audio_config.clock);

    // apply audio clocking - refclk source
    regs->ccm_refclk_input.pll_refclk_sel = clk->refclk_sel;

    // apply audio clocking - refclk frequency
    code_found = false;
    for (i = 0; i < (sizeof(cs35l41_pll_sysclk)/sizeof(struct cs35l41_register_encoding)); i++)
    {
        if (clk->refclk_freq == cs35l41_pll_sysclk[i].value)
        {
            code_found = true;
            regs->ccm_refclk_input.pll_refclk_freq = cs35l41_pll_sysclk[i].code;
            break;
        }
    }
    if (!code_found)
    {
        ret = CS35L41_STATUS_FAIL;
    }

    // apply audio clocking - sclk frequency
    code_found = false;
    for (i = 0; i < (sizeof(cs35l41_sclk_encoding)/sizeof(struct cs35l41_register_encoding)); i++)
    {
        if (clk->sclk == cs35l41_sclk_encoding[i].value)
        {
            code_found = true;
            regs->dataif_asp_control1.asp_bclk_freq = cs35l41_sclk_encoding[i].code;
            break;
        }
    }
    if (!code_found)
    {
        ret = CS35L41_STATUS_FAIL;
    }

    // The procedure below is taken from the datasheet, Section 4.13.9
    if (clk->sclk > CS35L41_FS_MON0_BETA)
    {
        regs->ccm_fs_mon0 = 0x00024010;
    }
    else
    {
        uint32_t x = 12 * CS35L41_FS_MON0_BETA / clk->sclk + 4;
        uint32_t y = 20 * CS35L41_FS_MON0_BETA / clk->sclk + 4;
        regs->ccm_fs_mon0 = x + (y * 4096);
    }

    // apply audio clocking - FS configuration
    code_found = false;
    for (i = 0; i < (sizeof(cs35l41_fs_rates)/sizeof(struct cs35l41_register_encoding)); i++)
    {
        if (clk->global_fs == cs35l41_fs_rates[i].value)
        {
            code_found = true;
            regs->ccm_global_sample_rate.global_fs = cs35l41_fs_rates[i].code;
            break;
        }
    }
    if (!code_found)
    {
        ret = CS35L41_STATUS_FAIL;
    }

    regs->ccm_refclk_input.pll_refclk_en = 1;

    /*
     * apply audio port configurations
     */
    cs35l41_asp_config_t *asp = &(driver->audio_config.asp);
    if (asp->is_i2s)
    {
        regs->dataif_asp_control2.asp_fmt = CS35L41_ASP_CONTROL2_ASP_FMT_I2S;
    }
    else
    {
        regs->dataif_asp_control2.asp_fmt = CS35L41_ASP_CONTROL2_ASP_FMT_DSPA;
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
    cs35l41_routing_config_t *routing = &(driver->audio_config.routing);
    regs->dacpcm1_input.src = routing->dac_src;
    regs->asptx1_input.src = routing->asp_tx1_src;
    regs->asptx2_input.src = routing->asp_tx2_src;
    regs->asptx3_input.src = routing->asp_tx3_src;
    regs->asptx4_input.src = routing->asp_tx4_src;
    regs->dsp1rx1_input.src = routing->dsp_rx1_src;
    regs->dsp1rx2_input.src = routing->dsp_rx2_src;

    /*
     * apply asp block enable configurations
     */
    regs->dataif_asp_enables1.asp_rx1_en = 0;
    if (cs35l41_private_functions_g->is_mixer_source_used(driver, CS35L41_INPUT_SRC_ASPRX1))
    {
        regs->dataif_asp_enables1.asp_rx1_en = 1;
    }

    regs->dataif_asp_enables1.asp_rx2_en = 0;
    if (cs35l41_private_functions_g->is_mixer_source_used(driver, CS35L41_INPUT_SRC_ASPRX2))
    {
        regs->dataif_asp_enables1.asp_rx2_en = 1;
    }

    if (routing->asp_tx1_src != CS35L41_INPUT_SRC_DISABLE)
    {
        regs->dataif_asp_enables1.asp_tx1_en = 1;
    }
    if (routing->asp_tx2_src != CS35L41_INPUT_SRC_DISABLE)
    {
        regs->dataif_asp_enables1.asp_tx2_en = 1;
    }
    if (routing->asp_tx3_src != CS35L41_INPUT_SRC_DISABLE)
    {
        regs->dataif_asp_enables1.asp_tx3_en = 1;
    }
    if (routing->asp_tx4_src != CS35L41_INPUT_SRC_DISABLE)
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
    cs35l41_amp_config_t *amp = &(driver->amp_config);

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
            ret = CS35L41_STATUS_FAIL;
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
    regs->boost_bst_loop_coeff.bst_k1 = cs35l41_bst_k1_table[lbst_code][cbst_code];
    regs->boost_bst_loop_coeff.bst_k2 = cs35l41_bst_k2_table[lbst_code][cbst_code];
    regs->boost_lbst_slope.bst_lbst_val = lbst_code;
    regs->boost_lbst_slope.bst_slope = cs35l41_bst_slope_table[lbst_code];

    // Bounds check the Peak Current configuration
    if ((amp->boost_ipeak_ma < 1600) || (amp->boost_ipeak_ma > 4500))
    {
        ret = CS35L41_STATUS_FAIL;
    }
    else
    {
        // Encoding corresponds to values in Datasheet Section 7.11.3
        ipk_code = ((amp->boost_ipeak_ma - 1600) / 50) + 0x10;
    }
    regs->boost_bst_ipk_ctl.bst_ipk = ipk_code;

    regs->boost_vbst_ctl_1.bst_ctl = amp->bst_ctl;
    regs->tempmon_warn_limit_threshold.temp_warn_thld = amp->temp_warn_thld;

    // Only if Class H is enabled, then apply Class H configurations
    if (amp->classh_enable)
    {
        regs->boost_vbst_ctl_2.bst_ctl_sel = amp->bst_ctl_sel;
        regs->boost_vbst_ctl_2.bst_ctl_lim_en = (amp->bst_ctl_lim_en ? 1 : 0);
        regs->pwrmgmt_classh_config.ch_mem_depth = amp->ch_mem_depth;
        regs->pwrmgmt_classh_config.ch_hd_rm = amp->ch_hd_rm;
        regs->pwrmgmt_classh_config.ch_rel_rate = amp->ch_rel_rate;
        if (amp->wkfet_amp_thld != CS35L41_WKFET_AMP_THLD_DISABLED)
        {
            regs->pwrmgmt_wkfet_amp_config.wkfet_amp_dly = amp->wkfet_amp_delay;
            regs->pwrmgmt_wkfet_amp_config.wkfet_amp_thld = amp->wkfet_amp_thld;
        }
    }

    /*
     * apply block enable configurations
     */
    // Always enable the Amplifier section
    regs->msm_block_enables.amp_en = 1;

#ifdef INCLUDE_FW
    // If DSP is booted, then turn on some blocks by default
    if (driver->state == CS35L41_STATE_DSP_STANDBY)
    {
        // The DSP needs VMON/IMON data for CSPL
        regs->msm_block_enables.vmon_en = 1;
        regs->msm_block_enables.imon_en = 1;
        // The DSP is using VPMON, CLASSH, and TEMPMON (see cs35l41_post_boot_config[])
        regs->msm_block_enables.vpmon_en = 1;
        regs->msm_block_enables2.classh_en = 1;
        regs->msm_block_enables.tempmon_en = 1;
    }
    // Otherwise, see if the blocks are being used somewhere in order to enable
    else
#endif // INCLUDE_FW
    {
        regs->msm_block_enables2.classh_en = 0;
        if (amp->classh_enable)
        {
            regs->msm_block_enables2.classh_en = 1;
        }

        regs->msm_block_enables.tempmon_en = 0;
        if (cs35l41_private_functions_g->is_mixer_source_used(driver, CS35L41_INPUT_SRC_TEMPMON))
        {
            regs->msm_block_enables.tempmon_en = 1;
        }

        regs->msm_block_enables.vpmon_en = 0;
        if (cs35l41_private_functions_g->is_mixer_source_used(driver, CS35L41_INPUT_SRC_VPMON))
        {
            regs->msm_block_enables.vpmon_en = 1;
        }
    }

    regs->msm_block_enables.vbstmon_en = 0;
    if (cs35l41_private_functions_g->is_mixer_source_used(driver, CS35L41_INPUT_SRC_VBSTMON))
    {
        regs->msm_block_enables.vbstmon_en = 1;
    }

    regs->msm_block_enables2.wkfet_amp_en = 0;
    if (amp->wkfet_amp_thld != CS35L41_WKFET_AMP_THLD_DISABLED)
    {
        regs->msm_block_enables2.wkfet_amp_en = 1;
    }

    // Always configure as Boost converter enabled.
    regs->msm_block_enables.bst_en = 0x2;

    return ret;
}

/**
 * Checks all hardware mixer source selections for a specific source.
 *
 * Implementation of cs35l41_private_functions_t.is_mixer_source_used
 *
 */
static bool cs35l41_is_mixer_source_used(cs35l41_t *driver, uint8_t source)
{
    cs35l41_routing_config_t *routing = &(driver->audio_config.routing);

    if ((routing->dac_src == source) || \
        (routing->asp_tx1_src == source) || \
        (routing->asp_tx2_src == source) || \
        (routing->asp_tx3_src == source) || \
        (routing->asp_tx4_src == source) || \
        (routing->dsp_rx1_src == source) || \
        (routing->dsp_rx2_src == source))
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
static cs35l41_private_functions_t cs35l41_private_functions_s =
{
    .timer_callback = &cs35l41_timer_callback,
    .cp_read_callback = &cs35l41_cp_read_callback,
    .cp_write_callback = &cs35l41_cp_write_callback,
    .irq_callback = &cs35l41_irq_callback,
    .read_reg = &cs35l41_read_reg,
    .write_reg = &cs35l41_write_reg,
    .reset_sm = &cs35l41_reset_sm,
#ifdef INCLUDE_FW
    .boot_sm = &cs35l41_boot_sm,
#endif // INCLUDE_FW
    .power_up_sm = &cs35l41_power_up_sm,
    .power_down_sm = &cs35l41_power_down_sm,
    .configure_sm = &cs35l41_configure_sm,
    .field_access_sm = &cs35l41_field_access_sm,
#ifdef INCLUDE_FW
    .calibration_sm = &cs35l41_calibration_sm,
    .get_dsp_status_sm = &cs35l41_get_dsp_status_sm,
#endif // INCLUDE_FW
    .event_sm = &cs35l41_event_sm,
    .get_errata = &cs35l41_get_errata,
    .cp_bulk_read = &cs35l41_cp_bulk_read,
    .cp_bulk_write = &cs35l41_cp_bulk_write,
    .apply_trim_word = &cs35l41_apply_trim_word,
#ifdef INCLUDE_FW
    .validate_boot_config = &cs35l41_validate_boot_config,
#endif // INCLUDE_FW
    .control_q_copy = &cs35l41_control_q_copy,
    .is_control_valid = &cs35l41_is_control_valid,
    .load_control = &cs35l41_load_control,
#ifdef INCLUDE_FW
    .is_mbox_status_correct = &cs35l41_is_mbox_status_correct,
#endif // INCLUDE_FW
    .irq_to_event_id = &cs35l41_irq_to_event_id,
    .apply_configs = &cs35l41_apply_configs,
    .is_mixer_source_used = &cs35l41_is_mixer_source_used
};

/**
 * Pointer to Private API implementation
 */
cs35l41_private_functions_t *cs35l41_private_functions_g = &cs35l41_private_functions_s;

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize driver state/handle
 *
 * Implementation of cs35l41_functions_t.initialize
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
        // Initialize the Control Request Queue
        ret = f_queue_if_g->initialize(&(driver->control_q),
                                       CS35L41_CONTROL_REQUESTS_SIZE,
                                       driver->control_requests,
                                       sizeof(cs35l41_control_request_t),
                                       cs35l41_private_functions_g->control_q_copy);

        if (ret == F_QUEUE_STATUS_OK)
        {
            ret = CS35L41_STATUS_OK;
        }
        else
        {
            ret = CS35L41_STATUS_FAIL;
        }
    }

    return ret;
}

/**
 * Configures driver state/handle
 *
 * Implementation of cs35l41_functions_t.configure
 *
 */
uint32_t cs35l41_configure(cs35l41_t *driver, cs35l41_config_t *config)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

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
        driver->state = CS35L41_STATE_CONFIGURED;

        driver->audio_config = config->audio_config;
        driver->amp_config = config->amp_config;
#ifdef INCLUDE_FW
        /*
         * Copy the Calibration data.  If it is not valid (is_valid = false), then it will not be sent to the device
         * during boot()
         */
        driver->cal_data = config->cal_data;
#endif // INCLUDE_FW

        ret = bsp_driver_if_g->register_gpio_cb(driver->bsp_int_gpio_id,
                                                cs35l41_private_functions_g->irq_callback,
                                                driver);

        if (ret == BSP_STATUS_OK)
        {
            ret = CS35L41_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Processes driver state machines
 *
 * Implementation of cs35l41_functions_t.process
 *
 */
uint32_t cs35l41_process(cs35l41_t *driver)
{
    uint32_t ret;
    uint32_t status = CS35L41_STATUS_OK;
    uint32_t sm_ret = CS35L41_STATUS_OK;

    // check for driver state
    if ((driver->state != CS35L41_STATE_UNCONFIGURED) && (driver->state != CS35L41_STATE_ERROR))
    {
        // check for driver mode
        if (driver->mode == CS35L41_MODE_HANDLING_EVENTS)
        {
            // run through event sm
            sm_ret = cs35l41_private_functions_g->event_sm(driver);

            if (sm_ret == CS35L41_STATUS_OK)
            {
                // check current status of Event SM
                if (driver->event_sm.state == CS35L41_SM_STATE_DONE)
                {
                    driver->mode = CS35L41_MODE_HANDLING_CONTROLS;

                    // If a control port transaction was interrupted, restart the current Control Request
                    if (CS35L41_IS_FLAG_SET(driver->control_sm.flags, CS35L41_FLAGS_REQUEST_RESTART))
                    {
                        driver->event_sm.state = CS35L41_EVENT_SM_STATE_INIT;
                        // Need to reset current Control SM here
                        driver->control_sm.state = CS35L41_SM_STATE_INIT;
                        driver->control_sm.flags = 0;
                    }
                }
            }
            else
            {
                driver->state = CS35L41_STATE_ERROR;
            }
        }

        // Instead of 'else' here, re-check driver mode in case Event Handler SM previously transitioned to DONE
        if (driver->mode == CS35L41_MODE_HANDLING_CONTROLS)
        {
            bool is_new_request_loaded;

            do
            {
                // Is currently loaded control valid?
                status = cs35l41_private_functions_g->is_control_valid(driver);

                // If invalid, unload it
                if (status == CS35L41_STATUS_INVALID)
                {
                    // Unload control
                    driver->control_sm.fp = NULL;
                    // Call request callback with status
                    cs35l41_control_request_t r = driver->current_request;
                    if (r.cb != NULL)
                    {
                        r.cb(r.id, CS35L41_STATUS_INVALID, r.cb_arg);
                    }
                }
                // Handle currently loaded request
                else if (status == CS35L41_STATUS_OK)
                {
                    // Step through Control SM
                    sm_ret = driver->control_sm.fp(driver);

                    /*
                     *  If Control SM is now in state DONE, update driver state based on which Control Request was
                     *  processed
                     */
                    if (driver->control_sm.state == CS35L41_SM_STATE_DONE)
                    {
                        switch (driver->current_request.id)
                        {
                            case CS35L41_CONTROL_ID_RESET:
#ifdef INCLUDE_FW
                                if ((driver->state == CS35L41_STATE_CONFIGURED) ||
                                    (driver->state == CS35L41_STATE_DSP_STANDBY))
#else
                                if (driver->state == CS35L41_STATE_CONFIGURED)
#endif // INCLUDE_FW
                                {
                                    driver->state = CS35L41_STATE_STANDBY;
                                }
                                break;

#ifdef INCLUDE_FW
                            case CS35L41_CONTROL_ID_BOOT:
                                if (driver->state == CS35L41_STATE_STANDBY)
                                {
                                    driver->state = CS35L41_STATE_DSP_STANDBY;
                                }
                                break;
#endif // INCLUDE_FW

                            case CS35L41_CONTROL_ID_POWER_UP:
                                if (driver->state == CS35L41_STATE_STANDBY)
                                {
                                    driver->state = CS35L41_STATE_POWER_UP;
                                }
#ifdef INCLUDE_FW
                                else if (driver->state == CS35L41_STATE_DSP_STANDBY)
                                {
                                    driver->state = CS35L41_STATE_DSP_POWER_UP;
                                }
#endif // INCLUDE_FW
                                break;

                            case CS35L41_CONTROL_ID_POWER_DOWN:
                                if (driver->state == CS35L41_STATE_POWER_UP)
                                {
                                    driver->state = CS35L41_STATE_STANDBY;
                                }
#ifdef INCLUDE_FW
                                else if (driver->state == CS35L41_STATE_DSP_POWER_UP)
                                {
                                    driver->state = CS35L41_STATE_DSP_STANDBY;
                                }
#endif // INCLUDE_FW
                                break;

                            case CS35L41_CONTROL_ID_CONFIGURE:
                            default:
                                break;
                        }
                    }

                    // If current control SM finished or error, unload it
                    if ((driver->control_sm.state == CS35L41_SM_STATE_DONE) || (sm_ret == CS35L41_STATUS_FAIL))
                    {
                        driver->control_sm.fp = NULL;
                        // Call request callback with status
                        cs35l41_control_request_t r = driver->current_request;
                        if (r.cb != NULL)
                        {
                            r.cb(r.id, sm_ret, r.cb_arg);
                        }

                        if (sm_ret == CS35L41_STATUS_FAIL)
                        {
                            driver->state = CS35L41_STATE_ERROR;
                        }
                    }
                }

                // If previous SM finished without error, try to load a new request from the Control Request Queue
                is_new_request_loaded = false;
                if ((sm_ret != CS35L41_STATUS_FAIL) && (driver->control_sm.fp == NULL))
                {
                    if (CS35L41_STATUS_OK == cs35l41_private_functions_g->load_control(driver))
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
            while ((sm_ret == CS35L41_STATUS_OK) && is_new_request_loaded);
        }

        if (driver->state == CS35L41_STATE_ERROR)
        {
            uint32_t temp_event_flag = 0;
            CS35L41_SET_FLAG(temp_event_flag, CS35L41_EVENT_FLAG_SM_ERROR);
            if (driver->notification_cb != NULL)
            {
                driver->notification_cb(temp_event_flag, driver->notification_cb_arg);
            }
        }
    }

    ret = sm_ret;

    return ret;
}

/**
 * Submit a Control Request to the driver
 *
 * Implementation of cs35l41_functions_t.control
 *
 */
uint32_t cs35l41_control(cs35l41_t *driver, cs35l41_control_request_t req)
{
    uint32_t ret = CS35L41_STATUS_FAIL;

    // Check for valid Control Request ID
    if ((req.id > CS35L41_CONTROL_ID_NONE) && (req.id <= CS35L41_CONTROL_ID_MAX))
    {
        // Insert new request into Control Request Queue
        ret = f_queue_if_g->insert(&(driver->control_q), &req);
        if (ret == F_QUEUE_STATUS_OK)
        {
            ret = CS35L41_STATUS_OK;
        }
    }

    return ret;
}

/**
 * Boot the CS35L41
 *
 * Implementation of cs35l41_functions_t.boot
 *
 */
uint32_t cs35l41_boot(cs35l41_t *driver, cs35l41_control_callback_t cb, void *cb_arg)
{
    uint32_t ret = CS35L41_STATUS_FAIL;
    cs35l41_control_request_t r;

    // Submit request for RESET Control
    r.id = CS35L41_CONTROL_ID_RESET;
    r.cb = cb;
    r.cb_arg = cb_arg;
    ret = cs35l41_functions_g->control(driver, r);

#ifdef INCLUDE_FW
    // Check that RESET Control submitted and that there are blocks of FW to load
    if ((ret == CS35L41_STATUS_OK) && (driver->boot_config->fw_blocks != NULL))
    {
        uint32_t temp_flags = 0;
        r.id = CS35L41_CONTROL_ID_BOOT;
        CS35L41_SET_FLAG(temp_flags, CS35L41_FLAGS_REQUEST_FW_BOOT);
        // Check that there are blocks of COEFF to load
        if (driver->boot_config->coeff_blocks != NULL)
        {
            CS35L41_SET_FLAG(temp_flags, CS35L41_FLAGS_REQUEST_COEFF_BOOT);
        }
        // Pass in flags for FW/COEFF boot to Control SM
        r.arg = (void *) temp_flags;
        // Submit request for BOOT Control
        ret = cs35l41_functions_g->control(driver, r);
    }
#endif // INCLUDE_FW

#ifndef I2S_CONFIG_SHORTCUT
    // If everything is okay, submit request for CONFIGURE Control
    if (ret == CS35L41_STATUS_OK)
    {
        r.id = CS35L41_CONTROL_ID_CONFIGURE;
        ret = cs35l41_functions_g->control(driver, r);
    }
#endif

    return ret;
}

/**
 * Change the power state
 *
 * Implementation of cs35l41_functions_t.power
 *
 */
uint32_t cs35l41_power(cs35l41_t *driver, uint32_t power_state, cs35l41_control_callback_t cb, void *cb_arg)
{
    uint32_t ret = CS35L41_STATUS_FAIL;
    cs35l41_control_request_t r;

    // Submit the correct request based on power_state
    if (power_state == CS35L41_POWER_UP)
    {
        r.id = CS35L41_CONTROL_ID_POWER_UP;
        r.cb = cb;
        r.cb_arg = cb_arg;
        ret = cs35l41_functions_g->control(driver, r);
    }
    else if (power_state == CS35L41_POWER_DOWN)
    {
        r.id = CS35L41_CONTROL_ID_POWER_DOWN;
        r.cb = cb;
        r.cb_arg = cb_arg;
        ret = cs35l41_functions_g->control(driver, r);
    }

    return ret;
}

#ifdef INCLUDE_FW
/**
 * Calibrate the HALO DSP Protection Algorithm
 *
 * Implementation of cs35l41_functions_t.calibrate
 *
 */
uint32_t cs35l41_calibrate(cs35l41_t *driver,
                           uint32_t ambient_temp_deg_c,
                           cs35l41_control_callback_t cb,
                           void *cb_arg)
{
    uint32_t ret = CS35L41_STATUS_FAIL;
    cs35l41_control_request_t r;

    // Submit Control Request for CALIBRATION
    r.id = CS35L41_CONTROL_ID_CALIBRATION;
    r.cb = cb;
    r.cb_arg = cb_arg;
    // Pass in Ambient deg C to Control SM
    r.arg = (void *) ambient_temp_deg_c;
    ret = cs35l41_functions_g->control(driver, r);

    return ret;
}
#endif // INCLUDE_FW

/**
 * Function pointer table for Public API implementation
 *
 * @attention Although not const, this should never be changed run-time in an end-product.  It is implemented this
 * way to facilitate unit testing.
 *
 */
static cs35l41_functions_t cs35l41_functions_s =
{
    .initialize = &cs35l41_initialize,
    .configure = &cs35l41_configure,
    .process = &cs35l41_process,
    .control = &cs35l41_control,
    .boot = &cs35l41_boot,
    .power = &cs35l41_power,
#ifdef INCLUDE_FW
    .calibrate = &cs35l41_calibrate,
#endif // INCLUDE_FW
};

/**
 * Pointer to Public API implementation
 */
cs35l41_functions_t *cs35l41_functions_g = &cs35l41_functions_s;
