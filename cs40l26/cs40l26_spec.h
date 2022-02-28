/**
 * @file cs40l26_spec.h
 *
 * @brief Constants and Types from CS40L26 datasheet
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2022 All Rights Reserved, http://www.cirrus.com/
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

#ifndef CS40L26_SPEC_H
#define CS40L26_SPEC_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>

#define CS40L26_DEVID           (0x0)
#define CS40L26_REVID           (0x4)
#define CS40L26_TEST_KEY_CTRL        (0x40)

#define CS40L26_PLL_REFCLK_DETECT_0       (0x2C28)

#define CS40L26_PWRMGT_CTL                (0x2900)

#define CS40L26_CALL_RAM_INIT             (0x2800FF8)

#define CS40L26_DSP_VIRTUAL1_MBOX_1       (0x13020)

#define CS40L26_DSP1_CCM_CORE_CONTROL     (0x2BC1000)

/**
 * @defgroup SECTION_7_3_MSM
 * @brief Register descriptions from datasheet
 *
 * @see Datasheet Section 7.3
 *
 * @{
 */

#define CS40L26_DSP_CCM_CORE_RESET    (0x00000281)

#define CS40L26_PLL_REFCLK_DET_EN_MASK    (1 >> 0)

#define CS40L26_DSP_HALO_STATE_RUN    (2)

#define CS40L26_T_RLPW_MS           (2)
#define CS40L26_T_IRS_MS            (1)
#define CS40L26_1_MS                (1)

/* State */

#define CS40L26_STATE_HIBERNATE     (0)
#define CS40L26_STATE_SHUTDOWN    (1)
#define CS40L26_STATE_STANDBY       (2)
#define CS40L26_STATE_ACTIVE      (3)
#define CS40L26_STATE_WAKE        (4)

#define CS40L26_PWLE_ENTRY_START   (0)

/* DSP State */

#define CS40L26_DSP_STATE_HIBERNATE     (0)
#define CS40L26_DSP_STATE_SHUTDOWN    (1)
#define CS40L26_DSP_STATE_STANDBY     (2)
#define CS40L26_DSP_STATE_ACTIVE    (3)
#define CS40L26_DSP_STATE_UNKNOWN    (4)

/* PM State */
#define CS40L26_PM_STATE_HIBERNATE    (0)
#define CS40L26_PM_STATE_WAKEUP    (1)
#define CS40L26_PM_STATE_PREVENT_HIBERNATE (2)
#define CS40L26_PM_STATE_ALLOW_HIBERNATE   (3)
#define CS40L26_PM_STATE_SHUTDOWN    (4)

#define CS40L26_DSP_STATE_MASK        (0xFF)

/* ROM Controls A1 */
#define CS40L26_A1_PM_CUR_STATE_STATIC_REG    (0x02800370)
#define CS40L26_A1_PM_TIMEOUT_TICKS_STATIC_REG    (0x02800350)
#define CS40L26_A1_DSP_HALO_STATE_REG       (0x02800fa8)
#define CS40L26_F0_ESTIMATION_REDC_REG      (0x028064FC)
#define CS40L26_F0_ESTIMATION_F0_REG        (0x02806504)
#define CS40L26_REDC_ESTIMATION_REG         (0x0340010C)


#define CS40L26_PM_STDBY_TIMEOUT_LOWER_OFFSET   (16)
#define CS40L26_PM_STDBY_TIMEOUT_UPPER_OFFSET   (20)
#define CS40L26_PM_TIMEOUT_TICKS_LOWER_MASK   /* GENMASK(23, 0) */ (0xFFFFFF)
#define CS40L26_PM_TIMEOUT_TICKS_UPPER_MASK   /* GENMASK(7, 0) */ (0xFF)
#define CS40L26_PM_TIMEOUT_TICKS_UPPER_SHIFT  (24)
#define CS40L26_PM_TICKS_MS_DIV     (32)

/* DSP mailbox controls */
#define CS40L26_DSP_MBOX_RESET      (0x0)

