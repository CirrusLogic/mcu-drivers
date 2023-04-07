/**
 * @file cs40l26.h
 *
 * @brief Functions and prototypes exported by the CS40L26 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021-2023 All Rights Reserved, http://www.cirrus.com/
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

#ifndef CS40L26_H
#define CS40L26_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "fw_img.h"
#include "cs40l26_sym.h"
#include "cs40l26_cal_sym.h"
#include "cs40l26_spec.h"
#include "cs40l26_syscfg_regs.h"
#include "regmap.h"

#include "sdk_version.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L26_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#define CS40L26_STATUS_OK                               (0)
#define CS40L26_STATUS_FAIL                             (1)
/** @} */

/**
 * @defgroup CS40L26_FW_STATE_
 * @brief Firmware state of the driver
 *
 * @see cs40l26_t member state
 *
 * @{
 */
#define CS40L26_FW_STATE_ROM             (0)
#define CS40L26_FW_STATE_CAL             (1)
#define CS40L26_FW_STATE_RUN             (2)
/** @} */

/**
 * @defgroup CS40L26_POWER_STATE_
 * @brief Power_state of the driver
 *
 * @see cs40l26_t member state
 *
 * @{
 */
#define CS40L26_POWER_STATE_PREVENT_HIBERNATE           (0)
#define CS40L26_POWER_STATE_ALLOW_HIBERNATE             (1)
 /** @} */

/**
 * @defgroup CS40L26_MODE_
 * @brief Mode of the driver
 *
 * @see cs40l26_t member mode
 *
 * @{
 */
#define CS40L26_MODE_HANDLING_CONTROLS                  (0)
#define CS40L26_MODE_HANDLING_EVENTS                    (1)
/** @} */

/**
 * @defgroup CS40L26_EVENT_FLAG_
 * @brief Flags passed to Notification Callback to notify BSP of specific driver events
 *
 * @see cs40l26_notification_callback_t argument event_flags
 *
 * @{
 */
#define CS40L26_EVENT_FLAG_DSP_ERROR                    (1 << 31)
#define CS40L26_EVENT_FLAG_STATE_ERROR                  (1 << 30)
#define CS40L26_EVENT_FLAG_DSP_VIRTUAL2_MBOX            (1 << 29)
#define CS40L26_EVENT_FLAG_AMP_ERROR                    (1 << 27)
#define CS40L26_EVENT_FLAG_TEMP_ERROR                   (1 << 24)
#define CS40L26_EVENT_FLAG_BST_ERROR                    (1 << 18)
#define CS40L26_EVENT_FLAG_WKSRC_CP                     (1 << 1)
#define CS40L26_EVENT_FLAG_WKSRC_GPIO                   (1 << 0)
/** @} */

/**
 * @defgroup CS40L26_POWER_SEQ
 * @brief Values associated with power-on write sequencer
 *
 * @see cs40l26_wseq_*
 *
 * @{
 */
#define CS40L26_POWER_SEQ_LENGTH                         42
#define CS40L26_POWER_SEQ_MAX_WORDS                      129
#define CS40L26_POWER_SEQ_OP_WRITE_REG_FULL              0x00
#define CS40L26_POWER_SEQ_OP_WRITE_REG_FULL_WORDS        3
#define CS40L26_POWER_SEQ_OP_WRITE_FIELD                 0x01
#define CS40L26_POWER_SEQ_OP_WRITE_FIELD_WORDS           4
#define CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8             0x02
#define CS40L26_POWER_SEQ_OP_WRITE_REG_ADDR8_WORDS       2
#define CS40L26_POWER_SEQ_OP_WRITE_REG_INCR              0x03
#define CS40L26_POWER_SEQ_OP_WRITE_REG_INCR_WORDS        2
#define CS40L26_POWER_SEQ_OP_WRITE_REG_L16               0x04
#define CS40L26_POWER_SEQ_OP_WRITE_REG_L16_WORDS         2
#define CS40L26_POWER_SEQ_OP_WRITE_REG_H16               0x05
#define CS40L26_POWER_SEQ_OP_WRITE_REG_H16_WORDS         2
#define CS40L26_POWER_SEQ_OP_DELAY                       0xFE
#define CS40L26_POWER_SEQ_OP_DELAY_WORDS                 1
#define CS40L26_POWER_SEQ_OP_END                         0xFF
#define CS40L26_POWER_SEQ_OP_END_WORDS                   1

/**
 *  Minimum firmware version that will be accepted by the boot function
 */
#define CS40L26_MIN_FW_VERSION                           (0x70223)
#define CS40L26_CAL_MIN_FW_VERSION                       (0x1011B)
#define CS40L26_WT_ONLY                                  (0x12345)

/**
 *  Maximum length of mailbox queue
 */
#define CS40L26_MAILBOX_QUEUE_MAX_LEN                    (7)
/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/**
 * debug printf safe to use when semihosting is disabled
 */
