/**
 * @file cs47l15_spec.h
 *
 * @brief Constants and Types from CS47L15 datasheet
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

#ifndef CS35L42_SPEC_H
#define CS35L42_SPEC_H

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
 * @defgroup CS35L42_DATASHEET
 * @brief All stuff from the datasheet
 *
 * @{
 */

/************************************************/
/* Software Reset and Hardware ID               */
/************************************************/
#define CS35L42_DEVID                                     0x0000000
#define CS35L42_REVID                                     0x0000004
#define CS35L42_FABID                                     0x0000008
#define CS35L42_RELID                                     0x000000C
#define CS35L42_OTPID                                     0x0000010
#define CS35L42_SFT_RESET                                 0x0000020

/************************************************/
/* Test Register Access                         */
/************************************************/
#define CS35L42_TEST_KEY_CTRL                             0x0000040
#define CS35L42_USER_KEY_CTRL                             0x0000044

/************************************************/
/* CTRL_ASYNC                                   */
/************************************************/
#define CS35L42_CTRL_ASYNC0                               0x0000050
#define CS35L42_CTRL_ASYNC1                               0x0000054
#define CS35L42_CTRL_ASYNC2                               0x0000058
#define CS35L42_CTRL_ASYNC3                               0x000005C

/************************************************/
/* Control Interface Configuration              */
/************************************************/
#define CS35L42_CTRL_IF_CONFIG1                           0x0000100
#define CS35L42_CTRL_IF_STATUS1                           0x0000104
#define CS35L42_CTRL_IF_STATUS2                           0x0000108
#define CS35L42_CTRL_IF_CONFIG2                           0x0000110
#define CS35L42_CTRL_IF_DEBUG1                            0x0000120
#define CS35L42_CTRL_IF_DEBUG2                            0x0000124
#define CS35L42_CTRL_IF_DEBUG3                            0x0000128
#define CS35L42_CIF_MON1                                  0x0000140
#define CS35L42_CIF_MON2                                  0x0000144
#define CS35L42_CIF_MON_PADDR                             0x0000148
#define CS35L42_CTRL_IF_SPARE1                            0x0000154
#define CS35L42_CTRL_IF_I2C                               0x0000158
#define CS35L42_CTRL_IF_I2C_1_CONTROL                     0x0000160
#define CS35L42_CTRL_IF_I2C_1_BROADCAST                   0x0000164
#define CS35L42_APB_MSTR_DSP_BRIDGE_ERR                   0x0000174
#define CS35L42_CIF1_BRIDGE_ERR                           0x0000178
#define CS35L42_CIF2_BRIDGE_ERR                           0x000017C

/************************************************/
/* One-Time Programmable (OTP) Control          */
/************************************************/
#define CS35L42_OTP_CTRL0                                 0x0000400
#define CS35L42_OTP_CTRL1                                 0x0000404
#define CS35L42_OTP_CTRL3                                 0x0000408
#define CS35L42_OTP_CTRL4                                 0x000040C
#define CS35L42_OTP_CTRL5                                 0x0000410
#define CS35L42_OTP_CTRL6                                 0x0000414
#define CS35L42_OTP_CTRL7                                 0x0000418
#define CS35L42_OTP_CTRL8                                 0x000041C

/************************************************/
/* OTP_CTRL_OTP_CTRL8                           */
/************************************************/
#define CS35L42_OTP_BOOT_BUS_ERR                          0x00000040  /* OTP_BOOT_BUS_ERR - [6] */
#define CS35L42_OTP_BOOT_BUS_ERR_MASK                     0x00000040  /* OTP_BOOT_BUS_ERR - [6] */
#define CS35L42_OTP_BOOT_BUS_ERR_SHIFT                    6           /* OTP_BOOT_BUS_ERR - [6] */
#define CS35L42_OTP_BOOT_BUS_ERR_WIDTH                    1           /* OTP_BOOT_BUS_ERR - [6] */
#define CS35L42_OTP_HDR_STS                               0x00000020  /* OTP_HDR_STS - [5] */
#define CS35L42_OTP_HDR_STS_MASK                          0x00000020  /* OTP_HDR_STS - [5] */
#define CS35L42_OTP_HDR_STS_SHIFT                         5           /* OTP_HDR_STS - [5] */
#define CS35L42_OTP_HDR_STS_WIDTH                         1           /* OTP_HDR_STS - [5] */
#define CS35L42_OTP_HDR_ERR                               0x00000010  /* OTP_HDR_ERR - [4] */
#define CS35L42_OTP_HDR_ERR_MASK                          0x00000010  /* OTP_HDR_ERR - [4] */
#define CS35L42_OTP_HDR_ERR_SHIFT                         4           /* OTP_HDR_ERR - [4] */
#define CS35L42_OTP_HDR_ERR_WIDTH                         1           /* OTP_HDR_ERR - [4] */
#define CS35L42_OTP_BOOT_DONE_STS                         0x00000004  /* OTP_BOOT_DONE_STS - [2] */
#define CS35L42_OTP_BOOT_DONE_STS_MASK                    0x00000004  /* OTP_BOOT_DONE_STS - [2] */
#define CS35L42_OTP_BOOT_DONE_STS_SHIFT                   2           /* OTP_BOOT_DONE_STS - [2] */
#define CS35L42_OTP_BOOT_DONE_STS_WIDTH                   1           /* OTP_BOOT_DONE_STS - [2] */
#define CS35L42_OTP_BOOT_ERR                              0x00000002  /* OTP_BOOT_ERR - [1] */
#define CS35L42_OTP_BOOT_ERR_MASK                         0x00000002  /* OTP_BOOT_ERR - [1] */
#define CS35L42_OTP_BOOT_ERR_SHIFT                        1           /* OTP_BOOT_ERR - [1] */
#define CS35L42_OTP_BOOT_ERR_WIDTH                        1           /* OTP_BOOT_ERR - [1] */
#define CS35L42_OTP_BOOT_TIMEOUT_ERR                      0x00000001  /* OTP_BOOT_TIMEOUT_ERR - [0] */
#define CS35L42_OTP_BOOT_TIMEOUT_ERR_MASK                 0x00000001  /* OTP_BOOT_TIMEOUT_ERR - [0] */
#define CS35L42_OTP_BOOT_TIMEOUT_ERR_SHIFT                0           /* OTP_BOOT_TIMEOUT_ERR - [0] */
#define CS35L42_OTP_BOOT_TIMEOUT_ERR_WIDTH                1           /* OTP_BOOT_TIMEOUT_ERR - [0] */

/************************************************/
/* Power, Global, and Release Control           */
/************************************************/
#define CS35L42_DEVICE_ID                                 0x0002004
#define CS35L42_FAB_ID                                    0x0002008
#define CS35L42_REV_ID                                    0x000200C
#define CS35L42_GLOBAL_ENABLES                            0x0002014
#define CS35L42_BLOCK_ENABLES                             0x0002018
#define CS35L42_BLOCK_ENABLES2                            0x000201C
#define CS35L42_GLOBAL_OVERRIDES                          0x0002020
#define CS35L42_GLOBAL_SYNC                               0x0002024
#define CS35L42_GLOBAL_STATUS                             0x0002028
#define CS35L42_DISCH_FILT                                0x000202C
#define CS35L42_OSC_TRIM                                  0x0002030
#define CS35L42_ERROR_RELEASE                             0x0002034
#define CS35L42_PLL_OVERRIDE                              0x0002038
#define CS35L42_CHIP_STATUS                               0x0002040
#define CS35L42_CHIP_STATUS2                              0x0002044
#define CS35L42_TEST_REGS1                                0x0002080
#define CS35L42_TEST_REGS2                                0x0002084
#define CS35L42_TEST_REGS3                                0x0002088
#define CS35L42_TEST_REGS4                                0x000208C
#define CS35L42_TEST_REGS5                                0x0002090
#define CS35L42_TEST_REGS6                                0x0002094
#define CS35L42_ERROR_RELEASE_EN                          0x0002100
#define CS35L42_BYPASS_MSM_ENABLE_PROXY                   0x0002104
#define CS35L42_UCNTL_MSM_INGRESS_OVERRIDES               0x0002110
#define CS35L42_UCNTL_MSM_EGRESS_OVERRIDES                0x0002114
#define CS35L42_UCNTL_MSM_AMP_INGRESS                     0x0002120
#define CS35L42_UCNTL_MSM_AMP_INGRESS_OVERRIDE            0x0002124
#define CS35L42_UCNTL_MSM_AMP_EGRESS                      0x0002128
#define CS35L42_UCNTL_MSM_AMP_EGRESS_OVERRIDE             0x000212C
#define CS35L42_UCNTL_MSM_BST_INGRESS                     0x0002130
#define CS35L42_UCNTL_MSM_BST_INGRESS_OVERRIDE            0x0002134
#define CS35L42_UCNTL_MSM_BST_EGRESS                      0x0002138
#define CS35L42_UCNTL_MSM_BST_EGRESS_OVERRIDE             0x000213C
#define CS35L42_UCNTL_MSM_CLASSH_INGRESS                  0x0002140
#define CS35L42_UCNTL_MSM_CLASSH_INGRESS_OVERRIDE         0x0002144
#define CS35L42_UCNTL_MSM_CLASSH_EGRESS                   0x0002148
#define CS35L42_UCNTL_MSM_CLASSH_EGRESS_OVERRIDE          0x000214C
#define CS35L42_UCNTL_MSM_MDSYNC_INGRESS                  0x0002150
#define CS35L42_UCNTL_MSM_MDSYNC_INGRESS_OVERRIDE         0x0002154
#define CS35L42_UCNTL_MSM_MDSYNC_EGRESS                   0x0002158
#define CS35L42_UCNTL_MSM_MDSYNC_EGRESS_OVERRIDE          0x000215C
#define CS35L42_UCNTL_MSM_CLK_INGRESS                     0x0002160
#define CS35L42_UCNTL_MSM_CLK_INGRESS_OVERRIDE            0x0002164
#define CS35L42_UCNTL_MSM_CLK_EGRESS                      0x0002168
#define CS35L42_UCNTL_MSM_CLK_EGRESS_OVERRIDE             0x000216C
#define CS35L42_UCNTL_MSM_VMON_INGRESS                    0x0002170
#define CS35L42_UCNTL_MSM_VMON_INGRESS_OVERRIDE           0x0002174
#define CS35L42_UCNTL_MSM_VMON_EGRESS                     0x0002178
#define CS35L42_UCNTL_MSM_VMON_EGRESS_OVERRIDE            0x000217C
#define CS35L42_UCNTL_MSM_IMON_INGRESS                    0x0002180
#define CS35L42_UCNTL_MSM_IMON_INGRESS_OVERRIDE           0x0002184
#define CS35L42_UCNTL_MSM_IMON_EGRESS                     0x0002188
#define CS35L42_UCNTL_MSM_IMON_EGRESS_OVERRIDE            0x000218C
#define CS35L42_UCNTL_MSM_VPMON_INGRESS                   0x0002190
#define CS35L42_UCNTL_MSM_VPMON_INGRESS_OVERRIDE          0x0002194
#define CS35L42_UCNTL_MSM_VPMON_EGRESS                    0x0002198
#define CS35L42_UCNTL_MSM_VPMON_EGRESS_OVERRIDE           0x000219C
#define CS35L42_UCNTL_MSM_VBSTMON_INGRESS                 0x00021A0
#define CS35L42_UCNTL_MSM_VBSTMON_INGRESS_OVERRIDE        0x00021A4
#define CS35L42_UCNTL_MSM_VBSTMON_EGRESS                  0x00021A8
#define CS35L42_UCNTL_MSM_VBSTMON_EGRESS_OVERRIDE         0x00021AC
#define CS35L42_UCNTL_MSM_TEMPMON_INGRESS                 0x00021B0
#define CS35L42_UCNTL_MSM_TEMPMON_INGRESS_OVERRIDE        0x00021B4
#define CS35L42_UCNTL_MSM_TEMPMON_EGRESS                  0x00021B8
#define CS35L42_UCNTL_MSM_TEMPMON_EGRESS_OVERRIDE         0x00021BC
#define CS35L42_UCNTL_MSM_VPBR_INGRESS                    0x00021C0
#define CS35L42_UCNTL_MSM_VPBR_INGRESS_OVERRIDE           0x00021C4
#define CS35L42_UCNTL_MSM_VPBR_EGRESS                     0x00021C8
#define CS35L42_UCNTL_MSM_VPBR_EGRESS_OVERRIDE            0x00021CC
#define CS35L42_UCNTL_MSM_VBBR_INGRESS                    0x00021D0
#define CS35L42_UCNTL_MSM_VBBR_INGRESS_OVERRIDE           0x00021D4
#define CS35L42_UCNTL_MSM_VBBR_EGRESS                     0x00021D8
#define CS35L42_UCNTL_MSM_VBBR_EGRESS_OVERRIDE            0x00021DC
#define CS35L42_UCNTL_MSM_VPI_INGRESS                     0x00021F0
#define CS35L42_UCNTL_MSM_VPI_INGRESS_OVERRIDE            0x00021F4
#define CS35L42_UCNTL_MSM_VPI_EGRESS                      0x00021F8
#define CS35L42_UCNTL_MSM_VPI_EGRESS_OVERRIDE             0x00021FC
#define CS35L42_UCNTL_MSM_AMP_DRE_INGRESS                 0x0002200
#define CS35L42_UCNTL_MSM_AMP_DRE_INGRESS_OVERRIDE        0x0002204
#define CS35L42_UCNTL_MSM_AMP_DRE_EGRESS                  0x0002208
#define CS35L42_UCNTL_MSM_AMP_DRE_EGRESS_OVERRIDE         0x000220C
#define CS35L42_UCNTL_MSM_WKFET_AMP_INGRESS               0x0002210
#define CS35L42_UCNTL_MSM_WKFET_AMP_INGRESS_OVERRIDE      0x0002214
#define CS35L42_UCNTL_MSM_WKFET_AMP_EGRESS                0x0002218
#define CS35L42_UCNTL_MSM_WKFET_AMP_EGRESS_OVERRIDE       0x000221C
#define CS35L42_UCNTL_MSM_CCM_INGRESS                     0x0002220
#define CS35L42_UCNTL_MSM_CCM_INGRESS_OVERRIDE            0x0002224
#define CS35L42_UCNTL_MSM_CCM_EGRESS                      0x0002228
#define CS35L42_UCNTL_MSM_CCM_EGRESS_OVERRIDE             0x000222C
#define CS35L42_UCNTL_MSM_OTP_INGRESS                     0x0002230
#define CS35L42_UCNTL_MSM_OTP_INGRESS_OVERRIDE            0x0002234
#define CS35L42_UCNTL_MSM_MISC_DIGITAL_INGRESS            0x0002240
#define CS35L42_UCNTL_MSM_MISC_DIGITAL_INGRESS_OVERRIDE   0x0002244
#define CS35L42_UCNTL_MSM_MISC_DIGITAL_EGRESS             0x0002248
#define CS35L42_UCNTL_MSM_MISC_DIGITAL_EGRESS_OVERRIDE    0x000224C
#define CS35L42_UCNTL_MSM_MISC_ANALOG_EGRESS              0x0002258
#define CS35L42_UCNTL_MSM_MISC_ANALOG_EGRESS_OVERRIDE     0x000225C

