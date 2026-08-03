/*
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
#ifndef CS40L5X_BSP_H
#define CS40L5X_BSP_H

#define DT_DRV_COMPAT cirrus_cs40l5x_alt_os

#include "cs40l5x.h"
#include <stdbool.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_HAPTICS_CS40L51
#include "cs40l51_firmware.h"
#define cs40l5x_total_fw_blocks cs40l51_total_fw_blocks
#define cs40l5x_fw_blocks       cs40l51_fw_blocks

#define cs40l5x_total_coeff_blocks_0 cs40l51_total_coeff_blocks_0
#define cs40l5x_total_coeff_blocks_1 cs40l51_total_coeff_blocks_1
#define cs40l5x_coeff_0_blocks       cs40l51_coeff_0_blocks
#define cs40l5x_coeff_1_blocks       cs40l51_coeff_1_blocks
#endif

#ifdef CONFIG_HAPTICS_CS40L52
#include "cs40l52_firmware.h"
#define cs40l5x_total_fw_blocks cs40l52_total_fw_blocks
#define cs40l5x_fw_blocks       cs40l52_fw_blocks

#define cs40l5x_total_coeff_blocks_0 cs40l52_total_coeff_blocks_0
#define cs40l5x_total_coeff_blocks_1 cs40l52_total_coeff_blocks_1
#define cs40l5x_coeff_0_blocks       cs40l52_coeff_0_blocks
#define cs40l5x_coeff_1_blocks       cs40l52_coeff_1_blocks
#endif

#ifdef CONFIG_HAPTICS_CS40L53
#include "cs40l53_firmware.h"
#define cs40l5x_total_fw_blocks cs40l53_total_fw_blocks
#define cs40l5x_fw_blocks       cs40l53_fw_blocks

#define cs40l5x_total_coeff_blocks_0 cs40l53_total_coeff_blocks_0
#define cs40l5x_total_coeff_blocks_1 cs40l53_total_coeff_blocks_1
#define cs40l5x_coeff_0_blocks       cs40l53_coeff_0_blocks
#define cs40l5x_coeff_1_blocks       cs40l53_coeff_1_blocks
#endif

#define regmap_cp_config_t     struct i2c_dt_spec
#define REGMAP_GET_CP(x)       x->config.bsp_config.i2c
#define regmap_read            cs40l5x_i2c_read_reg_dt
#define regmap_write           cs40l5x_i2c_write_reg_dt
#define regmap_update_reg      cs40l5x_update_reg_dt
#define regmap_write_array     cs40l5x_write_array_dt
#define regmap_poll_reg        cs40l5x_poll_reg_dt
#define regmap_write_acked_reg cs40l5x_write_acked_reg_dt
#define regmap_write_blocks    cs40l5x_i2c_write_bulk_dt

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @brief Host Initiated Haptics Effects metadata table offset
 *
 */
#define HIH_EFFECT_WT_OFFSET 5 //9 HIH effects should be in order, with this offset from index 0 in the wavetable


/**
 * @defgroup BSP_STATUS_
 * @brief Return values for all public and most private API calls
 *
 * @{
 */
#define BSP_STATUS_OK   (0)
#define BSP_STATUS_FAIL (1)
/** @} */

/**
 * @defgroup BSP_TIMER_DURATION_
 * @brief Values used for calls to BSP timer APIs
 *
 * @see bsp_driver_if_t.set_timer
 *
 * @{
 */
#define BSP_TIMER_DURATION_1MS  (1)
#define BSP_TIMER_DURATION_2MS  (2)
#define BSP_TIMER_DURATION_5MS  (5)
#define BSP_TIMER_DURATION_10MS (10)
#define BSP_TIMER_DURATION_2S   (2000)
/** @} */

/**
 * Value to indicate driving a GPIO low
 *
 * @see bsp_driver_if_t.set_gpio
 *
 */
#define BSP_GPIO_LOW      (0)
#define BSP_GPIO_INACTIVE (0)

/**
 * Value to indicate driving a GPIO high
 *
 * @see bsp_driver_if_t.set_gpio
 *
 */
