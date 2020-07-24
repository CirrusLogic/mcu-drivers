/**
 * @file otp_unpacker.c
 *
 * @brief The OTP Unpacker module
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
/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "otp_unpacker.h"

/***********************************************************************************************************************
 * LOCAL LITERAL SUBSTITUTIONS, TYPEDEFS
 **********************************************************************************************************************/
#define OU_REGLIST_GET_TOTAL_WORDS(A)           (((A) * 2) + 8) // 2 - words per reglist entry, 8 - unlock/lock wordsize
#define OU_REGLIST_GET_ADDRESS(A)               (reg_list[(A * 2) + 4]) // 4 - unlock prefix
#define OU_REGLIST_GET_VALUE(A)                 (reg_list[(A * 2) + 1 + 4]) // 1 - value is 2nd word of each entry

/***********************************************************************************************************************
 * LOCAL VARIABLES
 **********************************************************************************************************************/
static const cs35l41_otp_map_t *otp_map_s = NULL;
static uint8_t *otp_buffer_s = NULL;
static uint32_t *reg_list = NULL;
static uint8_t reg_list_size = 0;

/***********************************************************************************************************************
 * GLOBAL VARIABLES
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 **********************************************************************************************************************/
static uint32_t otp_unpacker_make_reg_list(void)
{

    bool *otp_map_is_reg_unique = (bool *) malloc(sizeof(bool) * otp_map_s->num_elements);
    if (otp_map_is_reg_unique == NULL)
    {
        return OTP_UNPACKER_STATUS_FAIL;
    }

    uint8_t total_unique_addresses = otp_map_s->num_elements;
    for (uint8_t i = 0; i < otp_map_s->num_elements; i++)
    {
        uint32_t current_address = otp_map_s->map[i].reg;

        otp_map_is_reg_unique[i] = true;

        for (uint8_t j = 0; j < i; j++)
        {
            if ((otp_map_s->map[j].reg == current_address) || (current_address == 0))
            {
                otp_map_is_reg_unique[i] = false;
                total_unique_addresses--;
                break;
            }
        }
    }

    // Add 8 words for UNLOCK/LOCK prepeding/appending
    reg_list = (uint32_t *) malloc(sizeof(uint32_t) * OU_REGLIST_GET_TOTAL_WORDS(total_unique_addresses));

    if (reg_list == NULL)
    {
        return OTP_UNPACKER_STATUS_FAIL;
    }

    reg_list_size = 0;
    for (uint8_t i = 0; i < otp_map_s->num_elements; i++)
    {
        if (reg_list_size == total_unique_addresses)
        {
            free(reg_list);
            reg_list = NULL;
            return OTP_UNPACKER_STATUS_FAIL;
        }
        else if (otp_map_is_reg_unique[i])
        {
            OU_REGLIST_GET_ADDRESS(reg_list_size++) = otp_map_s->map[i].reg;
        }
    }

    free(otp_map_is_reg_unique);
    otp_map_is_reg_unique = NULL;

    return OTP_UNPACKER_STATUS_OK;
}

uint32_t otp_unpacker_get_index_by_address(uint32_t address, uint8_t *index)
{
    if (index == NULL)
    {
        return OTP_UNPACKER_STATUS_FAIL;
    }

    for (uint8_t i = 0; i < reg_list_size; i++)
    {
        if (OU_REGLIST_GET_ADDRESS(i) == address)
        {
            *index = i;
            return OTP_UNPACKER_STATUS_OK;
        }
    }

    return OTP_UNPACKER_STATUS_FAIL;
}

static uint32_t cs35l41_apply_trim_word(uint8_t *otp_mem,
                                        uint32_t bit_count,
                                        uint32_t *reg_val,
                                        uint32_t shift,
                                        uint32_t size)
{
    uint32_t ret = CS35L41_STATUS_OK;

    if ((otp_mem == NULL) || (reg_val == NULL) || (size == 0))
    {
        ret = CS35L41_STATUS_FAIL;
    }
    else
    {
        // Create bit-field mask to use on OTP contents
        uint32_t bitmask = ~(0xFFFFFFFF << size);
        uint64_t otp_bits = 0;  // temporary storage of bit-field
        // Using bit_count, get index of current 32-bit word in otp_mem
        uint32_t otp_mem_word_index = bit_count >> 5; // divide by 32
        // Get position of current bit in the current word in otp_mem
        uint32_t otp_mem_msword_bit_index = bit_count - (otp_mem_word_index << 5);

        // Skip ahead to the current 32-bit word
        otp_mem += ((otp_mem_word_index) * sizeof(uint32_t));

        // Shift the first 32-bit word into register - OTP bytes come over I2C in Little-Endian 32-bit words!
        otp_bits |= *(otp_mem++);
        otp_bits <<= 8;
        otp_bits |= *(otp_mem++);
        otp_bits <<= 8;
        otp_bits |= *(otp_mem++);
        otp_bits <<= 8;
        otp_bits |= *(otp_mem++);

        // If there's bits to get in the second 32-bit word, get them
        if ((size + otp_mem_msword_bit_index) > 32)
        {
            uint64_t temp_word = 0;
            temp_word |= *(otp_mem++);
            temp_word <<= 8;
            temp_word |= *(otp_mem++);
            temp_word <<= 8;
            temp_word |= *(otp_mem++);
            temp_word <<= 8;
            temp_word |= *(otp_mem++);

            otp_bits |= temp_word << 32;
        }

        // Right-justify the bits to get from OTP
        otp_bits >>= otp_mem_msword_bit_index;
        // Get only required number of OTP bits
        otp_bits &= bitmask;

        // Mask off bits in the current register value
        bitmask <<= shift;
        *reg_val &= ~(bitmask);

        // Or the OTP bits into the current register value
        *reg_val |= (otp_bits << shift);
    }

    return ret;
}

