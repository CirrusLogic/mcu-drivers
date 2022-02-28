/**
 * @file cs40l25_ext.h
 *
 * @brief Functions and prototypes exported by the CS40L25 Driver Extended API module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2022 All Rights Reserved, http://www.cirrus.com/
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
#include "rth_types.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * Default value of Dynamic F0 table entry
 */
#define CS40L26_DYNAMIC_F0_TABLE_ENTRY_DEFAULT  (0x007FE000)

#define WF_LENGTH_DEFAULT            (0x3FFFFF)
#define PWLS_MS4                     (0)
#define WAIT_TIME_DEFAULT            (0)
#define REPEAT_DEFAULT               (0)
#define LEVEL_MS4                    (0)
#define TIME_DEFAULT                 (0)
#define PWLS_LS4                     (0)
#define EXT_FREQ_DEFAULT             (0)
#define AMP_REG_DEFAULT              (0)
#define BRAKING_DEFAULT              (0)
#define CHIRP_DEFAULT                (0)
#define FREQ_DEFAULT                 (0)
#define LEVEL_LS8                    (0)
#define VB_TAR_MS12                  (0)
#define VB_TAR_LS4                   (0)
#define LEVEL_DEFAULT                (0)
#define LEVEL_MS8_DEFAULT            (0)
#define LEVEL_LS4_DEFAULT            (0)

#define PWLE_API_ENABLE              (0)

#define WAV_LENGTH_DEFAULT           (0)
#define DATA_LENGTH_DEFAULT          (0)
#define F0_DEFAULT                   (0)
#define SCALED_REDC_DEFAULT          (0)

#define CS40L26_PLAY_RTH             (0)

#define CS40L26_RTH_TYPE_PCM         (0x8)

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

#ifdef PWLE_API_ENABLE
typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t wf_length                  : 24;
            uint32_t reserved                   : 8;
        };
    };
} cs40l26_pwle_word1_entry_t;

typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t pwls_ms4                    : 4;
            uint32_t wait_time                   : 12;
            uint32_t repeat                      : 8;
            uint32_t reserved                    : 8;
        };
    };
} cs40l26_pwle_word2_entry_t;

typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t level_ms4                   : 4;
            uint32_t time                        : 16;
            uint32_t pwls_ls4                    : 4;
            uint32_t reserved                    : 8;
        };
    };
} cs40l26_pwle_word3_entry_t;

typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t ext_freq                      : 1;
            uint32_t amp_reg                       : 1;
            uint32_t braking                       : 1;
            uint32_t chirp                         : 1;
            uint32_t freq                          : 12;
            uint32_t level_ls8                     : 8;
            uint32_t reserved                      : 8;
        };
    };
} cs40l26_pwle_word4_entry_t;

typedef struct
{
    union
    {
      uint32_t word;
      struct
      {
          uint32_t level_ms4                     : 4;
          uint32_t time                          : 16;
          uint32_t reserved                      : 12;
      };
    };
} cs40l26_pwle_word5_entry_t;

typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t ext_freq                      : 1;
            uint32_t amp_reg                       : 1;
            uint32_t braking                       : 1;
            uint32_t chirp                         : 1;
            uint32_t freq                          : 12;
            uint32_t level_ls8                     : 8;
            uint32_t reserved                      : 8;
        };
    };
} cs40l26_pwle_word6_entry_t;


typedef union
{
    uint32_t words[6];
    struct
    {
        cs40l26_pwle_word1_entry_t word1;
        cs40l26_pwle_word2_entry_t word2;
        cs40l26_pwle_word3_entry_t word3;
        cs40l26_pwle_word4_entry_t word4;
        cs40l26_pwle_word5_entry_t word5;
        cs40l26_pwle_word6_entry_t word6;
    };
} cs40l26_pwle_t;

typedef union
{
    uint32_t word1;
    struct
    {
        uint32_t level_ms8                   : 8;
        uint32_t time                        : 16;
        uint32_t reserved                    : 8;
    };
} cs40l26_pwle_short_word1_entry_t;

typedef union
{
    uint32_t word2;
    struct
    {
        uint32_t reserved_0                  : 4;
        uint32_t ext_freq                    : 1;
        uint32_t amp_reg                     : 1;
        uint32_t braking                     : 1;
        uint32_t chirp                       : 1;
        uint32_t freq                        : 12;
        uint32_t level_ls4                   : 4;
        uint32_t reserved_1                  : 8;
    };
} cs40l26_pwle_short_word2_entry_t;

typedef union
{
    uint32_t words[2];
    struct
    {
        cs40l26_pwle_short_word1_entry_t word1;
        cs40l26_pwle_short_word2_entry_t word2;
    };
} cs40l26_pwle_short_section_t;

#endif

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

#ifdef PWLE_API_ENABLE
uint32_t cs40l26_trigger_pwle(cs40l26_t *driver, rth_pwle_section_t **s);
uint32_t cs40l26_trigger_pwle_advanced(cs40l26_t *driver, rth_pwle_section_t **s, uint8_t repeat, uint8_t num_sections);
#endif
uint32_t cs40l26_trigger_pcm(cs40l26_t *driver, uint8_t *s, uint32_t num_sections, uint16_t buffer_size_samples, uint16_t f0, uint16_t redc);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L26_EXT_H