#define BSP_GPIO_HIGH   (1)
#define BSP_GPIO_ACTIVE (1)

/**
 * Value to indicate enabling or disabling a supply
 *
 * @see bsp_driver_if_t.set_supply
 *
 */
#define BSP_SUPPLY_DISABLE (0)
#define BSP_SUPPLY_ENABLE  (1)

/**
 */
#define MAX_HIH_EFFECT_NAME_SIZE (20)

/**
 * Example ROM wavetable waveform index
 */
#define CS40L5X_HAPTIC_ROM_CLICK_1_VCM (0x0A)

/**
 * Array of valid effect names for Host Initiated Haptics
 */
extern const char *HIH_effect_names[];

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/**
 * Macro to extract byte from multi-byte word
 *
 * @param [in] A                multi-byte word
 * @param [in] B                zero-indexed byte position
 *
 * @return                      byte at position B in word A
 */
#define GET_BYTE_FROM_WORD(A, B) ((A >> (B * 8)) & 0xFF)

/**
 * Macro to insert byte into multi-byte word
 *
 * @param [in, out] A           multi-byte word
 * @param [in] B                byte value
 * @param [in] C                zero-indexed byte position
 *
 * @return none
 */
#define ADD_BYTE_TO_WORD(A, B, C)                                                                  \
    {                                                                                          \
        A &= (0xFFFFFF00 << (C * 8));                                                      \
        A |= ((B & 0xFF) << (C * 8));                                                      \
    }

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * Callback type for BSP-to-Driver callbacks
 *
 * @param [in] status           Result of BSP call
 * @param [in] arg              Argument registered when BSP call was issued
 *
 * @return none
 *
 * @see BSP_STATUS_
 *
 */
typedef void (*bsp_callback_t)(uint32_t status, void *arg);

/**
 * BSP-to-Driver public API
 *
 * All API calls return a status @see CS40L5X_STATUS_
 *
 */
