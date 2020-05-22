/**
 * @file cs40l25_firmware.h
 *
 * @brief CS40L25 Firmware C Header File
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

#ifndef CS40L25_FIRMWARE_H
#define CS40L25_FIRMWARE_H

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
#define CS40L25_FIRMWARE_ID  0x1400E0
/** @} */

/**
 * Coefficient payload is included
 */
#define CS40L25_LOAD_COEFFICIENTS

/**
 * @defgroup CS40L25_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
#define CS40L25_ALGORITHM_GENERAL
#define CS40L25_ALGORITHM_VIBEGEN
#define CS40L25_ALGORITHM_CLAB
#define CS40L25_ALGORITHM_DYNAMIC_F0
/** @} */

/**
 * @defgroup CS40L25_FIRMWARE_CONTROL_ADDRESSES
 * @brief Firmware control parameter addresses
 *
 * @{
 */
#define CS40L25_FIRMWARE_REVISION 0x2800010

#ifdef CS40L25_ALGORITHM_GENERAL
#define CS40L25_SYSTEM_CONFIG_XM_STRUCT_T 0x2801510
#define CS40L25_HALO_STATE 0x2801510
#define CS40L25_HALO_HEARTBEAT 0x2801514
#define CS40L25_STATEMACHINE 0x2801518
#define CS40L25_BUTTONTOUCHCOUNT 0x280151C
#define CS40L25_BUTTONRELEASECOUNT 0x280152C
#define CS40L25_RXIN 0x280153C
#define CS40L25_BUTTONDETECT 0x2801540
#define CS40L25_RXBUFFER 0x2801550
#define CS40L25_RXACK 0x2801650
#define CS40L25_EVENTCONTROL 0x2801654
#define CS40L25_GPIO1EVENT 0x2801658
#define CS40L25_GPIO2EVENT 0x280165C
#define CS40L25_GPIO3EVENT 0x2801660
#define CS40L25_GPIO4EVENT 0x2801664
#define CS40L25_GPIOPLAYBACKEVENT 0x2801668
#define CS40L25_TRIGGERPLAYBACKEVENT 0x280166C
#define CS40L25_RXREADYEVENT 0x2801670
#define CS40L25_HARDWAREEVENT 0x2801674
#define CS40L25_INDEXBUTTONPRESS 0x2801678
#define CS40L25_INDEXBUTTONRELEASE 0x2801688
#define CS40L25_ENDPLAYBACK 0x2801698
#define CS40L25_AUDIO_BLK_SIZE 0x280169C
#define CS40L25_BUILD_JOB_NAME 0x28016A0
#define CS40L25_BUILD_JOB_NUMBER 0x28016AC
#define CS40L25_GPIO_BUTTONDETECT 0x28016B0
#define CS40L25_EVENT_TIMEOUT 0x28016B4
#define CS40L25_PRESS_RELEASE_TIMEOUT 0x28016B8
#define CS40L25_GAIN_CONTROL 0x28016BC
#define CS40L25_GPIO_ENABLE 0x28016C0
#define CS40L25_POWERSTATE 0x28016C4
#define CS40L25_FALSEI2CTIMEOUT 0x28016C8
#define CS40L25_POWERONSEQUENCE 0x28016CC
#define CS40L25_VMONMAX 0x2801850
#define CS40L25_VMONMIN 0x2801854
#define CS40L25_IMONMAX 0x2801858
#define CS40L25_IMONMIN 0x280185C
#define CS40L25_USER_CONTROL_IPDATA 0x2801860
#define CS40L25_USER_CONTROL_RESPONSE 0x2801864
#define CS40L25_I2S_ENABLED 0x2801868
#define CS40L25_F0_STORED 0x280186C
#define CS40L25_REDC_STORED 0x2801870
#define CS40L25_F0_OFFSET 0x2801874
#define CS40L25_IRQMASKSEQUENCE 0x2801878
#define CS40L25_IRQMASKSEQUENCE_VALID 0x2801898
#define CS40L25_Q_STORED 0x280189C
#define CS40L25_VPMONMAX 0x28018A0
#define CS40L25_VPMONMIN 0x28018A4
#define CS40L25_VMON_IMON_OFFSET_ENABLE 0x28018A8
#define CS40L25_IMON_VMON_OFFSET_DELAY 0x28018AC
#define CS40L25_GPIO_GAIN 0x28018B0
#define CS40L25_SPK_FORCE_TST_1_AUTO 0x28018C0
#define CS40L25_GPIO_POL 0x28018C4
#define CS40L25_FORCEWATCHDOG 0x28018C8
#define CS40L25_FEATURE_BITMAP 0x28018CC
#endif

