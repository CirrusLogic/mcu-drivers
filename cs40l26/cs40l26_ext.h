/**
 * @file cs40l25_ext.h
 *
 * @brief Functions and prototypes exported by the CS40L25 Driver Extended API module
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

#ifndef CS40L26_EXT_H
#define CS40L26_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "cs40l26.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * Default value of Dynamic F0 table entry
 */
#define CS40L26_DYNAMIC_F0_TABLE_ENTRY_DEFAULT  (0x007FE000)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * Dynamic F0 table entry type
 */
typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t f0                         : 13; ///< F0 in Q10.3 format
            uint32_t index                      : 10; ///< Index in Wave Table
            uint32_t reserved                   : 9;
        };
    };
} cs40l26_dynamic_f0_table_entry_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Enable the HALO FW Dynamic F0 Algorithm
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] enable           true to enable Dynamic F0, false to disable Dynamic F0
 *
 * @return
 * - CS40L26_STATUS_FAIL        if update of any HALO FW control fails
 * - CS40L26_STATUS_OK          otherwise
 *
 */
uint32_t cs40l26_set_dynamic_f0_enable(cs40l26_t *driver, bool enable);

/**
 * Get the Dynamic F0
 *
 * Get the current value of the F0 for a specific index into the WaveTable.  The index is specified in the 'f0_entry'
 * member 'index'.  The current F0 for WaveTable entries are stored in a Dynamic F0 table in FW, which only contains
 * a Dynamic F0 for WaveTable entries that have been played since power up.  This table has a maximum size of 20. If
 * the index specified is not found in the FW table, the table default CS40L26_DYNAMIC_F0_TABLE_ENTRY_DEFAULT is
 * returned.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in/out] f0_entry     Pointer to Dynamic F0 structure
 *
 * @return
 * - CS40L26_STATUS_FAIL
 *      - if any call to cs40l26_control fails
 *      - if the specified WaveTable index is >= 20
 * - CS40L26_STATUS_OK          otherwise
 *
 */
uint32_t cs40l26_get_dynamic_f0(cs40l26_t *driver, cs40l26_dynamic_f0_table_entry_t *f0_entry);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L26_EXT_H