#ifdef SEMIHOSTING
#define debug_printf(...) printf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * Available haptic effects banks
 *
 * @see cs40l26_trigger
 *
 */
typedef enum
{
    RAM_BANK      = 0,
    ROM_BANK      = 1,
    BUZZ_BANK     = 2,
    OWT_BANK      = 3
} cs40l26_wavetable_bank_t;

/**
 * Function pointer to Notification Callback
 *
 * @details
 * This callback will be registered at driver configuration.  This callback is called whenever the driver has detected
 * a significant event has occurred, such as an over-temperature condition.
 *
 * @see cs40l26_configure
 *
 * @param [in] event_flags      Flags to indicate which events have occurred
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs40l26_notification_callback_t)(uint32_t event_flags, void *arg);

/**
 * State of HALO FW Calibration
 *
 */
typedef struct
{
    bool is_valid_f0;   ///< (True) Calibration state is valid
    uint32_t f0;        ///< Encoded resonant frequency (f0) determined by Calibration procedure.
    uint32_t redc;      ///< Encoded DC resistance (ReDC) determined by Calibration procedure.
} cs40l26_calibration_t;

/**
* Entries used to write address value pairs to POWERONSEQUENCE.
*
* Write sequencer currently supports 4 V2 commands:
*
* WRITE_REG_FULL
* WRITE_REG_ADDR8
* WRITE_REG_L16
* WRITE_REG_H16
*
*/
typedef struct
{
    uint32_t operation;
    uint32_t size;
    uint32_t offset;
    uint32_t address;
    uint32_t value;
} cs40l26_wseq_entry_t;

/**
 * Configuration parameters required for calls to BSP-Driver Interface
 */
typedef struct
{
    uint32_t reset_gpio_id;                             ///< Used to ID CS40L26 Reset pin in bsp_driver_if calls
    uint32_t int_gpio_id;                               ///< Used to ID CS40L26 INT pin in bsp_driver_if calls
    cs40l26_notification_callback_t notification_cb;    ///< Notification callback registered for detected events
    void *notification_cb_arg;                          ///< Notification callback argument
    regmap_cp_config_t cp_config;                       ///< Control Port configuration for regmap calls
} cs40l26_bsp_config_t;

/**
 * Driver configuration data structure
 *
 * @see cs40l26_configure
 */
typedef struct
{
    cs40l26_bsp_config_t bsp_config;    ///< BSP Configuration
    uint32_t *syscfg_regs;              ///< Pointer to system configuration table
    uint32_t syscfg_regs_total;         ///< Total entries in system configuration table
    cs40l26_calibration_t cal_data;     ///< Calibration data from previous calibration sequence
    uint32_t bclk_freq;                 ///< Frequency of bclk
} cs40l26_config_t;

/**
 * Driver state data structure
 *
 * This is the type used for the handle to the driver for all driver public API calls.  This structure must be
 * instantiated outside the scope of the driver source and initialized by the cs40l26_initialize public API.
 */
typedef struct
{
    uint32_t fw_state;          ///< Firmware driver state - @see CS40L26_FW_STATE_
    uint32_t power_state;       ///< Power driver state - @see CS40L26_POWER_STATE_
    uint32_t mode;              ///< General driver mode - @see CS40L26_MODE_
    cs40l26_wseq_entry_t wseq_table[CS40L26_POWER_SEQ_LENGTH];
    uint32_t wseq_num_entries;
    bool wseq_written;
    uint32_t devid;             ///< CS40L26 DEVID of current device
    uint32_t revid;             ///< CS40L26 REVID of current device
    cs40l26_config_t config;    ///< Driver configuration fields - see cs40l26_config_t
    fw_img_info_t *fw_info;     ///< Current HALO FW/Coefficient boot configuration
    bool is_cal_boot;           ///< Mark for calibration firmware
    uint32_t event_flags;       ///< Most recent event_flags reported to BSP Notification callback
    uint32_t mailbox_queue[CS40L26_MAILBOX_QUEUE_MAX_LEN];
} cs40l26_t;

typedef union
{
    uint32_t word;
    struct
    {
        uint32_t asp_tx1_en                 : 1;
        uint32_t asp_tx2_en                 : 1;
        uint32_t asp_tx3_en                 : 1;
        uint32_t asp_tx4_en                 : 1;
        uint32_t reserved_0                 : 12;
        uint32_t asp_rx1_en                 : 1;
        uint32_t asp_rx2_en                 : 1;
        uint32_t asp_rx3_en                 : 1;
        uint32_t reserved_1                 : 13;
    };
} cs40l26_dataif_asp_enables1_t;

