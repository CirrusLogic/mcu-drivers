/**
 * @file cs40l25.h
 *
 * @brief Functions and prototypes exported by the CS40L25 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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

#ifndef CS40L25_H
#define CS40L25_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "fw_img_v1.h"
#include "cs40l25_sym.h"
#include "cs40l25_cal_sym.h"
#include "cs40l25_spec.h"
#include "cs40l25_syscfg_regs.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L25_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#define CS40L25_STATUS_OK                               (0)
#define CS40L25_STATUS_FAIL                             (1)
/** @} */

/**
 * @defgroup CS40L25_BUS_TYPE_
 * @brief Types of serial bus to control the CS40L25
 *
 * @see cs40l25_bsp_config_t member bus_type
 *
 * @{
 */
#define CS40L25_BUS_TYPE_I2C                            (0)
#define CS40L25_BUS_TYPE_SPI                            (1)
/** @} */

/**
 * @defgroup CS40L25_STATE_
 * @brief State of the driver
 *
 * @see cs40l25_t member state
 *
 * @{
 */
#define CS40L25_STATE_UNCONFIGURED                      (0)
#define CS40L25_STATE_CONFIGURED                        (1)
#define CS40L25_STATE_STANDBY                           (2)
#define CS40L25_STATE_POWER_UP                          (3)
#define CS40L25_STATE_ERROR                             (4)
#define CS40L25_STATE_DSP_POWER_UP                      (5)
#define CS40L25_STATE_DSP_STANDBY                       (6)
#define CS40L25_STATE_HIBERNATE                         (7)
#define CS40L25_STATE_CAL_POWER_UP                      (8)
#define CS40L25_STATE_CAL_STANDBY                       (9)
 /** @} */

/**
 * @defgroup CS40L25_MODE_
 * @brief Mode of the driver
 *
 * @see cs40l25_t member mode
 *
 * @{
 */
#define CS40L25_MODE_HANDLING_CONTROLS                  (0)
#define CS40L25_MODE_HANDLING_EVENTS                    (1)
/** @} */

/**
 * Length of Control Port Read buffer
 *
 * @attention The BSP is required to allocate a buffer of at least this length before initializing the driver.
 */
#define CS40L25_CP_READ_BUFFER_LENGTH_BYTES             (4)

/**
 * @defgroup CS40L25_CONTROL_ID_
 * @brief ID to indicate the type of Control Request
 *
 * @see cs40l25_control
 * @see cs40l25_control_request_t member id
 *
 * @{
 */
#define CS40L25_CONTROL_ID_GET_HANDLER(A)               ((A & 0xF0000000) >> 28)
#define CS40L25_CONTROL_ID_GET_CONTROL(A)               (A & 0x0FFFFFFF)
#define CS40L25_CONTROL_ID_HANDLER_FA_GET               (0)
#define CS40L25_CONTROL_ID_HANDLER_FA_SET               (1)
#define CS40L25_CONTROL_ID_HANDLER_DSP_STATUS           (2)
#define CS40L25_CONTROL_ID_FA_GET_MASK                  (CS40L25_CONTROL_ID_HANDLER_FA_GET << 28)
#define CS40L25_CONTROL_ID_FA_SET_MASK                  (CS40L25_CONTROL_ID_HANDLER_FA_SET << 28)
#define CS40L25_CONTROL_ID_DSP_STATUS_MASK              (CS40L25_CONTROL_ID_HANDLER_DSP_STATUS << 28)
#define CS40L25_CONTROL_ID_FA_GET(A)                    (A | CS40L25_CONTROL_ID_FA_GET_MASK)
#define CS40L25_CONTROL_ID_FA_SET(A)                    (A | CS40L25_CONTROL_ID_FA_SET_MASK)
#define CS40L25_CONTROL_ID_DSP_STATUS(A)                (A | CS40L25_CONTROL_ID_DSP_STATUS_MASK)

#define CS40L25_CONTROL_ID_GET_REG                      CS40L25_CONTROL_ID_FA_GET(0)
#define CS40L25_CONTROL_ID_SET_REG                      CS40L25_CONTROL_ID_FA_SET(0)
#define CS40L25_CONTROL_ID_GET_SYM                      CS40L25_CONTROL_ID_FA_GET(1)
#define CS40L25_CONTROL_ID_SET_SYM                      CS40L25_CONTROL_ID_FA_SET(1)

