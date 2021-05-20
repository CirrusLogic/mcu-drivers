/**
 * @file cs40l30.h
 *
 * @brief Functions and prototypes exported by the CS40L30 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2021 All Rights Reserved, http://www.cirrus.com/
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

#ifndef CS40L30_H
#define CS40L30_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "fw_img.h"
#include "cs40l30_sym.h"
#include "cs40l30_cal_sym.h"
#include "cs40l30_spec.h"
#include "sdk_version.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L30_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#define CS40L30_STATUS_OK                               (0)
#define CS40L30_STATUS_FAIL                             (1)
/** @} */

/**
 * @defgroup CS40L30_BUS_TYPE_
 * @brief Types of serial bus to control the CS40L30
 *
 * @see cs40l30_config_t member bus_type
 *
 * @{
 */
#define CS40L30_BUS_TYPE_I2C                            (0)
#define CS40L30_BUS_TYPE_SPI                            (1)
/** @} */

/**
 * @defgroup CS40L30_STATE_
 * @brief State of the driver
 *
 * @see cs40l30_t member state
 *
 * @{
 */
#define CS40L30_STATE_UNCONFIGURED                      (0)
#define CS40L30_STATE_CONFIGURED                        (1)
#define CS40L30_STATE_STANDBY                           (2)
#define CS40L30_STATE_POWER_UP                          (3)
#define CS40L30_STATE_ERROR                             (4)
#define CS40L30_STATE_DSP_POWER_UP                      (5)
#define CS40L30_STATE_HIBERNATE                         (6)
#define CS40L30_STATE_CAL_POWER_UP                      (7)
#define CS40L30_STATE_CAL_STANDBY                       (8)
 /** @} */

/**
 * @defgroup CS40L30_MODE_
 * @brief Mode of the driver
 *
 * @see cs40l30_t member mode
 *
 * @{
 */
#define CS40L30_MODE_HANDLING_CONTROLS                  (0)
#define CS40L30_MODE_HANDLING_EVENTS                    (1)
/** @} */

/**
 * @defgroup CS40L30_POWER_
 * @brief Power states passed on to power() API argument power_state
 *
 * @see cs40l30_functions_t member power
 *
 * @{
 */
#define CS40L30_POWER_UP                                (0)
#define CS40L30_POWER_DOWN                              (1) // Standby
#define CS40L30_POWER_PREVENT_HIBERNATE                 (2)
#define CS40L30_POWER_ALLOW_HIBERNATE                   (3)
/** @} */

/**
 * @defgroup CS40L30_CALIB_
 * @brief Calibration options passed on to calibrate() API argument calib_type
 *
 * @see cs40l30_functions_t member calibrate
 *
 * @{
 */
#define CS40L30_CALIB_F0                                (1 << 0)
#define CS40L30_CALIB_QEST                              (1 << 1)
#define CS40L30_CALIB_ALL                               (CS40L30_CALIB_F0|CS40L30_CALIB_QEST)
/** @} */

#define CS40L30_PM_TIMEOUT_COUNT                        (20)
#define CS40L30_PM_TIMEOUT_WAIT                         (1)

#define CS40L30_ACK_CTRL_TIMEOUT_COUNT                  (30)
#define CS40L30_ACK_CTRL_TIMEOUT_WAIT                   (5)

#define CS40L30_FWID_CAL                                (0x1700D5)
/**
 * @defgroup CS40L30_MBOX_
 * @brief DSP Mailbox commands
 *
 * @{
 */
#define CS40L30_MBOX_TYPE_HAPTIC                        (1)
#define CS40L30_MBOX_TYPE_POWER                         (2)
#define CS40L30_MBOX_TYPE_HAPTIC_CTRL                   (5)
#define CS40L30_MBOX_TYPE_MASK(A)                       (A << 24)

#define CS40L30_HAPTIC_TRIGGER(A)                       (A | CS40L30_MBOX_TYPE_MASK(CS40L30_MBOX_TYPE_HAPTIC))
#define CS40L30_POWER_MGMT(A)                           (A | CS40L30_MBOX_TYPE_MASK(CS40L30_MBOX_TYPE_POWER))
#define CS40L30_HAPTIC_TRIGGER_CTRL(A)                  (A | CS40L30_MBOX_TYPE_MASK(CS40L30_MBOX_TYPE_HAPTIC_CTRL))

