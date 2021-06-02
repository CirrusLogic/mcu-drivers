/**
 * @file cs47l15_ext.c
 *
 * @brief The CS47L15 Driver Extended API module
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020-2021 All Rights Reserved, http://www.cirrus.com/
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
/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stddef.h>
#include "cs47l15_ext.h"
#include "bsp_driver_if.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS DECLARATIONS
 **********************************************************************************************************************/
static uint32_t cs47l15_get_dsp_element_value(cs47l15_t *driver, uint32_t rb_struct_base_addr, dsp_struct_offsets_t offset, uint32_t *value);
static uint32_t cs47l15_set_dsp_element_value(cs47l15_t *driver, uint32_t rb_struct_base_addr, dsp_struct_offsets_t offset, uint32_t value);
static void     cs47l15_read_array(const uint8_t *array, uint8_t *target, uint32_t *length);
static uint32_t cs47l15_init_dsp_ringbuf_structure(cs47l15_t *driver, uint32_t rb_struct_base_addr, ring_buffer_struct_t *dsp_buffer);

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
/**
 * Write data to dsp ring buffer
 *
 * If data has already started streaming, it should only be called after IRQ signal from DSP, and after determining
 * that there is space available in buffer
 *
 * @param [in]
 * - driver              Pointer to the driver state
 * - buffer              Pointer to dsp ringbuff structure
 * - data                Pointer to array of data bytes
 * - data_len            Number of bytes to write from data array. Should not be longer to avail space in dsp buffer
 *                       or longer than the allocated buffer.
 *
 * @return
 * - CS47L15_STATUS_FAIL         Control port activity fails
 * - CS47L15_STATUS_OK           otherwise
 *
 * @see cs47l15_init_dsp_buffer
 * @see cs47l15_dsp_buf_avail
 *
 */
uint32_t cs47l15_dsp_buf_write(cs47l15_t *driver,
                                dsp_buffer_t *buffer,
                                uint8_t * data,
                                uint32_t data_len)
{
    uint32_t dsp_avail_wrap;
    uint32_t dsp_buff_add;
    uint32_t ret;

    if ((data_len > buffer->dsp_buf.avail) ||
        (data_len > buffer->buf_size))
    {
        return CS47L15_STATUS_FAIL;
    }

    // read a portion of data with padding
    cs47l15_read_array(data, buffer->linear_buf, &data_len);

    // determine remaining space in buffer
    dsp_avail_wrap = ((buffer->dsp_buf.buffer_size + buffer->dsp_buf.buffer_size / 3) - (buffer->dsp_buf.next_write_index * 4));
    if (data_len >= dsp_avail_wrap)// if data to be written exceeds buffer size, write up to available space
    {
        dsp_buff_add =(buffer->dsp_buf.buffer_base + (buffer->dsp_buf.next_write_index * CS47L15_DSP_OFFSET_MUL_VALUE));
        cs47l15_write_block(driver, dsp_buff_add, buffer->linear_buf, dsp_avail_wrap);
        data_len = data_len - dsp_avail_wrap;
        buffer->dsp_buf.next_write_index = 0;
    }
    else
    {
        dsp_avail_wrap = 0;
    }
    if (data_len > 0) // write normally, or write remaining data to start of buffer after filling the buffer
    {
        dsp_buff_add =(buffer->dsp_buf.buffer_base + (buffer->dsp_buf.next_write_index*CS47L15_DSP_OFFSET_MUL_VALUE));
        cs47l15_write_block(driver, dsp_buff_add, (buffer->linear_buf + dsp_avail_wrap), data_len);
        buffer->dsp_buf.next_write_index += (data_len / 4);
    }

    ret = cs47l15_set_dsp_element_value(driver, buffer->rb_struct_base_addr, next_write_index, buffer->dsp_buf.next_write_index);
    if (ret)
    {
        return ret;
    }

    ret = cs47l15_set_dsp_element_value(driver, buffer->rb_struct_base_addr, irq_ack, CS47L15_DSP_IRQ_ACK_VAL);
    if (ret)
    {
        return ret;
    }

    return CS47L15_STATUS_OK;
}

