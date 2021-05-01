/**
 * @file cs40l30_spec.h
 *
 * @brief Constants and Types from CS40L30 datasheet
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

#ifndef CS40L30_SPEC_H
#define CS40L30_SPEC_H

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
struct cs40l30_register_encoding
{
    uint32_t value; ///< Real-world value needing to be encoded
    uint8_t code;   ///< Code corresponding to value
};

/***********************************************************************************************************************
 * LITERALS, CONSTANTS, MACROS, ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * @defgroup CS40L30_DATASHEET
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
#define CS40L30_T_BST_PUP_MS                                                (1)             ///< @see Table 3-5
#define CS40L30_T_RLPW_MS                                                   (1)             ///< @see Table 3-16
#define CS40L30_T_IRS_MS                                                    (3)             ///< @see Table 3-16
/** @} */

/**
 * @defgroup SECTION_4_FUNCTIONAL_DESCRIPTION
 * @brief Functional Description from datasheet
 *
 * @see Datasheet Section 4
 *
 * @{
 */

/** @} */

/**
 * @defgroup SECTION_7_1_SW_RESET
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.1
 *
 * @{
 */
#define CS40L30_SW_RESET_DEVID_REG                                          (0x0)           ///< @see Section 7.1.1
#define CS40L30_SW_RESET_REVID_REG                                          (0x4)           ///< @see Section 7.1.2
/** @} */

/**
 * @defgroup SECTION_7_7_IRQ1
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.7
 *
 * @{
 */
#define CS40L30_IRQ1_STATUS_REG                                             (0x1004)        ///< @see Section 7.7.1

#define CS40L30_IRQ1_STATUS_IRQ1_STS1_BITMASK                               (0x1)           ///< @see Section 7.7.1

#define CS40L30_IRQ1_INT_2_REG                                              (0x1014)        ///< @see Section 7.7.3

#define CS40L30_IRQ1_INT_2_FIRST_WAKE_LVL_INT1_BITMASK                      (1 << 30)       ///< @see Section 7.7.3

#define CS40L30_IRQ1_INT_4_REG                                              (0x101C)        ///< @see Section 7.7.5

/**
 * Register definition for CS40L30_IRQ1_INT_4_REG
 *
 * @see CS40L30_IRQ1_INT_4_REG
 * @see Sections 7.7.5
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t bst_ovp_warn_rise      : 1;
        uint32_t bst_ovp_warn_fall      : 1;
        uint32_t bst_ovp_warn_lvl       : 1;
        uint32_t bst_ovp_err            : 1;
        uint32_t bst_dcm_uvp_err        : 1;
        uint32_t bst_short_err          : 1;
        uint32_t bst_ipk                : 1;
        uint32_t amp_short_err          : 1;
        uint32_t temp_warn_rise         : 1;
        uint32_t temp_warn_fall         : 1;
        uint32_t temp_warn_lvl          : 1;
        uint32_t temp_err               : 1;
        uint32_t vpbr_thresh            : 1;
        uint32_t vpbr_att_clr           : 1;
        uint32_t vbbr_thresh            : 1;
        uint32_t vbbr_att_clr           : 1;
    };
} cs40l30_irq1_int_4_t;

#define CS40L30_IRQ1_INT_9_REG                                              (0x1030)        ///< @see Section 7.7.10
#define CS40L30_IRQ1_INT_10_REG                                             (0x1034)        ///< @see Section 7.7.11

#define CS40L30_IRQ1_MASK_4_REG                                             (0x111C)        ///< @see Section 7.7.17

/**
 * Register definition for CS40L30_IRQ1_MASK_4_REG
 *
 * @see CS40L30_IRQ1_MASK_4_REG
 * @see Sections 7.7.17
 * @see cs40l30_irq1_mask_4_t
 *
 */
typedef cs40l30_irq1_int_4_t cs40l30_irq1_mask_4_t;

#define CS40L30_IRQ1_MASK_9_REG                                             (0x1130)        ///< @see Section 7.7.22
#define CS40L30_IRQ1_MASK_10_REG                                            (0x1134)        ///< @see Section 7.7.23
/** @} */

/**
 * @defgroup SECTION_7_8_MSM
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.8
 *
 * @{
 */
#define CS40L30_MSM_BLOCK_ENABLES_REG                                       (0x1418)        ///< @see Section 7.8.2

/**
 * Register definition for CS40L30_MSM_BLOCK_ENABLES_REG
 *
 * @see CS40L30_MSM_BLOCK_ENABLES_REG
 * @see Sections 7.8.2
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
} cs40l30_msm_block_enables_t;

#define CS40L30_MSM_ERROR_RELEASE_REG                                       (0x1434)        ///< @see Section 7.8.6
/** @} */

/**
 * @defgroup SECTION_7_13_AMP_PCM
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.13
 *
 * @{
 */
#define CS40L30_AMP_PCM_CONTROL_REG                                         (0x5000)        ///< @see Section 7.13.1
#define CS40L30_AMP_PCM_CONTROL_DEFAULT                                     (0x0)           ///< @see Section 7.13.1

