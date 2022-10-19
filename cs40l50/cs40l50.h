/**
 * @file cs40l50.h
 *
 * @brief Functions and prototypes exported by the CS40L50 Driver module
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
 * @defgroup CS40L26_POWER_STATE_
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
#define CS40L50_MIN_FW_VERSION     (0x70223)
#define CS40L50_WT_ONLY            (0x12345)


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
    bool is_valid_f0;   ///< (True) Calibration state is valid
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
    cs40l50_bsp_config_t bsp_config;                    ///< BSP Configuration
    uint32_t *syscfg_regs;                              ///< Pointer to array of configuration register/value pairs
    uint32_t syscfg_regs_total;                         ///< Total pairs in syscfg_regs[]
    cs40l50_calibration_t cal_data;     ///< Calibration data from previous calibration sequence
    bool is_ext_bst;                    ///< Indicates whether the device is internal or external boost
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