/***********************************************************************************************************************
 * API FUNCTIONS
 **********************************************************************************************************************/
uint32_t otp_unpacker_initialize(uint8_t otpid, uint8_t *otp_buffer)
{
    // Initialize library variables
    otp_map_s = NULL;
    otp_buffer_s = otp_buffer;
    reg_list_size = 0;

    if (reg_list)
    {
        otp_unpacker_deinitialize();
    }

    // Validate otpid
    for (uint8_t i = 0; i < (sizeof(cs35l41_otp_maps)/sizeof(cs35l41_otp_map_t)); i++)
    {
        if (cs35l41_otp_maps[i].id == otpid)
        {
            otp_map_s = &(cs35l41_otp_maps[i]);
        }
    }

    if (otp_map_s != NULL)
    {
        otp_unpacker_make_reg_list();
    }

    // Validate all pointers
    if ((otp_map_s != NULL) && (otp_buffer_s != NULL) && (reg_list != NULL))
    {
        return OTP_UNPACKER_STATUS_OK;
    }
    else
    {
        otp_unpacker_deinitialize();
    }

    return OTP_UNPACKER_STATUS_FAIL;
}

uint32_t otp_unpacker_deinitialize(void)
{
    if (reg_list)
    {
        free(reg_list);
        reg_list = NULL;
    }

    return OTP_UNPACKER_STATUS_OK;
}

uint32_t otp_unpacker_get_reg_list_total(uint8_t *total)
{
    if (total == NULL)
    {
        return OTP_UNPACKER_STATUS_FAIL;
    }

    *total = reg_list_size;

    return OTP_UNPACKER_STATUS_OK;
}

uint32_t otp_unpacker_get_reg_address(uint32_t *address, uint8_t index)
{
    if ((address == NULL) || (index >= reg_list_size))
    {
        return OTP_UNPACKER_STATUS_FAIL;
    }

    // +4 is to leave space at beginning for prepending UNLOCK sequence
    *address = OU_REGLIST_GET_ADDRESS(index);

    return OTP_UNPACKER_STATUS_OK;
}

uint32_t otp_unpacker_set_reg_value(uint8_t index, uint32_t value)
{
    if (index >= reg_list_size)
    {
        return OTP_UNPACKER_STATUS_FAIL;
    }

    OU_REGLIST_GET_VALUE(index) = value;

    return OTP_UNPACKER_STATUS_OK;
}

uint32_t otp_unpacker_get_unpacked_reg_list(uint32_t **reg_list_ptr, uint32_t *reg_list_total_words)
{
    uint8_t reg_list_index;

    if ((reg_list_ptr == NULL) || (reg_list_total_words == NULL))
    {
        return OTP_UNPACKER_STATUS_FAIL;
    }

    // First prepend the UNLOCK sequence
    reg_list[0] = CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG;
    reg_list[1] = CS35L41_TEST_KEY_CTRL_UNLOCK_1;
    reg_list[2] = CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG;
    reg_list[3] = CS35L41_TEST_KEY_CTRL_UNLOCK_2;

    // Unpack OTP and apply trims
    // Initialize OTP unpacking state - otp_bit_count.  There are bits in OTP to skip to reach the trims
    uint16_t otp_bit_count = otp_map_s->bit_offset;

    for (uint8_t i = 0; i < otp_map_s->num_elements; i++)
    {
        // Get trim entry
        cs35l41_otp_packed_entry_t temp_trim_entry = otp_map_s->map[i];

        // If the entry's 'reg' member is 0x0, it means skip that trim
        if (temp_trim_entry.reg != 0x00000000)
        {
            uint32_t ret;

            ret = otp_unpacker_get_index_by_address(temp_trim_entry.reg, &reg_list_index);
            if (ret)
            {
                return ret;
            }

            /*
             * Apply OTP trim bit-field to recently read trim register value.  OTP contents is saved in
             * cp_read_buffer + CS35L41_CP_REG_READ_LENGTH_BYTES
             *
             * Also reg_list[] indexing is +4 to leave space at beginning for prepending UNLOCK sequence
             */
            cs35l41_apply_trim_word(otp_buffer_s,
                                    otp_bit_count,
                                    &(OU_REGLIST_GET_VALUE(reg_list_index)),
                                    temp_trim_entry.shift,
                                    temp_trim_entry.size);
        }

        // Inrement the OTP unpacking state variable otp_bit_count
        otp_bit_count += temp_trim_entry.size;
    }


    // Appepend the LOCK sequence
    reg_list_index = reg_list_size;
    OU_REGLIST_GET_ADDRESS(reg_list_index) = CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG;
    OU_REGLIST_GET_VALUE(reg_list_index++) = CS35L41_TEST_KEY_CTRL_LOCK_1;
    OU_REGLIST_GET_ADDRESS(reg_list_index) = CS35L41_CTRL_KEYS_TEST_KEY_CTRL_REG;
    OU_REGLIST_GET_VALUE(reg_list_index++) = CS35L41_TEST_KEY_CTRL_LOCK_2;

    *reg_list_ptr = reg_list;
    *reg_list_total_words = OU_REGLIST_GET_TOTAL_WORDS(reg_list_size);

    return OTP_UNPACKER_STATUS_OK;
}
