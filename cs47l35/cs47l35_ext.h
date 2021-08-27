/**
 * @file cs47l35_ext.h
 *
 * @brief Functions and prototypes exported by the CS47L35 Driver Extended API module
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

#ifndef CS47L35_EXT_H
#define CS47L35_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "cs47l35.h"

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/
 /**
 * @defgroup CS47L35_DSP_
 * @brief Values for communicating with DSP
 *
 * @{
 */
#define CS47L35_DSP_OFFSET_MUL_VALUE              2
#define CS47L35_DSP_IRQ_ACK_VAL                   0x1
#define CS47L35_DSP_EOF_VAL                       0x1
#define CS47L35_DSP_ENC_ALGORITHM_STOPPED         0xFF000000
#define CS47L35_DSP_DEC_ALGORITHM_STOPPED         0x00FF0000

/***********************************************************************************************************************
 * MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * ENUMS, STRUCTS, UNIONS, TYPEDEFS
 **********************************************************************************************************************/

/**
 * Data structure for tracking DSP's ring buffer
 *
 * @see cs47l35_init_dsp_buffers
 */
typedef struct ring_buffer_t
{
    uint32_t buffer_base;
    uint32_t buffer_size;
    uint32_t irq_ack;
    uint32_t next_write_index;
    uint32_t next_read_index;
    uint32_t error;
    uint32_t avail;
    uint32_t buf_size;
} ring_buffer_struct_t;

/**
 * Data structure to hold anything buffer-related
 *
 * @see cs47l35_init_dsp_buffers
 */
typedef struct buffers {
    uint32_t rb_struct_base_addr;
    ring_buffer_struct_t dsp_buf;
    uint8_t *linear_buf;
    uint32_t buf_size;
} dsp_buffer_t;

/**
 * Data structure to identify DSP buffer elements
 *
 * @see cs47l35_get_dsp_element_value
 * @see cs47l35_set_dsp_element_value
 * @see cs47l35_update_writeIndex_irqAck_eof
 * @see cs47l35_init_dsp_ringbuf_structure
 */
typedef enum
{
    buffer_base=0,
    buffer_size,
    irq_ack,
    next_write_index,
    next_read_index,
    dsp_error,
    end_of_stream,
    playback_time_ms_high,
    playback_time_ms_low,
    higher_water_mark,
    lower_water_mark,
}dsp_struct_offsets_t;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
 /**
 * Write data to dsp ring buffer
 *
 * If data has already started streaming, it should only be called after IRQ signal from DSP
 *
 * @param [in]
 * - driver              Pointer to the driver state
 * - buffer              Pointer to dsp ringbuff structure
 * - data                Pointer to array of data bytes
 * - data_len            Number of bytes to write from data array. Should not be longer to avail space in dsp buffer
 *                       or longer than the allocated buffer.
 *
 * @return
 * - bytes_written    Number of bytes written to dsp ring buffer
 *
 * @see cs47l35_init_dsp_buffers
 * @see cs47l35_dsp_buf_avail
 *
 */
uint32_t cs47l35_dsp_buf_write(cs47l35_t *driver, dsp_buffer_t *buffer, uint8_t * data, uint32_t data_len);

/**
 * Read data from dsp ring buffer
 *
 * If data has already started streaming, it should only be called after IRQ signal from DSP, and after determining
 * that there is data available in buffer
 *
 * @param [in]
 * - driver              Pointer to the driver state
 * - buffer              Pointer to dsp ringbuff structure
 * - data                Pointer to array of data bytes to store incoming data
 * - data_len            Number of bytes to read from data array. Should not be longer to avail data in dsp buffer
 *                       or longer than the allocated buffer.
 *
 * @return
 * - CS47L35_STATUS_FAIL         Control port activity fails
 * - CS47L35_STATUS_OK           otherwise
 *
 * @see cs47l35_init_dsp_buffer
 * @see cs47l35_dsp_buf_data_avail
 *
 */
uint32_t cs47l35_dsp_buf_read(cs47l35_t *driver, dsp_buffer_t *buffer, uint8_t * data, uint32_t data_len);

/**
 * Initialize struct with buffers needed to send data to dsp
 *
 * @param [in]
 * - driver           Pointer to the driver state
 * - buffers          Pointer to dsp ringbuff structure
 * - lin_buff_ptr     Pointer to allocated buffer
 * - buf_size         Size of allocated buffer
 * - buf_symbol       Address contining location of the buffer
 * - dsp_core         Which DSP core to use
 *
 * @return
 * - CS47L35_STATUS_FAIL         Control port activity fails
 * - CS47L35_STATUS_OK          otherwise
 *
 * @see cs47l35_find_symbol
 * @see cs47l35_init_dsp_ringbuf_structure
 *
 */
uint32_t cs47l35_dsp_buf_init(cs47l35_t *driver, dsp_buffer_t *buffer, uint8_t *lin_buff_ptr, uint32_t buf_size, uint32_t buf_symbol, uint32_t dsp_core);

/**
 * Check available space on DSP decoder
 *
 * @param [in]
 * - driver           Pointer to the driver state
 * - buffer           Pointer to dsp ringbuff structure
 * - space_avail      Pointer to how much data is available in DSP buffer
 *
 * @return
 * - CS47L35_STATUS_FAIL         Control port activity fails
 * - CS47L35_STATUS_OK          otherwise
 *
 */
uint32_t cs47l35_dsp_buf_space_avail(cs47l35_t *driver, dsp_buffer_t *buffer, uint32_t * space_avail);

/**
 * Check available data on DSP encoder
 *
 * @param [in]
 * - driver           Pointer to the driver state
 * - buffer           Pointer to dsp ringbuff structure
 * - space_avail      Pointer to how much data is available in DSP buffer
 *
 * @return
 * - CS47L35_STATUS_FAIL         Control port activity fails
 * - CS47L35_STATUS_OK          otherwise
 *
 */
uint32_t cs47l35_dsp_buf_data_avail(cs47l35_t *driver, dsp_buffer_t *buffer, uint32_t * data_avail);

/**
 * Send EOF signal to dsp
 *
 * @param [in]
 * - driver           Pointer to the driver state
 * - buffer           Pointer to dsp ringbuff structure
 *
 * @return
 * - CS47L35_STATUS_FAIL         Control port activity fails
 * - CS47L35_STATUS_OK          otherwise
 *
 */
uint32_t cs47l35_dsp_buf_eof(cs47l35_t *driver, dsp_buffer_t *buffer);

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS47L35_EXT_H