#define CS40L30_MBOX_HAPTIC_TRIGGER_RAM_WAVEFORM(A)     CS40L30_HAPTIC_TRIGGER(A)
#define CS40L30_MBOX_HAPTIC_TRIGGER_ROM_MASK            (1 << 23)
#define CS40L30_MBOX_HAPTIC_TRIGGER_ROM_BANK_0(A)       (CS40L30_HAPTIC_TRIGGER((A - 0x1) | CS40L30_MBOX_HAPTIC_TRIGGER_ROM_MASK))
#define CS40L30_MBOX_HAPTIC_TRIGGER_ROM_BANK_1(A)       (CS40L30_HAPTIC_TRIGGER((A + 0xA) | CS40L30_MBOX_HAPTIC_TRIGGER_ROM_MASK))
#define CS40L30_MBOX_HAPTIC_TRIGGER_ROM_BANK_2(A)       (CS40L30_HAPTIC_TRIGGER((A + 0x15) | CS40L30_MBOX_HAPTIC_TRIGGER_ROM_MASK))
#define CS40L30_MBOX_HAPTIC_TRIGGER_ROM_BANK_3(A)       (CS40L30_HAPTIC_TRIGGER((A + 0x20) | CS40L30_MBOX_HAPTIC_TRIGGER_ROM_MASK))
#define CS40L30_MBOX_HAPTIC_TRIGGER_OTP_MASK            (1 << 7) | (1 << 23)
#define CS40L30_MBOX_HAPTIC_TRIGGER_OTP_BUZZ(A)         (CS40L30_HAPTIC_TRIGGER((A - 1) | CS40L30_MBOX_HAPTIC_TRIGGER_OTP_MASK))


#define CS40L30_MBOX_POWER_MGMT_PREVENT_HIBERNATE       CS40L30_POWER_MGMT(3)
#define CS40L30_MBOX_POWER_MGMT_ALLOW_HIBERNATE         CS40L30_POWER_MGMT(4)
#define CS40L30_MBOX_POWER_MGMT_SHUTDOWN                CS40L30_POWER_MGMT(5)
#define CS40L30_MBOX_POWER_MGMT_BOOT_TO_RAM             CS40L30_POWER_MGMT(6)

#define CS40L30_MBOX_HAPTIC_TRIGGER_CTRL_STOP_PLAYBACK  CS40L30_HAPTIC_TRIGGER_CTRL(0)

/** @} */
#define CS40L30_WSEQ_MAX_ENTRIES                        (48) ///< Maximum number of registers written on wakeup from hibernate

#define CS40L30_INT9_BTN_BITS           (0xF)
#define CS40L30_INT9_VIRT_BTN_SHIFT     5

#define CS40L30_MAX_VIRT_BTNS           4
#define CS40L30_VIRT_PRESS_MASK         (1 << 0)
#define CS40L30_VIRT_RELEASE_MASK       (1 << 1)
#define CS40L30_VIRT_GPI_MASK           ((1 << 31) | (1 << 22))
#define CS40L30_INT10_VIRT_BTN_MASK     ((1 << 15) | (1 << 4))
#define CS40L30_INT10_VIRT_GPI_MASK     ((1 << 3) | (1 << 0))

#define CS40L30_BUZZ_FREQ_MIN           100
#define CS40L30_BUZZ_DURATION_MAX       4000

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
 * @see cs40l30_functions_t member configure
 *
 * @param [in] event_flags      Flags to indicate which events have occurred
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs40l30_notification_callback_t)(uint32_t event_flags, void *arg);

/**
 * Entries used to write address value pairs to POWERONSEQUENCE.
 *
 * Write sequencer reads address/value pairs from POWERONSEQUENCE in following format:
 *
 *     byte_3 |         byte_2         |          byte_1       |      byte_0
 *     unused | address_ms [bits 8-15] | address_ls [bits 0-7] | val_3 [bits 24-31]
 *
 *     byte_3 |       byte_2       |        byte_1      |      byte_0
 *     unused | val_2 [bits 16-23] | val_1 [bits 8-15]  | val_0 [bits 0-7]
 */
