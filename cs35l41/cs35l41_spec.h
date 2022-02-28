/**
 * @file cs35l41_spec.h
 *
 * @brief Constants and Types from CS35L41 datasheet DS1215F2
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019-2020, 2022 All Rights Reserved, http://www.cirrus.com/
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

#ifndef CS35L41_SPEC_H
#define CS35L41_SPEC_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>

/***********************************************************************************************************************
 * GENERIC ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * Generic Value-to-Code Encoding Data Structure
 */
struct cs35l41_register_encoding
{
    uint32_t value; ///< Real-world value needing to be encoded
    uint8_t code;   ///< Code corresponding to value
};

/***********************************************************************************************************************
 * LITERALS, CONSTANTS, MACROS, ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * @defgroup CS35L41_DATASHEET
 * @brief All stuff from the datasheet
 *
 * @{
 */

/**
 * @defgroup SECTION_3_CHAR_AND_SPEC
 * @brief Characteristics and Specifications from datasheet
 *
 * @see Datasheet Section 3
 *
 * @{
 */
#define CS35L41_T_AMP_PUP_MS                                                (1)             ///< @see Table 3-4
#define CS35L41_T_RLPW_MS                                                   (2)             ///< @see Table 3-17
#define CS35L41_T_IRS_MS                                                    (1)             ///< @see Table 3-17
/** @} */

/**
 * @defgroup SECTION_4_FUNCTIONAL_DESCRIPTION
 * @brief Functional Description from datasheet
 *
 * @see Datasheet Section 4
 *
 * @{
 */
/**
 * Table for BST_K1 values based on L_BST and C_BST values
 *
 * Table is arranged as:
 * - index0 - L_BST value, in increasing order
 * - index1 - C_BST value, in increasing order
 *
 * @see BOOST_BST_LOOP_COEFF_REG
 * @see Section 4.4.6, Table 4-13
 *
 */
extern const uint8_t cs35l41_bst_k1_table[4][5];

/**
 * Table for BST_K2 values based on L_BST and C_BST values
 *
 * Table is arranged as:
 * - index0 - L_BST value, in increasing order
 * - index1 - C_BST value, in increasing order
 *
 * @see BOOST_BST_LOOP_COEFF_REG
 * @see Section 4.4.6, Table 4-13
 *
 */
extern const uint8_t cs35l41_bst_k2_table[4][5];

/**
 * Table for BST_SLOPE values based on L_BST
 *
 * Table is indexed by L_BST value, in increasing order
 *
 * @see BOOST_LBST_SLOPE_REG
 * @see Section 4.4.6, Table 4-13
 *
 */
extern const uint8_t cs35l41_bst_slope_table[4];

/**
 * CCM_FS_MON_0_REG register address
 *
 * @see Datasheet Section 4.13.9
 *
 */
#define CCM_FS_MON_0_REG                                                    (0x2D10)
/**
 * Beta value used to calculate value for CCM_FS_MON_0_REG
 *
 * @see Datasheet Section 4.13.9
 *
 */
#define CS35L41_FS_MON0_BETA                                                (6000000)
/** @} */

/**
 * @defgroup SECTION_7_1_SW_RESET
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.1
 *
 * @{
 */
#define CS35L41_SW_RESET_DEVID_REG                                          (0x0)           ///< @see Section 7.1.1
#define CS35L41_DEVID                                                       (0x35A40)       ///< @see Section 7.1.1
#define CS35L41R_DEVID                                                      (0x35B40)       ///< @see Section 7.1.1
#define CS35L41_SW_RESET_REVID_REG                                          (0x4)           ///< @see Section 7.1.2
#define CS35L41_SW_RESET_REVID_MTLREVID_BITMASK                             (0xF)           ///< @see Section 7.1.2
#define CS35L41_SW_RESET_REVID_AREVID_BITOFFSET                             (0x4)           ///< @see Section 7.1.2
#define CS35L41_SW_RESET_REVID_AREVID_BITMASK                               (0xF0)          ///< @see Section 7.1.2
#define CS35L41_REVID_B2                                                    (0xB2)          ///< @see Section 7.1.2
#define CS35L41_SW_RESET_OTPID_REG                                          (0x10)          ///< @see Section 7.1.4
#define CS35L41_SW_RESET_OTPID_OTPID_BITMASK                                (0xF)           ///< @see Section 7.1.4
/** @} */

/**
 * @defgroup SECTION_7_2_MSM
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.2
 *
 * @{
 */