typedef union
{
    uint32_t word;
    struct
    {
        uint32_t pll_refclk_sel             : 3;
        uint32_t pll_refclk_en              : 1;
        uint32_t pll_refclk_freq            : 6;
        uint32_t pll_loop                   : 1;
    };
} cs40l26_ccm_refclk_input_t;
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
 * - CS40L26_STATUS_FAIL        if pointer to driver is NULL
 * - CS40L26_STATUS_OK          otherwise
 *
 */
uint32_t cs40l26_initialize(cs40l26_t *driver);

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
 * - CS40L26_STATUS_FAIL        if any pointers are NULL
 * - CS40L26_STATUS_OK          otherwise
 *
 */
uint32_t cs40l26_configure(cs40l26_t *driver, cs40l26_config_t *config);

/**
 * Processes driver events and notifications
 *
 * This implements Event Handling and BSP Notification
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return                      always CS40L26_STATUS_OK
 *
 * @warning This MUST be placed either in baremetal or RTOS task while (1)
 *
 */
uint32_t cs40l26_process(cs40l26_t *driver);

/**
 * Reset the CS40L26
 *
 * This call performs all necessary reset of the CS40L26 from power-on-reset to being able to process haptics in
 * Basic Haptics Mode (BHM).
 * - toggling RESET line
 * - checks DSP state
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS40L26_STATUS_FAIL if:
 *      - any control port activity fails
 *      - any status bit polling times out
 *      - the part is not supported
 * - otherwise, returns CS40L26_STATUS_OK
 *
 */
uint32_t cs40l26_reset(cs40l26_t *driver);

/**
 * Write block of data to the CS40L26 register file
 *
 * This call is used to load the HALO FW/COEFF files to HALO RAM.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Starting address of loading destination
 * @param [in] data             Pointer to array of bytes to be written
 * @param [in] size             Size of array of bytes to be written
 *
 * @return
 * - CS40L26_STATUS_FAIL if:
 *      - Any pointers are NULL
 *      - size is not multiple of 4
 *      - Control port activity fails
 * - otherwise, returns CS40L26_STATUS_OK
 *
 */
uint32_t cs40l26_boot(cs40l26_t *driver, fw_img_info_t *fw_info);

/**
 * Change the power state
 *
 * Based on the current driver state, this call will change the driver state and call the appropriate power up/down
 * function.  This can result in the part exiting/entering any of the following power states:  Power Up, Power Down,
 * Hibernate, Wake.
 *
 * @see CS40L26_POWER_
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] power_state      New power state
 *
 * @return
 * - CS40L26_STATUS_FAIL        if requested power_state is invalid, or if the call to change power state fails
 * - CS40L26_STATUS_OK          otherwise
 *
 */
uint32_t cs40l26_power(cs40l26_t *driver, uint32_t power_state);

/**
 * Calibrate the HALO Core DSP Protection Algorithm
 *
 * This performs the calibration procedure for Prince Haptic Control firmwares.
 * This calibration information (cs40l26_calibration_t) will be saved in the driver state
 * and applied during subsequent boots of the part.  This calibration information will be available to the driver
 * until the driver is re-initialized.
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] calib_type           The calibration type to be performed
 *
 * @return
 * - CS40L26_STATUS_FAIL        if any control port transaction fails
 * - CS40L26_STATUS_OK          otherwise
 *
 * @see cs40l26_calibration_t
 *
 */
uint32_t cs40l26_calibrate(cs40l26_t *driver);

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
 * - CS40L26_STATUS_FAIL        if any control port transaction fails
 * - CS40L26_STATUS_OK          otherwise
 *
 */
uint32_t cs40l26_trigger(cs40l26_t *driver, uint32_t index, cs40l26_wavetable_bank_t bank);

/**
 * Set properties of buzzgen waveform
 *
 * This will set the frequency, level and duration of a specific buzz effect
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] freq                 Frequency (Hz) of the buzz effect (range: [0:max(uint_t)])
 * @param [in] level                Amplitude of the buzz effect (255 = 10V, 127=5V) (range: [0:255])
 * @param [in] duration             Duration of the buzz effect in 4mS steps (1=4mS, 2=8mS ...) (range: [0:max(int_t)])
 * @param [in] buzzgen_num          Index of buzz to be modified (range: [0:5])
 *                                  Buzz Effect 1 is known as the "OTP Buzz". Effects 2:6 are known as the "RAM Buzz"
 *
 * @return
 * - CS40L26_STATUS_FAIL        if any control port transaction fails
 * - CS40L26_STATUS_OK          otherwise
 *
 */
uint32_t cs40l26_buzzgen_set(cs40l26_t *driver, uint16_t freq,
                             uint16_t level, uint16_t duration, uint8_t buzzgen_num);
uint32_t cs40l26_start_i2s(cs40l26_t *driver);
uint32_t cs40l26_stop_i2s(cs40l26_t *driver);
uint32_t cs40l26_load_waveform(cs40l26_t *driver);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L26_H
