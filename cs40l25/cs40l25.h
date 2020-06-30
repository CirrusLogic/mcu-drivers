/**
 * @file cs40l25.h
 *
 * @brief Functions and prototypes exported by the CS40L25 Driver module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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
#include "fw_img_v1.h"
#include "cs40l25_sym.h"
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
#define CS40L25_EVENT_SM_STATE_READ_SCRATCH             (1)
#define CS40L25_EVENT_SM_STATE_READ_EVENT               (2)
#define CS40L25_EVENT_SM_STATE_CLEAR_EVENT              (3)
#define CS40L25_EVENT_SM_STATE_READ_IRQ_STATUS          (4)
#define CS40L25_EVENT_SM_STATE_CLEAR_IRQ_FLAGS          (5)
#define CS40L25_EVENT_SM_STATE_DISABLE_BOOST            (6)
#define CS40L25_EVENT_SM_STATE_TOGGLE_ERR_RLS           (7)
#define CS40L25_EVENT_SM_STATE_ENABLE_BOOST             (8)
#define CS40L25_EVENT_SM_STATE_WRITE_WAKE               (9)
#define CS40L25_EVENT_SM_STATE_DONE                     (CS40L25_SM_STATE_DONE)
#define CS40L25_EVENT_SM_STATE_ERROR                    (CS40L25_SM_STATE_ERROR)
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
 * @defgroup CS40L25_CONTROL_ID_
 * @brief ID to indicate the type of Control Request
 *
 * @see cs40l25_functions_t member control
 * @see cs40l25_control_request_t member id
 *
 * @{
 */
#define CS40L25_CONTROL_ID_GET_HANDLER(A)               ((A & 0xF0000000) >> 28)
#define CS40L25_CONTROL_ID_GET_CONTROL(A)               (A & 0x0FFFFFFF)
#define CS40L25_CONTROL_ID_HANDLER_FA_GET               (0)
#define CS40L25_CONTROL_ID_HANDLER_FA_SET               (1)
#define CS40L25_CONTROL_ID_HANDLER_DSP_STATUS           (2)
#define CS40L25_CONTROL_ID_HANDLER_DYNAMIC_F0           (3)
#define CS40L25_CONTROL_ID_FA_GET_MASK                  (CS40L25_CONTROL_ID_HANDLER_FA_GET << 28)
#define CS40L25_CONTROL_ID_FA_SET_MASK                  (CS40L25_CONTROL_ID_HANDLER_FA_SET << 28)
#define CS40L25_CONTROL_ID_DSP_STATUS_MASK              (CS40L25_CONTROL_ID_HANDLER_DSP_STATUS << 28)
#define CS40L25_CONTROL_ID_DYNAMIC_F0_MASK              (CS40L25_CONTROL_ID_HANDLER_DYNAMIC_F0 << 28)
#define CS40L25_CONTROL_ID_FA_GET(A)                      (A | CS40L25_CONTROL_ID_FA_GET_MASK)
#define CS40L25_CONTROL_ID_FA_SET(A)                      (A | CS40L25_CONTROL_ID_FA_SET_MASK)
#define CS40L25_CONTROL_ID_DSP_STATUS(A)                  (A | CS40L25_CONTROL_ID_DSP_STATUS_MASK)
#define CS40L25_CONTROL_ID_DYNAMIC_F0(A)                  (A | CS40L25_CONTROL_ID_DYNAMIC_F0_MASK)

#define CS40L25_CONTROL_ID_NONE                           (0)

#define CS40L25_CONTROL_ID_GET_DSP_STATUS               CS40L25_CONTROL_ID_DSP_STATUS(0)

#define CS40L25_CONTROL_ID_GET_DYNAMIC_F0               CS40L25_CONTROL_ID_DYNAMIC_F0(0)
#define CS40L25_CONTROL_ID_GET_DYNAMIC_REDC             CS40L25_CONTROL_ID_DYNAMIC_F0(1)
#define CS40L25_CONTROL_ID_ENABLE_DYNAMIC_F0            CS40L25_CONTROL_ID_DYNAMIC_F0(2)

