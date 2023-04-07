/**
 * @file cs40l50.h
 *
 * @brief Functions and prototypes exported by the CS40L50 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2022-2023 All Rights Reserved, http://www.cirrus.com/
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

#ifndef CS40L50_H
#define CS40L50_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "fw_img.h"
#include "cs40l50_syscfg_regs.h"
#include "cs40l50_spec.h"
#include "regmap.h"
#include "rth_types.h"

#include "sdk_version.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L50_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#define CS40L50_STATUS_OK                               (0)
#define CS40L50_STATUS_FAIL                             (1)
/** @} */

/**
 * @defgroup CS40L50_POWER_STATE_
 * @brief Power_state of the driver
 *
 * @see cs40l50_t member state
 *
 * @{
 */
#define CS40L50_POWER_STATE_WAKE                    (0)
#define CS40L50_POWER_STATE_HIBERNATE               (1)
#define CS40L50_POWER_STATE_SHUTDOWN                (2)
/** @} */

/**
 * @defgroup CS40L50_MODE_
 * @brief Mode of the driver
 *
 * @see cs40l50_t member mode
 *
 * @{
 */
#define CS40L50_MODE_HANDLING_CONTROLS                  (0)
#define CS40L50_MODE_HANDLING_EVENTS                    (1)
/** @} */

/**
 * @defgroup CS40L50_POWER_
 * @brief Power states passed on to power() API argument power_state
 *
 * @see cs40l50_power
 *
 * @{
 */
#define CS40L50_POWER_UP                                (0)
#define CS40L50_POWER_DOWN                              (1)
#define CS40L50_POWER_HIBERNATE                         (2)
#define CS40L50_POWER_WAKE                              (3)
/** @} */

/**
 * @defgroup CS40L50_EVENT_FLAG_
 * @brief Flags passed to Notification Callback to notify BSP of specific driver events
 *
 * @see cs40l50_notification_callback_t argument event_flags
 *
 * @{
 */
#define CS40L50_EVENT_FLAG_DSP_ERROR                    (1 << 31)
/** @} */

/**
 * @defgroup CS40L50_POLL_
 * @brief Polling constants for polling times and counts
 *
 * @{
 */
#define CS40L50_POLL_ACK_CTRL_MS                (1)     ///< Delay in ms between polling for ACKed memory writes
#define CS40L50_POLL_ACK_CTRL_MAX               (100)   ///< Maximum number of times to poll for ACKed memory writes
/** @} */

/**
 *  Minimum firmware version that will be accepted by the boot function
 */
#define CS40L50_MIN_FW_VERSION     (0x30405)
#define CS40L50_WT_ONLY            (0x12345)

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

/**
 * @defgroup CS40L50_EVENT_FLAG_
 * @brief Flags passed to Notification Callback to notify BSP of specific driver events
 *
 * @see CS40L50_notification_callback_t argument event_flags
 *
 * @{
 */
#define CS40L50_EVENT_FLAG_DSP_ERROR                    (1 << 31)
#define CS40L50_EVENT_FLAG_STATE_ERROR                  (1 << 30)
#define CS40L50_EVENT_FLAG_RUNTIME_SHORT_DETECTED       (1 << 23)
#define CS40L50_EVENT_FLAG_PERMANENT_SHORT_DETECTED     (1 << 22)
#define CS40L50_EVENT_FLAG_AWAKE                        (1 << 21)
#define CS40L50_EVENT_FLAG_INIT_COMPLETE                (1 << 20)
#define CS40L50_EVENT_FLAG_HAPTIC_COMPLETE_GPIO         (1 << 19)
#define CS40L50_EVENT_FLAG_HAPTIC_TRIGGER_GPIO          (1 << 18)
#define CS40L50_EVENT_FLAG_HAPTIC_COMPLETE_MBOX         (1 << 17)
#define CS40L50_EVENT_FLAG_HAPTIC_TRIGGER_MBOX          (1 << 16)
#define CS40L50_EVENT_FLAG_HAPTIC_COMPLETE_I2S          (1 << 15)
#define CS40L50_EVENT_FLAG_HAPTIC_TRIGGER_I2S           (1 << 14)
#define CS40L50_EVENT_FLAG_AMP_ERROR                    (1 << 2)
#define CS40L50_EVENT_FLAG_TEMP_ERROR                   (1 << 1)
#define CS40L50_EVENT_FLAG_BST_ERROR                    (1 << 0)
/** @} */

