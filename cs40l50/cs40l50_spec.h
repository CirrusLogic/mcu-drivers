/**
 * @file cs40l50_spec.h
 *
 * @brief Constants and Types from CS40L50 datasheet
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

#ifndef CS40L50_SPEC_H
#define CS40L50_SPEC_H

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

/***********************************************************************************************************************
 * LITERALS, CONSTANTS, MACROS, ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * @defgroup CS40L50_DATASHEET
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
#define CS40L50_T_BST_PUP_MS                                                (1)
#define CS40L50_T_RLPW_MS                                                   (2)
#define CS40L50_T_IRS_MS                                                    (1)
/** @} */

/**
 * @defgroup SECTION_7_1_SW_RESET
 * @brief Register descriptions from datasheet
 *
 * @{
 */
#define CS40L50_SW_RESET_DEVID_REG                                          (0x0)
#define CS40L50_SW_RESET_REVID_REG                                          (0x4)
#define CS40L50_REVID_B0                                                    (0xB0)
/** @} */


/**
 * @defgroup SECTION_7_2_CTRL_KEYS
 * @brief Register descriptions from datasheet
 *
 * @{
 */
#define CS40L50_CTRL_KEYS_TEST_KEY_CTRL_REG                                 (0x40)

#define CS40L50_TEST_KEY_CTRL_UNLOCK_1                                      (0x00000055)
#define CS40L50_TEST_KEY_CTRL_UNLOCK_2                                      (0x000000AA)
#define CS40L50_TEST_KEY_CTRL_LOCK_1                                        (0x000000CC)
#define CS40L50_TEST_KEY_CTRL_LOCK_2                                        (0x00000033)
/** @} */

#define CS40L50_DSP_VIRTUAL1_MBOX_1                                         (0x11020)
#define CS40L50_CMD_INDEX_ROM_WAVE                                          (0x01800000)
#define CS40L50_CMD_INDEX_RAM_WAVE                                          (0x01000000)

#define CS40L50_F0_ESTIMATION_REDC                                          (0x02802F7C)
#define CS40L50_F0_ESTIMATION_F0_EST                                        (0x02802F84)

#define CS40L50_F0_OTP_STORED                                               (0x02805C00)
#define CS40L50_REDC_OTP_STORED                                             (0x02805C04)
#define CS40L50_VIBEGEN_COMPENSATION_ENABLE                                 (0x02805C30)

#define CS40L50_SVC_RE_EST_STATUS                                           (0x03401110)


#define CS40L50_DSP1_CCM_CORE_CONTROL                                       (0x02BC1000)
#define CS40L50_FIRMWARE_CALL_RAM_INIT                                      (0x028021DC)
#define CS40L50_PWRMGT_CTL                                                  (0x00002900)

/** @} */

#define  IRQ1_STATUS_REG                                                    (0xE004)

/* Broadcast */
#define CS40L50_I2C_BROADCAST                                               (0x00000168)
#define CS40L50_I2C_BROADCAST_EN_MASK                                       (1 << 15)
#define CS40L50_I2C_BROADCAST_ADDR_DEFAULT                                  (0x00000088)

/* State */

#define CS40L50_STATE_HIBERNATE                                             (0)
#define CS40L50_STATE_SHUTDOWN                                              (1)
#define CS40L50_STATE_STANDBY                                               (2)
#define CS40L50_STATE_ACTIVE                                                (3)
#define CS40L50_STATE_WAKE                                                  (4)

#define CS40L50_PWLE_ENTRY_START                                            (0)

/* DSP State */

#define CS40L50_DSP_STATE_HIBERNATE                                         (0)
#define CS40L50_DSP_STATE_SHUTDOWN                                          (1)
#define CS40L50_DSP_STATE_STANDBY                                           (2)
#define CS40L50_DSP_STATE_ACTIVE                                            (3)
#define CS40L50_DSP_STATE_UNKNOWN                                           (4)

/* PM State */
#define CS40L50_PM_STATE_HIBERNATE                                          (0)
#define CS40L50_PM_STATE_WAKEUP                                             (1)
#define CS40L50_PM_STATE_PREVENT_HIBERNATE                                  (2)
#define CS40L50_PM_STATE_ALLOW_HIBERNATE                                    (3)
#define CS40L50_PM_STATE_SHUTDOWN                                           (4)

#define CS40L50_PM_TIMEOUT_MS_MAX                                           (10000)
#define CS40L50_PM_TICKS_MS_DIV                                             (32)
#define CS40L50_PM_TIMEOUT_TICKS_UPPER_SHIFT                                (24)
#define CS40L50_PM_TIMEOUT_TICKS_UPPER_MASK                                 (0x00FFFFFF)
#define CS40L50_PM_TIMEOUT_TICKS_LOWER_MASK                                 (0xFFFFFFFF)
#define CS40L50_PM_TIMER_TIMEOUT_TICKS_3_L                                  (0x02804300)

#define CS40L50_DSP_STATE_MASK                                              (0xFF)