#define CS40L25_CONTROL_ID_GET_DSP_STATUS               CS40L25_CONTROL_ID_DSP_STATUS(0)
/** @} */

/**
 * @defgroup CS40L25_POWER_
 * @brief Power states passed on to power() API argument power_state
 *
 * @see cs40l25_power
 *
 * @{
 */
#define CS40L25_POWER_UP                                (0)
#define CS40L25_POWER_DOWN                              (1) // Standby
#define CS40L25_POWER_HIBERNATE                         (2)
#define CS40L25_POWER_WAKE                              (3)
/** @} */

/**
 * @defgroup CS40L25_CALIB_
 * @brief Calibration options passed on to calibrate() API argument calib_type
 *
 * @see cs40l25_calibrate
 *
 * @{
 */
#define CS40L25_CALIB_F0                                (1 << 0)
#define CS40L25_CALIB_QEST                              (1 << 1)
#define CS40L25_CALIB_ALL                               (CS40L25_CALIB_F0|CS40L25_CALIB_QEST)
/** @} */

/**
 * @defgroup CS40L25_EVENT_FLAG_
 * @brief Flags passed to Notification Callback to notify BSP of specific driver events
 *
 * @see cs40l25_notification_callback_t argument event_flags
 *
 * @{
 */
#define CS40L25_EVENT_FLAG_DSP_ERROR                    (1 << 31)
#define CS40L25_EVENT_FLAG_AMP_SHORT                    (1 << 30)
#define CS40L25_EVENT_FLAG_OVERTEMP_ERROR               (1 << 29)
#define CS40L25_EVENT_FLAG_OVERTEMP_WARNING             (1 << 28)
#define CS40L25_EVENT_FLAG_BOOST_INDUCTOR_SHORT         (1 << 27)
#define CS40L25_EVENT_FLAG_BOOST_UNDERVOLTAGE           (1 << 26)
#define CS40L25_EVENT_FLAG_BOOST_OVERVOLTAGE            (1 << 25)
#define CS40L25_EVENT_FLAG_STATE_ERROR                  (1 << 12)
#define CS40L25_EVENT_FLAG_READY_FOR_DATA               (1 << 11)
#define CS40L25_EVENT_FLAG_CP_PLAYBACK_DONE             (0x1 << 9)
#define CS40L25_EVENT_FLAG_CP_PLAYBACK_SUSPEND          (0x2 << 9)
#define CS40L25_EVENT_FLAG_CP_PLAYBACK_RESUME           (0x3 << 9)
#define CS40L25_EVENT_FLAG_GPIO_PLAYBACK_DONE           (1 << 8)
#define CS40L25_EVENT_FLAG_GPIO_4_RELEASE               (1 << 7)
#define CS40L25_EVENT_FLAG_GPIO_4_PRESS                 (1 << 6)
#define CS40L25_EVENT_FLAG_GPIO_3_RELEASE               (1 << 5)
#define CS40L25_EVENT_FLAG_GPIO_3_PRESS                 (1 << 4)
#define CS40L25_EVENT_FLAG_GPIO_2_RELEASE               (1 << 3)
#define CS40L25_EVENT_FLAG_GPIO_2_PRESS                 (1 << 2)
#define CS40L25_EVENT_FLAG_GPIO_1_RELEASE               (1 << 1)
#define CS40L25_EVENT_FLAG_GPIO_1_PRESS                 (1 << 0)
/** @} */

#define CS40L25_CONFIG_REGISTERS_TOTAL                  (2)     ///< Total registers modified during cs40l25_boot
#define CS40L25_DSP_STATUS_WORDS_TOTAL                  (2)     ///< Total registers to read for Get DSP Status control
#define CS40L25_WSEQ_MAX_ENTRIES                        (48)    ///< Maximum registers written on wakeup from hibernate

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
 * @see cs40l25_configure
 *
 * @param [in] event_flags      Flags to indicate which events have occurred
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs40l25_notification_callback_t)(uint32_t event_flags, void *arg);

/**
 * Data structure to describe a Control Request
 *
 * @see cs40l25_control
 */
typedef struct
{
    uint32_t id;    ///< Control ID
    void *arg;      ///< Argument for Control Request (nature depends on type of request)
} cs40l25_control_request_t;

