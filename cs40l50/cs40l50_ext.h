/**
 * @file cs40l50_ext.h
 *
 * @brief Functions and prototypes exported by the CS40L50 Driver Extended API module
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

#ifndef CS40L50_EXT_H
#define CS40L50_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "cs40l50.h"
#include "rth_types.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
/**
 * Default values for different open wavetable fields
 */
#define WF_LENGTH_DEFAULT            (0x3FFFFF)
#define PWLS_MS4                     (0)
#define WAIT_TIME_DEFAULT            (0)
#define REPEAT_DEFAULT               (0)
#define LEVEL_MS4                    (0)
#define TIME_DEFAULT                 (0)
#define PWLS_LS4                     (0)
#define EXT_FREQ_DEFAULT             (1)
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

#define CS40L50_PLAY_RTH             (0)

#define CS40L50_RTH_TYPE_PCM         (0x8)
#define CS40L50_RTH_TYPE_PWLE        (12)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
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
} cs40l50_pwle_word1_entry_t;

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
} cs40l50_pwle_word2_entry_t;

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
} cs40l50_pwle_word3_entry_t;

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
} cs40l50_pwle_word4_entry_t;

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
} cs40l50_pwle_word5_entry_t;

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
} cs40l50_pwle_word6_entry_t;


typedef union
{
    uint32_t words[6];
    struct
    {
        cs40l50_pwle_word1_entry_t word1;
        cs40l50_pwle_word2_entry_t word2;
        cs40l50_pwle_word3_entry_t word3;
        cs40l50_pwle_word4_entry_t word4;
        cs40l50_pwle_word5_entry_t word5;
        cs40l50_pwle_word6_entry_t word6;
    };
} cs40l50_pwle_t;

typedef union
{
    uint32_t word1;
    struct
    {
        uint32_t level_ms8                   : 8;
        uint32_t time                        : 16;
        uint32_t reserved                    : 8;
    };
} cs40l50_pwle_short_word1_entry_t;

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
} cs40l50_pwle_short_word2_entry_t;

typedef union
{
    uint32_t words[2];
    struct
    {
        cs40l50_pwle_short_word1_entry_t word1;
        cs40l50_pwle_short_word2_entry_t word2;
    };
} cs40l50_pwle_short_section_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t cs40l50_set_click_compensation_enable(cs40l50_t *driver, bool f0_enable, bool redc_enable);
uint32_t cs40l50_trigger_pwle(cs40l50_t *driver, rth_pwle_section_t **s);
uint32_t cs40l50_trigger_pwle_advanced(cs40l50_t *driver, rth_pwle_section_t **s, uint8_t repeat, uint8_t num_sections);
uint32_t cs40l50_trigger_pcm(cs40l50_t *driver, uint8_t *s, uint32_t num_sections, uint16_t buffer_size_samples, uint16_t f0, uint16_t redc);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L50_EXT_H
