/**
 * @file otp_unpacker.h
 *
 * @brief Functions and prototypes exported by the OTP Unpacker module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
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