#ifdef CS40L25_ALGORITHM_VIBEGEN
#define CS40L25_VIBEGEN_XM_STRUCT_T 0x2800B4C
#define CS40L25_ENABLE 0x2800B4C
#define CS40L25_STATUS 0x2800B50
#define CS40L25_TIMEOUT_MS 0x2800B54
#define CS40L25_NUMBEROFWAVES 0x2800B58
#define CS40L25_COMPENSATION_ENABLE 0x2800B5C
#define CS40L25_WAVETABLE 0x2800B60
#define CS40L25_VIBEGEN_YM_STRUCT_T 0x3400000
#define CS40L25_WAVETABLEYM 0x3400000
#endif

#ifdef CS40L25_ALGORITHM_CLAB
#define CS40L25_CLAB_XM_STRUCT_T 0x2802910
#define CS40L25_CLAB_ENABLED 0x2802910
#define CS40L25_NUM_BRAKING_PULSES 0x2802914
#define CS40L25_COOL_OFF_DELAY 0x2802918
#define CS40L25_K_BRAKE 0x280291C
#define CS40L25_ATTENUATION_TO_STOP 0x2802920
#define CS40L25_BRAKE_COOL_OFF_DELAY 0x2802924
#define CS40L25_LOOP_DELAY 0x2802928
#define CS40L25_F0_MULTIPLIER 0x280292C
#define CS40L25_PEAK_AMPLITUDE_CONTROL 0x2802930
#define CS40L25_TUNED_DELAY 0x2802934
#define CS40L25_BRAKING_PULSE_DURATION 0x2802938
#define CS40L25_ZC_DURATION 0x280293C
#define CS40L25_BRAKING_PULSE_FREQ 0x2802940
#define CS40L25_INV_SAMPLING_FREQ 0x2802944
#endif

#ifdef CS40L25_ALGORITHM_DYNAMIC_F0
#define CS40L25_DYNAMIC_F0_XM_STRUCT_T 0x2802950
#define CS40L25_DYNAMIC_F0_ENABLED 0x2802950
#define CS40L25_IMONRINGPPTHRESHOLD 0x2802954
#define CS40L25_FRME_SKIP 0x2802958
#define CS40L25_NUM_PEAKS_TOFIND 0x280295C
#define CS40L25_DYN_F0_TABLE 0x2802960
#define CS40L25_DYNAMIC_REDC 0x28029B0
#endif

/** @} */

/**
 * Total blocks of CS40L25 Firmware
 */
#define cs40l25_total_fw_blocks (21)

/**
 * Total blocks of CS40L25 Coefficient 0 data
 */
#define cs40l25_total_coeff_blocks_clab (1)

/**
 * Total blocks of CS40L25 Coefficient 1 data
 */
#define cs40l25_total_coeff_blocks_wt (2)

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
extern const halo_boot_block_t cs40l25_fw_blocks[];

/**
 * Coefficient 0 memory block metadata
 */
extern const halo_boot_block_t cs40l25_coeff_blocks_clab[];

/**
 * Coefficient 1 memory block metadata
 */
extern const halo_boot_block_t cs40l25_coeff_blocks_wt[];

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/

#endif // CS40L25_FIRMWARE_H