#define MSM_GLOBAL_ENABLES_REG                                              (0x2014)        ///< @see Section 7.2.1
#define MSM_GLOBAL_ENABLES_GLOBAL_EN_BITMASK                                (0x1)           ///< @see Section 7.2.1
#define MSM_BLOCK_ENABLES_REG                                               (0x2018)        ///< @see Section 7.2.2
#define MSM_BLOCK_ENABLES_BST_EN_BITMASK                                    (0x30)          ///< @see Section 7.2.2
/**
 * Register definition for MSM_BLOCK_ENABLES_REG
 *
 * @see MSM_BLOCK_ENABLES_REG
 * @see Section 7.2.2
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t amp_en                     : 1;
        uint32_t reserved_0                 : 3;
        uint32_t bst_en                     : 2;
        uint32_t reserved_1                 : 2;
        uint32_t vpmon_en                   : 1;
        uint32_t vbstmon_en                 : 1;
        uint32_t tempmon_en                 : 1;
        uint32_t reserved_2                 : 1;
        uint32_t vmon_en                    : 1;
        uint32_t imon_en                    : 1;
        uint32_t reserved_3                 : 18;
    };
} cs35l41_msm_block_enables_t;

#define MSM_BLOCK_ENABLES2_REG                                              (0x201C)        ///< @see Section 7.2.3
/**
 * Register definition for MSM_BLOCK_ENABLES2_REG
 *
 * @see MSM_BLOCK_ENABLES2_REG
 * @see Section 7.2.3
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t reserved_0                 : 4;
        uint32_t classh_en                  : 1;
        uint32_t reserved_1                 : 3;
        uint32_t sync_en                    : 1;
        uint32_t reserved_2                 : 3;
        uint32_t vpbr_en                    : 1;
        uint32_t vbbr_en                    : 1;
        uint32_t reserved_3                 : 6;
        uint32_t amp_dre_en                 : 1;
        uint32_t reserved_4                 : 3;
        uint32_t wkfet_amp_en               : 1;
        uint32_t reserved_5                 : 7;
    };
} cs35l41_msm_block_enables2_t;

#define MSM_ERROR_RELEASE_REG                                               (0x2034)        ///< @see Section 7.2.6
/** @} */

/**
 * @defgroup SECTION_7_3_PAD_INTF
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.3
 *
 * @{
 */
#define PAD_INTF_GPIO_PAD_CONTROL_REG                                       (0x242C)        ///< @see Section 7.3.3
/**
 * Register definition for PAD_INTF_GPIO_PAD_CONTROL_REG
 *
 * @see PAD_INTF_GPIO_PAD_CONTROL_REG
 * @see Section 7.3.3
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t reserved_0                 : 16;
        uint32_t gp1_ctrl                   : 3;
        uint32_t reserved_1                 : 5;
        uint32_t gp2_ctrl                   : 3;
        uint32_t reserved_2                 : 5;
    };
} cs35l41_pad_intf_gpio_pad_control_t;
/** @} */

/**
 * @defgroup SECTION_7_4_PWRMGT
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.4
 *
 * @{
 */
#define PWRMGT_PWRMGT_CTL                                                   (0x2900)        ///< @see Section 7.4.1
#define PWRMGT_WAKESRC_CTL                                                  (0x2904)        ///< @see Section 7.4.2
#define PWRMGT_PWRMGT_STS                                                   (0x2908)        ///< @see Section 7.4.3
#define PWRMGT_PWRMGT_STS_WR_PENDSTS_BITMASK                                (0x00000002)    ///< @see Section 7.4.3
/** @} */

/**
 * @defgroup SECTION_7_5_CCM
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.5
 *
 * @{
 */
#define CCM_REFCLK_INPUT_REG                                                (0x2C04)        ///< @see Section 7.5.1
#define CS35L41_PLL_REFLCLK_SEL_BCLK                                        (0x0)           ///< @see Section 7.5.1
#define CS35L41_PLL_REFLCLK_SEL_FSYNC                                       (0x1)           ///< @see Section 7.5.1
#define CS35L41_PLL_REFLCLK_SEL_MCLK                                        (0x3)           ///< @see Section 7.5.1
/**
 * Register definition for CCM_REFCLK_INPUT_REG
 *
 * @see CCM_REFCLK_INPUT_REG
 * @see Section 7.5.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t pll_refclk_sel             : 3;
        uint32_t reserved_0                 : 1;
        uint32_t pll_refclk_en              : 1;
        uint32_t pll_refclk_freq            : 6;
        uint32_t pll_open_loop              : 1;
        uint32_t reserved_1                 : 4;
        uint32_t pll_force_en               : 1;
        uint32_t reserved_2                 : 15;
    };
} cs35l41_ccm_refclk_input_t;

/**
 * Encoding for CCM_REFCLK_INPUT_REG field PLL_REFCLK_FREQ
 *
 * Encodes from raw PLL input reference clock frequency in Hertz to bitfield code.
 *
 * @see CCM_REFCLK_INPUT_REG
 * @see Section 7.5.1
 *
 */
