/**
 * @file mock_bsp.h
 *
 * @brief Mock implementation of the BSP-to-CS35L56 Driver interface
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
 *
 */

#ifndef MOCK_BSP_H
#define MOCK_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdbool.h>
#include "bsp_driver_if.h"
#include "mock.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
#define MOCK_BSP_MAX_CALLS_PER_TEST (10)
#define MOCK_SERIAL_BUFFER_LENGTH   (8)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/
extern uint8_t mock_bsp_called_count;
extern void *mock_bsp_call_queue[];
extern uint32_t mock_bsp_ret_array[];
extern uint32_t mock_bsp_args[];
extern void *mock_bsp_ptr_args[];
extern uint32_t mock_bsp_args_by_ref[];

extern uint8_t mock_bsp_cp_write_buffer[];
extern uint8_t mock_bsp_cp_read_buffer[];

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
/**
 * @brief Initialize the Mock BSP
 *
 * @param none
 *
 * @return none
 */
void mock_bsp_initialize(void);
/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // MOCK_BSP_H
