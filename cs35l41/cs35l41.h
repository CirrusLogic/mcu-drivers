/**
 * @file cs35l41.h
 *
 * @brief Functions and prototypes exported by the CS35L41 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2019 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
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
#include "cs35l41_spec.h"
#ifdef INCLUDE_FW
#include "cs35l41_firmware.h"
#endif // INCLUDE_FW
#include "f_queue.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup CS35L41_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#define CS35L41_STATUS_OK                               (0)
#define CS35L41_STATUS_FAIL                             (1)
#define CS35L41_STATUS_BOOT_REQUEST                     (2)
#define CS35L41_STATUS_INVALID                          (3)
/** @} */

/**
 * @defgroup CS35L41_BUS_TYPE_
 * @brief Types of serial bus to control the CS35L41
 *
 * @see cs35l41_config_t member bus_type
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
#ifdef INCLUDE_FW
#define CS35L41_STATE_DSP_POWER_UP                      (5)
#define CS35L41_STATE_DSP_STANDBY                       (6)
#endif // INCLUDE_FW
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

/**
 * @defgroup CS35L41_SM_STATE_
 * @brief Generic states used for all child state machines
 *
 * @see cs35l41_sm_t member state
 *
 * @{
 */
#define CS35L41_SM_STATE_INIT                           (0)
#define CS35L41_SM_STATE_DONE                           (0xFF)
#define CS35L41_SM_STATE_ERROR                          (0xFE)


/**
 * @defgroup CS35L41_EVENT_SM_STATE_
 * @brief States for the Event Handler state machine
 *
 * @see cs35l41_event_sm
 *
 * @{
 */
#define CS35L41_EVENT_SM_STATE_INIT                     (CS35L41_SM_STATE_INIT)
#define CS35L41_EVENT_SM_STATE_READ_IRQ_STATUS          (1)
#define CS35L41_EVENT_SM_STATE_READ_IRQ_MASK            (2)
#define CS35L41_EVENT_SM_STATE_CLEAR_IRQ_FLAGS          (3)
#define CS35L41_EVENT_SM_STATE_DISABLE_BOOST            (4)
#define CS35L41_EVENT_SM_STATE_TOGGLE_ERR_RLS           (5)
#define CS35L41_EVENT_SM_STATE_ENABLE_BOOST             (6)
#define CS35L41_EVENT_SM_STATE_DONE                     (CS35L41_SM_STATE_DONE)
#define CS35L41_EVENT_SM_STATE_ERROR                    (CS35L41_SM_STATE_ERROR)
/** @} */

/**
 * @defgroup CS35L41_RESET_SM_STATE_
 * @brief States for the Reset state machine
 *
 * @see cs35l41_reset_sm
 *
 * @{
 */
#define CS35L41_RESET_SM_STATE_INIT                     (CS35L41_SM_STATE_INIT)
#define CS35L41_RESET_SM_STATE_WAIT_T_RLPW              (1)
#define CS35L41_RESET_SM_STATE_WAIT_T_IRS               (2)
#define CS35L41_RESET_SM_STATE_WAIT_OTP_BOOT_DONE       (3)
#define CS35L41_RESET_SM_STATE_READ_ID                  (4)
#define CS35L41_RESET_SM_STATE_WRITE_IRQ_ERRATA         (5)
#define CS35L41_RESET_SM_STATE_READ_OTPID               (6)
#define CS35L41_RESET_SM_STATE_READ_OTP                 (7)
#define CS35L41_RESET_SM_STATE_WRITE_OTP_UNLOCK         (8)
#define CS35L41_RESET_SM_STATE_READ_TRIM_WORD           (9)
#define CS35L41_RESET_SM_STATE_WRITE_TRIM_WORD          (10)
#define CS35L41_RESET_SM_STATE_WRITE_TRIM_LOCK          (11)
#define CS35L41_RESET_SM_STATE_WRITE_CCM_CORE_CTRL      (12)
#define CS35L41_RESET_SM_STATE_DONE                     (CS35L41_SM_STATE_DONE)
#define CS35L41_RESET_SM_STATE_ERROR                    (CS35L41_SM_STATE_ERROR)
/** @} */

#ifdef INCLUDE_FW
/**
 * @defgroup CS35L41_BOOT_SM_STATE_
 * @brief States for the Boot state machine
 *
 * @see cs35l41_boot_sm
 *
 * @{
 */
#define CS35L41_BOOT_SM_STATE_INIT                      (CS35L41_SM_STATE_INIT)
#define CS35L41_BOOT_SM_STATE_LOAD_FW                   (1)
#define CS35L41_BOOT_SM_STATE_LOAD_COEFF                (2)
#define CS35L41_BOOT_SM_STATE_POST_BOOT_CONFIG          (3)
#define CS35L41_BOOT_SM_STATE_APPLY_CAL_DATA            (4)
#define CS35L41_BOOT_SM_STATE_ERROR                     (CS35L41_SM_STATE_ERROR)
#define CS35L41_BOOT_SM_STATE_DONE                      (CS35L41_SM_STATE_DONE)
/** @} */
#endif // INCLUDE_FW

/**
 * @defgroup CS35L41_POWER_UP_SM_STATE_
 * @brief States for the Power Up state machine
 *
 * @see cs35l41_power_up_sm
 *
 * @{
 */