extern const struct cs35l41_register_encoding cs35l41_pll_sysclk[64];

#define CCM_GLOBAL_SAMPLE_RATE_REG                                          (0x2C0C)        ///< @see Section 7.5.3
/**
 * Register definition for CCM_GLOBAL_SAMPLE_RATE_REG
 *
 * @see CCM_GLOBAL_SAMPLE_RATE_REG
 * @see Section 7.5.3
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t global_fs                  : 5;
        uint32_t reserved                   : 27;
    };
} cs35l41_ccm_global_sample_rate_t;

/**
 * Encoding for CCM_GLOBAL_SAMPLE_RATE_REG field GLOBAL_FS
 *
 * Encodes from raw sample rate in Hertz to bitfield code.
 *
 * @see CCM_GLOBAL_SAMPLE_RATE_REG
 * @see Section 7.5.3
 *
 */
extern const struct cs35l41_register_encoding cs35l41_fs_rates[13];
/** @} */

/**
 * @defgroup SECTION_7_7_BOOST
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.7
 *
 * @{
 */
#define BOOST_VBST_CTL_1_REG                                                (0x3800)        ///< @see Section 7.7.1
/**
 * Register definition for BOOST_VBST_CTL_1_REG
 *
 * @see BOOST_VBST_CTL_1_REG
 * @see Section 7.7.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t bst_ctl                    : 8;
        uint32_t reserved                   : 24;
    };
} cs35l41_boost_vbst_ctl_1_t;

#define BOOST_VBST_CTL_2_REG                                                (0x3804)        ///< @see Section 7.7.2
/**
 * Register definition for BOOST_VBST_CTL_2_REG
 *
 * @see BOOST_VBST_CTL_2_REG
 * @see Section 7.7.2
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t bst_ctl_sel                : 2;
        uint32_t bst_ctl_lim_en             : 1;
        uint32_t reserved                   : 29;
    };
} cs35l41_boost_vbst_ctl_2_t;

#define BOOST_BST_IPK_CTL_REG                                               (0x3808)        ///< @see Section 7.7.3
/**
 * Register definition for BOOST_BST_IPK_CTL_REG
 *
 * @see BOOST_BST_IPK_CTL_REG
 * @see Section 7.7.3
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t bst_ipk                    : 7;
        uint32_t reserved                   : 25;
    };
} cs35l41_boost_bst_ipk_ctl_t;

#define BOOST_BST_LOOP_COEFF_REG                                            (0x3810)        ///< @see Section 7.7.5
/**
 * Register definition for BOOST_BST_LOOP_COEFF_REG
 *
 * @see BOOST_BST_LOOP_COEFF_REG
 * @see Section 7.7.5
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t bst_k1                     : 8;
        uint32_t bst_k2                     : 8;
        uint32_t reserved                   : 16;
    };
} cs35l41_boost_bst_loop_coeff_t;

#define BOOST_LBST_SLOPE_REG                                                (0x3814)        ///< @see Section 7.7.6
/**
 * Register definition for BOOST_LBST_SLOPE_REG
 *
 * @see BOOST_LBST_SLOPE_REG
 * @see Section 7.7.6
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t bst_lbst_val               : 2;
        uint32_t reserved_0                 : 6;
        uint32_t bst_slope                  : 8;
        uint32_t reserved_1                 : 16;
    };
} cs35l41_boost_lbst_slope_t;

#define BOOST_BST_DCM_CTL_REG                                               (0x381C)        ///< @see Section 7.7.8
/** @} */

/**
 * @defgroup SECTION_7_9_TEMPMON
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.9
 *
 * @{
 */
#define TEMPMON_WARN_LIMIT_THRESHOLD_REG                                    (0x4220)        ///< @see Section 7.9.1
/**
 * Register definition for TEMPMON_WARN_LIMIT_THRESHOLD_REG
 *
 * @see TEMPMON_WARN_LIMIT_THRESHOLD_REG
 * @see Section 7.9.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t temp_warn_thld             : 2;
        uint32_t reserved                   : 30;
    };
} cs35l41_tempmon_warn_limit_threshold_t;
/** @} */

/**
 * @defgroup SECTION_7_10_DATAIF
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.10
 *
 * @{
 */
