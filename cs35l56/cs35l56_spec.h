/**
 * @file cs35l56_spec.h
 *
 * @brief Constants and Types from CS35L56 datasheet
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

#ifndef CS35L56_SPEC_H
#define CS35L56_SPEC_H

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
struct cs35l56_register_encoding {
    uint32_t value; ///< Real-world value needing to be encoded
    uint8_t code;   ///< Code corresponding to value
};

/***********************************************************************************************************************
 * LITERALS, CONSTANTS, MACROS, ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * @defgroup CS35L56_DATASHEET
 * @brief All stuff from the datasheet
 *
 * @{
 */

/**
 * @defgroup SECTION_3_CHAR_AND_SPEC
 * @brief Characteristics and Specifications from datasheet
 *
 * @{
 */
#define CS35L56_T_BST_PUP_MS (1)
#define CS35L56_T_RLPW_MS    (2)
#define CS35L56_T_IRS_MS     (1)
/** @} */

/**
 * @defgroup SECTION_7_1_SW_RESET
 * @brief Register descriptions from datasheet
 *
 * @{
 */
#define CS35L56_SW_RESET_DEVID_REG (0x0)
#define CS35L56_SW_RESET_REVID_REG (0x4)
#define CS35L56_REVID_B2           (0xB2)
/** @} */

/**
 * Encoding for CCM register REFCLK_INPUT field PLL_REFCLK_FREQ
 *
 * Encodes from raw PLL input reference clock frequency in Hertz to bitfield code.
 *
 * @see REFCLK_INPUT
 * @see Section 7.11.1
 *
 */
#define CS35L56_NUM_VALD_PLL_REFCLKS 34
extern const struct cs35l56_register_encoding cs35l56_pll_refclk[CS35L56_NUM_VALD_PLL_REFCLKS];

/**
 * @defgroup SECTION_7_2_CTRL_KEYS
 * @brief Register descriptions from datasheet
 *
 * @{
 */
#define CS35L56_CTRL_KEYS_TEST_KEY_CTRL_REG (0x40)

#define CS35L56_TEST_KEY_CTRL_UNLOCK_1 (0x00000055)
#define CS35L56_TEST_KEY_CTRL_UNLOCK_2 (0x000000AA)
#define CS35L56_TEST_KEY_CTRL_LOCK_1   (0x000000CC)
#define CS35L56_TEST_KEY_CTRL_LOCK_2   (0x00000033)
/** @} */

#define CS35L56_DSP_VIRTUAL1_MBOX_1 (0x11020)
#define CS35L56_CMD_INDEX_ROM_WAVE  (0x01800000)
#define CS35L56_CMD_INDEX_RAM_WAVE  (0x01000000)

#define CS35L56_F0_ESTIMATION_REDC   (0x02802F7C)
#define CS35L56_F0_ESTIMATION_F0_EST (0x02802F84)

#define CS35L56_F0_OTP_STORED               (0x02805C00)
#define CS35L56_REDC_OTP_STORED             (0x02805C04)
#define CS35L56_VIBEGEN_COMPENSATION_ENABLE (0x02805C30)

#define CS35L56_SVC_RE_EST_STATUS (0x03401110)

#define CS35L56_DSP1_CCM_CORE_CONTROL  (0x02BC1000)
#define CS35L56_FIRMWARE_CALL_RAM_INIT (0x028021DC)
#define CS35L56_HALO_STATE             (0x028021E0)
#define CS35L56_PWRMGT_CTL             (0x00002900)

#define CS35L56_DATA_BYTES    (4)
#define CS35L56_ADDR_BYTES    (4)
#define CS35L56_I2C_MSG_BYTES CS35L56_DATA_BYTES + CS35L56_ADDR_BYTES

/** @} */

/* DSP CCM core control */
#define DSP1_CCM_CORE_EN_MASK    (1)
#define DSP1_CCM_PM_REMAP_MASK   (1 << 7)
#define DSP1_CCM_CORE_RESET_MASK (1 << 9)

#define IRQ1_STATUS_REG (0xE004)

