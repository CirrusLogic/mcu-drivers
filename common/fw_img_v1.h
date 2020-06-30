/**
 * @file fw_img_v1.h
 *
 * @brief Functions and prototypes exported by the fw_img_v1 decode module
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

#ifndef FW_IMG_V1_H
#define FW_IMG_V1_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#define FW_IMG_STATUS_OK                               (0)
#define FW_IMG_STATUS_FAIL                             (1)
#define FW_IMG_STATUS_AGAIN                            (2)
#define FW_IMG_STATUS_NODATA                           (4)
#define FW_IMG_STATUS_DATA_READY                       (5)

/**
 * @defgroup FW_IMG_BOOT_STATE_
 * @brief State of the driver
 *
 * @see fw_img_boot_state_t member state
 *
 * @{
 */
#define FW_IMG_BOOT_STATE_INIT                         (0)
#define FW_IMG_BOOT_STATE_READ_SYMBOLS                 (1)
#define FW_IMG_BOOT_STATE_READ_ALGIDS                  (2)
#define FW_IMG_BOOT_STATE_READ_DATA_HEADER             (3)
#define FW_IMG_BOOT_STATE_WRITE_DATA                   (4)
#define FW_IMG_BOOT_STATE_READ_FOOTER                  (5)
#define FW_IMG_BOOT_STATE_DONE                         (6)
 /** @} */

/**
 * @defgroup FW_IMG_BOOT_
 * @brief State of the driver
 *
 * @see fw_img_boot_state_t
 *
 * @{
 */
#define FW_IMG_BOOT_FW_IMG_V1_MAGIC_1                  (0x54b998ff)
#define FW_IMG_BOOT_FW_IMG_V1_MAGIC_2                  (0x936be2a6)
 /** @} */

/**
 * Footer for fw_img_v1
 */
typedef struct
{
    uint32_t img_magic_number_2;
    uint32_t img_checksum;
} fw_img_v1_footer_t;

/**
 * Header for fw_img_v1 data blocks
 */
typedef struct
{
    uint32_t block_size;
    uint32_t block_addr;
} fw_img_v1_data_block_t;

/**
 * Symbol table struct for fw_img_v1
 */
typedef struct
{
    uint32_t sym_id;
    uint32_t sym_addr;
} fw_img_v1_sym_table_t;

/**
 * Header for fw_img_v1
 */
typedef struct
{
    uint32_t img_magic_number_1;
    uint32_t img_format_rev;
    uint32_t img_size;
    uint32_t sym_table_size;
    uint32_t alg_id_list_size;
    uint32_t fw_id;
    uint32_t fw_version;
    uint32_t data_blocks;
} fw_img_v1_header_t;

/**
 * Data structure to describe HALO firmware info
 */
typedef struct
{
    fw_img_v1_header_t header;
    fw_img_v1_sym_table_t *sym_table;
    uint32_t *alg_id_list;
} fw_img_v1_info_t;

/**
 * Data structure to describe HALO firmware and coefficient download.
 */
typedef struct
{
    int8_t state;
    uint32_t count;
    uint32_t fw_img_blocks_size;                // Initialised by user
    uint8_t *fw_img_blocks;                     // Initialised by user

    uint8_t *fw_img_blocks_end;

    fw_img_v1_data_block_t block;
    uint32_t block_data_size;                   // Initialised by user after fw_img_read_header()
    uint8_t *block_data;                        // Initialised by user after fw_img_read_header()

    fw_img_v1_info_t fw_info;
    fw_img_v1_footer_t footer;
} fw_img_boot_state_t;

extern uint32_t fw_img_read_header(fw_img_boot_state_t *state);
extern uint32_t fw_img_process(fw_img_boot_state_t *state);

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // CS40L30_H