#define DATAIF_ASP_ENABLES1_REG                                             (0x4800)        ///< @see Section 7.10.1
/**
 * Register definition for DATAIF_ASP_ENABLES1_REG
 *
 * @see DATAIF_ASP_ENABLES1_REG
 * @see Section 7.10.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t asp_tx1_en                 : 1;
        uint32_t asp_tx2_en                 : 1;
        uint32_t asp_tx3_en                 : 1;
        uint32_t asp_tx4_en                 : 1;
        uint32_t reserved_0                 : 12;
        uint32_t asp_rx1_en                 : 1;
        uint32_t asp_rx2_en                 : 1;
        uint32_t reserved_1                 : 14;
    };
} cs35l41_dataif_asp_enables1_t;

#define DATAIF_ASP_CONTROL1_REG                                             (0x4804)        ///< @see Section 7.10.2
/**
 * Register definition for DATAIF_ASP_CONTROL1_REG
 *
 * @see DATAIF_ASP_CONTROL1_REG
 * @see Section 7.10.2
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t asp_bclk_freq              : 6;
        uint32_t reserved                   : 16;
    };
} cs35l41_dataif_asp_control1_t;


/**
 * Encoding for DATAIF_ASP_CONTROL1_REG field ASP_BCLK_FREQ
 *
 * Encodes from raw ASP_BCLK frequency in Hertz to bitfield code.
 *
 * @see DATAIF_ASP_CONTROL1_REG
 * @see Section 7.10.2
 *
 */
extern const struct cs35l41_register_encoding cs35l41_sclk_encoding[48];

#define DATAIF_ASP_CONTROL2_REG                                             (0x4808)        ///< @see Section 7.10.3
#define CS35L41_ASP_CONTROL2_ASP_FMT_DSPA                                   (0x0)           ///< @see Section 7.10.3
#define CS35L41_ASP_CONTROL2_ASP_FMT_I2S                                    (0x2)           ///< @see Section 7.10.3
/**
 * Register definition for DATAIF_ASP_CONTROL2_REG
 *
 * @see DATAIF_ASP_CONTROL2_REG
 * @see Section 7.10.3
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t asp_fsync_mstr             : 1;
        uint32_t asp_fsync_frc              : 1;
        uint32_t asp_fsync_inv              : 1;
        uint32_t reserved_0                 : 1;
        uint32_t asp_bclk_mstr              : 1;
        uint32_t asp_bclk_frc               : 1;
        uint32_t asp_bclk_inv               : 1;
        uint32_t reserved_1                 : 1;
        uint32_t asp_fmt                    : 3;
        uint32_t reserved_2                 : 5;
        uint32_t asp_tx_width               : 8;
        uint32_t asp_rx_width               : 8;
    };
} cs35l41_dataif_asp_control2_t;

#define DATAIF_ASP_CONTROL3_REG                                             (0x480C)        ///< @see Section 7.10.4
/**
 * Register definition for DATAIF_ASP_CONTROL3_REG
 *
 * @see DATAIF_ASP_CONTROL3_REG
 * @see Section 7.10.4
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t asp_dout_hiz_ctrl          : 2;
        uint32_t reserved                   : 30;
    };
} cs35l41_dataif_asp_control3_t;

#define DATAIF_ASP_FRAME_CONTROL1_REG                                       (0x4810)        ///< @see Section 7.10.5
/**
 * Register definition for DATAIF_ASP_FRAME_CONTROL1_REG
 *
 * @see DATAIF_ASP_FRAME_CONTROL1_REG
 * @see Section 7.10.5
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t asp_tx1_slot               : 6;
        uint32_t reserved_0                 : 2;
        uint32_t asp_tx2_slot               : 6;
        uint32_t reserved_1                 : 2;
        uint32_t asp_tx3_slot               : 6;
        uint32_t reserved_2                 : 2;
        uint32_t asp_tx4_slot               : 6;
        uint32_t reserved_3                 : 2;
    };
} cs35l41_dataif_asp_frame_control1_t;

#define DATAIF_ASP_FRAME_CONTROL5_REG                                       (0x4820)        ///< @see Section 7.10.6
/**
 * Register definition for DATAIF_ASP_FRAME_CONTROL5_REG
 *
 * @see DATAIF_ASP_FRAME_CONTROL5_REG
 * @see Section 7.10.6
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t asp_rx1_slot               : 6;
        uint32_t reserved_0                 : 2;
        uint32_t asp_rx2_slot               : 6;
        uint32_t reserved_1                 : 18;
    };
} cs35l41_dataif_asp_frame_control5_t;

#define DATAIF_ASP_DATA_CONTROL1_REG                                        (0x4830)        ///< @see Section 7.10.7
/**
 * Register definition for DATAIF_ASP_DATA_CONTROL1_REG
 *
 * @see DATAIF_ASP_DATA_CONTROL1_REG
 * @see Section 7.10.7
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t asp_tx_wl                  : 6;
        uint32_t reserved                   : 26;
    };
} cs35l41_dataif_asp_data_control1_t;

#define DATAIF_ASP_DATA_CONTROL5_REG                                        (0x4840)        ///< @see Section 7.10.8
/**
 * Register definition for DATAIF_ASP_DATA_CONTROL5_REG
 *
 * @see DATAIF_ASP_DATA_CONTROL5_REG
 * @see Section 7.10.8
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t asp_rx_wl                  : 6;
        uint32_t reserved                   : 26;
    };
} cs35l41_dataif_asp_data_control5_t;
/** @} */