typedef struct {
    /**
     * Set GPIO to LOW/HIGH
     *
     * @param [in] gpio_id      ID for GPIO to change - can be defined in implementation header
     * @param [in] gpio_state   LOW or HIGH
     *
     * @return
     * - BSP_STATUS_FAIL        if gpio_id is invalid, or call to MCU HAL fails
     * - BSP_STATUS_OK          otherwise
     *
     * @see BSP_GPIO_LOW BSP_GPIO_HIGH
     *
     */
    uint32_t (*set_gpio)(const struct gpio_dt_spec *gpio_id, uint8_t gpio_state);

    /**
     * Enable or disable a supply
     *
     * @param [in] supply_id      ID for supply to change - can be defined in implementation
     * header
     * @param [in] supply_state   Enable or Disable
     *
     * @return
     * - BSP_STATUS_FAIL        if gpio_id is invalid, or call to MCU HAL fails
     * - BSP_STATUS_OK          otherwise
     *
     * @note this function shouldn't return until the supply has finished rising or falling
     *
     * @see BSP_SUPPLY_ENABLE BSP_SUPPLY_DISABLE
     *
     */
    uint32_t (*set_supply)(uint32_t supply_id, uint8_t supply_state);

    /**
     * Register GPIO Callback
     *
     * Register a callback for when a GPIO changes state.
     *
     * @param [in] gpio_id      ID for GPIO to change - can be defined in implementation header
     * @param [in] cb           pointer to callback function
     * @param [in] cb_arg       pointer to argument to use when calling callback
     *
     * @return
     * - BSP_STATUS_FAIL        if gpio_id is invalid, if any pointers are NULL
     * - BSP_STATUS_OK          otherwise
     *
     */
    uint32_t (*register_gpio_cb)(const struct gpio_dt_spec *gpio_id, bsp_callback_t cb,
                     void *cb_arg);

    /**
     * Set a timer to expire
     *
     * @param [in] duration_ms  Duration of timer in milliseconds
     * @param [in] cb           pointer to callback function
     * @param [in] cb_arg       pointer to argument to use when calling callback
     *
     * @return
     * - BSP_STATUS_FAIL        if duration_ms is invalid, if any pointers are NULL
     * - BSP_STATUS_OK          otherwise
     *
     */
    uint32_t (*set_timer)(uint32_t duration_ms, bsp_callback_t cb, void *cb_arg);

    /**
     * Reset I2C Port used for a specific device
     *
     * Abort the current I2C transaction and reset the I2C peripheral.  This is required for
     * quickly handling of CS40L5X IRQ events.
     *
     * @param [in] bsp_dev_id       ID of the I2C device corresponding to the I2C peripheral to
     * reset
     * @param [out] was_i2c_busy    flag to indicate whether an I2C transaction was in progress
     * when reset
     *
     * @return
     * - BSP_STATUS_FAIL        if bsp_dev_id is invalid
     * - BSP_STATUS_OK          otherwise
     *
     */
    uint32_t (*i2c_reset)(uint32_t bsp_dev_id, bool *was_i2c_busy);

    /**
     * Perform an I2C Write-Repeated Start-Read transaction
     *
     * This is the common way to read data from an I2C device with a register file, since the
     * address of the register to read must first be written to the device before reading any
     * contents.
     *
     * Perform transaction in the order:
     * 1. I2C Start
     * 2. I2C write of \b write_length bytes from \b write_buffer
     * 3. I2C Repeated Start
     * 4. I2C read of \b read_length bytes into \b read_buffer
     * 5. I2C Stop
     *
     * BSP will decode \b bsp_dev_id to the correct I2C bus and I2C address.
     *
     * @param [in] bsp_dev_id       ID of the I2C device corresponding to the I2C peripheral to
     * reset
     * @param [in] write_buffer     pointer to array of bytes to write
     * @param [in] write_length     total number of bytes in \b write_buffer
     * @param [in] read_buffer      pointer to array of bytes to load with I2C bytes read
     * @param [in] read_length      total number of bytes to read into \b read_buffer
     * @param [in] cb               pointer to callback function
     * @param [in] cb_arg           pointer to argument to use when calling callback
     *
     * @return
     * - BSP_STATUS_FAIL            if bsp_dev_id is invalid, if any portion of I2C transaction
     * failed
     * - BSP_STATUS_OK              otherwise
     *
     */
    uint32_t (*i2c_read_repeated_start)(uint32_t bsp_dev_id, uint8_t *write_buffer,
                        uint32_t write_length, uint8_t *read_buffer,
                        uint32_t read_length, bsp_callback_t cb, void *cb_arg);

    /**
     * Perform I2C Write
     *
     * BSP will decode \b bsp_dev_id to the correct I2C bus and I2C address.
     *
     * @param [in] bsp_dev_id       ID of the I2C device corresponding to the I2C peripheral to
     * reset
     * @param [in] write_buffer     pointer to array of bytes to write
     * @param [in] write_length     total number of bytes in \b write_buffer
     * @param [in] cb               pointer to callback function
     * @param [in] cb_arg           pointer to argument to use when calling callback
     *
     * @return
     * - BSP_STATUS_FAIL            if bsp_dev_id is invalid, if any portion of I2C transaction
     * failed
     * - BSP_STATUS_OK              otherwise
     *
     */
    uint32_t (*i2c_write)(uint32_t bsp_dev_id, uint8_t *write_buffer, uint32_t write_length,
                  bsp_callback_t cb, void *cb_arg);

    /**
     * Perform a Double-Buffered ("db") I2C Write
     *
     * This will first write the contents of \b write_buffer_0 to the I2C device, and then write
     * the contents of
     * \b write_buffer_1.
     *
     * @param [in] bsp_dev_id       ID of the I2C device corresponding to the I2C peripheral to
     * reset
     * @param [in] write_buffer_0   pointer to array of first batch of bytes to write
     * @param [in] write_length_0   total number of bytes in \b write_buffer_0
     * @param [in] write_buffer_1   pointer to array of second batch of bytes to write
     * @param [in] write_length_1   total number of bytes in \b write_buffer_1
     * @param [in] cb               pointer to callback function
     * @param [in] cb_arg           pointer to argument to use when calling callback
     *
     * @return
     * - BSP_STATUS_FAIL            if bsp_dev_id is invalid, if any portion of I2C transaction
     * failed
     * - BSP_STATUS_OK              otherwise
     *
     */
    uint32_t (*i2c_db_write)(uint32_t bsp_dev_id, uint8_t *write_buffer_0,
                 uint32_t write_length_0, uint8_t *write_buffer_1,
                 uint32_t write_length_1, bsp_callback_t cb, void *cb_arg);

    /**
     * Perform a SPI read
     *
     * This function will write and then read back data from a SPI device with a register file.
     * Padding will automatically be added.
     *
     * Perform transaction in the order:
     * 1. SPI CS low
     * 2. SPI write of \b addr_length bytes from \b addr_buffer
     * 3. SPI write of pad_len padding clock cycles
     * 4. SPI read of \b data_length bytes into \b data_buffer
     * 5. SPI CS high
     *
     * BSP will decode \b bsp_dev_id to the correct SPI bus and SPI address.
     *
     * @param [in] bsp_dev_id       ID of the SPI device corresponding to the SPI peripheral to
     * reset
     * @param [in] addr_buffer      pointer to array of bytes to write
     * @param [in] addr_length      total number of bytes in \b write_buffer
     * @param [in] data_buffer      pointer to array of bytes to load with SPI bytes read
     * @param [in] data_length      total number of bytes to read into \b read_buffer
     * @param [in] pad_len          total number of bytes of padding between the addr write
     * transaction and the data read
     *
     * @return
     * - BSP_STATUS_FAIL            if bsp_dev_id is invalid, if any portion of SPI transaction
     * failed
     * - BSP_STATUS_OK              otherwise
     *
     */
    uint32_t (*spi_read)(uint32_t bsp_dev_id, uint8_t *addr_buffer, uint32_t addr_length,
                 uint8_t *data_buffer, uint32_t data_length, uint32_t pad_len);

    /**
     * Perform a SPI write
     *
     * This function will write data to a SPI device with a register file. Padding will
     * automatically be added.
     *
     * Perform transaction in the order:
     * 1. SPI CS low
     * 2. SPI write of \b addr_length bytes from \b addr_buffer
     * 3. SPI write of pad_len padding clock cycles
     * 4. SPI write of \b data_length bytes into \b data_buffer
     * 5. SPI CS high
     *
     * BSP will decode \b bsp_dev_id to the correct SPI bus and SPI address.
     *
     * @param [in] bsp_dev_id       ID of the SPI device corresponding to the SPI peripheral to
     * reset
     * @param [in] addr_buffer      pointer to array of bytes to write
     * @param [in] addr_length      total number of bytes in \b write_buffer
     * @param [in] data_buffer      pointer to array of bytes to load with SPI bytes read
     * @param [in] data_length      total number of bytes to read into \b read_buffer
     * @param [in] pad_len          total number of bytes of padding between the addr write
     * transaction and the data write
     *
     * @return
     * - BSP_STATUS_FAIL            if bsp_dev_id is invalid, if any portion of SPI transaction
     * failed
     * - BSP_STATUS_OK              otherwise
     *
     */
    uint32_t (*spi_write)(uint32_t bsp_dev_id, uint8_t *addr_buffer, uint32_t addr_length,
                  uint8_t *data_buffer, uint32_t data_length, uint32_t pad_len);

    /**
     * Global enable of interrupts
     *
     * Since this is MCU-platform specific, it is included as part of the BSP-Driver interface.
     *
     * @return
     * - BSP_STATUS_FAIL            if bsp_dev_id is invalid, if any portion of I2C transaction
     * failed
     * - BSP_STATUS_OK              otherwise
     *
     */
    uint32_t (*enable_irq)(void);

    /**
     * Global disable of interrupts
     *
     * Since this is MCU-platform specific, it is included as part of the BSP-Driver interface.
     *
     * @return
     * - BSP_STATUS_FAIL            if bsp_dev_id is invalid, if any portion of I2C transaction
     * failed
     * - BSP_STATUS_OK              otherwise
     *
     */
    uint32_t (*disable_irq)(void);

    /**
     * Temporarily change the clock speed of the SPI bus
     *
     * Since portions of a driver may have a maximum bus speed limitation, this API allows for
     * temporarily specifying the maximum bus speed.
     *
     * @return
     * - BSP_STATUS_FAIL            if a slower speed is requested by the current SPI speed is
     * already slowest available
     * - BSP_STATUS_OK              otherwise
     *
     */
    uint32_t (*spi_throttle_speed)(uint32_t speed_hz);

    /**
     * Restore the clock speed of the SPI bus to the original configuration
     *
     * After a call to spi_throttle_speed(), this API allows for restoring the bus clock speed
     * of the SPI bus to the original configuration given during BSP initialization.
     *
     * @return                      BSP_STATUS_OK always
     *
     */
    uint32_t (*spi_restore_speed)(void);
} bsp_driver_if_t;