/************************************************/
/* Power Control 1                              */
/************************************************/
#define CS35L42_GLOBAL_EN_FROM_GPIO_STS                   0x00000200  /* GLOBAL_EN_FROM_GPIO_STS - [9] */
#define CS35L42_GLOBAL_EN_FROM_GPIO_STS_MASK              0x00000200  /* GLOBAL_EN_FROM_GPIO_STS - [9] */
#define CS35L42_GLOBAL_EN_FROM_GPIO_STS_SHIFT             9           /* GLOBAL_EN_FROM_GPIO_STS - [9] */
#define CS35L42_GLOBAL_EN_FROM_GPIO_STS_WIDTH             1           /* GLOBAL_EN_FROM_GPIO_STS - [9] */
#define CS35L42_GLOBAL_EN_FROM_GPIO_CTRL                  0x00000100  /* GLOBAL_EN_FROM_GPIO_CTRL - [8] */
#define CS35L42_GLOBAL_EN_FROM_GPIO_CTRL_MASK             0x00000100  /* GLOBAL_EN_FROM_GPIO_CTRL - [8] */
#define CS35L42_GLOBAL_EN_FROM_GPIO_CTRL_SHIFT            8           /* GLOBAL_EN_FROM_GPIO_CTRL - [8] */
#define CS35L42_GLOBAL_EN_FROM_GPIO_CTRL_WIDTH            1           /* GLOBAL_EN_FROM_GPIO_CTRL - [8] */
#define CS35L42_GLOBAL_EN                                 0x00000001  /* GLOBAL_EN - [0] */
#define CS35L42_GLOBAL_EN_MASK                            0x00000001  /* GLOBAL_EN - [0] */
#define CS35L42_GLOBAL_EN_SHIFT                           0           /* GLOBAL_EN - [0] */
#define CS35L42_GLOBAL_EN_WIDTH                           1           /* GLOBAL_EN - [0] */

/************************************************/
/* Digital I/O Pad Control                      */
/************************************************/
#define CS35L42_BIAS_PTE_MODE_CONTROL                     0x0002404
#define CS35L42_SCL_PAD_CONTROL                           0x0002408
#define CS35L42_SDA_PAD_CONTROL                           0x000240C
#define CS35L42_LRCK_PAD_CONTROL                          0x0002418
#define CS35L42_SCLK_PAD_CONTROL                          0x000241C
#define CS35L42_SDIN_PAD_CONTROL                          0x0002420
#define CS35L42_SDOUT_PAD_CONTROL                         0x0002424
#define CS35L42_GPIO_PAD_CONTROL                          0x000242C
#define CS35L42_MDSYNC_PAD_CONTROL                        0x0002430
#define CS35L42_TEST_I01_PAD_CONTROL                      0x0002434
#define CS35L42_JTAG_CONTROL                              0x0002438
#define CS35L42_GPIO1_PAD_CONTROL                         0x000243C
#define CS35L42_GPIO_GLOBAL_ENABLE_CONTROL                0x0002440
#define CS35L42_GPIO_LEVELSHIFT_BYPASS                    0x0002444

/************************************************/
/* Auxiliary Control                            */
/************************************************/
#define CS35L42_I2C_ADDR_DETECT_CNTL0                     0x0002800
#define CS35L42_I2C_ADDR_DET_STATUS0                      0x0002820
#define CS35L42_OTP_OVERRIDE_CNTL                         0x0002844
#define CS35L42_DEVID_METAL                               0x0002854

/************************************************/
/* Hibernation Power Management                 */
/************************************************/
#define CS35L42_PWRMGT_CTL                                0x0002900
#define CS35L42_WAKESRC_CTL                               0x0002904
#define CS35L42_WAKEI2C_CTL                               0x0002908
#define CS35L42_PWRMGT_STS                                0x000290C
#define CS35L42_PWRMGT_RST                                0x0002910
#define CS35L42_TEST_CTL                                  0x0002914

/************************************************/
/* Device Clocking and Sample Rate Control      */
/************************************************/
#define CS35L42_REFCLK_INPUT                              0x0002C04
#define CS35L42_DSP_CLOCK_GEARING                         0x0002C08
#define CS35L42_GLOBAL_SAMPLE_RATE                        0x0002C0C
#define CS35L42_DATA_FS_SEL                               0x0002C10
#define CS35L42_FREE_RUN_FORCE                            0x0002C14
#define CS35L42_ASP_RATE_DOUBLE_CONTROL0                  0x0002C18
#define CS35L42_NZ_AUDIO_DETECT0                          0x0002C1C
#define CS35L42_NZ_AUDIO_DETECT1                          0x0002C20
#define CS35L42_NZ_AUDIO_DETECT2                          0x0002C24
#define CS35L42_PLL_REFCLK_DETECT_0                       0x0002C28
#define CS35L42_SP_SCLK_CLOCKING                          0x0002D00
#define CS35L42_CONFIG0                                   0x0002D04
#define CS35L42_CONFIG1                                   0x0002D08
#define CS35L42_CONFIG2                                   0x0002D0C
#define CS35L42_FS_MON_0                                  0x0002D10
#define CS35L42_FS_MON_1                                  0x0002D14
#define CS35L42_FS_MON_2                                  0x0002D18
#define CS35L42_FS_MON_OVERRIDE                           0x0002D1C
#define CS35L42_DFT                                       0x0002D20
#define CS35L42_ANALOG_ADC_CONTROLS                       0x0002D24
#define CS35L42_SPK_CHOP_CLK_CONTROLS                     0x0002D28
#define CS35L42_TST_DIVIDE_OVR_ROOT_CLK                   0x0002D2C
#define CS35L42_TRIGGER_RESYNC1                           0x0002D30
#define CS35L42_TRIGGER_RESYNC2                           0x0002D34
#define CS35L42_DSP1_SAMPLE_RATE_RX1                      0x0002D3C
#define CS35L42_DSP1_SAMPLE_RATE_RX2                      0x0002D40
#define CS35L42_DSP1_SAMPLE_RATE_TX1                      0x0002D60
#define CS35L42_DSP1_SAMPLE_RATE_TX2                      0x0002D64
#define CS35L42_CLOCK_PHASE                               0x0002D80

/************************************************/
/* DPLL                                         */
/************************************************/
#define CS35L42_USER_CONTROL                              0x0003000
#define CS35L42_CONFIG_RATES                              0x0003004
#define CS35L42_LOOP_PARAMETERS                           0x0003008
#define CS35L42_LDOA_CONTROL                              0x000300C
#define CS35L42_DCO_CONTROL                               0x0003010
#define CS35L42_MISC_CONTROL                              0x0003014
#define CS35L42_LOOP_OVERRIDES                            0x0003018
#define CS35L42_DCO_CTRL_OVERRIDES                        0x000301C
#define CS35L42_CONTROL_READ                              0x0003020
#define CS35L42_CONTROL_READ_2                            0x0003024
#define CS35L42_DCO_CAL_CONTROL_1                         0x0003028
#define CS35L42_DCO_CAL_CONTROL_2                         0x000302C
#define CS35L42_DCO_CAL_STATUS                            0x0003030
#define CS35L42_TEST2                                     0x0003034

/************************************************/
/* Multidevice Synchronization                  */
/************************************************/
#define CS35L42_SYNC_TX_RX_ENABLES                        0x0003400
#define CS35L42_SYNC_POWER_CTL                            0x0003404
#define CS35L42_SYNC_SW_TX_ID                             0x0003408
#define CS35L42_MDSYNC_SYNC_SW_BLOCKED                    0x0003410
#define CS35L42_SYNC_SW_RX                                0x0003414
#define CS35L42_SYNC_SW_TX                                0x0003418
#define CS35L42_MDSYNC_SYNC_LSW_BLOCKED                   0x0003420
#define CS35L42_SYNC_LSW_RX                               0x0003424
#define CS35L42_SYNC_LSW_TX                               0x0003428
#define CS35L42_SYNC_SW_DATA_TX_STATUS                    0x0003430
#define CS35L42_SYNC_SW_DATA_RX_STATUS                    0x0003434
#define CS35L42_SYNC_ERROR_STATUS                         0x0003438
#define CS35L42_MDSYNC_SYNC_RX_DECODE_CTL_1               0x0003500
#define CS35L42_MDSYNC_SYNC_RX_DECODE_CTL_2               0x0003504
#define CS35L42_MDSYNC_SYNC_TX_ENCODE_CTL                 0x0003508
#define CS35L42_MDSYNC_SYNC_IDLE_STATE_CTL                0x000350C
#define CS35L42_MDSYNC_SYNC_SLEEP_STATE_CTL               0x0003510
#define CS35L42_MDSYNC_SYNC_TYPE                          0x0003514
#define CS35L42_MDSYNC_SYNC_TRIGGER                       0x0003518
#define CS35L42_MDSYNC_SYNC_PTE0                          0x0003520
#define CS35L42_MDSYNC_SYNC_PTE1                          0x0003524
#define CS35L42_MDSYNC_SYNC_PTE2                          0x0003528
#define CS35L42_MDSYNC_SYNC_PTE3                          0x000352C
#define CS35L42_MDSYNC_SYNC_TEST_CTL                      0x0003530
#define CS35L42_MDSYNC_SYNC_MSM_STATUS                    0x000353C
#define CS35L42_MDSYNC_RX_ZERO_TOGGLE_CNT                 0x0003540
#define CS35L42_MDSYNC_RX_ONE_TOGGLE_CNT                  0x0003544
#define CS35L42_MDSYNC_RX_SYNC_TOGGLE_CNT                 0x0003548
#define CS35L42_MDSYNC_RX_SYNC_LONG_TOGGLE_CNT            0x000354C

/************************************************/
/* Digital Boost Converter                      */
/************************************************/
#define CS35L42_VBST_CTL_1                                0x0003800
#define CS35L42_VBST_CTL_2                                0x0003804
#define CS35L42_BST_IPK_CTL                               0x0003808
#define CS35L42_SOFT_RAMP                                 0x000380C
#define CS35L42_BST_LOOP_COEFF                            0x0003810
#define CS35L42_LBST_SLOPE                                0x0003814
#define CS35L42_BST_SW_FREQ                               0x0003818
#define CS35L42_BST_DCM_CTL                               0x000381C
#define CS35L42_DCM_FORCE                                 0x0003820
#define CS35L42_VBST_OVP                                  0x0003830
#define CS35L42_BST_DCR                                   0x0003840
#define CS35L42_RSVD_1                                    0x0003850
#define CS35L42_RSVD_2                                    0x0003854
#define CS35L42_RSVD_3                                    0x0003858
#define CS35L42_RSVD_4                                    0x000385C
#define CS35L42_TEST_DUTY                                 0x0003900
#define CS35L42_TEST_CMODE                                0x0003904
#define CS35L42_TEST_VMODE1                               0x0003908
#define CS35L42_TEST_VMODE2                               0x000390C
#define CS35L42_TEST_DCM1                                 0x0003910
#define CS35L42_TEST_DCM2                                 0x0003914
#define CS35L42_TEST_DCM3                                 0x0003918
#define CS35L42_TEST_LBST                                 0x000391C
#define CS35L42_TEST_LIEST                                0x0003920
#define CS35L42_TEST_LIEST_TRIM                           0x0003924
#define CS35L42_TEST_IADC                                 0x0003928
#define CS35L42_TEST_IPK                                  0x000392C
#define CS35L42_TEST_VBST                                 0x0003930
#define CS35L42_TEST_CURR                                 0x0003934
#define CS35L42_TEST_VFILT                                0x0003938
#define CS35L42_TEST_MANUAL                               0x000393C
#define CS35L42_TEST_WAIT                                 0x0003940
#define CS35L42_TEST_GL                                   0x0003944
#define CS35L42_TEST_ANA1                                 0x0003948
#define CS35L42_TEST_ANA2                                 0x000394C
#define CS35L42_TEST_MONITOR                              0x0003960
#define CS35L42_TEST_RDA                                  0x0003964
#define CS35L42_TEST_MSM_OVR                              0x0003968
#define CS35L42_TEST_CAL_CFG                              0x0003970
#define CS35L42_TEST_CAL_IADC_STAT                        0x0003974
#define CS35L42_TEST_CAL_IPEAK_STAT                       0x0003978
#define CS35L42_TEST_STAT1                                0x0003980
#define CS35L42_TEST_STAT2                                0x0003984
#define CS35L42_TEST_STAT3                                0x0003988
#define CS35L42_TEST_STAT4                                0x000398C
#define CS35L42_TEST_STAT5                                0x0003990
#define CS35L42_TEST_STAT6                                0x0003994
#define CS35L42_TEST_STAT7                                0x0003998
#define CS35L42_TEST_STAT8                                0x000399C
#define CS35L42_TEST_STAT9                                0x00039A0
#define CS35L42_TEST_STAT10                               0x00039A4