/**
 * Default value of Dynamic F0 table entry
 */
#define CS40L50_DYNAMIC_F0_TABLE_ENTRY_DEFAULT          (0x007FE000)

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/
/**
 * Available haptic effects banks
 *
 * @see cs40l50_trigger
 *
 */
typedef enum
{
    ROM_BANK      = 0,
    RAM_BANK      = 1
} cs40l50_wavetable_bank_t;

/**
 * Available gpios to configure their triggered waveform
 *
 * @see cs40l50_configure_gpio_trigger
 *
 */
typedef enum
{
    GPIO3_RISE,
    GPIO3_FALL,
    GPIO4_RISE,
    GPIO4_FALL,
    GPIO5_RISE,
    GPIO5_FALL,
    GPIO6_RISE,
    GPIO6_FALL,
    GPIO10_RISE,
    GPIO10_FALL,
    GPIO11_RISE,
    GPIO11_FALL,
    GPIO12_RISE,
    GPIO12_FALL,
    GPIO13_RISE,
    GPIO13_FALL
} cs40l50_gpio_bank_t;

/**
 * Function pointer to Notification Callback
 *
 * @details
 * This callback will be registered at driver configuration.  This callback is called whenever the driver has detected
 * a significant event has occurred, such as an over-temperature condition.
 *
 * @see cs40l50_configure
 *
 * @param [in] event_flags      Flags to indicate which events have occurred
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs40l50_notification_callback_t)(uint32_t event_flags, void *arg);

/**
 * State of HALO FW Calibration
 *
 */
typedef struct
{
    bool is_valid;   ///< (True) Calibration state is valid
    uint32_t f0;        ///< Encoded resonant frequency (f0) determined by Calibration procedure.
    uint32_t redc;      ///< Encoded DC resistance (ReDC) determined by Calibration procedure.
} cs40l50_calibration_t;

/**
 * Configuration parameters required for calls to BSP-Driver Interface
 */
typedef struct
{
    uint32_t reset_gpio_id;                             ///< Used to ID CS35L42 Reset pin in bsp_driver_if calls
    uint32_t int_gpio_id;                               ///< Used to ID CS35L42 INT pin in bsp_driver_if calls
    cs40l50_notification_callback_t notification_cb;    ///< Notification callback registered for detected events
    void *notification_cb_arg;                          ///< Notification callback argument
    regmap_cp_config_t cp_config;
} cs40l50_bsp_config_t;

/**
 * Driver configuration data structure
 *
 * @see cs40l50_configure
 */
typedef struct
{
    cs40l50_bsp_config_t bsp_config;    ///< BSP Configuration
    uint32_t *syscfg_regs;              ///< Pointer to array of configuration register/value pairs
    uint32_t syscfg_regs_total;         ///< Total pairs in syscfg_regs[]
    cs40l50_calibration_t cal_data;     ///< Calibration data from previous calibration sequence
    bool is_ext_bst;                    ///< Indicates whether the device is internal or external boost
    bool enable_mbox_irq;               ///< Enable IRQ for MBOX after device reset
    uint32_t dynamic_f0_threshold;      ///< imonRingPPThreshold
} cs40l50_config_t;

/**
 * Driver state data structure
 *
 * This is the type used for the handle to the driver for all driver public API calls.  This structure must be
 * instantiated outside the scope of the driver source and initialized by the cs40l50_initialize public API.
 */
typedef struct
{
    uint32_t fw_state;                          ///< Firmware driver state - @see CS40L50_FW_STATE_
    uint32_t power_state;                       ///< Power driver state - @see CS40L50_POWER_STATE_
    uint32_t mode;                              ///< General driver mode - @see CS40L50_MODE_
    // Driver configuration fields - see cs40l50_config_t
    cs40l50_config_t config;                    ///< Driver configuration fields - see cs40l50_config_t

    // Extra state material used by reset and boot
    uint32_t devid;                             ///< CS40L50 DEVID of current device
    uint32_t revid;                             ///< CS40L50 REVID of current device
    fw_img_info_t *fw_info;                     ///< Current HALO FW/Coefficient boot configuration

    uint32_t event_flags;                       ///< Most recent event_flags reported to BSP Notification callback
} cs40l50_t;

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

/**
 * Dynamic F0 table1 entry type
 */
typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t f0                         : 13;   ///< F0 in Q10.3 format
            uint32_t index                      : 8;    ///< Index in Wave Table
            uint32_t wave_in_owt                : 1;    ///< Waveform is OWT Entry
            uint32_t wave_in_rom                : 1;    ///< Waveform is ROM Entry
            uint32_t reserved                   : 9;
        };
    };
} cs40l50_df0_table1_entry_t;

/**
 * Dynamic F0 table2 entry type
 *
 * Table holding design F0 and design ReDC values for each dyn_f0_table1 entry
 */
typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t design_redc_stored         : 12;   ///< (ReDC / VImon ratio) * 128; VImon ratio = 8.276; ReDC = (0, 31.989 Ohms)
            uint32_t design_f0_stored           : 12;   ///< F0 Stored value = (frequency - 50) * 8
            uint32_t reserved                   : 8;
        };
    };
} cs40l50_df0_table2_entry_t;

/**
 * Dynamic F0 table entry type
 */
typedef struct
{
    struct
    {
        cs40l50_df0_table1_entry_t table1;
        cs40l50_df0_table2_entry_t table2;
        uint32_t table3;                    ///< Table holding attenuation factors for each dyn_f0_table1 entry. (1, 100)
    };
} cs40l50_df0_table_entry_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
/**
 * Initialize driver state/handle
 *
 * Sets all driver state members to 0
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS40L50_STATUS_FAIL        if pointer to driver is NULL
 * - CS40L50_STATUS_OK          otherwise
 *
 */
uint32_t cs40l50_initialize(cs40l50_t *driver);

/**
 * Configures driver state/handle
 *
 * Including the following:
 * - Applies all one-time configurations to the driver state
 * - Registers the IRQ Callback for INTb GPIO with the BSP
 * - Applies calibration data (if valid) to the driver state
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] config           Pointer to driver configuration data structure
 *
 * @return
 * - CS40L50_STATUS_FAIL        if any pointers are NULL
 * - CS40L50_STATUS_OK          otherwise
 *
 */
uint32_t cs40l50_configure(cs40l50_t *driver, cs40l50_config_t *config);

/**
 * Processes driver events and notifications
 *
 * This implements Event Handling and BSP Notification
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - if in UNCONFIGURED or ERROR state, returns CS40L50_STATUS_OK
 * - else if in HANDLING_CONTROLS mode, returns CS40L50_STATUS_OK
 * - otherwise, returns status Event Handler
 *
 * @warning This MUST be placed either in baremetal or RTOS task while (1)
 *
 */
uint32_t cs40l50_process(cs40l50_t *driver);

/**
 * Reset the CS40L50
 *
 * This call performs all necessary reset of the CS40L50 from power-on-reset to being able to process haptics in
 * Basic Haptics Mode (BHM).
 * - toggling RESET line
 * - verifying entry to BHM is successful
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS40L50_STATUS_FAIL if:
 *      - any control port activity fails
 *      - any status bit polling times out
 *      - the part is not supported
 * - otherwise, returns CS40L50_STATUS_OK
 *
 */
uint32_t cs40l50_reset(cs40l50_t *driver);

/**
 * Finish booting the CS40L50
 *
 * While cs40l50_write_block loads the actual FW/COEFF data into HALO RAM, cs40l50_boot will finish the boot process
 * by:
 * - loading the fw_img_info_t fw_info member of the driver handle
 * - Performing any post-boot configuration writes
 * - Loading Calibration data (if valid)
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] fw_info          Pointer to FW information and FW Control Symbol Table
 *
 * @return
 * - CS40L50_STATUS_FAIL if:
 *      - Any pointers are null
 *      - Control port activity fails
 *      - Required FW Control symbols are not found in the symbol table
 * - CS40L50_STATUS_OK          otherwise
 *
 */
uint32_t cs40l50_boot(cs40l50_t *driver, fw_img_info_t *fw_info);

/**
 * Change the power state
 *
 * Based on the current driver state, this call will change the driver state and call the appropriate power up/down
 * function.  This can result in the part exiting/entering any of the following power states:  Power Up, Standby,
 * Hibernate, Wake.
 *
 * @see CS40L50_POWER_
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] power_state      New power state
 *
 * @warning CS40L50_POWER_DOWN should only be used when exiting BHM mode or switching between firmware or coefficient
 * files.  For low power mode while running firmware, CS40L50_POWER_HIBERNATE should be used.
 *
 * @return
 * - CS40L50_STATUS_FAIL        if requested power_state is invalid, or if the call to change power state fails
 * - CS40L50_STATUS_OK          otherwise
 *
 */