struct cs40l5x_haptic_source_config {
    int index;
    int bank;
};

struct cs40l5x_bsp {
    cs40l5x_t priv;
    struct cs40l5x_haptic_source_config hap_cfg;
};

struct cs40l5x_owt_section_params {
    uint8_t nested_repeats;
    uint8_t waveform_idx;
    uint8_t amplitude;
    uint16_t delay;
    uint8_t owt_subwave;
    uint8_t rom_subwave;
    uint8_t duration_present;
    uint32_t duration;
};

typedef enum {
    EFFECT_OWT_IDX,
    EFFECT_MSFT_ID,
    EFFECT_NAME
} effectLabel;

//HIH effect data structure to contain info about effect position in wavetable
typedef struct {
    uint8_t wt_idx;
    uint32_t msft_ID;
    char effectName[MAX_HIH_EFFECT_NAME_SIZE];
    uint32_t length_ms; //effect length in ms in q30.2 format
} HIH_effect_metadata;



//HIH effect data structure to pass into HIH trigger BSP function
typedef struct {
    effectLabel label;
    union {
        uint16_t owtIdx;
        uint32_t msft_ID;
        char effectName[MAX_HIH_EFFECT_NAME_SIZE];
    } HIH_effect_identifier;
} HIH_effect;

extern const struct gpio_dt_spec reset;
extern bsp_driver_if_t *bsp_driver_if_g;

