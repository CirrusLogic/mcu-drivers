/**
 * @file cs40l50_spec.h
 *
 * @brief Constants and Types from CS40L50 datasheet
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022 All Rights Reserved, http://www.cirrus.com/
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
 * @see Datasheet Section 3
 *
 * @{
 */
#define CS40L50_T_BST_PUP_MS                                                (1)             ///< @see Table 3-5
#define CS40L50_T_RLPW_MS                                                   (2)             ///< @see Table 3-14
#define CS40L50_T_IRS_MS                                                    (1)             ///< @see Table 3-14
/** @} */

/**
 * @defgroup SECTION_7_1_SW_RESET
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.1
 *
 * @{
 */
#define CS40L50_SW_RESET_DEVID_REG                                          (0x0)           ///< @see Section 7.1.1
#define CS40L50_SW_RESET_REVID_REG                                          (0x4)           ///< @see Section 7.1.2
/** @} */

/**
 * @defgroup SECTION_7_2_CTRL_KEYS
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.2
 *
 * @{
 */
#define CS40L50_CTRL_KEYS_TEST_KEY_CTRL_REG                                 (0x40)          ///< @see Section 7.2.1

#define CS40L50_TEST_KEY_CTRL_UNLOCK_1                                      (0x00000055)    ///< @see Section 7.2.1
#define CS40L50_TEST_KEY_CTRL_UNLOCK_2                                      (0x000000AA)    ///< @see Section 7.2.1
#define CS40L50_TEST_KEY_CTRL_LOCK_1                                        (0x000000CC)    ///< @see Section 7.2.1
#define CS40L50_TEST_KEY_CTRL_LOCK_2                                        (0x00000033)    ///< @see Section 7.2.1
/** @} */

#define FIRMWARE_CS40L50_HALO_STATE                                         (0x02801E58)
#define CS40L50_HALO_HEARTBEAT                                              (0x02809E5C)
#define CS40L50_DSP_VIRTUAL1_MBOX_1                                         (0x11020)
#define CS40L50_REDC_ESTIMATION_REG                                         (0x0280284C)
#define CS40L50_F0_OTP_STORED                                               (0x02805C00)
#define CS40L50_REDC_OTP_STORED                                             (0x02805C04)
#define CS40L50_RE_EST_STATUS_REG                                           (0x03401B40)
#define CS40L50_F0_ESTIMATION_REG                                           (0x02802854)
#define CS40L50_VIBEGEN_COMPENSATION_ENABLE_REG                             (0x02805C30)
#define CS40L50_REVID_A1                                                    (0xA1)

#define CS40L50_CMD_INDEX_ROM_WAVE                                          (0x01800000)
#define CS40L50_CMD_INDEX_RAM_WAVE                                          (0x01000000)
/** @} */

#define  IRQ1_IRQ1_STATUS_REG                                               (0xE004)

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
#define CS40L50_PM_TIMER_TIMEOUT_TICKS_3_L                                  (0x02803FF0)
#define CS40L50_PM_TIMER_TIMEOUT_TICKS_3_H                                  (0x02803FF4)

#define CS40L50_DSP_STATE_MASK                                              (0xFF)

/* DSP mailbox controls */
#define CS40L50_DSP_MBOX_RESET                                              (0x0)
#define CS40L50_DSP_MBOX_CMD_HIBER                                          (0x02000001)
#define CS40L50_DSP_MBOX_CMD_WAKEUP                                         (0x02000002)
#define CS40L50_DSP_MBOX_CMD_PREVENT_HIBER                                  (0x02000003)
#define CS40L50_DSP_MBOX_CMD_ALLOW_HIBER                                    (0x02000004)
#define CS40L50_DSP_MBOX_CMD_SHUTDOWN                                       (0x02000005)
#define CS40L50_DSP_MBOX_PM_CMD_BASE                                        CS40L50_DSP_MBOX_CMD_HIBER

#define CS40L50_DSP_BYTES_PER_WORD                                          (4)
#define CS40L50_DSP_MBOX_QUEUE_BASE                                         (0x02803FBC)
#define CS40L50_DSP_MBOX_QUEUE_LEN                                          (0x02803FC0)
#define CS40L50_DSP_MBOX_QUEUE_WT                                           (0x02803FC4)
#define CS40L50_DSP_MBOX_QUEUE_RD                                           (0x02803FC8)

#define CS40L50_DSP_MBOX_F0_EST                                             (0x7000001)
#define CS40L50_DSP_MBOX_REDC_EST                                           (0x7000002)
#define CS40L50_DSP_MBOX_REDC_EST_START                                     (0x7000012)
#define CS40L50_DSP_MBOX_REDC_EST_DONE                                      (0x7000022)
/* OWT/RTH */
#define  CS40L50_OWT_WAVE_XM_TABLE                                          (0x02805C48)
#define  CS40L50_OWT_SLOT0_OFFSET                                           (CS40L50_OWT_WAVE_XM_TABLE + 0x4)
#define  CS40L50_OWT_SLOT0_LENGTH                                           (CS40L50_OWT_WAVE_XM_TABLE + 0x8)
#define  CS40L50_OWT_SLOT0_DATA                                             (CS40L50_OWT_WAVE_XM_TABLE + 0xC)
#define  CS40L50_OWT_SLOT1_TYPE                                             (0x02805150)
#define  CS40L50_OWT_SLOT1_OFFSET                                           (CS40L50_OWT_SLOT1_TYPE + 0x4)
#define  CS40L50_OWT_SLOT1_LENGTH                                           (CS40L50_OWT_SLOT1_TYPE + 0x8)
#define  CS40L50_OWT_SLOT1_DATA                                             (CS40L50_OWT_SLOT1_TYPE + 0xC)
#define  CS40L50_VIBEGEN_OWT_BASE_XM                                        (0x02805C34)
#define  CS40L50_OWT_PUSH                                                   (0x03000008)
#define  CS40L50_TRIGGER_RTH                                                (0x01400000)
#define  CS40L50_MAX_PWLE_SECTIONS                                          (126)
#define  CS40L50_SLOT0_MAX_PWLE_SECTIONS                                    (61)
#define  CS40l50_SLOT1_MAX_PWLE_SECTIONS                                    (65)

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L50_SPEC_H
