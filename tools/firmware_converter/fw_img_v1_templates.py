#==========================================================================
# (c) 2020 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Templates for C Source and Header files
# File    : fw_img_v1_templates.py
#--------------------------------------------------------------------------
# Redistribution and use of this file in source and binary forms, with
# or without modification, are permitted.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#--------------------------------------------------------------------------
#
# Environment Requirements: None
#
#==========================================================================

#==========================================================================
# IMPORTS
#==========================================================================
import re

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
header_file_template_str = """/**
 * @file {part_number_lc}_fw_img.h
 *
 * @brief {part_number_uc} FW IMG Header File
 *
 * @copyright
 * Copyright (c) Cirrus Logic 2020 All Rights Reserved, http://www.cirrus.com/
 *
 * This code and information are provided 'as-is' without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a particular
 * purpose.
{metadata_text} *
 */

#ifndef {part_number_uc}_FW_IMG_H
#define {part_number_uc}_FW_IMG_H

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>

/***********************************************************************************************************************
 * FW_IMG
 **********************************************************************************************************************/

/**
 * @defgroup {part_number_uc}_FW_IMG
 * @brief Firmware image
 *
 * @{
 */

const uint8_t {part_number_lc}_fw_img[] = {
// Header
{magic_number_1} // IMG_MAGIC_NUMBER_1
{img_format_rev} // IMG_FORMAT_REV
{img_size} // IMG_SIZE
{sym_table_size} // SYM_TABLE_SIZE
{alg_list_size} // ALG_LIST_SIZE
{fw_id} // FW_ID
{fw_ver} // FW_VERSION
{data_block_count} // DATA_BLOCKS
// Symbol Linking Table
{sym_table}
// Algorithm ID List
{alg_list}
// Firmware Data
{fw_data_blocks}
{coeff_data}
// Footer
{magic_number_2} // IMG_MAGIC_NUMBER_2
{img_checksum} // IMG_CHECKSUM
};

/** @} */

/**********************************************************************************************************************/

#endif // {part_number_uc}_FW_IMG_H

"""

source_file_template_fw_block_str = """{fw_block_size} // FW_BLOCK_SIZE_{block_index}
{fw_block_addr} // FW_BLOCK_ADDR_{block_index}
{block_bytes}"""

source_file_template_coeff_strs = {
    'include_coeff_0': """// COEFF_DATA_BLOCKS_{coeff_index}
{coeff_block_arrays}"""
}

source_file_template_coeff_block_str = """{coeff_block_size} // COEFF_BLOCK_SIZE_{coeff_index}_{block_index}
{coeff_block_addr} // COEFF_BLOCK_ADDR_{coeff_index}_{block_index}
{block_bytes}"""