/**
 * Writes the contents of a single register/memory address via i2c
 *
 * @param [in] spec                 Pointer to the i2c struct reference
 * @param [in] reg_addr             32-bit address to be written
 * @param [in] value                32-bit value to be written
 *
 * @return
 * - -EIO                           on failure
 * - BSP_STATUS_OK                  otherwise
 *
 */
int cs40l5x_i2c_write_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                 const uint32_t value);

/**
 * Reads the contents of a single register/memory address via i2c
 *
 * @param [in] spec                 Pointer to the i2c struct reference
 * @param [in] reg_addr             32-bit address to be read
 * @param [out] value               Pointer to variable to read data into
 *
 * @return
 * - -EIO                           on failure
 * - BSP_STATUS_OK                  otherwise
 *
 */
int cs40l5x_i2c_read_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                uint32_t *value);

/**
 * Read-Modify-Write of register using 32-bit mask via i2c
 *
 * @param [in] spec                 Pointer to the i2c struct reference
 * @param [in] reg_addr             32-bit address to be read
 * @param [in] mask                 32-bit mask for bits to be modified
 * @param [in] value                32-bit value to be written
 *
 * @return
 * - BSP_STATUS_FAIL                on read failure
 * - BSP_STATUS_OK                  otherwise
 */
int cs40l5x_update_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
              const uint32_t mask, const uint32_t value);

/**
 * Writes an array of addr/word pairs to its corresponding register/memory addresses via i2c
 *
 * @param [in] spec                 Pointer to the i2c struct reference
 * @param [in] array                Array of address/word pairs with the addr at array[i] and the
 * word at array[i+1]
 * @param [in] words                Size of array to be written (number of words * 2)
 *
 * @return
 * - BSP_STATUS_FAIL                on write failure
 * - BSP_STATUS_OK                  otherwise
 */
int cs40l5x_write_array_dt(const struct i2c_dt_spec *spec, const uint32_t *array, uint32_t words);

