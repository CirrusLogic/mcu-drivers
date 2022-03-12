/**
 * @file cs40l26.h
 *
 * @brief Functions and prototypes exported by the CS40L26 Driver module
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
#define CS40L26_POWER_STATE_WAKE                    (0)
#define CS40L26_POWER_STATE_HIBERNATE               (1)
#define CS40L26_POWER_STATE_SHUTDOWN                (2)
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
 * @defgroup CS40L26_POWER_
 * @brief Power states passed on to power() API argument power_state
 *
 * @see cs40l26_power
 *
 * @{
 */
#define CS40L26_POWER_UP                                (0)
#define CS40L26_POWER_DOWN                              (1)
#define CS40L26_POWER_HIBERNATE                         (2)
#define CS40L26_POWER_WAKE                              (3)
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
#define CS40L26_EVENT_FLAG_WKSRC_CP                     (1 << 1)
#define CS40L26_EVENT_FLAG_WKSRC_GPIO                   (1 << 0)
/** @} */

/**
 *  Minimum firmware version that will be accepted by the boot function
 */
#define CS40L26_MIN_FW_VERSION     (0x7021B)

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
    uint32_t devid;             ///< CS40L26 DEVID of current device
    uint32_t revid;             ///< CS40L26 REVID of current device
    cs40l26_config_t config;    ///< Driver configuration fields - see cs40l26_config_t
    fw_img_info_t *fw_info;     ///< Current HALO FW/Coefficient boot configuration
    uint32_t event_flags;       ///< Most recent event_flags reported to BSP Notification callback
} cs40l26_t;

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

/*
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
 * @param [in] is_rom               Inidicates ROM wavetable (true) or RAM
 *                                  wavetable (false)
 *
 * @return
 * - CS40L26_STATUS_FAIL        if any control port transaction fails
 * - CS40L26_STATUS_OK          otherwise
 *
 */
uint32_t cs40l26_trigger(cs40l26_t *driver, uint32_t index, bool is_rom);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L26_H
