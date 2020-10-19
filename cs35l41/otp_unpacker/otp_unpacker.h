/**
 * @file otp_unpacker.h
 *
 * @brief Functions and prototypes exported by the OTP Unpacker module
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

#ifndef OTP_UNPACKER
#define OTP_UNPACKER

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "cs35l41.h"

/***********************************************************************************************************************
 * LITERALS, CONSTANTS, MACROS
 **********************************************************************************************************************/
/**
 * @defgroup OTP_UNPACKER_STATUS_
 * @brief Return codes for OTP Unpacker API calls
 *
 * @{
 */
#define OTP_UNPACKER_STATUS_OK          (0)
#define OTP_UNPACKER_STATUS_FAIL        (1)
/** @} */

#define OTP_UNPACKER_OTP_ADDRESS        (CS35L41_OTP_IF_OTP_MEM0_REG)

#define OTP_UNPACKER_OTP_SIZE_WORDS     (CS35L41_OTP_SIZE_WORDS)

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern const cs35l41_otp_map_t cs35l41_otp_maps[2];  // Extern-ed from cs35l41.c

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t otp_unpacker_initialize(uint8_t otpid, uint8_t *otp_buffer);
uint32_t otp_unpacker_deinitialize(void);
uint32_t otp_unpacker_get_reg_list_total(uint8_t *total);
uint32_t otp_unpacker_get_reg_address(uint32_t *address, uint8_t index);
uint32_t otp_unpacker_set_reg_value(uint8_t index, uint32_t value);
uint32_t otp_unpacker_get_unpacked_reg_list(uint32_t **reg_list, uint32_t *reg_list_total_words);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // OTP_UNPACKER