/**
 * Data structure to describe a field to read via cs40l25_field_access
 *
 * @see cs40l25_field_access
 */
typedef struct
{
    uint32_t address;   ///< Control Port address of field to access
    uint32_t id;        ///< Id of symbol in symbol table
    uint32_t value;     ///< Value to write/value read
    uint8_t size;       ///< Bitwise size of field to access in register
    uint8_t shift;      ///< Bitwise shift of field to access in register
    bool ack_ctrl;      ///< (True) Signal field is an acknowledge control
    uint32_t ack_reset; ///< The value the field should reset to on ack (only valid for ack ctrls)
} cs40l25_field_accessor_t;

/**
 * Data structure to provide access to bit-fields to enable Event Control sources
 *
 * @see cs40l25_config_registers_t
 */
typedef struct
{
    union
    {
        dsp_reg_t reg;
        struct
        {
            uint32_t gpio1                      : 1;
            uint32_t gpio2                      : 1;
            uint32_t gpio3                      : 1;
            uint32_t gpio4                      : 1;
            uint32_t playback_resume            : 1;
            uint32_t playback_end_suspend       : 1;
            uint32_t rx_ready                   : 1;
            uint32_t reserved_0                 : 16;
            uint32_t hardware                   : 1;
            uint32_t reserved_1                 : 8;
        };
    };
} cs40l25_event_control_t;

/** Data structure for entry in Wake Write Sequencer (WSEQ)
 *
 * Each entry corresponds to 16-bits of address and 32-bits of data.  Only 16-bits of address is needed due to the Wake
 * handling in HALO Core DSP firmware only needing to restore hardware addresses up to 0xFFFF.
 *
 * The shuffling of members is to facilitate when writing values to HALO Core packed 24-bit memory.
 *
 * @see cs40l25_wseq_table_update
 */
typedef struct
{
    union
    {
        uint32_t words[2];
        struct
        {
            uint32_t reserved_0 : 8;
            uint32_t address_ms : 8;
            uint32_t address_ls : 8;
            uint32_t val_3      : 8;
            uint32_t reserved_1 : 8;
            uint32_t val_2      : 8;
            uint32_t val_1      : 8;
            uint32_t val_0      : 8;

        };
    };
    uint8_t changed : 1;
} cs40l25_wseq_entry_t;

/**
 * State of HALO FW Calibration
 *
 */
typedef struct
{
    bool is_valid_f0;   ///< (True) Calibration state is valid
    uint32_t f0;        ///< Encoded resonant frequency (f0) determined by Calibration procedure.
    uint32_t redc;      ///< Encoded DC resistance (ReDC) determined by Calibration procedure.
    uint32_t backemf;   ///< Encoded Back EMF determined by Calibration procedure.
    bool is_valid_qest; ///< (True) Calibration state is valid
    uint32_t qest;      ///< Encoded estimated Q value (Q Est) determined by Calibration procedure.
} cs40l25_calibration_t;

/**
 * Status of HALO FW
 *
 * List of registers can be accessed via status values, or indexed via words (when reading via Control Port).  These
 * fields are read multiple times to determine statuses such as is_hb_inc and is_temp_changed.
 *
 * @warning  The list of registers MUST correspond to the addresses in cs40l25_dsp_status_addresses.
 *
 * @see cs40l25_dsp_status_addresses
 */
typedef struct
{
    union
    {
        uint32_t words[CS40L25_DSP_STATUS_WORDS_TOTAL];
        struct
        {
            uint32_t halo_state;        ///< Most recent HALO STATE
            uint32_t halo_heartbeat;    ///< Most recent HALO HEARTBEAT value
        };
    } data;                         ///< Data read from Control Port
    bool is_hb_inc;                 ///< (True) The HALO HEARTBEAT is incrementing
} cs40l25_dsp_status_t;

/**
 * Data structure for HALO Core DSP Firmware Revision
 *
 */
typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t patch                      : 8;
            uint32_t minor                      : 8;
            uint32_t major                      : 8;
            uint32_t reserved                   : 8;
        };
    };
} cs40l25_fw_revision_t;

/**
 * Configuration parameters required for calls to BSP-Driver Interface
 */