#define CS40L25_CONTROL_ID_GET_VOLUME                   CS40L25_CONTROL_ID_FA_GET(0)
#define CS40L25_CONTROL_ID_SET_VOLUME                   CS40L25_CONTROL_ID_FA_SET(0)
#define CS40L25_CONTROL_ID_GET_BHM_HALO_HEARTBEAT       CS40L25_CONTROL_ID_FA_GET(1)
#define CS40L25_CONTROL_ID_GET_RAM_HALO_HEARTBEAT       CS40L25_CONTROL_ID_FA_GET(2)
#define CS40L25_CONTROL_ID_SET_BHM_BUZZ_TRIGGER         CS40L25_CONTROL_ID_FA_SET(3)
#define CS40L25_CONTROL_ID_SET_TRIGGER_INDEX            CS40L25_CONTROL_ID_FA_SET(4)
#define CS40L25_CONTROL_ID_SET_TRIGGER_MS               CS40L25_CONTROL_ID_FA_SET(5)
#define CS40L25_CONTROL_ID_SET_TIMEOUT_MS               CS40L25_CONTROL_ID_FA_SET(6)
#define CS40L25_CONTROL_ID_SET_GPIO_ENABLE              CS40L25_CONTROL_ID_FA_SET(7)
#define CS40L25_CONTROL_ID_SET_GPIO1_BUTTON_DETECT      CS40L25_CONTROL_ID_FA_SET(8)
#define CS40L25_CONTROL_ID_SET_GPIO2_BUTTON_DETECT      CS40L25_CONTROL_ID_FA_SET(9)
#define CS40L25_CONTROL_ID_SET_GPIO3_BUTTON_DETECT      CS40L25_CONTROL_ID_FA_SET(10)
#define CS40L25_CONTROL_ID_SET_GPIO4_BUTTON_DETECT      CS40L25_CONTROL_ID_FA_SET(11)
#define CS40L25_CONTROL_ID_SET_CLAB_ENABLED             CS40L25_CONTROL_ID_FA_SET(12)
#define CS40L25_CONTROL_ID_SET_GPI_GAIN_CONTROL         CS40L25_CONTROL_ID_FA_SET(13)
#define CS40L25_CONTROL_ID_SET_CTRL_PORT_GAIN_CONTROL   CS40L25_CONTROL_ID_FA_SET(14)
#define CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_PRESS CS40L25_CONTROL_ID_FA_SET(15)
#define CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_PRESS CS40L25_CONTROL_ID_FA_SET(16)
#define CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_PRESS CS40L25_CONTROL_ID_FA_SET(17)
#define CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_PRESS CS40L25_CONTROL_ID_FA_SET(18)
#define CS40L25_CONTROL_ID_SET_GPIO1_INDEX_BUTTON_RELEASE CS40L25_CONTROL_ID_FA_SET(19)
#define CS40L25_CONTROL_ID_SET_GPIO2_INDEX_BUTTON_RELEASE CS40L25_CONTROL_ID_FA_SET(20)
#define CS40L25_CONTROL_ID_SET_GPIO3_INDEX_BUTTON_RELEASE CS40L25_CONTROL_ID_FA_SET(21)
#define CS40L25_CONTROL_ID_SET_GPIO4_INDEX_BUTTON_RELEASE CS40L25_CONTROL_ID_FA_SET(22)
#define CS40L25_CONTROL_ID_GET_FW_REVISION              CS40L25_CONTROL_ID_FA_GET(23)
#define CS40L25_CONTROL_ID_FA_MAX                       (23)

#define CS40L25_CONTROL_ID_GET_HALO_HEARTBEAT           (24)
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
#define CS40L25_POWER_DOWN                              (1) // Standby
#define CS40L25_POWER_HIBERNATE                         (2)
#define CS40L25_POWER_WAKE                              (3)
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
#define CS40L25_EVENT_FLAG_DSP_ERROR                    (1 << 31)
#define CS40L25_EVENT_FLAG_AMP_SHORT                    (1 << 30)
#define CS40L25_EVENT_FLAG_OVERTEMP_ERROR               (1 << 29)
#define CS40L25_EVENT_FLAG_OVERTEMP_WARNING             (1 << 28)
#define CS40L25_EVENT_FLAG_BOOST_INDUCTOR_SHORT         (1 << 27)
#define CS40L25_EVENT_FLAG_BOOST_UNDERVOLTAGE           (1 << 26)
#define CS40L25_EVENT_FLAG_BOOST_OVERVOLTAGE            (1 << 25)
#define CS40L25_EVENT_FLAG_STATE_ERROR                  (1 << 13)
#define CS40L25_EVENT_FLAG_READY_FOR_DATA               (1 << 12)
#define CS40L25_EVENT_FLAG_RESUME_PLAYBACK              (1 << 11)
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

#define CS40L25_CONFIG_REGISTERS_TOTAL                  (14)    ///< Total registers modified during Configure SM

#define CS40L25_INPUT_SRC_DISABLE                       (0x00)  ///< Data Routing value to indicate 'disabled'

#ifdef INCLUDE_CAL
#define DSP_REG(X)                                      (driver->state == CS40L25_STATE_DSP_POWER_UP || driver->state == CS40L25_STATE_DSP_STANDBY ? CS40L25_ ## X : CS40L25_CAL_ ## X)
#else
#define DSP_REG(X)                                      (CS40L25_ ## X)
#endif // INCLUDE_CAL
#define CS40L25_DSP_STATUS_WORDS_TOTAL                  (2)     ///< Total registers to read for Get DSP Status control

#define CS40L25_WSEQ_MAX_ENTRIES                        (48)    ///< Maximum registers written on wakeup from hibernate
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
    uint32_t id;                     ///< Control ID
    void *arg;                      ///< Argument for Control Request (nature depends on type of request)
} cs40l25_control_request_t;