/**
 * Reads a register via i2c for a specific value for a set number of tries while waiting
 * between reads.
 *
 * @param [in] spec                 Pointer to the i2c struct reference
 * @param [in] reg_addr             Address to read from.
 * @param [in] value                Value to compare the read value to.
 * @param [in] tries                How many times to read the address.
 * @param [in] delay                How long to delay between each read.
 *
 * @return
 * - BSP_STATUS_FAIL                if the i2c read failed or if value not polled
 *                                  within the amount of tries
 * - BSP_STATUS_OK                  otherwise
 */
int cs40l5x_poll_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr, uint32_t value,
            uint32_t tries, uint32_t delay);

/**
 * Write a value to a register via i2c and poll for an updated value
 *
 * @param [in] spec                 Pointer to the i2c struct reference
 * @param [in] reg_addr             Address to read from.
 * @param [in] val                  32-bit value to be written
 * @param [in] acked_val            Value to poll for after writing 'val'
 * @param [in] tries                How many times to read the address.
 * @param [in] delay                How long to delay between each read (ms)
 *
 * @return
 * - BSP_STATUS_FAIL                if the i2c write failed or if value not polled
 *                                  within the amount of tries
 * - BSP_STATUS_OK                  otherwise
 */
int cs40l5x_write_acked_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
                   uint32_t val, uint32_t acked_val, uint8_t tries, uint32_t delay);

/**
 * Update the driver's current haptic configuration for haptic triggers
 *
 * @param [in] dev                  Pointer to the driver state
 * @param [in] hap_cfg              Haptic source data structure containing
 * effect bank and index
 *
 * @return
 * - BSP_STATUS_FAIL if dev or hap_cfg invalid
 * - BSP_STATUS_OK                  otherwise
 */
int cs40l5x_set_haptic_cfg(const struct device *dev, struct cs40l5x_haptic_source_config *hap_cfg);

/**
 * Write initial header for an OWT composite to next available OWT index
 *
 * @param [in] dev                      Pointer to the driver state
 * @param [in] num_waveforms            Total number of individual effects making up
 * the OWT composite waveform
 * @param [in] repeats                  Number of repeats of the overall composite
 * waveform
 *
 * @return
 * - BSP_STATUS_FAIL        If failure to write OWT header with
 * cs40l5x_write_owt_composite_header()
 * - BSP_STATUS_OK          otherwise
 */
int bsp_cs40l5x_write_owt_header(const struct device *dev, uint8_t num_waveforms, uint8_t repeats);

/**
 * Write a single section for an OWT composite waveform following an OWT header
 *
 * This function should be called N times following the call to bsp_cs40l5x_write_owt_header
 * based on the "num_waveforms" parameter passed into the OWT header.
 *
 * @param [in] dev                      Pointer to the driver state
 * @param [in] section                  Data structure containing OWT section
 * parameters
 *
 * @return
 * - BSP_STATUS_FAIL        If failure to write OWT section with
 * cs40l5x_write_owt_composite_section()
 * - BSP_STATUS_OK          otherwise
 */
int bsp_cs40l5x_write_owt_section(const struct device *dev,
                  struct cs40l5x_owt_section_params section);

/**
 * Push the most recently written OWT effect to the OWT based on the number of sections
 * written and the next available OWT index
 *
 * @param [in] dev              Pointer to the driver state
 *
 * @return
 * - BSP_STATUS_FAIL        If failure to call cs40l5x_push_owt_composite
 * - BSP_STATUS_OK          otherwise
 */
int bsp_cs40l5x_push_owt(const struct device *dev);

/**
 * Function to minimize i2c transactions while adding a new composite waveform to the OWT
 * specifically with only one composite section
 *
 * @param [in] dev              Pointer to the driver state
 * @param [in] section          Data structure containing OWT effect parameters
 *
 * @return
 * - BSP_STATUS_FAIL if failure to write new OWT waveform
 * - BSP_STATUS_OK                  otherwise
 */
int bsp_cs40l5x_write_owt_composite_one_section(const struct device *dev,
                        struct cs40l5x_owt_section_params section);

