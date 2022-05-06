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

/************************************************/
/* One-Time Programmable (OTP) Control          */
/************************************************/
#define CS35L42_OTP_CTRL8                                 0x000041C

/************************************************/
/* OTP_CTRL_OTP_CTRL8                           */
/************************************************/
#define CS35L42_OTP_BOOT_DONE_STS                         0x00000004  /* OTP_BOOT_DONE_STS - [2] */
#define CS35L42_OTP_BOOT_DONE_STS_MASK                    0x00000004  /* OTP_BOOT_DONE_STS - [2] */
#define CS35L42_OTP_BOOT_DONE_STS_SHIFT                   2           /* OTP_BOOT_DONE_STS - [2] */
#define CS35L42_OTP_BOOT_DONE_STS_WIDTH                   1           /* OTP_BOOT_DONE_STS - [2] */

/************************************************/
/* Power, Global, and Release Control           */
/************************************************/
#define CS35L42_GLOBAL_ENABLES                            0x0002014
#define CS35L42_BLOCK_ENABLES                             0x0002018
#define CS35L42_ERROR_RELEASE                             0x0002034

/************************************************/
/* Power Control 1                              */
/************************************************/
#define CS35L42_GLOBAL_EN                                 0x00000001  /* GLOBAL_EN - [0] */
#define CS35L42_GLOBAL_EN_MASK                            0x00000001  /* GLOBAL_EN - [0] */
#define CS35L42_GLOBAL_EN_SHIFT                           0           /* GLOBAL_EN - [0] */
#define CS35L42_GLOBAL_EN_WIDTH                           1           /* GLOBAL_EN - [0] */

/************************************************/
/* Power Control 2                              */
/************************************************/
#define CS35L42_BST_EN                                    0x00000030  /* BST_EN - [5:4] */
#define CS35L42_BST_EN_MASK                               0x00000030  /* BST_EN - [5:4] */
#define CS35L42_BST_EN_SHIFT                              4           /* BST_EN - [5:4] */
#define CS35L42_BST_EN_WIDTH                              2           /* BST_EN - [5:4] */

/************************************************/
/* Digital I/O Pad Control                      */
/************************************************/
#define CS35L42_GPIO_PAD_CONTROL                          0x000242C

/************************************************/
/* Hibernation Power Management                 */
/************************************************/
#define CS35L42_PWRMGT_CTL                                0x0002900
#define CS35L42_WAKESRC_CTL                               0x0002904

/************************************************/
/* Hibernation Control                          */
/************************************************/
#define CS35L42_MEM_RDY                                   0x00000002  /* MEM_RDY - [1] */
#define CS35L42_MEM_RDY_MASK                              0x00000002  /* MEM_RDY - [1] */
#define CS35L42_MEM_RDY_SHIFT                             1           /* MEM_RDY - [1] */
#define CS35L42_MEM_RDY_WIDTH                             1           /* MEM_RDY - [1] */

/************************************************/
/* Device Clocking and Sample Rate Control      */
/************************************************/
#define CS35L42_GLOBAL_SAMPLE_RATE                        0x0002C0C

/************************************************/
/* Digital Boost Converter                      */
/************************************************/
#define CS35L42_VBST_CTL_1                                0x0003800
#define CS35L42_VBST_CTL_2                                0x0003804

/************************************************/
/* ASP Data Interface                           */
/************************************************/
#define CS35L42_ASP_CONTROL2                              0x0004808
#define CS35L42_ASP_DATA_CONTROL1                         0x0004830
#define CS35L42_ASP_DATA_CONTROL5                         0x0004840

/************************************************/
/* Data Routing                                 */
/************************************************/
#define CS35L42_DACPCM1_INPUT                             0x0004C00
#define CS35L42_ASPTX1_INPUT                              0x0004C20
#define CS35L42_ASPTX2_INPUT                              0x0004C24
#define CS35L42_ASPTX3_INPUT                              0x0004C28
#define CS35L42_ASPTX4_INPUT                              0x0004C2C
#define CS35L42_DSP1RX1_INPUT                             0x0004C40
#define CS35L42_DSP1RX2_INPUT                             0x0004C44
#define CS35L42_DSP1RX5_INPUT                             0x0004C50
#define CS35L42_DSP1RX6_INPUT                             0x0004C54

/************************************************/
/* Amplifier Volume Control                     */
/************************************************/
#define CS35L42_AMP_CTRL                                  0x0006000

/************************************************/
/* Amplifier Digital Volume Control             */
/************************************************/
#define CS35L42_AMP_VOL_PCM                              0x00003FF8  /* AMP_VOL_PCM - [13:3] */
#define CS35L42_AMP_VOL_PCM_MASK                         0x00003FF8  /* AMP_VOL_PCM - [13:3] */
#define CS35L42_AMP_VOL_PCM_SHIFT                        3           /* AMP_VOL_PCM - [13:3] */
#define CS35L42_AMP_VOL_PCM_WIDTH                        11          /* AMP_VOL_PCM - [13:3] */

