/**
 * @file fw_img_v1.c
 *
 * @brief The fw_img_v1 decode module
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
#include <stddef.h>
#include "fw_img_v1.h"

static uint32_t fw_img_copy_data(fw_img_boot_state_t *state, uint32_t *data, uint32_t data_size)
{
    while ((state->count * sizeof(uint32_t)) < data_size &&
            state->fw_img_blocks < state->fw_img_blocks_end)
    {
        data[state->count++] = *(uint32_t *)state->fw_img_blocks;
        state->fw_img_blocks += sizeof(uint32_t);
    }

    if ((state->count * sizeof(uint32_t)) == data_size)
        return FW_IMG_STATUS_AGAIN;
    else
        return FW_IMG_STATUS_NODATA;
}

static uint32_t fw_img_process_data(fw_img_boot_state_t *state)
{
    uint32_t ret = FW_IMG_STATUS_AGAIN;
    fw_img_v1_info_t *fw_info = &state->fw_info;

    switch (state->state)
    {
        case FW_IMG_BOOT_STATE_INIT:
            state->count = 0;
            break;

        case FW_IMG_BOOT_STATE_READ_SYMBOLS:
            ret = fw_img_copy_data(state, (uint32_t *) fw_info->sym_table, fw_info->header.sym_table_size * sizeof(fw_img_v1_sym_table_t));
            break;

        case FW_IMG_BOOT_STATE_READ_ALGIDS:
            ret = fw_img_copy_data(state, fw_info->alg_id_list, fw_info->header.alg_id_list_size * sizeof(fw_info->header.alg_id_list_size));
            break;

        case FW_IMG_BOOT_STATE_READ_DATA_HEADER:
            if (fw_info->header.data_blocks > 0)
            {
                ret = fw_img_copy_data(state, (uint32_t *)&state->block, sizeof(state->block));
            }
            else
            {
                state->state = FW_IMG_BOOT_STATE_READ_FOOTER - 1; // Will be incremented
            }
            break;

        case FW_IMG_BOOT_STATE_WRITE_DATA:
            if (state->block.block_size > state->block_data_size)
            {
                ret = FW_IMG_STATUS_FAIL;
                break;
            }
            ret = fw_img_copy_data(state, (uint32_t *)state->block_data, state->block.block_size);
            if (ret == FW_IMG_STATUS_AGAIN)
            {
                ret = FW_IMG_STATUS_DATA_READY;
                state->count = 0;
                fw_info->header.data_blocks--;
                state->state = FW_IMG_BOOT_STATE_READ_DATA_HEADER;
            }
            break;

        case FW_IMG_BOOT_STATE_READ_FOOTER:
            ret = fw_img_copy_data(state, (uint32_t *)&state->footer, sizeof(state->footer));
            if (ret == FW_IMG_STATUS_AGAIN)
            {
                if (state->footer.img_magic_number_2 != FW_IMG_BOOT_FW_IMG_V1_MAGIC_2)
                {
                    ret = FW_IMG_STATUS_FAIL;
                }
                else
                {
                    state->state = FW_IMG_BOOT_STATE_DONE;
                    ret = FW_IMG_STATUS_OK;
                }
            }
            break;

        case FW_IMG_BOOT_STATE_DONE:
            state->state = FW_IMG_BOOT_STATE_INIT - 1; // Will be incremented
            break;

        default:
            ret = FW_IMG_STATUS_FAIL;
            break;
    }

    if (ret == FW_IMG_STATUS_AGAIN)
    {
        state->state++;
        state->count = 0;
    }

    return ret;
}

uint32_t fw_img_read_header(fw_img_boot_state_t *state)
{
    uint32_t ret = FW_IMG_STATUS_OK;
    fw_img_v1_info_t *fw_info = &state->fw_info;

    if (state == NULL || state->fw_img_blocks == NULL || state->fw_img_blocks_size ==  0)
    {
        return FW_IMG_STATUS_FAIL;
    }

    state->fw_img_blocks_end = state->fw_img_blocks + state->fw_img_blocks_size;

    ret = fw_img_copy_data(state, (uint32_t *)&fw_info->header, sizeof(fw_info->header));
    if (ret != FW_IMG_STATUS_AGAIN ||
        fw_info->header.img_magic_number_1 != FW_IMG_BOOT_FW_IMG_V1_MAGIC_1)
    {
        ret = FW_IMG_STATUS_FAIL;
    }

    state->fw_img_blocks_end = NULL;
    state->count = 0;

    return FW_IMG_STATUS_OK;
}

uint32_t fw_img_process(fw_img_boot_state_t *state)
{
    uint32_t ret = FW_IMG_STATUS_OK;

    if (state == NULL || state->fw_img_blocks == NULL || state->fw_img_blocks_size == 0 || state->block_data == NULL || state->fw_info.sym_table == NULL || state->fw_info.alg_id_list == NULL)
    {
        return FW_IMG_STATUS_FAIL;
    }

    if (state->fw_img_blocks_end == NULL)
    {
        state->fw_img_blocks_end = state->fw_img_blocks + state->fw_img_blocks_size;
    }

    do {
        ret = fw_img_process_data(state);
    } while (ret == FW_IMG_STATUS_AGAIN);

    if (ret == FW_IMG_STATUS_NODATA)
    {
        state->fw_img_blocks_end = NULL;
    }

    return ret;
}
