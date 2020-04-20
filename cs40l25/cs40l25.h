/**
 * @file cs40l25.h
 *
 * @brief Functions and prototypes exported by the CS40L25 Driver module
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
#include "cs40l25_spec.h"
#include "cs40l25_firmware.h"
#ifdef INCLUDE_CAL
#include "cs40l25_cal_firmware.h"
#endif // INCLUDE_CAL
#include "f_queue.h"

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
#define CS40L25_STATUS_BOOT_REQUEST                     (2)
#define CS40L25_STATUS_INVALID                          (3)
/** @} */

/**
 * @defgroup CS40L25_BUS_TYPE_
 * @brief Types of serial bus to control the CS40L25
 *
 * @see cs40l25_config_t member bus_type
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
 * @defgroup CS40L25_SM_STATE_
 * @brief Generic states used for all child state machines
 *
 * @see cs40l25_sm_t member state
 *
 * @{
 */
#define CS40L25_SM_STATE_INIT                           (0)
#define CS40L25_SM_STATE_DONE                           (0xFF)
#define CS40L25_SM_STATE_ERROR                          (0xFE)


/**
 * @defgroup CS40L25_EVENT_SM_STATE_
 * @brief States for the Event Handler state machine
 *
 * @see cs40l25_event_sm
 *
 * @{
 */
#define CS40L25_EVENT_SM_STATE_INIT                     (CS40L25_SM_STATE_INIT)
#define CS40L25_EVENT_SM_STATE_READ_IRQ_STATUS          (1)
#define CS40L25_EVENT_SM_STATE_READ_IRQ_MASK            (2)
#define CS40L25_EVENT_SM_STATE_CLEAR_IRQ_FLAGS          (3)
#define CS40L25_EVENT_SM_STATE_DISABLE_BOOST            (4)
#define CS40L25_EVENT_SM_STATE_TOGGLE_ERR_RLS           (5)
#define CS40L25_EVENT_SM_STATE_ENABLE_BOOST             (6)
#define CS40L25_EVENT_SM_STATE_DONE                     (CS40L25_SM_STATE_DONE)
#define CS40L25_EVENT_SM_STATE_ERROR                    (CS40L25_SM_STATE_ERROR)
/** @} */

/**
 * @defgroup CS40L25_RESET_SM_STATE_
 * @brief States for the Reset state machine
 *
 * @see cs40l25_reset_sm
 *
 * @{
 */
#define CS40L25_RESET_SM_STATE_INIT                     (CS40L25_SM_STATE_INIT)
#define CS40L25_RESET_SM_STATE_WAIT_T_RLPW              (1)
#define CS40L25_RESET_SM_STATE_WAIT_T_IRS               (2)
#define CS40L25_RESET_SM_STATE_WAIT_OTP_BOOT_DONE       (3)
#define CS40L25_RESET_SM_STATE_OTP_ERR_STATUS           (4)
#define CS40L25_RESET_SM_STATE_READ_ID                  (5)
#define CS40L25_RESET_SM_STATE_WAIT_BHM_BOOT_DONE       (6)
#define CS40L25_RESET_SM_STATE_DONE                     (CS40L25_SM_STATE_DONE)
#define CS40L25_RESET_SM_STATE_ERROR                    (CS40L25_SM_STATE_ERROR)
/** @} */

/**
 * @defgroup CS40L25_BOOT_SM_STATE_
 * @brief States for the Boot state machine
 *
 * @see cs40l25_boot_sm
 *
 * @{
 */