/************************************************/
/* VPI Input Current Limiting                   */
/************************************************/
#define CS35L42_VPI_LIMIT_MODE                            0x0003C04
#define CS35L42_VPI_LIMITING                              0x0003C08
#define CS35L42_VPI_VP_THLDS                              0x0003C0C
#define CS35L42_VPI_TRACKING                              0x0003C10
#define CS35L42_VPI_TRIG_TIME                             0x0003C14
#define CS35L42_VPI_TRIG_STEPS                            0x0003C18
#define CS35L42_VPI_STATES                                0x0003E04
#define CS35L42_VPI_OUTPUT_RATE                           0x0003E08

/************************************************/
/* VMON and IMON Signal Monitoring              */
/************************************************/
#define CS35L42_VMON_IMON_VOL_POL                         0x0004000
#define CS35L42_SPKMON_RATE_SEL                           0x0004004
#define CS35L42_MONITOR_FILT                              0x0004008
#define CS35L42_IMON_COMP                                 0x0004010
#define CS35L42_SPKMON_RESYNC                             0x0004100
#define CS35L42_SPKMON_RESYNC_COUNTS                      0x0004104
#define CS35L42_SPKMON_INP_SEL                            0x0004110
#define CS35L42_SPKMON_TESTBITS                           0x0004114
#define CS35L42_SPKMON_DELAYLINE_TESTBITS                 0x0004120
#define CS35L42_SPKMON_ADC_FSM_VIMON_FORCE                0x0004124
#define CS35L42_SPKMON_ADC_VMON_ANA_CNTL_TESTBITS         0x0004148
#define CS35L42_SPKMON_ADC_IMON_ANA_CNTL_TESTBITS         0x000414C
#define CS35L42_SPKMON_ADC_DEM_ANA_CNTL_TESTBITS          0x0004154
#define CS35L42_SPKMON_ADC_CAL_TESTBITS                   0x000417C
#define CS35L42_SPKMON_CAL_SCALE_OVRD_VMON                0x0004180
#define CS35L42_SPKMON_CAL_SCALE_OVRD_IMON                0x0004184
#define CS35L42_SPKMON_DEC_OUT_READ_SEL                   0x00041B0
#define CS35L42_SPKMON_VMON_DEC_OUT_DATA                  0x00041B4
#define CS35L42_SPKMON_IMON_DEC_OUT_DATA                  0x00041B8
#define CS35L42_SPKMON_ADC_FSM_STATUS                     0x00041F0

/************************************************/
/* Die Temperature Monitoring                   */
/************************************************/
#define CS35L42_WARN_LIMIT_THRESHOLD                      0x0004220
#define CS35L42_CONFIGURATION                             0x0004224
#define CS35L42_STATUS                                    0x0004300
#define CS35L42_ENABLES_AND_CODES_ANA                     0x0004304
#define CS35L42_ENABLES_AND_CODES_DIG                     0x0004308
#define CS35L42_CALIBR_STATUS                             0x000430C
#define CS35L42_TEMP_RESYNC                               0x0004310
#define CS35L42_ERROR_LIMIT_THRESHOLD_OVERIDE             0x0004320
#define CS35L42_WARN_LIMIT_THRESHOLD_OVERIDE              0x0004324
#define CS35L42_TEST_CONFIGURATIONS                       0x0004340
#define CS35L42_SAR_TEMP_MSM_OVERRIDE_CNTL                0x0004344
#define CS35L42_TEST_CONFIG_FORCE_TEMP                    0x0004348
#define CS35L42_CALIBR_ROUTINE_CONFIGURATIONS             0x0004368
#define CS35L42_STATUS_FS                                 0x0004380

/************************************************/
/* VPMON and VBSTMON Signal Monitoring          */
/************************************************/
#define CS35L42_SAR_FS_SEL                                0x0004400
#define CS35L42_SAR_LVL_DETECT_STATUS                     0x000440C
#define CS35L42_SAR_LVL_DETECTOR_CNTL_1_0                 0x0004410
#define CS35L42_SAR_LVL_DETECTOR_CNTL_3_2                 0x0004414
#define CS35L42_SAR_LVL_DETECTOR_CNTL_5_4                 0x0004418
#define CS35L42_SAR_VP_CTL                                0x0004440
#define CS35L42_SAR_VBST_CTL                              0x0004444
#define CS35L42_SAR_ANA_TRIM                              0x000444C
#define CS35L42_SAR_MSM_STATUS                            0x0004450
#define CS35L42_SAR_MSM                                   0x0004454
#define CS35L42_SAR_VP_STATUS_VALUE                       0x0004458
#define CS35L42_SAR_VP_STATUS_FLAGS                       0x000445C
#define CS35L42_SAR_VP_CAL_OFF_VAL_STATUS                 0x0004460
#define CS35L42_SAR_VP_CAL_GAIN_VAL_STATUS                0x0004464
#define CS35L42_SAR_VP_CAL_D1_IDEAL                       0x0004468
#define CS35L42_SAR_VP_CAL_D2_IDEAL                       0x000446C
#define CS35L42_SAR_VBST_STATUS_VALUE                     0x0004470
#define CS35L42_SAR_VBST_STATUS_FLAGS                     0x0004474
#define CS35L42_SAR_VBST_CAL_OFF_VAL_STATUS               0x0004478
#define CS35L42_SAR_VBST_CAL_GAIN_VAL_STATUS              0x000447C
#define CS35L42_SAR_VBST_CAL_D1_IDEAL                     0x0004480
#define CS35L42_SAR_VBST_CAL_D2_IDEAL                     0x0004484
#define CS35L42_SAR_ANA_VIS_CNTL                          0x0004488

/************************************************/
/* ASP Data Interface                           */
/************************************************/
#define CS35L42_ASP_ENABLES1                              0x0004800
#define CS35L42_ASP_CONTROL1                              0x0004804
#define CS35L42_ASP_CONTROL2                              0x0004808
#define CS35L42_ASP_CONTROL3                              0x000480C
#define CS35L42_ASP_FRAME_CONTROL1                        0x0004810
#define CS35L42_ASP_FRAME_CONTROL5                        0x0004820
#define CS35L42_ASP_DATA_CONTROL1                         0x0004830
#define CS35L42_ASP_DATA_CONTROL5                         0x0004840
#define CS35L42_ASP_LATENCY1                              0x0004850
#define CS35L42_ASP_CONTROL4                              0x0004854
#define CS35L42_ASP_FSYNC_CONTROL1                        0x0004860
#define CS35L42_ASP_FSYNC_CONTROL2                        0x0004864
#define CS35L42_ASP_FSYNC_STATUS1                         0x0004868
#define CS35L42_ASP_TEST1                                 0x000486C
#define CS35L42_ASP_CLOCK_OVD1                            0x0004870

/************************************************/
/* Data Routing                                 */
/************************************************/
#define CS35L42_DACPCM1_INPUT                             0x0004C00
#define CS35L42_DACMETA1_INPUT                            0x0004C04
#define CS35L42_DACPCM2_INPUT                             0x0004C08
#define CS35L42_ASPTX1_INPUT                              0x0004C20
#define CS35L42_ASPTX2_INPUT                              0x0004C24
#define CS35L42_ASPTX3_INPUT                              0x0004C28
#define CS35L42_ASPTX4_INPUT                              0x0004C2C
#define CS35L42_DSP1RX1_INPUT                             0x0004C40
#define CS35L42_DSP1RX2_INPUT                             0x0004C44
#define CS35L42_DSP1RX3_INPUT                             0x0004C48
#define CS35L42_DSP1RX4_INPUT                             0x0004C4C
#define CS35L42_DSP1RX5_INPUT                             0x0004C50
#define CS35L42_DSP1RX6_INPUT                             0x0004C54
#define CS35L42_NGATE1_INPUT                              0x0004C60
#define CS35L42_NGATE2_INPUT                              0x0004C64

/************************************************/
/* Digital and Analog Test Visibility           */
/************************************************/
#define CS35L42_SPARE_CP_BITS_0                           0x0005C00
#define CS35L42_VIS_ADDR_CNTL1_4                          0x0005C40
#define CS35L42_VIS_ADDR_CNTL5_8                          0x0005C44
#define CS35L42_VIS_ADDR_CNTL9_12                         0x0005C48
#define CS35L42_VIS_ADDR_CNTL13_16                        0x0005C4C
#define CS35L42_VIS_ADDR_CNTL_17_20                       0x0005C50
#define CS35L42_BLOCK_SEL_CNTL0_3                         0x0005C54
#define CS35L42_BIT_SEL_CNTL                              0x0005C5C
#define CS35L42_ANALOG_VIS_CNTL                           0x0005C60

/************************************************/
/* Amplifier Volume Control                     */
/************************************************/
#define CS35L42_AMP_CTRL                                  0x0006000
#define CS35L42_HPF_TST                                   0x0006004
#define CS35L42_VC_TST1                                   0x0006008
#define CS35L42_VC_TST2                                   0x000600C
#define CS35L42_INTP_TST                                  0x0006010

/************************************************/
/* Amplifier Digital Volume Control             */
/************************************************/
#define CS35L42_AMP_HPF_PCM_EN                           0x00008000  /* AMP_HPF_PCM_EN - [15] */
#define CS35L42_AMP_HPF_PCM_EN_MASK                      0x00008000  /* AMP_HPF_PCM_EN - [15] */
#define CS35L42_AMP_HPF_PCM_EN_SHIFT                     15          /* AMP_HPF_PCM_EN - [15] */
#define CS35L42_AMP_HPF_PCM_EN_WIDTH                     1           /* AMP_HPF_PCM_EN - [15] */
#define CS35L42_AMP_INV_PCM                              0x00004000  /* AMP_INV_PCM - [14] */
#define CS35L42_AMP_INV_PCM_MASK                         0x00004000  /* AMP_INV_PCM - [14] */
#define CS35L42_AMP_INV_PCM_SHIFT                        14          /* AMP_INV_PCM - [14] */
#define CS35L42_AMP_INV_PCM_WIDTH                        1           /* AMP_INV_PCM - [14] */
#define CS35L42_AMP_VOL_PCM                              0x00003FF8  /* AMP_VOL_PCM - [13:3] */
#define CS35L42_AMP_VOL_PCM_MASK                         0x00003FF8  /* AMP_VOL_PCM - [13:3] */
#define CS35L42_AMP_VOL_PCM_SHIFT                        3           /* AMP_VOL_PCM - [13:3] */
#define CS35L42_AMP_VOL_PCM_WIDTH                        11          /* AMP_VOL_PCM - [13:3] */
#define CS35L42_AMP_RAMP_PCM                             0x00000007  /* AMP_RAMP_PCM - [2:0] */
#define CS35L42_AMP_RAMP_PCM_MASK                        0x00000007  /* AMP_RAMP_PCM - [2:0] */
#define CS35L42_AMP_RAMP_PCM_SHIFT                       0           /* AMP_RAMP_PCM - [2:0] */
#define CS35L42_AMP_RAMP_PCM_WIDTH                       3           /* AMP_RAMP_PCM - [2:0] */

#define CS35L42_AMP_VOL_PCM_MUTE                         (0x400)         ///< @see Section 7.16.1
#define CS35L42_AMP_VOL_PCM_0DB                          (0)             ///< @see Section 7.16.1
#define CS35L42_AMP_VOL_PCM_MAX_DB                       (12)            ///< @see Section 7.16.1
#define CS35L42_AMP_VOL_PCM_MIN_DB                       (-102)          ///< @see Section 7.16.1

/************************************************/
/* SRC_MAGCOMP                                  */
/************************************************/
#define CS35L42_SRC_MAGCOMP_TST                           0x0006200
#define CS35L42_SRC_MAGCOMP_B0_OVERRIDE                   0x0006204
#define CS35L42_SRC_MAGCOMP_B1_OVERRIDE                   0x0006208
#define CS35L42_SRC_MAGCOMP_A1_N_OVERRIDE                 0x000620C

/************************************************/
/* VP and VBST Brownout Prevention and Temperature Warning */
/************************************************/
#define CS35L42_VPBR_CONFIG                               0x0006404
#define CS35L42_VBBR_CONFIG                               0x0006408
#define CS35L42_VPBR_STATUS                               0x000640C
#define CS35L42_VBBR_STATUS                               0x0006410
#define CS35L42_OTW_CONFIG                                0x0006414
#define CS35L42_AMP_ERROR_VOL_SEL                         0x0006418
#define CS35L42_VPBR_TEST_REG                             0x0006440
#define CS35L42_VBBR_TEST_REG                             0x0006444
#define CS35L42_VPBR_FILTER_CONFIG                        0x0006448
#define CS35L42_VBBR_FILTER_CONFIG                        0x000644C
#define CS35L42_VOL_STATUS_TO_DSP                         0x0006450
#define CS35L42_MISC_CONFIG_TEST                          0x0006454

/************************************************/
/* Power Management Class H, Weak-FET, and Noise Gating */
/************************************************/
#define CS35L42_CLASSH_CONFIG                             0x0006800
#define CS35L42_WKFET_AMP_CONFIG                          0x0006804
#define CS35L42_NG_CONFIG                                 0x0006808
#define CS35L42_CLASSH_TEST                               0x0006864
#define CS35L42_WKFET_AMP_TEST                            0x0006868

