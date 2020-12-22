/**
 * @file cs35l41.h
 *
 * @brief Functions and prototypes exported by the CS35L41 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019, 2020 All Rights Reserved, http://www.cirrus.com/
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

#ifndef CS35L41_H
#define CS35L41_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "fw_img_v1.h"
#include "cs35l41_sym.h"
#include "cs35l41_spec.h"
#include "cs35l41_syscfg_regs.h"

/***********************************************************************************************************************
 * LITERALS, CONSTANTS, MACROS
 **********************************************************************************************************************/

/**
 * @defgroup CS35L41_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#define CS35L41_STATUS_OK                               (0)
#define CS35L41_STATUS_FAIL                             (1)
/** @} */

/**
 * @defgroup CS35L41_BUS_TYPE_
 * @brief Types of serial bus to control the CS35L41
 *
 * @see cs35l41_bsp_config_t member bus_type
 *
 * @{
 */
#define CS35L41_BUS_TYPE_I2C                            (0)
#define CS35L41_BUS_TYPE_SPI                            (1)
/** @} */

/**
 * @defgroup CS35L41_STATE_
 * @brief State of the driver
 *
 * @see cs35l41_t member state
 *
 * @{
 */
#define CS35L41_STATE_UNCONFIGURED                      (0)
#define CS35L41_STATE_CONFIGURED                        (1)
#define CS35L41_STATE_STANDBY                           (2)
#define CS35L41_STATE_POWER_UP                          (3)
#define CS35L41_STATE_ERROR                             (4)
#define CS35L41_STATE_DSP_POWER_UP                      (5)
#define CS35L41_STATE_DSP_STANDBY                       (6)
#define CS35L41_STATE_HIBERNATE                         (7)
/** @} */

/**
 * @defgroup CS35L41_MODE_
 * @brief Mode of the driver
 *
 * @see cs35l41_t member mode
 *
 * @{
 */
#define CS35L41_MODE_HANDLING_CONTROLS                  (0)
#define CS35L41_MODE_HANDLING_EVENTS                    (1)
/** @} */

#define CS35L41_POLL_OTP_BOOT_DONE_MS                   (10)    ///< Delay in ms between polling OTP_BOOT_DONE
#define CS35L41_POLL_OTP_BOOT_DONE_MAX                  (10)    ///< Maximum number of times to poll OTP_BOOT_DONE
#define CS35L41_OTP_SIZE_WORDS                          (32)    ///< Total size of CS35L41 OTP in 32-bit words

#define CS35L41_CP_BULK_READ_LENGTH_BYTES               (CS35L41_OTP_SIZE_WORDS * 4)
///< Maximum size of Control Port Bulk Read
#define CS35L41_CP_REG_READ_LENGTH_BYTES                (4) ///< Length of Control Port Read of registers

/**
 * Length of Control Port Read buffer
 *
 * @attention The BSP is required to allocate a buffer of this length before initializing the driver.
 */
#define CS35L41_CP_READ_BUFFER_LENGTH_BYTES             (CS35L41_CP_BULK_READ_LENGTH_BYTES + \
                                                         CS35L41_CP_REG_READ_LENGTH_BYTES)

/**
 * @defgroup CS35L41_CONTROL_ID_
 * @brief ID to indicate the type of Control Request
 *
 * @see cs35l41_control
 * @see cs35l41_control_request_t member id
 *
 * @{
 */
#define CS35L41_CONTROL_ID_GET_HANDLER(A)               ((A & 0xF0000000) >> 28)
#define CS35L41_CONTROL_ID_GET_CONTROL(A)               (A & 0x0FFFFFFF)
#define CS35L41_CONTROL_ID_HANDLER_FA_GET               (0)
#define CS35L41_CONTROL_ID_HANDLER_FA_SET               (1)
#define CS35L41_CONTROL_ID_HANDLER_DSP_STATUS           (2)