typedef union
{
    uint8_t words[8];
    struct
    {
        uint8_t reserved_0 : 8;
        uint8_t address_ms : 8;
        uint8_t address_ls : 8;
        uint8_t val_3      : 8;
        uint8_t reserved_1 : 8;
        uint8_t val_2      : 8;
        uint8_t val_1      : 8;
        uint8_t val_0      : 8;
    };
} cs40l30_wseq_entry_t;

/**
 * HALO FW Revision
 *
 * FW Revision is denoted 'major'.'minor'.'patch'
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
} cs40l30_fw_revision_t;

/**
 * State of HALO FW Calibration
 *
 * Currently just a stub
 *
 */
typedef struct
{
    uint32_t stub;
} cs40l30_calibration_t;

/**
 * Fsense Button enumeration
 *
 */
typedef enum
{
    CS40L30_NO_BTN = 0,
    CS40L30_VIRT_BTN1 = 1,
    CS40L30_VIRT_BTN2 = 2,
    CS40L30_VIRT_BTN3 = 3,
    CS40L30_VIRT_BTN4 = 4,
} cs40l30_fsense_btn_t;

/**
 * Fsense Button event types
 *
 */
typedef enum
{
    CS40L30_NO_EVENT = 0,
    CS40L30_RELEASE = 1,
    CS40L30_PRESS = 2,
} cs40l30_fsense_ev_t;

/**
 * Fsense Button descriptor
 *
 */
typedef struct
{
    uint32_t code;
    uint32_t type;
    cs40l30_fsense_btn_t btn_id;
    cs40l30_fsense_ev_t ev_id;
} cs40l30_fsense_input_desc_t;

/**
 * HALO FW Event Notifier bitfields
 *
 */
typedef union
{
    uint32_t words[2];
    struct
    {
        uint32_t virtual_button_1_press         : 1;
        uint32_t virtual_button_1_release       : 1;
        uint32_t virtual_button_1_tap           : 1;
        uint32_t virtual_button_1_doubletap     : 1;
        uint32_t virtual_button_1_reserved      : 1;
        uint32_t virtual_button_2_press         : 1;
        uint32_t virtual_button_2_release       : 1;
        uint32_t virtual_button_2_tap           : 1;
        uint32_t virtual_button_2_doubletap     : 1;
        uint32_t virtual_button_2_reserved      : 1;
        uint32_t virtual_button_3_press         : 1;
        uint32_t virtual_button_3_release       : 1;
        uint32_t virtual_button_3_tap           : 1;
        uint32_t virtual_button_3_doubletap     : 1;
        uint32_t virtual_button_3_reserved      : 1;
        uint32_t virtual_button_4_press         : 1;
        uint32_t virtual_button_4_release       : 1;
        uint32_t virtual_button_4_tap           : 1;
        uint32_t virtual_button_4_doubletap     : 1;
        uint32_t virtual_button_4_reserved      : 1;
        uint32_t lra_start                      : 1;
        uint32_t lra_end                        : 1;
        uint32_t gpi_2_rise                     : 1;
        uint32_t gpi_2_fall                     : 1;
        uint32_t gpi_3_rise                     : 1;
        uint32_t gpi_3_fall                     : 1;
        uint32_t gpi_4_rise                     : 1;
        uint32_t gpi_4_fall                     : 1;
        uint32_t gpi_5_rise                     : 1;
        uint32_t gpi_5_fall                     : 1;
        uint32_t gpi_6_rise                     : 1;
        uint32_t gpi_6_fall                     : 1;
        uint32_t gpi_7_rise                     : 1;
        uint32_t gpi_7_fall                     : 1;
        uint32_t gpi_11_rise                    : 1;
        uint32_t gpi_11_fall                    : 1;
        uint32_t virtual_button_1_push          : 1;
        uint32_t virtual_button_1_double_push   : 1;
        uint32_t virtual_button_1_long_push     : 1;
        uint32_t virtual_button_2_push          : 1;
        uint32_t virtual_button_2_double_push   : 1;
        uint32_t virtual_button_2_long_push     : 1;
        uint32_t virtual_button_3_push          : 1;
        uint32_t virtual_button_3_double_push   : 1;
        uint32_t virtual_button_3_long_push     : 1;
        uint32_t virtual_button_4_push          : 1;
        uint32_t virtual_button_4_double_push   : 1;
        uint32_t virtual_button_4_long_push     : 1;
        uint32_t svc_error                      : 1;
        uint32_t reserved                       : 15;
    };
} cs40l30_dsp_event_notifier_t;