/*Dynamic F0*/
#define CS40L50_DYNAMIC_F0_ENABLED                                          (0x02802F8C)
#define CS40L50_DYNAMIC_F0_THRESHOLD                                        (0x02802F90)
#define CS40L50_DYNAMIC_F0_TABLE                                            (0x02802FA0)

/* DSP mailbox controls */
#define CS40L50_DSP_MBOX_RESET                                              (0x0)
#define CS40L50_DSP_MBOX_CMD_HIBER                                          (0x02000001)
#define CS40L50_DSP_MBOX_CMD_WAKEUP                                         (0x02000002)
#define CS40L50_DSP_MBOX_CMD_PREVENT_HIBER                                  (0x02000003)
#define CS40L50_DSP_MBOX_CMD_ALLOW_HIBER                                    (0x02000004)
#define CS40L50_DSP_MBOX_CMD_SHUTDOWN                                       (0x02000005)
#define CS40L50_DSP_MBOX_PM_CMD_BASE                                        CS40L50_DSP_MBOX_CMD_HIBER

#define CS40L50_DSP_BYTES_PER_WORD                                          (4)
#define CS40L50_MAILBOX_QUEUE_BASE                                          (0x028042C0)
#define CS40L50_MAILBOX_QUEUE_LEN_OFFSET                                    (4)
#define CS40L50_MAILBOX_QUEUE_WT_OFFSET                                     (8)
#define CS40L50_MAILBOX_QUEUE_RD_OFFSET                                     (12)

#define CS40L50_DSP_MBOX_F0_EST                                             (0x7000001)
#define CS40L50_DSP_MBOX_REDC_EST                                           (0x7000002)
#define CS40L50_DSP_MBOX_REDC_EST_START                                     (0x7000012)
#define CS40L50_DSP_MBOX_REDC_EST_DONE                                      (0x7000022)
#define CS40L50_DSP_MBOX_F0_EST_START                                       (0x7000011)
#define CS40L50_DSP_MBOX_F0_EST_DONE                                        (0x7000021)
/* OWT/RTH */
#define  CS40L50_VIBEGEN_OWT_BASE_XM                                        (0x02805C34)
#define  CS40L50_OWT_WAVE_XM_TABLE                                          (0x02805C48)
#define  CS40L50_OWT_PUSH                                                   (0x03000008)
#define  CS40L50_TRIGGER_RTH                                                (0x01400000)
#define  CS40L50_MAX_PWLE_SECTIONS                                          (126)
#define  CS40L50_SLOT0_MAX_PWLE_SECTIONS                                    (61)
#define  CS40l50_SLOT1_MAX_PWLE_SECTIONS                                    (65)
/*GPIO trigger configuration*/
#define CS40L50_GPIO_HANDLERS_BASE                                          (0x02804140)
#define CS40L50_GPIO_HANDLERS_ENTRY_LENGTH_BYTES                            (4)

#define CS40L50_IRQ1_IRQ1_STATUS                                            (0x0000E004)

#define CS40L50_IRQ1_INT_1                                                  (0x0000E010)
#define IRQ1_INT_1_AMP_SHORT_ERR_INT1_BITMASK                               (1 << 31)

#define CS40L50_IRQ1_INT_2                                                  (0x0000E014)

#define CS40L50_IRQ1_INT_8                                                  (0x0000E02C)
#define IRQ1_INT_8_TEMP_ERR_INT1_BITMASK                                    (1 << 31)

#define CS40L50_IRQ1_INT_9                                                  (0x0000E030)
#define IRQ1_INT_9_BST_ILIMIT_ERR_INT1_BITMASK                              (1 << 8)
#define IRQ1_INT_9_BST_SHORT_ERR_INT1_BITMASK                               (1 << 7)
#define IRQ1_INT_9_BST_UVP_ERR_INT1_BITMASK                                 (1 << 6)

#define CS40L50_IRQ1_INT_10                                                 (0x0000E034)
#define IRQ1_INT_10_UVLO_VDDBATT_ERR_INT1_BITMASK                           (1 << 16)

#define CS40L50_IRQ1_IRQ1_MASK_1                                            (0x0000E090)

#define CS40L50_MSM_ERROR_RELEASE                                           (0x00002034)
#define CS40L50_MSM_ERROR_RELEASE_GLOBAL_ERR_RELEASE_BITMASK                (1 << 11)

#define CS40L50_IRQ1_MASK_2                                                 (0x0000E094)
#define CS40L50_IRQ1_MASK_2_DSP_VIRTUAL2_MBOX_WR_MASK1                      (1 << 21)

#define CS40L50_GPIO_STATUS1                                                (0x0000F000)
#define CS40L50_GPIO1_STS_BITMASK                                           (1)
#define CS40L50_GPIO_CTRL1                                                  (0x0000F008)
#define CS40L50_GPIO_CTRL1_DIR_BITMASK                                      (0x80000000)
#define CS40L50_GPIO_CTRL1_FN_BITMASK                                       (0x00000007)
#define CS40L50_GPIO_CTRL1_FN_INPUT_OUTPUT                                  (0x00000001)

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L50_SPEC_H
