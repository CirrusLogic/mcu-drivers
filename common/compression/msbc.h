/**
 * @file msbc.h
 *
 * @brief Functions and prototypes for mSBC decompression module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2023 All Rights Reserved, http://www.cirrus.com/
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

#ifndef MSBC_H
#define MSBC_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "decompr.h"

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

/**
 * Allocate and initialize an msbc decompression structure with a given output endian format
 *
 * @param [in]
 * - output_endian       The endianness to use for the output data
 *
 * @return
 * - void *              An msbc context structure
 * - NULL                Failed to allocation and initialize an msbc decompression structure
 *
 */
void *msbc_init(endian_t output_endian);

/**
 * Decompress data in mSBC format
 *
 * @param [in]
 * - context              Pointer to the msbc decompression state structure
 * - decompr_data_buf_ptr Pointer to the data buffer to decompress data to
 * - compr_data_buf_ptr   Pointer to the data buffer containing the compressed data
 *
 * @param [out]
 * - bytes_decompressed   Pointer to the length of data added to the decompr_data_buf_ptr
 *
 * @return
 * - DECOMPR_STATUS_FAIL         Failed to decompress the given data
 * - DECOMPR_STATUS_OK           otherwise
 *
 */
uint32_t msbc_decompress(void *context,
                         data_ringbuf_t *decompr_data_buf_ptr,
                         data_ringbuf_t *compr_data_buf_ptr,
                         uint32_t *bytes_decompressed);

/**
 * Deinitialize and free the msbc decompression structure, freeing all resources used
 *
 * @param [in]
 * - context              Pointer to the msbc decompression state structure
 *
 */
void msbc_deinit(void *context);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // MSBC_H
