/**
 * @file cs35l42.h
 *
 * @brief Functions and prototypes exported by the CS35L42 Driver module
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

#ifndef CS35L42_H
#define CS35L42_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "fw_img.h"
#include "cs35l42_syscfg_regs.h"
#include "cs35l42_spec.h"
#include "regmap.h"

#include "sdk_version.h"

/***********************************************************************************************************************
 * LITERALS, CONSTANTS, MACROS
 **********************************************************************************************************************/

/**
 * @defgroup CS35L42_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#define CS35L42_STATUS_OK                               (0)
#define CS35L42_STATUS_FAIL                             (1)
/** @} */

/**
 * @defgroup CS35L42_STATE_
 * @brief State of the driver
 *
 * @see cs35l42_t member state
 *
 * @{
 */
#define CS35L42_STATE_UNCONFIGURED                      (0)
#define CS35L42_STATE_CONFIGURED                        (1)
#define CS35L42_STATE_STANDBY                           (2)
#define CS35L42_STATE_POWER_UP                          (3)
#define CS35L42_STATE_ERROR                             (4)
#define CS35L42_STATE_DSP_POWER_UP                      (5)
#define CS35L42_STATE_DSP_STANDBY                       (6)
#define CS35L42_STATE_HIBERNATE                         (7)
/** @} */

/**
 * @defgroup CS35L42_MODE_
 * @brief Mode of the driver
 *
 * @see cs35l42_t member mode
 *
 * @{
 */
#define CS35L42_MODE_HANDLING_CONTROLS                  (0)
#define CS35L42_MODE_HANDLING_EVENTS                    (1)
/** @} */

#define CS35L42_POLL_OTP_BOOT_DONE_MS                   (10)        ///< Delay in ms between polling OTP_BOOT_DONE
#define CS35L42_POLL_OTP_BOOT_DONE_MAX                  (10)        ///< Maximum number of times to poll OTP_BOOT_DONE
#define CS35L42_OTP_SIZE_BYTES                          (32 * 4)    ///< Total size of CS35L42 OTP in bytes

/**
 * @defgroup CS35L42_POWER_
 * @brief Power states passed on to power() API argument power_state
 *
 * @see cs35l42_power
 *
 * @{
 */
#define CS35L42_POWER_UP                                (0)
#define CS35L42_POWER_DOWN                              (1)
#define CS35L42_POWER_HIBERNATE                         (2)
#define CS35L42_POWER_WAKE                              (3)
/** @} */

/**
 * @defgroup CS35L42_EVENT_FLAG_
 * @brief Flags passed to Notification Callback to notify BSP of specific driver events
 *
 * @see cs35l42_notification_callback_t argument event_flags
 *
 * @{
 */
 #define CS35L42_EVENT_FLAG_STATE_ERROR                  (0)
/** @} */

#define CS35L42_DSP_STATUS_WORDS_TOTAL                  (9)     ///< Total registers to read for Get DSP Status control

#define CS35L42_CONTROL_PORT_MAX_PAYLOAD_BYTES          (4140)  ///< Maximum bytes CS35L42 can transfer

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

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
 * @see cs35l42_configure
 * @see cs35l42_bsp_config_t
 *
 * @param [in] event_flags      Flags to indicate which events have occurred
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs35l42_notification_callback_t)(uint32_t event_flags, void *arg);

/**
 * Configuration parameters required for calls to BSP-Driver Interface
 */
typedef struct
{
    uint32_t reset_gpio_id;                             ///< Used to ID CS35L42 Reset pin in bsp_driver_if calls
    uint32_t int_gpio_id;                               ///< Used to ID CS35L42 INT pin in bsp_driver_if calls
    cs35l42_notification_callback_t notification_cb;    ///< Notification callback registered for detected events
    void *notification_cb_arg;                          ///< Notification callback argument
    regmap_cp_config_t cp_config;                       ///< Regmap control port configuration
} cs35l42_bsp_config_t;

/**
 * Driver configuration data structure
 *
 * @see cs35l42_configure
 */
typedef struct
{
    cs35l42_bsp_config_t bsp_config;    ///< BSP Configuration
    const uint32_t *syscfg_regs;    ///< Pointer to array of configuration register/value pairs
    uint32_t syscfg_regs_total;         ///< Total pairs in syscfg_regs[]
} cs35l42_config_t;

/**
 * Driver state data structure
 *
 * This is the type used for the handle to the driver for all driver public API calls.  This structure must be
 * instantiated outside the scope of the driver source and initialized by the initialize() public API.
 */
typedef struct
{
    // Primary driver state fields
    uint32_t state;     ///< General driver state - @see CS35L42_STATE_
    uint32_t mode;      ///< General driver mode - @see CS35L42_MODE_

    // Driver configuration fields - see cs35l42_config_t
    cs35l42_config_t config;

    // Extra state material used by reset and boot
    uint32_t devid;                     ///< CS35L42 DEVID of current device
    uint32_t revid;                     ///< CS35L42 REVID of current device
    fw_img_info_t *fw_info;             ///< Current HALO FW/Coefficient boot configuration

    uint32_t event_flags;               ///< Flags set by Event Handler that are passed to noticiation callback
    uint8_t otp_contents[CS35L42_OTP_SIZE_BYTES];   ///< Cache storage for OTP contents
} cs35l42_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize driver state/handle
 *
 * Sets all driver state members to 0, and initializes Control Request Queue.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L42_STATUS_FAIL        if pointer to driver is NULL
 * - CS35L42_STATUS_OK          otherwise
 *
 */
