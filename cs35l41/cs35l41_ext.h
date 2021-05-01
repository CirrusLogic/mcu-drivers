/**
 * @file cs35l41_ext.h
 *
 * @brief Functions and prototypes exported by the CS35L41 Driver Extended API module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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

#ifndef CS35L41_EXT_H
#define CS35L41_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "cs35l41.h"

/***********************************************************************************************************************
 * LITERALS, CONSTANTS, MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * ID of CS35L41 GPIO
 */
typedef enum
{
   GPIO1_ID = 0,
   GPIO_ID_MIN = GPIO1_ID,
   GPIO2_ID,
   GPIO3_ID,
   GPIO4_ID,
   GPIO_ID_MAX = GPIO4_ID
 } cs35l41_gpio_id_t;

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
 * - CS35L41_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Gain is outside range
 * - CS35L41_STATUS_OK          otherwise
 *
 */
uint32_t cs35l41_set_dig_gain(cs35l41_t *driver, uint32_t *gain);

/**
 * Configure CS35L41 GPIO Direction
 *
 * This will configure the CS35L41 GPIO as either an input or an output.
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] gpio_id              ID for which GPIO to poll
 * @param [in] is_output            true = configure for Output, false = configure for Input
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Any pointers are NULL
 * - CS35L41_STATUS_OK          otherwise
 *
 */
uint32_t cs35l41_config_gpio(cs35l41_t *driver, cs35l41_gpio_id_t gpio_id, bool is_output);

/**
 * Set CS35L41 GPIO Level
 *
 * This will set the CS35L41 GPIO level as low or high.
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] gpio_id              ID for which GPIO to poll
 * @param [in] is_high              true = set GPIO High, false = set GPIO Low
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Any pointers are NULL
 * - CS35L41_STATUS_OK          otherwise
 *
 */
uint32_t cs35l41_set_gpio(cs35l41_t *driver, cs35l41_gpio_id_t gpio_id, bool is_high);

/**
 * Get CS35L41 GPIO Level
 *
 * This will poll the level of the CS35L41 GPIO indicated by 'gpio_id' and return the value (0x0 or 0x1) to 'level'.
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] gpio_id              ID for which GPIO to poll
 * @param [out] level               Current level of GPIO
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Any pointers are NULL
 * - CS35L41_STATUS_OK          otherwise
 *
 */
uint32_t cs35l41_get_gpio(cs35l41_t *driver, cs35l41_gpio_id_t gpio_id, uint32_t *level);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L41_EXT_H