#define CS35L41_POWER_UP_SM_STATE_INIT                  (CS35L41_SM_STATE_INIT)
#ifdef INCLUDE_FW
#define CS35L41_POWER_UP_SM_STATE_LOCK_MEM              (1)
#define CS35L41_POWER_UP_SM_STATE_SET_FRAME_SYNC        (2)
#define CS35L41_POWER_UP_SM_STATE_CLOCKS_TO_DSP         (3)
#endif // INCLUDE_FW
#define CS35L41_POWER_UP_SM_STATE_PUP_PATCH             (4)
#define CS35L41_POWER_UP_SM_STATE_SET_GLOBAL_EN         (5)
#define CS35L41_POWER_UP_SM_STATE_WAIT_T_AMP_PUP        (6)
#ifdef INCLUDE_FW
#define CS35L41_POWER_UP_SM_STATE_MBOX_CLR_UNMASK_IRQ   (7)
#define CS35L41_POWER_UP_SM_STATE_MBOX_READ_STATUS_1    (8)
#define CS35L41_POWER_UP_SM_STATE_MBOX_WRITE_CMD        (9)
#define CS35L41_POWER_UP_SM_STATE_MBOX_WAIT_1MS         (10)
#define CS35L41_POWER_UP_SM_STATE_MBOX_READ_IRQ         (11)
#define CS35L41_POWER_UP_SM_STATE_MBOX_MASK_CLR_IRQ     (12)
#define CS35L41_POWER_UP_SM_STATE_MBOX_READ_STATUS_2    (13)
#endif // INCLUDE_FW
#define CS35L41_POWER_UP_SM_STATE_ERROR                 (CS35L41_SM_STATE_ERROR)
#define CS35L41_POWER_UP_SM_STATE_DONE                  (CS35L41_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS35L41_POWER_DOWN_SM_STATE_
 * @brief States for the Power Down state machine
 *
 * @see cs35l41_power_down_sm
 *
 * @{
 */
#define CS35L41_POWER_DOWN_SM_STATE_INIT                (CS35L41_SM_STATE_INIT)
#ifdef INCLUDE_FW
#define CS35L41_POWER_DOWN_SM_STATE_STOP_WDT            (1)
#define CS35L41_POWER_DOWN_SM_STATE_STOP_DSP            (2)
#define CS35L41_POWER_DOWN_SM_STATE_MBOX_CLR_UNMASK_IRQ (3)
#define CS35L41_POWER_DOWN_SM_STATE_MBOX_WRITE_CMD      (4)
#define CS35L41_POWER_DOWN_SM_STATE_MBOX_WAIT_1MS       (5)
#define CS35L41_POWER_DOWN_SM_STATE_MBOX_READ_IRQ       (6)
#define CS35L41_POWER_DOWN_SM_STATE_MBOX_MASK_CLR_IRQ   (7)
#define CS35L41_POWER_DOWN_SM_STATE_MBOX_READ_STATUS    (8)
#endif // INCLUDE_FW
#define CS35L41_POWER_DOWN_SM_STATE_CLEAR_GLOBAL_EN     (9)
#define CS35L41_POWER_DOWN_SM_STATE_READ_PDN_IRQ        (10)
#define CS35L41_POWER_DOWN_SM_STATE_READ_PDN_IRQ_WAIT   (11)
#define CS35L41_POWER_DOWN_SM_STATE_CLEAR_PDN_IRQ       (12)
#define CS35L41_POWER_DOWN_SM_STATE_WRITE_PDN_PATCH     (13)
#define CS35L41_POWER_DOWN_SM_STATE_ERROR               (CS35L41_SM_STATE_ERROR)
#define CS35L41_POWER_DOWN_SM_STATE_DONE                (CS35L41_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS35L41_CONFIGURE_SM_STATE_
 * @brief States for the Configure state machine
 *
 * @see cs35l41_configure_sm
 *
 * @{
 */
#define CS35L41_CONFIGURE_SM_STATE_INIT                 (CS35L41_SM_STATE_INIT)
#define CS35L41_CONFIGURE_SM_STATE_UNLOCK_REGS          (1)
#define CS35L41_CONFIGURE_SM_STATE_READ_REGS            (2)
#define CS35L41_CONFIGURE_SM_STATE_WRITE_REGS           (3)
#define CS35L41_CONFIGURE_SM_STATE_LOCK_REGS            (4)
#define CS35L41_CONFIGURE_SM_STATE_ERROR                (CS35L41_SM_STATE_ERROR)
#define CS35L41_CONFIGURE_SM_STATE_DONE                 (CS35L41_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS35L41_FIELD_ACCESS_SM_STATE_
 * @brief States for the Field Access state machine
 *
 * @see cs35l41_field_access_sm
 *
 * @{
 */
#define CS35L41_FIELD_ACCESS_SM_STATE_INIT              (CS35L41_SM_STATE_INIT)
#define CS35L41_FIELD_ACCESS_SM_STATE_READ_MEM          (1)
#define CS35L41_FIELD_ACCESS_SM_STATE_WRITE_MEM         (2)
#define CS35L41_FIELD_ACCESS_SM_STATE_ERROR             (CS35L41_SM_STATE_ERROR)
#define CS35L41_FIELD_ACCESS_SM_STATE_DONE              (CS35L41_SM_STATE_DONE)
/** @} */

#ifdef INCLUDE_FW
/**
 * @defgroup CS35L41_CALIBRATION_SM_STATE_
 * @brief States for the Calibration state machine
 *
 * @see cs35l41_calibration_sm
 *
 * @{
 */
#define CS35L41_CALIBRATION_SM_STATE_INIT               (CS35L41_SM_STATE_INIT)
#define CS35L41_CALIBRATION_SM_STATE_SET_TEMP           (1)
#define CS35L41_CALIBRATION_SM_STATE_WAIT_2S            (2)
#define CS35L41_CALIBRATION_SM_STATE_READ_DATA          (3)
#define CS35L41_CALIBRATION_SM_STATE_ERROR              (CS35L41_SM_STATE_ERROR)
#define CS35L41_CALIBRATION_SM_STATE_DONE               (CS35L41_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS35L41_GET_DSP_STATUS_SM_STATE_
 * @brief States for the Get DSP Status state machine
 *
 * @see cs35l41_get_dsp_status_sm
 *
 * @{
 */
#define CS35L41_GET_DSP_STATUS_SM_STATE_INIT            (CS35L41_SM_STATE_INIT)
#define CS35L41_GET_DSP_STATUS_SM_STATE_READ_STATUSES_1 (1)
#define CS35L41_GET_DSP_STATUS_SM_STATE_WAIT            (2)
#define CS35L41_GET_DSP_STATUS_SM_STATE_READ_STATUSES_2 (3)
#define CS35L41_GET_DSP_STATUS_SM_STATE_ERROR           (CS35L41_SM_STATE_ERROR)
#define CS35L41_GET_DSP_STATUS_SM_STATE_DONE            (CS35L41_SM_STATE_DONE)
/** @} */
#endif // INCLUDE_FW
/** @} */

/**
 * @defgroup CS35L41_FLAGS_
 * @brief Flags set by ISRs used to trigger transitions in state machines
 *
 * @see cs35l41_sm_t member flags
 *
 * @{
 */
#define CS35L41_FLAGS_TIMEOUT                           (0) ///< Flag for BSP timer timeout
#define CS35L41_FLAGS_CP_RW_DONE                        (1) ///< Flag for BSP Control Port Read/Write done
#define CS35L41_FLAGS_CP_RW_ERROR                       (2) ///< Flag for BSP Control Port Read/Write error
#ifdef INCLUDE_FW
#define CS35L41_FLAGS_REQUEST_FW_BOOT                   (3) ///< Flag to indicate a request to boot firmware
#define CS35L41_FLAGS_REQUEST_COEFF_BOOT                (4) ///< Flag to indicate a request to boot coeff
#endif // INCLUDE_FW
#define CS35L41_FLAGS_IS_GET_REQUEST                    (5) ///< Flag to indicate Field Access GET request
#define CS35L41_FLAGS_REQUEST_RESTART                   (6) ///< Flag to indicate a need to restart the state machine
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
 * Size of Control Request Queue
 *
 * @attention The system designer should size this queue to handle the maximum number of Control Requests the system
 * could possibly send to the driver before ability to process.
 */
#define CS35L41_CONTROL_REQUESTS_SIZE                   (10)

/**
 * @defgroup CS35L41_CONTROL_ID_
 * @brief ID to indicate the type of Control Request
 *
 * @see cs35l41_functions_t member control
 * @see cs35l41_control_request_t member id
 *
 * @{
 */
#define CS35L41_CONTROL_ID_NONE                         (0)
#define CS35L41_CONTROL_ID_RESET                        (1)
#ifdef INCLUDE_FW
#define CS35L41_CONTROL_ID_BOOT                         (2)
#endif // INCLUDE_FW
#define CS35L41_CONTROL_ID_POWER_UP                     (3)
#define CS35L41_CONTROL_ID_POWER_DOWN                   (4)
#define CS35L41_CONTROL_ID_CONFIGURE                    (5)
#define CS35L41_CONTROL_ID_GET_VOLUME                   (6)
#define CS35L41_CONTROL_ID_SET_VOLUME                   (7)
#ifdef INCLUDE_FW
#define CS35L41_CONTROL_ID_GET_HALO_HEARTBEAT           (8)
#define CS35L41_CONTROL_ID_CALIBRATION                  (9)
#define CS35L41_CONTROL_ID_GET_DSP_STATUS               (10)
#endif // INCLUDE_FW
#ifdef INCLUDE_FW
#define CS35L41_CONTROL_ID_MAX                          (CS35L41_CONTROL_ID_GET_DSP_STATUS)
#else
#define CS35L41_CONTROL_ID_MAX                          (CS35L41_CONTROL_ID_SET_VOLUME)
#endif // INCLUDE_FW
/** @} */

#ifdef INCLUDE_FW
/**
 * @defgroup CS35L41_DSP_MBOX_STATUS_
 * @brief Statuses of the HALO DSP Mailbox
 *
 * @{
 */
#define CS35L41_DSP_MBOX_STATUS_RUNNING                 (0)
#define CS35L41_DSP_MBOX_STATUS_PAUSED                  (1)
#define CS35L41_DSP_MBOX_STATUS_RDY_FOR_REINIT          (2)
/** @} */

/**
 * @defgroup CS35L41_DSP_MBOX_CMD_
 * @brief HALO DSP Mailbox commands
 *
 * @see cs35l41_t member mbox_cmd
 *
 * @{
 */
#define CS35L41_DSP_MBOX_CMD_NONE                       (0)
#define CS35L41_DSP_MBOX_CMD_PAUSE                      (1)
#define CS35L41_DSP_MBOX_CMD_RESUME                     (2)
#define CS35L41_DSP_MBOX_CMD_REINIT                     (3)
#define CS35L41_DSP_MBOX_CMD_STOP_PRE_REINIT            (4)
#define CS35L41_DSP_MBOX_CMD_UNKNOWN                    (-1)
/** @} */
#endif // INCLUDE_FW

/**
 * @defgroup CS35L41_POWER_
 * @brief Power states passed on to power() API argument power_state
 *
 * @see cs35l41_functions_t member power
 *
 * @{
 */
#define CS35L41_POWER_UP                                (0)
#define CS35L41_POWER_DOWN                              (1)
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
#define CS35L41_EVENT_FLAG_SM_ERROR                     (5)
/** @} */

#define CS35L41_CONFIG_REGISTERS_TOTAL                  (32)    ///< Total registers modified during Configure SM

#define CS35L41_INPUT_SRC_DISABLE                       (0x00)  ///< Data Routing value to indicate 'disabled'

#define CS35L41_WKFET_AMP_THLD_DISABLED                 (0x0)   ///< Weak-FET amp drive threshold to indicate disabled

#ifdef INCLUDE_FW
#define CS35L41_DSP_STATUS_WORDS_TOTAL                  (9)     ///< Total registers to read for Get DSP Status control
#endif // INCLUDE_FW

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

#ifdef INCLUDE_FW
/**
 * Data structure to describe HALO firmware and coefficient download.
 */
typedef struct
{
    uint32_t total_fw_blocks;               ///< Total blocks of firmware payload
    halo_boot_block_t *fw_blocks;       ///< Pointer to list of firmware boot blocks
    uint32_t total_coeff_blocks;            ///< Total blocks of coefficient payload
    halo_boot_block_t *coeff_blocks;    ///< Pointer to list of coefficient boot blocks
} cs35l41_boot_config_t;
#endif // INCLUDE_FW

/**
 * Function pointer to driver control state machine implementation.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return General driver API call status
 */
typedef uint32_t (*cs35l41_sm_fp_t)(void *driver);

/**
 * Data structure describing driver control state machine implementation
 */
typedef struct
{
    uint8_t state;      ///< Current state of state machine
    uint32_t flags;     ///< Current event/notification flags the state machine should process
    uint8_t count;      ///< Counter allocated to simplify number of states
    cs35l41_sm_fp_t fp; ///< Pointer to state machine implementation
} cs35l41_sm_t;

/**
 * Function pointer to Notification Callback
 *
 * @details
 * This callback will be registered at driver configuration.  This callback is called whenever the driver has detected
 * a significant event has occurred, such as an over-temperature condition.
 *
 * @see cs35l41_functions_t member configure
 *
 * @param [in] event_flags      Flags to indicate which events have occurred
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs35l41_notification_callback_t)(uint32_t event_flags, void *arg);

/**
 * Function pointer to Control Request Callback
 *
 * @details
 * This callback will be registered when the Control Request is requested, i.e. via control(), boot() API.
 *
 * @see cs35l41_functions_t member control
 *
 * @param [in] id               Control ID just completed
 * @param [in] status           Status of Control Request
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs35l41_control_callback_t)(uint8_t id, uint32_t status, void *arg);

/**
 * Data structure to describe a Control Request
 *
 * @see cs35l41_functions_t member control
 */
typedef struct
{
    uint8_t id;                     ///< Control ID
    void *arg;                      ///< Argument for Control Request (nature depends on type of request)
    cs35l41_control_callback_t cb;  ///< Control Request complete callback
    void *cb_arg;                   ///< Argument for Control Request complete callback
} cs35l41_control_request_t;

/**
 * Data structure to describe a field to read via the Field Access SM
 *
 * @see cs35l41_load_control
 */
typedef struct
{
    uint32_t address;   ///< Control Port address of field to access
    uint32_t value;     ///< Value to write/value read
    uint8_t size;       ///< Bitwise size of field to access in register
    uint8_t shift;      ///< Bitwise shift of field to access in register
} cs35l41_field_accessor_t;

/**
 * Configuration of amplifier audio hardware
 */
typedef struct
{
    uint8_t dout_hiz_ctrl;  ///< ASP TX data pin Hi-Z control.  See datasheet Section 7.15.4
    bool is_master_mode;    ///< (True) Set ASP in Master Mode
    bool fsync_inv;         ///< (True) Invert polarity of FSYNC
    bool bclk_inv;          ///< (True) Invert polarity of BCLK
    bool amp_dre_en;        ///< (True) Enable Amplifier DRE
    bool ng_enable;         ///< (True) Enable Noise Gate
    uint8_t ng_thld;        ///< Noise Gate threshold.  See datasheet Section 7.19.3
    uint8_t ng_delay;       ///< Noise Gate delay.    See datasheet Section 7.19.3
    uint8_t amp_gain_pcm;   ///< Amplifier analog gain for PCM input path.  See datasheet Section 7.20.1
    uint8_t amp_ramp_pcm;   ///< Amplifier PCM audio digital soft-ramp rate.  See datasheet Section 7.17.1
} cs35l41_audio_hw_config_t;

/**
 * Configuration of amplifier Audio Serial Port (ASP)
 */
typedef struct
{
    bool is_i2s;        ///< (True) Port is in I2S mode; (False) Port is in DSPA mode
    uint8_t rx1_slot;   ///< Slot position for RX Channel 1
    uint8_t rx2_slot;   ///< Slot position for RX Channel 2
    uint8_t tx1_slot;   ///< Slot position for TX Channel 1
    uint8_t tx2_slot;   ///< Slot position for TX Channel 2
    uint8_t tx3_slot;   ///< Slot position for TX Channel 3
    uint8_t tx4_slot;   ///< Slot position for TX Channel 4
    uint8_t tx_wl;      ///< TX active data width (in number of BCLK cycles)
    uint8_t tx_width;   ///< TX slot width (in number of BCLK cycles)
    uint8_t rx_wl;      ///< RX active data width (in number of BCLK cycles)
    uint8_t rx_width;   ///< RX slot width (in number of BCLK cycles)
} cs35l41_asp_config_t;

/**
 * Routing of audio data to Amplifier DAC, DSP, and ASP TX channels
 *
 * @see CS35L41_INPUT_SRC_
 */
typedef struct
{
    uint8_t dac_src;        ///< Amplifier DAC audio mixer source
    uint8_t dsp_rx1_src;    ///< DSP RX Channel 1 audio mixer source
    uint8_t dsp_rx2_src;    ///< DSP RX Channel 2 audio mixer source
    uint8_t asp_tx1_src;    ///< ASP TX Channel 1 audio mixer source
    uint8_t asp_tx2_src;    ///< ASP TX Channel 2 audio mixer source
    uint8_t asp_tx3_src;    ///< ASP TX Channel 3 audio mixer source
    uint8_t asp_tx4_src;    ///< ASP TX Channel 4 audio mixer source
} cs35l41_routing_config_t;

/**
 * Configuration of internal clocking.
 */
typedef struct
{
    uint8_t refclk_sel;     ///< Clock source for REFCLK @see CS35L41_PLL_REFLCLK_SEL_
    uint32_t sclk;          ///< BCLK (or SCLK) frequency in Hz
    uint32_t refclk_freq;   ///< REFCLK frequency in Hz
    uint32_t global_fs;     ///< FSYNC frequency in Hz
} cs35l41_clock_config_t;

/**
 * Collection of audio-related configurations
 */
typedef struct
{
    cs35l41_audio_hw_config_t hw;
    cs35l41_asp_config_t asp;
    cs35l41_routing_config_t routing;
    cs35l41_clock_config_t clock;
    uint16_t volume;    ///< Volume to be applied at reset
} cs35l41_audio_config_t;

/**
 * Amplifier-related configurations
 */
typedef struct
{
    uint16_t boost_inductor_value_nh;   ///< Boost inductor value in nH
    uint16_t boost_capacitor_value_uf;  ///< Boost capacitor value in uF
    uint16_t boost_ipeak_ma;            ///< Boost peak current in mA
    uint8_t bst_ctl;                    ///< Boost converter target voltage.  See datasheet Section 7.11.1
    uint8_t temp_warn_thld;             ///< Amplifier overtemperature warning threshold.  See datasheet Section 7.13.1
    bool classh_enable;                 ///< (True) Enable Class H functionality
    uint8_t bst_ctl_sel;                ///< Boost converter control source selection.  See datasheet Section 7.11.2
    bool bst_ctl_lim_en;                ///< Class H boost control max limit.  See datasheet Section 7.11.2
    uint8_t ch_mem_depth;               ///< Class H algorithm memory depth.  See datasheet Section 7.19.1
    uint8_t ch_hd_rm;                   ///< Class H algorithm headroom.  See datasheet Section 7.19.1
    uint8_t ch_rel_rate;                ///< Class H release rate.  See datasheet Section 7.19.1
    uint8_t wkfet_amp_delay;            ///< Weak-FET entry delay.  See datasheet Section 7.19.2
    uint8_t wkfet_amp_thld;             ///< Weak-FET amplifier drive threshold.  See datasheet Section 7.19.2
} cs35l41_amp_config_t;

/**
 * Registers modified for amplifier configuration.
 *
 * List of registers can be accessed via bitfields (when mapping from driver config/state), or indexed via words
 * (when reading/writing via Control Port).
 *
 * All register types are defined according to the datasheet and specified in cs35l41_spec.h.
 *
 * @warning  The list of registers MUST correspond to the addresses in cs35l41_config_register_addresses.
 *
 * @see cs35l41_config_register_addresses
 */
typedef union
{
    uint32_t words[CS35L41_CONFIG_REGISTERS_TOTAL];

    struct
    {
        cs35l41_intp_amp_ctrl_t intp_amp_ctrl;
        cs35l41_dre_amp_gain_t dre_amp_gain;
        cs35l41_mixer_t asptx1_input;
        cs35l41_mixer_t asptx2_input;
        cs35l41_mixer_t asptx3_input;
        cs35l41_mixer_t asptx4_input;
        cs35l41_mixer_t dsp1rx1_input;
        cs35l41_mixer_t dsp1rx2_input;
        cs35l41_mixer_t dacpcm1_input;
        cs35l41_ccm_global_sample_rate_t ccm_global_sample_rate;
        cs35l41_noise_gate_mixer_ngate_ch1_cfg_t noise_gate_mixer_ngate_ch1_cfg;
        cs35l41_noise_gate_mixer_ngate_ch2_cfg_t noise_gate_mixer_ngate_ch2_cfg;
        cs35l41_ccm_refclk_input_t ccm_refclk_input;
        cs35l41_msm_block_enables_t msm_block_enables;
        cs35l41_msm_block_enables2_t msm_block_enables2;
        cs35l41_dataif_asp_enables1_t dataif_asp_enables1;
        cs35l41_dataif_asp_control2_t dataif_asp_control2;
        cs35l41_dataif_asp_frame_control5_t dataif_asp_frame_control5;
        cs35l41_dataif_asp_frame_control1_t dataif_asp_frame_control1;
        cs35l41_dataif_asp_control3_t dataif_asp_control3;
        cs35l41_dataif_asp_data_control5_t dataif_asp_data_control5;
        cs35l41_dataif_asp_data_control1_t dataif_asp_data_control1;
        uint32_t ccm_fs_mon0;
        cs35l41_dataif_asp_control1_t dataif_asp_control1;
        cs35l41_boost_lbst_slope_t boost_lbst_slope;
        cs35l41_boost_bst_loop_coeff_t boost_bst_loop_coeff;
        cs35l41_boost_bst_ipk_ctl_t boost_bst_ipk_ctl;
        cs35l41_boost_vbst_ctl_1_t boost_vbst_ctl_1;
        cs35l41_boost_vbst_ctl_2_t boost_vbst_ctl_2;
        cs35l41_tempmon_warn_limit_threshold_t tempmon_warn_limit_threshold;
        cs35l41_pwrmgmt_classh_config_t pwrmgmt_classh_config;
        cs35l41_pwrmgmt_wkfet_amp_config_t pwrmgmt_wkfet_amp_config;
    };
} cs35l41_config_registers_t;

#ifdef INCLUDE_FW
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
 * @warning  The list of registers MUST correspond to the addresses in cs35l41_dsp_status_addresses.
 *
 * @see cs35l41_dsp_status_addresses
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
#endif // INCLUDE_FW

/**
 * Driver state data structure
 *
 * This is the type used for the handle to the driver for all driver public API calls.  This structure must be
 * instantiated outside the scope of the driver source and initialized by the initialize() public API.
 */
typedef struct
{
    // Primary driver state fields
    uint32_t state;             ///< General driver state - @see CS35L41_STATE_
    uint32_t mode;              ///< General driver mode - @see CS35L41_MODE_
    cs35l41_sm_t control_sm;    ///< Control State Machine for current Control Request
    cs35l41_sm_t event_sm;      ///< Event Handler State Machine

    // Control Request handling fields
    f_queue_t control_q;                                                        ///< Queue of Control Requests
    cs35l41_control_request_t control_requests[CS35L41_CONTROL_REQUESTS_SIZE];  ///< Buffered Control Requests
    cs35l41_control_request_t current_request;                                  ///< Current Control Request

    // Extra state material used by Control and Event Handler SMs
    uint32_t register_buffer;                   ///< buffer for reading Control Port registers
    uint32_t devid;                             ///< CS35L41 DEVID of current device
    uint32_t revid;                             ///< CS35L41 REVID of current device
    uint16_t otp_bit_count;                     ///< Current bit position when processing OTP
    const uint32_t *errata;                     ///< Pointer to relevant errata for the current device
    const cs35l41_otp_map_t *otp_map;           ///< Pointer to relevant OTP Map for the current device
#ifdef INCLUDE_FW
    uint32_t mbox_cmd;                          ///< HALO FW Mailbox command to be sent
#endif // INCLUDE_FW
    cs35l41_config_registers_t config_regs;     ///< Contents of Control Port registers to configure
#ifdef INCLUDE_FW
    uint32_t ambient_temp_deg_c;                ///< Ambient Temperature (in degrees C) to use for Calibration
#endif // INCLUDE_FW
    cs35l41_field_accessor_t field_accessor;    ///< Current Control Port field to access

    // Driver configuration fields - see cs35l41_config_t
    uint8_t bsp_dev_id;
    uint32_t bsp_reset_gpio_id;
    uint32_t bsp_int_gpio_id;
    uint8_t bus_type;
    uint8_t *cp_write_buffer;
    uint8_t *cp_read_buffer;
    cs35l41_notification_callback_t notification_cb;
    void *notification_cb_arg;
    cs35l41_audio_config_t audio_config;
    cs35l41_amp_config_t amp_config;
#ifdef INCLUDE_FW
    cs35l41_calibration_t cal_data;
#endif // INCLUDE_FW

#ifdef INCLUDE_FW
    cs35l41_boot_config_t *boot_config;     ///< Current HALO FW/Coefficient boot configuration
#endif // INCLUDE_FW
} cs35l41_t;

/**
 * Driver configuration data structure
 *
 * @see cs35l41_functions_t member configure
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
    cs35l41_audio_config_t audio_config;                ///< Amplifier audio-related configuration
    cs35l41_amp_config_t amp_config;                    ///< Amplifier amp-related configuration
#ifdef INCLUDE_FW
    cs35l41_calibration_t cal_data;                     ///< Calibration data from previous calibration sequence
#endif // INCLUDE_FW
} cs35l41_config_t;

/**
 * Driver public API
 *
 * All API calls require a handle (cs35l41_t *) to the driver state.
 * All API calls return a status @see CS35L41_STATUS_
 *
 * @attention The process() API must be called in either (1) a baremetal superloop or (2) an RTOS task loop in order to
 * execute the following APIs:
 * - control()
 * - boot()
 * - power()
 * - calibrate()
 */
typedef struct
{
    /**
     * Initialize driver state/handle
     *
     * Sets all driver state members to 0, and initializes Control Request Queue.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if call to f_queue_if_t fails
     * - CS35L41_STATUS_OK          otherwise
     *
     */
    uint32_t (*initialize)(cs35l41_t *driver);

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
    uint32_t (*configure)(cs35l41_t *driver, cs35l41_config_t *config);

    /**
     * Processes driver state machines
     *
     * This implements the 'CS35L41 Process Flowchart' found in the driver Tech Note.  This includes:
     * - calling Event Handler SM if in HANDLING_EVENTS mode
     * - calling current Control SM if in HANLDING_CONTROLS mode
     * - loading new controls and checking validity per driver state
     * - calling BSP callbacks upon completion of Control Requests
     * - handling Control Request errors
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - if in UNCONFIGURED or ERROR state, returns CS35L41_STATUS_OK
     * - else if in HANDLING_CONTROLS mode and not currently processing Control SM< returns CS35L41_STATUS_OK
     * - otherwise, returns status from any state machine executed (either Event Handler or Control SM)
     *
     * @warning This MUST be placed either in baremetal or RTOS task while (1)
     *
     */
    uint32_t (*process)(cs35l41_t *driver);

    /**
     * Submit a Control Request to the driver
     *
     * Caller will initialize a cs35l41_control_request_t 'req' based on the control it wishes to access.  This request
     * will then be added to the Control Request Queue, and the callback given in 'req' will be called once either:
     * - the Control Request has completed successfully
     * - there was an ERROR when executing the Control Request
     * - the Control Request is invalid for the current driver state
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] req              data structure for control request \b passed by value
     *
     * @return
     * - CS35L41_STATUS_FAIL        if Control Request ID is invalid OR if insertion into Control Request Queue failed
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see cs35l41_control_request_t
     *
     */
    uint32_t (*control)(cs35l41_t *driver, cs35l41_control_request_t req);

    /**
     * Boot the CS35L41
     *
     * Boot may include stepping through the following state machines:
     * - Reset SM - toggle RESET GPIO, unpack and apply OTP trims, HW configuration
     * - Boot SM - load a HALO DSP FW image and COEFF image
     * - Configure SM - apply the driver configuration to the CS35L41 register file
     *
     * This function is essentially an alias for multiple calls to cs35l41_control - it submits Control Requests for the
     * controls above that apply to the current driver configuration.
     *
     * @attention It should be noted that the callback \b cb will be called upon completion (either successful or
     * unsuccessful) of \b each Control Request when booting.  Thus the caller could get the callback called for RESET,
     * BOOT, and CONFIGURE controls.
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] cb               Callback for completion of each control stage of boot process
     * @param [in] cb_arg           Pointer to argument to use for callback
     *
     * @return
     * - CS35L41_STATUS_FAIL        if any submission of Control Request failed
     * - CS35L41_STATUS_OK          otherwise
     *
     */
    uint32_t (*boot)(cs35l41_t *driver, cs35l41_control_callback_t cb, void *cb_arg);

    /**
     * Change the power state
     *
     * This submits a Control Request to either Power Up or Power Down the CS35L41.  Although there is no checking here
     * for whether the request is invalid (i.e. Power Down when the driver state is already STANDBY), this checking will
     * be performed once the request is loaded in cs35l41_process().
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] power_state      New power state
     * @param [in] cb               Callback for completion of each control stage of boot process
     * @param [in] cb_arg           Pointer to argument to use for callback
     *
     * @return
     * - CS35L41_STATUS_FAIL        if submission of Control Request failed
     * - CS35L41_STATUS_OK          otherwise
     *
     */
    uint32_t (*power)(cs35l41_t *driver, uint32_t power_state, cs35l41_control_callback_t cb, void *cb_arg);

#ifdef INCLUDE_FW
    /**
     * Calibrate the HALO DSP Protection Algorithm
     *
     * This performs the calibration procedure required for CSPL Protection Algorithm to obtain the currently measured
     * speaker load impedance.  This calibration information (cs35l41_calibration_t) will be saved in the driver state
     * and applied during subsequent boots of the part.  This calibration information will be available to the driver
     * until the driver is re-initialized.
     *
     * @param [in] driver               Pointer to the driver state
     * @param [in] ambient_temp_deg_c   Current Ambient Temperature in degrees Celsius
     * @param [in] cb                   Callback for completion of each control stage of boot process
     * @param [in] cb_arg               Pointer to argument to use for callback
     *
     * @return
     * - CS35L41_STATUS_FAIL        if submission of Control Request failed
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see cs35l41_calibration_t
     *
     */
    uint32_t (*calibrate)(cs35l41_t *driver,
                          uint32_t ambient_temp_deg_c,
                          cs35l41_control_callback_t cb,
                          void *cb_arg);
#endif // INCLUDE_FW
} cs35l41_functions_t;

/**
 * Driver private API
 *
 */
typedef struct
{
    /**
     * Notify the driver when the BSP Timer expires.
     *
     * This callback is given as a parameter for all non-blocking calls to the set_timer() API in bsp_driver_if_t.
     * Currently, the timer is only used for state machines handling Control Requests, so the only action required
     * is to set the CS35L41_FLAGS_TIMEOUT for the state machine being currently processed.
     *
     * @param [in] status           Status of the asynchronous call to set_timer()
     * @param [in] cb_arg           A pointer to callback argument registered for set_timer().  For the driver, this arg
     *                              is used for a pointer to the driver state cs35l41_t.
     *
     * @return none
     *
     * @see bsp_driver_if_t member set_timer.
     * @see bsp_callback_t
     *
     */
    void (*timer_callback)(uint32_t status, void *cb_arg);

    /**
     * Notify the driver when the BSP Control Port (cp) read transaction completes.
     *
     * This callback is given as a parameter for all non-blocking calls to the following API in bsp_driver_if_t.
     * - i2c_read_repeated_start
     *
     * @note Currently, the BSP and driver only support I2C transactions.
     *
     * The primary task of this callback is to set flags in either the flag cache for the currently processed Control
     * Request state machine, or the Event Handler state machine.
     *
     * @param [in] status           Status of the asynchronous call to BSP Control Port read
     * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
     *                              pointer to the driver state cs35l41_t.
     *
     * @return none
     *
     * @see bsp_driver_if_t member i2c_read_repeated_start.
     * @see bsp_callback_t
     *
     * @warning Contains platform-dependent code.
     *
     */
    void (*cp_read_callback)(uint32_t status, void *cb_arg);

    /**
     * Notify the driver when the BSP Control Port (cp) write transaction completes.
     *
     * This callback is given as a parameter for all non-blocking calls to the following API in bsp_driver_if_t.
     * - i2c_write
     * - i2c_db_write
     *
     * @note Currently, the BSP and driver only support I2C transactions.
     *
     * The primary task of this callback is to set flags in either the flag cache for the currently processed Control
     * Request state machine, or the Event Handler state machine.
     *
     * @param [in] status           Status of the asynchronous call to BSP Control Port write
     * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
     *                              pointer to the driver state cs35l41_t.
     *
     * @return none
     *
     * @see bsp_driver_if_t member i2c_write, i2c_db_write.
     * @see bsp_callback_t
     *
     */
    void (*cp_write_callback)(uint32_t status, void *cb_arg);

    /**
     * Notify the driver when the CS35L41 INTb GPIO drops low.
     *
     * This callback is registered with the BSP in the register_gpio_cb() API call.
     *
     * The primary task of this callback is to transition the driver mode from CS35L41_MODE_HANDLING_CONTROLS to
     * CS35L41_MODE_HANDLING_EVENTS, in order to start processing the Event Handler state machine.  It also resets the
     * Event Handler state machine, if it was not currently being processed.
     *
     * @param [in] status           Status of the asynchronous call to BSP Control Port write
     * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
     *                              pointer to the driver state cs35l41_t.
     *
     * @return none
     *
     * @see bsp_driver_if_t member register_gpio_cb.
     * @see bsp_callback_t
     *
     */
    void (*irq_callback)(uint32_t status, void *cb_arg);

    /**
     * Reads the contents of a single register/memory address
     *
     * The main purpose is to handle buffering and BSP calls required for reading a single memory address.
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] addr             32-bit address to be read
     * @param [out] val             Pointer to register value read (only used for non-blocking calls)
     * @param [in] is_blocking      Indicates whether request is for blocking (true) or non-blocking (false) call
     *
     * @return
     * - CS35L41_STATUS_FAIL        if the call to BSP failed
     * - CS35L41_STATUS_OK          otherwise
     *
     * @warning Contains platform-dependent code.
     *
     */
    uint32_t (*read_reg)(cs35l41_t *driver, uint32_t addr, uint32_t *val, bool is_blocking);

    /**
     * Writes the contents of a single register/memory address
     *
     * The main purpose is to handle buffering and BSP calls required for writing a single memory address.
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] addr             32-bit address to be written
     * @param [in] val              32-bit value to be written
     * @param [in] is_blocking      Indicates whether request is for blocking (true) or non-blocking (false) call
     *
     * @return
     * - CS35L41_STATUS_FAIL        if the call to BSP failed
     * - CS35L41_STATUS_OK          otherwise
     *
     * @warning Contains platform-dependent code.
     *
     */
    uint32_t (*write_reg)(cs35l41_t *driver, uint32_t addr, uint32_t val, bool is_blocking);

    /**
     * Reset State Machine
     *
     * This state machine performs all necessary steps to reset the CS35L41 and put it into STANDBY state.  The state
     * machine design is documented in the Reset State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Reset State Diagram
     *
     */
    uint32_t (*reset_sm)(cs35l41_t *driver);

#ifdef INCLUDE_FW
    /**
     * Boot State Machine
     *
     * This state machine loads FW and COEFF images into the CS35L41 HALO DSP memory regions.  Completing the
     * state machine results in the driver transition to DSP_STANDBY state.  The state machine design is documented in
     * the Boot State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Boot State Diagram
     *
     */
    uint32_t (*boot_sm)(cs35l41_t *driver);
#endif // INCLUDE_FW

    /**
     * Power Up State Machine
     *
     * This state machine performs all necessary steps to transition the CS35L41 to be ready to pass audio through the
     * amplifier DAC.  Completing the state machine results in the driver transition to POWER_UP state.  The state
     * machine design is documented in the Power Up State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Power Up State Diagram
     *
     */
    uint32_t (*power_up_sm)(cs35l41_t *driver);

    /**
     * Power Down State Machine
     *
     * This state machine performs all necessary steps to transition the CS35L41 to be in Standby power mode. Completing
     * the state machine results in the driver transition to STANDBY or DSP_STANDBY state.  The state machine
     * design is documented in the Power Down State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Power Down State Diagram
     *
     */
    uint32_t (*power_down_sm)(cs35l41_t *driver);

    /**
     * Configure State Machine
     *
     * This state machine performs all CS35L41 configuration required after Reset (and DSP Boot, if required).
     * Completing the state machine does not result in any driver state change.  Although the configuration should
     * almost always be performed when the driver is in STANDBY or DSP_STANDBY state, it is not prohibited.  The state
     * machine design is documented in the Configure State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Configure State Diagram
     * @see cs35l41_apply_configs()
     *
     */
    uint32_t (*configure_sm)(cs35l41_t *driver);

    /**
     * Field Access State Machine
     *
     * This state machine performs actions required to do a Get/Set of a Control Port register or HALO DSP Memory
     * bit-field.  Completing the state machine does not result in any driver state change.  The state machine design
     * is documented in the Field Access State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Field Access State Diagram
     * @see cs35l41_field_accessor_t
     *
     */
    uint32_t (*field_access_sm)(cs35l41_t *driver);

#ifdef INCLUDE_FW
    /**
     * Calibration State Machine
     *
     * This state machine performs the Calibration sequence required for CSPL HALO DSP firmware Protect Algorithm.
     * Completing the state machine does not result in any driver state change.  The state machine design is documented
     * in the Calibration State Diagram found in the Driver Tech Note.
     *
     * @attention The Calibration sequence can only be successfully performed under the following conditions:
     * - while the driver is in POWER_UP state
     * - after HALO DSP FW and Calibration COEFF has been loaded
     * - while the ASP is clocked with valid I2S clocks
     * - while the ASP is being sourced with Silence
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Calibration State Diagram
     * @see cs35l41_calibration_t
     *
     */
    uint32_t (*calibration_sm)(cs35l41_t *driver);

    /**
     * Get DSP Status State Machine
     *
     * This state machine performs all register/memory field address reads to get the current HALO DSP status.  Since
     * some statuses are only determined by observing changes in values of a given field, the fields are read once,
     * then after a delay of 10 milliseconds, are read a second time to observe changes.  Completing the state machine
     * does not result in any driver state change.  The state machine design is documented in the Get DSP Status State
     * Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Get DSP Status State Diagram
     * @see cs35l41_dsp_status_t
     *
     */
    uint32_t (*get_dsp_status_sm)(cs35l41_t *driver);
#endif // INCLUDE_FW

    /**
     * Event Handler State Machine
     *
     * This state machine performs all steps to handle IRQ and other asynchronous events the driver is aware of,
     * resulting in calling of the notification callback (cs35l41_notification_callback_t).
     *
     * Beginning the state machine results in transition of driver \b mode from HANDLING_CONTROLS to HANDLING_EVENTS,
     * while completing the state machine will result in transition of driver \b mode from HANDLING_EVENTS to
     * HANDLING_CONTROLS in cs35l41_process(). The state machine design is documented in the Event Handler State Diagram
     * found in the Driver Tech Note.
     *
     * If there are any IRQ events that include Speaker-Safe Mode Errors or Boost-related events, then the procedure
     * outlined in the Datasheet Section 4.16.1.1 is implemented here.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS35L41_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Event Handler State Diagram
     * @see cs35l41_notification_callback_t
     *
     */
    uint32_t (*event_sm)(void *driver);

    /**
     * Gets pointer to correct errata based on DEVID/REVID
     *
     * @param [in] devid            DEVID read from CS35L41_SW_RESET_DEVID_REG
     * @param [in] revid            REVID read from CS35L41_SW_RESET_REVID_REG
     * @param [out] errata          Pointer to array of uint32_t implementing errata
     *
     * @return
     * - CS35L41_STATUS_FAIL        corresponding errata not found
     * - CS35L41_STATUS_OK          otherwise
     *
     */
    uint32_t (*get_errata)(uint32_t devid, uint32_t revid, const uint32_t **errata);

    /**
     * Reads contents from a consecutive number of memory addresses
     *
     * Starting at 'addr', this will read 'length' number of 32-bit values into the BSP-allocated buffer from the
     * control port.  This bulk read will place contents into the BSP buffer starting at the 4th byte address.
     * Bytes 0-3 in the buffer are reserved for non-bulk reads (i.e. calls to cs35l41_read_reg).  This control port
     * call only supports non-blocking calls.  This function also only supports I2C transactions.
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] addr             32-bit address to be read
     * @param [in] length           number of memory addresses (i.e. 32-bit words) to read
     *
     * @return
     * - CS35L41_STATUS_FAIL        if the call to BSP failed, or if 'length' exceeds the size of BSP buffer
     * - CS35L41_STATUS_OK          otherwise
     *
     * @warning Contains platform-dependent code.
     *
     */
    uint32_t (*cp_bulk_read)(cs35l41_t *driver, uint32_t addr, uint32_t length);

    /**
     * Writes from byte array to consecutive number of Control Port memory addresses
     *
     * This control port call only supports non-blocking calls.  This function also only supports I2C transactions.
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] addr             32-bit address to be read
     * @param [in] bytes            pointer to array of bytes to write via Control Port bus
     * @param [in] length           number of bytes to write
     *
     * @return
     * - CS35L41_STATUS_FAIL        if the call to BSP failed
     * - CS35L41_STATUS_OK          otherwise
     *
     * @warning Contains platform-dependent code.
     *
     */
    uint32_t (*cp_bulk_write)(cs35l41_t *driver, uint32_t addr, uint8_t *bytes, uint32_t length);

    /**
     * Applies OTP trim bit-field to current register word value.
     *
     * During the Reset SM, trim bit-fields must be tweezed from OTP and applied to corresponding Control Port register
     * contents.
     *
     * @param [in] otp_mem          pointer byte array consisting of entire contents of OTP
     * @param [in] bit_count        current bit index into otp_mem, i.e. location of bit-field in OTP memory
     * @param [in,out] reg_val      contents of register to modify with trim bit-field
     * @param [in] shift            location of bit-field in control port register in terms of bits from bit 0
     * @param [in] size             size of bit-field in bits
     *
     * @return
     * - CS35L41_STATUS_FAIL        if any pointers are NULL or if the bit-field size is 0
     * - CS35L41_STATUS_OK          otherwise
     *
     */
    uint32_t (*apply_trim_word)(uint8_t *otp_mem,
                                uint32_t bit_count,
                                uint32_t *reg_val,
                                uint32_t shift,
                                uint32_t size);

#ifdef INCLUDE_FW
    /**
     * Check HALO MBOX Status against the MBOX Command sent.
     *
     * Only some states of HALO MBOX Status are valid for each HALO MBOX Command
     *
     * @param [in] cmd              which HALO MBOX Command was most recently sent
     * @param [in] status           what is the HALO MBOX Status read
     *
     * @return
     * - true                       Status is correct/valid
     * - false                      Status is incorrect/invalid
     *
     */
    bool (*is_mbox_status_correct)(uint32_t cmd, uint32_t status);

    /**
     * Validates the boot configuration provided by the BSP.
     *
     * According to 'is_fw_boot' and 'is_coeff_boot' flags, checks that all required pointers to boot/coeff images are
     * not NULL.
     *
     * @param [in] config           pointer to boot configuration
     * @param [in] is_fw_boot       whether current action is booting FW; (true) is a FW boot action, (false) is NOT a
     *                              FW boot action
     * @param [in] is_coeff_boot    whether current action is booting COEFF; (true) is a COEFF boot action, (false) is
     *                              NOT a COEFF boot action
     *
     * @return
     * - CS35L41_STATUS_FAIL        if 'config' is NULL, if any required block pointers are NULL
     * - CS35L41_STATUS_OK          otherwise
     *
     */
    uint32_t (*validate_boot_config)(cs35l41_boot_config_t *config, bool is_fw_boot, bool is_coeff_boot);
#endif // INCLUDE_FW

    /**
     * Implements 'copy' method for Control Request Queue contents
     *
     * Initialization of f_queue_t requires a 'copy' method for copying the data type that comprises the elements of the
     * queue.  This implements the method for the queue used for Control Requests.
     *
     * @param [in] from             pointer to element to copy contents from
     * @param [in] to               pointer of empty element to which the contents should be copied
     *
     * @return
     * - false                      if either pointers are null
     * - true                       otherwise
     *
     * @see f_queue_copy
     * @see f_queue_if_t member initialize
     *
     */
    bool (*control_q_copy)(void *from, void *to);

    /**
     * Check that the currently processed Control Request is valid for the current state of the driver.
     *
     * Since the state of the driver is asynchronous to the Control Request currently being processed, cs35l41_process
     * needs to check whether the request is valid for the current state.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if no request is being processed, or if current request is invalid
     * - CS35L41_STATUS_OK          otherwise
     *
     */
    uint32_t (*is_control_valid)(cs35l41_t *driver);

    /**
     * Load new Control Request to be processed
     *
     * Removes next element from Control Request Queue and initializes the corresponding state machine.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if ID for new Control Request is not valid
     * - CS35L41_STATUS_OK          otherwise
     *
     */
    uint32_t (*load_control)(cs35l41_t *driver);

    /**
     * Maps IRQ Flag to Event ID passed to BSP
     *
     * Allows for abstracting driver events relayed to BSP away from IRQ flags, to allow the possibility that multiple
     * IRQ flags correspond to a single event to relay.
     *
     * @param [in] irq_statuses     pointer to array of 32-bit words from IRQ1_IRQ1_EINT_*_REG registers
     *
     * @return                      32-bit word with CS35L41_EVENT_FLAG_* set for each event detected
     *
     * @see CS35L41_EVENT_FLAG_
     *
     */
    uint32_t (*irq_to_event_id)(uint32_t *irq_statuses);

    /**
     * Apply all driver one-time configurations to corresponding Control Port register/memory addresses
     *
     * Performs the following:
     * - applies all configurations from cs35l41_audio_config_t
     * - applies all configurations from cs35l41_amp_config_t
     * - based on configurations, sets/clears hardware block enables
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS35L41_STATUS_FAIL        if any configuration parameters are outside bounds or do not result in proper
     *                              register bit-field encoding
     * - CS35L41_STATUS_OK          otherwise
     *
     */
    uint32_t (*apply_configs)(cs35l41_t *driver);

    /**
     * Checks all hardware mixer source selections for a specific source.
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] source           Mixer source to search for
     *
     * @return
     * - true                       Mixer source 'source' is used
     * - false                      Mixer source 'source' is not used
     *
     * @see CS35L41_INPUT_SRC_
     *
     */
    bool (*is_mixer_source_used)(cs35l41_t *driver, uint8_t source);
} cs35l41_private_functions_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/**
 * Pointer to Public API implementation
 */
extern cs35l41_functions_t *cs35l41_functions_g;

/**
 * Pointer to Private API implementation
 */
extern cs35l41_private_functions_t *cs35l41_private_functions_g;

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS35L41_H