#define CS35L42_AMP_VOL_PCM_MUTE                         (0x400)         ///< @see Section 7.16.1
#define CS35L42_AMP_VOL_PCM_0DB                          (0)             ///< @see Section 7.16.1
#define CS35L42_AMP_VOL_PCM_MAX_DB                       (12)            ///< @see Section 7.16.1
#define CS35L42_AMP_VOL_PCM_MIN_DB                       (-102)          ///< @see Section 7.16.1

/************************************************/
/* Power Management Class H, Weak-FET, and Noise Gating */
/************************************************/
#define CS35L42_NG_CONFIG                                 0x0006808

/************************************************/
/* Dynamic Range Enhancement                    */
/************************************************/
#define CS35L42_AMP_GAIN                                  0x0006C04

/************************************************/
/* Amplifier Path Additional Control            */
/************************************************/
#define CS35L42_TST_DAC_MSM_CONFIG                        0x0007404

/************************************************/
/* Interrupt Status and Mask Control            */
/************************************************/
#define CS35L42_IRQ1_EINT_1                               0x0010010
#define CS35L42_IRQ1_MASK_1                               0x0010110

/************************************************/
/* IRQ1_IRQ1_EINT_1                             */
/************************************************/
#define CS35L42_DSP_VIRTUAL2_MBOX_WR_EINT1                0x80000000  /* DSP_VIRTUAL2_MBOX_WR_EINT1 - [31] */
#define CS35L42_DSP_VIRTUAL2_MBOX_WR_EINT1_MASK           0x80000000  /* DSP_VIRTUAL2_MBOX_WR_EINT1 - [31] */
#define CS35L42_DSP_VIRTUAL2_MBOX_WR_EINT1_SHIFT          31          /* DSP_VIRTUAL2_MBOX_WR_EINT1 - [31] */
#define CS35L42_DSP_VIRTUAL2_MBOX_WR_EINT1_WIDTH          1           /* DSP_VIRTUAL2_MBOX_WR_EINT1 - [31] */
#define CS35L42_DSP_VIRTUAL1_MBOX_WR_EINT1                0x40000000  /* DSP_VIRTUAL1_MBOX_WR_EINT1 - [30] */
#define CS35L42_DSP_VIRTUAL1_MBOX_WR_EINT1_MASK           0x40000000  /* DSP_VIRTUAL1_MBOX_WR_EINT1 - [30] */
#define CS35L42_DSP_VIRTUAL1_MBOX_WR_EINT1_SHIFT          30          /* DSP_VIRTUAL1_MBOX_WR_EINT1 - [30] */
#define CS35L42_DSP_VIRTUAL1_MBOX_WR_EINT1_WIDTH          1           /* DSP_VIRTUAL1_MBOX_WR_EINT1 - [30] */
#define CS35L42_AMP_ERR_EINT1                             0x08000000  /* AMP_ERR_EINT1 - [27] */
#define CS35L42_AMP_ERR_EINT1_MASK                        0x08000000  /* AMP_ERR_EINT1 - [27] */
#define CS35L42_AMP_ERR_EINT1_SHIFT                       27          /* AMP_ERR_EINT1 - [27] */
#define CS35L42_AMP_ERR_EINT1_WIDTH                       1           /* AMP_ERR_EINT1 - [27] */
#define CS35L42_TEMP_ERR_EINT1                            0x04000000  /* TEMP_ERR_EINT1 - [26] */
#define CS35L42_TEMP_ERR_EINT1_MASK                       0x04000000  /* TEMP_ERR_EINT1 - [26] */
#define CS35L42_TEMP_ERR_EINT1_SHIFT                      26          /* TEMP_ERR_EINT1 - [26] */
#define CS35L42_TEMP_ERR_EINT1_WIDTH                      1           /* TEMP_ERR_EINT1 - [26] */
#define CS35L42_BST_SHORT_ERR_EINT1                       0x00400000  /* BST_SHORT_ERR_EINT1 - [22] */
#define CS35L42_BST_SHORT_ERR_EINT1_MASK                  0x00400000  /* BST_SHORT_ERR_EINT1 - [22] */
#define CS35L42_BST_SHORT_ERR_EINT1_SHIFT                 22          /* BST_SHORT_ERR_EINT1 - [22] */
#define CS35L42_BST_SHORT_ERR_EINT1_WIDTH                 1           /* BST_SHORT_ERR_EINT1 - [22] */
#define CS35L42_BST_DCM_UVP_ERR_EINT1                     0x00200000  /* BST_DCM_UVP_ERR_EINT1 - [21] */
#define CS35L42_BST_DCM_UVP_ERR_EINT1_MASK                0x00200000  /* BST_DCM_UVP_ERR_EINT1 - [21] */
#define CS35L42_BST_DCM_UVP_ERR_EINT1_SHIFT               21          /* BST_DCM_UVP_ERR_EINT1 - [21] */
#define CS35L42_BST_DCM_UVP_ERR_EINT1_WIDTH               1           /* BST_DCM_UVP_ERR_EINT1 - [21] */
#define CS35L42_BST_OVP_ERR_EINT1                         0x00100000  /* BST_OVP_ERR_EINT1 - [20] */
#define CS35L42_BST_OVP_ERR_EINT1_MASK                    0x00100000  /* BST_OVP_ERR_EINT1 - [20] */
#define CS35L42_BST_OVP_ERR_EINT1_SHIFT                   20          /* BST_OVP_ERR_EINT1 - [20] */
#define CS35L42_BST_OVP_ERR_EINT1_WIDTH                   1           /* BST_OVP_ERR_EINT1 - [20] */
#define CS35L42_MSM_PUP_DONE_EINT1                        0x00020000  /* MSM_PUP_DONE_EINT1 - [17] */
#define CS35L42_MSM_PUP_DONE_EINT1_MASK                   0x00020000  /* MSM_PUP_DONE_EINT1 - [17] */
#define CS35L42_MSM_PUP_DONE_EINT1_SHIFT                  17          /* MSM_PUP_DONE_EINT1 - [17] */
#define CS35L42_MSM_PUP_DONE_EINT1_WIDTH                  1           /* MSM_PUP_DONE_EINT1 - [17] */
#define CS35L42_MSM_PDN_DONE_EINT1                        0x00010000  /* MSM_PDN_DONE_EINT1 - [16] */
#define CS35L42_MSM_PDN_DONE_EINT1_MASK                   0x00010000  /* MSM_PDN_DONE_EINT1 - [16] */
#define CS35L42_MSM_PDN_DONE_EINT1_SHIFT                  16          /* MSM_PDN_DONE_EINT1 - [16] */
#define CS35L42_MSM_PDN_DONE_EINT1_WIDTH                  1           /* MSM_PDN_DONE_EINT1 - [16] */
#define CS35L42_WKSRC_STATUS6_EINT1                       0x00004000  /* WKSRC_STATUS6_EINT1 - [14] */
#define CS35L42_WKSRC_STATUS6_EINT1_MASK                  0x00004000  /* WKSRC_STATUS6_EINT1 - [14] */
#define CS35L42_WKSRC_STATUS6_EINT1_SHIFT                 14          /* WKSRC_STATUS6_EINT1 - [14] */
#define CS35L42_WKSRC_STATUS6_EINT1_WIDTH                 1           /* WKSRC_STATUS6_EINT1 - [14] */
#define CS35L42_WKSRC_STATUS_ANY_EINT1                    0x00000100  /* WKSRC_STATUS_ANY_EINT1 - [8] */
#define CS35L42_WKSRC_STATUS_ANY_EINT1_MASK               0x00000100  /* WKSRC_STATUS_ANY_EINT1 - [8] */
#define CS35L42_WKSRC_STATUS_ANY_EINT1_SHIFT              8           /* WKSRC_STATUS_ANY_EINT1 - [8] */
#define CS35L42_WKSRC_STATUS_ANY_EINT1_WIDTH              1           /* WKSRC_STATUS_ANY_EINT1 - [8] */

/************************************************/
/* DSP Virtual 1 Scratch Space                  */
/************************************************/
#define CS35L42_DSP_VIRTUAL1_MBOX_1                       0x0013020

/************************************************/
/* Combined DSP1 X Memory and Register Space Unpacked 24-bit View */
/************************************************/
#define CS35L42_DSP1_CCM_CORE_CONTROL                     0x2BC1000

/************************************************/
/* XM_UNPACKED24_DSP1_CCM_CORE_CONTROL          */
/************************************************/
#define CS35L42_DSP1_CCM_CORE_EN                          0x00000001  /* DSP1_CCM_CORE_EN - [0] */
#define CS35L42_DSP1_CCM_CORE_EN_MASK                     0x00000001  /* DSP1_CCM_CORE_EN - [0] */
#define CS35L42_DSP1_CCM_CORE_EN_SHIFT                    0           /* DSP1_CCM_CORE_EN - [0] */
#define CS35L42_DSP1_CCM_CORE_EN_WIDTH                    1           /* DSP1_CCM_CORE_EN - [0] */

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L42_SPEC_H