/************************************************/
/* Dynamic Range Enhancement                    */
/************************************************/
#define CS35L42_AMP_GAIN                                  0x0006C04
#define CS35L42_DRECNTL                                   0x0006E00
#define CS35L42_DREMDDECNTL                               0x0006E04
#define CS35L42_DREHYSTCNTL2                              0x0006E08
#define CS35L42_DREZCCTRL                                 0x0006E0C
#define CS35L42_DREPATTERN                                0x0006E10
#define CS35L42_DREHPFS                                   0x0006E14
#define CS35L42_DRERAMP                                   0x0006E1C
#define CS35L42_DREZCCTRL2                                0x0006E20
#define CS35L42_AMPGAINCTL                                0x0006E24
#define CS35L42_DREHYSTCNTL3                              0x0006E28
#define CS35L42_DREHYSTCNTL4                              0x0006E2C
#define CS35L42_DREANAOFFSET7                             0x0006E64

/************************************************/
/* Digital PWM Modulator                        */
/************************************************/
#define CS35L42_DIGPWM_CTRL                               0x0007060
#define CS35L42_DIGPWM_CONFIG                             0x0007064
#define CS35L42_DIGPWM_CONFIG2                            0x0007068
#define CS35L42_DIGPWM_IOCTRL                             0x000706C
#define CS35L42_DIGPWM_STATUS                             0x0007070
#define CS35L42_DIGPWM_CONFIG3                            0x0007074
#define CS35L42_DIGPWM_FB_OFFSET_CAL                      0x0007080
#define CS35L42_DIGPWM_FB_OFFSET_CAL2                     0x0007084
#define CS35L42_DIGPWM_LOOP_CFG8                          0x000709C
#define CS35L42_DIGPWM_LOOP_CFG10                         0x00070A0
#define CS35L42_DIGPWM_LOOP_CFG11                         0x00070A4
#define CS35L42_DIGPWM_LOOP_CFG12                         0x00070A8
#define CS35L42_DIGPWM_LOOP_CFG13                         0x00070AC
#define CS35L42_DIGPWM_LOOP_COEFF1                        0x00070B0
#define CS35L42_DIGPWM_LOOP_COEFF2                        0x00070B4
#define CS35L42_DIGPWM_LOOP_COEFF3                        0x00070B8
#define CS35L42_DIGPWM_LOOP_COEFF4                        0x00070BC
#define CS35L42_DIGPWM_LOOP_COEFF5                        0x00070C0
#define CS35L42_DIGPWM_LOOP_COEFF6                        0x00070C4
#define CS35L42_DIGPWM_LOOP_COEFF7                        0x00070C8
#define CS35L42_DIGPWM_MISC_COEFF                         0x00070DC
#define CS35L42_DIGPWM_LOOP_CLASSH_CTRL                   0x00070F0
#define CS35L42_DIGPWM_LOOP_ADC_CTRL                      0x00070F4
#define CS35L42_DIGPWM_LOOP_STS1                          0x00070F8
#define CS35L42_DIGPWM_LOOP_STS2                          0x00070FC
#define CS35L42_DIGPWM_DEBUG1                             0x0007100
#define CS35L42_DIGPWM_DEBUG2                             0x0007104
#define CS35L42_DIGPWM_DEBUG3                             0x0007108
#define CS35L42_DIGPWM_MC_COEFF1                          0x0007120
#define CS35L42_DIGPWM_MC_COEFF2                          0x0007124
#define CS35L42_DIGPWM_MC_COEFF3                          0x0007128
#define CS35L42_DIGPWM_MC_COEFF4                          0x000712C
#define CS35L42_DIGPWM_MC_COEFF5                          0x0007130
#define CS35L42_DIGPWM_MC_COEFF1_2                        0x0007134
#define CS35L42_DIGPWM_MC_COEFF2_2                        0x0007138
#define CS35L42_DIGPWM_MC_COEFF3_2                        0x000713C
#define CS35L42_DIGPWM_MC_COEFF4_2                        0x0007140
#define CS35L42_DIGPWM_MC_COEFF5_2                        0x0007144
#define CS35L42_DIGPWM_LOOP_CLASSH_CTRL_2                 0x0007164

/************************************************/
/* Negative Impedance Control                   */
/************************************************/
#define CS35L42_SVC_CTRL                                  0x0007200
#define CS35L42_SVC_SER_R                                 0x0007204
#define CS35L42_SVC_R_LPF                                 0x0007208
#define CS35L42_SVC_FILT_CFG                              0x000720C
#define CS35L42_SVC_SER_L_CTRL                            0x0007218
#define CS35L42_SVC_SER_C_CTRL                            0x000721C
#define CS35L42_SVC_PAR_RLC_SF                            0x0007220
#define CS35L42_SVC_PAR_RLC_C1                            0x0007224
#define CS35L42_SVC_PAR_RLC_C2                            0x0007228
#define CS35L42_SVC_PAR_RLC_B1                            0x000722C
#define CS35L42_SVC_GAIN                                  0x0007230
#define CS35L42_SVC_TEST1                                 0x0007234
#define CS35L42_SVC_STATUS                                0x0007238
#define CS35L42_SVC_IMON_SF                               0x000723C

/************************************************/
/* Amplifier Path Additional Control            */
/************************************************/
#define CS35L42_DAC_MSM_CONFIG                            0x0007400
#define CS35L42_TST_DAC_MSM_CONFIG                        0x0007404
#define CS35L42_DAC_STROBE_TST_1                          0x0007408
#define CS35L42_DAC_STROBE_TST_2                          0x000740C
#define CS35L42_TST_SPK_1                                 0x0007410
#define CS35L42_TST_SPK_2                                 0x0007414
#define CS35L42_TST_SPK_3_TRIM                            0x0007418
#define CS35L42_TST_SPK_4_TRIM                            0x000741C
#define CS35L42_TST_SPK_5_VIS                             0x0007420
#define CS35L42_ALIVE_DCIN_WD                             0x0007424
#define CS35L42_SPK_FORCE_TST_1                           0x0007428
#define CS35L42_SPK_FORCE_TST_2                           0x000742C
#define CS35L42_SPK_MSM_TST_1                             0x0007430
#define CS35L42_SPK_MSM_TST_2                             0x0007434
#define CS35L42_SPK_MSM_TST_3                             0x0007438
#define CS35L42_SPK_MSM_TST_4                             0x000743C
#define CS35L42_SPK_MSM_TST_5                             0x0007440
#define CS35L42_SPK_MSM_TST_6                             0x0007444
#define CS35L42_SPK_MSM_TST_7                             0x0007448
#define CS35L42_SPK_MSM_TST_8                             0x000744C
#define CS35L42_SPK_MSM_TST_9                             0x0007450
#define CS35L42_SPK_MSM_TST_10                            0x0007454
#define CS35L42_SPK_MSM_TST_11                            0x0007458
#define CS35L42_SPK_MSM_TST_12                            0x000745C
#define CS35L42_SPK_MSM_TST_13                            0x0007460
#define CS35L42_SPK_MSM_TST_14                            0x0007464
#define CS35L42_SPK_MSM_TST_15                            0x0007468
#define CS35L42_TST_DCIN                                  0x000749C

/************************************************/
/* DAC_MSM_ALIVE_DCIN_WD                        */
/************************************************/
#define CS35L42_WD_MODE                                   0x00000C00  /* WD_MODE - [11:10] */
#define CS35L42_WD_MODE_MASK                              0x00000C00  /* WD_MODE - [11:10] */
#define CS35L42_WD_MODE_SHIFT                             10          /* WD_MODE - [11:10] */
#define CS35L42_WD_MODE_WIDTH                             2           /* WD_MODE - [11:10] */
#define CS35L42_DCIN_WD_DUR                               0x00000380  /* DCIN_WD_DUR - [9:7] */
#define CS35L42_DCIN_WD_DUR_MASK                          0x00000380  /* DCIN_WD_DUR - [9:7] */
#define CS35L42_DCIN_WD_DUR_SHIFT                         7           /* DCIN_WD_DUR - [9:7] */
#define CS35L42_DCIN_WD_DUR_WIDTH                         3           /* DCIN_WD_DUR - [9:7] */
#define CS35L42_DCIN_WD_THLD                              0x0000007E  /* DCIN_WD_THLD - [6:1] */
#define CS35L42_DCIN_WD_THLD_MASK                         0x0000007E  /* DCIN_WD_THLD - [6:1] */
#define CS35L42_DCIN_WD_THLD_SHIFT                        1           /* DCIN_WD_THLD - [6:1] */
#define CS35L42_DCIN_WD_THLD_WIDTH                        6           /* DCIN_WD_THLD - [6:1] */
#define CS35L42_DCIN_WD_EN                                0x00000001  /* DCIN_WD_EN - [0] */
#define CS35L42_DCIN_WD_EN_MASK                           0x00000001  /* DCIN_WD_EN - [0] */
#define CS35L42_DCIN_WD_EN_SHIFT                          0           /* DCIN_WD_EN - [0] */
#define CS35L42_DCIN_WD_EN_WIDTH                          1           /* DCIN_WD_EN - [0] */

/************************************************/
/* SPK_MAGCOMP                                  */
/************************************************/
#define CS35L42_MAGCOMP_CONFIG                            0x0007800
#define CS35L42_MAGCOMP_FILTER_SELECT                     0x0007848
#define CS35L42_MAGCOMP_COEF_B0                           0x0007850
#define CS35L42_MAGCOMP_COEF_B1                           0x0007854
#define CS35L42_MAGCOMP_COEF_B2                           0x0007858
#define CS35L42_MAGCOMP_COEF_B3                           0x000785C
#define CS35L42_MAGCOMP_COEF_NEGA1                        0x0007860
#define CS35L42_MAGCOMP_COEF_NEGA2                        0x0007864
#define CS35L42_MAGCOMP_COEF_NEGA3                        0x0007868

/************************************************/
/* DAC_SRC                                      */
/************************************************/
#define CS35L42_OVERRIDES                                 0x0007C40
#define CS35L42_UP_RATIO_TEST                             0x0007C44
#define CS35L42_TFSO1_TEST                                0x0007C48
#define CS35L42_TFSO2_TEST                                0x0007C4C
#define CS35L42_TFSO1_PLUS_TFSO2_TEST                     0x0007C50
#define CS35L42_TFSO2_SQ_BY_2_TEST                        0x0007C54
#define CS35L42_T_INCREMENT_TEST                          0x0007C58
#define CS35L42_SRC_NUM_TEST                              0x0007C5C

/************************************************/
/* Trim Register Bank                           */
/************************************************/
#define CS35L42_T_DIGPWM_LOOP_CFG5                        0x0009200
#define CS35L42_T_DIGPWM_LOOP_CFG6                        0x0009204
#define CS35L42_T_DIGPWM_LOOP_CFG7                        0x0009208
#define CS35L42_T_TEST_REF_REGS                           0x000920C
#define CS35L42_T_TEST_ANA_TRIM2                          0x0009210
#define CS35L42_T_SPKMON_OTP_VALUES1                      0x0009214
#define CS35L42_T_SPKMON_OTP_VALUES2                      0x0009218
#define CS35L42_T_SPKMON_OTP_VALUES3                      0x000921C
#define CS35L42_T_SPKMON_OTP_VALUES4                      0x0009220
#define CS35L42_T_SPKMON_ADC_VIMON_ANA_TEST_CNTL_TESTBITS 0x0009224
#define CS35L42_T_CALIBR_OTP_CONFIGURATIONS               0x0009228
#define CS35L42_T_SAR_VP_VBST_OFFSET_GAIN_VALUE           0x000922C
#define CS35L42_T_SAR_ANA_TRIM                            0x0009230
#define CS35L42_T_BST_LEST_OFFSET_TRIM                    0x0009234
#define CS35L42_T_TST_SPK_4_TRIM                          0x0009238
#define CS35L42_T_SPK_MSM_TST_2                           0x000923C
#define CS35L42_T_DREGAINERRCOEFF0                        0x0009240
#define CS35L42_T_DREGAINERRCOEFF1                        0x0009244
#define CS35L42_T_DREGAINERRCOEFF2                        0x0009248
#define CS35L42_T_DREGAINERRCOEFF3                        0x000924C
#define CS35L42_T_DREANAOFFSET0                           0x0009250
#define CS35L42_T_DREANAOFFSET1                           0x0009254
#define CS35L42_T_DREANAOFFSET2                           0x0009258
#define CS35L42_T_DREANAOFFSET3                           0x000925C
#define CS35L42_T_DREANAOFFSET4                           0x0009260
#define CS35L42_T_DREANAOFFSET5                           0x0009264
#define CS35L42_T_DREANAOFFSET6                           0x0009268
#define CS35L42_T_DREANAOFFSET7                           0x000926C
#define CS35L42_T_PLL_FREQ                                0x0009270
#define CS35L42_T_TEST_ANA_TRIM1                          0x0009274
#define CS35L42_T_TEST_ANA_TRIM3                          0x0009278
#define CS35L42_T_TEST_ANA_TRIM4                          0x000927C
#define CS35L42_T_SPKMON_OTP_VALUES5                      0x0009280
#define CS35L42_T_MASK_DSP_EN                             0x0009350