uint32_t cs40l50_power(cs40l50_t *driver, uint32_t power_state);

/**
 * Sets the timeout ticks for hibernate.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] ms               Duration before hibernation when allowing hibernate
 *
 * @return
 * - CS40L50_STATUS_FAIL        if any control port activity fails
 * - CS40L50_STATUS_OK          otherwise
 */
uint32_t cs40l50_timeout_ticks_set(cs40l50_t *driver, uint32_t ms);

/**
 * Calibrate the HALO Core DSP Protection Algorithm
 *
 * This performs the calibration procedure for Prince Haptic Control firmwares.
 * This calibration information (cs40l50_calibration_t) will be saved in the driver state
 * and applied during subsequent boots of the part.  This calibration information will be available to the driver
 * until the driver is re-initialized.
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] calib_type           The calibration type to be performed
 *
 * @return
 * - CS40L50_STATUS_FAIL        if driver in invalid state for calibration, or any control port activity fails
 * - CS40L50_STATUS_OK          otherwise
 *
 * @see cs40l50_calibration_t
 *
 */
uint32_t cs40l50_calibrate(cs40l50_t *driver);

/**
 * Sets a given ReDC value to the REDC_OTP_STORED register
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] redc                 Value of ReDC to be written. In Q7.17 format and in units Ohm * 2.9/24
 *
 * @return
 * - CS40L50_STATUS_FAIL        if writing the value fails
 * - CS40L50_STATUS_OK          otherwise
 */
uint32_t cs40l50_set_redc(cs40l50_t *driver, uint32_t redc);

/**
 * Sets a given F0 value to the F0_OTP_STORED register
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] f0                   Value of F0 to be written. In Q10.14 format and in units Hz
 *
 * @return
 * - CS40L50_STATUS_FAIL        if writing the value fails
 * - CS40L50_STATUS_OK          otherwise
 */
uint32_t cs40l50_set_f0(cs40l50_t *driver, uint32_t f0);

/**
 * Trigger haptic effect
 *
 * This will trigger a haptic effect from either the ROM or RAM wavetable
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] index                Index into the wavetable
 * @param [in] bank                 Inidicates which wavetable bank to trigger from
 *
 * @return
 * - CS40L50_STATUS_FAIL        if any control port transaction fails
 * - CS40L50_STATUS_OK          otherwise
 *
 */
uint32_t cs40l50_trigger(cs40l50_t *driver, uint32_t index, cs40l50_wavetable_bank_t bank);

uint32_t cs40l50_configure_gpio_trigger(cs40l50_t *driver, cs40l50_gpio_bank_t gpio, bool rth,
                                        uint8_t attenuation, bool ram, uint8_t plybck_index);

uint32_t cs40l50_set_click_compensation_enable(cs40l50_t *driver, bool f0_enable, bool redc_enable);
uint32_t cs40l50_trigger_pwle(cs40l50_t *driver, rth_pwle_section_t **s);
uint32_t cs40l50_trigger_pwle_advanced(cs40l50_t *driver, rth_pwle_section_t **s, uint8_t repeat, uint8_t num_sections);
uint32_t cs40l50_trigger_pcm(cs40l50_t *driver, uint8_t *s, uint32_t num_sections, uint16_t buffer_size_samples, uint16_t f0, uint16_t redc);
uint32_t cs40l50_set_dynamic_f0(cs40l50_t *driver, bool enable);
uint32_t cs40l50_get_dynamic_f0(cs40l50_t *driver, cs40l50_df0_table_entry_t *f0_entry);

/*
 * Reads the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Address of the register to be read
 * @param [out] val             Pointer to where the read register value will be stored
 *
 * @return
 * - CS40L50_STATUS_FAIL if:
 *      - Control port activity fails
 * - otherwise, returns CS40L50_STATUS_OK
 *
 */
uint32_t cs40l50_read_reg(cs40l50_t *driver, uint32_t addr, uint32_t *val);

/*
 * Writes the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Address of the register to be written
 * @param [in] val              Value to be written to the register
 *
 * @return
 * - CS40L50_STATUS_FAIL if:
 *      - Control port activity fails
 * - otherwise, returns CS40L50_STATUS_OK
 *
 */
uint32_t cs40l50_write_reg(cs40l50_t *driver, uint32_t addr, uint32_t val);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L50_H