/**
 * Configuration parameters required for calls to BSP-Driver Interface
 */
typedef struct
{
    uint8_t bsp_dev_id;                                 ///< Used to ID CS40L30 in bsp_driver_if calls
    uint32_t bsp_reset_gpio_id;                         ///< Used to ID CS40L30 Reset pin in bsp_driver_if calls
    uint32_t bsp_int_gpio_id;                           ///< Used to ID CS40L30 INT pin in bsp_driver_if calls
    uint8_t bus_type;                                   ///< Control Port type - I2C or SPI
    cs40l30_notification_callback_t notification_cb;    ///< Notification callback registered for detected events
    void *notification_cb_arg;                          ///< Notification callback argument

    cs40l30_fsense_input_desc_t *fsense_desc;
    uint8_t fsense_input_count;
} cs40l30_bsp_config_t;

/**
 * Driver configuration data structure
 *
 * @see cs40l30_configure
 */
typedef struct
{
    cs40l30_bsp_config_t bsp_config;
    const uint32_t *syscfg_regs;
    uint32_t syscfg_regs_total;
} cs40l30_config_t;

/**
 * Driver Event Handler flags
 *
 */
typedef union
{
    uint32_t words[1];
    struct
    {
        uint32_t virtual_button_1   : 4;
        uint32_t virtual_button_2   : 4;
        uint32_t virtual_button_3   : 4;
        uint32_t virtual_button_4   : 4;
        uint32_t lra_start          : 1;
        uint32_t lra_end            : 1;
        uint32_t boost_overvoltage  : 1;
        uint32_t boost_undervoltage : 1;
        uint32_t boost_short        : 1;
        uint32_t boost_peak_current : 1;
        uint32_t amp_short          : 1;
        uint32_t overtemp           : 1;
        uint32_t brownout           : 1;

        uint32_t reserved           : 7;

        uint32_t driver_state_error : 1;
    };
} cs40l30_event_flags_t;

/**
 * Driver state data structure
 *
 * This is the type used for the handle to the driver for all driver public API calls.  This structure must be
 * instantiated outside the scope of the driver source and initialized by the cs40l30_initialize public API.
 */
typedef struct
{
    uint32_t state;
    uint32_t mode;
    cs40l30_config_t config;
#ifndef CONFIG_NO_SHADOW_OTP
    bool need_shadow_otp;
#endif

    cs40l30_wseq_entry_t wseq_table[CS40L30_WSEQ_MAX_ENTRIES];
    uint32_t wseq_num_entries;

    uint32_t devid;
    uint32_t revid;

    fw_img_info_t *fw_info;

    cs40l30_event_flags_t event_flags;
} cs40l30_t;

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
 * - CS40L30_STATUS_FAIL        if pointer to driver is NULL
 * - CS40L30_STATUS_OK          otherwise
 *
 */
uint32_t cs40l30_initialize(cs40l30_t *driver);

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
 * - CS40L30_STATUS_FAIL        if any pointers are NULL
 * - CS40L30_STATUS_OK          otherwise
 *
 */
uint32_t cs40l30_configure(cs40l30_t *driver, cs40l30_config_t *config);

/**
 * Processes driver events and notifications
 *
 * This implements Event Handling and BSP Notification
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - if in UNCONFIGURED or ERROR state, returns CS40L30_STATUS_OK
 * - else if in HANDLING_CONTROLS mode, returns CS40L30_STATUS_OK
 * - otherwise, returns status Event Handler
 *
 * @warning This MUST be placed either in baremetal or RTOS task while (1)
 *
 */
uint32_t cs40l30_process(cs40l30_t *driver);

/**
 * Reset the CS40L30
 *
 * This call performs all necessary reset of the CS40L30 from power-on-reset to being able to process haptics
 * and button presses in ROM mode.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return
 * - CS40L30_STATUS_FAIL if:
 *      - any control port activity fails
 *      - any status bit polling times out
 *      - the part is not supported
 * - CS40L30_STATUS_OK          otherwise
 *
 */
uint32_t cs40l30_reset(cs40l30_t *driver);