#define CS35L41_CONTROL_ID_FA_GET_MASK                  (CS35L41_CONTROL_ID_HANDLER_FA_GET << 28)
#define CS35L41_CONTROL_ID_FA_SET_MASK                  (CS35L41_CONTROL_ID_HANDLER_FA_SET << 28)
#define CS35L41_CONTROL_ID_FA_GET(A)                    (A | CS35L41_CONTROL_ID_FA_GET_MASK)
#define CS35L41_CONTROL_ID_FA_SET(A)                    (A | CS35L41_CONTROL_ID_FA_SET_MASK)
#define CS35L41_CONTROL_ID_GET_REG                      CS35L41_CONTROL_ID_FA_GET(0)
#define CS35L41_CONTROL_ID_SET_REG                      CS35L41_CONTROL_ID_FA_SET(0)
#define CS35L41_CONTROL_ID_GET_SYM                      CS35L41_CONTROL_ID_FA_GET(1)
#define CS35L41_CONTROL_ID_SET_SYM                      CS35L41_CONTROL_ID_FA_SET(1)

#define CS35L41_CONTROL_ID_DSP_STATUS_MASK              (CS35L41_CONTROL_ID_HANDLER_DSP_STATUS << 28)
#define CS35L41_CONTROL_ID_DSP_STATUS(A)                (A | CS35L41_CONTROL_ID_DSP_STATUS_MASK)
#define CS35L41_CONTROL_ID_GET_DSP_STATUS               CS35L41_CONTROL_ID_DSP_STATUS(0)
/** @} */

/**
 * @defgroup CS35L41_POWER_
 * @brief Power states passed on to power() API argument power_state
 *
 * @see cs35l41_power
 *
 * @{
 */
#define CS35L41_POWER_UP                                (0)
#define CS35L41_POWER_DOWN                              (1)
#define CS35L41_POWER_HIBERNATE                         (2)
#define CS35L41_POWER_WAKE                              (3)
/** @} */

/**
 * @defgroup CS35L41_EVENT_FLAG_
 * @brief Flags passed to Notification Callback to notify BSP of specific driver events
 *
 * @see cs35l41_notification_callback_t argument event_flags
 *
 * @{
 */
#define CS35L41_EVENT_FLAG_AMP_SHORT                    (0)
#define CS35L41_EVENT_FLAG_OVERTEMP                     (1)
#define CS35L41_EVENT_FLAG_BOOST_INDUCTOR_SHORT         (2)
#define CS35L41_EVENT_FLAG_BOOST_UNDERVOLTAGE           (3)
#define CS35L41_EVENT_FLAG_BOOST_OVERVOLTAGE            (4)
#define CS35L41_EVENT_FLAG_STATE_ERROR                  (5)
/** @} */

#define CS35L41_DSP_STATUS_WORDS_TOTAL                  (9)     ///< Total registers to read for Get DSP Status control

#define CS35L41_CONTROL_PORT_MAX_PAYLOAD_BYTES          (4140)  ///< Maximum bytes CS35L41 can transfer

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/**
 * @defgroup CS35L41_FLAG_MACROS
 * @brief Macros to get/set bitfield flags
 *
 * @{
 */
#define CS35L41_FLAG_TO_MASK(A)                (0x1 << A)
#define CS35L41_FLAG_VAL_TO_MASK(A, B)         (B << A)
#define CS35L41_SET_FLAG(A, B)                 (A |= CS35L41_FLAG_TO_MASK(B))
#define CS35L41_CLEAR_FLAG(A, B)               (A &= ~(CS35L41_FLAG_TO_MASK(B)))
#define CS35L41_IS_FLAG_SET(A, B)              ((A & CS35L41_FLAG_TO_MASK(B)) == CS35L41_FLAG_TO_MASK(B))
#define CS35L41_IS_FLAG_CLEAR(A, B)            ((A & CS35L41_FLAG_TO_MASK(B)) == 0)
#define CS35L41_CHANGE_FLAG(A, B, C)           (A = ((A & ~CS35L41_FLAG_TO_MASK(B)) | CS35L41_FLAG_VAL_TO_MASK(B, C)))
/** @} */

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * Entry in OTP Map of packed bitfield entries
 */
typedef struct
{
    uint32_t reg;   ///< Register address to trim
    uint8_t shift;  ///< Bitwise shift of register bitfield
    uint8_t size;   ///< Bitwise size of register bitfield
} cs35l41_otp_packed_entry_t;

/**
 * Data structure describing an OTP Map
 */