/**
 * Initialize struct with buffer needed to send data to dsp
 *
 * @param [in]
 * - driver           Pointer to the driver state
 * - buffer           Pointer to dsp ringbuff structure
 * - lin_buff_ptr     Pointer to allocated buffer
 * - buf_size         Size of allocated buffer
 * - buf_symbol       Symbol holding address of DSP buffer
 * - dsp_core         Which DSP core to use
 *
 * @return
 * - CS47L15_STATUS_FAIL         Control port activity fails
 * - CS47L15_STATUS_OK           otherwise
 *
 * @see cs47l15_find_symbol
 * @see cs47l15_init_dsp_ringbuf_structure
 *
 */
uint32_t cs47l15_dsp_buf_init(cs47l15_t *driver,
                                  dsp_buffer_t *buffer,
                                  uint8_t *lin_buf_ptr,
                                  uint32_t buf_size,
                                  uint32_t buf_symbol,
                                  uint32_t dsp_core)
{
    uint32_t ret;
    uint32_t addr;
    uint32_t count = 0;
    uint32_t xmem_addr;
    ring_buffer_struct_t dsp_buf;

    switch (dsp_core)
    {
        case 1:
            xmem_addr = CS47L15_DSP1_XMEM_0;
            break;

        default:
            return CS47L15_STATUS_FAIL;
            break;
    }
    // Find ring buffer address
    if (buf_symbol == 0)
    {
        return CS47L15_STATUS_FAIL;
    }
    else
    {
        ret = cs47l15_read_reg(driver, buf_symbol, &addr);
        if (ret)
        {
            return ret;
        }
        if (!addr)
        {
            while (!addr & (count < 10))
            {
                bsp_driver_if_g->set_timer(5, NULL, NULL);
                ret = cs47l15_read_reg(driver, buf_symbol, &addr);
                if (ret)
                {
                    return ret;
                }
                count++;
            }
            if (count==10)
            {
                return CS47L15_STATUS_FAIL;
            }
        }

        buffer->rb_struct_base_addr = (addr * 2) + xmem_addr;
    }

    ret = cs47l15_init_dsp_ringbuf_structure(driver, buffer->rb_struct_base_addr, &dsp_buf);
    buffer->dsp_buf = dsp_buf;
    if (ret)
    {
      return ret;
    }

    buffer->buf_size = buf_size - (buf_size / 4);

    buffer->linear_buf = lin_buf_ptr;

    return CS47L15_STATUS_OK;
}

/**
 * Initialize struct with buffer needed to send data to dsp
 *
 * @param [in]
 * - driver           Pointer to the driver state
 * - buffer           Pointer to dsp ringbuff structure
 * - space_avail      Pointer to how much data is available in DSP buffer
 *
 * @return
 * - CS47L15_STATUS_FAIL         Control port activity fails
 * - CS47L15_STATUS_OK          otherwise
 *
 */
uint32_t cs47l15_dsp_buf_avail(cs47l15_t *driver,
                                dsp_buffer_t *buffer,
                                uint32_t * space_avail)
{
    int32_t size;
    uint32_t ret;

    ret = cs47l15_get_dsp_element_value(driver, buffer->rb_struct_base_addr, next_read_index, &buffer->dsp_buf.next_read_index);
    if (ret)
    {
        return ret;
    }

    size = (buffer->dsp_buf.next_read_index - buffer->dsp_buf.next_write_index);
    size *= 3;

    if (size <= 0)
    {
        size = size + (buffer->dsp_buf.buffer_size);
    }
    if (size <= 3)
    {
        buffer->dsp_buf.avail =  0;
        *space_avail = buffer->dsp_buf.avail;
        return CS47L15_STATUS_OK;
    }

    // maintain minimum 1 3-byte word gap between writeindex and readindex while filling the buffer */
    buffer->dsp_buf.avail = (size - 3);
    *space_avail = buffer->dsp_buf.avail;
    return CS47L15_STATUS_OK;
}

/**
 * Send EOF signal to dsp
 *
 * @param [in]
 * - driver           Pointer to the driver state
 * - buffer           Pointer to dsp ringbuff structure
 *
 * @return
 * - CS47L15_STATUS_FAIL         Control port activity fails
 * - CS47L15_STATUS_OK          otherwise
 *
 */
