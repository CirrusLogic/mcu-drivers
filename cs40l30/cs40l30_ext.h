/**
 * @file cs40l30_ext.h
 *
 * @brief Functions and prototypes exported by the CS40L30 Driver Extended API module
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

#ifndef CS40L30_EXT_H
#define CS40L30_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "cs40l30.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

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
uint32_t cs40l30_trigger(cs40l30_t *driver, uint32_t index);

#ifdef CS40L30_ALGORITHM_BUZZGEN
uint32_t cs40l30_buzzgen_config(cs40l30_t *driver, uint8_t id, uint8_t freq, uint8_t level, uint32_t duration);
#endif //CS40L30_ALGORITHM_BUZZGEN

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L30_EXT_H
