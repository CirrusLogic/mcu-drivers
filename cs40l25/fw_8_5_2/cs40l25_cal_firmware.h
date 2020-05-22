/**
 * @file cs40l25_cal_firmware.h
 *
 * @brief CS40L25_CAL Firmware C Header File
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

#ifndef CS40L25_CAL_FIRMWARE_H
#define CS40L25_CAL_FIRMWARE_H

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L25_FIRMWARE_META
 * @brief Firmware meta data
 *
 * @{
 */
#define CS40L25_FIRMWARE_ID  0x1400C6
/** @} */

/**
 * @defgroup CS40L25_CAL_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS40L25_CAL_ALGORITHM_GENERAL
#define CS40L25_CAL_ALGORITHM_F0_TRACKING
#define CS40L25_CAL_ALGORITHM_Q_ESTIMATION
/** @} */

/**
 * @defgroup CS40L25_CAL_FIRMWARE_CONTROL_ADDRESSES
 * @brief Firmware control parameter addresses
 *
 * @{
 */
#define CS40L25_CAL_FIRMWARE_REVISION 0x2800010

#ifdef CS40L25_CAL_ALGORITHM_GENERAL
#define CS40L25_CAL_SYSTEM_CONFIG_XM_STRUCT_T 0x2800B58
#define CS40L25_CAL_HALO_STATE 0x2800B58
#define CS40L25_CAL_HALO_HEARTBEAT 0x2800B5C
#define CS40L25_CAL_SHUTDOWNREQUEST 0x2800B60
#define CS40L25_CAL_AUDIO_BLK_SIZE 0x2800B64
#define CS40L25_CAL_BUILD_JOB_NAME 0x2800B68
#define CS40L25_CAL_BUILD_JOB_NUMBER 0x2800B74
#define CS40L25_CAL_FEATURE_BITMAP 0x2800B78
#endif

#ifdef CS40L25_CAL_ALGORITHM_F0_TRACKING
#define CS40L25_CAL_F0_TRACKING_XM_STRUCT_T 0x2800F98
#define CS40L25_CAL_CENTRE_FREQUENCY 0x2800F98
#define CS40L25_CAL_ALPHA 0x2800F9C
#define CS40L25_CAL_GAIN 0x2800FA0
#define CS40L25_CAL_SINE_TONE_NOISE_FLOOR 0x2800FA4
#define CS40L25_CAL_BACKEMF_NOISE_FLOOR 0x2800FA8
#define CS40L25_CAL_BETA_RE 0x2800FAC
#define CS40L25_CAL_F0 0x2800FB0
#define CS40L25_CAL_CLOSED_LOOP 0x2800FB4
#define CS40L25_CAL_REDC 0x2800FB8
#define CS40L25_CAL_LAG_TIME 0x2800FBC
#define CS40L25_CAL_LAG_DETUNE 0x2800FC0
#define CS40L25_CAL_TONELEVEL 0x2800FC4
#define CS40L25_CAL_F0_TRACKING_ENABLE 0x2800FC8
#define CS40L25_CAL_SMOOTHING_FACTOR_F0 0x2800FCC
#define CS40L25_CAL_DEFAULT_REDC 0x2800FD0
#define CS40L25_CAL_PILOT_TONE_AMPLITUDE 0x2800FD4
#define CS40L25_CAL_MAXBACKEMF 0x2800FD8
#endif

#ifdef CS40L25_CAL_ALGORITHM_Q_ESTIMATION
#define CS40L25_CAL_Q_ESTIMATION_XM_STRUCT_T 0x2800B4C
#define CS40L25_CAL_FSPAN 0x2800B4C
#define CS40L25_CAL_TONE_DURATION_MS 0x2800B50
#define CS40L25_CAL_Q_EST 0x2800B54
#endif

/** @} */

/**
 * Total blocks of CS40L25_CAL Firmware
 */
#define cs40l25_cal_total_fw_blocks (9)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * Block of HALO P/X/Y Memory contents for either firmware or coefficient download.
 */
#ifndef HALO_BOOT_BLOCK_T_DEFINED
#define HALO_BOOT_BLOCK_T_DEFINED
typedef struct
{
    uint32_t block_size;    ///< Size of block in bytes
    uint32_t address;       ///< Control Port register address at which to begin loading
    const uint8_t *bytes;   ///< Pointer to array of bytes consisting of block payload
} halo_boot_block_t;
#endif

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/**
 * Firmware memory block metadata
 */
extern const halo_boot_block_t cs40l25_cal_fw_blocks[];

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/

#endif // CS40L25_CAL_FIRMWARE_H