typedef struct
{
    uint32_t id;                            ///< OTP Map ID (corresponds to value read from OTPID register bitfield)
    uint32_t num_elements;                  ///< Number of entries in OTP Map
    const cs35l41_otp_packed_entry_t *map;  ///< Pointer to OTP Map
    uint32_t bit_offset;                    ///< Bitwise offset at which to start unpacking OTP
} cs35l41_otp_map_t;

/**
 * Function pointer to Notification Callback
 *
 * @details
 * This callback will be registered at driver configuration.  This callback is called whenever the driver has detected
 * a significant event has occurred, such as an over-temperature condition.
 *
 * @see cs35l41_configure
 * @see cs35l41_bsp_config_t
 *
 * @param [in] event_flags      Flags to indicate which events have occurred
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs35l41_notification_callback_t)(uint32_t event_flags, void *arg);

/**
 * Data structure to describe a Control Request
 *
 * @see cs35l41_control
 */
typedef struct
{
    uint32_t id;    ///< Control ID
    void *arg;      ///< Argument for Control Request (nature depends on type of request)
} cs35l41_control_request_t;

/**
 * Data structure to describe a field to read via the Field Access SM
 *
 * @see cs35l41_field_access
 */
typedef struct
{
    uint32_t id;        ///< Id of symbol in symbol table.  If 0, indicates fixed address
    uint32_t address;   ///< Control Port address of field to access
    uint32_t value;     ///< Value to write/value read
    uint8_t size;       ///< Bitwise size of field to access in register
    uint8_t shift;      ///< Bitwise shift of field to access in register
} cs35l41_field_accessor_t;

/**
 * State of HALO FW Calibration
 *
 * To convert from encoded impedance 'r' to Ohms, the following formula can be used:
 * - rdc_ohms = ( 'r' / 213 ) * 5.8571434021
 */
typedef struct
{
    bool is_valid;  ///< (True) Calibration state is valid
    uint32_t r;     ///< Encoded Load Impedance determined by Calibration procedure.
} cs35l41_calibration_t;

/**
 * Status of HALO FW
 *
 * List of registers can be accessed via status values, or indexed via words (when reading via Control Port).  These
 * fields are read multiple times to determine statuses such as is_hb_inc and is_temp_changed.
 *
 * @warning  The list of registers MUST correspond to the addresses in cs35l41_dsp_status_controls.
 *
 * @see cs35l41_dsp_status_controls
 */
typedef struct
{
    union
    {
        uint32_t words[CS35L41_DSP_STATUS_WORDS_TOTAL];
        struct
        {
            uint32_t halo_state;
            uint32_t halo_heartbeat;
            uint32_t cspl_state;
            uint32_t cal_set_status;
            uint32_t cal_r_selected;
            uint32_t cal_r;
            uint32_t cal_status;
            uint32_t cal_checksum;
            uint32_t cspl_temperature;
        };
    } data;                         ///< Data read from Control Port
    bool is_hb_inc;                 ///< (True) The HALO HEARTBEAT is incrementing
    bool is_calibration_applied;    ///< (True) Calibration values are applied
    bool is_temp_changed;           ///< (True) Monitored temperature is varying.
} cs35l41_dsp_status_t;

/**
 * Configuration parameters required for calls to BSP-Driver Interface
 */
typedef struct
{
    uint8_t bsp_dev_id;                                 ///< Used to ID CS35L41 in bsp_driver_if calls
    uint32_t bsp_reset_gpio_id;                         ///< Used to ID CS35L41 Reset pin in bsp_driver_if calls
    uint32_t bsp_int_gpio_id;                           ///< Used to ID CS35L41 INT pin in bsp_driver_if calls
    uint8_t bus_type;                                   ///< Control Port type - I2C or SPI
    uint8_t *cp_write_buffer;                           ///< Pointer to Control Port write byte buffer
    uint8_t *cp_read_buffer;                            ///< Pointer to Control Port read byte buffer
    cs35l41_notification_callback_t notification_cb;    ///< Notification callback registered for detected events
    void *notification_cb_arg;                          ///< Notification callback argument
} cs35l41_bsp_config_t;

/**
 * Driver configuration data structure
 *
 * @see cs35l41_configure
 */