#define CS40L26_DSP_MBOX_CMD_HIBER          (0x02000001)
#define CS40L26_DSP_MBOX_CMD_WAKEUP         (0x02000002)
#define CS40L26_DSP_MBOX_CMD_PREVENT_HIBER  (0x02000003)
#define CS40L26_DSP_MBOX_CMD_ALLOW_HIBER    (0x02000004)
#define CS40L26_DSP_MBOX_CMD_SHUTDOWN       (0x02000005)
#define CS40L26_DSP_MBOX_PM_CMD_BASE        CS40L26_DSP_MBOX_CMD_HIBER

#define CS40L26_DSP_MBOX_REDC_EST         (0x7000002)
#define CS40L26_DSP_MBOX_F0_EST           (0x7000001)

#define CS40L26_CMD_INDEX_ROM_WAVE    (0x01800000)
#define CS40L26_CMD_INDEX_RAM_WAVE    (0x01000000)

/* OWT/RTH */
#define CS40L26_OWT_SLOT0_TYPE     (0x02804F44)
#define CS40L26_OWT_SLOT0_OFFSET   (CS40L26_OWT_SLOT0_TYPE + 0x4)
#define CS40L26_OWT_SLOT0_LENGTH   (CS40L26_OWT_SLOT0_TYPE + 0x8)
#define CS40L26_OWT_SLOT0_DATA     (CS40L26_OWT_SLOT0_TYPE + 0xC)
#define CS40L26_OWT_SLOT1_TYPE     (0x02805150)
#define CS40L26_OWT_SLOT1_OFFSET   (CS40L26_OWT_SLOT1_TYPE + 0x4)
#define CS40L26_OWT_SLOT1_LENGTH   (CS40L26_OWT_SLOT1_TYPE + 0x8)
#define CS40L26_OWT_SLOT1_DATA     (CS40L26_OWT_SLOT1_TYPE + 0xC)
#define CS40L26_VIBEGEN_OWT_XM     (0x028041dc)
#define CS40L26_OWT_PUSH           (0x03000008)
#define CS40L26_TRIGGER_RTH        (0x01400000)
#define CS40L26_MAX_PWLE_SECTIONS  (126)
#define CS40L26_SLOT0_MAX_PWLE_SECTIONS  (61)
#define CS40l26_SLOT1_MAX_PWLE_SECTIONS  (65)


/* Dynamic F0 */

#define CS40L26_DYNAMIC_F0_ENABLED              (0x2800F48)
#define CS40L26_DYNAMIC_F0_IMONRINGPPTHRESHOLD  (0x2800F4C)
#define CS40L26_DYNAMIC_F0_FRME_SKIP            (0x2800F50)
#define CS40L26_DYNAMIC_F0_NUM_PEAKS_TOFIND     (0x2800F54)
#define CS40L26_DYNAMIC_F0_TABLE                (0x2800F58)

#define IRQ1_IRQ1_STATUS_REG            (0x00010004)
#define IRQ1_IRQ1_EINT_1_REG            (0x00010010)
#define IRQ1_IRQ1_EINT_1_WKSRC_STATUS_ANY_EINT1_BITMASK (1 << 8)
#define IRQ1_IRQ1_EINT_1_WKSRC_STATUS1_EINT1_BITMASK    (1 << 9)
#define IRQ1_IRQ1_EINT_1_WKSRC_STATUS2_EINT1_BITMASK    (1 << 10)
#define IRQ1_IRQ1_EINT_1_WKSRC_STATUS3_EINT1_BITMASK    (1 << 11)
#define IRQ1_IRQ1_EINT_1_WKSRC_STATUS4_EINT1_BITMASK    (1 << 12)
#define IRQ1_IRQ1_EINT_1_WKSRC_STATUS5_EINT1_BITMASK    (1 << 13)
#define IRQ1_IRQ1_EINT_1_WKSRC_STATUS6_EINT1_BITMASK    (1 << 14)
#define IRQ1_IRQ1_MASK_1_REG            (0x00010110)

#define CS40L26_MEM_RDY_MASK             (1)
#define CS40L26_MEM_RDY_SHIFT      (1)

  /** @} */

#endif /* __CS40L26_SPEC_H__ */
