#==========================================================================
# (c) 2020-2021 Cirrus Logic, Inc.
#--------------------------------------------------------------------------
# Project : Templates for C Source and Header files
# File    : fw_img_v1_templates.py
#--------------------------------------------------------------------------
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
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
from collections import OrderedDict
import time

#==========================================================================
# CONSTANTS/GLOBALS
#==========================================================================
header_file_template_str = """/**
 * @file {part_number_lc}_fw_img.h
 *
 * @brief {part_number_uc} FW IMG Header File
 *
 * @copyright
 * Copyright (c) Cirrus Logic """ + time.strftime("%Y") + """ All Rights Reserved, http://www.cirrus.com/
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

#ifndef {part_number_uc}_FW_IMG_H
#define {part_number_uc}_FW_IMG_H

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include <stdint.h>

/***********************************************************************************************************************
 * FW_IMG
 **********************************************************************************************************************/

extern const uint8_t {part_number_lc}_fw_img[];

/**********************************************************************************************************************/

#endif // {part_number_uc}_FW_IMG_H

"""

source_file_template_str = """/**
 * @file {part_number_lc}_fw_img.c
 *
 * @brief {part_number_uc} FW IMG Source File
 *
 * @copyright
 * Copyright (c) Cirrus Logic """ + time.strftime("%Y") + """ All Rights Reserved, http://www.cirrus.com/
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
{metadata_text} *
 */

/***********************************************************************************************************************
 * INCLUDES
 **********************************************************************************************************************/
#include "{part_number_lc}_fw_img.h"

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
{max_block_size}
{bin_ver}
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
 * Copyright (c) Cirrus Logic """ + time.strftime("%Y") + """ All Rights Reserved, http://www.cirrus.com/
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
{metadata_text} *
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
 * @defgroup {part_number_uc}_ALGORITHMS
 * @brief Defines indicating presence of HALO Core Algorithms
 *
 * @{
 */
{algorithm_defines}
/** @} */

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
    def __init__(self, attributes, version):
        firmware_exporter.__init__(self, attributes)
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
        self.terms['metadata_text'] = ' *\n'
        self.terms['algorithm_defines'] = ''
        self.total_coeff_blocks = []
        self.fw_data_block_list = []
        self.coeff_data_block_list = []
        self.uint8_per_line = 24
        self.blocks = []
        self.sym_id_input = self.attributes['symbol_id_input']
        self.sym_id_output = self.attributes['symbol_id_output']

        self.terms['magic_num1'] = 0x54b998ff
        self.terms['version'] = version
        self.terms['img_size'] = 0
        self.terms['sym_table_size'] = 0
        self.terms['alg_list_size'] = 0
        self.terms['fw_id'] = self.attributes['fw_meta']['fw_id']
        self.terms['fw_rev'] = self.attributes['fw_meta']['fw_rev']
        self.terms['bin_ver'] = self.attributes['fw_img_version']
        self.terms['data_blocks'] = 0
        self.terms['max_block_size'] = self.attributes['max_block_size']

        self.algorithms = OrderedDict()

        self.algorithm_controls = OrderedDict()

        self.terms['fw_block_arrays'] = ''
        self.terms['coeff_block_arrays'] = []

        self.terms['magic_num2'] = 0x936be2a6
        self.terms['checksum'] = 0

        self.c0 = 0x0
        self.c1 = 0x0

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

        self.terms['img_size'] += byte_count
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
        matches = re.findall(r"^#define " + self.terms['part_number_uc'] + "_SYM_" + sym_name.upper() + " .*", self.sym_header, re.M)
        if len(matches) == 1:
            return int(re.sub(r"\).*$", "", re.sub(r"^.*\(", "", matches[0])), base=16)
        return 0

    def generate_symbol_header(self, symbol_id_list):
        output_str = symbol_id_header_file_template_str

        # Iterate through algorithms and respective lists of controls and generate symbol id define list
        temp_str = ''
        # the count for control ids starts at 1 - the value 0 is a special value used by parsing library
        control_id = 1

        alg_list_str = ''
        if self.algorithms:
            # First get all control strings and calculate the longest name
            longest_str_len = 0
            for alg_name, alg_id in self.algorithms.items():
                for control in self.algorithm_controls[alg_name]:
                    temp_ctl_str = self.terms['part_number_uc'] + "_SYM_" + control[0].upper()

                    if ((symbol_id_list is None) or
                        ((symbol_id_list is not None) and (temp_ctl_str in symbol_id_list))):
                        temp_ctl_str = "#define " + temp_ctl_str
                        if (len(temp_ctl_str) > longest_str_len):
                            longest_str_len = len(temp_ctl_str)

            # Round up to nearest tab/4 spaces
            longest_str_len += (4 - (longest_str_len % 4))

            # Create string of all symbol id defines
            for alg_name, alg_id in self.algorithms.items():
                temp_alg_str = ""
                for control in self.algorithm_controls[alg_name]:
                    temp_ctl_str = self.terms['part_number_uc'] + "_SYM_" + control[0].upper()
                    if ((symbol_id_list is None) or
                        ((symbol_id_list is not None) and (temp_ctl_str in symbol_id_list))):
                        temp_ctl_str = "#define " + temp_ctl_str
                        temp_ctl_str += ' ' * (longest_str_len - len(temp_ctl_str))
                        temp_ctl_str += '(' + hex(control_id) + ')\n'

                        temp_alg_str += temp_ctl_str

                        control_id += 1
                if (temp_alg_str != ""):
                    alg_list_str += "#define {part_number_uc}_ALGORITHM_" + alg_name.upper() + "\n"
                    temp_str += "// " + alg_name + "\n" + temp_alg_str

        # Replace algorithm list
        output_str = output_str.replace('{algorithm_defines}\n', alg_list_str)        
        
        # Replace symbol id define list
        output_str = output_str.replace('{symbol_id_define_list}\n', temp_str)

        # Replace metadata
        output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])

        # Replace part number strings
        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])
        output_str = output_str.replace('\n\n\n', '\n\n')

        if (self.sym_id_input is None):
            if (self.sym_id_output is None):
                self.sym_id_output = self.terms['part_number_lc'] + '_sym.h'
            f = open(self.sym_id_output, 'w')
            f.write(output_str)
            f.close()

            print("Generated Symbol ID Header " + self.sym_id_output)

        return output_str

    def extract_symbol_ids(self, filename):
        symbol_id_list = []
        f = open(filename, 'r')
        lines = f.readlines()
        for line in lines:
            words = line.split()
            if (len(words) == 3):
                if ((words[0] == '#define') and ('_SYM_' in words[1]) and (words[1] not in symbol_id_list)):
                    symbol_id_list.append(words[1])
        f.close()

        return symbol_id_list

    def fletch32(self, word):
        modval = pow(2, 16) - 1
        bytes = word.to_bytes(4, byteorder='little')
        self.c0 = (self.c0 + (bytes[0] + (bytes[1] << 8))) % modval
        self.c1 = (self.c1 + self.c0) % modval
        self.c0 = (self.c0 + (bytes[2] + (bytes[3] << 8))) % modval
        self.c1 = (self.c1 + self.c0) % modval

    def calc_checksum(self):
        self.fletch32(self.terms['magic_num1'])
        self.fletch32(self.terms['version'])
        self.fletch32(self.terms['img_size'])
        self.fletch32(self.terms['sym_table_size'])
        self.fletch32(self.terms['alg_list_size'])
        self.fletch32(self.terms['fw_id'])
        self.fletch32(self.terms['fw_rev'])
        self.fletch32(self.terms['data_blocks'])
        if self.terms['version'] != 1:
            self.fletch32(self.terms['max_block_size'])
            self.fletch32(self.terms['bin_ver'])

        if self.algorithms:
            for alg_name, alg_id in self.algorithms.items():
                for control in self.algorithm_controls[alg_name]:
                    sym_id = self.find_symbol_id(control[0])
                    if sym_id:
                        self.fletch32(sym_id)
                        self.fletch32(control[1])

        if self.algorithms:
            for alg_name, alg_id in self.algorithms.items():
                self.fletch32(alg_id)

        for f in self.fw_data_block_list:
            self.fletch32(f[0])
            self.fletch32(f[1])
            for i in range(0, len(f[2]), 4):
                temp_word = f[2][i]
                temp_word = temp_word | (f[2][i + 1] << 8)
                temp_word = temp_word | (f[2][i + 2] << 16)
                temp_word = temp_word | (f[2][i + 3] << 24)
                self.fletch32(temp_word)

        for coeff_blocks in self.coeff_data_block_list:
            for coeff_block in coeff_blocks:
                self.fletch32(coeff_block[0])
                self.fletch32(coeff_block[1])
                for i in range(0,  len(coeff_block[2]), 4):
                    temp_word = coeff_block[2][i]
                    temp_word = temp_word | (coeff_block[2][i + 1] << 8)
                    temp_word = temp_word | (coeff_block[2][i + 2] << 16)
                    temp_word = temp_word | (coeff_block[2][i + 3] << 24)
                    self.fletch32(temp_word)

        self.fletch32(self.terms['magic_num2'])

        return self.c0 + (self.c1 << 16)


    def to_byte_array(self):
        word_list = []
        word_list.append(self.terms['magic_num1'])
        word_list.append(self.terms['version'])
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

            self.terms['sym_table_size'] = control_count
            word_list.append(self.terms['sym_table_size'])

            self.terms['alg_list_size'] = len(self.algorithms)
            word_list.append(self.terms['alg_list_size'])
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

        self.terms['data_blocks'] = total_data_blocks
        word_list.append(self.terms['data_blocks'])
        if self.terms['version'] != 1:
            word_list.append(self.terms['max_block_size'])

        word_list.extend(temp_ctl_words)
        word_list.extend(temp_alg_words)

        word_list.extend(data_block_words)

        word_list.append(self.terms['magic_num2'])
        if self.terms['version'] == 1:
            word_list.append(0xf0f00f0f)
        else:
            word_list.append(self.calc_checksum())

        # Compute image size
        self.terms['img_size'] = len(word_list) * 4
        word_list[2] = self.terms['img_size']

        # Convert to byte array
        byte_list = []
        for w in word_list:
            byte_list.extend(w.to_bytes(4, byteorder="little", signed=False))

        return bytearray(byte_list)

    def __str__(self):
        return ''

    def create_source_file_text(self):
        output_str = source_file_template_str

        # Update firmware metadata
        output_str = output_str.replace('{magic_number_1}', self.int_to_bytes(self.terms['magic_num1']))
        output_str = output_str.replace('{img_format_rev}', self.int_to_bytes(self.terms['version']))

        self.terms['alg_list_size'] = len(self.algorithms)
        output_str = output_str.replace('{alg_list_size}', self.int_to_bytes(self.terms['alg_list_size']))
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

            self.terms['sym_table_size'] = control_count

            self.terms['algorithm_defines'] = temp_alg_str
            output_str = output_str.replace('{alg_list}\n', self.terms['algorithm_defines'])
            self.terms['control_defines'] = temp_ctl_str
            output_str = output_str.replace('{sym_table}\n', self.terms['control_defines'])
        else:
            self.terms['sym_table_size'] = 0

            output_str = output_str.replace('{alg_list}\n', '')
            output_str = output_str.replace('{sym_table}\n', '')

        output_str = output_str.replace('{sym_table_size}', self.int_to_bytes(self.terms['sym_table_size']))

        output_str = output_str.replace('{fw_id}', self.int_to_bytes(self.terms['fw_id']))
        output_str = output_str.replace('{fw_ver}', self.int_to_bytes(self.terms['fw_rev']))

        if self.terms['version'] == 1:
            output_str = output_str.replace('{bin_ver}', "")
        else:
            output_str = output_str.replace('{bin_ver}', self.int_to_bytes(self.terms['bin_ver']) + " // FW_IMG_VERSION")

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

        self.terms['data_blocks'] = blocks
        output_str = output_str.replace('{data_block_count}', self.int_to_bytes(self.terms['data_blocks']))
        if self.terms['version'] == 1:
            output_str = output_str.replace('{max_block_size}', "")
        else:
            output_str = output_str.replace('{max_block_size}', self.int_to_bytes(self.terms['max_block_size']) + " // MAX_BLOCK_SIZE")

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])

        output_str = output_str.replace('{magic_number_2}', self.int_to_bytes(self.terms['magic_num2']))

        # need to add 8 to include the checksum and the img_size field itself.
        self.terms['img_size'] = self.terms['img_size'] + 8
        output_str = output_str.replace('{img_size}', self.int_to_bytes(self.terms['img_size']))
        # int_to_bytes increments img_size, so subtract 4 again to get us back to where we should be
        self.terms['img_size'] = self.terms['img_size'] - 4

        if self.terms['version'] == 1:
            output_str = output_str.replace('{img_checksum}', self.int_to_bytes(0xf0f00f0f))
        else:
            output_str = output_str.replace('{img_checksum}', self.int_to_bytes(self.calc_checksum()))

        output_str = output_str.replace('{metadata_text}', self.terms['metadata_text'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

    def create_header_file_text(self):
        output_str = header_file_template_str

        output_str = output_str.replace('{part_number_lc}', self.terms['part_number_lc'])
        output_str = output_str.replace('{part_number_uc}', self.terms['part_number_uc'])

        output_str = output_str.replace('\n\n\n', '\n\n')
        return output_str

    def to_file(self):
        results_str = 'Exported to files:\n'

        temp_filename = self.attributes['part_number_str'] + self.attributes['suffix'] + "_fw_img"

        # Open or generate the symbol id header
        symbol_id_list = None
        if (self.sym_id_input is not None):
            symbol_id_list = self.extract_symbol_ids(self.sym_id_input)
            tmp = open(self.sym_id_input, 'r')
            self.sym_header = tmp.read()
            tmp.close()
        else:
            self.sym_header = self.generate_symbol_header(symbol_id_list)

        if (not self.attributes['binary_output']):
            self.template_str = header_file_template_str
            f = open(temp_filename + ".h", 'w')
            f.write(self.create_header_file_text())
            f.close()
            results_str = results_str + temp_filename + '.h\n'

            self.template_str = source_file_template_str
            f = open(temp_filename + ".c", 'w')
            f.write(self.create_source_file_text())
            f.close()
            results_str = results_str + temp_filename + '.c\n'

        else:
            temp_filename = temp_filename + ".bin"
            f = open(temp_filename, "wb")
            f.write(self.to_byte_array())
            f.close()
            results_str = results_str + temp_filename + '\n'

        return results_str

#==========================================================================
# HELPER FUNCTIONS
#==========================================================================

#==========================================================================
# MAIN PROGRAM
#==========================================================================

