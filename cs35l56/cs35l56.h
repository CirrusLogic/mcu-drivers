/**
 * @file cs35l56.c
 *
 * @brief Functions and prototypes exported by the CS35L56 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2026 All Rights Reserved, http://www.cirrus.com/
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

#ifndef CS35L56_H
#define CS35L56_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "fw_img.h"
#include "cs35l56_spec.h"
#include "rth_types.h"
#ifdef CIRRUS_SDK
#include "regmap.h"
#endif

#ifdef ZEPHYR_INCLUDE_KERNEL_H
#include "cs35l56_bsp.h"
#endif

#include "sdk_version.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS35L56_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#define CS35L56_STATUS_OK   (0)
#define CS35L56_STATUS_FAIL (1)
/** @} */

/**
 * @defgroup CS35L56_DSP_STATE_
 * @brief State to read back after booting DSP
 *
 * @{
 */
#define CS35L56_DSP_STATE_RUNNING (0x2)
#define CS35L56_DSP_STATE_FAULT   (0x6)
/** @} */

/**
 * @defgroup CS35L56_POWER_STATE_
 * @brief Power_state of the driver
 *
 * @see cs35l56_t member state
 *
 * @{
 */
#define CS35L56_POWER_STATE_WAKE      (0)
#define CS35L56_POWER_STATE_HIBERNATE (1)
#define CS35L56_POWER_STATE_SHUTDOWN  (2)
/** @} */

/**
 * @defgroup CS35L56_POWER_
 * @brief Power states passed on to power() API argument power_state
 *
 * @see cs35l56_power
 *
 * @{
 */
#define CS35L56_POWER_UP        (0)
#define CS35L56_POWER_DOWN      (1)
#define CS35L56_POWER_HIBERNATE (2)
#define CS35L56_POWER_WAKE      (3)
/** @} */

/**
 * @defgroup CS35L56_POLL_
 * @brief Polling constants for polling times and counts
 *
 * @{
 */
#define CS35L56_POLL_ACK_CTRL_MS  (1)   ///< Delay in ms between polling for ACKed memory writes
#define CS35L56_POLL_ACK_CTRL_MAX (100) ///< Maximum number of times to poll for ACKed memory writes
#define CS35L56_BOOT_TIMEMOUT_MS                                                                   \
    (10) ///< Maximum delay in ms between attempts reading back successful DSP boot
/** @} */

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
 * This callback will be registered at driver configuration.  This callback is called whenever the
 * driver has detected a significant event has occurred, such as an over-temperature condition.
 *
 * @see cs35l56_configure
 *
 * @param [in] event_flags      Flags to indicate which events have occurred
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs35l56_notification_callback_t)(uint32_t event_flags, void *arg);

/**
 * State of HALO FW Calibration
 *
 */
typedef struct {
    bool is_valid; ///< (True) Calibration state is valid
    uint32_t f0;   ///< Encoded resonant frequency (f0) determined by Calibration procedure.
    uint32_t redc; ///< Encoded DC resistance (ReDC) determined by Calibration procedure.
} cs35l56_calibration_t;

/**
 * Configuration parameters required for calls to BSP-Driver Interface
 */
typedef struct {
#ifdef CIRRUS_ZEPHYR_SAMPLE
    const struct i2c_dt_spec *i2c;
    const struct gpio_dt_spec
        *reset_gpio_id; ///< Used to interact with CS35L56 Reset pin in zephyr bsp
    const struct gpio_dt_spec
        *int_gpio_id; ///< Used to interact with CS35L56 INT pin in zephyr bsp
#else
    uint32_t reset_gpio_id; ///< Used to ID CS35L56 Reset pin in bsp_driver_if calls
    uint32_t int_gpio_id;   ///< Used to ID CS35L56 INT pin in bsp_driver_if calls
#endif
    cs35l56_notification_callback_t
        notification_cb;   ///< Notification callback registered for detected events
    void *notification_cb_arg; ///< Notification callback argument
#ifdef CIRRUS_SDK
    regmap_cp_config_t cp_config;
#endif
} cs35l56_bsp_config_t;

/**
 * Driver configuration data structure
 *
 * @see cs35l56_configure
 */
typedef struct {
    cs35l56_bsp_config_t bsp_config; ///< BSP Configuration
    uint32_t *syscfg_regs;           ///< Pointer to array of configuration register/value pairs
    uint32_t syscfg_regs_total;      ///< Total pairs in syscfg_regs[]
    cs35l56_calibration_t cal_data;  ///< Calibration data from previous calibration sequence
    bool is_ext_bst;      ///< Indicates whether the device is internal or external boost
    bool enable_mbox_irq; ///< Enable IRQ for MBOX after device reset
    uint32_t dynamic_f0_threshold; ///< imonRingPPThreshold
    bool broadcast;                ///< Broadcast I2C data and triggers
} cs35l56_config_t;

/**
 * Driver state data structure
 *
 * This is the type used for the handle to the driver for all driver public API calls.  This
 * structure must be instantiated outside the scope of the driver source and initialized by the
 * cs35l56_initialize public API.
 */