uint32_t cs35l42_initialize(cs35l42_t *driver);

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
 * - CS35L42_STATUS_FAIL        if any pointers are NULL
 * - CS35L42_STATUS_OK          otherwise
 *
 */
uint32_t cs35l42_configure(cs35l42_t *driver, cs35l42_config_t *config);

/**
 * Processes driver states and modes
 *
 * This implements the 'CS35L42 Process Flowchart' found in the driver Tech Note.  This includes:
 * - calling Event Handler if in HANDLING_EVENTS mode
 * - calling BSP Notification Callback to notify of any events or error conditions
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - if in UNCONFIGURED or ERROR state, returns CS35L42_STATUS_OK
 * - else if in HANDLING_CONTROLS mode, returns CS35L42_STATUS_OK
 * - otherwise, returns status Event Handler
 *
 * @warning This MUST be placed either in baremetal or RTOS task while (1)
 *
 */
uint32_t cs35l42_process(cs35l42_t *driver);

/**
 * Reset the CS35L42 and prepare for HALO FW booting
 *
 * This call performs all necessary reset of the CS35L42 from power-on-reset to prepare the part for loading HALO
 * FW.  This includes:
 * - toggling RESET line
 * - Application of proper errata configuration
 * - OTP unpacking
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L42_STATUS_FAIL if:
 *      - any control port activity fails
 *      - any status bit polling times out
 *      - the part is not supported
 *      - no OTP unpacking map exists for the part
 * - otherwise, returns CS35L42_STATUS_OK
 *
 */
uint32_t cs35l42_reset(cs35l42_t *driver);

/**
 * Finish booting the CS35L42
 *
 * While cs35l42_write_block loads the actual FW/COEFF data into HALO RAM, cs35l42_boot will finish the boot process
 * by:
 * - loading the fw_img_info_t fw_info member of the driver handle
 * - Performing any post-boot configuration writes
 * - Loading Calibration data (if valid)
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] fw_info          Pointer to FW information and FW Control Symbol Table
 *
 * @return
 * - CS35L42_STATUS_FAIL if:
 *      - Any pointers are null
 *      - Control port activity fails
 *      - Required FW Control symbols are not found in the symbol table
 * - CS35L42_STATUS_OK          otherwise
 *
 */
uint32_t cs35l42_boot(cs35l42_t *driver, fw_img_info_t *fw_info);

/**
 * Change the power state
 *
 * Based on the current driver state, this call will change the driver state and call the appropriate power up/down
 * function.  This can result in the part exiting/entering any of the following power states:  Power Up, Standby,
 * Hibernate.
 *
 * @see CS35L42_POWER_
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] power_state      New power state
 *
 * @return
 * - CS35L42_STATUS_FAIL        if requested power_state is invalid, or if the call to change power state fails
 * - CS35L42_STATUS_OK          otherwise
 *
 */
uint32_t cs35l42_power(cs35l42_t *driver, uint32_t power_state);

/*
 * Reads the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Address of the register to be read
 * @param [out] val             Pointer to where the read register value will be stored
 *
 * @return
 * - CS35L42_STATUS_FAIL if:
 *      - Control port activity fails
 * - otherwise, returns CS35L42_STATUS_OK
 *
 */
uint32_t cs35l42_read_reg(cs35l42_t *driver, uint32_t addr, uint32_t *val);

/*
 * Writes the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Address of the register to be written
 * @param [in] val              Value to be written to the register
 *
 * @return
 * - CS35L42_STATUS_FAIL if:
 *      - Control port activity fails
 * - otherwise, returns CS35L42_STATUS_OK
 *
 */
uint32_t cs35l42_write_reg(cs35l42_t *driver, uint32_t addr, uint32_t val);

/*
 * Reads, updates and writes (if there's a change) the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Address of the register to be written
 * @param [in] mask             Mask of the bits within the register to update
 * @param [in] val              Value to be written to the register (only bits matching the mask will be written)
 *
 * @return
 * - CS35L42_STATUS_FAIL if:
 *      - Control port activity fails
 * - otherwise, returns CS35L42_STATUS_OK
 *
 */
uint32_t cs35l42_update_reg(cs35l42_t *driver, uint32_t addr, uint32_t mask, uint32_t val);

/*
 * Write block of data to the CS35L42 register file
 *
 * This call is used to load the HALO FW/COEFF files to HALO RAM.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Starting address of loading destination
 * @param [in] data             Pointer to array of bytes to be written
 * @param [in] size             Size of array of bytes to be written
 *
 * @return
 * - CS35L42_STATUS_FAIL if:
 *      - Any pointers are NULL
 *      - size is not multiple of 4
 *      - Control port activity fails
 * - otherwise, returns CS35L42_STATUS_OK
 *
 */
uint32_t cs35l42_write_block(cs35l42_t *driver, uint32_t addr, uint8_t *data, uint32_t size);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L42_H
