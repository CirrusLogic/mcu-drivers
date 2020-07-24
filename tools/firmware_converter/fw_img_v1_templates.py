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
import os
from firmware_exporter import firmware_exporter
from fw_img_v1 import fw_img_v1

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

symbol_id_header_file_template_str = """
/**
 * @file {part_number_lc}_sym.h
 *
 * @brief Master table of known firmware symbols for the {part_number_uc} Driver module
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

#ifndef {part_number_uc}_SYM_H
#define {part_number_uc}_SYM_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * LITERALS & CONSTANTS
 **********************************************************************************************************************/

/**
 * @defgroup {part_number_uc}_SYM_
 * @brief Single source of truth for firmware symbols known to the driver.
 *
 * @{
 */
{symbol_id_define_list}

/** @} */

/**********************************************************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // {part_number_uc}_SYM_H

"""

#==========================================================================
# CLASSES
#==========================================================================
class fw_img_v1_file(firmware_exporter):
    def __init__(self, attributes):
        firmware_exporter.__init__(self, attributes)
        self.template_str = header_file_template_str
        self.includes_coeff = False
        self.output_str = ''
        self.terms = dict()
        self.terms['part_uc'] = self.attributes['part_number_str'].upper()
        self.terms['part_lc'] = self.attributes['part_number_str'].lower()
        self.terms['part_number_lc'] = (self.attributes['part_number_str'] + self.attributes['suffix']).lower()
        self.terms['part_number_uc'] = (self.attributes['part_number_str'] + self.attributes['suffix']).upper()
        self.terms['total_fw_blocks'] = 0
        self.terms['algorithm_defines'] = ''
        self.terms['control_defines'] = ''
        self.terms['include_coeff_0'] = ''
        self.terms['fw_block_arrays'] = ''
        self.terms['coeff_block_arrays'] = []
        self.terms['fw_id'] = self.attributes['fw_meta']['fw_id']
        self.terms['fw_rev'] = self.attributes['fw_meta']['fw_rev']
        self.terms['metadata_text'] = ' *\n'
        self.total_coeff_blocks = []
        self.fw_data_block_list = []
        self.coeff_data_block_list = []
        self.algorithms = dict()
        self.algorithm_controls = dict()
        self.uint8_per_line = 24
        self.img_size = 0
        self.sym_table_file = self.attributes['symbol_table']
        
        return

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

    def update_block_info(self, fw_block_total, coeff_block_totals): pass

    def add_control(self, algorithm_name, algorithm_id, control_name, address):
        if (not self.attributes['wmdr_only']):
            if self.algorithms.get(algorithm_name, None) is None:
                self.algorithms[algorithm_name] = algorithm_id
                self.algorithm_controls[algorithm_name] = []
            self.algorithm_controls[algorithm_name].append((control_name, address))

    def add_fw_block(self, address, data_bytes):
        if (not self.attributes['wmdr_only']):
            # Create entry into data block list
            self.fw_data_block_list.append((len(data_bytes), address, data_bytes))
            
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
            
        # Create entry into data block list
        if (len(self.coeff_data_block_list) < (index + 1)):
            self.coeff_data_block_list.append([])
            
        self.coeff_data_block_list[index].append((len(data_bytes), address, data_bytes))

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

    def find_symbol_id(self, sym_name):
        matches = re.findall(r"^#define " + self.terms['part_uc'] + "_SYM_" + sym_name.upper() + " .*", self.sym_header, re.M)
        if len(matches) == 1:
            return int(re.sub(r"\).*$", "", re.sub(r"^.*\(", "", matches[0])), base=16)
        return 0
        
    def generate_symbol_header(self):
        self.sym_table_file = self.terms['part_lc'] + '_sym.h'
        f = open(self.sym_table_file, 'w')
        
        output_str = symbol_id_header_file_template_str
        
        # Iterate through algorithms and respective lists of controls and generate symbol id define list
        temp_str = ''
        # the count for control ids starts at 1 - the value 0 is a special value used by parsing library
        control_id = 1
        
        if self.algorithms:
            # First get all control strings and calculate the longest name
            longest_str_len = 0
            for alg_name, alg_id in self.algorithms.items():
                for control in self.algorithm_controls[alg_name]:
                    temp_ctl_str = "#define " + self.terms['part_uc'] + "_SYM_" + control[0]
                    if (len(temp_ctl_str) > longest_str_len):
                        longest_str_len = len(temp_ctl_str)
                        
            # Round up to nearest tab/4 spaces
            longest_str_len += (4 - (longest_str_len % 4))
            
            # Create string of all symbol id defines
            for alg_name, alg_id in self.algorithms.items():
                temp_str += "// " + alg_name + "\n"
                for control in self.algorithm_controls[alg_name]:
                    temp_ctl_str = "#define " + self.terms['part_uc'] + "_SYM_" + control[0].upper()
                    temp_ctl_str += ' ' * (longest_str_len - len(temp_ctl_str))
                    temp_ctl_str += '(0x' + str(control_id) + ')\n'
                    
                    temp_str += temp_ctl_str
                    
                    control_id += 1
        
        # Replace symbol id define list
        output_str = output_str.replace('{symbol_id_define_list}\n', temp_str)
        
        # Replace part number strings
        output_str = output_str.replace('{part_number_lc}', self.terms['part_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_uc'])        
        output_str = output_str.replace('\n\n\n', '\n\n')
        
        self.sym_header = output_str
        
        f.write(self.sym_header)
        f.close()
        
        print("Generated " + self.sym_table_file)
        
        return
        
    def to_byte_array(self):
        word_list = []
        word_list.append(0x54b998ff)
        word_list.append(0x1)
        word_list.append(0)
        
        control_count = 0
        temp_alg_words = []
        temp_ctl_words = []
        if self.algorithms:
            for alg_name, alg_id in self.algorithms.items():
                temp_alg_words.append(alg_id)

                for control in self.algorithm_controls[alg_name]:
                    sym_id = self.find_symbol_id(control[0])
                    if sym_id:
                        temp_ctl_words.append(sym_id)
                        temp_ctl_words.append(control[1])

                        control_count = control_count + 1
            
            word_list.append(control_count)
            word_list.append(len(self.algorithms))
        else:
            word_list.append(0)
            word_list.append(0)
            
        word_list.append(self.terms['fw_id'])
        word_list.append(self.terms['fw_rev'])

        total_data_blocks = len(self.fw_data_block_list)
        data_block_words = []
        for f in self.fw_data_block_list:
            data_block_words.append(f[0])
            data_block_words.append(f[1])
            
            for i in range(0, len(f[2]), 4):
                temp_word = f[2][i]
                temp_word = temp_word | (f[2][i + 1] << 8)
                temp_word = temp_word | (f[2][i + 2] << 16)
                temp_word = temp_word | (f[2][i + 3] << 24)
                data_block_words.append(temp_word)
                
        for coeff_blocks in self.coeff_data_block_list:
        
            total_data_blocks = total_data_blocks + len(coeff_blocks)
            
            for coeff_block in coeff_blocks:
                data_block_words.append(coeff_block[0])
                data_block_words.append(coeff_block[1])
                
                for i in range(0,  len(coeff_block[2]), 4):
                    temp_word = coeff_block[2][i]
                    temp_word = temp_word | (coeff_block[2][i + 1] << 8)
                    temp_word = temp_word | (coeff_block[2][i + 2] << 16)
                    temp_word = temp_word | (coeff_block[2][i + 3] << 24)
                    data_block_words.append(temp_word)

        word_list.append(total_data_blocks)
        
        word_list.extend(temp_ctl_words)
        word_list.extend(temp_alg_words)
        
        word_list.extend(data_block_words)

        word_list.append(0x936be2a6)
        word_list.append(0xf0f00f0f)
        
        # Compute image size
        word_list[2] = len(word_list) * 4
        
        # Convert to byte array
        byte_list = []
        for w in word_list:
            byte_list.extend(w.to_bytes(4, byteorder="little", signed=False))
        
        return bytearray(byte_list)

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
                    sym_id = self.find_symbol_id(control[0])
                    if sym_id:
                        temp_ctl_str = temp_ctl_str + self.int_to_bytes(sym_id) + " // " + control[0].upper() + "\n" \
                            + self.int_to_bytes(control[1]) + " // " + hex(control[1]) + "\n"
                        control_count = control_count + 1

            output_str = output_str.replace('{sym_table_size}', self.int_to_bytes(control_count))

            self.terms['algorithm_defines'] = temp_alg_str
            output_str = output_str.replace('{alg_list}\n', self.terms['algorithm_defines'])
            self.terms['control_defines'] = temp_ctl_str
            output_str = output_str.replace('{sym_table}\n', self.terms['control_defines'])
        else:
            output_str = output_str.replace('{sym_table_size}', self.int_to_bytes(0))
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
        
    def to_file(self):
        temp_filename = self.attributes['part_number_str'] + self.attributes['suffix'] + "_fw_img"
        
        # Open or generate the symbol id header
        if self.sym_table_file is None:
            self.generate_symbol_header()
        tmp = open(self.sym_table_file, 'r')
        self.sym_header = tmp.read()
        tmp.close()
        
        if (not self.attributes['binary_output']):
            temp_filename = temp_filename + ".h"
            
            f = open(temp_filename, 'w')
            f.write(self.__str__())
            f.close()
        else:
            temp_filename = temp_filename + ".bin"
            f = open(temp_filename, "wb")
            f.write(self.to_byte_array())
            f.close()
        
        return "Exported to " + temp_filename

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