typedef struct {
    uint32_t fw_state;    ///< Firmware driver state - @see CS35L56_FW_STATE_
    uint32_t power_state; ///< Power driver state - @see CS35L56_POWER_STATE_
    uint32_t mode;        ///< General driver mode - @see CS35L56_MODE_
    // Driver configuration fields - see cs35l56_config_t
    cs35l56_config_t config; ///< Driver configuration fields - see cs35l56_config_t

    // Extra state material used by reset and boot
    uint32_t devid;         ///< CS35L56 DEVID of current device
    uint32_t revid;         ///< CS35L56 REVID of current device
    fw_img_info_t *fw_info; ///< Current HALO FW/Coefficient boot configuration

    uint32_t event_flags; ///< Most recent event_flags reported to BSP Notification callback
} cs35l56_t;

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
 * - CS35L56_STATUS_FAIL        if pointer to driver is NULL
 * - CS35L56_STATUS_OK          otherwise
 *
 */
uint32_t cs35l56_initialize(cs35l56_t *driver);

/**
 * Reset the CS35L56
 *
 * This call performs all necessary reset of the CS35L56 from power-on-reset to being able to
 * process haptics in Basic Haptics Mode (BHM).
 * - toggling RESET line
 * - verifying entry to BHM is successful
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L56_STATUS_FAIL if:
 *      - any control port activity fails
 *      - any status bit polling times out
 *      - the part is not supported
 * - otherwise, returns CS35L56_STATUS_OK
 *
 */
uint32_t cs35l56_reset(cs35l56_t *driver);

/**
 * Finish booting the CS35L56
 *
 * While cs35l56_write_block loads the actual FW/COEFF data into HALO RAM, cs35l56_boot will finish
 * the boot process by:
 * - loading the fw_img_info_t fw_info member of the driver handle
 * - Performing any post-boot configuration writes
 * - Loading Calibration data (if valid)
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] fw_info          Pointer to FW information and FW Control Symbol Table
 *
 * @return
 * - CS35L56_STATUS_FAIL if:
 *      - Any pointers are null
 *      - Control port activity fails
 *      - Required FW Control symbols are not found in the symbol table
 * - CS35L56_STATUS_OK          otherwise
 *
 */
uint32_t cs35l56_boot(cs35l56_t *driver, fw_img_info_t *fw_info);

/**
 * Change the power state
 *
 * Based on the current driver state, this call will change the driver state and call the
 * appropriate power up/down function.  This can result in the part exiting/entering any of the
 * following power states:  Power Up, Standby, Hibernate, Wake.
 *
 * @see CS35L56_POWER_
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] power_state      New power state
 *
 * @warning CS35L56_POWER_DOWN should only be used when exiting BHM mode or switching between
 * firmware or coefficient files.  For low power mode while running firmware,
 * CS35L56_POWER_HIBERNATE should be used.
 *
 * @return
 * - CS35L56_STATUS_FAIL        if requested power_state is invalid, or if the call to change power
 * state fails
 * - CS35L56_STATUS_OK          otherwise
 *
 */
uint32_t cs35l56_power(cs35l56_t *driver, uint32_t power_state);

/**
 * Sets the timeout ticks for hibernate.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] ms               Duration before hibernation when allowing hibernate
 *
 * @return
 * - CS35L56_STATUS_FAIL        if any control port activity fails
 * - CS35L56_STATUS_OK          otherwise
 */
uint32_t cs35l56_timeout_ticks_set(cs35l56_t *driver, uint32_t ms);

/**
 * Calibrate the HALO Core DSP Protection Algorithm
 *
 * This performs the calibration procedure for Jamerson Audio Control firmwares.
 * This calibration information (cs35l56_calibration_t) will be saved in the driver state
 * and applied during subsequent boots of the part.  This calibration information will be available
 * to the driver until the driver is re-initialized.
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] calib_type           The calibration type to be performed
 *
 * @return
 * - CS35L56_STATUS_FAIL        if driver in invalid state for calibration, or any control port
 * activity fails
 * - CS35L56_STATUS_OK          otherwise
 *
 * @see cs35l56_calibration_t
 *
 */
uint32_t cs35l56_calibrate(cs35l56_t *driver);

/**
 * Enable the Audio Serial Port (ASP) interface for streaming
 *
 * This enables the ASP interface in i2s mode to enable streaming of audio.
 * The configuration enables 24-bit 48kHz stereo audio that passes through the HALO DSP
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] enable               Boolean to enable or disable ASP streaming
 * @param [in] freq                 Target PLL frequency at which to enable ASP
 *
 * @return
 * - CS35L56_STATUS_FAIL        if driver in invalid state for calibration, or any control port
 * activity fails
 * - CS35L56_STATUS_OK          otherwise
 *
 */
uint32_t cs35l56_set_asp_enable(cs35l56_t *driver, bool enable, uint32_t freq);

/*
 * Reads the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Address of the register to be read
 * @param [out] val             Pointer to where the read register value will be stored
 *
 * @return
 * - CS35L56_STATUS_FAIL if:
 *      - Control port activity fails
 * - otherwise, returns CS35L56_STATUS_OK
 *
 */
uint32_t cs35l56_read_reg(cs35l56_t *driver, uint32_t addr, uint32_t *val);

/*
 * Writes the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Address of the register to be written
 * @param [in] val              Value to be written to the register
 *
 * @return
 * - CS35L56_STATUS_FAIL if:
 *      - Control port activity fails
 * - otherwise, returns CS35L56_STATUS_OK
 *
 */
uint32_t cs35l56_write_reg(cs35l56_t *driver, uint32_t addr, uint32_t val);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L56_H