#define CS40L25_BOOT_SM_STATE_INIT                      (CS40L25_SM_STATE_INIT)
#define CS40L25_BOOT_SM_STATE_LOAD_CAL                  (1)
#define CS40L25_BOOT_SM_STATE_LOAD_FW                   (2)
#define CS40L25_BOOT_SM_STATE_LOAD_COEFF                (3)
#define CS40L25_BOOT_SM_STATE_POST_BOOT_CONFIG          (4)
#define CS40L25_BOOT_SM_STATE_WRITE_F0                  (5)
#define CS40L25_BOOT_SM_STATE_WRITE_REDC                (6)
#define CS40L25_BOOT_SM_STATE_WRITE_Q                   (7)
#define CS40L25_BOOT_SM_STATE_ERROR                     (CS40L25_SM_STATE_ERROR)
#define CS40L25_BOOT_SM_STATE_DONE                      (CS40L25_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS40L25_POWER_UP_SM_STATE_
 * @brief States for the Power Up state machine
 *
 * @see cs40l25_power_up_sm
 *
 * @{
 */
#define CS40L25_POWER_UP_SM_STATE_INIT                  (CS40L25_SM_STATE_INIT)
#define CS40L25_POWER_UP_SM_STATE_ERRATA                (1)
#define CS40L25_POWER_UP_SM_STATE_SET_FRAME_SYNC        (2)
#define CS40L25_POWER_UP_SM_STATE_PUP_PATCH             (3)
#define CS40L25_POWER_UP_SM_STATE_CLOCKS_TO_DSP         (4)
#define CS40L25_POWER_UP_SM_STATE_WAIT_HALO_STATE_T     (5)
#define CS40L25_POWER_UP_SM_STATE_WAIT_HALO_STATE       (6)
#define CS40L25_POWER_UP_SM_STATE_WAIT_HALO_SCRATCH     (7)
#define CS40L25_POWER_UP_SM_STATE_ERROR                 (CS40L25_SM_STATE_ERROR)
#define CS40L25_POWER_UP_SM_STATE_DONE                  (CS40L25_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS40L25_POWER_DOWN_SM_STATE_
 * @brief States for the Power Down state machine
 *
 * @see cs40l25_power_down_sm
 *
 * @{
 */
#define CS40L25_POWER_DOWN_SM_STATE_INIT                (CS40L25_SM_STATE_INIT)
#define CS40L25_POWER_DOWN_SM_STATE_BHM_SD_WAIT         (1)
#define CS40L25_POWER_DOWN_SM_STATE_BHM_SD_READ         (2)
#define CS40L25_POWER_DOWN_SM_STATE_BHM_SM_READ         (3)
#define CS40L25_POWER_DOWN_SM_STATE_BHM_AS_READ         (4)
#define CS40L25_POWER_DOWN_SM_STATE_BHM_REVERT_PATCH    (5)
#define CS40L25_POWER_DOWN_SM_STATE_CAL_START           (6)
#define CS40L25_POWER_DOWN_SM_STATE_CAL_TIMER           (7)
#define CS40L25_POWER_DOWN_SM_STATE_CAL_READ            (8)
#define CS40L25_POWER_DOWN_SM_STATE_MBOX_START          (9)
#define CS40L25_POWER_DOWN_SM_STATE_MBOX_TIMER          (10)
#define CS40L25_POWER_DOWN_SM_STATE_MBOX_READ           (11)
#define CS40L25_POWER_DOWN_SM_STATE_CORE_CTRL           (12)
#define CS40L25_POWER_DOWN_SM_STATE_COMPLETE            (13)
#define CS40L25_POWER_DOWN_SM_STATE_ERROR               (CS40L25_SM_STATE_ERROR)
#define CS40L25_POWER_DOWN_SM_STATE_DONE                (CS40L25_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS40L25_CONFIGURE_SM_STATE_
 * @brief States for the Configure state machine
 *
 * @see cs40l25_configure_sm
 *
 * @{
 */
#define CS40L25_CONFIGURE_SM_STATE_INIT                 (CS40L25_SM_STATE_INIT)
#define CS40L25_CONFIGURE_SM_STATE_UNLOCK_REGS          (1)
#define CS40L25_CONFIGURE_SM_STATE_READ_REGS            (2)
#define CS40L25_CONFIGURE_SM_STATE_WRITE_REGS           (3)
#define CS40L25_CONFIGURE_SM_STATE_LOCK_REGS            (4)
#define CS40L25_CONFIGURE_SM_STATE_ERROR                (CS40L25_SM_STATE_ERROR)
#define CS40L25_CONFIGURE_SM_STATE_DONE                 (CS40L25_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS40L25_FIELD_ACCESS_SM_STATE_
 * @brief States for the Field Access state machine
 *
 * @see cs40l25_field_access_sm
 *
 * @{
 */
#define CS40L25_FIELD_ACCESS_SM_STATE_INIT              (CS40L25_SM_STATE_INIT)
#define CS40L25_FIELD_ACCESS_SM_STATE_READ_MEM          (1)
#define CS40L25_FIELD_ACCESS_SM_STATE_WRITE_MEM         (2)
#define CS40L25_FIELD_ACCESS_SM_STATE_ACK_START         (3)
#define CS40L25_FIELD_ACCESS_SM_STATE_ACK_TIMER         (4)
#define CS40L25_FIELD_ACCESS_SM_STATE_ACK_READ          (5)
#define CS40L25_FIELD_ACCESS_SM_STATE_ERROR             (CS40L25_SM_STATE_ERROR)
#define CS40L25_FIELD_ACCESS_SM_STATE_DONE              (CS40L25_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS40L25_CALIBRATION_SM_STATE_
 * @brief States for the Calibration state machine
 *
 * @see cs40l25_calibration_sm
 *
 * @{
 */
#define CS40L25_CALIBRATION_SM_STATE_INIT               (CS40L25_SM_STATE_INIT)
#define CS40L25_CALIBRATION_SM_STATE_GET_VOL            (1)
#define CS40L25_CALIBRATION_SM_STATE_SET_VOL            (2)
// F0 path
#define CS40L25_CALIBRATION_SM_STATE_SET_MAXBEMF        (3)
#define CS40L25_CALIBRATION_SM_STATE_CLEAR_CLOSED_LOOP  (4)
#define CS40L25_CALIBRATION_SM_STATE_SET_F0_TRACK_1     (5)
#define CS40L25_CALIBRATION_SM_STATE_WAIT_500MS         (6)
#define CS40L25_CALIBRATION_SM_STATE_SET_CLOSED_LOOP    (7)
#define CS40L25_CALIBRATION_SM_STATE_WAIT_2S            (8)
#define CS40L25_CALIBRATION_SM_STATE_CLEAR_F0_TRACK     (9)
#define CS40L25_CALIBRATION_SM_STATE_READ_F0            (10)
#define CS40L25_CALIBRATION_SM_STATE_READ_REDC          (11)
#define CS40L25_CALIBRATION_SM_STATE_READ_MAXBEMF       (12)
// QEst path
#define CS40L25_CALIBRATION_SM_STATE_SET_F0_TRACK_2     (13)
#define CS40L25_CALIBRATION_SM_STATE_READ_F0_TRACK_T    (14)
#define CS40L25_CALIBRATION_SM_STATE_READ_F0_TRACK      (15)
#define CS40L25_CALIBRATION_SM_STATE_READ_QEST          (16)
// Tail
#define CS40L25_CALIBRATION_SM_STATE_RESTORE_VOL        (17)
#define CS40L25_CALIBRATION_SM_STATE_ERROR              (CS40L25_SM_STATE_ERROR)
#define CS40L25_CALIBRATION_SM_STATE_DONE               (CS40L25_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS40L25_GET_DSP_STATUS_SM_STATE_
 * @brief States for the Get DSP Status state machine
 *
 * @see cs40l25_get_dsp_status_sm
 *
 * @{
 */
#define CS40L25_GET_DSP_STATUS_SM_STATE_INIT            (CS40L25_SM_STATE_INIT)
#define CS40L25_GET_DSP_STATUS_SM_STATE_READ_STATUSES_1 (1)
#define CS40L25_GET_DSP_STATUS_SM_STATE_WAIT            (2)
#define CS40L25_GET_DSP_STATUS_SM_STATE_READ_STATUSES_2 (3)
#define CS40L25_GET_DSP_STATUS_SM_STATE_ERROR           (CS40L25_SM_STATE_ERROR)
#define CS40L25_GET_DSP_STATUS_SM_STATE_DONE            (CS40L25_SM_STATE_DONE)
/** @} */

/**
 * @defgroup CS40L25_FLAGS_
 * @brief Flags set by ISRs used to trigger transitions in state machines
 *
 * @see cs40l25_sm_t member flags
 *
 * @{
 */
#define CS40L25_FLAGS_TIMEOUT                           (0) ///< Flag for BSP timer timeout
#define CS40L25_FLAGS_CP_RW_DONE                        (1) ///< Flag for BSP Control Port Read/Write done
#define CS40L25_FLAGS_CP_RW_ERROR                       (2) ///< Flag for BSP Control Port Read/Write error
#define CS40L25_FLAGS_REQUEST_FW_BOOT                   (3) ///< Flag to indicate a request to boot firmware
#define CS40L25_FLAGS_REQUEST_COEFF_BOOT                (4) ///< Flag to indicate a request to boot coeff
#define CS40L25_FLAGS_REQUEST_CAL_BOOT                  (5) ///< Flag to indicate a request to boot calibration firmware
#define CS40L25_FLAGS_IS_GET_REQUEST                    (6) ///< Flag to indicate Field Access GET request
/** @} */

#define CS40L25_POLL_ACK_CTRL_MS                        (10)    ///< Delay in ms between polling OTP_BOOT_DONE
#define CS40L25_POLL_ACK_CTRL_MAX                       (10)    ///< Maximum number of times to poll OTP_BOOT_DONE

#define CS40L25_POLL_OTP_BOOT_DONE_MS                   (10)    ///< Delay in ms between polling OTP_BOOT_DONE
#define CS40L25_POLL_OTP_BOOT_DONE_MAX                  (10)    ///< Maximum number of times to poll OTP_BOOT_DONE
#define CS40L25_OTP_SIZE_WORDS                          (32)    ///< Total size of CS40L25 OTP in 32-bit words

#define CS40L25_CP_BULK_READ_LENGTH_BYTES               (CS40L25_OTP_SIZE_WORDS * 4)
///< Maximum size of Control Port Bulk Read
#define CS40L25_CP_REG_READ_LENGTH_BYTES                (4) ///< Length of Control Port Read of registers
/**
 * Length of Control Port Read buffer
 *
 * @attention The BSP is required to allocate a buffer of this length before initializing the driver.
 */
#define CS40L25_CP_READ_BUFFER_LENGTH_BYTES             (CS40L25_CP_BULK_READ_LENGTH_BYTES + \
                                                         CS40L25_CP_REG_READ_LENGTH_BYTES)

/**
 * Size of Control Request Queue
 *
 * @attention The system designer should size this queue to handle the maximum number of Control Requests the system
 * could possibly send to the driver before ability to process.
 */
#define CS40L25_CONTROL_REQUESTS_SIZE                   (20)

/**
 * @defgroup CS40L25_CONTROL_ID_
 * @brief ID to indicate the type of Control Request
 *
 * @see cs40l25_functions_t member control
 * @see cs40l25_control_request_t member id
 *
 * @{
 */
#define CS40L25_CONTROL_ID_NONE                           (0)
#define CS40L25_CONTROL_ID_RESET                          (1)
#define CS40L25_CONTROL_ID_BOOT                           (2)
#define CS40L25_CONTROL_ID_POWER_UP                       (3)
#define CS40L25_CONTROL_ID_POWER_DOWN                     (4)
#define CS40L25_CONTROL_ID_CONFIGURE                      (5)
#define CS40L25_CONTROL_ID_GET_VOLUME                     (6)
#define CS40L25_CONTROL_ID_SET_VOLUME                     (7)
#define CS40L25_CONTROL_ID_GET_HALO_HEARTBEAT             (8)
#define CS40L25_CONTROL_ID_SET_BHM_BUZZ_TRIGGER           (9)
#define CS40L25_CONTROL_ID_CALIBRATION                  (10)
#define CS40L25_CONTROL_ID_GET_DSP_STATUS               (11)
#define CS40L25_CONTROL_ID_SET_TRIGGER_INDEX            (12)
#define CS40L25_CONTROL_ID_SET_TRIGGER_MS               (13)
#define CS40L25_CONTROL_ID_SET_TIMEOUT_MS               (14)
#define CS40L25_CONTROL_ID_SET_GPIO_ENABLE              (15)
#define CS40L25_CONTROL_ID_SET_GPIO1_BUTTON_DETECT      (16)
#define CS40L25_CONTROL_ID_SET_GPIO2_BUTTON_DETECT      (17)
#define CS40L25_CONTROL_ID_SET_GPIO3_BUTTON_DETECT      (18)
#define CS40L25_CONTROL_ID_SET_GPIO4_BUTTON_DETECT      (19)
#define CS40L25_CONTROL_ID_SET_GPI_GAIN_CONTROL         (20)
#define CS40L25_CONTROL_ID_SET_CTRL_PORT_GAIN_CONTROL   (21)
#define CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_PRESS (22)
#define CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_PRESS (23)
#define CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_PRESS (24)
#define CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_PRESS (25)
#define CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_RELEASE (26)
#define CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_RELEASE (27)
#define CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_RELEASE (28)
#define CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_RELEASE (29)
#define CS40L25_CONTROL_ID_SET_CLAB_ENABLED             (30)
#define CS40L25_CONTROL_ID_GET_FW_REVISION              (31)
#define CS40L25_CONTROL_ID_MAX                          (CS40L25_CONTROL_ID_GET_FW_REVISION)
/** @} */

/**
 * @defgroup CS40L25_DSP_MBOX_STATUS_
 * @brief Statuses of the HALO DSP Mailbox
 *
 * @{
 */
#define CS40L25_DSP_MBOX_STATUS_RUNNING                 (0)
#define CS40L25_DSP_MBOX_STATUS_PAUSED                  (1)
#define CS40L25_DSP_MBOX_STATUS_RDY_FOR_REINIT          (2)
/** @} */

/**
 * @defgroup CS40L25_DSP_MBOX_CMD_
 * @brief HALO DSP Mailbox commands
 *
 * @see cs40l25_t member mbox_cmd
 *
 * @{
 */
#define CS40L25_DSP_MBOX_CMD_NONE                       (0)
#define CS40L25_DSP_MBOX_CMD_PAUSE                      (1)
#define CS40L25_DSP_MBOX_CMD_RESUME                     (2)
#define CS40L25_DSP_MBOX_CMD_REINIT                     (3)
#define CS40L25_DSP_MBOX_CMD_STOP_PRE_REINIT            (4)
#define CS40L25_DSP_MBOX_CMD_UNKNOWN                    (-1)
/** @} */

/**
 * @defgroup CS40L25_POWER_
 * @brief Power states passed on to power() API argument power_state
 *
 * @see cs40l25_functions_t member power
 *
 * @{
 */
#define CS40L25_POWER_UP                                (0)
#define CS40L25_POWER_DOWN                              (1)
/** @} */

/**
 * @defgroup CS40L25_CALIB_
 * @brief Calibration options passed on to calibrate() API argument calib_type
 *
 * @see cs40l25_functions_t member calibrate
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
#define CS40L25_EVENT_FLAG_AMP_SHORT                    (0)
#define CS40L25_EVENT_FLAG_OVERTEMP                     (1)
#define CS40L25_EVENT_FLAG_BOOST_INDUCTOR_SHORT         (2)
#define CS40L25_EVENT_FLAG_BOOST_UNDERVOLTAGE           (3)
#define CS40L25_EVENT_FLAG_BOOST_OVERVOLTAGE            (4)
#define CS40L25_EVENT_FLAG_SM_ERROR                     (5)
/** @} */

#define CS40L25_CONFIG_REGISTERS_TOTAL                  (39)    ///< Total registers modified during Configure SM
#define CS40L25_CONFIG_REGISTERS_CODEC                  (26)    ///< Total codec registers modified during Configure SM

#define CS40L25_INPUT_SRC_DISABLE                       (0x00)  ///< Data Routing value to indicate 'disabled'

#ifdef INCLUDE_CAL
#define DSP_REG(X)                                      (driver->state == CS40L25_STATE_DSP_POWER_UP || driver->state == CS40L25_STATE_DSP_STANDBY ? CS40L25_ ## X : CS40L25_CAL_ ## X)
#else
#define DSP_REG(X)                                      (CS40L25_ ## X)
#endif // INCLUDE_CAL
#define CS40L25_DSP_STATUS_WORDS_TOTAL                  (2)     ///< Total registers to read for Get DSP Status control

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/**
 * @defgroup CS40L25_FLAG_MACROS
 * @brief Macros to get/set bitfield flags
 *
 * @{
 */
#define CS40L25_FLAG_TO_MASK(A)                (0x1 << A)
#define CS40L25_FLAG_VAL_TO_MASK(A, B)         (B << A)
#define CS40L25_SET_FLAG(A, B)                 (A |= CS40L25_FLAG_TO_MASK(B))
#define CS40L25_CLEAR_FLAG(A, B)               (A &= ~(CS40L25_FLAG_TO_MASK(B)))
#define CS40L25_IS_FLAG_SET(A, B)              ((A & CS40L25_FLAG_TO_MASK(B)) == CS40L25_FLAG_TO_MASK(B))
#define CS40L25_IS_FLAG_CLEAR(A, B)            ((A & CS40L25_FLAG_TO_MASK(B)) == 0)
#define CS40L25_CHANGE_FLAG(A, B, C)           (A = ((A & ~CS40L25_FLAG_TO_MASK(B)) | CS40L25_FLAG_VAL_TO_MASK(B, C)))
/** @} */

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

#define CS40L25_MAX_COEFF_FILES                         (3)

/**
 * Wrapper for lists of halo blocks used by cs40l24_boot_config_t
 */
typedef struct
{
    halo_boot_block_t *data; ///< Pointer to list of data blocks
    uint32_t total_blocks;  ///< Number of data blocks in list

} cs40l25_halo_boot_file_t;

/**
 * Data structure to describe HALO firmware and coefficient download.
 */
typedef struct
{
    uint32_t total_fw_blocks;                                          ///< Total blocks of firmware payload
    halo_boot_block_t *fw_blocks;                                      ///< Pointer to list of firmware boot blocks
    uint32_t total_coeff_blocks;                                       ///< Total blocks of payload of all coeff files
    cs40l25_halo_boot_file_t coeff_files[CS40L25_MAX_COEFF_FILES];     ///< List of coeffiecient boot block list wrappers
    uint32_t total_cal_blocks;                                          ///< Total blocks of calibration firmware payload
    halo_boot_block_t *cal_blocks;                                      ///< Pointer to list of calibration firmware boot blocks
} cs40l25_boot_config_t;

/**
 * Function pointer to driver control state machine implementation.
 *
 * @param [in] driver           Pointer to the driver state
 *
 * @return General driver API call status
 */
typedef uint32_t (*cs40l25_sm_fp_t)(void *driver);

/**
 * Data structure describing driver control state machine implementation
 */
typedef struct
{
    uint8_t state;      ///< Current state of state machine
    uint32_t flags;     ///< Current event/notification flags the state machine should process
    uint8_t count;      ///< Counter allocated to simplify number of states
    cs40l25_sm_fp_t fp; ///< Pointer to state machine implementation
} cs40l25_sm_t;

/**
 * Function pointer to Notification Callback
 *
 * @details
 * This callback will be registered at driver configuration.  This callback is called whenever the driver has detected
 * a significant event has occurred, such as an over-temperature condition.
 *
 * @see cs40l25_functions_t member configure
 *
 * @param [in] event_flags      Flags to indicate which events have occurred
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs40l25_notification_callback_t)(uint32_t event_flags, void *arg);

/**
 * Function pointer to Control Request Callback
 *
 * @details
 * This callback will be registered when the Control Request is requested, i.e. via control(), boot() API.
 *
 * @see cs40l25_functions_t member control
 *
 * @param [in] id               Control ID just completed
 * @param [in] status           Status of Control Request
 * @param [in] arg              Callback arg registered by upper layer
 *
 * @return none
 */
typedef void (*cs40l25_control_callback_t)(uint8_t id, uint32_t status, void *arg);

/**
 * Data structure to describe a Control Request
 *
 * @see cs40l25_functions_t member control
 */
typedef struct
{
    uint8_t id;                     ///< Control ID
    void *arg;                      ///< Argument for Control Request (nature depends on type of request)
    cs40l25_control_callback_t cb;  ///< Control Request complete callback
    void *cb_arg;                   ///< Argument for Control Request complete callback
} cs40l25_control_request_t;

/**
 * Data structure to describe a field to read via the Field Access SM
 *
 * @see cs40l25_load_control
 */
typedef struct
{
    uint32_t address;   ///< Control Port address of field to access
    uint32_t value;     ///< Value to write/value read
    uint8_t size;       ///< Bitwise size of field to access in register
    uint8_t shift;      ///< Bitwise shift of field to access in register
    bool ack_ctrl;      ///< (True) Signal field is an acknowledge control
    uint32_t ack_reset;     ///< The value the field should reset to on ack (only valid for ack ctrls)
} cs40l25_field_accessor_t;

/**
 * Configuration of amplifier audio hardware
 */
typedef struct
{
    bool is_master_mode;    ///< (True) Set ASP in Master Mode
    bool fsync_inv;         ///< (True) Invert polarity of FSYNC
    bool bclk_inv;          ///< (True) Invert polarity of BCLK
    bool amp_dre_en;        ///< (True) Enable Amplifier DRE
    bool ng_enable;         ///< (True) Enable Noise Gate
    uint8_t ng_thld;        ///< Noise Gate threshold.  See datasheet Section 7.19.3
    uint8_t ng_delay;       ///< Noise Gate delay.    See datasheet Section 7.19.3
    uint8_t amp_ramp_pcm;   ///< Amplifier PCM audio digital soft-ramp rate.  See datasheet Section 7.17.1
} cs40l25_audio_hw_config_t;

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
} cs40l25_asp_config_t;

/**
 * Routing of audio data to Amplifier DAC, DSP, and ASP TX channels
 *
 * @see CS40L25_INPUT_SRC_
 */
typedef struct
{
    uint8_t dac_src;        ///< Amplifier DAC audio mixer source
    uint8_t dsp_rx1_src;    ///< DSP RX Channel 1 audio mixer source
    uint8_t dsp_rx2_src;    ///< DSP RX Channel 2 audio mixer source
    uint8_t dsp_rx3_src;    ///< DSP RX Channel 3 audio mixer source
    uint8_t dsp_rx4_src;    ///< DSP RX Channel 4 audio mixer source
    uint8_t asp_tx1_src;    ///< ASP TX Channel 1 audio mixer source
    uint8_t asp_tx2_src;    ///< ASP TX Channel 2 audio mixer source
    uint8_t asp_tx3_src;    ///< ASP TX Channel 3 audio mixer source
    uint8_t asp_tx4_src;    ///< ASP TX Channel 4 audio mixer source
} cs40l25_routing_config_t;

/**
 * Configuration of internal clocking.
 */
typedef struct
{
    uint8_t refclk_sel;     ///< Clock source for REFCLK @see CS40L25_PLL_REFLCLK_SEL_
    uint32_t sclk;          ///< BCLK (or SCLK) frequency in Hz
    uint32_t refclk_freq;   ///< REFCLK frequency in Hz
    uint32_t global_fs;     ///< FSYNC frequency in Hz
} cs40l25_clock_config_t;

/**
 * Collection of audio-related configurations
 */
typedef struct
{
    cs40l25_audio_hw_config_t hw;
    cs40l25_asp_config_t asp;
    cs40l25_routing_config_t routing;
    cs40l25_clock_config_t clock;
    uint16_t volume;    ///< Volume to be applied at reset
} cs40l25_audio_config_t;

/**
 * Amplifier-related configurations
 */
typedef struct
{
    uint16_t boost_inductor_value_nh;   ///< Boost inductor value in nH
    uint16_t boost_capacitor_value_uf;  ///< Boost capacitor value in uF
    uint16_t boost_ipeak_ma;            ///< Boost peak current in mA
    uint8_t bst_ctl;                    ///< Boost converter target voltage.  See datasheet Section 7.11.1
    bool classh_enable;                 ///< (True) Enable Class H functionality
    uint8_t bst_ctl_sel;                ///< Boost converter control source selection.  See datasheet Section 7.11.2
    bool bst_ctl_lim_en;                ///< Class H boost control max limit.  See datasheet Section 7.11.2
} cs40l25_amp_config_t;

/**
 * Registers modified for amplifier configuration.
 *
 * List of registers can be accessed via bitfields (when mapping from driver config/state), or indexed via words
 * (when reading/writing via Control Port).
 *
 * All register types are defined according to the datasheet and specified in cs40l25_spec.h.
 *
 * @warning  The list of registers MUST correspond to the addresses in cs40l25_config_register_addresses.
 *
 * @see cs40l25_config_register_addresses
 */
typedef union
{
    uint32_t words[CS40L25_CONFIG_REGISTERS_TOTAL];

    struct
    {
        cs40l25_intp_amp_ctrl_t intp_amp_ctrl;
        cs40l25_mixer_t asptx1_input;
        cs40l25_mixer_t asptx2_input;
        cs40l25_mixer_t asptx3_input;
        cs40l25_mixer_t asptx4_input;
        cs40l25_mixer_t dsp1rx1_input;
        cs40l25_mixer_t dsp1rx2_input;
        cs40l25_mixer_t dsp1rx3_input;
        cs40l25_mixer_t dsp1rx4_input;
        cs40l25_mixer_t dacpcm1_input;
        cs40l25_ccm_refclk_input_t ccm_refclk_input;
        cs40l25_msm_block_enables_t msm_block_enables;
        cs40l25_msm_block_enables2_t msm_block_enables2;
        cs40l25_dataif_asp_enables1_t dataif_asp_enables1;
        cs40l25_dataif_asp_control2_t dataif_asp_control2;
        cs40l25_dataif_asp_frame_control5_t dataif_asp_frame_control5;
        cs40l25_dataif_asp_frame_control1_t dataif_asp_frame_control1;
        cs40l25_dataif_asp_data_control5_t dataif_asp_data_control5;
        cs40l25_dataif_asp_data_control1_t dataif_asp_data_control1;
        uint32_t ccm_fs_mon0;
        cs40l25_dataif_asp_control1_t dataif_asp_control1;
        cs40l25_boost_lbst_slope_t boost_lbst_slope;
        cs40l25_boost_bst_loop_coeff_t boost_bst_loop_coeff;
        cs40l25_boost_bst_ipk_ctl_t boost_bst_ipk_ctl;
        cs40l25_boost_vbst_ctl_1_t boost_vbst_ctl_1;
        cs40l25_boost_vbst_ctl_2_t boost_vbst_ctl_2;
        dsp_gpio_button_detect_reg_t dsp_gpio_button_detect;
        dsp_reg_t dsp_gpio_enable;
        dsp_gain_control_reg_t dsp_gain_control;
        dsp_reg_t dsp_gpio1_index_button_press;
        dsp_reg_t dsp_gpio2_index_button_press;
        dsp_reg_t dsp_gpio3_index_button_press;
        dsp_reg_t dsp_gpio4_index_button_press;
        dsp_reg_t dsp_gpio1_index_button_release;
        dsp_reg_t dsp_gpio2_index_button_release;
        dsp_reg_t dsp_gpio3_index_button_release;
        dsp_reg_t dsp_gpio4_index_button_release;
        dsp_reg_t clab_enabled;
        dsp_reg_t peak_amplitude_control;
    };
} cs40l25_config_registers_t;

/** List of firmware controls configured through cs40l25_config_t
 *
 * These values will be mapped to a register in cs40l25_config_registers_t;
 *
 * @see cs40l25_config_registers_t
 */
typedef struct
{
    bool dsp_gpio1_button_detect_enable;
    bool dsp_gpio2_button_detect_enable;
    bool dsp_gpio3_button_detect_enable;
    bool dsp_gpio4_button_detect_enable;
    bool dsp_gpio_enable;
    uint16_t dsp_gpi_gain_control;
    uint16_t dsp_ctrl_gain_control;
    uint32_t dsp_gpio1_index_button_press;
    uint32_t dsp_gpio2_index_button_press;
    uint32_t dsp_gpio3_index_button_press;
    uint32_t dsp_gpio4_index_button_press;
    uint32_t dsp_gpio1_index_button_release;
    uint32_t dsp_gpio2_index_button_release;
    uint32_t dsp_gpio3_index_button_release;
    uint32_t dsp_gpio4_index_button_release;
    bool clab_enable;          ///< Whether Closed-loop Active Braking shoud be enabled.
    uint32_t peak_amplitude;   ///< Clab braking pulses are scaled down if they exceed this amplitude. Q2.22
} cs40l25_dsp_config_controls_t;

/**
 * State of HALO FW Calibration
 *
 */
typedef struct
{
    bool is_valid_f0;    ///< (True) Calibration state is valid
    uint32_t f0;      ///< Encoded resonant frequency (f0) determined by Calibration procedure.
    uint32_t redc;    ///< Encoded DC resistance (ReDC) determined by Calibration procedure.
    uint32_t backemf; ///< Encoded Back EMF determined by Calibration procedure.
    bool is_valid_qest;    ///< (True) Calibration state is valid
    uint32_t qest;   ///< Encoded estimated Q value (Q Est) determined by Calibration procedure.
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
} cs40l25_dsp_status_t;

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
 * Driver state data structure
 *
 * This is the type used for the handle to the driver for all driver public API calls.  This structure must be
 * instantiated outside the scope of the driver source and initialized by the initialize() public API.
 */
typedef struct
{
    // Primary driver state fields
    uint32_t state;             ///< General driver state - @see CS40L25_STATE_
    uint32_t mode;              ///< General driver mode - @see CS40L25_MODE_
    cs40l25_sm_t control_sm;    ///< Control State Machine for current Control Request
    cs40l25_sm_t event_sm;      ///< Event Handler State Machine

    // Control Request handling fields
    f_queue_t control_q;                                                        ///< Queue of Control Requests
    cs40l25_control_request_t control_requests[CS40L25_CONTROL_REQUESTS_SIZE];  ///< Buffered Control Requests
    cs40l25_control_request_t current_request;                                  ///< Current Control Request

    // Extra state material used by Control and Event Handler SMs
    uint32_t register_buffer;                   ///< buffer for reading Control Port registers
    uint32_t devid;                             ///< CS40L25 DEVID of current device
    uint32_t revid;                             ///< CS40L25 REVID of current device
    const uint32_t *errata;                     ///< Pointer to relevant errata for the current device
    uint32_t mbox_cmd;                          ///< HALO FW Mailbox command to be sent
    cs40l25_config_registers_t config_regs;     ///< Contents of Control Port registers to configure
    cs40l25_dsp_config_controls_t dsp_config_ctrls; ///< Configurable DSP controls
    uint32_t calib_pcm_vol;
    cs40l25_field_accessor_t field_accessor;    ///< Current Control Port field to access

    // Driver configuration fields - see cs40l25_config_t
    uint8_t bsp_dev_id;
    uint32_t bsp_reset_gpio_id;
    uint32_t bsp_int_gpio_id;
    uint8_t bus_type;
    uint8_t *cp_write_buffer;
    uint8_t *cp_read_buffer;
    cs40l25_notification_callback_t notification_cb;
    void *notification_cb_arg;
    cs40l25_audio_config_t audio_config;
    cs40l25_amp_config_t amp_config;
    cs40l25_calibration_t cal_data;

    cs40l25_boot_config_t *boot_config;     ///< Current HALO FW/Coefficient boot configuration
} cs40l25_t;

/**
 * Driver configuration data structure
 *
 * @see cs40l25_functions_t member configure
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
    cs40l25_audio_config_t audio_config;                ///< Amplifier audio-related configuration
    cs40l25_amp_config_t amp_config;                    ///< Amplifier amp-related configuration
    cs40l25_calibration_t cal_data;                     ///< Calibration data from previous calibration sequence
    cs40l25_dsp_config_controls_t dsp_config_ctrls;     ///< Configurable DSP controls
} cs40l25_config_t;

/**
 * Driver public API
 *
 * All API calls require a handle (cs40l25_t *) to the driver state.
 * All API calls return a status @see CS40L25_STATUS_
 *
 * @attention The process() API must be called in either (1) a baremetal superloop or (2) an RTOS task loop in order to
 * execute the following APIs:
 * - control()
 * - reset()
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
     * - CS40L25_STATUS_FAIL        if call to f_queue_if_t fails
     * - CS40L25_STATUS_OK          otherwise
     *
     */
    uint32_t (*initialize)(cs40l25_t *driver);

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
    uint32_t (*configure)(cs40l25_t *driver, cs40l25_config_t *config);

    /**
     * Processes driver state machines
     *
     * This implements the 'CS40L25 Process Flowchart' found in the driver Tech Note.  This includes:
     * - calling Event Handler SM if in HANDLING_EVENTS mode
     * - calling current Control SM if in HANLDING_CONTROLS mode
     * - loading new controls and checking validity per driver state
     * - calling BSP callbacks upon completion of Control Requests
     * - handling Control Request errors
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - if in UNCONFIGURED or ERROR state, returns CS40L25_STATUS_OK
     * - else if in HANDLING_CONTROLS mode and not currently processing Control SM< returns CS40L25_STATUS_OK
     * - otherwise, returns status from any state machine executed (either Event Handler or Control SM)
     *
     * @warning This MUST be placed either in baremetal or RTOS task while (1)
     *
     */
    uint32_t (*process)(cs40l25_t *driver);

    /**
     * Submit a Control Request to the driver
     *
     * Caller will initialize a cs40l25_control_request_t 'req' based on the control it wishes to access.  This request
     * will then be added to the Control Request Queue, and the callback given in 'req' will be called once either:
     * - the Control Request has completed successfully
     * - there was an ERROR when executing the Control Request
     * - the Control Request is invalid for the current driver state
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] req              data structure for control request \b passed by value
     *
     * @return
     * - CS40L25_STATUS_FAIL        if Control Request ID is invalid OR if insertion into Control Request Queue failed
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see cs40l25_control_request_t
     *
     */
    uint32_t (*control)(cs40l25_t *driver, cs40l25_control_request_t req);

    /**
     * Reset the CS40L25
     *
     * Reset will include stepping through the following state machine:
     * - Reset SM - toggle RESET GPIO, identify device
     *
     * This function is essentially an alias for a call to cs40l25_control
     *
     * @attention It should be noted that the callback \b cb will be called upon completion (either successful or
     * unsuccessful) of the Reset Control Request when resetting.
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] cb               Callback for completion of each control stage of reset process
     * @param [in] cb_arg           Pointer to argument to use for callback
     *
     * @return
     * - CS40L25_STATUS_FAIL        if any submission of Control Request failed
     * - CS40L25_STATUS_OK          otherwise
     *
     */
    uint32_t (*reset)(cs40l25_t *driver, cs40l25_control_callback_t cb, void *cb_arg);

    /**
     * Boot the CS40L25
     *
     * Boot may include stepping through the following state machines:
     * - Boot SM - load a HALO DSP FW image and COEFF image
     * - Configure SM - apply the driver configuration to the CS40L25 register file
     *
     * This function is essentially an alias for multiple calls to cs40l25_control - it submits Control Requests for the
     * controls above that apply to the current driver configuration.
     *
     * @attention It should be noted that the callback \b cb will be called upon completion (either successful or
     * unsuccessful) of \b each Control Request when booting.  Thus the caller could get the callback called for BOOT and CONFIGURE controls.
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] cal_boot         Boolean to specify whether this is a calibration boot.
     * @param [in] cb               Callback for completion of each control stage of boot process
     * @param [in] cb_arg           Pointer to argument to use for callback
     *
     * @return
     * - CS40L25_STATUS_FAIL        if any submission of Control Request failed
     * - CS40L25_STATUS_OK          otherwise
     *
     */
    uint32_t (*boot)(cs40l25_t *driver, bool cal_boot, cs40l25_control_callback_t cb, void *cb_arg);

    /**
     * Change the power state
     *
     * This submits a Control Request to either Power Up or Power Down the CS40L25.  Although there is no checking here
     * for whether the request is invalid (i.e. Power Down when the driver state is already STANDBY), this checking will
     * be performed once the request is loaded in cs40l25_process().
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] power_state      New power state
     * @param [in] cb               Callback for completion of each control stage of boot process
     * @param [in] cb_arg           Pointer to argument to use for callback
     *
     * @return
     * - CS40L25_STATUS_FAIL        if submission of Control Request failed
     * - CS40L25_STATUS_OK          otherwise
     *
     */
    uint32_t (*power)(cs40l25_t *driver, uint32_t power_state, cs40l25_control_callback_t cb, void *cb_arg);

    /**
     * Calibrate the HALO DSP Protection Algorithm
     *
     * This performs the calibration procedure for Prince Haptic Control firmwares.
     * This calibration information (cs40l25_calibration_t) will be saved in the driver state
     * and applied during subsequent boots of the part.  This calibration information will be available to the driver
     * until the driver is re-initialized.
     *
     * @param [in] driver               Pointer to the driver state
     * @param [in] calib_type           The calibration type to be performed
     * @param [in] cb                   Callback for completion of each control stage of boot process
     * @param [in] cb_arg               Pointer to argument to use for callback
     *
     * @return
     * - CS40L25_STATUS_FAIL        if submission of Control Request failed
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see cs40l25_calibration_t
     *
     */
    uint32_t (*calibrate)(cs40l25_t *driver,
                          uint32_t calib_type,
                          cs40l25_control_callback_t cb,
                          void *cb_arg);
} cs40l25_functions_t;

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
     * is to set the CS40L25_FLAGS_TIMEOUT for the state machine being currently processed.
     *
     * @param [in] status           Status of the asynchronous call to set_timer()
     * @param [in] cb_arg           A pointer to callback argument registered for set_timer().  For the driver, this arg
     *                              is used for a pointer to the driver state cs40l25_t.
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
     *                              pointer to the driver state cs40l25_t.
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
     *                              pointer to the driver state cs40l25_t.
     *
     * @return none
     *
     * @see bsp_driver_if_t member i2c_write, i2c_db_write.
     * @see bsp_callback_t
     *
     */
    void (*cp_write_callback)(uint32_t status, void *cb_arg);

    /**
     * Notify the driver when the CS40L25 INTb GPIO drops low.
     *
     * This callback is registered with the BSP in the register_gpio_cb() API call.
     *
     * The primary task of this callback is to transition the driver mode from CS40L25_MODE_HANDLING_CONTROLS to
     * CS40L25_MODE_HANDLING_EVENTS, in order to start processing the Event Handler state machine.  It also resets the
     * Event Handler state machine, if it was not currently being processed.
     *
     * @param [in] status           Status of the asynchronous call to BSP Control Port write
     * @param [in] cb_arg           A pointer to callback argument registered.  For the driver, this arg is used for a
     *                              pointer to the driver state cs40l25_t.
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
     * - CS40L25_STATUS_FAIL        if the call to BSP failed
     * - CS40L25_STATUS_OK          otherwise
     *
     * @warning Contains platform-dependent code.
     *
     */
    uint32_t (*read_reg)(cs40l25_t *driver, uint32_t addr, uint32_t *val, bool is_blocking);

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
     * - CS40L25_STATUS_FAIL        if the call to BSP failed
     * - CS40L25_STATUS_OK          otherwise
     *
     * @warning Contains platform-dependent code.
     *
     */
    uint32_t (*write_reg)(cs40l25_t *driver, uint32_t addr, uint32_t val, bool is_blocking);

    /**
     * Reset State Machine
     *
     * This state machine performs all necessary steps to reset the CS40L25 and put it into POWER_UP state.  The state
     * machine design is documented in the Reset State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Reset State Diagram
     *
     */
    uint32_t (*reset_sm)(cs40l25_t *driver);

    /**
     * Boot State Machine
     *
     * This state machine loads FW and COEFF images into the CS40L25 HALO DSP memory regions.  Completing the
     * state machine results in the driver transition to DSP_STANDBY state, or the CAL_STANDBY state for the
     * calibration FW.  The state machine design is documented in the Boot State Diagram found in the Driver
     * Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Boot State Diagram
     *
     */
    uint32_t (*boot_sm)(cs40l25_t *driver);

    /**
     * Power Up State Machine
     *
     * This state machine performs all necessary steps to transition the CS40L25 to be ready to process haptic events.
     *  Completing the state machine results in the driver transition to the DSP_POWER_UP state.  The state
     * machine design is documented in the Power Up State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Power Up State Diagram
     *
     */
    uint32_t (*power_up_sm)(cs40l25_t *driver);

    /**
     * Power Down State Machine
     *
     * This state machine performs all necessary steps to transition the CS40L25 to be in Standby power mode. Completing
     * the state machine results in the driver transition to STANDBY, DSP_STANDBY or CAL_STANDBY state.  The state machine
     * design is documented in the Power Down State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Power Down State Diagram
     *
     */
    uint32_t (*power_down_sm)(cs40l25_t *driver);

    /**
     * Configure State Machine
     *
     * This state machine performs all CS40L25 configuration required after Reset and DSP Boot.
     * Completing the state machine does not result in any driver state change.  Although the configuration should
     * almost always be performed when the driver is in STANDBY or DSP_STANDBY state, it is not prohibited.  The state
     * machine design is documented in the Configure State Diagram found in the Driver Tech Note.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Configure State Diagram
     * @see cs40l25_apply_configs()
     *
     */
    uint32_t (*configure_sm)(cs40l25_t *driver);

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
     * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Field Access State Diagram
     * @see cs40l25_field_accessor_t
     *
     */
    uint32_t (*field_access_sm)(cs40l25_t *driver);

    /**
     * Calibration State Machine
     *
     * This state machine performs the Calibration sequence for Prince Haptic Control firmwares.
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
     * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Calibration State Diagram
     * @see cs40l25_calibration_t
     *
     */
    uint32_t (*calibration_sm)(cs40l25_t *driver);

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
     * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Get DSP Status State Diagram
     * @see cs40l25_dsp_status_t
     *
     */
    uint32_t (*get_dsp_status_sm)(cs40l25_t *driver);

    /**
     * Event Handler State Machine
     *
     * This state machine performs all steps to handle IRQ and other asynchronous events the driver is aware of,
     * resulting in calling of the notification callback (cs40l25_notification_callback_t).
     *
     * Beginning the state machine results in transition of driver \b mode from HANDLING_CONTROLS to HANDLING_EVENTS,
     * while completing the state machine will result in transition of driver \b mode from HANDLING_EVENTS to
     * HANDLING_CONTROLS in cs40l25_process(). The state machine design is documented in the Event Handler State Diagram
     * found in the Driver Tech Note.
     *
     * If there are any IRQ events that include Speaker-Safe Mode Errors or Boost-related events, then the procedure
     * outlined in the Datasheet Section 4.16.1.1 is implemented here.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS40L25_STATUS_FAIL        if state machine transitioned to ERROR state for any reason
     * - CS40L25_STATUS_OK          otherwise
     *
     * @see Driver Tech Note, Event Handler State Diagram
     * @see cs40l25_notification_callback_t
     *
     */
    uint32_t (*event_sm)(void *driver);

    /**
     * Gets pointer to correct errata based on DEVID/REVID
     *
     * @param [in] devid            DEVID read from CS40L25_SW_RESET_DEVID_REG
     * @param [in] revid            REVID read from CS40L25_SW_RESET_REVID_REG
     * @param [out] errata          Pointer to array of uint32_t implementing errata
     *
     * @return
     * - CS40L25_STATUS_FAIL        corresponding errata not found
     * - CS40L25_STATUS_OK          otherwise
     *
     */
    uint32_t (*get_errata)(uint32_t devid, uint32_t revid, const uint32_t **errata);

    /**
     * Reads contents from a consecutive number of memory addresses
     *
     * Starting at 'addr', this will read 'length' number of 32-bit values into the BSP-allocated buffer from the
     * control port.  This bulk read will place contents into the BSP buffer starting at the 4th byte address.
     * Bytes 0-3 in the buffer are reserved for non-bulk reads (i.e. calls to cs40l25_read_reg).  This control port
     * call only supports non-blocking calls.  This function also only supports I2C transactions.
     *
     * @param [in] driver           Pointer to the driver state
     * @param [in] addr             32-bit address to be read
     * @param [in] length           number of memory addresses (i.e. 32-bit words) to read
     *
     * @return
     * - CS40L25_STATUS_FAIL        if the call to BSP failed, or if 'length' exceeds the size of BSP buffer
     * - CS40L25_STATUS_OK          otherwise
     *
     * @warning Contains platform-dependent code.
     *
     */
    uint32_t (*cp_bulk_read)(cs40l25_t *driver, uint32_t addr, uint32_t length);

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
     * - CS40L25_STATUS_FAIL        if the call to BSP failed
     * - CS40L25_STATUS_OK          otherwise
     *
     * @warning Contains platform-dependent code.
     *
     */
    uint32_t (*cp_bulk_write)(cs40l25_t *driver, uint32_t addr, uint8_t *bytes, uint32_t length);

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
     * - CS40L25_STATUS_FAIL        if 'config' is NULL, if any required block pointers are NULL
     * - CS40L25_STATUS_OK          otherwise
     *
     */
    uint32_t (*validate_boot_config)(cs40l25_boot_config_t *config, bool is_fw_boot, bool is_coeff_boot, bool is_cal_boot);

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
     * Since the state of the driver is asynchronous to the Control Request currently being processed, cs40l25_process
     * needs to check whether the request is valid for the current state.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS40L25_STATUS_FAIL        if no request is being processed, or if current request is invalid
     * - CS40L25_STATUS_OK          otherwise
     *
     */
    uint32_t (*is_control_valid)(cs40l25_t *driver);

    /**
     * Load new Control Request to be processed
     *
     * Removes next element from Control Request Queue and initializes the corresponding state machine.
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS40L25_STATUS_FAIL        if ID for new Control Request is not valid
     * - CS40L25_STATUS_OK          otherwise
     *
     */
    uint32_t (*load_control)(cs40l25_t *driver);

    /**
     * Maps IRQ Flag to Event ID passed to BSP
     *
     * Allows for abstracting driver events relayed to BSP away from IRQ flags, to allow the possibility that multiple
     * IRQ flags correspond to a single event to relay.
     *
     * @param [in] irq_statuses     pointer to array of 32-bit words from IRQ1_IRQ1_EINT_*_REG registers
     *
     * @return                      32-bit word with CS40L25_EVENT_FLAG_* set for each event detected
     *
     * @see CS40L25_EVENT_FLAG_
     *
     */
    uint32_t (*irq_to_event_id)(uint32_t *irq_statuses);

    /**
     * Apply all driver one-time configurations to corresponding Control Port register/memory addresses
     *
     * Performs the following:
     * - applies all configurations from cs40l25_audio_config_t
     * - applies all configurations from cs40l25_amp_config_t
     * - based on configurations, sets/clears hardware block enables
     *
     * @param [in] driver           Pointer to the driver state
     *
     * @return
     * - CS40L25_STATUS_FAIL        if any configuration parameters are outside bounds or do not result in proper
     *                              register bit-field encoding
     * - CS40L25_STATUS_OK          otherwise
     *
     */
    uint32_t (*apply_configs)(cs40l25_t *driver);

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
     * @see CS40L25_INPUT_SRC_
     *
     */
    bool (*is_mixer_source_used)(cs40l25_t *driver, uint8_t source);
} cs40l25_private_functions_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/**
 * Pointer to Public API implementation
 */
extern cs40l25_functions_t *cs40l25_functions_g;

/**
 * Pointer to Private API implementation
 */
extern cs40l25_private_functions_t *cs40l25_private_functions_g;

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L25_H