typedef struct
{
    cs35l41_bsp_config_t bsp_config;    ///< BSP Configuration
    const syscfg_reg_t *syscfg_regs;    ///< Pointer to array of configuration register/value pairs
    uint32_t syscfg_regs_total;         ///< Total pairs in syscfg_regs[]
    cs35l41_calibration_t cal_data;     ///< Calibration data from previous calibration sequence
} cs35l41_config_t;

/**
 * Driver state data structure
 *
 * This is the type used for the handle to the driver for all driver public API calls.  This structure must be
 * instantiated outside the scope of the driver source and initialized by the initialize() public API.
 */
typedef struct
{
    // Primary driver state fields
    uint32_t state;     ///< General driver state - @see CS35L41_STATE_
    uint32_t mode;      ///< General driver mode - @see CS35L41_MODE_

    // Driver configuration fields - see cs35l41_config_t
    cs35l41_config_t config;

    // Control Request handling fields
    cs35l41_control_request_t current_request;  ///< Current Control Request

    // Extra state material used by reset and boot
    uint32_t devid;                     ///< CS35L41 DEVID of current device
    uint32_t revid;                     ///< CS35L41 REVID of current device
    const cs35l41_otp_map_t *otp_map;   ///< Pointer to relevant OTP Map for the current device
    fw_img_info_t *fw_info;             ///< Current HALO FW/Coefficient boot configuration
    bool is_cal_boot;                   ///< Flag to indicate current HALO FW boot is for Calibration

    uint32_t event_flags;               ///< Flags set by Event Handler that are passed to noticiation callback
} cs35l41_t;

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
 * - CS35L41_STATUS_FAIL        if pointer to driver is NULL
 * - CS35L41_STATUS_OK          otherwise
 *
 */
uint32_t cs35l41_initialize(cs35l41_t *driver);

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
 * - CS35L41_STATUS_FAIL        if any pointers are NULL
 * - CS35L41_STATUS_OK          otherwise
 *
 */
uint32_t cs35l41_configure(cs35l41_t *driver, cs35l41_config_t *config);

/**
 * Processes driver state machines
 *
 * This implements the 'CS35L41 Process Flowchart' found in the driver Tech Note.  This includes:
 * - calling Event Handler if in HANDLING_EVENTS mode
 * - calling BSP Notification Callback to notify of any events or error conditions
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - if in UNCONFIGURED or ERROR state, returns CS35L41_STATUS_OK
 * - else if in HANDLING_CONTROLS mode, returns CS35L41_STATUS_OK
 * - otherwise, returns status Event Handler
 *
 * @warning This MUST be placed either in baremetal or RTOS task while (1)
 *
 */
uint32_t cs35l41_process(cs35l41_t *driver);

/**
 * Reset the CS35L41 and prepare for HALO FW booting
 *
 * This call performs all necessary reset of the CS35L41 from power-on-reset to prepare the part for loading HALO
 * FW.  This includes:
 * - toggling RESET line
 * - Application of proper errata configuration
 * - OTP unpacking
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - any control port activity fails
 *      - any status bit polling times out
 *      - the part is not supported
 *      - no OTP unpacking map exists for the part
 * - otherwise, returns CS35L41_STATUS_OK
 *
 */
uint32_t cs35l41_reset(cs35l41_t *driver);

/*
 * Write block of data to the CS35L41 register file
 *
 * This call is used to load the HALO FW/COEFF files to HALO RAM.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Starting address of loading destination
 * @param [in] data             Pointer to array of bytes to be written
 * @param [in] size             Size of array of bytes to be written
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Any pointers are NULL
 *      - size is not multiple of 4
 *      - Control port activity fails
 * - otherwise, returns CS35L41_STATUS_OK
 *
 */
uint32_t cs35l41_write_block(cs35l41_t *driver, uint32_t addr, uint8_t *data, uint32_t size);

/**
 * Finish booting the CS35L41
 *
 * While cs35l41_write_block loads the actual FW/COEFF data into HALO RAM, cs35l41_boot will finish the boot process
 * by:
 * - loading the fw_img_info_t fw_info member of the driver handle
 * - Performing any post-boot configuration writes
 * - Loading Calibration data (if valid)
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] fw_info          Pointer to FW information and FW Control Symbol Table
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Any pointers are null
 *      - Control port activity fails
 *      - Required FW Control symbols are not found in the symbol table
 * - CS35L41_STATUS_OK          otherwise
 *
 */
