/**
 * @file cs35l42_ext.h
 *
 * @brief Functions and prototypes exported by the CS35L42 Driver Extended API module
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

#ifndef CS35L42_EXT_H
#define CS35L42_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "cs35l42.h"

/***********************************************************************************************************************
 * LITERALS, CONSTANTS, MACROS
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

/**
 * Set HW Digital Gain
 *
 * This will set the digital gain value.  The 'gain' parameter will have range checking applied to it, but the only
 * correction will be masking to the size of the gain bitfield (11 bits).
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in,out] gain             Pointer to integer gain value
 *
 * @return
 * - CS35L42_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Gain is outside range
 * - CS35L42_STATUS_OK          otherwise
 *
 */
uint32_t cs35l42_set_dig_gain(cs35l42_t *driver, uint32_t *gain);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L42_EXT_H
