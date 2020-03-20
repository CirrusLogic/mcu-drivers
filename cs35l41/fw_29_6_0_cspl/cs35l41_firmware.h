/**
 * @file cs35l41_firmware.h
 *
 * @brief CS35L41 Firmware C Header File
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

#ifndef CS35L41_FIRMWARE_H
#define CS35L41_FIRMWARE_H

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * Coefficient payload is included
 */
#define CS35L41_LOAD_COEFFICIENTS

/**
 * @defgroup CS35L41_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS35L41_ALGORITHM_GENERAL
#define CS35L41_ALGORITHM_CSPL
/** @} */

/**
 * @defgroup CS35L41_FIRMWARE_CONTROL_ADDRESSES
 * @brief Firmware control parameter addresses
 *
 * @{
 */
#ifdef CS35L41_ALGORITHM_GENERAL
#define CS35L41_SYSTEM_CONFIG_XM_STRUCT_T 0x2800650
#define CS35L41_HALO_STATE 0x2800650
#define CS35L41_HALO_HEARTBEAT 0x2800654
#define CS35L41_AUDIO_BLK_SIZE 0x2800658
#define CS35L41_BUILD_JOB_NAME 0x280065C
#define CS35L41_BUILD_JOB_NUMBER 0x2800668
#define CS35L41_GLOBAL_SAMPLE_RATE_HW 0x280066C
#define CS35L41_MULTIRATE_NUMBER_BANDS 0x2800670
#define CS35L41_FIRMWARE_STATE 0x2800674
#define CS35L41_EVENT_TIMEOUT 0x2800678
#endif

#ifdef CS35L41_ALGORITHM_CSPL
#define CS35L41_CSPL_XM_STRUCT_T 0x2800250
#define CS35L41_CSPL_ENABLE 0x2800250
#define CS35L41_CSPL_COMMAND 0x2800254
#define CS35L41_CSPL_STATE 0x2800258
#define CS35L41_CSPL_ERRORNO 0x280025C
#define CS35L41_CSPL_TEMPERATURE 0x2800260
#define CS35L41_CSPL_OVERSIGHT_GAIN 0x2800264
#define CS35L41_UPDT_PRMS 0x2800268
#define CS35L41_CAL_R 0x280026C
#define CS35L41_CAL_AMBIENT 0x2800270
#define CS35L41_CAL_STATUS 0x2800274
#define CS35L41_CAL_CHECKSUM 0x2800278
#define CS35L41_CAL_R_SELECTED 0x280027C
#define CS35L41_CAL_SET_STATUS 0x2800280
#define CS35L41_DIAG_F0 0x2800284
#define CS35L41_DIAG_Z_LOW_DIFF 0x2800288
#define CS35L41_DIAG_F0_STATUS 0x280028C
#define CS35L41_RTLOG_ENABLE 0x2800290
#define CS35L41_RTLOG_STATE 0x2800294
#define CS35L41_RTLOG_COUNT 0x2800298
#define CS35L41_RTLOG_FRMPRWIN 0x28002A4
#define CS35L41_RTLOG_VARIABLE 0x28002A8
#define CS35L41_RTLOG_DATA 0x28002E4
#define CS35L41_BDLOG_MAX_TEMP 0x280035C
#define CS35L41_BDLOG_MAX_EXC 0x2800360
#define CS35L41_BDLOG_OVER_TEMP_COUNT 0x2800364
#define CS35L41_BDLOG_OVER_EXC_COUNT 0x2800368
#define CS35L41_BDLOG_ABNORMAL_MUTE 0x280036C
#define CS35L41_REDUCE_POWER 0x2800370
#define CS35L41_CH_BAL 0x2800374
#define CS35L41_ATTENUATION 0x2800378
#define CS35L41_SPK_OUTPUT_POWER 0x280037C
#define CS35L41_CSPL_YM_STRUCT_T 0x3400038
#define CS35L41_CSPL_UPDATE_PARAMS_CONFIG 0x3400038
#define CS35L41_CSPL_CONFIG 0x34001D0
#endif
/** @} */

/**
 * Total blocks of CS35L41 Firmware
 */
#define cs35l41_total_fw_blocks (12)

/**
 * Total blocks of CS35L41 Coefficient data
 */
#define cs35l41_total_coeff_blocks (1)

/**
 * Total blocks of CS35L41 Calibration Coefficient data
 */
#define cs35l41_total_calibration_coeff_blocks (2)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * Block of HALO P/X/Y Memory contents for either firmware or coefficient download.
 */
typedef struct
{
    uint32_t block_size;    ///< Size of block in bytes
    uint32_t address;       ///< Control Port register address at which to begin loading
    const uint8_t *bytes;   ///< Pointer to array of bytes consisting of block payload
} halo_boot_block_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/**
 * Firmware memory block metadata
 */
extern const halo_boot_block_t cs35l41_fw_blocks[];

/**
 * Coefficient memory block metadata
 */
extern const halo_boot_block_t cs35l41_coeff_blocks[];

/**
 * Calibration Coefficient memory block metadata
 */
extern const halo_boot_block_t cs35l41_calibration_coeff_blocks[];

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/

#endif // CS35L41_FIRMWARE_H