uint32_t cs47l15_dsp_buf_eof(cs47l15_t *driver,
                             dsp_buffer_t *buffer)
{
    uint32_t ret;
    ret = cs47l15_set_dsp_element_value(driver, buffer->rb_struct_base_addr, end_of_stream, CS47L15_DSP_EOF_VAL);
    return ret;
}

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/

/**
 * Initialize each element of dsp ringbuff struct, and communicate values with DSP when needed
 *
 */

static uint32_t cs47l15_init_dsp_ringbuf_structure(cs47l15_t *driver,
                                  uint32_t rb_struct_base_addr,
                                  ring_buffer_struct_t *dsp_buffer)
{
    cs47l15_get_dsp_element_value(driver, rb_struct_base_addr, buffer_base, &(dsp_buffer->buffer_base));
    if (!dsp_buffer->buffer_base)
    {
        return CS47L15_STATUS_FAIL;
    }
    else
    {
        dsp_buffer->buffer_base = (dsp_buffer->buffer_base * 2) + CS47L15_DSP1_XMEM_0;
    }
    cs47l15_get_dsp_element_value(driver, rb_struct_base_addr, buffer_size, &(dsp_buffer->buffer_size));
    dsp_buffer->buffer_size *= 3; //convert to unpadded bytes
    dsp_buffer->avail = dsp_buffer->buffer_size - 3;
    cs47l15_get_dsp_element_value(driver, rb_struct_base_addr, irq_ack, &(dsp_buffer->irq_ack));
    dsp_buffer->next_write_index = 0;
    cs47l15_set_dsp_element_value(driver, rb_struct_base_addr, next_write_index, dsp_buffer->next_write_index);
    dsp_buffer->next_read_index = 0;
    cs47l15_set_dsp_element_value(driver, rb_struct_base_addr, next_read_index, dsp_buffer->next_read_index);
    cs47l15_get_dsp_element_value(driver, rb_struct_base_addr, dsp_error, &(dsp_buffer->error));
    cs47l15_set_dsp_element_value(driver, rb_struct_base_addr, end_of_stream, 0);

    return CS47L15_STATUS_OK;
}

/**
 * Read array into target buffer, and add padding for 24bit data on DSP (1 byte of 0s for every 3 bytes of data)
 *
 */

static void cs47l15_read_array(const uint8_t *array, uint8_t *target, uint32_t *length)
{
    uint32_t rem;
    uint32_t end_padding_len;
    uint32_t j = 0; // keep track of where to put data in target
    for (uint32_t i = 0; i < *length; i++)
    {
        if (i % 3 == 0) // count each 3 bytes to pad with 0s
        {
            target[j] = 0x00;
            j++;
        }
        target[j] = array[i];
        j++;
    }

    rem = j % 4;
    if (rem > 0)
    {
        end_padding_len = 4 - rem;
        for (uint32_t i = 0; i < end_padding_len; i++)
        {
            target[j] = 0x00;
            j++;
        }
    }

    *length = j;

    return;
}

/**
 * Read a value of an element of buffer struct from DSP
 *
 */
static uint32_t cs47l15_get_dsp_element_value(cs47l15_t *driver,
                             uint32_t rb_struct_base_addr,
                             dsp_struct_offsets_t offset,
                             uint32_t *value)
{
    uint32_t addr = (rb_struct_base_addr + offset * CS47L15_DSP_OFFSET_MUL_VALUE);
    uint32_t ret;

    ret = cs47l15_read_reg(driver, addr, value);
    *value = *value & 0xFFFFFF; // 24bit values on ADSP2
    if (ret)
    {
        return CS47L15_STATUS_FAIL;
    }
    else
    {
        return CS47L15_STATUS_OK;
    }
}

/**
 * Set a value of an element of buffer struct to DSP
 *
 */
static uint32_t cs47l15_set_dsp_element_value(cs47l15_t *driver,
                             uint32_t rb_struct_base_addr,
                             dsp_struct_offsets_t offset,
                             uint32_t value)
{
    uint32_t addr = (rb_struct_base_addr + offset * CS47L15_DSP_OFFSET_MUL_VALUE);
    uint32_t ret;

    value = value & 0x00FFFFFF; // 24bit values on ADSP2
    ret = cs47l15_write_reg(driver, addr, value);
    if (ret)
    {
        return CS47L15_STATUS_FAIL;
    }
    else
    {
        return CS47L15_STATUS_OK;
    }
}