/**
 * @defgroup SECTION_7_11_MIXER
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.11
 *
 * @{
 */
#define CS35L41_MIXER_DACPCM1_INPUT_REG                                     (0x4C00)        ///< @see Section 7.11.1
#define CS35L41_MIXER_ASPTX1_INPUT_REG                                      (0x4C20)        ///< @see Section 7.11.2
#define CS35L41_MIXER_ASPTX2_INPUT_REG                                      (0x4C24)        ///< @see Section 7.11.3
#define CS35L41_MIXER_ASPTX3_INPUT_REG                                      (0x4C28)        ///< @see Section 7.11.4
#define CS35L41_MIXER_ASPTX4_INPUT_REG                                      (0x4C2C)        ///< @see Section 7.11.5
#define CS35L41_MIXER_DSP1RX1_INPUT_REG                                     (0x4C40)        ///< @see Section 7.11.6
#define CS35L41_MIXER_DSP1RX2_INPUT_REG                                     (0x4C44)        ///< @see Section 7.11.7
#define CS35L41_MIXER_DSP1RX5_INPUT_REG                                     (0x4C50)        ///< @see Section 7.11.10
#define CS35L41_MIXER_DSP1RX6_INPUT_REG                                     (0x4C54)        ///< @see Section 7.11.11
#define CS35L41_MIXER_DSP1RX7_INPUT_REG                                     (0x4C58)        ///< @see Section 7.11.12
#define CS35L41_MIXER_DSP1RX8_INPUT_REG                                     (0x4C5C)        ///< @see Section 7.11.13

/**
 * @defgroup CS35L41_INPUT_SRC_
 * @brief Settings for MIXER Source Values
 *
 * @details See datasheet Section 7.11.1 - MIXER Source Values
 *
 * @{
 */
#define CS35L41_INPUT_SRC_ZERO_FILL                                         (0x00)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_DIAG_GEN                                          (0x04)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_ASPRX1                                            (0x08)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_ASPRX2                                            (0x09)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_VMON                                              (0x18)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_IMON                                              (0x19)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_ERR_VOL                                           (0x20)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_CLASSH                                            (0x21)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_VPMON                                             (0x28)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_VBSTMON                                           (0x29)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_DSP1TX1                                           (0x32)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_DSP1TX2                                           (0x33)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_DSP1TX3                                           (0x34)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_DSP1TX4                                           (0x35)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_DSP1TX5                                           (0x36)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_DSP1TX6                                           (0x37)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_DSP1TX7                                           (0x38)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_DSP1TX8                                           (0x39)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_TEMPMON                                           (0x3A)          ///< @see Section 7.11.1
#define CS35L41_INPUT_SRC_RSVD                                              (0x3B)          ///< @see Section 7.11.1
/** @} */

/**
 * Register definition for CS35L41_MIXER_DACPCM1_INPUT_REG to CS35L41_MIXER_DSP1RX8_INPUT_REG
 *
 * @see Sections 7.11.1 to 7.11.15
 * @see CS35L41_INPUT_SRC_
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t src                        : 7;
        uint32_t reserved                   : 25;
    };
} cs35l41_mixer_t;
/** @} */

/**
 * @defgroup SECTION_7_12_INTP
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.12
 *
 * @{
 */
#define CS35L41_INTP_AMP_CTRL_REG                                           (0x6000)        ///< @see Section 7.12.1
#define CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITOFFSET                         (0x3)           ///< @see Section 7.12.1
#define CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITWIDTH                          (0xB)           ///< @see Section 7.12.1
#define CS35L41_INTP_AMP_CTRL_AMP_VOL_PCM_BITMASK                           (0x3FF8)        ///< @see Section 7.12.1