/**
 * Send an OWT trigger mailbox command to trigger an effect at a specific OWT index
 *
 * @param [in] dev              Pointer to the driver state
 * @param [in] owt_idx          Effect index in OWT to trigger
 *
 * @return
 * - BSP_STATUS_FAIL        If failure to write OWT trigger mailbox command
 * - BSP_STATUS_OK          otherwise
 */
int bsp_cs40l5x_trigger_owt(const struct device *dev, int owt_idx);

/**
 * Send an OWT delete mailbox command to delete the effect at a specific OWT index
 *
 * @param [in] dev              Pointer to the driver state
 * @param [in] owt_idx          Effect index in OWT to delete
 *
 * @return
 * - BSP_STATUS_FAIL        If failure to write OWT delete mailbox command
 * - BSP_STATUS_OK          otherwise
 */
int bsp_cs40l5x_delete_owt(const struct device *dev, int owt_idx);

/**
 * Read back the number of waveforms currently in the OWT
 *
 * @param [in] dev              Pointer to the driver state
 * @param [out] num             Returned number of effects in OWT
 *
 * @return
 * - BSP_STATUS_FAIL        If failure to readback number of OWT waveforms
 * - BSP_STATUS_OK          otherwise
 */
int bsp_cs40l5x_get_num_owt_wf(const struct device *dev, uint32_t *num);

/**
 * Calculates and returns the length of the SVC pilot tone in us
 *
 * This function will calculate the total length of the SVC pilot tone by reading back
 * tone section data from SVC_INIT_PH_PILOT_HI_START_SMP, SVC_INIT_PH_OFST_CAL_START_SMP,
 * SVC_INIT_PH_OFST_CAL_NSMP, SVC_INIT_PH_OFST_STL_NSMP, and converting this to microseconds
 * given the current sample rate.
 *
 * @param [in] dev           Pointer to the driver state
 * @param [in, out] length   Pointer to uint32_t length value to be returned in us
 *
 * @return
 * - BSP_STATUS_OK if:
 *      - SVC length successfully read back and calculated in us
 * - otherwise, returns BSP_STATUS_FAIL
 *
 */
int bsp_cs40l5x_get_SVC_tone_length(const struct device *dev, uint32_t *length);

/**
 * Adds a new host initiated waveform to the OWT and triggers it.
 *
 * This function deletes a prior existing OWT effect, constructs a new composite effect based
 * on input parameters, adds this new composite effect to the OWT, then triggers this new effect.
 *
 * @param [in] dev                  Pointer to the driver state
 * @param [in] effect               HIH_effect structure that allows for effect to be referenced by
 * effect name, microsoft ID, or OWT table index
 * @param [in] intensity            New effect amplitude, from 1-200%
 * @param [in] repeats              Number of repeats of selected waveform, from 0-255
 * @param [in] retrigger_period     Delay between waveform repeats, from 0-10000 ms
 * @param [in] cutoff_time          Maximimum playback time of waveform in ms
 *
 * @return
 * - CS40L5X_STATUS_FAIL if:
 *      - Deletion of last added OWT waveform fails
 *      - Function fails to find effect index in wavetable
 *      - Addition of new OWT waveform fails
 *      - Triggering new OWT waveform fails
 * - otherwise, returns CS40L5X_STATUS_OK
 *
 */
int bsp_cs40l5x_host_initiated_trigger(const struct device *dev, HIH_effect effect,
                       uint32_t intensity, uint32_t repeats,
                       uint32_t retrigger_period, uint32_t cutoff_time);

/**
 * Lists the available host initiated haptics effects and their durations
 *
 * This function list the effects with their duration, accounting for the additional
 * playback time added by SVC.
 *
 * @param [in] dev           Pointer to the driver state
 *
 * @return
 * - BSP_STATUS_FAIL if:
 *      - Issue reading back SVC length
 * - otherwise, returns BSP_STATUS_OK
 *
 */
int bsp_cs40l5x_list_host_initiated_effects(const struct device *dev);
int bsp_cs40l5x_dump_regs(const struct device *dev, uint32_t addr, uint32_t num_words);
int bsp_cs40l5x_get_pwle_length(const struct device *dev, uint32_t idx, uint32_t* len);
#endif