/************************************************/
/* Interrupt Status and Mask Control            */
/************************************************/
#define CS35L42_IRQ1_CFG                                  0x0010000
#define CS35L42_IRQ1_STATUS                               0x0010004
#define CS35L42_IRQ1_EINT_1                               0x0010010
#define CS35L42_IRQ1_EINT_2                               0x0010014
#define CS35L42_IRQ1_EINT_3                               0x0010018
#define CS35L42_IRQ1_EINT_4                               0x001001C
#define CS35L42_IRQ1_EINT_5                               0x0010020
#define CS35L42_IRQ1_STS_1                                0x0010090
#define CS35L42_IRQ1_STS_2                                0x0010094
#define CS35L42_IRQ1_STS_3                                0x0010098
#define CS35L42_IRQ1_STS_4                                0x001009C
#define CS35L42_IRQ1_STS_5                                0x00100A0
#define CS35L42_IRQ1_MASK_1                               0x0010110
#define CS35L42_IRQ1_MASK_2                               0x0010114
#define CS35L42_IRQ1_MASK_3                               0x0010118
#define CS35L42_IRQ1_MASK_4                               0x001011C
#define CS35L42_IRQ1_MASK_5                               0x0010120
#define CS35L42_IRQ1_FRC_1                                0x0010190
#define CS35L42_IRQ1_FRC_2                                0x0010194
#define CS35L42_IRQ1_FRC_3                                0x0010198
#define CS35L42_IRQ1_FRC_4                                0x001019C
#define CS35L42_IRQ1_FRC_5                                0x00101A0
#define CS35L42_IRQ1_EDGE_1                               0x0010210
#define CS35L42_IRQ1_POL_1                                0x0010290
#define CS35L42_IRQ1_POL_2                                0x0010294
#define CS35L42_IRQ1_POL_3                                0x0010298
#define CS35L42_IRQ1_POL_5                                0x00102A0
#define CS35L42_IRQ1_DB_2                                 0x0010314

/************************************************/
/* IRQ1_IRQ1_EINT_1                             */
/************************************************/
#define CS35L42_MSM_PUP_DONE_EINT1                        0x00020000  /* MSM_PUP_DONE_EINT1 - [17] */
#define CS35L42_MSM_PUP_DONE_EINT1_MASK                   0x00020000  /* MSM_PUP_DONE_EINT1 - [17] */
#define CS35L42_MSM_PUP_DONE_EINT1_SHIFT                  17          /* MSM_PUP_DONE_EINT1 - [17] */
#define CS35L42_MSM_PUP_DONE_EINT1_WIDTH                  1           /* MSM_PUP_DONE_EINT1 - [17] */
#define CS35L42_MSM_PDN_DONE_EINT1                        0x00010000  /* MSM_PDN_DONE_EINT1 - [16] */
#define CS35L42_MSM_PDN_DONE_EINT1_MASK                   0x00010000  /* MSM_PDN_DONE_EINT1 - [16] */
#define CS35L42_MSM_PDN_DONE_EINT1_SHIFT                  16          /* MSM_PDN_DONE_EINT1 - [16] */
#define CS35L42_MSM_PDN_DONE_EINT1_WIDTH                  1           /* MSM_PDN_DONE_EINT1 - [16] */


/************************************************/
/* IRQ1_IRQ1_MASK_1                             */
/************************************************/
#define CS35L42_DSP_VIRTUAL2_MBOX_WR_MASK1                0x80000000  /* DSP_VIRTUAL2_MBOX_WR_MASK1 - [31] */
#define CS35L42_DSP_VIRTUAL2_MBOX_WR_MASK1_MASK           0x80000000  /* DSP_VIRTUAL2_MBOX_WR_MASK1 - [31] */
#define CS35L42_DSP_VIRTUAL2_MBOX_WR_MASK1_SHIFT          31          /* DSP_VIRTUAL2_MBOX_WR_MASK1 - [31] */
#define CS35L42_DSP_VIRTUAL2_MBOX_WR_MASK1_WIDTH          1           /* DSP_VIRTUAL2_MBOX_WR_MASK1 - [31] */
#define CS35L42_DSP_VIRTUAL1_MBOX_WR_MASK1                0x40000000  /* DSP_VIRTUAL1_MBOX_WR_MASK1 - [30] */
#define CS35L42_DSP_VIRTUAL1_MBOX_WR_MASK1_MASK           0x40000000  /* DSP_VIRTUAL1_MBOX_WR_MASK1 - [30] */
#define CS35L42_DSP_VIRTUAL1_MBOX_WR_MASK1_SHIFT          30          /* DSP_VIRTUAL1_MBOX_WR_MASK1 - [30] */
#define CS35L42_DSP_VIRTUAL1_MBOX_WR_MASK1_WIDTH          1           /* DSP_VIRTUAL1_MBOX_WR_MASK1 - [30] */
#define CS35L42_DC_WATCHDOG_IRQ_FALL_MASK1                0x20000000  /* DC_WATCHDOG_IRQ_FALL_MASK1 - [29] */
#define CS35L42_DC_WATCHDOG_IRQ_FALL_MASK1_MASK           0x20000000  /* DC_WATCHDOG_IRQ_FALL_MASK1 - [29] */
#define CS35L42_DC_WATCHDOG_IRQ_FALL_MASK1_SHIFT          29          /* DC_WATCHDOG_IRQ_FALL_MASK1 - [29] */
#define CS35L42_DC_WATCHDOG_IRQ_FALL_MASK1_WIDTH          1           /* DC_WATCHDOG_IRQ_FALL_MASK1 - [29] */
#define CS35L42_DC_WATCHDOG_IRQ_RISE_MASK1                0x10000000  /* DC_WATCHDOG_IRQ_RISE_MASK1 - [28] */
#define CS35L42_DC_WATCHDOG_IRQ_RISE_MASK1_MASK           0x10000000  /* DC_WATCHDOG_IRQ_RISE_MASK1 - [28] */
#define CS35L42_DC_WATCHDOG_IRQ_RISE_MASK1_SHIFT          28          /* DC_WATCHDOG_IRQ_RISE_MASK1 - [28] */
#define CS35L42_DC_WATCHDOG_IRQ_RISE_MASK1_WIDTH          1           /* DC_WATCHDOG_IRQ_RISE_MASK1 - [28] */
#define CS35L42_AMP_ERR_MASK1                             0x08000000  /* AMP_ERR_MASK1 - [27] */
#define CS35L42_AMP_ERR_MASK1_MASK                        0x08000000  /* AMP_ERR_MASK1 - [27] */
#define CS35L42_AMP_ERR_MASK1_SHIFT                       27          /* AMP_ERR_MASK1 - [27] */
#define CS35L42_AMP_ERR_MASK1_WIDTH                       1           /* AMP_ERR_MASK1 - [27] */
#define CS35L42_TEMP_ERR_MASK1                            0x04000000  /* TEMP_ERR_MASK1 - [26] */
#define CS35L42_TEMP_ERR_MASK1_MASK                       0x04000000  /* TEMP_ERR_MASK1 - [26] */
#define CS35L42_TEMP_ERR_MASK1_SHIFT                      26          /* TEMP_ERR_MASK1 - [26] */
#define CS35L42_TEMP_ERR_MASK1_WIDTH                      1           /* TEMP_ERR_MASK1 - [26] */
#define CS35L42_TEMP_WARN_FALL_MASK1                      0x02000000  /* TEMP_WARN_FALL_MASK1 - [25] */
#define CS35L42_TEMP_WARN_FALL_MASK1_MASK                 0x02000000  /* TEMP_WARN_FALL_MASK1 - [25] */
#define CS35L42_TEMP_WARN_FALL_MASK1_SHIFT                25          /* TEMP_WARN_FALL_MASK1 - [25] */
#define CS35L42_TEMP_WARN_FALL_MASK1_WIDTH                1           /* TEMP_WARN_FALL_MASK1 - [25] */
#define CS35L42_TEMP_WARN_RISE_MASK1                      0x01000000  /* TEMP_WARN_RISE_MASK1 - [24] */
#define CS35L42_TEMP_WARN_RISE_MASK1_MASK                 0x01000000  /* TEMP_WARN_RISE_MASK1 - [24] */
#define CS35L42_TEMP_WARN_RISE_MASK1_SHIFT                24          /* TEMP_WARN_RISE_MASK1 - [24] */
#define CS35L42_TEMP_WARN_RISE_MASK1_WIDTH                1           /* TEMP_WARN_RISE_MASK1 - [24] */
#define CS35L42_BST_IPK_FLAG_MASK1                        0x00800000  /* BST_IPK_FLAG_MASK1 - [23] */
#define CS35L42_BST_IPK_FLAG_MASK1_MASK                   0x00800000  /* BST_IPK_FLAG_MASK1 - [23] */
#define CS35L42_BST_IPK_FLAG_MASK1_SHIFT                  23          /* BST_IPK_FLAG_MASK1 - [23] */
#define CS35L42_BST_IPK_FLAG_MASK1_WIDTH                  1           /* BST_IPK_FLAG_MASK1 - [23] */
#define CS35L42_BST_SHORT_ERR_MASK1                       0x00400000  /* BST_SHORT_ERR_MASK1 - [22] */
#define CS35L42_BST_SHORT_ERR_MASK1_MASK                  0x00400000  /* BST_SHORT_ERR_MASK1 - [22] */
#define CS35L42_BST_SHORT_ERR_MASK1_SHIFT                 22          /* BST_SHORT_ERR_MASK1 - [22] */
#define CS35L42_BST_SHORT_ERR_MASK1_WIDTH                 1           /* BST_SHORT_ERR_MASK1 - [22] */
#define CS35L42_BST_DCM_UVP_ERR_MASK1                     0x00200000  /* BST_DCM_UVP_ERR_MASK1 - [21] */
#define CS35L42_BST_DCM_UVP_ERR_MASK1_MASK                0x00200000  /* BST_DCM_UVP_ERR_MASK1 - [21] */
#define CS35L42_BST_DCM_UVP_ERR_MASK1_SHIFT               21          /* BST_DCM_UVP_ERR_MASK1 - [21] */
#define CS35L42_BST_DCM_UVP_ERR_MASK1_WIDTH               1           /* BST_DCM_UVP_ERR_MASK1 - [21] */
#define CS35L42_BST_OVP_ERR_MASK1                         0x00100000  /* BST_OVP_ERR_MASK1 - [20] */
#define CS35L42_BST_OVP_ERR_MASK1_MASK                    0x00100000  /* BST_OVP_ERR_MASK1 - [20] */
#define CS35L42_BST_OVP_ERR_MASK1_SHIFT                   20          /* BST_OVP_ERR_MASK1 - [20] */
#define CS35L42_BST_OVP_ERR_MASK1_WIDTH                   1           /* BST_OVP_ERR_MASK1 - [20] */
#define CS35L42_BST_OVP_FLAG_FALL_MASK1                   0x00080000  /* BST_OVP_FLAG_FALL_MASK1 - [19] */
#define CS35L42_BST_OVP_FLAG_FALL_MASK1_MASK              0x00080000  /* BST_OVP_FLAG_FALL_MASK1 - [19] */
#define CS35L42_BST_OVP_FLAG_FALL_MASK1_SHIFT             19          /* BST_OVP_FLAG_FALL_MASK1 - [19] */
#define CS35L42_BST_OVP_FLAG_FALL_MASK1_WIDTH             1           /* BST_OVP_FLAG_FALL_MASK1 - [19] */
#define CS35L42_BST_OVP_FLAG_RISE_MASK1                   0x00040000  /* BST_OVP_FLAG_RISE_MASK1 - [18] */
#define CS35L42_BST_OVP_FLAG_RISE_MASK1_MASK              0x00040000  /* BST_OVP_FLAG_RISE_MASK1 - [18] */
#define CS35L42_BST_OVP_FLAG_RISE_MASK1_SHIFT             18          /* BST_OVP_FLAG_RISE_MASK1 - [18] */
#define CS35L42_BST_OVP_FLAG_RISE_MASK1_WIDTH             1           /* BST_OVP_FLAG_RISE_MASK1 - [18] */
#define CS35L42_MSM_PUP_DONE_MASK1                        0x00020000  /* MSM_PUP_DONE_MASK1 - [17] */
#define CS35L42_MSM_PUP_DONE_MASK1_MASK                   0x00020000  /* MSM_PUP_DONE_MASK1 - [17] */
#define CS35L42_MSM_PUP_DONE_MASK1_SHIFT                  17          /* MSM_PUP_DONE_MASK1 - [17] */
#define CS35L42_MSM_PUP_DONE_MASK1_WIDTH                  1           /* MSM_PUP_DONE_MASK1 - [17] */
#define CS35L42_MSM_PDN_DONE_MASK1                        0x00010000  /* MSM_PDN_DONE_MASK1 - [16] */
#define CS35L42_MSM_PDN_DONE_MASK1_MASK                   0x00010000  /* MSM_PDN_DONE_MASK1 - [16] */
#define CS35L42_MSM_PDN_DONE_MASK1_SHIFT                  16          /* MSM_PDN_DONE_MASK1 - [16] */
#define CS35L42_MSM_PDN_DONE_MASK1_WIDTH                  1           /* MSM_PDN_DONE_MASK1 - [16] */
#define CS35L42_MSM_GLOBAL_EN_ASSERT_MASK1                0x00008000  /* MSM_GLOBAL_EN_ASSERT_MASK1 - [15] */
#define CS35L42_MSM_GLOBAL_EN_ASSERT_MASK1_MASK           0x00008000  /* MSM_GLOBAL_EN_ASSERT_MASK1 - [15] */
#define CS35L42_MSM_GLOBAL_EN_ASSERT_MASK1_SHIFT          15          /* MSM_GLOBAL_EN_ASSERT_MASK1 - [15] */
#define CS35L42_MSM_GLOBAL_EN_ASSERT_MASK1_WIDTH          1           /* MSM_GLOBAL_EN_ASSERT_MASK1 - [15] */
#define CS35L42_WKSRC_STATUS6_MASK1                       0x00004000  /* WKSRC_STATUS6_MASK1 - [14] */
#define CS35L42_WKSRC_STATUS6_MASK1_MASK                  0x00004000  /* WKSRC_STATUS6_MASK1 - [14] */
#define CS35L42_WKSRC_STATUS6_MASK1_SHIFT                 14          /* WKSRC_STATUS6_MASK1 - [14] */
#define CS35L42_WKSRC_STATUS6_MASK1_WIDTH                 1           /* WKSRC_STATUS6_MASK1 - [14] */
#define CS35L42_WKSRC_STATUS5_MASK1                       0x00002000  /* WKSRC_STATUS5_MASK1 - [13] */
#define CS35L42_WKSRC_STATUS5_MASK1_MASK                  0x00002000  /* WKSRC_STATUS5_MASK1 - [13] */
#define CS35L42_WKSRC_STATUS5_MASK1_SHIFT                 13          /* WKSRC_STATUS5_MASK1 - [13] */
#define CS35L42_WKSRC_STATUS5_MASK1_WIDTH                 1           /* WKSRC_STATUS5_MASK1 - [13] */
#define CS35L42_WKSRC_STATUS4_MASK1                       0x00001000  /* WKSRC_STATUS4_MASK1 - [12] */
#define CS35L42_WKSRC_STATUS4_MASK1_MASK                  0x00001000  /* WKSRC_STATUS4_MASK1 - [12] */
#define CS35L42_WKSRC_STATUS4_MASK1_SHIFT                 12          /* WKSRC_STATUS4_MASK1 - [12] */
#define CS35L42_WKSRC_STATUS4_MASK1_WIDTH                 1           /* WKSRC_STATUS4_MASK1 - [12] */
#define CS35L42_WKSRC_STATUS3_MASK1                       0x00000800  /* WKSRC_STATUS3_MASK1 - [11] */
#define CS35L42_WKSRC_STATUS3_MASK1_MASK                  0x00000800  /* WKSRC_STATUS3_MASK1 - [11] */
#define CS35L42_WKSRC_STATUS3_MASK1_SHIFT                 11          /* WKSRC_STATUS3_MASK1 - [11] */
#define CS35L42_WKSRC_STATUS3_MASK1_WIDTH                 1           /* WKSRC_STATUS3_MASK1 - [11] */
#define CS35L42_WKSRC_STATUS2_MASK1                       0x00000400  /* WKSRC_STATUS2_MASK1 - [10] */
#define CS35L42_WKSRC_STATUS2_MASK1_MASK                  0x00000400  /* WKSRC_STATUS2_MASK1 - [10] */
#define CS35L42_WKSRC_STATUS2_MASK1_SHIFT                 10          /* WKSRC_STATUS2_MASK1 - [10] */
#define CS35L42_WKSRC_STATUS2_MASK1_WIDTH                 1           /* WKSRC_STATUS2_MASK1 - [10] */
#define CS35L42_WKSRC_STATUS1_MASK1                       0x00000200  /* WKSRC_STATUS1_MASK1 - [9] */
#define CS35L42_WKSRC_STATUS1_MASK1_MASK                  0x00000200  /* WKSRC_STATUS1_MASK1 - [9] */
#define CS35L42_WKSRC_STATUS1_MASK1_SHIFT                 9           /* WKSRC_STATUS1_MASK1 - [9] */
#define CS35L42_WKSRC_STATUS1_MASK1_WIDTH                 1           /* WKSRC_STATUS1_MASK1 - [9] */
#define CS35L42_WKSRC_STATUS_ANY_MASK1                    0x00000100  /* WKSRC_STATUS_ANY_MASK1 - [8] */
#define CS35L42_WKSRC_STATUS_ANY_MASK1_MASK               0x00000100  /* WKSRC_STATUS_ANY_MASK1 - [8] */
#define CS35L42_WKSRC_STATUS_ANY_MASK1_SHIFT              8           /* WKSRC_STATUS_ANY_MASK1 - [8] */
#define CS35L42_WKSRC_STATUS_ANY_MASK1_WIDTH              1           /* WKSRC_STATUS_ANY_MASK1 - [8] */
#define CS35L42_GPIO4_FALL_MASK1                          0x00000080  /* GPIO4_FALL_MASK1 - [7] */
#define CS35L42_GPIO4_FALL_MASK1_MASK                     0x00000080  /* GPIO4_FALL_MASK1 - [7] */
#define CS35L42_GPIO4_FALL_MASK1_SHIFT                    7           /* GPIO4_FALL_MASK1 - [7] */
#define CS35L42_GPIO4_FALL_MASK1_WIDTH                    1           /* GPIO4_FALL_MASK1 - [7] */
#define CS35L42_GPIO4_RISE_MASK1                          0x00000040  /* GPIO4_RISE_MASK1 - [6] */
#define CS35L42_GPIO4_RISE_MASK1_MASK                     0x00000040  /* GPIO4_RISE_MASK1 - [6] */
#define CS35L42_GPIO4_RISE_MASK1_SHIFT                    6           /* GPIO4_RISE_MASK1 - [6] */
#define CS35L42_GPIO4_RISE_MASK1_WIDTH                    1           /* GPIO4_RISE_MASK1 - [6] */
#define CS35L42_GPIO3_FALL_MASK1                          0x00000020  /* GPIO3_FALL_MASK1 - [5] */
#define CS35L42_GPIO3_FALL_MASK1_MASK                     0x00000020  /* GPIO3_FALL_MASK1 - [5] */
#define CS35L42_GPIO3_FALL_MASK1_SHIFT                    5           /* GPIO3_FALL_MASK1 - [5] */
#define CS35L42_GPIO3_FALL_MASK1_WIDTH                    1           /* GPIO3_FALL_MASK1 - [5] */
#define CS35L42_GPIO3_RISE_MASK1                          0x00000010  /* GPIO3_RISE_MASK1 - [4] */
#define CS35L42_GPIO3_RISE_MASK1_MASK                     0x00000010  /* GPIO3_RISE_MASK1 - [4] */
#define CS35L42_GPIO3_RISE_MASK1_SHIFT                    4           /* GPIO3_RISE_MASK1 - [4] */
#define CS35L42_GPIO3_RISE_MASK1_WIDTH                    1           /* GPIO3_RISE_MASK1 - [4] */
#define CS35L42_GPIO2_FALL_MASK1                          0x00000008  /* GPIO2_FALL_MASK1 - [3] */
#define CS35L42_GPIO2_FALL_MASK1_MASK                     0x00000008  /* GPIO2_FALL_MASK1 - [3] */
#define CS35L42_GPIO2_FALL_MASK1_SHIFT                    3           /* GPIO2_FALL_MASK1 - [3] */
#define CS35L42_GPIO2_FALL_MASK1_WIDTH                    1           /* GPIO2_FALL_MASK1 - [3] */
#define CS35L42_GPIO2_RISE_MASK1                          0x00000004  /* GPIO2_RISE_MASK1 - [2] */
#define CS35L42_GPIO2_RISE_MASK1_MASK                     0x00000004  /* GPIO2_RISE_MASK1 - [2] */
#define CS35L42_GPIO2_RISE_MASK1_SHIFT                    2           /* GPIO2_RISE_MASK1 - [2] */
#define CS35L42_GPIO2_RISE_MASK1_WIDTH                    1           /* GPIO2_RISE_MASK1 - [2] */
#define CS35L42_GPIO1_FALL_MASK1                          0x00000002  /* GPIO1_FALL_MASK1 - [1] */
#define CS35L42_GPIO1_FALL_MASK1_MASK                     0x00000002  /* GPIO1_FALL_MASK1 - [1] */
#define CS35L42_GPIO1_FALL_MASK1_SHIFT                    1           /* GPIO1_FALL_MASK1 - [1] */
#define CS35L42_GPIO1_FALL_MASK1_WIDTH                    1           /* GPIO1_FALL_MASK1 - [1] */
#define CS35L42_GPIO1_RISE_MASK1                          0x00000001  /* GPIO1_RISE_MASK1 - [0] */
#define CS35L42_GPIO1_RISE_MASK1_MASK                     0x00000001  /* GPIO1_RISE_MASK1 - [0] */
#define CS35L42_GPIO1_RISE_MASK1_SHIFT                    0           /* GPIO1_RISE_MASK1 - [0] */
#define CS35L42_GPIO1_RISE_MASK1_WIDTH                    1           /* GPIO1_RISE_MASK1 - [0] */

