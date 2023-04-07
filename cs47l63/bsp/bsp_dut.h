/**
 * @file bsp_dut.h
 *
 * @brief Functions and prototypes exported by the BSP module for the cs47l63 platform.
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021, 2023 All Rights Reserved, http://www.cirrus.com/
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

#ifndef BSP_DUT_H
#define BSP_DUT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdbool.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define BSP_DUT_I2C_ADDRESS_8BIT                            (0x80)

#define BSP_DUT_I2S_HALF_SIZE                               (4096)
#define BSP_DUT_I2S_SIZE                                    (BSP_DUT_I2S_HALF_SIZE * 2)
#define BSP_DUT_BUFFER_SIZE                                 (BSP_DUT_I2S_SIZE * 2)
#define BSP_DUT_RECORDING_SIZE                              (BSP_DUT_I2S_SIZE * 3)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/
#define BSP_USE_CASE_TG_HP_EN                                   (0x0)
#define BSP_USE_CASE_TG_HP_DIS                                  (0x1)
#define BSP_USE_CASE_SCC_RECORD_PACKED16                        (0x2)
#define BSP_USE_CASE_SCC_RECORD_MSBC                            (0x3)
#define BSP_USE_CASE_SCC_MANUAL_TRIGGER                         (0x4)
#define BSP_USE_CASE_SCC_TRIGGERED                              (0x5)
#define BSP_USE_CASE_SCC_PROCESS_IRQ                            (0x6)
#define BSP_USE_CASE_SCC_PROCESS_I2S                            (0x7)
#define BSP_USE_CASE_SCC_STOP_RECORDING                         (0x8)

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern bool bsp_process_irq;
extern volatile bool bsp_process_i2s;

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t bsp_dut_initialize(void);
uint32_t bsp_dut_reset(void);
uint32_t bsp_dut_boot(void);
uint32_t bsp_dut_use_case(uint32_t use_case);
uint32_t bsp_dut_process(void);
uint32_t bsp_dut_get_driver_handle(void **driver);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // BSP_DUT_H