#define CS35L42_AMP_VOL_PCM_MUTE                                            (0x400)         ///< @see Section 7.12.1
#define CS35L42_AMP_VOL_PCM_0DB                                             (0)             ///< @see Section 7.12.1
#define CS35L42_AMP_VOL_PCM_MAX_DB                                          (12)            ///< @see Section 7.12.1
#define CS35L42_AMP_VOL_PCM_MIN_DB                                          (-102)          ///< @see Section 7.12.1


/**
 * Register definition for CS35L41_INTP_AMP_CTRL_REG
 *
 * @see CS35L41_INTP_AMP_CTRL_REG
 * @see Sections 7.12.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t amp_ramp_pcm               : 3;
        uint32_t amp_vol_pcm                : 11;
        uint32_t amp_inv_pcm                : 1;
        uint32_t amp_hpf_pcm_en             : 1;
        uint32_t reserved                   : 16;
    };
} cs35l41_intp_amp_ctrl_t;
/** @} */


/**
 * @defgroup SECTION_7_14_PWRMGMT
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.14
 *
 * @{
 */
#define PWRMGMT_CLASSH_CONFIG_REG                                           (0x6800)        ///< @see Section 7.14.1
/**
 * Register definition for PWRMGMT_CLASSH_CONFIG_REG
 *
 * @see PWRMGMT_CLASSH_CONFIG_REG
 * @see Sections 7.14.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t ch_mem_depth               : 3;
        uint32_t reserved_0                 : 5;
        uint32_t ch_rel_rate                : 8;
        uint32_t ch_hd_rm                   : 7;
        uint32_t reserved                   : 9;
    };
} cs35l41_pwrmgmt_classh_config_t;

#define PWRMGMT_WKFET_AMP_CONFIG_REG                                        (0x6804)        ///< @see Section 7.14.2
/**
 * Register definition for PWRMGMT_WKFET_AMP_CONFIG_REG
 *
 * @see PWRMGMT_WKFET_AMP_CONFIG_REG
 * @see Sections 7.14.2
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t wkfet_amp_frc              : 1;
        uint32_t wkfet_amp_frc_en           : 1;
        uint32_t wkfet_amp_dly              : 3;
        uint32_t reserved_0                 : 3;
        uint32_t wkfet_amp_thld             : 4;
        uint32_t reserved_1                 : 20;
    };
} cs35l41_pwrmgmt_wkfet_amp_config_t;
/** @} */

/**
 * @defgroup SECTION_7_15_DRE
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.15
 *
 * @{
 */
#define CS35L41_DRE_AMP_GAIN_REG                                            (0x6C04)        ///< @see Section 7.15.1
#define CS35L41_DRE_AMP_GAIN_DEFAULT                                        (0x13)          ///< @see Section 7.15.1
/**
 * Register definition for CS35L41_DRE_AMP_GAIN_REG
 *
 * @see CS35L41_DRE_AMP_GAIN_REG
 * @see Sections 7.15.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t amp_gain_pdm               : 5;
        uint32_t amp_gain_pcm               : 5;
        uint32_t amp_gain_zc                : 1;
        uint32_t reserved                   : 21;
    };
} cs35l41_dre_amp_gain_t;
/** @} */

/**
 * @defgroup SECTION_7_18_IRQ1
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.18
 *
 * @{
 */
#define IRQ1_IRQ1_EINT_1_REG                                                (0x10010)       ///< @see Section 7.18.3
#define IRQ1_IRQ1_EINT_1_BST_OVP_ERR_EINT1_BITMASK                          (0x40)          ///< @see Section 7.18.3
#define IRQ1_IRQ1_EINT_1_BST_DCM_UVP_ERR_EINT1_BITMASK                      (0x80)          ///< @see Section 7.18.3
#define IRQ1_IRQ1_EINT_1_BST_SHORT_ERR_EINT1_BITMASK                        (0x100)         ///< @see Section 7.18.3
#define IRQ1_IRQ1_EINT_1_TEMP_WARN_RISE_EINT1_BITMASK                       (0x8000)        ///< @see Section 7.18.3
#define IRQ1_IRQ1_EINT_1_TEMP_ERR_EINT1_BITMASK                             (0x20000)       ///< @see Section 7.18.3
#define IRQ1_IRQ1_EINT_1_MSM_PDN_DONE_EINT1_BITMASK                         (0x800000)      ///< @see Section 7.18.3
#define IRQ1_IRQ1_EINT_1_AMP_ERR_EINT1_BITMASK                              (0x80000000)    ///< @see Section 7.18.3
#define IRQ1_IRQ1_EINT_2_REG                                                (0x10014)       ///< @see Section 7.18.4
#define IRQ1_IRQ1_EINT_2_DSP_VIRTUAL2_MBOX_WR_EINT1_BITMASK                 (0x200000)      ///< @see Section 7.18.4
#define IRQ1_IRQ1_MASK_1_REG                                                (0x10110)       ///< @see Section 7.18.11
/** @} */