/************************************************/
/* GPIO Control                                 */
/************************************************/
#define CS35L42_GPIO_STATUS1                              0x0011000
#define CS35L42_GPIO_FORCE                                0x0011004
#define CS35L42_GPIO1_CTRL1                               0x0011008
#define CS35L42_GPIO2_CTRL1                               0x001100C
#define CS35L42_GPIO3_CTRL1                               0x0011010
#define CS35L42_GPIO4_CTRL1                               0x0011014

/************************************************/
/* DSP Noise Gate Control                       */
/************************************************/
#define CS35L42_MIXER_NGATE_CFG                           0x0012000
#define CS35L42_MIXER_NGATE_CH1_CFG                       0x0012004
#define CS35L42_MIXER_NGATE_CH2_CFG                       0x0012008

/************************************************/
/* DSP Scratch Space                            */
/************************************************/
#define CS35L42_DSP_MBOX_1                                0x0013000
#define CS35L42_DSP_MBOX_2                                0x0013004
#define CS35L42_DSP_MBOX_3                                0x0013008
#define CS35L42_DSP_MBOX_4                                0x001300C
#define CS35L42_DSP_MBOX_5                                0x0013010
#define CS35L42_DSP_MBOX_6                                0x0013014
#define CS35L42_DSP_MBOX_7                                0x0013018
#define CS35L42_DSP_MBOX_8                                0x001301C

/************************************************/
/* DSP Virtual 1 Scratch Space                  */
/************************************************/
#define CS35L42_DSP_VIRTUAL1_MBOX_1                       0x0013020
#define CS35L42_DSP_VIRTUAL1_MBOX_2                       0x0013024
#define CS35L42_DSP_VIRTUAL1_MBOX_3                       0x0013028
#define CS35L42_DSP_VIRTUAL1_MBOX_4                       0x001302C
#define CS35L42_DSP_VIRTUAL1_MBOX_5                       0x0013030
#define CS35L42_DSP_VIRTUAL1_MBOX_6                       0x0013034
#define CS35L42_DSP_VIRTUAL1_MBOX_7                       0x0013038
#define CS35L42_DSP_VIRTUAL1_MBOX_8                       0x001303C

/************************************************/
/* DSP Virtual 2 Scratch Space                  */
/************************************************/
#define CS35L42_DSP_VIRTUAL2_MBOX_1                       0x0013040
#define CS35L42_DSP_VIRTUAL2_MBOX_2                       0x0013044
#define CS35L42_DSP_VIRTUAL2_MBOX_3                       0x0013048
#define CS35L42_DSP_VIRTUAL2_MBOX_4                       0x001304C
#define CS35L42_DSP_VIRTUAL2_MBOX_5                       0x0013050
#define CS35L42_DSP_VIRTUAL2_MBOX_6                       0x0013054
#define CS35L42_DSP_VIRTUAL2_MBOX_7                       0x0013058
#define CS35L42_DSP_VIRTUAL2_MBOX_8                       0x001305C

/************************************************/
/* DSP Timer                                    */
/************************************************/
#define CS35L42_TIMER1_CONTROL                            0x0015000
#define CS35L42_TIMER1_COUNT_PRESET                       0x0015004
#define CS35L42_TIMER1_START_AND_STOP                     0x001500C
#define CS35L42_TIMER1_STATUS                             0x0015010
#define CS35L42_TIMER1_COUNT_READBACK                     0x0015014
#define CS35L42_TIMER1_DSP_CLOCK_CONFIG                   0x0015018
#define CS35L42_TIMER1_DSP_CLOCK_STATUS                   0x001501C
#define CS35L42_TIMER2_CONTROL                            0x0015100
#define CS35L42_TIMER2_COUNT_PRESET                       0x0015104
#define CS35L42_TIMER2_START_AND_STOP                     0x001510C
#define CS35L42_TIMER2_STATUS                             0x0015110
#define CS35L42_TIMER2_COUNT_READBACK                     0x0015114
#define CS35L42_TIMER2_DSP_CLOCK_CONFIG                   0x0015118
#define CS35L42_TIMER2_DSP_CLOCK_STATUS                   0x001511C

/************************************************/
/* DFT Control                                  */
/************************************************/
#define CS35L42_DFT_JTAG_CTRL                             0x0016000

/************************************************/
/* OTP ID Readback                              */
/************************************************/
#define CS35L42_TEMP_CAL2                                 0x001704C
#define CS35L42_MEM_TEST                                  0x0017050

/************************************************/
/* OTP_IF_MEM                                   */
/************************************************/
#define CS35L42_OTP_MEM0                                  0x0030000
#define CS35L42_OTP_MEM1                                  0x0030004
#define CS35L42_OTP_MEM2                                  0x0030008
#define CS35L42_OTP_MEM3                                  0x003000C
#define CS35L42_OTP_MEM4                                  0x0030010
#define CS35L42_OTP_MEM5                                  0x0030014
#define CS35L42_OTP_MEM6                                  0x0030018
#define CS35L42_OTP_MEM7                                  0x003001C
#define CS35L42_OTP_MEM8                                  0x0030020
#define CS35L42_OTP_MEM9                                  0x0030024
#define CS35L42_OTP_MEM10                                 0x0030028
#define CS35L42_OTP_MEM11                                 0x003002C
#define CS35L42_OTP_MEM12                                 0x0030030
#define CS35L42_OTP_MEM13                                 0x0030034
#define CS35L42_OTP_MEM14                                 0x0030038
#define CS35L42_OTP_MEM15                                 0x003003C
#define CS35L42_OTP_MEM16                                 0x0030040
#define CS35L42_OTP_MEM17                                 0x0030044
#define CS35L42_OTP_MEM18                                 0x0030048
#define CS35L42_OTP_MEM19                                 0x003004C
#define CS35L42_OTP_MEM20                                 0x0030050
#define CS35L42_OTP_MEM21                                 0x0030054
#define CS35L42_OTP_MEM22                                 0x0030058
#define CS35L42_OTP_MEM23                                 0x003005C
#define CS35L42_OTP_MEM24                                 0x0030060
#define CS35L42_OTP_MEM25                                 0x0030064
#define CS35L42_OTP_MEM26                                 0x0030068
#define CS35L42_OTP_MEM27                                 0x003006C
#define CS35L42_OTP_MEM28                                 0x0030070
#define CS35L42_OTP_MEM29                                 0x0030074
#define CS35L42_OTP_MEM30                                 0x0030078
#define CS35L42_OTP_MEM31                                 0x003007C

