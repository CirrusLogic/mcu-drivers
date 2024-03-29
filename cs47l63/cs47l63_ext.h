/**
 * @file cs47l63_ext.h
 *
 * @brief Functions and prototypes exported by the CS47L63 Driver Extended API module
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

#ifndef CS47L63_EXT_H
#define CS47L63_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stddef.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
 /**
 * @defgroup CS47L63_DSP_
 * @brief Values for communicating with DSP
 *
 * @{
 */
#define CS47L63_DSP_OFFSET_MUL_VALUE              4
#define CS47L63_DSP_UNPACKED24_BYTES_PER_REG      4
#define CS47L63_DSP_IRQ_ACK_VAL                   0x1

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS47L63_EXT_H