/**
 * @defgroup SECTION_7_19_IRQ2
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.19
 *
 * @{
 */
#define IRQ2_IRQ2_EINT_2_REG                                                (0x10814)       ///< @see Section 7.19.4
#define IRQ2_IRQ2_EINT_2_DSP_VIRTUAL1_MBOX_WR_EINT2_BITMASK                 (0x100000)      ///< @see Section 7.19.4
#define IRQ2_IRQ2_MASK_2_REG                                                (0x10914)       ///< @see Section 7.19.12
#define IRQ2_IRQ2_MASK_2_DSP_VIRTUAL1_MBOX_WR_MASK2_BITMASK                 (0x100000)      ///< @see Section 7.19.12
/** @} */

/**
 * @defgroup SECTION_7_20_GPIO
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.20
 *
 * @{
 */
#define GPIO_STATUS1_REG                                                    (0x11000)       ///< @see Section 7.20.1
/**
 * Register definition for GPIO_STATUS1_REG
 *
 * @see GPIO_STATUS1_REG
 * @see Sections 7.20.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t gp1_sts                    : 1;
        uint32_t gp2_sts                    : 1;
        uint32_t gp3_sts                    : 1;
        uint32_t gp4_sts                    : 1;
        uint32_t reserved                   : 28;
    };
} cs35l41_gpio_status1_t;

#define GPIO_GPIO1_CTRL1_REG                                                (0x11008)       ///< @see Section 7.20.2
#define GPIO_GPIO2_CTRL1_REG                                                (0x1100C)       ///< @see Section 7.20.3
#define GPIO_GPIO3_CTRL1_REG                                                (0x11010)       ///< @see Section 7.20.4
#define GPIO_GPIO4_CTRL1_REG                                                (0x11014)       ///< @see Section 7.20.5
/**
 * Register definition for GPIO_GPIOx_CTRL1_REG
 *
 * @see GPIO_GPIO1_CTRL1_REG
 * @see Sections 7.20.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t reserved_0                 : 12;
        uint32_t gp_pol                     : 1;
        uint32_t gp_db                      : 1;
        uint32_t reserved_1                 : 1;
        uint32_t gp_lvl                     : 1;
        uint32_t gp_dbtime                  : 4;
        uint32_t reserved_2                 : 11;
        uint32_t gp_dir                     : 1;
    };
} cs35l41_gpio_ctrl1_t;
/** @} */

/**
 * @defgroup SECTION_7_21_NOISE_GATE
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.21
 *
 * @{
 */
#define NOISE_GATE_MIXER_NGATE_CH1_CFG_REG                                  (0x12004)       ///< @see Section 7.21.1
/**
 * Register definition for NOISE_GATE_MIXER_NGATE_CH1_CFG_REG
 *
 * @see NOISE_GATE_MIXER_NGATE_CH1_CFG_REG
 * @see Sections 7.21.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t aux_ngate_ch1_thr          : 3;
        uint32_t reserved_0                 : 5;
        uint32_t aux_ngate_ch1_hold         : 4;
        uint32_t reserved_1                 : 4;
        uint32_t aux_ngate_ch1_en           : 1;
        uint32_t reserved_2                 : 15;
    };
} cs35l41_noise_gate_mixer_ngate_ch1_cfg_t;

#define NOISE_GATE_MIXER_NGATE_CH2_CFG_REG                                  (0x12008)       ///< @see Section 7.21.2
/**
 * Register definition for NOISE_GATE_MIXER_NGATE_CH2_CFG_REG
 *
 * @see NOISE_GATE_MIXER_NGATE_CH2_CFG_REG
 * @see Sections 7.21.2
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t aux_ngate_ch2_thr          : 3;
        uint32_t reserved_0                 : 5;
        uint32_t aux_ngate_ch2_hold         : 4;
        uint32_t reserved_1                 : 4;
        uint32_t aux_ngate_ch2_en           : 1;
        uint32_t reserved_2                 : 15;
    };
} cs35l41_noise_gate_mixer_ngate_ch2_cfg_t;
/** @} */

/**
 * @defgroup SECTION_RESERVED
 * @brief Reserved definitions required by the driver
 *
 * @{
 */