/* Broadcast */
#define CS35L56_I2C_BROADCAST              (0x00000168)
#define CS35L56_I2C_BROADCAST_EN_MASK      (1 << 15)
#define CS35L56_I2C_BROADCAST_ADDR_DEFAULT (0x00000088)

/* State */
#define CS35L56_STATE_HIBERNATE (0)
#define CS35L56_STATE_SHUTDOWN  (1)
#define CS35L56_STATE_STANDBY   (2)
#define CS35L56_STATE_ACTIVE    (3)
#define CS35L56_STATE_WAKE      (4)

/* DSP State */
#define CS35L56_DSP_STATE_HIBERNATE (0)
#define CS35L56_DSP_STATE_SHUTDOWN  (1)
#define CS35L56_DSP_STATE_STANDBY   (2)
#define CS35L56_DSP_STATE_ACTIVE    (3)
#define CS35L56_DSP_STATE_UNKNOWN   (4)

/* PM State */
#define CS35L56_PM_STATE_SHUTDOWN (1)
#define CS35L56_PM_STATE_STANDBY  (2)
#define CS35L56_PM_STATE_ACTIVE   (3)
#define CS35L56_PM_STATE          (0x02804308)

#define CS35L56_PM_TIMEOUT_MS_MAX            (10000)
#define CS35L56_PM_TICKS_MS_DIV              (32)
#define CS35L56_PM_TIMEOUT_TICKS_UPPER_SHIFT (24)
#define CS35L56_PM_TIMEOUT_TICKS_UPPER_MASK  (0x00FFFFFF)
#define CS35L56_PM_TIMEOUT_TICKS_LOWER_MASK  (0xFFFFFFFF)
#define CS35L56_PM_TIMER_TIMEOUT_TICKS_3_L   (0x02804300)

#define CS35L56_DSP_STATE_MASK (0xFF)

/*Dynamic F0*/
#define CS35L56_DYNAMIC_F0_ENABLED   (0x02802F8C)
#define CS35L56_DYNAMIC_F0_THRESHOLD (0x02802F90)
#define CS35L56_DYNAMIC_F0_TABLE     (0x02802FA0)

/* DSP mailbox controls */
#define CS35L56_DSP_MBOX_RESET             (0x0)
#define CS35L56_DSP_MBOX_CMD_HIBER         (0x02000001)
#define CS35L56_DSP_MBOX_CMD_WAKEUP        (0x02000002)
#define CS35L56_DSP_MBOX_CMD_PREVENT_HIBER (0x02000003)
#define CS35L56_DSP_MBOX_CMD_ALLOW_HIBER   (0x02000004)
#define CS35L56_DSP_MBOX_CMD_SHUTDOWN      (0x02000005)
#define CS35L56_DSP_MBOX_CMD_SYSTEM_RESET  (0x02000007)
#define CS35L56_DSP_MBOX_PM_CMD_BASE       CS35L56_DSP_MBOX_CMD_HIBER

#define CS35L56_DSP_BYTES_PER_WORD       (4)
#define CS35L56_MAILBOX_QUEUE_BASE       (0x028042C0)
#define CS35L56_MAILBOX_QUEUE_LEN_OFFSET (4)
#define CS35L56_MAILBOX_QUEUE_WT_OFFSET  (8)
#define CS35L56_MAILBOX_QUEUE_RD_OFFSET  (12)

#define CS35L56_MBOX_RD_MASK (0x1F)
#define CS35L56_MBOX_RD_SIZE (0x1C)

#define CS35L56_DSP_MBOX_CMD_PLAY  (0x0B000001)
#define CS35L56_DSP_MBOX_CMD_PAUSE (0x0B000002)

/*GPIO trigger configuration*/
#define CS35L56_GPIO_HANDLERS_BASE               (0x02804140)
#define CS35L56_GPIO_HANDLERS_ENTRY_LENGTH_BYTES (4)

#define CS35L56_IRQ1_IRQ1_STATUS (0x0000E004)

#define CS35L56_IRQ1_INT_1                    (0x0000E010)
#define IRQ1_INT_1_AMP_SHORT_ERR_INT1_BITMASK (1 << 31)

#define CS35L56_IRQ1_INT_2 (0x0000E014)

