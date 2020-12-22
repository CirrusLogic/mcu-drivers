/**
 * @file cs35l41_tune_fw_img.c
 *
 * @brief CS35L41_TUNE FW IMG Source File
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
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
 * firmware_converter.py version: 3.1.0
 * Command:  ..\..\tools\firmware_converter\firmware_converter.py fw_img_v2 cs35l41 halo_cspl_RAM_revB2_29.45.0.wmfw --sym-input ..\cs35l41_sym.h --wmdr-only --suffix tune --wmdr Protect_Lite_full_6.43.0_7.0ohm_delta1ohm_L41_revB2.bin
 *
 */

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "cs35l41_tune_fw_img.h"

/***********************************************************************************************************************
 * FW_IMG
 **********************************************************************************************************************/

/**
 * @defgroup CS35L41_TUNE_FW_IMG
 * @brief Firmware image
 *
 * @{
 */

const uint8_t cs35l41_tune_fw_img[] = {
// Header
0xFF,0x98,0xB9,0x54, // IMG_MAGIC_NUMBER_1
0x02,0x00,0x00,0x00, // IMG_FORMAT_REV
0xBC,0x0B,0x00,0x00, // IMG_SIZE
0x00,0x00,0x00,0x00, // SYM_TABLE_SIZE
0x00,0x00,0x00,0x00, // ALG_LIST_SIZE
0xA4,0x00,0x04,0x00, // FW_ID
0x00,0x19,0x00,0x00, // FW_VERSION
0x02,0x00,0x00,0x00, // DATA_BLOCKS
0x2C,0x10,0x00,0x00, // MAX_BLOCK_SIZE
0x00,0x00,0x00,0x00, // FW_IMG_VERSION
// Symbol Linking Table
// Algorithm ID List
// Firmware Data

// COEFF_DATA_BLOCKS_0
0x70,0x0B,0x00,0x00, // COEFF_BLOCK_SIZE_0_0
0x5C,0x01,0xC0,0x02, // COEFF_BLOCK_ADDR_0_0
0x04,0x00,0x03,0xD3,0x03,0x00,0x00,0x00,0x00,0x04,0xD4,0x00,0x00,0x00,0x0D,0x23,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
0x80,0x00,0x00,0x00,0x00,0x20,0x00,0xBB,0x00,0x00,0x0E,0x00,0x07,0x00,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x00,0x01,0x00,
0x02,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x08,0x00,0x03,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x04,0x00,
0x00,0x00,0x00,0x01,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x20,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
0x35,0x00,0x00,0x20,0x00,0x22,0x00,0x00,0x00,0x03,0x98,0x00,0x86,0x00,0x03,0x87,0x03,0xB6,0x00,0x03,0x00,0x03,0xAE,0x00,
0xAC,0x00,0x03,0xCD,0x03,0xAD,0x00,0x03,0x00,0x03,0xCB,0x00,0xCC,0x00,0x04,0x2A,0x04,0x4C,0x00,0x03,0x00,0x03,0x90,0x00,
0xB4,0x00,0x04,0x1A,0x03,0xB3,0x00,0x03,0x00,0x03,0x9D,0x00,0xA2,0x00,0x03,0xA5,0x04,0x18,0x00,0x03,0x00,0x03,0xA6,0x00,
0x7C,0x00,0x03,0xA4,0x00,0x6F,0x00,0x00,0x00,0x03,0xCE,0x00,0xA1,0x00,0x03,0x9C,0x03,0x9E,0x00,0x03,0x00,0x03,0x9F,0x00,
0xD4,0x00,0x03,0xB0,0x04,0x14,0x00,0x03,0x00,0x04,0x17,0x00,0x82,0x00,0x04,0x16,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,
0x11,0x00,0x00,0x06,0x00,0x01,0x00,0x00,0x00,0x00,0x07,0x00,0x02,0x00,0x00,0x11,0x00,0x08,0x00,0x00,0x00,0x00,0x11,0x00,
0x09,0x00,0x00,0x03,0x00,0x06,0x00,0x00,0x00,0x00,0x07,0x00,0x07,0x00,0x00,0x08,0x00,0x1E,0x00,0x00,0x00,0x00,0x07,0x00,
0x00,0x00,0x00,0x07,0x21,0xC3,0x00,0x00,0x3F,0xDE,0x3C,0x40,0x00,0x00,0x00,0x00,0x00,0x07,0x80,0x00,0x00,0x00,0x07,0x00,
0x00,0x00,0x00,0x07,0x00,0x00,0x40,0x00,0x00,0x00,0x20,0x00,0xB7,0x00,0x00,0x04,0x00,0x20,0x17,0x6D,0x00,0x00,0x05,0x00,
0x20,0x00,0x98,0xFA,0x00,0x06,0x00,0x00,0x08,0x17,0x48,0x00,0x07,0x00,0x00,0x20,0x13,0x37,0x00,0x00,0x00,0x00,0x20,0x02,
0xE5,0x00,0x00,0x08,0x00,0x20,0x02,0x3E,0x00,0x00,0x09,0x00,0x20,0x00,0x05,0x77,0x00,0x0A,0x00,0x00,0x1B,0x80,0x00,0x00,
0x0B,0x00,0x00,0x20,0x08,0x54,0x00,0x00,0x00,0x00,0x2B,0x00,0x02,0x00,0x00,0x03,0x00,0x01,0x00,0x00,0x00,0x00,0x0D,0x00,
0xC7,0x00,0x00,0x0C,0x00,0x20,0x00,0x20,0x00,0x00,0x22,0x00,0x31,0x00,0x00,0x00,0x00,0x06,0x00,0x20,0x00,0x00,0x0E,0x00,
0x31,0x00,0x00,0x20,0x00,0x09,0x00,0x20,0x00,0x00,0x0F,0x00,0x20,0x00,0x00,0x20,0x00,0x23,0x00,0x00,0xFA,0x80,0x00,0x00,
0x06,0x00,0x00,0x07,0x00,0x08,0x00,0x00,0x43,0x33,0x33,0x00,0x27,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x08,0x00,
0x03,0x00,0x00,0x0A,0x00,0x0A,0x00,0x00,0x00,0x00,0x0A,0x00,0xE7,0x80,0x3C,0xA9,0x00,0x72,0x7F,0x86,0x00,0x00,0x72,0x00,
0x07,0x00,0x00,0x72,0x00,0x0A,0x00,0x00,0x00,0x00,0x0A,0x00,0x00,0x08,0x00,0x00,0x00,0x32,0x00,0x00,0x00,0x00,0x06,0x00,
0x55,0x00,0x00,0x0B,0x69,0xA8,0x87,0x8D,0x01,0x0D,0x2C,0x71,0x2C,0x01,0x0D,0x2C,0x00,0x07,0x01,0x0D,0x00,0x00,0x0B,0x00,
0x00,0x00,0x00,0x0B,0x00,0x00,0x08,0x00,0x00,0x00,0x32,0x00,0x0B,0x00,0x00,0x0B,0x64,0x90,0x00,0x00,0x79,0xBF,0xC5,0x83,
0x2C,0x01,0x0D,0x2C,0x0D,0x2C,0x01,0x0D,0x00,0x00,0x07,0x01,0x0B,0x00,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,
0x0B,0x00,0x00,0x3E,0x00,0x0B,0x00,0x00,0x89,0x9A,0x21,0x00,0xF6,0x6D,0xA1,0x19,0xB3,0xF6,0x01,0xB3,0x01,0xB3,0xF6,0x01,
0x07,0x00,0x00,0x08,0x00,0x0B,0x00,0x00,0x00,0x00,0x0B,0x00,0x00,0x08,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x00,0x0B,0x00,
0x1A,0x00,0x00,0x0B,0x0E,0x83,0x84,0x68,0x01,0xB3,0xF6,0x78,0xF6,0x01,0xB3,0xF6,0x00,0x08,0x01,0xB3,0x00,0x00,0x07,0x00,
0x0B,0x00,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x0B,0x00,0x00,0x3E,0x00,0x0B,0x00,0x00,0x84,0x1F,0xF0,0x00,
0xF6,0x7C,0xD8,0x12,0x00,0x00,0x01,0x93,0xFE,0x6C,0x0A,0x00,0x3E,0x00,0x00,0x20,0x00,0x0B,0x00,0x00,0x00,0x00,0x0B,0x00,
0x12,0x84,0x1F,0xF0,0x93,0xF6,0x7C,0xD8,0x00,0x00,0x00,0x01,0x20,0xFE,0x6C,0x0A,0x00,0x20,0x00,0x00,0x00,0x00,0x24,0x00,
0x32,0x00,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x00,0x0C,0x00,0xA8,0x87,0x8D,0x55,0x0D,0x2C,0x71,0x69,0x01,0x0D,0x2C,0x01,
0x07,0x01,0x0D,0x2C,0x00,0x0C,0x00,0x00,0x00,0x00,0x0C,0x00,0x00,0x08,0x00,0x00,0x00,0x32,0x00,0x00,0x00,0x00,0x0C,0x00,
0x90,0x00,0x00,0x0C,0xBF,0xC5,0x83,0x64,0x01,0x0D,0x2C,0x79,0x2C,0x01,0x0D,0x2C,0x00,0x07,0x01,0x0D,0x00,0x00,0x0C,0x00,
0x00,0x00,0x00,0x0C,0x00,0x00,0x08,0x00,0x00,0x00,0x3E,0x00,0x0C,0x00,0x00,0x0C,0x9A,0x21,0x00,0x00,0x6D,0xA1,0x19,0x89,
0xF6,0x01,0xB3,0xF6,0xB3,0xF6,0x01,0xB3,0x00,0x00,0x08,0x01,0x0C,0x00,0x00,0x07,0x00,0x0C,0x00,0x00,0x08,0x00,0x00,0x00,
0x3E,0x00,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,0x00,0x0C,0x00,0x83,0x84,0x68,0x1A,0xB3,0xF6,0x78,0x0E,0x01,0xB3,0xF6,0x01,
0x08,0x01,0xB3,0xF6,0x00,0x07,0x00,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,0x00,0x0C,0x00,0x00,0x08,0x00,0x00,0x00,0x3E,0x00,
0x0C,0x00,0x00,0x0C,0x1F,0xF0,0x00,0x00,0x7C,0xD8,0x12,0x84,0x00,0x01,0x93,0xF6,0x6C,0x0A,0x00,0x00,0x00,0x00,0x20,0xFE,
0x0C,0x00,0x00,0x3E,0x00,0x0C,0x00,0x00,0x84,0x1F,0xF0,0x00,0xF6,0x7C,0xD8,0x12,0x00,0x00,0x01,0x93,0xFE,0x6C,0x0A,0x00,
0x13,0x00,0x00,0x20,0x00,0x0B,0x00,0x00,0x00,0x00,0x0C,0x00,0xE0,0x00,0x00,0x25,0x00,0x11,0x00,0x01,0x00,0x02,0x0C,0x00,
0x33,0x00,0x00,0x0D,0x00,0x25,0x00,0x00,0x00,0x00,0x24,0x00,0x31,0x00,0x00,0x10,0x00,0x0A,0x00,0x20,0x00,0x00,0x26,0x00,
0x3C,0x00,0x00,0x20,0x00,0x0A,0x00,0x00,0x00,0x00,0x22,0x00,0x0D,0x00,0x00,0x10,0x00,0x27,0x00,0x00,0x00,0x00,0x28,0x00,
0x2A,0x00,0x00,0x11,0x00,0x29,0x00,0x00,0x00,0x00,0x26,0x00,0x74,0x00,0xFE,0x8B,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xFC,0x02,0xC0,0xA1,0xC8,0xFC,0xFE,0xBA,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0xCC,0x00,0x00,0x00,0xFE,0x1A,0x00,0x00,0x00,0x2A,0x8B,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x32,0x00,0x00,0x00,0x00,0x00,0x12,0xF5,0x00,
0xCC,0x00,0x23,0xAA,0x00,0x0C,0x00,0x10,0x00,0x00,0x00,0x00,0x5B,0x00,0x00,0x0D,0x41,0x51,0x00,0x00,0x03,0x28,0x5D,0x00,
0x82,0x01,0x7F,0xFE,0x49,0xA9,0x00,0x68,0x00,0x20,0xB6,0x00,0x3E,0x00,0x10,0xC7,0x0B,0xB2,0x00,0x0C,0x00,0x00,0x0C,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCE,0x00,0x0C,0x00,0x1B,0x28,0x2D,0x51,0x00,0x54,0x01,0x3C,0xB9,0x02,
0x14,0x00,0x50,0xCC,0x0C,0x72,0x00,0x18,0x00,0x0B,0x38,0x00,0x63,0x00,0x00,0x0C,0xD4,0x69,0xD6,0xF4,0x5B,0xEC,0xD9,0x6B,
0x9F,0x21,0x6F,0xE3,0x1D,0x28,0x0E,0xAE,0x04,0x7C,0x7B,0x0A,0x59,0x04,0x01,0x48,0x03,0x2B,0x03,0x52,0x02,0xEB,0x33,0x03,
0x0C,0x02,0xE8,0x42,0x71,0xD5,0x00,0x00,0x6C,0xEE,0x34,0x0D,0x27,0x62,0x6D,0xE8,0x77,0x58,0x2F,0x25,0x11,0x37,0x63,0x17,
0x02,0x08,0x39,0xB9,0x3D,0x00,0x07,0x68,0x05,0xB3,0x37,0x06,0x16,0x05,0x89,0x40,0x00,0x0C,0x05,0x84,0x19,0x4B,0x15,0x00,
0xE7,0x0F,0x33,0x68,0x58,0xCD,0x10,0x36,0x08,0xF3,0xB7,0x0E,0x4E,0x07,0x20,0xBB,0x62,0xA0,0x03,0xBA,0x02,0xE4,0xBC,0x03,
0x47,0x02,0xA9,0x76,0x95,0x09,0x02,0x97,0x00,0x00,0x0C,0x02,0x01,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x05,0x00,
0x15,0x00,0x00,0x0B,0x00,0x27,0x00,0x00,0x00,0x00,0x49,0x00,0xFD,0x00,0x00,0x88,0x01,0xD5,0x00,0x00,0x00,0x03,0x64,0x00,
0x00,0x00,0x00,0x00,0x18,0x93,0x00,0x00,0x00,0x07,0xA0,0x00,0xC8,0xD6,0x55,0x56,0x05,0x76,0x00,0x10,0x05,0x00,0x00,0x00,
0xE0,0x04,0xC7,0xF4,0x00,0x00,0x00,0x01,0x01,0x80,0x00,0x00,0x64,0x2C,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x34,0x80,0x00,0x00,0x65,0x00,0x00,0x00,0x00,0x0A,0x00,0x1A,0x00,0x00,0x28,0x00,0x0A,0x00,0x00,0x00,0x00,0x0A,0x00,
0x31,0x00,0x00,0x01,0x00,0x0A,0x00,0x20,0x00,0x00,0x2B,0x00,0x65,0x00,0x00,0x01,0x00,0x0A,0x00,0x00,0x00,0x00,0x29,0x00,
0x0A,0x00,0x00,0x1A,0x00,0x0A,0x00,0x00,0x00,0x00,0x01,0x00,0x0A,0x00,0x20,0x31,0x00,0x2C,0x00,0x00,0x00,0x00,0x01,0x00,
0x0A,0x00,0x00,0x65,0x00,0x2A,0x00,0x00,0x00,0x00,0x1A,0x00,0x0A,0x00,0x00,0x0A,0x00,0x01,0x00,0x00,0x00,0x20,0x31,0x00,
0x12,0x00,0x00,0x0A,0x00,0x01,0x00,0x00,0x00,0x00,0x5B,0x00,0x26,0x00,0x00,0x27,0x00,0x2B,0x00,0x00,0x00,0x00,0x12,0x00,
0x0C,0x00,0x00,0x13,0x00,0x12,0x00,0x03,0x00,0x00,0x07,0x00,0x0A,0x00,0x00,0x06,0x33,0x33,0x00,0x00,0x00,0x00,0x01,0x43,
0x0A,0x00,0x00,0x32,0x00,0x0A,0x00,0x00,0x81,0x2F,0x4B,0x00,0x19,0x7D,0xA6,0xFA,0x2D,0xE7,0x7E,0xD2,0x7E,0xD2,0x19,0x81,
0x0A,0x00,0x00,0x32,0x00,0x0A,0x00,0x00,0x81,0x2F,0x4B,0x00,0x19,0x7D,0xA6,0xFA,0x2D,0xE7,0x7E,0xD2,0x7E,0xD2,0x19,0x81,
0x2D,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x00,0x2D,0x00,0x00,0x27,0x00,0x2E,0x00,0x00,0x00,0x00,0x07,0x00,
0x0B,0x00,0x00,0x09,0x66,0x66,0x00,0x00,0x00,0x00,0x03,0x62,0x0B,0x00,0x00,0x32,0x00,0x0B,0x00,0x00,0x81,0x2F,0x4B,0x00,
0x19,0x7D,0xA6,0xFA,0x2D,0xE7,0x7E,0xD2,0x7E,0xD2,0x19,0x81,0x0B,0x00,0x00,0x32,0x00,0x0B,0x00,0x00,0x81,0x2F,0x4B,0x00,
0x19,0x7D,0xA6,0xFA,0x2D,0xE7,0x7E,0xD2,0x7E,0xD2,0x19,0x81,0x0B,0x00,0x00,0x2E,0x00,0x0A,0x00,0x00,0x00,0x00,0x0A,0x00,
0x22,0x00,0x00,0x2E,0x00,0x0A,0x00,0x00,0x2E,0xDB,0x6D,0x00,0xAC,0x80,0x16,0xF1,0x5C,0x28,0x7F,0xFF,0x1D,0x5C,0x31,0x7F,
0x00,0x39,0x99,0x99,0x00,0x07,0x40,0x00,0x00,0x00,0x0A,0x00,0xD4,0x00,0x00,0x0A,0x00,0x00,0x3F,0x47,0x00,0x00,0x07,0x00,
0x0B,0x00,0x00,0x09,0x93,0x88,0x00,0x00,0x00,0x00,0x00,0x50,0x0B,0x00,0x00,0x03,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,
0xFF,0x00,0x00,0x00,0x00,0x00,0x7F,0xFF,0x00,0x00,0x00,0x00,0x0B,0x00,0x00,0x03,0x00,0x0B,0x00,0x00,0x85,0x1A,0xF6,0x00,
0xB7,0x77,0xE9,0x09,0x41,0xB7,0x00,0x41,0x00,0x41,0xB7,0x00,0x0A,0x00,0x20,0x31,0x00,0x14,0x00,0x00,0x00,0x00,0x30,0x00,
0x0B,0x00,0x20,0x31,0x00,0x15,0x00,0x00,0x00,0x00,0x30,0x00,0x15,0x00,0x20,0x2F,0x00,0x14,0x00,0x00,0x00,0x00,0x16,0x00,
0x30,0x00,0x00,0x01,0x4C,0xC4,0x00,0x00,0x7C,0x4C,0xC4,0x7C,0x00,0x02,0x22,0xBE,0x36,0xF9,0x00,0x00,0x00,0x00,0x05,0x7D,
0x00,0x00,0x00,0x00,0x00,0x33,0x30,0x00,0x00,0x00,0x2B,0x00,0x17,0x00,0x00,0x23,0x00,0x33,0x00,0x00,0x00,0x00,0x2C,0x00,
0x18,0x00,0x00,0x23,0x00,0x03,0x00,0x00,0x00,0x00,0x07,0x00,0x7C,0x00,0x00,0x0A,0xAC,0x08,0x83,0x38,0x7E,0x4B,0x0A,0x79,
0x09,0x81,0xB5,0x27,0x00,0x03,0x7E,0x4B,0x00,0x00,0x0A,0x00,0x83,0x00,0x00,0x0A,0x17,0xD7,0x80,0xFB,0x7E,0x4B,0x0A,0x7E,
0x0C,0x81,0xB6,0x50,0x00,0x03,0x7E,0x4B,0x00,0x00,0x0A,0x00,0x59,0x00,0x00,0x0A,0x94,0x46,0x80,0x3B,0x7E,0x4B,0x0A,0x7F,
0x09,0x81,0xB7,0x34,0x00,0x03,0x7E,0x4B,0x00,0x00,0x0A,0x00,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,0x00,0x7F,0xFF,0xFF,0x00,
0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x0B,0x00,0xF6,0x00,0x00,0x0B,0xE9,0x09,0x85,0x1A,0x00,0x41,0xB7,0x77,
0xB7,0x00,0x41,0xB7,0x00,0x1B,0x00,0x41,0x00,0x00,0x0A,0x00,0x00,0x00,0x00,0x0C,0x00,0x01,0x00,0x00,0x7F,0xF8,0x1D,0x00,
0x0B,0x00,0x00,0x1B,0x00,0x0D,0x00,0x00,0x00,0x00,0x00,0x00,0x1D,0x00,0x00,0x01,0x20,0x30,0x7F,0xF8,0x00,0x00,0x0A,0x00,
0x19,0x00,0x00,0x0C,0x00,0x08,0x00,0x00,0x00,0xCC,0xCC,0x00,0xCC,0x40,0x00,0x00,0x8F,0x5C,0x0C,0xCC,0x00,0x14,0x7A,0x02,
0xC0,0x00,0x27,0x10,0x00,0x2D,0x01,0xD4,0x00,0x00,0x0B,0x00,0x0B,0x00,0x00,0x0D,0x00,0x19,0x00,0x00,0x00,0xCA,0x7F,0x00,
0x00,0x00,0x3C,0xBF,0x00,0x00,0x00,0x00,0x00,0x00,0x1E,0x00,0x0B,0x00,0x00,0x0B,0x00,0x16,0x00,0x00,0x7F,0xFF,0xFD,0x00,
0xC4,0x20,0x00,0x74,0x00,0x00,0x00,0x20,0x00,0x00,0x07,0x80,0x0B,0x00,0x00,0x0B,0x00,0x60,0x00,0x00,0x00,0x00,0x03,0x40,
0x0B,0x00,0x00,0x29,0x00,0x0C,0x00,0x00,0x00,0x00,0x0B,0x00,0x1B,0x00,0x00,0x10,0x00,0x0B,0x00,0x00,0x00,0x00,0x0D,0x00,
0x01,0x00,0x00,0x90,0xF4,0x2C,0x00,0x00,0x00,0x00,0x16,0x7F,0x0A,0x00,0x00,0x0A,0x00,0xA0,0x00,0x00,0x00,0x00,0x28,0x00,
0x0D,0x00,0x00,0x0D,0x3E,0xAA,0x00,0x00,0x00,0x20,0x31,0x02,0x1A,0x00,0x00,0x0B,0x00,0x20,0x00,0x00,0x00,0x20,0x31,0x00,
0x1B,0x00,0x00,0x0D,0x00,0x20,0x00,0x00,0x00,0x00,0x12,0x00,0x0B,0x00,0x00,0x0D,0x00,0x10,0x00,0x00,0x00,0x80,0x00,0x00,
0x00,0x00,0x80,0x00,0x80,0x00,0x00,0x80,0x00,0x80,0x00,0x00,0x00,0x00,0x80,0x00,0x80,0x00,0x00,0x80,0x00,0x80,0x00,0x00,
0x00,0x00,0x80,0x00,0x80,0x00,0x00,0x80,0x00,0x80,0x00,0x00,0x00,0x00,0x80,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x27,0x00,
0x0B,0x00,0x00,0x0A,0x00,0x0D,0x00,0x00,0x00,0x00,0x27,0x00,0x0B,0x00,0x00,0x0C,0x00,0x0B,0x00,0x00,0x00,0x20,0x31,0x00,
0x1C,0x00,0x00,0x0B,0x00,0x20,0x00,0x00,0x00,0x00,0x03,0x00,0x0A,0x00,0x00,0x0A,0x2B,0xC7,0x00,0x00,0x5B,0xBA,0xA8,0x95,
0x3A,0x6C,0x58,0xC6,0x58,0xC6,0x93,0xA7,0x00,0x00,0x03,0x6C,0x0A,0x00,0x00,0x0A,0x2B,0xC7,0x00,0x00,0x5B,0xBA,0xA8,0x95,
0x3A,0x6C,0x58,0xC6,0x58,0xC6,0x93,0xA7,0x00,0x00,0x03,0x6C,0x0B,0x00,0x00,0x0D,0x2B,0xC7,0x00,0x00,0x5B,0xBA,0xA8,0x95,
0x8D,0x01,0x84,0x8D,0x84,0x8D,0x01,0x84,0x00,0x00,0x03,0x01,0x0B,0x00,0x00,0x0B,0x2B,0xC7,0x00,0x00,0x5B,0xBA,0xA8,0x95,
0x8D,0x01,0x84,0x8D,0x84,0x8D,0x01,0x84,0x00,0x00,0x04,0x01,0x0A,0x00,0x00,0x0B,0x00,0x0A,0x00,0x00,0x00,0x00,0x16,0x00,
0x0B,0x00,0x00,0x0A,0x00,0x10,0x00,0x00,0x00,0x00,0x1B,0x00,0x0A,0x00,0x00,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x34,0x00,
0x28,0x7F,0xFF,0x36,0x00,0x0A,0x00,0x00,0x00,0x00,0x0A,0x00,0x03,0x08,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x00,0x0A,0x00,
0x20,0xB8,0xC8,0xFC,0x4C,0x45,0x27,0x9F,0x06,0x4C,0x45,0x06,0x27,0x06,0x4C,0x45,0x00,0x0B,0x00,0x00,0x00,0x00,0x0A,0x00,
0x07,0x00,0x00,0x0A,0x00,0x0A,0x00,0x00,0x00,0x00,0x0A,0x00,0x04,0x40,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x0A,0x00,
0x00,0x00,0x00,0x0A,0x00,0x00,0x04,0x00,0x00,0x20,0x31,0x00,0x1D,0x00,0x00,0x07,0x00,0x20,0x00,0x00,0x00,0x00,0x2C,0x00,
0x2F,0x00,0x00,0x13,0x00,0x02,0x00,0x00,0x0F,0x00,0x00,0x00,0x02,0x4B,0x00,0x00,0xFF,0xFF,0x00,0x00,0x20,0x26,0xF3,0x7F,
0x17,0x00,0x00,0x2C,0x00,0x30,0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x14,0x00,0x00,0x80,0x00,0x15,0x40,0x17,0xC0,0x00,0x16,
0x05,0x19,0x00,0x00,0xFF,0xFF,0x00,0x00,0x79,0xD1,0xF5,0x7F,0x9F,0x67,0x47,0xD4,0x1F,0x53,0x48,0x61,0x00,0x00,0x36,0x1D,
0x30,0x00,0x00,0x17,0x00,0x31,0x00,0x00,0x00,0x00,0x1E,0x00,0xFF,0x07,0x53,0x00,0x26,0xE7,0x7F,0xFF,0x00,0x00,0x36,0x40,
0x2F,0x00,0x00,0x17,0x00,0x33,0x00,0x00,0x00,0x00,0x32,0x00,0xFF,0x07,0x53,0x00,0xFF,0xFF,0x7F,0xFF,0x00,0x00,0x20,0x7F,
0x00,0x00,0x00,0x34,0x00,0x65,0x08,0x00,0x00,0x00,0x07,0x00,0x1E,0x00,0x00,0x34,0x00,0x07,0x00,0x00,0x00,0x00,0x0B,0x00,
0xEE,0x00,0x00,0x31,0xFC,0x12,0x40,0x03,0x00,0x00,0x00,0x3F,0x42,0x80,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,0x00,0x1F,0x00,
0x00,0x00,0x00,0x20,0x00,0x1E,0x08,0x00,0x00,0x00,0x07,0x00,0x33,0x00,0x00,0x07,0x00,0x32,0x00,0x00,0x3F,0xFF,0xCD,0x40,
0x00,0x00,0x00,0x00,0x00,0x27,0x80,0x00,0x00,0x00,0x0B,0x00,0x0B,0x00,0x00,0x07,0x00,0x27,0x00,0x00,0x00,0x00,0x0A,0x00,
0x0A,0x00,0x00,0x0B,0x00,0x07,0x00,0x00,0x00,0x00,0x0A,0x00,0xC2,0x00,0x00,0x0C,0x00,0x00,0x3F,0x4A,0x00,0x00,0x19,0x00,
0x0A,0x00,0x00,0x0A,0x00,0x30,0x00,0x00,0x00,0x16,0xA7,0x00,0x60,0x00,0x06,0x11,0x00,0x03,0x00,0x09,0x00,0x00,0x0A,0x00,
0x4F,0x00,0x00,0x0A,0x5A,0x1E,0x80,0xD4,0x00,0x00,0xAE,0x7E,0xAE,0x00,0x00,0xAE,0x00,0x04,0x00,0x00,0x00,0x00,0x0C,0x00,
0x0A,0x00,0x00,0x0A,0x00,0x10,0x00,0x00,0x00,0x00,0x0A,0x00,0x10,0x00,0x00,0x04,0x00,0x0A,0x00,0x00,0x00,0x00,0x05,0x00,
0x0B,0x00,0x00,0x42,0x00,0x20,0x00,0x00,0x00,0x00,0x20,0x00,0x42,0x08,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x21,0x00,
0x0C,0x00,0x00,0x00, // COEFF_BLOCK_SIZE_0_1
0x10,0x11,0x40,0x03, // COEFF_BLOCK_ADDR_0_1
0x00,0x00,0x00,0x20,0x00,0x08,0x00,0x00,0x00,0x7F,0xFF,0xFF,

// Footer
0xA6,0xE2,0x6B,0x93, // IMG_MAGIC_NUMBER_2
0xA6,0x4E,0xBF,0x0F, // IMG_CHECKSUM
};

/** @} */

/**********************************************************************************************************************/