#==========================================================================
# CLASSES
#==========================================================================
class fw_img_v1_file:
    def __init__(self, part_number_str, suffix, fw_meta, symbol_table):
        self.template_str = header_file_template_str
        self.includes_coeff = False
        self.output_str = ''
        self.terms = dict()
        self.terms['part_uc'] = part_number_str.upper()
        self.terms['part_lc'] = part_number_str.lower()
        self.terms['part_number_lc'] = (part_number_str + suffix).lower()
        self.terms['part_number_uc'] = (part_number_str + suffix).upper()
        self.terms['total_fw_blocks'] = 0
        self.terms['algorithm_defines'] = ''
        self.terms['control_defines'] = ''
        self.terms['include_coeff_0'] = ''
        self.terms['fw_block_arrays'] = ''
        self.terms['coeff_block_arrays'] = []
        self.terms['fw_id'] = fw_meta['fw_id']
        self.terms['fw_rev'] = fw_meta['fw_rev']
        self.terms['metadata_text'] = ' *\n'
        self.total_coeff_blocks = []
        self.algorithms = dict()
        self.algorithm_controls = dict()
        self.uint8_per_line = 24
        self.img_size = 0
        self.sym_table_file = symbol_table
        if self.sym_table_file is None:
            tmp = open("../" + self.terms['part_lc'] + "_sym.h", 'r')
        else:
            tmp = open(self.sym_table_file, 'r')
        self.sym_header = tmp.read()
        tmp.close()

    def create_block_string(self, data_bytes):
        temp_data_str = ''
        byte_count = 0
        for data_byte in data_bytes:
            temp_byte_str = "0x" + "{0:0{1}X}".format(data_byte, 2) + ","
            byte_count = byte_count + 1
            if (((byte_count % self.uint8_per_line) == 0) and (byte_count < len(data_bytes))):
                temp_byte_str = temp_byte_str + "\n"
            temp_data_str = temp_data_str + temp_byte_str

        self.img_size += byte_count
        return temp_data_str

    def int_to_bytes(self, val):
        return self.create_block_string(val.to_bytes(4, byteorder='little'))

    def add_control(self, algorithm_name, algorithm_id, control_name, address):
        if self.algorithms.get(algorithm_name, None) is None:
            self.algorithms[algorithm_name] = algorithm_id
            self.algorithm_controls[algorithm_name] = []
        self.algorithm_controls[algorithm_name].append((control_name, address))

    def add_fw_block(self, address, data_bytes):
        # Create string for block data
        temp_str = source_file_template_fw_block_str.replace('{block_index}', str(self.terms['total_fw_blocks']))
        temp_str = temp_str.replace('{fw_block_size}', self.int_to_bytes(len(data_bytes)))
        temp_str = temp_str.replace('{fw_block_addr}', self.int_to_bytes(address))
        temp_str = temp_str.replace('{block_bytes}', self.create_block_string(data_bytes))

        self.terms['fw_block_arrays'] = self.terms['fw_block_arrays'] + temp_str + '\n'

        self.terms['total_fw_blocks'] = self.terms['total_fw_blocks'] + 1

    def add_coeff_block(self, index, address, data_bytes):
        self.includes_coeff = True

        # Create list elements if they do not exist
        if len(self.terms['coeff_block_arrays']) < (index + 1):
            self.terms['coeff_block_arrays'].append('')
        if len(self.total_coeff_blocks) < (index + 1):
            self.total_coeff_blocks.append(0)

        # Create string for block data
        temp_str = source_file_template_coeff_block_str.replace('{block_index}', str(self.total_coeff_blocks[index]))
        temp_str = temp_str.replace('{coeff_block_size}', self.int_to_bytes(len(data_bytes)))
        temp_str = temp_str.replace('{coeff_block_addr}', self.int_to_bytes(address))
        temp_str = temp_str.replace('{block_bytes}', self.create_block_string(data_bytes))
        temp_str = temp_str.replace('{coeff_index}', str(index))

        self.terms['coeff_block_arrays'][index] = self.terms['coeff_block_arrays'][index] + temp_str + '\n'

        self.total_coeff_blocks[index] = self.total_coeff_blocks[index] + 1
        
    def add_metadata_text_line(self, line):
        self.terms['metadata_text'] = self.terms['metadata_text'] + ' * ' + line + '\n'
        return

    def find_symbol_id(self, alg_name, sym_name):
        matches = re.findall(r"^#define " + self.terms['part_uc'] + "_SYM_" + alg_name.upper() + "_" + sym_name.upper() + " .*", self.sym_header, re.M)
        if len(matches) == 1:
            return int(re.sub(r"\).*$", "", re.sub(r"^.*\(", "", matches[0])), base=16)
        return 0

    def __str__(self):
        output_str = self.template_str

        # Update firmware metadata
        output_str = output_str.replace('{magic_number_1}', self.int_to_bytes(0x54b998ff))
        output_str = output_str.replace('{img_format_rev}', self.int_to_bytes(0x1))

        output_str = output_str.replace('{alg_list_size}', self.int_to_bytes(len(self.algorithms)))
        control_count = 0
        if self.algorithms:
            temp_alg_str = ""
            temp_ctl_str = ""
            for alg_name, alg_id in self.algorithms.items():
                temp_alg_str = temp_alg_str + self.int_to_bytes(alg_id) + " // " + alg_name + "\n"

                for control in self.algorithm_controls[alg_name]:
                    sym_id = self.find_symbol_id(alg_name, control[0])
                    if sym_id:
                        temp_ctl_str = temp_ctl_str + self.int_to_bytes(sym_id) + " // " + control[0] + "\n" \
                            + self.int_to_bytes(control[1]) + " // " + hex(control[1]) + "\n"
                        control_count = control_count + 1

            output_str = output_str.replace('{sym_table_size}', self.int_to_bytes(control_count))

            self.terms['algorithm_defines'] = temp_alg_str
            output_str = output_str.replace('{alg_list}\n', self.terms['algorithm_defines'])
            self.terms['control_defines'] = temp_ctl_str
            output_str = output_str.replace('{sym_table}\n', self.terms['control_defines'])
        else:
            output_str = output_str.replace('{alg_list}\n', '')
            output_str = output_str.replace('{sym_table}\n', '')

        output_str = output_str.replace('{fw_id}', self.int_to_bytes(self.terms['fw_id']))
        output_str = output_str.replace('{fw_ver}', self.int_to_bytes(self.terms['fw_rev']))

        output_str = output_str.replace('{fw_data_blocks}', self.terms['fw_block_arrays'])

        blocks = self.terms['total_fw_blocks']

        if self.includes_coeff:
            temp_str = ''
            for i in range(0, len(self.terms['coeff_block_arrays'])):
                temp_str = temp_str + source_file_template_coeff_strs['include_coeff_0'].replace('{coeff_index}', str(i))
                blocks = blocks + self.total_coeff_blocks[i]
                temp_str = temp_str.replace('{coeff_block_arrays}', self.terms['coeff_block_arrays'][i])
                temp_str = temp_str + '\n'
            output_str = output_str.replace('{coeff_data}\n', temp_str)
        else:
            output_str = output_str.replace('{coeff_data}\n', '')

        output_str = output_str.replace('{data_block_count}', self.int_to_bytes(blocks))

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])

        output_str = output_str.replace('{magic_number_2}', self.int_to_bytes(0x936be2a6))

        output_str = output_str.replace('{img_checksum}', self.int_to_bytes(0xf0f00f0f))
        # need to add 4 to include the img_size field itself.
        output_str = output_str.replace('{img_size}', self.int_to_bytes(self.img_size + 4))
        
        output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])
        
        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