typedef struct
{
    uint8_t bsp_dev_id;                                 ///< Used to ID CS40L25 in bsp_driver_if calls
    uint32_t bsp_reset_gpio_id;                         ///< Used to ID CS40L25 Reset pin in bsp_driver_if calls
    uint32_t bsp_int_gpio_id;                           ///< Used to ID CS40L25 INT pin in bsp_driver_if calls
    uint8_t bus_type;                                   ///< Control Port type - I2C or SPI
    uint8_t *cp_write_buffer;                           ///< Pointer to Control Port write byte buffer
    uint8_t *cp_read_buffer;                            ///< Pointer to Control Port read byte buffer
    cs40l25_notification_callback_t notification_cb;    ///< Notification callback registered for detected events
    void *notification_cb_arg;                          ///< Notification callback argument
} cs40l25_bsp_config_t;

/**
 * Driver configuration data structure
 *
 * @see cs40l25_configure
 */
typedef struct
{
    cs40l25_bsp_config_t bsp_config;                ///< BSP Configuration
    const syscfg_reg_t *syscfg_regs;                ///< Pointer to system configuration table
    uint32_t syscfg_regs_total;                     ///< Total entries in system configuration table
    cs40l25_calibration_t cal_data;                 ///< Calibration data from previous calibration sequence
    cs40l25_event_control_t event_control;          ///< Event Control configuration
} cs40l25_config_t;

/**
 * Configuration of HALO FW GPIO Triggering controls
 *
 * @see cs40l25_haptic_config_t
 */
typedef struct
{
    bool enable;                    ///< Enable for this specific GPIO
    uint32_t button_press_index;    ///< Index in wavetable of wave to play upon button press
    uint32_t button_release_index;  ///< Index in wavetable of wave to play upon button release
} cs40l25_gpio_trigger_config_t;

/**
 * Configuration of HALO FW Haptic controls
 *
 * @see cs40l25_update_haptic_config
 */
typedef struct
{
    uint32_t cp_gain_control;                               ///< Gain for Control Port triggered effects
    bool gpio_enable;                                       ///< Global enable for triggering via GPIO
    uint32_t gpio_gain_control;                             ///< Gain for GPIO triggered effects
    cs40l25_gpio_trigger_config_t gpio_trigger_config[4];   ///< Triggering configuration for GPIO1-GPIO4
} cs40l25_haptic_config_t;

/**
 * Driver state data structure
 *
 * This is the type used for the handle to the driver for all driver public API calls.  This structure must be
 * instantiated outside the scope of the driver source and initialized by the cs40l25_initialize public API.
 */
typedef struct
{
    uint32_t state;                             ///< General driver state - @see CS40L25_STATE_
    uint32_t mode;                              ///< General driver mode - @see CS40L25_MODE_
    cs40l25_control_request_t current_request;  ///< Current Control Request
    uint32_t devid;                             ///< CS40L25 DEVID of current device
    uint32_t revid;                             ///< CS40L25 REVID of current device
    /*
     * List of register address/value pairs to write on wake up from hibernate
     */
    cs40l25_wseq_entry_t wseq_table[CS40L25_WSEQ_MAX_ENTRIES];
    uint8_t wseq_num_entries;                   ///< Number of entries currently in wseq_table
    bool wseq_initialized;                      ///< Flag indicating if the wseq_table has been initialized
    cs40l25_config_t config;                    ///< Driver configuration fields - see cs40l25_config_t
    fw_img_v1_info_t *fw_info;                  ///< Current HALO FW/Coefficient boot configuration
    uint32_t event_flags;                       ///< Most recent event_flags reported to BSP Notification callback
} cs40l25_t;

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
 * - CS40L25_STATUS_FAIL        if pointer to driver is NULL
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_initialize(cs40l25_t *driver);

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
 * - CS40L25_STATUS_FAIL        if any pointers are NULL
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_configure(cs40l25_t *driver, cs40l25_config_t *config);

/**
 * Processes driver events and notifications
 *
 * This implements Event Handling and BSP Notification
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - if in UNCONFIGURED or ERROR state, returns CS40L25_STATUS_OK
 * - else if in HANDLING_CONTROLS mode, returns CS40L25_STATUS_OK
 * - otherwise, returns status Event Handler
 *
 * @warning This MUST be placed either in baremetal or RTOS task while (1)
 *
 */
uint32_t cs40l25_process(cs40l25_t *driver);