/**
 * Data structure to describe a field to read via the Field Access SM
 *
 * @see cs40l25_load_control
 */
typedef struct
{
    bool symbol;        ///< If True, field is in firmware and only the id is known
    uint32_t address;   ///< Control Port address of field to access
    uint32_t id;                    ///< Id of symbol in symbol table
    uint32_t value;     ///< Value to write/value read
    uint8_t size;       ///< Bitwise size of field to access in register
    uint8_t shift;      ///< Bitwise shift of field to access in register
    bool ack_ctrl;      ///< (True) Signal field is an acknowledge control
    uint32_t ack_reset;     ///< The value the field should reset to on ack (only valid for ack ctrls)
} cs40l25_field_accessor_t;



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
        cs40l25_event_control_t event_control;
    };
} cs40l25_config_registers_t;

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
 * @see cs40l25_functions_t member configure
 */
typedef struct
{
    cs40l25_bsp_config_t bsp_config;
    const syscfg_reg_t *syscfg_regs;
    uint32_t syscfg_regs_total;
    cs40l25_calibration_t cal_data;                     ///< Calibration data from previous calibration sequence
    cs40l25_dsp_config_controls_t dsp_config_ctrls;     ///< Configurable DSP controls
    cs40l25_event_control_t event_control;              ///< Event Control configuration
} cs40l25_config_t;

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

    // Control Request handling fields
    cs40l25_control_request_t current_request;                                  ///< Current Control Request

    // Extra state material used by Control and Event Handler SMs
    uint32_t devid;                             ///< CS40L25 DEVID of current device
    uint32_t revid;                             ///< CS40L25 REVID of current device
    cs40l25_config_registers_t config_regs;     ///< Contents of Control Port registers to configure
    cs40l25_wseq_entry_t wseq_table[CS40L25_WSEQ_MAX_ENTRIES];  ///< List of register address/value pairs to write on wake up from hibernate
    uint8_t wseq_num_entries;                   ///< Number of entries currently in wseq_table
    bool wseq_initialized;                      ///< Flag indicating if the wseq_table has been initialized
    uint32_t calib_pcm_vol;
    cs40l25_field_accessor_t field_accessor;    ///< Current Control Port field to access

    // Driver configuration fields - see cs40l25_config_t
    cs40l25_config_t config;

    fw_img_v1_info_t *fw_info;     ///< Current HALO FW/Coefficient boot configuration

    uint32_t event_flags;
    uint32_t event_counter;
} cs40l25_t;

typedef struct
{
    union
    {
        uint32_t word;
        struct
        {
            uint32_t f0                         : 13; ///< F0 in Q10.3 format
            uint32_t index                      : 10; ///< Index in Wave Table
            uint32_t reserved                   : 9;
        };
    };
} cs40l25_dynamic_f0_table_entry_t;

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
 * - CS40L25_STATUS_FAIL        if call to f_queue_if_t fails
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
uint32_t cs40l25_process(cs40l25_t *driver);

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
uint32_t cs40l25_control(cs40l25_t *driver, cs40l25_control_request_t req);

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
 *
 * @return
 * - CS40L25_STATUS_FAIL        if any submission of Control Request failed
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_reset(cs40l25_t *driver);

/**
 * Write a fw block to the DSP's memory.
 *
 * This function writes a data block to the firmware's memory.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] addr             The address to which the data should be written
 * @param [in] data             The data to be written out to the bus
 * @param [in] size             The size of the data block pointed to by 'data'
 *
 * @return
 * - CS40L25_STATUS_FAIL        if any submission of Control Request failed
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_write_block(cs40l25_t *driver, uint32_t addr, uint8_t *data, uint32_t size);

/**
 * Boot the CS40L25
 *
 * This function informs the driver that a firmware has been loaded, and is described by the
 * passed in fw_img_v1_info_t struct.
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] fw_info          Pointer to the current fw's info
 *
 * @return
 * - CS40L25_STATUS_FAIL        if any submission of Control Request failed
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_boot(cs40l25_t *driver, fw_img_v1_info_t *fw_info);

/**
 * Change the power state
 *
 * This submits a Control Request to either Power Up or Power Down the CS40L25.  Although there is no checking here
 * for whether the request is invalid (i.e. Power Down when the driver state is already STANDBY), this checking will
 * be performed once the request is loaded in cs40l25_process().
 *
 * @param [in] driver           Pointer to the driver state
 * @param [in] power_state      New power state
 *
 * @return
 * - CS40L25_STATUS_FAIL        if submission of Control Request failed
 * - CS40L25_STATUS_OK          otherwise
 *
 */
uint32_t cs40l25_power(cs40l25_t *driver, uint32_t power_state);

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
 * Performas all register/memory field address updates required to put the HALO DSP
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
 * Performas all register/memory field address updates required to pull the HALO DSP
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