#define CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG                                 (0x40)
#define CS35L41_TEST_KEY_CTRL_UNLOCK_1                                      (0x00000055)
#define CS35L41_TEST_KEY_CTRL_UNLOCK_2                                      (0x000000AA)
#define CS35L41_TEST_KEY_CTRL_LOCK_1                                        (0x000000CC)
#define CS35L41_TEST_KEY_CTRL_LOCK_2                                        (0x00000033)

#define CS35L41_OTP_IF_OTP_MEM0_REG                                         (0x400)
#define CS35L41_OTP_CTRL_OTP_CTRL8_REG                                      (0x51C)
#define OTP_CTRL_OTP_CTRL8_OTP_BOOT_DONE_STS_BITMASK                        (0x4)

#define DSP_MBOX_DSP_MBOX_2_REG                                             (0x13004)
#define DSP_VIRTUAL1_MBOX_DSP_VIRTUAL1_MBOX_1_REG                           (0x13020)

#define XM_UNPACKED24_DSP1_SAMPLE_RATE_RX1_REG                              (0x2B80080)
#define CS35L41_DSP1_SAMPLE_RATE_G1R2                                       (0x00000001)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_RX2_REG                              (0x2B80088)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_RX3_REG                              (0x2B80090)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_RX4_REG                              (0x2B80098)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_RX5_REG                              (0x2B800A0)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_RX6_REG                              (0x2B800A8)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_RX7_REG                              (0x2B800B0)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_RX8_REG                              (0x2B800B8)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_TX1_REG                              (0x2B80280)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_TX2_REG                              (0x2B80288)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_TX3_REG                              (0x2B80290)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_TX4_REG                              (0x2B80298)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_TX5_REG                              (0x2B802A0)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_TX6_REG                              (0x2B802A8)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_TX7_REG                              (0x2B802B0)
#define XM_UNPACKED24_DSP1_SAMPLE_RATE_TX8_REG                              (0x2B802B8)

#define XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_REG                             (0x2BC1000)
#define XM_UNPACKED24_DSP1_CCM_CORE_CONTROL_DSP1_CCM_CORE_EN_BITMASK        (0x1)

#define XM_UNPACKED24_DSP1_MPU_XMEM_ACCESS_0_REG                            (0x2BC3000)
#define XM_UNPACKED24_DSP1_MPU_YMEM_ACCESS_0_REG                            (0x2BC3004)
#define XM_UNPACKED24_DSP1_MPU_WINDOW_ACCESS_0_REG                          (0x2BC3008)
#define XM_UNPACKED24_DSP1_MPU_XREG_ACCESS_0_REG                            (0x2BC300C)
#define XM_UNPACKED24_DSP1_MPU_YREG_ACCESS_0_REG                            (0x2BC3014)
#define XM_UNPACKED24_DSP1_MPU_XMEM_ACCESS_1_REG                            (0x2BC3018)
#define XM_UNPACKED24_DSP1_MPU_YMEM_ACCESS_1_REG                            (0x2BC301C)
#define XM_UNPACKED24_DSP1_MPU_WINDOW_ACCESS_1_REG                          (0x2BC3020)
#define XM_UNPACKED24_DSP1_MPU_XREG_ACCESS_1_REG                            (0x2BC3024)
#define XM_UNPACKED24_DSP1_MPU_YREG_ACCESS_1_REG                            (0x2BC302C)
#define XM_UNPACKED24_DSP1_MPU_XMEM_ACCESS_2_REG                            (0x2BC3030)
#define XM_UNPACKED24_DSP1_MPU_YMEM_ACCESS_2_REG                            (0x2BC3034)
#define XM_UNPACKED24_DSP1_MPU_WINDOW_ACCESS_2_REG                          (0x2BC3038)
#define XM_UNPACKED24_DSP1_MPU_XREG_ACCESS_2_REG                            (0x2BC303C)
#define XM_UNPACKED24_DSP1_MPU_YREG_ACCESS_2_REG                            (0x2BC3044)
#define XM_UNPACKED24_DSP1_MPU_XMEM_ACCESS_3_REG                            (0x2BC3048)
#define XM_UNPACKED24_DSP1_MPU_YMEM_ACCESS_3_REG                            (0x2BC304C)
#define XM_UNPACKED24_DSP1_MPU_WINDOW_ACCESS_3_REG                          (0x2BC3050)
#define XM_UNPACKED24_DSP1_MPU_XREG_ACCESS_3_REG                            (0x2BC3054)
#define XM_UNPACKED24_DSP1_MPU_YREG_ACCESS_3_REG                            (0x2BC305C)
#define XM_UNPACKED24_DSP1_MPU_LOCK_CONFIG_REG                              (0x2BC3140)
/** @} */

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L41_SPEC_H