/**
 * Submit a Control Request to the driver
 *
 * Caller will initialize a cs40l25_control_request_t 'req' based on the control it wishes to access.  This request
 * will then be processed and any return values will be available via the 'req' parameter.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] req              data structure for control request \b passed by value
 *
 * @return
 * - CS40L25_STATUS_FAIL        if Control Request ID is invalid OR if processing of control fails
 * - CS40L25_STATUS_OK          otherwise
 *
 * @see cs40l25_control_request_t
 *
 */
uint32_t cs40l25_control(cs40l25_t *driver, cs40l25_control_request_t req);

/**
 * Reset the CS40L25
 *
 * This call performs all necessary reset of the CS40L25 from power-on-reset to being able to process haptics in
 * Basic Haptics Mode (BHM).
 * - toggling RESET line
 * - verifying entry to BHM is successful
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS40L25_STATUS_FAIL if:
 *      - any control port activity fails
 *      - any status bit polling times out
 *      - the part is not supported
 * - otherwise, returns CS40L25_STATUS_OK
 *
 */
uint32_t cs40l25_reset(cs40l25_t *driver);

/*
 * Write block of data to the CS40L25 register file
 *
 * This call is used to load the HALO FW/COEFF files to HALO RAM.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Starting address of loading destination
 * @param [in] data             Pointer to array of bytes to be written
 * @param [in] size             Size of array of bytes to be written
 *
 * @return
 * - CS40L25_STATUS_FAIL if:
 *      - Any pointers are NULL
 *      - size is not multiple of 4
 *      - Control port activity fails
 * - otherwise, returns CS40L25_STATUS_OK
 *
 */
uint32_t cs40l25_write_block(cs40l25_t *driver, uint32_t addr, uint8_t *data, uint32_t size);

/**
 * Finish booting the CS40L25
 *
 * While cs35l41_write_block loads the actual FW/COEFF data into HALO RAM, cs35l41_boot will finish the boot process
 * by:
 * - loading the fw_img_v1_info_t fw_info member of the driver handle
 * - Performing any post-boot configuration writes
 * - Loading Calibration data (if valid)
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] fw_info          Pointer to FW information and FW Control Symbol Table
 *
 * @return
 * - CS40L25_STATUS_FAIL if:
 *      - Any pointers are null
 *      - Control port activity fails
 *      - Required FW Control symbols are not found in the symbol table
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_boot(cs40l25_t *driver, fw_img_v1_info_t *fw_info);

/**
 * Change the power state
 *
 * Based on the current driver state, this call will change the driver state and call the appropriate power up/down
 * function.  This can result in the part exiting/entering any of the following power states:  Power Up, Standby,
 * Hibernate, Wake.
 *
 * @see CS40L25_POWER_
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] power_state      New power state
 *
 * @return
 * - CS40L25_STATUS_FAIL        if requested power_state is invalid, or if the call to change power state fails
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_power(cs40l25_t *driver, uint32_t power_state);

/**
 * Calibrate the HALO Core DSP Protection Algorithm
 *
 * This performs the calibration procedure for Prince Haptic Control firmwares.
 * This calibration information (cs40l25_calibration_t) will be saved in the driver state
 * and applied during subsequent boots of the part.  This calibration information will be available to the driver
 * until the driver is re-initialized.
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] calib_type           The calibration type to be performed
 *
 * @return
 * - CS40L25_STATUS_FAIL        if submission of Control Request failed
 * - CS40L25_STATUS_OK          otherwise
 *
 * @see cs40l25_calibration_t
 *
 */
uint32_t cs40l25_calibrate(cs40l25_t *driver, uint32_t calib_type);

/**
 * Start I2S Streaming Mode
 *
 * Performas all register/memory field address updates required to put the HALO Core DSP
 * in to I2S Streaming Mode
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
 * - CS40L25_STATUS_OK          otherwise
 *
 * @see cs40l25_dsp_status_t
 */
uint32_t cs40l25_start_i2s(cs40l25_t *driver);

/**
 * Stop I2S Streaming Mode
 *
 * Performas all register/memory field address updates required to pull the HALO Core DSP
 * out of I2S Streaming Mode
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
 * - CS40L25_STATUS_OK          otherwise
 *
 * @see cs40l25_dsp_status_t
 */
uint32_t cs40l25_stop_i2s(cs40l25_t *driver);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L25_H