uint32_t cs35l41_boot(cs35l41_t *driver, fw_img_info_t *fw_info);

/**
 * Change the power state
 *
 * Based on the current driver state, this call will change the driver state and call the appropriate power up/down
 * function.  This can result in the part exiting/entering any of the following power states:  Power Up, Standby,
 * Hibernate.
 *
 * @see CS35L41_POWER_
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] power_state      New power state
 *
 * @return
 * - CS35L41_STATUS_FAIL        if requested power_state is invalid, or if the call to change power state fails
 * - CS35L41_STATUS_OK          otherwise
 *
 */
uint32_t cs35l41_power(cs35l41_t *driver, uint32_t power_state);

/**
 * Submit a Control Request to the driver
 *
 * Caller will initialize a cs35l41_control_request_t 'req' based on the control it wishes to access.  This request
 * will then be processed and any return values will be available via the 'req' parameter.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] req              data structure for control request \b passed by value
 *
 * @return
 * - CS35L41_STATUS_FAIL        if Control Request ID is invalid OR if processing of control fails
 * - CS35L41_STATUS_OK          otherwise
 *
 * @see cs35l41_control_request_t
 *
 */
uint32_t cs35l41_control(cs35l41_t *driver, cs35l41_control_request_t req);

/**
 * Send a set of HW configuration registers
 *
 * This can be used to write a set of configuration registers to the CS35L41.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] cfg              Pointer to the HW configuration array
 * @param [in] cfg_length       Length of the configuration array in number of entries
 *
 * @return
 * - CS35L41_STATUS_FAIL        if any control port transaction fails
 * - CS35L41_STATUS_OK          otherwise
 *
 * @see syscfg_reg_t
 *
 */
uint32_t cs35l41_send_syscfg(cs35l41_t *driver, const syscfg_reg_t *cfg, uint16_t cfg_length);

/**
 * Start the process for updating the tuning for the HALO FW
 *
 * This call will start the process to update the tuning for the HALO FW.  Most likely, this call will be followed by:
 * - an update of the tuning by processing a fw_img and writing it to the CS35L41
 * - a call to cs35l41_finish_tuning_switch
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - any control port activity fails
 *      - any status bit polling times out
 *      - any mailbox status is not correct for the command sent
 * - otherwise, returns CS35L41_STATUS_OK
 *
 */
uint32_t cs35l41_start_tuning_switch(cs35l41_t *driver);

/**
 * Finish the process for updating the tuning for the HALO FW
 *
 * This call will finish the process to update the tuning for the HALO FW.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - any control port activity fails
 *      - any status bit polling times out
 *      - any mailbox status is not correct for the command sent
 * - otherwise, returns CS35L41_STATUS_OK
 *
 */
uint32_t cs35l41_finish_tuning_switch(cs35l41_t *driver);

/**
 * Calibrate the HALO DSP Protection Algorithm
 *
 * This performs the calibration procedure required for CSPL Protection Algorithm to obtain the currently measured
 * speaker load impedance.  This calibration information (cs35l41_calibration_t) will be saved in the driver state
 * and applied during subsequent boots of the part.  This calibration information will be available to the driver
 * until the driver is re-initialized.
 *
 * @attention The Calibration sequence can only be successfully performed under the following conditions:
 * - while the driver is in POWER_UP state
 * - after HALO DSP FW and Calibration COEFF has been loaded
 * - while the ASP is clocked with valid I2S clocks
 * - while the ASP is being sourced with Silence
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] ambient_temp_deg_c   Current Ambient Temperature in degrees Celsius
 *
 * @return
 * - CS35L41_STATUS_FAIL if:
 *      - Control port activity fails
 *      - Required FW Control symbols are not found in the symbol table
 *      - Calibration process encounters an error - FW failure, invalid checksum
 * - CS35L41_STATUS_OK          otherwise
 *
 * @see cs35l41_calibration_t
 *
 */
uint32_t cs35l41_calibrate(cs35l41_t *driver, uint32_t ambient_temp_deg_c);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L41_H