/************************************************/
/* DSP1 X Memory Range Packed                   */
/************************************************/
#define CS35L42_DSP1_XMEM_PACKED_0                        0x2000000
#define CS35L42_DSP1_XMEM_PACKED_1                        0x2000004
#define CS35L42_DSP1_XMEM_PACKED_2                        0x2000008
#define CS35L42_DSP1_XMEM_PACKED_6141                     0x2005FF4
#define CS35L42_DSP1_XMEM_PACKED_6142                     0x2005FF8
#define CS35L42_DSP1_XMEM_PACKED_6143                     0x2005FFC
#define CS35L42_DSP1_XROM_PACKED_0                        0x2006000
#define CS35L42_DSP1_XROM_PACKED_1                        0x2006004
#define CS35L42_DSP1_XROM_PACKED_2                        0x2006008
#define CS35L42_DSP1_XROM_PACKED_4602                     0x200A7E8
#define CS35L42_DSP1_XROM_PACKED_4603                     0x200A7EC
#define CS35L42_DSP1_XROM_PACKED_4604                     0x200A7F0

/************************************************/
/* Combined DSP1 X Memory and Register Space Unpacked 32-bit View */
/************************************************/
#define CS35L42_DSP1_XMEM_UNPACKED32_0                    0x2400000
#define CS35L42_DSP1_XMEM_UNPACKED32_1                    0x2400004
#define CS35L42_DSP1_XMEM_UNPACKED32_4094                 0x2403FF8
#define CS35L42_DSP1_XMEM_UNPACKED32_4095                 0x2403FFC
#define CS35L42_DSP1_XROM_UNPACKED32_0                    0x2404000
#define CS35L42_DSP1_XROM_UNPACKED32_1                    0x2404004
#define CS35L42_DSP1_XROM_UNPACKED32_3069                 0x2406FF4
#define CS35L42_DSP1_XROM_UNPACKED32_3070                 0x2406FF8
#define CS35L42_DSP1_TIMESTAMP_COUNT                      0x25C0800
#define CS35L42_DSP1_SYS_INFO_ID                          0x25E0000
#define CS35L42_DSP1_SYS_INFO_VERSION                     0x25E0004
#define CS35L42_DSP1_SYS_INFO_CORE_ID                     0x25E0008
#define CS35L42_DSP1_SYS_INFO_AHB_ADDR                    0x25E000C
#define CS35L42_DSP1_SYS_INFO_XM_SRAM_SIZE                0x25E0010
#define CS35L42_DSP1_SYS_INFO_XM_ROM_SIZE                 0x25E0014
#define CS35L42_DSP1_SYS_INFO_YM_SRAM_SIZE                0x25E0018
#define CS35L42_DSP1_SYS_INFO_YM_ROM_SIZE                 0x25E001C
#define CS35L42_DSP1_SYS_INFO_PM_SRAM_SIZE                0x25E0020
#define CS35L42_DSP1_SYS_INFO_PM_BOOT_SIZE                0x25E0028
#define CS35L42_DSP1_SYS_INFO_FEATURES                    0x25E002C
#define CS35L42_DSP1_SYS_INFO_FIR_FILTERS                 0x25E0030
#define CS35L42_DSP1_SYS_INFO_LMS_FILTERS                 0x25E0034
#define CS35L42_DSP1_SYS_INFO_XM_BANK_SIZE                0x25E0038
#define CS35L42_DSP1_SYS_INFO_YM_BANK_SIZE                0x25E003C
#define CS35L42_DSP1_SYS_INFO_PM_BANK_SIZE                0x25E0040
#define CS35L42_DSP1_SYS_INFO_STREAM_ARB                  0x25E0044
#define CS35L42_DSP1_SYS_INFO_XM_EMEM_SIZE                0x25E0048
#define CS35L42_DSP1_SYS_INFO_YM_EMEM_SIZE                0x25E004C
#define CS35L42_DSP1_AHBM_WINDOW0_CONTROL_0               0x25E2000
#define CS35L42_DSP1_AHBM_WINDOW0_CONTROL_1               0x25E2004
#define CS35L42_DSP1_AHBM_WINDOW1_CONTROL_0               0x25E2008
#define CS35L42_DSP1_AHBM_WINDOW1_CONTROL_1               0x25E200C
#define CS35L42_DSP1_AHBM_WINDOW2_CONTROL_0               0x25E2010
#define CS35L42_DSP1_AHBM_WINDOW2_CONTROL_1               0x25E2014
#define CS35L42_DSP1_AHBM_WINDOW3_CONTROL_0               0x25E2018
#define CS35L42_DSP1_AHBM_WINDOW3_CONTROL_1               0x25E201C
#define CS35L42_DSP1_AHBM_WINDOW4_CONTROL_0               0x25E2020
#define CS35L42_DSP1_AHBM_WINDOW4_CONTROL_1               0x25E2024
#define CS35L42_DSP1_AHBM_WINDOW5_CONTROL_0               0x25E2028
#define CS35L42_DSP1_AHBM_WINDOW5_CONTROL_1               0x25E202C
#define CS35L42_DSP1_AHBM_WINDOW6_CONTROL_0               0x25E2030
#define CS35L42_DSP1_AHBM_WINDOW6_CONTROL_1               0x25E2034
#define CS35L42_DSP1_AHBM_WINDOW7_CONTROL_0               0x25E2038
#define CS35L42_DSP1_AHBM_WINDOW7_CONTROL_1               0x25E203C
#define CS35L42_DSP1_AHBM_WINDOW_DEBUG_0                  0x25E2040
#define CS35L42_DSP1_AHBM_WINDOW_DEBUG_1                  0x25E2044