#define CS35L56_IRQ1_INT_4                    (0x0000E01C)
#define IRQ1_INT_4_OTP_BOOT_DONE_INT1_BITMASK (1 << 1)

#define CS35L56_IRQ1_INT_8               (0x0000E02C)
#define IRQ1_INT_8_TEMP_ERR_INT1_BITMASK (1 << 31)

#define CS35L56_IRQ1_INT_9                     (0x0000E030)
#define IRQ1_INT_9_BST_ILIMIT_ERR_INT1_BITMASK (1 << 8)
#define IRQ1_INT_9_BST_SHORT_ERR_INT1_BITMASK  (1 << 7)
#define IRQ1_INT_9_BST_UVP_ERR_INT1_BITMASK    (1 << 6)

#define CS35L56_IRQ1_INT_10                       (0x0000E034)
#define IRQ1_INT_10_UVLO_VDDBATT_ERR_INT1_BITMASK (1 << 16)

#define CS35L56_IRQ1_IRQ1_MASK_1 (0x0000E090)

#define CS35L56_MSM_ERROR_RELEASE                            (0x00002034)
#define CS35L56_MSM_ERROR_RELEASE_GLOBAL_ERR_RELEASE_BITMASK (1 << 11)

#define CS35L56_IRQ1_MASK_2                            (0x0000E094)
#define CS35L56_IRQ1_MASK_2_DSP_VIRTUAL2_MBOX_WR_MASK1 (1 << 21)

#define CS35L56_IRQ1_MASK_4                     (0x0000E09C)
#define CS35L56_IRQ1_MASK_4_OTP_BOOT_DONE_MASK1 (1 << 1)

#define CS35L56_GPIO1_RISE_EINT_BITMASK  (1)
#define CS35L56_GPIO1_FALL_EINT_BITMASK  (1 << 1)
#define CS35L56_GPIO2_RISE_EINT_BITMASK  (1 << 2)
#define CS35L56_GPIO2_FALL_EINT_BITMASK  (1 << 3)
#define CS35L56_GPIO3_RISE_EINT_BITMASK  (1 << 4)
#define CS35L56_GPIO3_FALL_EINT_BITMASK  (1 << 5)
#define CS35L56_GPIO4_RISE_EINT_BITMASK  (1 << 6)
#define CS35L56_GPIO4_FALL_EINT_BITMASK  (1 << 7)
#define CS35L56_GPIO5_RISE_EINT_BITMASK  (1 << 8)
#define CS35L56_GPIO5_FALL_EINT_BITMASK  (1 << 9)
#define CS35L56_GPIO6_RISE_EINT_BITMASK  (1 << 10)
#define CS35L56_GPIO6_FALL_EINT_BITMASK  (1 << 11)
#define CS35L56_GPIO7_RISE_EINT_BITMASK  (1 << 12)
#define CS35L56_GPIO7_FALL_EINT_BITMASK  (1 << 13)
#define CS35L56_GPIO8_RISE_EINT_BITMASK  (1 << 14)
#define CS35L56_GPIO8_FALL_EINT_BITMASK  (1 << 15)
#define CS35L56_GPIO9_RISE_EINT_BITMASK  (1 << 16)
#define CS35L56_GPIO9_FALL_EINT_BITMASK  (1 << 17)
#define CS35L56_GPIO10_RISE_EINT_BITMASK (1 << 18)
#define CS35L56_GPIO10_FALL_EINT_BITMASK (1 << 19)
#define CS35L56_GPIO11_RISE_EINT_BITMASK (1 << 20)
#define CS35L56_GPIO11_FALL_EINT_BITMASK (1 << 21)
#define CS35L56_GPIO12_RISE_EINT_BITMASK (1 << 22)
#define CS35L56_GPIO12_FALL_EINT_BITMASK (1 << 23)
#define CS35L56_GPIO13_RISE_EINT_BITMASK (1 << 24)
#define CS35L56_GPIO13_FALL_EINT_BITMASK (1 << 25)