/*
 * Write block of data to the CS40L30 register file
 *
 * This call is used to load the HALO FW/COEFF files to HALO RAM.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             Starting address of loading destination
 * @param [in] data             Pointer to array of bytes to be written
 * @param [in] size             Size of array of bytes to be written
 *
 * @return
 * - CS40L30_STATUS_FAIL if:
 *      - Any pointers are NULL
 *      - size is not multiple of 4
 *      - Control port activity fails
 * - otherwise, returns CS40L30_STATUS_OK
 *
 */
uint32_t cs40l30_write_block(cs40l30_t *driver, uint32_t addr, uint8_t *data, uint32_t size);

/**
 * Finish booting the CS40L30
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
 * - CS40L30_STATUS_FAIL if:
 *      - Any pointers are null
 *      - Control port activity fails
 *      - Required FW Control symbols are not found in the symbol table
 * - CS40L30_STATUS_OK          otherwise
 *
 */
uint32_t cs40l30_boot(cs40l30_t *driver, fw_img_info_t *fw_info);

/**
 * Change the power state
 *
 * Based on the current driver state, this call will change the driver state and call the appropriate power up/down
 * function.  This can result in the part exiting/entering any of the following power states:  Power Up, Standby,
 * Hibernate, Wake.
 *
 * @see CS40L30_POWER_
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] power_state      New power state
 *
 * @return
 * - CS40L30_STATUS_FAIL        if requested power_state is invalid, or if the call to change power state fails
 * - CS40L30_STATUS_OK          otherwise
 *
 */
uint32_t cs40l30_power(cs40l30_t *driver, uint32_t power_state);

/**
 * Calibrate the HALO FW
 *
 * This performs the calibration procedure for Prince Haptic Control firmwares.
 * This calibration information (cs40l30_calibration_t) will be saved in the driver state
 * and applied during subsequent boots of the part.  This calibration information will be available to the driver
 * until the driver is re-initialized.
 *
 * @param [in] driver               Pointer to the driver state
 * @param [in] calib_type           The calibration type to be performed
 *
 * @return
 * - CS40L30_STATUS_FAIL        if submission of Control Request failed
 * - CS40L30_STATUS_OK          otherwise
 *
 * @see cs40l30_calibration_t
 *
 */
uint32_t cs40l30_calibrate(cs40l30_t *driver, uint32_t calib_type);

/**
 * Reads the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             32-bit address to be read
 * @param [out] val             Pointer to register value read
 *
 * @return
 * - CS40L30_STATUS_FAIL        if the call to BSP failed
 * - CS40L30_STATUS_OK          otherwise
 *
 * @warning Contains platform-dependent code.
 *
 */
uint32_t cs40l30_read_reg(cs40l30_t *driver, uint32_t addr, uint32_t *val);

/**
 * Writes the contents of a single register/memory address
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             32-bit address to be written
 * @param [in] val              32-bit value to be written
 *
 * @return
 * - CS40L30_STATUS_FAIL        if the call to BSP failed
 * - CS40L30_STATUS_OK          otherwise
 *
 * @warning Contains platform-dependent code.
 *
 */
uint32_t cs40l30_write_reg(cs40l30_t *driver, uint32_t addr, uint32_t val);

/**
 * Find if a symbol is in the symbol table and return its address if it is.
 *
 * This will search through the symbol table pointed to in the 'fw_info' member of the driver state and return
 * the control port register address to use for access.  The 'symbol_id' parameter must be from the group CS40L30_SYM_.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] symbol_id        id of symbol to search for
 *
 * @return
 * - non-0 - symbol register address
 * - 0 - symbol not found.
 *
 */
uint32_t cs40l30_find_symbol(cs40l30_t *driver, uint32_t symbol_id);

/**
 * Writes the contents of a single register/memory address that ACK's with a default value
 *
 * This performs the same function as cs40l30_write_reg, with the addition of, after writing the value to the address
 * specified, will periodically read back the register and verify that a default value is restored, the 'acked_val',
 * indicating the write succeeded.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             32-bit address to be written
 * @param [in] val              32-bit value to be written
 * @param [in] acked_val        32-bit value the address should be restored to
 *
 * @return
 * - CS40L30_STATUS_FAIL        if the call to BSP failed or if register is never restored to acked_val
 * - CS40L30_STATUS_OK          otherwise
 *
 * @warning Contains platform-dependent code.
 *
 */
uint32_t cs40l30_write_acked_reg(cs40l30_t *driver, uint32_t addr, uint32_t val, uint32_t acked_val);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L30_H