#define CS40L30_AMP_PCM_CONTROL_AMP_VOL_PCM_BITOFFSET                       (0)            ///< @see Section 7.13.1
#define CS40L30_AMP_PCM_CONTROL_AMP_VOL_PCM_BITWIDTH                        (10)           ///< @see Section 7.13.1
#define CS40L30_AMP_VOLUME_MUTE                                             (0x400)         ///< @see Section 7.13.1
#define CS40L30_AMP_VOLUME_0DB                                              (0)             ///< @see Section 7.13.1
#define CS40L30_AMP_VOLUME_N6DB                                             (0x7CF)         ///< @see Section 7.13.1

/**
 * Register definition for CS40L30_AMP_PCM_CONTROL_REG
 *
 * @see CS40L30_AMP_PCM_CONTROL_REG
 * @see Sections 7.13.1
 *
 */
typedef union
{
    uint32_t word;

    struct
    {
        uint32_t amp_vol_pcm                : 11;
        uint32_t reserved_0                 : 1;
        uint32_t amp_ramp_pcm               : 3;
        uint32_t reserved_1                 : 1;
        uint32_t amp_inv_pcm                : 1;
        uint32_t reserved_2                 : 3;
        uint32_t amp_hpf_pcm_en             : 1;
        uint32_t reserved_3                 : 11;
    };
} cs40l30_amp_pcm_control_t;

/** @} */

/**
 * @defgroup SECTION_7_20_ALWAYS_ON
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.20
 *
 * @{
 */
#define CS40L30_ALWAYS_ON_AO_CTRL_REG                                       (0x9000)        ///< @see Section 7.20.1
#define CS40L30_AO_CTRL_FIRST_WAKE_CLR_BITMASK                              (1 << 9)        ///< @see Section 7.20.1

#define CS40L30_ALWAYS_ON_MEM_RET_REG                                       (0x903C)           ///< @see Section 7.20.9
#define CS40L30_ALWAYS_ON_MEM_RET_BITMASK                                   (0x1)           ///< @see Section 7.20.9

/** @} */

/**
 * @defgroup SECTION_7_26_DSP_VIRTUAL1_MBOX
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.26
 *
 * @{
 */
#define CS40L30_DSP_VIRTUAL1_MBOX_1_REG                                    (0x17420)      ///< @see Section 7.26.1
/** @} */

/**
 * @defgroup SECTION_7_29_OTP_IF
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.29
 *
 * @{
 */
#define CS40L30_OTP_MEM0_REG                                                (0x30000)           ///< @see Section 7.29.1

#define CS40L30_TRIM_SIZE                                                   (156)               ///< @see Section 4.2.4

#define CS40L30_OTP_CONFIG_START_REG                                        (CS40L30_OTP_MEM0_REG + CS40L30_TRIM_SIZE)           ///< @see Section 4.2.4
#define CS40L30_OTP_CONFIG_BLOCK_SIZE_WORDS                                 4
#define CS40L30_CONFIG_SIZE                                                 (96)               ///< @see Section 4.2.4

#define CS40L30_OTP_CALIB_START_REG                                         (CS40L30_OTP_CONFIG_START_REG + CS40L30_CONFIG_SIZE)           ///< @see Section 4.2.4
#define CS40L30_OTP_CALIB_MAX_SLOTS                                         8

#define CS40L30_CONFIG_SHADOW_OTP_START_REG                                 (0x02800740)
#define CS40L30_CONFIG_SHADOW_OTP_SIZE_WORDS                                (48)

#define CS40L30_CALIB_SHADOW_OTP_START_REG                                  (0x02800808)
#define CS40L30_CALIB_SHADOW_OTP_SIZE_WORDS                                 (66)

#define CS40L30_SKIP_CINIT_REG                                              (0x02800804)
#define CS40L30_SKIP_CINIT                                                  (0x1)

#define CS40L30_BOOT_RAM_OTP_SHADOW_ENABLED                                 (0x2)
#define CS40L30_BOOT_ROM_OTP_SHADOW_ENABLED                                 (0x3)
/** @} */

/**
 * Definitions of dsp register types
 */
typedef union
{
    uint32_t word;

    struct {
        uint32_t halo_word    : 24;
        uint32_t reserved     : 8;
    };
} dsp_reg_t;

/** @} */

/**
 * @defgroup SECTION_7_38_XM_UNPACKED_24
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.38
 *
 * @{
 */
#define CS40L30_DSP1_POWERONSEQUENCE_REG                                    (0x2801330)     ///< @see Section 7.38

#define CS40L30_DSP1_CCM_CORE_CONTROL_REG                                   (0x2bc1000)           ///< @see Section 7.38.57
#define CS40L30_DSP1_CCM_CORE_CONTROL_EN_BITMASK                            (0x1)           ///< @see Section 7.38.57
#define CS40L30_DSP1_CCM_CORE_CONTROL_PM_REMAP_BITMASK                      (0x100)         ///< @see Section 7.38.57
#define CS40L30_DSP1_CCM_CORE_CONTROL_RESET_BITMASK                         (0x200)         ///< @see Section 7.38.57

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L30_SPEC_H