#define CS35L56_GPIO_STATUS1       (0x0000F000)
#define CS35L56_GPIO1_STS_BITMASK  (1)
#define CS35L56_GPIO2_STS_BITMASK  (1 << 1)
#define CS35L56_GPIO3_STS_BITMASK  (1 << 2)
#define CS35L56_GPIO4_STS_BITMASK  (1 << 3)
#define CS35L56_GPIO5_STS_BITMASK  (1 << 4)
#define CS35L56_GPIO6_STS_BITMASK  (1 << 5)
#define CS35L56_GPIO7_STS_BITMASK  (1 << 6)
#define CS35L56_GPIO8_STS_BITMASK  (1 << 7)
#define CS35L56_GPIO9_STS_BITMASK  (1 << 8)
#define CS35L56_GPIO10_STS_BITMASK (1 << 9)
#define CS35L56_GPIO11_STS_BITMASK (1 << 10)
#define CS35L56_GPIO12_STS_BITMASK (1 << 11)
#define CS35L56_GPIO13_STS_BITMASK (1 << 12)

#define CS35L56_GPIO_CTRL1  (0x0000F008)
#define CS35L56_GPIO_CTRL2  (0x0000F00C)
#define CS35L56_GPIO_CTRL3  (0x0000F010)
#define CS35L56_GPIO_CTRL4  (0x0000F014)
#define CS35L56_GPIO_CTRL5  (0x0000F018)
#define CS35L56_GPIO_CTRL6  (0x0000F01C)
#define CS35L56_GPIO_CTRL7  (0x0000F020)
#define CS35L56_GPIO_CTRL8  (0x0000F024)
#define CS35L56_GPIO_CTRL9  (0x0000F028)
#define CS35L56_GPIO_CTRL10 (0x0000F02C)
#define CS35L56_GPIO_CTRL11 (0x0000F030)
#define CS35L56_GPIO_CTRL12 (0x0000F034)
#define CS35L56_GPIO_CTRL13 (0x0000F038)

#define CS35L56_GPIO_CTRL_DIR_BITMASK     (0x80000000)
#define CS35L56_GPIO_CTRL_FN_BITMASK      (0x00000007)
#define CS35L56_GPIO_CTRL_FN_INPUT_OUTPUT (0x00000001)
#define CS35L56_GPIO_CTRL_FN_IRQ1         (0x00000003)

/*ASP Configuration*/
#define CS35L56_BLOCK_ENABLES2                (0x0000201C)
#define CS35L56_BLOCK_ENABLES2_ASP_EN_MASK    (1 << 27)
#define CS35L56_BLOCK_ENABLES2_OTW_EN_MASK    (1 << 26)
#define CS35L56_BLOCK_ENABLES2_CLASSH_EN_MASK (1 << 4)

#define CS35L56_ASP1_ENABLES          (0x00004800)
#define CS35L56_ASP1_ENABLES_RX_SHIFT (16)
#define CS35L56_ASP1_ENABLES_TX_SHIFT (0)

#define CS35L56_ASP1_CTRL_1                    (0x00004804)
#define CS35L56_ASP1_CTRL_2                    (0x00004808)
#define CS35L56_ASP1_CTRL_2_ASP1_FMT_BCLK_MASK (0x2 << 8)
#define CS35L56_ASP1_CTRL_RX_WIDTH_OFFSET      (24)
#define CS35L56_ASP1_CTRL_TX_WIDTH_OFFSET      (16)

#define CS35L56_ASP1_DATA_CONTROL5 (0x00004840)

#define CS35L56_FIRMWARE_PLL_REFCLK_FREQ        (0x00002C04)
#define CS35L56_FIRMWARE_PLL_REFCLK_FREQ_OFFSET (5)

#define CS35L56_FIRMWARE_PLL_OPEN_LOOP_MASK (1 << 11)

#define CS35L56_DACPCM1_INPUT          (0x00004C00)
#define CS35L56_DACPCM1_INPUT_DSP1_CH5 (0x36)

#define CS35L56_ASP1TX1_INPUT (0x00004C20)
#define CS35L56_ASP1TX2_INPUT (0x00004C24)
#define CS35L56_ASP1TX3_INPUT (0x00004C28)
#define CS35L56_ASP1TX4_INPUT (0x00004C2C)

#define CS35L56_FIRMWARE_PLL_REFCLK_EN_MASK (1 << 4)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L56_SPEC_H