/************************************************/
/* Combined DSP1 X Memory and Register Space Unpacked 24-bit View */
/************************************************/
#define CS35L42_DSP1_XMEM_UNPACKED24_0                    0x2800000
#define CS35L42_DSP1_XMEM_UNPACKED24_1                    0x2800004
#define CS35L42_DSP1_XMEM_UNPACKED24_2                    0x2800008
#define CS35L42_DSP1_XMEM_UNPACKED24_3                    0x280000C
#define CS35L42_DSP1_XMEM_UNPACKED24_8188                 0x2807FF0
#define CS35L42_DSP1_XMEM_UNPACKED24_8189                 0x2807FF4
#define CS35L42_DSP1_XMEM_UNPACKED24_8190                 0x2807FF8
#define CS35L42_DSP1_XMEM_UNPACKED24_8191                 0x2807FFC
#define CS35L42_DSP1_XROM_UNPACKED24_0                    0x2808000
#define CS35L42_DSP1_XROM_UNPACKED24_1                    0x2808004
#define CS35L42_DSP1_XROM_UNPACKED24_2                    0x2808008
#define CS35L42_DSP1_XROM_UNPACKED24_3                    0x280800C
#define CS35L42_DSP1_XROM_UNPACKED24_6138                 0x280DFE8
#define CS35L42_DSP1_XROM_UNPACKED24_6139                 0x280DFEC
#define CS35L42_DSP1_XROM_UNPACKED24_6140                 0x280DFF0
#define CS35L42_DSP1_XROM_UNPACKED24_6141                 0x280DFF4
#define CS35L42_DSP1_CLOCK_FREQ                           0x2B80000
#define CS35L42_DSP1_CLOCK_STATUS                         0x2B80008
#define CS35L42_DSP1_CORE_SOFT_RESET                      0x2B80010
#define CS35L42_DSP1_CORE_WRAP_STATUS                     0x2B80020
#define CS35L42_DSP1_DEBUG                                0x2B80040
#define CS35L42_DSP1_TIMER_CONTROL                        0x2B80048
#define CS35L42_DSP1_STREAM_ARB_CONTROL                   0x2B80050
#define CS35L42_DSP1_NMI_CONTROL1                         0x2B80480
#define CS35L42_DSP1_NMI_CONTROL2                         0x2B80488
#define CS35L42_DSP1_NMI_CONTROL3                         0x2B80490
#define CS35L42_DSP1_NMI_CONTROL4                         0x2B80498
#define CS35L42_DSP1_NMI_CONTROL5                         0x2B804A0
#define CS35L42_DSP1_NMI_CONTROL6                         0x2B804A8
#define CS35L42_DSP1_NMI_CONTROL7                         0x2B804B0
#define CS35L42_DSP1_NMI_CONTROL8                         0x2B804B8
#define CS35L42_DSP1_RESUME_CONTROL                       0x2B80500
#define CS35L42_DSP1_IRQ1_CONTROL                         0x2B80508
#define CS35L42_DSP1_IRQ2_CONTROL                         0x2B80510
#define CS35L42_DSP1_IRQ3_CONTROL                         0x2B80518
#define CS35L42_DSP1_IRQ4_CONTROL                         0x2B80520
#define CS35L42_DSP1_IRQ5_CONTROL                         0x2B80528
#define CS35L42_DSP1_IRQ6_CONTROL                         0x2B80530
#define CS35L42_DSP1_IRQ7_CONTROL                         0x2B80538
#define CS35L42_DSP1_IRQ8_CONTROL                         0x2B80540
#define CS35L42_DSP1_IRQ9_CONTROL                         0x2B80548
#define CS35L42_DSP1_IRQ10_CONTROL                        0x2B80550
#define CS35L42_DSP1_IRQ11_CONTROL                        0x2B80558
#define CS35L42_DSP1_IRQ12_CONTROL                        0x2B80560
#define CS35L42_DSP1_IRQ13_CONTROL                        0x2B80568
#define CS35L42_DSP1_IRQ14_CONTROL                        0x2B80570
#define CS35L42_DSP1_IRQ15_CONTROL                        0x2B80578
#define CS35L42_DSP1_IRQ16_CONTROL                        0x2B80580
#define CS35L42_DSP1_IRQ17_CONTROL                        0x2B80588
#define CS35L42_DSP1_IRQ18_CONTROL                        0x2B80590
#define CS35L42_DSP1_IRQ19_CONTROL                        0x2B80598
#define CS35L42_DSP1_IRQ20_CONTROL                        0x2B805A0
#define CS35L42_DSP1_IRQ21_CONTROL                        0x2B805A8
#define CS35L42_DSP1_IRQ22_CONTROL                        0x2B805B0
#define CS35L42_DSP1_IRQ23_CONTROL                        0x2B805B8
#define CS35L42_DSP1_SCRATCH1                             0x2B805C0
#define CS35L42_DSP1_SCRATCH2                             0x2B805C8
#define CS35L42_DSP1_SCRATCH3                             0x2B805D0
#define CS35L42_DSP1_SCRATCH4                             0x2B805D8
#define CS35L42_DSP1_CCM_CORE_CONTROL                     0x2BC1000
#define CS35L42_DSP1_CCM_CLK_OVERRIDE                     0x2BC1008
#define CS35L42_DSP1_MEM_CTRL_XM_MSTR_EN                  0x2BC2000
#define CS35L42_DSP1_MEM_CTRL_XM_CORE_PRIO                0x2BC2008
#define CS35L42_DSP1_MEM_CTRL_XM_PL0_PRIO                 0x2BC2010
#define CS35L42_DSP1_MEM_CTRL_XM_PL1_PRIO                 0x2BC2018
#define CS35L42_DSP1_MEM_CTRL_XM_PL2_PRIO                 0x2BC2020
#define CS35L42_DSP1_MEM_CTRL_XM_NPL0_PRIO                0x2BC2078
#define CS35L42_DSP1_MEM_CTRL_YM_MSTR_EN                  0x2BC20C0
#define CS35L42_DSP1_MEM_CTRL_YM_CORE_PRIO                0x2BC20C8
#define CS35L42_DSP1_MEM_CTRL_YM_PL0_PRIO                 0x2BC20D0
#define CS35L42_DSP1_MEM_CTRL_YM_PL1_PRIO                 0x2BC20D8
#define CS35L42_DSP1_MEM_CTRL_YM_PL2_PRIO                 0x2BC20E0
#define CS35L42_DSP1_MEM_CTRL_YM_NPL0_PRIO                0x2BC2138
#define CS35L42_DSP1_MEM_CTRL_PM_MSTR_EN                  0x2BC2180
#define CS35L42_DSP1_MEM_CTRL_FIXED_PRIO                  0x2BC2184
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH0_ADDR              0x2BC2188
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH0_EN                0x2BC218C
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH0_DATA_LO           0x2BC2190
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH0_DATA_HI           0x2BC2194
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH1_ADDR              0x2BC2198
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH1_EN                0x2BC219C
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH1_DATA_LO           0x2BC21A0
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH1_DATA_HI           0x2BC21A4
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH2_ADDR              0x2BC21A8
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH2_EN                0x2BC21AC
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH2_DATA_LO           0x2BC21B0
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH2_DATA_HI           0x2BC21B4
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH3_ADDR              0x2BC21B8
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH3_EN                0x2BC21BC
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH3_DATA_LO           0x2BC21C0
#define CS35L42_DSP1_MEM_CTRL_PM_PATCH3_DATA_HI           0x2BC21C4
#define CS35L42_DSP1_MPU_XMEM_ACCESS_0                    0x2BC3000
#define CS35L42_DSP1_MPU_YMEM_ACCESS_0                    0x2BC3004
#define CS35L42_DSP1_MPU_WINDOW_ACCESS_0                  0x2BC3008
#define CS35L42_DSP1_MPU_XREG_ACCESS_0                    0x2BC300C
#define CS35L42_DSP1_MPU_YREG_ACCESS_0                    0x2BC3014
#define CS35L42_DSP1_MPU_XMEM_ACCESS_1                    0x2BC3018
#define CS35L42_DSP1_MPU_YMEM_ACCESS_1                    0x2BC301C
#define CS35L42_DSP1_MPU_WINDOW_ACCESS_1                  0x2BC3020
#define CS35L42_DSP1_MPU_XREG_ACCESS_1                    0x2BC3024
#define CS35L42_DSP1_MPU_YREG_ACCESS_1                    0x2BC302C
#define CS35L42_DSP1_MPU_XMEM_ACCESS_2                    0x2BC3030
#define CS35L42_DSP1_MPU_YMEM_ACCESS_2                    0x2BC3034
#define CS35L42_DSP1_MPU_WINDOW_ACCESS_2                  0x2BC3038
#define CS35L42_DSP1_MPU_XREG_ACCESS_2                    0x2BC303C
#define CS35L42_DSP1_MPU_YREG_ACCESS_2                    0x2BC3044
#define CS35L42_DSP1_MPU_XMEM_ACCESS_3                    0x2BC3048
#define CS35L42_DSP1_MPU_YMEM_ACCESS_3                    0x2BC304C
#define CS35L42_DSP1_MPU_WINDOW_ACCESS_3                  0x2BC3050
#define CS35L42_DSP1_MPU_XREG_ACCESS_3                    0x2BC3054
#define CS35L42_DSP1_MPU_YREG_ACCESS_3                    0x2BC305C
#define CS35L42_DSP1_MPU_X_EXT_MEM_ACCESS_0               0x2BC3060
#define CS35L42_DSP1_MPU_Y_EXT_MEM_ACCESS_0               0x2BC3064
#define CS35L42_DSP1_MPU_XM_VIO_ADDR                      0x2BC3100
#define CS35L42_DSP1_MPU_XM_VIO_STATUS                    0x2BC3104
#define CS35L42_DSP1_MPU_YM_VIO_ADDR                      0x2BC3108
#define CS35L42_DSP1_MPU_YM_VIO_STATUS                    0x2BC310C
#define CS35L42_DSP1_MPU_PM_VIO_ADDR                      0x2BC3110
#define CS35L42_DSP1_MPU_PM_VIO_STATUS                    0x2BC3114
#define CS35L42_DSP1_MPU_LOCK_CONFIG                      0x2BC3140
#define CS35L42_DSP1_MPU_WDT_RESET_CONTROL                0x2BC3180
#define CS35L42_DSP1_STREAM_ARB_MSTR1_CONFIG_0            0x2BC5000
#define CS35L42_DSP1_STREAM_ARB_MSTR1_CONFIG_1            0x2BC5004
#define CS35L42_DSP1_STREAM_ARB_MSTR1_CONFIG_2            0x2BC5008
#define CS35L42_DSP1_STREAM_ARB_MSTR2_CONFIG_0            0x2BC5010
#define CS35L42_DSP1_STREAM_ARB_MSTR2_CONFIG_1            0x2BC5014
#define CS35L42_DSP1_STREAM_ARB_MSTR2_CONFIG_2            0x2BC5018
#define CS35L42_DSP1_STREAM_ARB_MSTR3_CONFIG_0            0x2BC5020
#define CS35L42_DSP1_STREAM_ARB_MSTR3_CONFIG_1            0x2BC5024
#define CS35L42_DSP1_STREAM_ARB_MSTR3_CONFIG_2            0x2BC5028
#define CS35L42_DSP1_STREAM_ARB_MSTR4_CONFIG_0            0x2BC5030
#define CS35L42_DSP1_STREAM_ARB_MSTR4_CONFIG_1            0x2BC5034
#define CS35L42_DSP1_STREAM_ARB_MSTR4_CONFIG_2            0x2BC5038
#define CS35L42_DSP1_STREAM_ARB_TX1_CONFIG_0              0x2BC5200
#define CS35L42_DSP1_STREAM_ARB_TX1_CONFIG_1              0x2BC5204
#define CS35L42_DSP1_STREAM_ARB_TX2_CONFIG_0              0x2BC5208
#define CS35L42_DSP1_STREAM_ARB_TX2_CONFIG_1              0x2BC520C
#define CS35L42_DSP1_STREAM_ARB_TX3_CONFIG_0              0x2BC5210
#define CS35L42_DSP1_STREAM_ARB_TX3_CONFIG_1              0x2BC5214
#define CS35L42_DSP1_STREAM_ARB_TX4_CONFIG_0              0x2BC5218
#define CS35L42_DSP1_STREAM_ARB_TX4_CONFIG_1              0x2BC521C
#define CS35L42_DSP1_STREAM_ARB_TX5_CONFIG_0              0x2BC5220
#define CS35L42_DSP1_STREAM_ARB_TX5_CONFIG_1              0x2BC5224
#define CS35L42_DSP1_STREAM_ARB_TX6_CONFIG_0              0x2BC5228
#define CS35L42_DSP1_STREAM_ARB_TX6_CONFIG_1              0x2BC522C
#define CS35L42_DSP1_STREAM_ARB_RX1_CONFIG_0              0x2BC5400
#define CS35L42_DSP1_STREAM_ARB_RX1_CONFIG_1              0x2BC5404
#define CS35L42_DSP1_STREAM_ARB_RX2_CONFIG_0              0x2BC5408
#define CS35L42_DSP1_STREAM_ARB_RX2_CONFIG_1              0x2BC540C
#define CS35L42_DSP1_STREAM_ARB_RX3_CONFIG_0              0x2BC5410
#define CS35L42_DSP1_STREAM_ARB_RX3_CONFIG_1              0x2BC5414
#define CS35L42_DSP1_STREAM_ARB_RX4_CONFIG_0              0x2BC5418
#define CS35L42_DSP1_STREAM_ARB_RX4_CONFIG_1              0x2BC541C
#define CS35L42_DSP1_STREAM_ARB_RX5_CONFIG_0              0x2BC5420
#define CS35L42_DSP1_STREAM_ARB_RX5_CONFIG_1              0x2BC5424
#define CS35L42_DSP1_STREAM_ARB_RX6_CONFIG_0              0x2BC5428
#define CS35L42_DSP1_STREAM_ARB_RX6_CONFIG_1              0x2BC542C
#define CS35L42_DSP1_STREAM_ARB_IRQ1_CONFIG_0             0x2BC5600
#define CS35L42_DSP1_STREAM_ARB_IRQ1_CONFIG_1             0x2BC5604
#define CS35L42_DSP1_STREAM_ARB_IRQ1_CONFIG_2             0x2BC5608
#define CS35L42_DSP1_STREAM_ARB_IRQ2_CONFIG_0             0x2BC5610
#define CS35L42_DSP1_STREAM_ARB_IRQ2_CONFIG_1             0x2BC5614
#define CS35L42_DSP1_STREAM_ARB_IRQ2_CONFIG_2             0x2BC5618
#define CS35L42_DSP1_STREAM_ARB_IRQ3_CONFIG_0             0x2BC5620
#define CS35L42_DSP1_STREAM_ARB_IRQ3_CONFIG_1             0x2BC5624
#define CS35L42_DSP1_STREAM_ARB_IRQ3_CONFIG_2             0x2BC5628
#define CS35L42_DSP1_STREAM_ARB_IRQ4_CONFIG_0             0x2BC5630
#define CS35L42_DSP1_STREAM_ARB_IRQ4_CONFIG_1             0x2BC5634
#define CS35L42_DSP1_STREAM_ARB_IRQ4_CONFIG_2             0x2BC5638
#define CS35L42_DSP1_STREAM_ARB_RESYNC_MSK1               0x2BC5A00
#define CS35L42_DSP1_STREAM_ARB_ERR_STATUS                0x2BC5A08
#define CS35L42_DSP1_INTP_CTL_CORE_RESUME_STATIC          0x2BC6000
#define CS35L42_DSP1_INTP_CTL_CORE_RESUME_DYNAMIC         0x2BC6004
#define CS35L42_DSP1_INTP_CTL_NMI_CONTROL                 0x2BC6008
#define CS35L42_DSP1_INTP_CTL_IRQ_INV                     0x2BC6010
#define CS35L42_DSP1_INTP_CTL_IRQ_MODE                    0x2BC6014
#define CS35L42_DSP1_INTP_CTL_IRQ_EN                      0x2BC6018
#define CS35L42_DSP1_INTP_CTL_IRQ_MSK                     0x2BC601C
#define CS35L42_DSP1_INTP_CTL_IRQ_FLUSH                   0x2BC6020
#define CS35L42_DSP1_INTP_CTL_IRQ_MSK_CLR                 0x2BC6024
#define CS35L42_DSP1_INTP_CTL_IRQ_FRC                     0x2BC6028
#define CS35L42_DSP1_INTP_CTL_IRQ_MSK_SET                 0x2BC602C
#define CS35L42_DSP1_INTP_CTL_IRQ_ERR_STATUS              0x2BC6030
#define CS35L42_DSP1_INTP_CTL_IRQ_PEND_STATUS             0x2BC6034
#define CS35L42_DSP1_INTP_CTL_IRQ_GEN                     0x2BC6038
#define CS35L42_DSP1_INTP_CTL_TESTBITS                    0x2BC6040
#define CS35L42_DSP1_WDT_CONTROL                          0x2BC7000
#define CS35L42_DSP1_WDT_STATUS                           0x2BC7008
#define CS35L42_DSP1_ACCEL_DB_IN                          0x2BCD000
#define CS35L42_DSP1_ACCEL_LINEAR_OUT                     0x2BCD008
#define CS35L42_DSP1_ACCEL_LINEAR_IN                      0x2BCD010
#define CS35L42_DSP1_ACCEL_DB_OUT                         0x2BCD018
#define CS35L42_DSP1_ACCEL_RAND_NUM                       0x2BCD020

/************************************************/
/* DSP1 Y Memory Range Packed                   */
/************************************************/
#define CS35L42_DSP1_YMEM_PACKED_0                        0x2C00000
#define CS35L42_DSP1_YMEM_PACKED_1                        0x2C00004
#define CS35L42_DSP1_YMEM_PACKED_2                        0x2C00008
#define CS35L42_DSP1_YMEM_PACKED_1530                     0x2C017E8
#define CS35L42_DSP1_YMEM_PACKED_1531                     0x2C017EC
#define CS35L42_DSP1_YMEM_PACKED_1532                     0x2C017F0

/************************************************/
/* DSP1 Y Memory Range Unpacked 32-bit View     */
/************************************************/
#define CS35L42_DSP1_YMEM_UNPACKED32_0                    0x3000000
#define CS35L42_DSP1_YMEM_UNPACKED32_1                    0x3000004
#define CS35L42_DSP1_YMEM_UNPACKED32_1021                 0x3000FF4
#define CS35L42_DSP1_YMEM_UNPACKED32_1022                 0x3000FF8

/************************************************/
/* DSP1 Y Memory Range Unpacked 24-bit View     */
/************************************************/
#define CS35L42_DSP1_YMEM_UNPACKED24_0                    0x3400000
#define CS35L42_DSP1_YMEM_UNPACKED24_1                    0x3400004
#define CS35L42_DSP1_YMEM_UNPACKED24_2                    0x3400008
#define CS35L42_DSP1_YMEM_UNPACKED24_3                    0x340000C
#define CS35L42_DSP1_YMEM_UNPACKED24_2042                 0x3401FE8
#define CS35L42_DSP1_YMEM_UNPACKED24_2043                 0x3401FEC
#define CS35L42_DSP1_YMEM_UNPACKED24_2044                 0x3401FF0
#define CS35L42_DSP1_YMEM_UNPACKED24_2045                 0x3401FF4

/************************************************/
/* DSP1 Program Memory Range Packed             */
/************************************************/
#define CS35L42_DSP1_PMEM_0                               0x3800000
#define CS35L42_DSP1_PMEM_1                               0x3800004
#define CS35L42_DSP1_PMEM_2                               0x3800008
#define CS35L42_DSP1_PMEM_3                               0x380000C
#define CS35L42_DSP1_PMEM_4                               0x3800010
#define CS35L42_DSP1_PMEM_5110                            0x3804FD8
#define CS35L42_DSP1_PMEM_5111                            0x3804FDC
#define CS35L42_DSP1_PMEM_5112                            0x3804FE0
#define CS35L42_DSP1_PMEM_5113                            0x3804FE4
#define CS35L42_DSP1_PMEM_5114                            0x3804FE8
#define CS35L42_DSP1_PROM_0                               0x3C60000
#define CS35L42_DSP1_PROM_1                               0x3C60004
#define CS35L42_DSP1_PROM_2                               0x3C60008
#define CS35L42_DSP1_PROM_3                               0x3C6000C
#define CS35L42_DSP1_PROM_4                               0x3C60010
#define CS35L42_DSP1_PROM_30710                           0x3C7DFD8
#define CS35L42_DSP1_PROM_30711                           0x3C7DFDC
#define CS35L42_DSP1_PROM_30712                           0x3C7DFE0
#define CS35L42_DSP1_PROM_30713                           0x3C7DFE4
#define CS35L42_DSP1_PROM_30714                           0x3C7DFE8

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L42_SPEC_H
